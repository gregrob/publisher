#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266httpUpdate.h>

#include "hawkbit_ctrl.h"

#include "utils.h"
#include "debug.h"
#include "nvm_cfg.h"
#include "wifi.h"


// Set the module call interval
#define MODULE_CALL_INTERVAL            HAWKBIT_CTRL_CYCLIC_RATE

// Time to stay in idle state in S
#define HAWKBIT_CTRL_IDLE_TIME_RST_S    (10)

// Size of the JSON document
#define HAWKBIT_CTRL_JSON_DOCUMENT_SIZE (1024)


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
static const char * hawkbitCtrlStateNames[] {
    "stmHawkbitRestart",
    "stmHawkbitIdle",
    "stmHawkbitPoll",
    "stmHawkbitCancel",
    "stmHawkbitCancelAck",
    "stmHawkbitDeploy",
    "stmHawkbitDeployAckFail",
    "stmHawkbitConfig"
};

// Module name for debug messages
const char* hawkbitCtrlModuleName = "hawkbitCtrl";

// Hawkbit tennant ID
const char* hawkbitCtrlTennantID = "DEFAULT";

// Hawkbit server
const char* hawkbitCtrlServerURL = "svr.max.lan:9090";

// Hawkbit gateway security token
const char* hawkbitCtrlGatewayToken = "2401a1f9f311a30de365b861a35bc620";


// JSON static document
static StaticJsonDocument<HAWKBIT_CTRL_JSON_DOCUMENT_SIZE> doc; 

// Current state
static hawkbitCtrlStm hawkbitCtrlCurrentState;

// Hawkbit controller identification
String hawkbitCtrlControllerID;

