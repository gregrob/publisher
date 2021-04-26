#include <Arduino.h>

#include "debug.h"

#include "utils.h"

// esc code table
static const escCode textColourEscCodes[] = {{"\u001b[0m",  "reset"},
                                             {"\u001b[30m", "black"},
                                             {"\u001b[31m", "red"},
                                             {"\u001b[32m", "green"},
                                             {"\u001b[33m", "yellow"},
                                             {"\u001b[34m", "blue"},
                                             {"\u001b[35m", "magenta"},
                                             {"\u001b[36m", "cyan"},
                                             {"\u001b[37m", "white"},
                                             {"\u001b[90m", "bright black (gray)"},
                                             {"\u001b[91m", "bright red"},
                                             {"\u001b[92m", "bright green"},
                                             {"\u001b[93m", "bright yellow"},
                                             {"\u001b[94m", "bright blue"},
                                             {"\u001b[95m", "bright magenta"},
                                             {"\u001b[96m", "bright cyan"},
                                             {"\u001b[97m", "bright white"}
};

// Pointer to debug serial port 
static HardwareSerial *debugSerial;

/**
    Set the pointer to the debug serial port.

    @param[in]     serialPort pointer to the serial port.
    @return        pointer to the selected serial port.
*/
HardwareSerial* const debugSetSerial(HardwareSerial* const serialPort) {
    debugSerial = serialPort;

    return(debugSerial);
}

/**
    Test esc code printing.
*/
void escCodeTest(void) {
    String message = "ESC code print test...";
    debugPrintln(&message, reset);

    for(unsigned int i = 0; i < (sizeof(textColourEscCodes)/sizeof(textColourEscCodes[0])); i++) {
        //debugSerial->println(String() + textColourEscCodes[i].code + (String)i + ". " + textColourEscCodes[i].description + textColourEscCodes[textColour::reset].code);
        debugSerial->println(String() + textColourEscCodes[i].code + i + ". " + textColourEscCodes[i].description + textColourEscCodes[textColour::reset].code);
    }
}

/**
    Prints to the debug serial port.

    @param[in]     rawData pointer to the data to print.
    @param[in]     rawDataColour the colour to print the data in.
*/
void debugPrint(String* const rawData, textColour rawDataColour) {
    #ifndef DEBUG_BW
    debugSerial->print(String() + textColourEscCodes[rawDataColour].code + *rawData + textColourEscCodes[textColour::reset].code);
    #else
    debugSerial->print(*rawData);
    #endif  
}

/**
    Prints a line to the debug serial port.

    @param[in]     rawData pointer to the data to print.
    @param[in]     rawDataColour the colour to print the data in.
*/
void debugPrintln(String* const rawData, textColour rawDataColour) {
    #ifndef DEBUG_BW
    debugSerial->println(String() + textColourEscCodes[rawDataColour].code + *rawData + textColourEscCodes[textColour::reset].code);
    #else
    debugSerial->println(*rawData);
    #endif
}

/**
    Prints to the debug log header

    @param[in]     level log level formatting to use for the header.
*/
static void debugLogHeader(logLevel level) {
    
    String header;

    char currentTime[STRNLEN_INT(MAX_VALUE_32BIT_UNSIGNED_DEC) + 1];

    // Format the current time string (keep it 10 digits so it is consistent)
    snprintf(currentTime, sizeof(currentTime), "%10lu", millis());

    // Print the header
    switch(level) {
        case error:
            header = String() + "[E" + currentTime + "ms] ";
            debugPrint(&header, brred);
            break;

        case warning:
            header = String() + "[W" + currentTime + "ms] ";
            debugPrint(&header, bryellow); 
            break;

        case info:
        default:
            header = String() + "[I" + currentTime + "ms] ";
            debugPrint(&header, brgreen); 
            break;
    }
}

/**
    Prints to the log.

    @param[in]     message pointer to the message to be printed.
    @param[in]     level log level formatting to use for the header.
*/
void debugLog(String* const message, logLevel level) {    
    
    // Print the header
    debugLogHeader(level);

    // Print the remainder of the message
    debugPrintln(message, reset);        
}

/**
    Prints to the log.

    @param[in]     message pointer to the message to be printed.
    @param[in]     module module printing to the log.
    @param[in]     level log level formatting to use for the header.
*/
void debugLog(String* const message, const char * const module, logLevel level) {

    String header;

    // Print the header
    debugLogHeader(level);
    
    header = String() + module + ": ";
    debugPrint(&header, brwhite); 

    // Print the remainder of the message
    debugPrintln(message, reset);
}
