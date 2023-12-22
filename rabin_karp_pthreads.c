#include "thread_helpers.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HASH_BASE 256
#define HASH_PRIME 101

#define NUM_MAIN_THREADS 8

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
  for (int i = 0; i < pattern_length; ++i) {
    if (text[text_offset + i] != pattern[i]) {
      is_matching = 0;
      break;
    }
  }

  return is_matching;
}

void *thread_text_fn(void *arg) {
  pthread_text_arg_t *text_arg = (pthread_text_arg_t *)arg;
  char *text = text_arg->text;
  size_t pattern_length = text_arg->pattern_length;
  int pattern_hash = text_arg->pattern_hash;
  char *pattern = text_arg->pattern;
  output_t *output = text_arg->output;

  for (size_t text_offset = text_arg->start; text_offset < text_arg->end; ++text_offset) {
      // Compute the hash of the current window
      int text_window_hash = compute_hash(text + text_offset, pattern_length);
      if (text_window_hash == pattern_hash) {
        int is_matching =
            is_pattern_matching(text, text_offset, pattern, pattern_length);
        if (is_matching) {
          pthread_mutex_lock(text_arg->lock);
          output->identified_patterns[text_arg->pattern_idx]
              ->indexes[output->identified_patterns[text_arg->pattern_idx]->len++] = text_offset;
          pthread_mutex_unlock(text_arg->lock);
        }
      }
    }

  return NULL;
}

void *thread_pattern_fn(void *arg) {
  pthread_pattern_arg_t *pattern_arg = (pthread_pattern_arg_t *)arg;
  int start = pattern_arg->start;
  int end = pattern_arg->end;
  output_t *output = pattern_arg->output;
  char **patterns = pattern_arg->patterns;
  char *text = pattern_arg->text;
  size_t text_length = pattern_arg->text_length;

  for (int i = start; i < end; i++) {
    output->identified_patterns[i] = alloc_pattern_w_idx();
    if (!output->identified_patterns[i]) {
      return NULL;
    }

    char *pattern = patterns[i];
    size_t pattern_length = strlen(pattern);

    strcpy(output->identified_patterns[i]->pattern, pattern);
    output->identified_patterns[i]->len = 0;

    int pattern_hash = compute_hash(pattern, pattern_length);

    size_t sliding_points = text_length - pattern_length;

    pthread_t threads[NUM_MAIN_THREADS];
    pthread_text_arg_t args[NUM_MAIN_THREADS];

    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

  for (int j = 0; j < NUM_MAIN_THREADS; j++) {
        args[j].start = j * (double) (sliding_points + 1) / NUM_MAIN_THREADS;
        args[j].end = MIN((j + 1) * (double) (sliding_points + 1) / NUM_MAIN_THREADS, sliding_points + 1);
        args[j].text = text;
        args[j].text_length = text_length;
        args[j].pattern_length = pattern_length;
        args[j].pattern_hash = pattern_hash;
        args[j].pattern = pattern;
        args[j].output = output;
        args[j].pattern_idx = i;
        args[j].lock = &lock;
        int r = pthread_create(&threads[j], NULL, thread_text_fn, (void *)&args[j]);

        if (r) {
            exit(-1);
        }
  }

  for (int i = 0; i < NUM_MAIN_THREADS; i++) {
    void *s;
    int r = pthread_join(threads[i], &s);

    if (r) {
      exit(-1);
    }
  }

  pthread_mutex_destroy(&lock);
  }

  return NULL;
}

output_t *rabin_karp_pthreads(input_t *input) {
  // Parse input parameters
  char *text = input->text;
  int n_patterns = input->n_patterns;
  char **patterns = input->patterns;

  size_t text_length = strlen(text);

  output_t *output = (output_t *)(malloc(sizeof(output_t)));
  if (output == NULL) {
    perror("Error allocating memory for output");
    return NULL;
  }

  output->identified_patterns =
      (pattern_w_idx_t **)(malloc(n_patterns * sizeof(pattern_w_idx_t *)));
  if (output->identified_patterns == NULL) {
    perror("Error allocating memory for output.identified_patterns\n");
    free(output);
    return NULL;
  }

  output->n_patterns = n_patterns;

  pthread_t threads[NUM_MAIN_THREADS];
  pthread_pattern_arg_t args[NUM_MAIN_THREADS];

  // no reason to launch a lot of threads for too few patterns
  int threads_count = MIN(NUM_MAIN_THREADS, n_patterns);

  for (int i = 0; i < threads_count; i++) {
        args[i].start = i * (double) n_patterns / threads_count;
        args[i].end = MIN((i + 1) * (double) n_patterns / threads_count, n_patterns);
        args[i].output = output;
        args[i].patterns = patterns;
        args[i].text = text;
        args[i].text_length = text_length;

        int r = pthread_create(&threads[i], NULL, thread_pattern_fn, (void *)&args[i]);

        if (r) {
            exit(-1);
        }
  }

  for (int i = 0; i < threads_count; i++) {
    void *s;
    int r = pthread_join(threads[i], &s);

    if (r) {
      exit(-1);
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

  for (int i = 0; i < number_of_tests; i++) {
    output_t *output = rabin_karp_pthreads(inputs[i]);

    if (output == NULL) {
      perror("Error computing the output");
      destroy_tests(inputs, ref, number_of_tests);
      return -1;
    }

    // Check correctness
    const char *correctness =
        check_correctness(output, ref[i]) ? "FAILED" : "PASSED";
    printf("test %d: %s\n", i, correctness);
    free_output_struct(output);
  }

  destroy_tests(inputs, ref, number_of_tests);

  return 0;
}