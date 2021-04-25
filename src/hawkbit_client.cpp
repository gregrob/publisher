#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>

#include "hawkbit_client.h"

#include "utils.h"
#include "debug.h"
#include "nvm_cfg.h"
#include "reset_ctrl.h"
#include "wifi.h"

// Two step process as defined in - https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html
// Stops bricking upon crash / power outage during update
#define ATOMIC_FS_UPDATE

// Set the module call interval
#define MODULE_CALL_INTERVAL                HAWKBIT_CLIENT_CYCLIC_RATE

// TODO: Fix this poll rate, way too fast
// Time to stay in idle state in S
#define HAWKBIT_CLIENT_IDLE_TIME_RST_S      (1)

// Size of the JSON document
#define HAWKBIT_CLIENT_JSON_DOCUMENT_SIZE   (1024)

// Threshold % for sending programming status update
#define HAWKBIT_CLIENT_THRESOLD_STATUS_SEND (6)


// HTTP REST API types
enum hawkbitClientHttpRestTypes {
    hawkbitClientHttpGET,
    hawkbitClientHttpPOST,
    hawkbitClientHttpPUT,

    hawkbitClientHttpRestTypesTotal
};


// HTTP REST API type names
static const char * hawkbitClientHttpRestTypeNames[] {
    "hawkbitClientHttpGET",
    "hawkbitClientHttpPOST",
    "hawkbitClientHttpPUT"
};

// Hawkbit state machine state names
static const char * hawkbitClientStateNames[] {
    "stmHawkbitRestart",
    "stmHawkbitIdle",
    "stmHawkbitPoll",
    "stmHawkbitCancel",
    "stmHawkbitCancelAck",
    "stmHawkbitDeploy",
    "stmHawkbitDeployAck",
    "stmHawkbitDeploySuccessReboot",
    "stmHawkbitConfig"
};

// Module name for debug messages
const char* hawkbitClientModuleName = "hawkbitClient";

// Hawkbit tennant ID
const char* hawkbitClientTennantID = "DEFAULT";

// Hawkbit server
const char* hawkbitClientServerURL = "svr.max.lan:9090";

// Hawkbit gateway security token
const char* hawkbitClientGatewayToken = "2401a1f9f311a30de365b861a35bc620";


// JSON static document
static StaticJsonDocument<HAWKBIT_CLIENT_JSON_DOCUMENT_SIZE> doc; 

// Current state
static hawkbitClientStm hawkbitClientCurrentState;

// Hawkbit controller identification
String hawkbitClientControllerID;

// Hawkbit base server API path
String hawkbitClientServerPathBase;


/**
    Access a HTTP REST API.
        
    @param[in]     serverPath the full server / API path.
    @param[in]     docPtr pointer to the JSON buffer.
    @param[in]     apiType API type (GET/POST/PUT).
    @return        bool as success / failure.
*/
static bool hawkbitClientHttp(const String serverPath, JsonDocument * const docPtr, const hawkbitClientHttpRestTypes apiType) {

    // Debug message
    String debugMessage;

    // Wifi and http client (wifi must be defined first)
    WiFiClient wifi;
    HTTPClient http;

    // Payloads for Tx and Rx (raw HTTP message after JSON)
    String txPayload;
    String rxPayload;
    
    // Response codes for HTTP and JSON requests
    int httpResponseCode = HTTP_CODE_OK;
    DeserializationError jsonResponse = DeserializationError::Ok;

    // Function return value
    bool returnValue = false;

    // Start the HTTP request and set the authentication
    http.begin(wifi, serverPath.c_str());
    http.addHeader("Authorization", String() + "GatewayToken " + hawkbitClientGatewayToken);

    // Make sure apiType is within ranage
    if (apiType < hawkbitClientHttpRestTypesTotal) {

        switch(apiType) {

            case hawkbitClientHttpGET:
                http.addHeader("Accept", "application/hal+json");
                httpResponseCode = http.GET();
                
                if (httpResponseCode == HTTP_CODE_OK) {
                    rxPayload = http.getString();

                    docPtr->clear();
                    jsonResponse = deserializeJson(*docPtr, rxPayload);
                }
                break;

            case hawkbitClientHttpPOST:
                http.addHeader("Content-Type", "application/json;charset=UTF-8");
                (void) serializeJson(*docPtr, txPayload);
                httpResponseCode = http.POST(txPayload);
                break;

            case hawkbitClientHttpPUT:
                http.addHeader("Content-Type", "application/json;charset=UTF-8");
                (void) serializeJson(*docPtr, txPayload);
                httpResponseCode = http.PUT(txPayload);
                break;

            // The range check should prevent these requests from ever being requested
            case hawkbitClientHttpRestTypesTotal:
            default:
                break;
        }

        // Construct a debug string for the http operation
        debugMessage = String() + hawkbitClientModuleName + ": " + hawkbitClientHttpRestTypeNames[apiType] + " " + serverPath + ", returned " + httpResponseCode;

        // Positive HTTP response
        if (httpResponseCode == HTTP_CODE_OK) {
            debugLog(&debugMessage, info);

            // JSON deserialisation problem
            if (jsonResponse) {
                debugMessage = String() + hawkbitClientModuleName + ": JSON deserialisation failed with code " + jsonResponse.f_str();
                debugLog(&debugMessage, warning);

                returnValue = false;
            }

            else {
                returnValue = true;
            }
        }

        else {
            debugLog(&debugMessage, error);

            returnValue = false;
        }
    }

    http.end();

    return(returnValue);
}


