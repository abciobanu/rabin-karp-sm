#include "helpers.h"

#include <stdio.h>
#include <stdlib.h>


input_t *parse_input_file(const char *fname) {
    /** @brief Parses a test input file with the following format:
     * num_patterns
     * pattern1
     * pattern2
     * ...
     * text
     * 
     * @param fname The path to the file.
     * @return The data parsed in RabinKarpInput struct.
     */
    input_t *res = (input_t *)(malloc(sizeof(input_t)));
    if (!res) {
        perror("malloc failed for input_t alloc");
        return NULL;
    }

    FILE *fp = fopen(fname, "r");
    if (!fp) {
        perror("error opening test input file");
        goto failure_input_file;
    }

    fscanf(fp, "%d\n", &res->n_patterns);

    res->patterns = (char **)(malloc(res->n_patterns * sizeof(char *)));
    if (!res->patterns) {
        perror("malloc failed for patterns array");
        goto failure_input_pattern_array;
    }

    int i;
    for (i = 0; i < res->n_patterns; i++) {
        res->patterns[i] = (char *)(malloc(MAX_PATTERN_LENGTH * sizeof(char)));
        if (!res->patterns[i]) {
            perror("malloc failed for pattern");
            goto failure_input_pattern;
        }
        fgets(res->patterns[i], MAX_PATTERN_LENGTH, fp);
        REMOVE_NEWLINE(res->patterns[i]);
    }

    res->text = (char *)(malloc(MAX_TEXT_LENGTH * sizeof(char)));
    if (!res->text) {
        perror("malloc failed for text");
        goto failure_input_pattern;
    }
    fgets(res->text, MAX_TEXT_LENGTH, fp);
    REMOVE_NEWLINE(res->text);

    fclose(fp);

    return res;


failure_input_pattern:
    for (int j = 0; j < i; j++)
        free(res->patterns[j]);
failure_input_pattern_array:
    fclose(fp);
failure_input_file:
    free(res);
    return NULL;
}

pattern_w_idx_t *alloc_pattern_w_idx() {
    pattern_w_idx_t *res = (pattern_w_idx_t *)(malloc(sizeof(pattern_w_idx_t)));
    if (!res) {
        return NULL;
    }

    res->pattern = (char *)(malloc(MAX_PATTERN_LENGTH * sizeof(char)));
    if (!res->pattern) {
        free(res);
        return NULL;
    }

    res->indexes = (int *)malloc(MAX_FOUND_PATTERNS * sizeof(int));
    if (!res->indexes) {
        free(res->pattern);
        free(res);
        return NULL;
    }

    return res;
}

void free_pattern_w_idx(pattern_w_idx_t *ptr) {
    free(ptr->pattern);
    free(ptr->indexes);
    free(ptr);
}

/** @brief Parses a test output/ref file with the following format:
 * 
 * pattern1: idx1, idx2, ...
 * pattern2: idx1, idx2, ...
 * ...
 * @param fname The path to the file.
 * @return The data parsed in RabinKarpOutput struct.
 */
output_t *parse_output_file(const char *fname, int n_patterns) {
    output_t *res = (output_t *)(malloc(sizeof(output_t)));
    if (!res) {
        perror("malloc failed for output_t alloc");
        return NULL;
    }

    res->n_patterns = n_patterns;
    res->identified_patterns = (pattern_w_idx_t **)(malloc(n_patterns * sizeof(pattern_w_idx_t *)));
    if (!res->identified_patterns) {
        perror("malloc failed for identified patterns");
        free(res);
        return NULL;
    }

    FILE *fp = fopen(fname, "r");
    if (!fp) {
        perror("error opening test output file");
        goto failure_output_file;
    }


    char *buffer = (char *)(malloc(MAX_OUTPUT_LINE_LENGTH * sizeof(char)));
    int i;
    for (i = 0; i < n_patterns; i++) {
        fgets(buffer, MAX_OUTPUT_LINE_LENGTH, fp);
        REMOVE_NEWLINE(buffer);
        pattern_w_idx_t *pattern_w_idx = alloc_pattern_w_idx();
        if (!pattern_w_idx) {
            goto failure_output_identified_patterns;
        }

        pattern_w_idx->len = 0;

        strncpy(pattern_w_idx->pattern, strtok(buffer, ":"), MAX_PATTERN_LENGTH);

        char *numbers = strtok(NULL, ":");
        char *number = strtok(numbers, " ");
        pattern_w_idx->indexes[pattern_w_idx->len++] = atoi(number);

        while (number = strtok(NULL, " ")) {
            pattern_w_idx->indexes[pattern_w_idx->len++] = atoi(number);
        }

        res->identified_patterns[i] = pattern_w_idx;
    }

    return res;

failure_output_identified_patterns:
    for (int j = 0; j < i; j++) {
        free_pattern_w_idx(res->identified_patterns[j]);
    }
    free(res->identified_patterns);
failure_output_buffer:
    fclose(fp);
failure_output_file:
    free(res);
    return NULL;
}