// Hawkbit base server API path
String hawkbitCtrlServerPathBase;


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
    http.addHeader("Authorization", String() + "GatewayToken " + hawkbitCtrlGatewayToken);

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
        debugMessage = String() + hawkbitCtrlModuleName + ": " + hawkbitClientHttpRestTypeNames[apiType] + " " + serverPath + ", returned " + httpResponseCode;

        // Positive HTTP response
        if (httpResponseCode == HTTP_CODE_OK) {
            debugLog(&debugMessage, info);

            // JSON deserialisation problem
            if(jsonResponse) {
                debugMessage = String() + hawkbitCtrlModuleName + ": JSON deserialisation failed with code " + jsonResponse.f_str();
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
    Hawkbit controller init.
*/
void hawkbitCtrlInit(void) {
    
    // Pointer to the RAM mirror
    const nvmCompleteStructure * ramMirrorPtr;

    // Set-up pointer to RAM mirror
    (void) nvmGetRamMirrorPointerRO(&ramMirrorPtr);

    hawkbitCtrlCurrentState = stmHawkbitRestart;
    hawkbitCtrlControllerID = String() + getWiFiModuleDetails()->moduleHostName;
    hawkbitCtrlServerPathBase = String() + "http://" + hawkbitCtrlServerURL + "/" + hawkbitCtrlTennantID + "/controller/v1/" + hawkbitCtrlControllerID;
    doc.clear();
}

void update_finished()
{
String lastActionedID = doc["id"] | "";

doc.clear();

    Serial1.println("all done");

               doc["id"] = lastActionedID;

            // TODO: Put real status here

            doc["status"]["execution"] = "closed";
            doc["status"]["result"]["finished"] = "success";

            hawkbitClientHttp(hawkbitCtrlServerPathBase + "/deploymentBase/" + lastActionedID + "/feedback", &doc, hawkbitClientHttpPOST);
}

/**
    Hawkbit controller state machine.
*/
void hawkbitCtrlStateMachine(void) {
    
    WiFiClient wifi;

    // Debug message
    String debugMessage;

    // State timer
    static uint32_t stateTimer;
    
    // Last state
    static hawkbitCtrlStm lastState = stmHawkbitRestart;

    // Next state
    hawkbitCtrlStm nextState = hawkbitCtrlCurrentState;

    // Last actioned ID
    static String lastActionedID;

    // Current ID
    String currentID;

    // Strings for hrefs
    static String hrefCancel;
    static String hrefConfig;
    static String hrefDeployment;

    // Handle the state machine
    switch(hawkbitCtrlCurrentState) {
        
        case(stmHawkbitRestart):
            stateTimer = SECS_TO_CALLS(HAWKBIT_CTRL_IDLE_TIME_RST_S);
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
            if(!hawkbitClientHttp(hawkbitCtrlServerPathBase, &doc, hawkbitClientHttpGET)) {
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
            if(!hawkbitClientHttp(hrefCancel, &doc, hawkbitClientHttpGET)) {
                nextState = stmHawkbitRestart;
            }

            // Success with GET HTTP and JSON decoding
            else {
                nextState = stmHawkbitCancelAck;
            }
            break;

        case(stmHawkbitCancelAck):            
            currentID = doc["cancelAction"]["stopId"] | "";

            doc.clear();

            doc["id"] = currentID;
            doc["status"]["execution"] = "closed";
            doc["status"]["result"]["finished"] = "success";

            // TODO: Put handling for returning failures here - if (lastActionedID.compareTo(currentID))

            hawkbitClientHttp(hawkbitCtrlServerPathBase + "/cancelAction/" + currentID + "/feedback", &doc, hawkbitClientHttpPOST);
            nextState = stmHawkbitRestart;
            break;

        case(stmHawkbitDeploy):
            if(!hawkbitClientHttp(hrefDeployment, &doc, hawkbitClientHttpGET)) {
                nextState = stmHawkbitRestart;
            }

            // Success with GET HTTP and JSON decoding
            else {
                lastActionedID = doc["id"] | "";
                
                String urll = doc["deployment"]["chunks"][0]["artifacts"][0]["_links"]["download-http"]["href"] | "";

                doc.clear();

                doc["id"] = lastActionedID;

                doc["status"]["execution"] = "proceeding";
                doc["status"]["result"]["finished"] = "none";

                hawkbitClientHttp(hawkbitCtrlServerPathBase + "/deploymentBase/" + lastActionedID + "/feedback", &doc, hawkbitClientHttpPOST);
                

            //ESPhttpUpdate.update(wifi);

            ESPhttpUpdate.onEnd(update_finished);

            Serial1.println(ESPhttpUpdate.update(wifi, urll));
            // TODO: Flashing here           
            //Serial1.println(hawkbitRxPayload);
            //Serial1.println(String() + (doc["deployment"]["chunks"][0]["artifacts"][0]["_links"]["download-http"]["href"] | ""));

            //HTTP_UPDATE_FAILED (bad image etc)
            //HTTP_UPDATE_NO_UPDATES
            //HTTP_UPDATE_OK
            nextState = stmHawkbitDeployAckFail;
            }
            break;

        // Failed deployment (unit would reset before it got here)
        case(stmHawkbitDeployAckFail):
            doc.clear();

            doc["id"] = lastActionedID;

            doc["status"]["execution"] = "closed";
            doc["status"]["details"][0] = "its busted";
            doc["status"]["result"]["finished"] = "failure";

            hawkbitClientHttp(hawkbitCtrlServerPathBase + "/deploymentBase/" + lastActionedID + "/feedback", &doc, hawkbitClientHttpPOST);
            nextState = stmHawkbitRestart;
            break;

        // Configuration requested
        case(stmHawkbitConfig):
            doc.clear();

            doc["mode"] = "replace";

            doc["data"]["device"] = "ESP8266 D1 Mini";
            doc["data"]["hw revision"] = "1";
            doc["data"]["sw version"] = "1.0.0";
            doc["data"]["serial"] = "J123456789"; 
            doc["data"]["stm"] = "hawkbit_ctrl";
           
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
    if (hawkbitCtrlCurrentState != nextState) {
        debugMessage = String() + hawkbitCtrlModuleName + ": State change to " + hawkbitCtrlStateNames[nextState];
        debugLog(&debugMessage, info);
    }
    
    // Update last states and current states (in this order)
    lastState = hawkbitCtrlCurrentState;
    hawkbitCtrlCurrentState = nextState;
}

/**
    Get the current state of the hawkbit state machine.

    @return        state machine state.
*/
hawkbitCtrlStm hawkbitCtrlGetCurrentState(void) {
    return(hawkbitCtrlCurrentState);
}