/**
    Update status for an update action.
    Call to set Action ID and JSON doc pointer before calling in callback.
        
    @param[in]     progress the current progress of total.
    @param[in]     total the total to program.
    @param[in]     newActionID the action ID being processed (leave "" if called from callback).
    @param[in]     newDocPtr pointer to the JSON doc (leave NULL if called from the callback).
*/
void hawkbitClientUpdateProgress(unsigned int progress, unsigned int total, const String newActionID, JsonDocument * const newDocPtr) {
            
    // Debug strings
    String debugMessage;
    char percentageCompleteString[STRNLEN_INT(100%) + 1];    
    
    // Store the action ID (only gets updated while something is passed)
    static String actionID;

    // Store the pointer to the JSON doc (only gets updated while something is passed)
    static JsonDocument * docPtr;

    // Threshold where a new http message is sent (so it doesnt get sent everytime this is called)
    static unsigned int percentageCompleteThresholdSend = 0;

    // Percentage of the download complete 
    unsigned int percentageComplete;

    // Update pointer if it is not NULL
    if(newDocPtr != NULL) {
        docPtr = newDocPtr;
    }

    // Update action ID and reset the trigger to send threshold if it is not empty
    if (!newActionID.isEmpty()) {
        actionID = newActionID;
        percentageCompleteThresholdSend = 0;
    }

    // Calculate the % complete
    if ((progress == 0) || (total == 0)) {
        percentageComplete = 0;
    }
    else {
        percentageComplete = (progress / (total / 100));
    }

    // Threshold to send progress response met
    if (percentageComplete >= percentageCompleteThresholdSend) {
        sprintf(percentageCompleteString, "%u%%", percentageComplete);

        debugMessage = String() + hawkbitClientModuleName + ": Updating software " + percentageCompleteString + ".";
        debugLog(&debugMessage, info);

        // Prepare the JSON response and send
        docPtr->clear();
        (*docPtr)["id"] = actionID;
        (*docPtr)["status"]["execution"] = "proceeding";
        (*docPtr)["status"]["result"]["finished"] = "none"; 
        (*docPtr)["status"]["details"][0] = String() + "Progress " + percentageCompleteString;
        (*docPtr)["status"]["details"][1] = String() + progress + " of " + total + "bytes";

        hawkbitClientHttp(hawkbitClientServerPathBase + "/deploymentBase/" + actionID + "/feedback", docPtr, hawkbitClientHttpPOST);

        // At the start and end of update the progress can sit at 0% or 100% respectively
        // To stop 100% being transmitted repetitively first checl that the threshold is under 100% (i.e. approaching 100)
        if (percentageCompleteThresholdSend < 100) {
            
            // Update the next threshold level to send at
            percentageCompleteThresholdSend += HAWKBIT_CLIENT_THRESOLD_STATUS_SEND;
        
            // If the threshold has exceeded 100%, truncate it to 100%
            if (percentageCompleteThresholdSend > 100) {
                percentageCompleteThresholdSend = 100;
            }
        }

        // If we are 100% or more, make the trigger threshold over 100% because it should never get there
        else {
            percentageCompleteThresholdSend = 200;
        }      
    }
}


