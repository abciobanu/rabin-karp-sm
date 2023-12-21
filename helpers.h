#ifndef HELPERS_H__
#define HELPERS_H__

#include <string.h>

#define MAX_TEXT_LENGTH 100001
#define MAX_PATTERN_LENGTH 81
#define MAX_FOUND_PATTERNS 10000
#define MAX_OUTPUT_LINE_LENGTH MAX_PATTERN_LENGTH + MAX_FOUND_PATTERNS * 4


/**
 * @brief fgets can leave a '\n' at the end of the string, this macro removes
 * it.
 * @param str The string.
*/
#define REMOVE_NEWLINE(str) \
    do { \
        size_t len = strlen(str); \
        if (len > 0 && str[len - 1] == '\n') { \
            str[len - 1] = '\0'; \
        } \
    } while(0)


/**
 * @brief Struct for handling the input.
 * @var n_patterns: The number of patterns.
 * @var patterns: An array of strings (of patterns of course)
 * @var text: The text where to search the patterns.
*/
typedef struct RabinKarpInput {
    int n_patterns;
    char **patterns;
    char *text;
} input_t;


typedef struct IdentifiedPattern {
    char *pattern;
    int len;
    int *indexes;
} pattern_w_idx_t;

/**
 * @brief Struct for handling the output/ref.
 * @var n_patterns: The number of patterns.
 * @var patterns: An array of strings (of patterns of course)
*/
typedef struct RabinKarpOutput {
    int n_patterns;
    pattern_w_idx_t **identified_patterns;
} output_t;


#endif