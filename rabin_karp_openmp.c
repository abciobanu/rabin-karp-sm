#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <omp.h>

#include "helpers.h"

#define HASH_BASE 256
#define HASH_PRIME 101

int compute_hash(char *str, int len) {
  int hash = 0;

  for (int i = 0; i < len; i++) {
    hash = ((hash * HASH_BASE) % HASH_PRIME + str[i]) % HASH_PRIME;
  }

  return hash;
}

int is_pattern_matching(char *text, int text_offset, char *pattern,
                        int pattern_length) {
  int is_matching = 1;
  int i;
  #pragma omp parallel for default(none) shared(is_matching, pattern, pattern_length, text, text_offset) private(i)
  for (i = 0; i < pattern_length; ++i) {
    if (is_matching == 0) {
        continue;
    }
    if (text[text_offset + i] != pattern[i]) {
      is_matching = 0;
    }
  }

  return is_matching;
}

output_t *rabin_karp_omp(input_t *input) {
  // Parse input parameters
  char *text = input->text;
  int n_patterns = input->n_patterns;
  char **patterns = input->patterns;

  size_t text_length = strlen(text);

  // Initialize output parameters
  output_t *output = (output_t *)(malloc(sizeof(output_t)));
  if (output == NULL) {
    perror("Error allocating memory for output");
    return NULL;
  }

  output->n_patterns = n_patterns;

  // Allocate memory for the identified patterns array
  output->identified_patterns =
      (pattern_w_idx_t **)(malloc(n_patterns * sizeof(pattern_w_idx_t *)));
  if (output->identified_patterns == NULL) {
    perror("Error allocating memory for output.identified_patterns\n");
    free(output);
    return NULL;
  }

  // Allocate memory for each identified pattern
  for (int i = 0; i < n_patterns; ++i) {
    output->identified_patterns[i] = alloc_pattern_w_idx();
    if (output->identified_patterns[i] == NULL) {
      perror("Error allocating memory for output.identified_patterns[i]\n");
      free(output->identified_patterns);
      free(output);
      return NULL;
    }
  }
  // Do the search for each pattern
  #pragma omp parallel for schedule(auto)
  for (int pattern_idx = 0; pattern_idx < n_patterns; ++pattern_idx) {
    char *pattern = patterns[pattern_idx];
    size_t pattern_length = strlen(pattern);

    output->identified_patterns[pattern_idx]->pattern = pattern;
    output->identified_patterns[pattern_idx]->len = 0;

    // Compute the hash of the current pattern
    int pattern_hash = compute_hash(pattern, pattern_length);

    // Move the sliding window over the text
    size_t sliding_points = text_length - pattern_length;

    #pragma omp parallel for schedule(static)
    for (size_t text_offset = 0; text_offset <= sliding_points; ++text_offset) {
      // Compute the hash of the current window
      int text_window_hash = compute_hash(text + text_offset, pattern_length);
      if (text_window_hash == pattern_hash) {
        int is_matching =
            is_pattern_matching(text, text_offset, pattern, pattern_length);
        if (is_matching) {
          output->identified_patterns[pattern_idx]
              ->indexes[output->identified_patterns[pattern_idx]->len++] =
              text_offset;
        }
      }
    }
  }

  return output;
}

int main(int argc, char *argv[]) {
  // Sanity check for arguments
  if (argc != 3) {
    printf("Usage: %s <tests_directory_path> <number_of_tests>\n", argv[0]);
    return -1;
  }

  // Get arguments
  char *tests_directory_path = argv[1];
  int number_of_tests = atoi(argv[2]);

  input_t **inputs =
      parse_all_input_files(tests_directory_path, number_of_tests);

  output_t **ref =
      parse_all_ref_files(tests_directory_path, inputs, number_of_tests);

  int stop_flag = 0;
  #pragma omp parallel for
  for (int i = 0; i < number_of_tests; i++) {
    if (stop_flag) {
        continue;
    }
    output_t *output = rabin_karp_omp(inputs[i]);

    if (output == NULL) {
      perror("Error computing the output");
      destroy_tests(inputs, ref, number_of_tests);
      stop_flag = 1;
      continue;
    }

    // Check correctness
    const char *correctness = check_correctness(output, ref[i]) ? "FAILED" : "PASSED";
    printf("test %d: %s\n", i, correctness);
  }

  destroy_tests(inputs, ref, number_of_tests);

  return 0;
}