/**
    Handle an update.
        
    @param[in]     updateImagePath the update image href.
    @param[in]     actionID the action ID being processed.
    @param[in]     newDocPtr pointer to the JSON doc.
    @return        success is an empty string OR string with failure message.
*/
static String hawkbitClientUpdateHandler(const String updateImagePath, const String actionID, JsonDocument * const docPtr) {

    String returnValue;

    // Debug message
    String debugMessage;

    // Wifi client
    WiFiClient wifi;

    // Update status
    HTTPUpdateResult updateStatus;

    // Set up the update progress function
    hawkbitClientUpdateProgress(0, 0, actionID, docPtr);

    // Call back for progress
    ESPhttpUpdate.onProgress([](unsigned int progress, unsigned int total) {
        hawkbitClientUpdateProgress(progress, total, "", NULL);
    });
    
    // Stop the update process resetting after it completes, use the system standard reset
    ESPhttpUpdate.rebootOnUpdate(false);

    // TODO: How to set basic security in download - New versions of arudino lib have void ESP8266HTTPUpdate::setAuthorization(const String &auth)???
    // TODO: Secure update
    // TODO: MD5 hash check

    // Do the update
    updateStatus = ESPhttpUpdate.update(wifi, updateImagePath);

    // Set-up the return message / error info
    switch (updateStatus) {
      case HTTP_UPDATE_FAILED:        
        returnValue = String() + "HTTP_UPDATE_FAILD Error " + ESPhttpUpdate.getLastError() + " (" + ESPhttpUpdate.getLastErrorString().c_str() + ").";
        break;

      case HTTP_UPDATE_NO_UPDATES:
        returnValue = String() + "HTTP_UPDATE_NO_UPDATES.";
        break;

      case HTTP_UPDATE_OK:
        returnValue = "";
        break;
    }

    // If the return value is empty, success
    if (returnValue.isEmpty()) {
        debugMessage = String() + hawkbitClientModuleName + ": Update completed successfully.";
        debugLog(&debugMessage, info);
    }

    // Otherwise error
    else {
        debugMessage = String() + hawkbitClientModuleName + ": Update failed: " + returnValue;
        debugLog(&debugMessage, error);
    }

    return(returnValue);
}


/**
    Hawkbit controller init.
*/
void hawkbitClientInit(void) {
    
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);

    // TODO: Acessing globals here needs to be fixed?
    // TODO: Setup hawkbit server parameters from NVM
    hawkbitClientCurrentState = stmHawkbitRestart;
    hawkbitClientControllerID = String() + getWiFiModuleDetails()->moduleHostName;
    hawkbitClientServerPathBase = String() + "http://" + hawkbitClientServerURL + "/" + hawkbitClientTennantID + "/controller/v1/" + hawkbitClientControllerID;
    doc.clear();
}


