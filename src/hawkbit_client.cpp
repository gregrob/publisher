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
#include "version.h"


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
        debugMessage = String() + hawkbitClientHttpRestTypeNames[apiType] + " " + serverPath + ", returned " + httpResponseCode;

        // Positive HTTP response
        if (httpResponseCode == HTTP_CODE_OK) {
            debugLog(&debugMessage, hawkbitClientModuleName, info);

            // JSON deserialisation problem
            if (jsonResponse) {
                debugMessage = String() + "JSON deserialisation failed with code " + jsonResponse.f_str();
                debugLog(&debugMessage, hawkbitClientModuleName, warning);

                returnValue = false;
            }

            else {
                returnValue = true;
            }
        }

        else {
            debugLog(&debugMessage, hawkbitClientModuleName, error);

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
static void hawkbitClientUpdateProgress(unsigned int progress, unsigned int total, const String newActionID, JsonDocument * const newDocPtr) {
            
    // Debug strings
    String debugMessage;
    char percentageCompleteString[STRNLEN_INT(MAX_PERCENTAGE_STRING) + 1];    
    
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
        
        snprintf(percentageCompleteString, sizeof(percentageCompleteString), "%u%%", percentageComplete);

        debugMessage = String() + "Updating software " + percentageCompleteString;
        debugLog(&debugMessage, hawkbitClientModuleName, info);

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
    // TODO: Consider FreeRTOS to better handle the blocking nature of the re-programming
    // TODO: Consider how to handle IO / OS / MQQT while this is blocking
    // TODO: Consider what could happen if second programming request comes via OTA (perhaps OTS should check if this has already processed a programming reqest - is in the Deployment states)

    // Do the update
    updateStatus = ESPhttpUpdate.update(wifi, updateImagePath);

    // Set-up the return message / error info
    switch (updateStatus) {
      case HTTP_UPDATE_FAILED:        
        returnValue = String() + "HTTP_UPDATE_FAILD Error " + ESPhttpUpdate.getLastError() + " (" + ESPhttpUpdate.getLastErrorString().c_str() + ")";
        break;

      case HTTP_UPDATE_NO_UPDATES:
        returnValue = String() + "HTTP_UPDATE_NO_UPDATES";
        break;

      case HTTP_UPDATE_OK:
        returnValue = "";
        break;
    }

    // If the return value is empty, success
    if (returnValue.isEmpty()) {
        debugMessage = String() + "Update completed successfully";
        debugLog(&debugMessage, hawkbitClientModuleName, info);
        
    }

    // Otherwise error
    else {
        debugMessage = String() + "Update failed: " + returnValue;
        debugLog(&debugMessage, hawkbitClientModuleName, error);
    }

    return(returnValue);
}


/**
    Construct and transmit HTTP PUT for configuration data.
        
    @param[in]     serverPath pointer to the full server path for the PUT.
    @param[in]     docPtr pointer to the JSON doc.
*/
void hawkbitClientConfigResponse(const String * const serverPath, JsonDocument * const docPtr) {
    
    // Version data
    const versionData * versionDataPtr;
    const uint32_t versionDataSize = versionGetData(&versionDataPtr);
    
    // Wifi data
    wifiModuleDetail * wifiModuleDetailsPtr = getWiFiModuleDetails();

    // Prepare the JSON response
    docPtr->clear();

    (*docPtr)["mode"] = "replace";
    (*docPtr)["status"]["execution"] = "closed";
    (*docPtr)["status"]["result"]["finished"] = "success";

    (*docPtr)["data"]["device"] = "publisher";
    (*docPtr)["data"]["stm"] = hawkbitClientModuleName;
    (*docPtr)["data"]["mac"] = wifiModuleDetailsPtr->moduleMAC;
    (*docPtr)["data"]["host"] = wifiModuleDetailsPtr->moduleHostName;

    // TODO: Add a parameter for the microcontroller type
    // TODO: Add a parameter for HW revision

    // Handle all version info
    for(unsigned int i = 0; i < versionDataSize; i++) {
        (*docPtr)["data"][versionDataPtr[i].versionName] = versionDataPtr[i].versionContents;
    }

    // Send the response
    hawkbitClientHttp(*serverPath, docPtr, hawkbitClientHttpPUT);
}


/**
    Hawkbit controller init.
*/
void hawkbitClientInit(void) {
    
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);

    // TODO: How should local globals be accessed within this function?
    // TODO: Read hawkbit configuration parameters from NVM
    hawkbitClientCurrentState = stmHawkbitRestart;
    hawkbitClientServerPathBase = String() + "http://" + hawkbitClientServerURL + "/" + hawkbitClientTennantID + "/controller/v1/" + getWiFiModuleDetails()->moduleHostName;
    doc.clear();
}


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

    // TODO: How should local globals be accessed within this function?

    // Handle the state machine
    switch(hawkbitClientCurrentState) {
        
        // Start here when the statemachine is reset
        case(stmHawkbitRestart):
            stateTimer = SECS_TO_CALLS(HAWKBIT_CLIENT_IDLE_TIME_RST_S);
            nextState = stmHawkbitIdle;
            break;
        
        // Wait here for a period of time before polling
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
        
        // Poll hawkbit server for pending requests
        case(stmHawkbitPoll):
            // Try to send the GET and if there is a failure restart
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

        // Get details of a cancellation request
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

        // Acknowledge cancellation
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

        // Get details of deployment and attempt programming
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
            
        // Acknowledge sucessfull or unsucessfull deployment attempt
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

        // Programming success so reboot
        case(stmHawkbitDeploySuccessReboot):
            restCtrlImmediateHandle(rstTypeReset);
            nextState = stmHawkbitRestart;
            break;
        
        // Configuration requested
        case(stmHawkbitConfig):
            hawkbitClientConfigResponse(&hrefConfig, &doc);
            nextState = stmHawkbitRestart;
            break;

        // Reset state machine
        default:
            nextState = stmHawkbitRestart;
            break;
    }    
 
    // State change
    if (hawkbitClientCurrentState != nextState) {
        debugMessage = String() + "State change to " + hawkbitClientStateNames[nextState];
        debugLog(&debugMessage, hawkbitClientModuleName, info);
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
