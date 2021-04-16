#ifndef UTILS_H
#define UTILS_H

// Stringify argument
#define STR_IMPL_(x)       (#x)

// String length of a integer (null removed)
// Indirect to expand argument macros
#define STRNLEN_INT(x)      (sizeof(STR_IMPL_(x)) - 1)

#endif