// TODO: BIG TIDY UP IN HERE STM (including GLOBALS)
// TODO: Interaction with standard OTA, each need to disable each other (i.e. if OTA is running then return rejection here, if this is running reject OTA)
/**
    Hawkbit controller state machine.
*/
void hawkbitClientStateMachine(void) {
    
    // Debug message
    String debugMessage;

    // State timer
    static uint32_t stateTimer;

    // Next state
    hawkbitClientStm nextState = hawkbitClientCurrentState;

    // Action ID
    static String actionID;

    // Result from the update
    static String updateResult;

    // Strings for hrefs
    static String hrefCancel;
    static String hrefConfig;
    static String hrefDeployment;

    // Handle the state machine
    switch(hawkbitClientCurrentState) {
        
        case(stmHawkbitRestart):
            stateTimer = SECS_TO_CALLS(HAWKBIT_CLIENT_IDLE_TIME_RST_S);
            nextState = stmHawkbitIdle;
            break;
        
        case(stmHawkbitIdle):
            // Timer running
            if(stateTimer > 0) {
                stateTimer--;
            }
                
            // Timer elapsed
            else {
                nextState = stmHawkbitPoll;
            }
            break;
        

        case(stmHawkbitPoll):
            if(!hawkbitClientHttp(hawkbitClientServerPathBase, &doc, hawkbitClientHttpGET)) {
                nextState = stmHawkbitRestart;
            }

            // Success with GET HTTP and JSON decoding
            else {

                hrefCancel = doc["_links"]["cancelAction"]["href"] | "";
                hrefConfig = doc["_links"]["configData"]["href"] | "";
                hrefDeployment = doc["_links"]["deploymentBase"]["href"] | "";

                // Cancel request
                if(hrefCancel.isEmpty() == false) {
                    nextState = stmHawkbitCancel;
                }

                // Config request
                else if(hrefConfig.isEmpty() == false) {
                    nextState = stmHawkbitConfig;
                }

                // Deployment request
                else if(hrefDeployment.isEmpty() == false) {
                    nextState = stmHawkbitDeploy;
                }

                // No request so reset
                else {
                    nextState = stmHawkbitRestart;
                }
            }          
            break;

        case(stmHawkbitCancel):                                      
            // Try to send the GET and if there is a failure restart
            if(!hawkbitClientHttp(hrefCancel, &doc, hawkbitClientHttpGET)) {
                nextState = stmHawkbitRestart;
            }

            // If there is no failure, record the action ID and move to acknowledgement
            else {
                actionID = doc["cancelAction"]["stopId"] | "";
                nextState = stmHawkbitCancelAck;
            }
            break;

        case(stmHawkbitCancelAck):            
            // Prepare the JSON acknowledgement
            doc.clear();
            doc["id"] = actionID;
            doc["status"]["execution"] = "closed";
            doc["status"]["result"]["finished"] = "success";

            // Send the POST and move to restart
            hawkbitClientHttp(hawkbitClientServerPathBase + "/cancelAction/" + actionID + "/feedback", &doc, hawkbitClientHttpPOST);
            nextState = stmHawkbitRestart;
            break;

        case(stmHawkbitDeploy):
            // Try to send the GET and if there is a failure restart
            if(!hawkbitClientHttp(hrefDeployment, &doc, hawkbitClientHttpGET)) {
                nextState = stmHawkbitRestart;
            }

            // If there is no failure, record the action ID, re-program, and move to acknowledgement
            else {
                actionID = doc["id"] | "";                

                updateResult = hawkbitClientUpdateHandler(doc["deployment"]["chunks"][0]["artifacts"][0]["_links"]["download-http"]["href"] | "", actionID, &doc);
                
                nextState = stmHawkbitDeployAck;
            }
            break;
            
        case(stmHawkbitDeployAck):
            // Prepare the JSON acknowledgement
            doc.clear();
            doc["id"] = actionID;
            doc["status"]["execution"] = "closed";
            
            // Success
            if(updateResult.isEmpty()) {
                doc["status"]["result"]["finished"] = "success";
                nextState = stmHawkbitDeploySuccessReboot;
            }

            // Failed (no reboot)
            else {
                doc["status"]["details"][0] = updateResult;
                doc["status"]["result"]["finished"] = "failure";
                nextState = stmHawkbitRestart;
            }

            hawkbitClientHttp(hawkbitClientServerPathBase + "/deploymentBase/" + actionID + "/feedback", &doc, hawkbitClientHttpPOST);
            break;

        case(stmHawkbitDeploySuccessReboot):
            restCtrlImmediateHandle(rstTypeReset);
            nextState = stmHawkbitRestart;
            break;
        
        // Configuration requested
        case(stmHawkbitConfig):
            // TODO: Real data here
            
            doc.clear();

            doc["mode"] = "replace";

            doc["data"]["device"] = "ESP8266 D1 Mini";
            doc["data"]["hw revision"] = "1";
            doc["data"]["sw version"] = "1.0.0";
            doc["data"]["serial"] = "J123456789"; 
            doc["data"]["stm"] = "hawkbit_client";
           
            doc["status"]["execution"] = "closed";
            doc["status"]["result"]["finished"] = "success";

            hawkbitClientHttp(hrefConfig, &doc, hawkbitClientHttpPUT);
            nextState = stmHawkbitRestart;
            break;

        // Reset state machine
        default:
            nextState = stmHawkbitRestart;
            break;
    }    
 
    // State change
    if (hawkbitClientCurrentState != nextState) {
        debugMessage = String() + hawkbitClientModuleName + ": State change to " + hawkbitClientStateNames[nextState];
        debugLog(&debugMessage, info);
    }
    
    // Update last states and current states (in this order)
    hawkbitClientCurrentState = nextState;
}

/**
    Get the current state of the hawkbit state machine.

    @return        state machine state.
*/
hawkbitClientStm hawkbitClientGetCurrentState(void) {
    return(hawkbitClientCurrentState);
}
