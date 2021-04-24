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


// JSON static document
static StaticJsonDocument<HAWKBIT_CTRL_JSON_DOCUMENT_SIZE> doc; 

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

// Hawkbit tennant ID
const char* hawkbitCtrlTennantID = "DEFAULT";

// Hawkbit server
const char* hawkbitCtrlServerURL = "svr.max.lan:9090";

// Hawkbit gateway security token
const char* hawkbitCtrlGatewayToken = "2401a1f9f311a30de365b861a35bc620";

// Module name for debug messages
const char* hawkbitCtrlModuleName = "hawkbitCtrl";

// Current state
static hawkbitCtrlStm hawkbitCtrlCurrentState;

// Hawkbit controller identification
String hawkbitCtrlControllerID;

// Hawkbit base server API path
String hawkbitCtrlServerPathBase;

// Hawkbit authentication
String hawkbitCtrlAuthentication;

// Hawkbit Rx payload
String hawkbitRxPayload;

// Hawkbit Tx payload
String hawkbitTxPayload;


static int hawkbitCtrlGET(const String serverPath) {

    String debugMessage;

    WiFiClient wifi;
    HTTPClient http;
    
    int httpResponseCode;
    
    http.begin(wifi, serverPath.c_str());

    http.addHeader("Authorization", hawkbitCtrlAuthentication);
    http.addHeader("Accept", "application/hal+json");

    httpResponseCode = http.GET();

    debugMessage = String() + hawkbitCtrlModuleName + ": GET " + serverPath + ", returned " + httpResponseCode;

    if (httpResponseCode == HTTP_CODE_OK) {
        debugLog(&debugMessage, info);
        hawkbitRxPayload = http.getString();
    }

    else {
        debugLog(&debugMessage, error);
    }

    http.end();

    return(httpResponseCode);
}

static int hawkbitCtrlPOST(const String serverPath) {

    String debugMessage;

    WiFiClient wifi;
    HTTPClient http;
    
    int httpResponseCode;
    
    http.begin(wifi, serverPath.c_str());

    http.addHeader("Authorization", hawkbitCtrlAuthentication);
    http.addHeader("Content-Type", "application/json;charset=UTF-8");
    //http.addHeader("Accept", "application/hal+json");
    
    hawkbitTxPayload = "";
    (void) serializeJson(doc, hawkbitTxPayload);

    httpResponseCode = http.POST(hawkbitTxPayload);
    
    debugMessage = String() + hawkbitCtrlModuleName + ": POST " + serverPath + ", returned " + httpResponseCode;

    if (httpResponseCode == HTTP_CODE_OK) {
        debugLog(&debugMessage, info);
        hawkbitRxPayload = http.getString();
    }

    else {
        debugLog(&debugMessage, error);
    }

    http.end();

    return(httpResponseCode);
}


static int hawkbitCtrlPUT(const String serverPath) {

    String debugMessage;

    WiFiClient wifi;
    HTTPClient http;
    
    int httpResponseCode;
    
    http.begin(wifi, serverPath.c_str());

    http.addHeader("Authorization", hawkbitCtrlAuthentication);
    http.addHeader("Content-Type", "application/json;charset=UTF-8");
    
    hawkbitTxPayload = "";
    (void) serializeJson(doc, hawkbitTxPayload);

    httpResponseCode = http.PUT(hawkbitTxPayload);
    
    debugMessage = String() + hawkbitCtrlModuleName + ": PUT " + serverPath + ", returned " + httpResponseCode;

    if (httpResponseCode == HTTP_CODE_OK) {
        debugLog(&debugMessage, info);
        hawkbitRxPayload = http.getString();
    }

    else {
        debugLog(&debugMessage, error);
    }

    http.end();

    return(httpResponseCode);
}


static DeserializationError hawkbitCtrlExtractJson(void) {
    
    String debugMessage;
    DeserializationError error;

    doc.clear();

    error = deserializeJson(doc, hawkbitRxPayload);

    if (error) {
        debugMessage = String() + hawkbitCtrlModuleName + ": JSON deserialisation failed with code " + error.f_str();
        debugLog(&debugMessage, warning);
    }

    return(error);
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
    hawkbitCtrlAuthentication = String() + "GatewayToken " + hawkbitCtrlGatewayToken;
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

            hawkbitCtrlPOST(hawkbitCtrlServerPathBase + "/deploymentBase/" + lastActionedID + "/feedback");
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
            // GET HTTP encountered an error
            if(hawkbitCtrlGET(hawkbitCtrlServerPathBase) != HTTP_CODE_OK) {
                nextState = stmHawkbitRestart;
            }

            // Decoding the JSON encountered an error
            else if (hawkbitCtrlExtractJson()) {
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
            // GET HTTP encountered an error
            if(hawkbitCtrlGET(hrefCancel) != HTTP_CODE_OK) {
                nextState = stmHawkbitRestart;
            }

            // Decoding the JSON encountered an error
            else if (hawkbitCtrlExtractJson()) {
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

            hawkbitCtrlPOST(hawkbitCtrlServerPathBase + "/cancelAction/" + currentID + "/feedback");
            nextState = stmHawkbitRestart;
            break;

        case(stmHawkbitDeploy):
            // GET HTTP encountered an error
            if(hawkbitCtrlGET(hrefDeployment) != HTTP_CODE_OK) {
                nextState = stmHawkbitRestart;
            }

            // Decoding the JSON encountered an error
            else if (hawkbitCtrlExtractJson()) {
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

                hawkbitCtrlPOST(hawkbitCtrlServerPathBase + "/deploymentBase/" + lastActionedID + "/feedback");
                

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

            hawkbitCtrlPOST(hawkbitCtrlServerPathBase + "/deploymentBase/" + lastActionedID + "/feedback");
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

            hawkbitCtrlPUT(hrefConfig);
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
