#ifndef UTILS_H
#define UTILS_H

// Stringify argument
#define STR_IMPL_(x)                    (#x)

// String length of a integer (null removed)
// Indirect to expand argument macros
#define STRNLEN_INT(x)                  (sizeof(STR_IMPL_(x)) - 1)

// Convert a parameter from seconds to calls defined by the local call interval <MODULE_CALL_INTERVAL>
#define SECS_TO_CALLS(seconds)          (seconds * (1000 / MODULE_CALL_INTERVAL))

// Convert a parameter from calls defined by the local call interval <MODULE_CALL_INTERVAL> to seconds
#define CALLS_TO_SECS(calls)            ((calls * MODULE_CALL_INTERVAL)/1000)

// Enabled
#define ENABLED                         0x1

// Disabled
#define DISABLED                        0x0

// Maximum of a 32bit unsigned variable
#define MAX_VALUE_32BIT_UNSIGNED_DEC    4294967295

// Maximum of a 32bit unsigned variable
#define MAX_VALUE_32BIT_UNSIGNED_HEX    0xFFFFFFFF

// Maximum of a percentage string
#define MAX_PERCENTAGE_STRING           100%

#endif
