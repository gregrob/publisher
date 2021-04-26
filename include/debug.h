#ifndef DEBUG_H
#define DEBUG_H

// Structure for storing esc code
typedef struct {
  const char* code;
  const char* description;
} escCode;

// Text color enum
enum textColour {
    reset       = 0,
    black       = 1, 
    red         = 2,
    green       = 3,
    yellow      = 4,
    blue        = 5,
    magenta     = 6,
    cyan        = 7,
    white       = 8,
    brblack    = 9,
    brred      = 10,
    brgreen    = 11,
    bryellow   = 12,
    brblue     = 13,
    brmagenta  = 14,
    brcyan     = 15,
    brwhite    = 16
};

// Log level enum
enum logLevel {
    info       = 0,
    warning    = 1, 
    error      = 2
};

// Set the pointer to the debug serial port
HardwareSerial* const debugSetSerial(HardwareSerial* const serialPort);

// Test esc code printing
void escCodeTest(void);

// Prints to the debug serial port
void debugPrint(String* const rawData, textColour rawDataColour);

// Prints a line to the debug serial port
void debugPrintln(String* const rawData, textColour rawDataColour);

// Prints to the log
void debugLog(String* const message, logLevel level);
void debugLog(String* const message, const char * const module, logLevel level);

#endif