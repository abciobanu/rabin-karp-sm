#include "helpers.h"

#include <dirent.h>
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

  char buffer[MAX_PATTERN_DIGITS];
  fgets(buffer, MAX_PATTERN_DIGITS, fp);
  res->n_patterns = atoi(buffer);

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
  res->identified_patterns =
      (pattern_w_idx_t **)(malloc(n_patterns * sizeof(pattern_w_idx_t *)));
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
  if (!buffer) {
    goto failure_output_buffer;
  }
  int i;
  for (i = 0; i < n_patterns; i++) {
    fgets(buffer, MAX_OUTPUT_LINE_LENGTH, fp);
    REMOVE_NEWLINE(buffer);
    res->identified_patterns[i] = alloc_pattern_w_idx();
    if (!res->identified_patterns[i]) {
      goto failure_output_identified_patterns;
    }

    res->identified_patterns[i]->len = 0;

    strncpy(res->identified_patterns[i]->pattern, strtok(buffer, ":"),
            MAX_PATTERN_LENGTH);

    char *numbers = strtok(NULL, ":");
    char *number = strtok(numbers, " ");
    if (!number) {
      continue;
    }
    res->identified_patterns[i]->indexes[res->identified_patterns[i]->len++] =
        atoi(number);

    while ((number = strtok(NULL, " "))) {
      res->identified_patterns[i]->indexes[res->identified_patterns[i]->len++] =
          atoi(number);
    }
  }
  free(buffer);

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

void free_input_struct(input_t *ptr) {
  for (int i = 0; i < ptr->n_patterns; i++) {
    free(ptr->patterns[i]);
  }
  free(ptr->patterns);
  free(ptr->text);
  free(ptr);
}

void free_output_struct(output_t *ptr) {
  for (int i = 0; i < ptr->n_patterns; i++) {
    free(ptr->identified_patterns[i]->indexes);
    free(ptr->identified_patterns[i]->pattern);
    free(ptr->identified_patterns[i]);
  }
  free(ptr->identified_patterns);
  free(ptr);
}

// Those can be merged in a single function, with parameters... but I found it
// quicker to do them like this.
input_t **parse_all_input_files(const char *root_folder, int num_tests) {
  /** @brief Parses all the files that end with '.in' in the root_folder, into
   * input_t structs.
   * @param root_folder The folder where we find the .in files.
   * @param num_tests The number of tests (should be read as a command line
   * argument).
   * @return An array with input_t structs for each test case.
   */
  input_t **res = (input_t **)(malloc(num_tests * sizeof(input_t *)));
  if (!res) {
    return NULL;
  }

  struct dirent *entry;

  DIR *dir = opendir(root_folder);

  if (dir == NULL) {
    perror("Error opening directory");
    return NULL;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG && strstr(entry->d_name, ".in") != NULL) {
      char full_path[MAX_FILE_PATH];
      int test_num;
      sscanf(entry->d_name, "test%d.in", &test_num);
      // watchout for the separator, if there are errors on your platform
      snprintf(full_path, MAX_FILE_PATH, "%s/%s", root_folder,
               entry->d_name);
      input_t *input_ptr = parse_input_file(full_path);
      if (!input_ptr) {
        free(res);
        perror("Failed to parse input files!");
        exit(-1);
      }
      res[test_num] = input_ptr;
    }
  }

  closedir(dir);

  return res;
}

output_t **parse_all_ref_files(const char *root_folder, input_t **inputs,
                               int num_tests) {
  /** @brief Parses all the files that end with '.ref' in the root_folder, into
   * output_t structs.
   * @param root_folder The folder where we find the .in files.
   * @param inputs The inputs that were parsed with parse_all_input_files.
   * @param num_tests The number of tests (should be read as a command line
   * argument).
   * @return An array with output_t structs for each test case.
   */
  output_t **res = (output_t **)(malloc(num_tests * sizeof(output_t *)));
  if (!res) {
    return NULL;
  }

  struct dirent *entry;

  DIR *dir = opendir(root_folder);

  if (dir == NULL) {
    perror("Error opening directory");
    return NULL;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG && strstr(entry->d_name, ".ref") != NULL) {
      char full_path[MAX_FILE_PATH];
      int test_num;
      sscanf(entry->d_name, "test%d.in", &test_num);
      // watchout for the separator, if there are errors on your platform
      snprintf(full_path, sizeof(full_path), "%s/%s", root_folder,
               entry->d_name);
      output_t *output_ptr =
          parse_output_file(full_path, inputs[test_num]->n_patterns);
      if (!output_ptr) {
        free(res);
        perror("Failed to parse ref files!");
        exit(-1);
      }
      res[test_num] = output_ptr;
    }
  }

  closedir(dir);

  return res;
}

void destroy_tests(input_t **inputs, output_t **outputs, int num_tests) {
  for (int i = 0; i < num_tests; i++) {
    free_input_struct(inputs[i]);
    free_output_struct(outputs[i]);
  }

  free(inputs);
  free(outputs);
}

int cmp_patterns(const void *a, const void *b) {
  const pattern_w_idx_t *patternA = *(const pattern_w_idx_t **)a;
  const pattern_w_idx_t *patternB = *(const pattern_w_idx_t **)b;

  return strcmp(patternA->pattern, patternB->pattern);
}

int cmp_indexes(const void *a, const void *b) {
  return *((int *)a) - *((int *)b);
}

int check_correctness(output_t *output, output_t *gt) {
  /** @brief Compares two output structs.
   * @param output The output produced by your program
   * @param gt The ref / ground truth.
   * @return 0 if they are equal, 1 otherwise, and also prints the diff.
   */
  if (output->n_patterns != gt->n_patterns) {
    printf("Different num of patterns: %d (output) vs %d (gt)\n",
           output->n_patterns, gt->n_patterns);
    return 1;
  }

  qsort(output->identified_patterns, output->n_patterns,
        sizeof(pattern_w_idx_t *), cmp_patterns);
  qsort(gt->identified_patterns, gt->n_patterns, sizeof(pattern_w_idx_t *),
        cmp_patterns);

  for (int i = 0; i < output->n_patterns; i++) {
    char *output_pattern = output->identified_patterns[i]->pattern;
    char *gt_pattern = gt->identified_patterns[i]->pattern;

    if (strcmp(output_pattern, gt_pattern) != 0) {
      printf("Different patterns found at index (after sorting) %d: %s "
             "(output) vs %s (gt)\n",
             i, output_pattern, gt_pattern);
      return 1;
    }

    int output_len = output->identified_patterns[i]->len;
    int gt_len = gt->identified_patterns[i]->len;

    if (output_len != gt_len) {
      printf("Different length for pattern %s: %d (output) vs %d (gt)\n",
             output_pattern, output_len, gt_len);
      return 1;
    }

    int *output_indexes = output->identified_patterns[i]->indexes;
    int *gt_indexes = gt->identified_patterns[i]->indexes;

    qsort(output_indexes, output_len, sizeof(int), cmp_indexes);
    qsort(gt_indexes, gt_len, sizeof(int), cmp_indexes);

    for (int j = 0; j < output_len; j++) {
      if (output_indexes[j] != gt_indexes[j]) {
        printf("Indexes differ at position %d: %d (output) vs %d gt for "
               "pattern %s\n",
               j, output_indexes[j], gt_indexes[j], gt_pattern);
        return 1;
      }
    }
  }

  return 0;
}
