#ifndef __THREAD_HELPERS__
#define __THREAD_HELPERS__

#include "helpers.h"
#include <pthread.h>

typedef struct PThreadPatternArg {
  int start;
  int end;

  output_t *output;

  char **patterns;

  char *text;
  size_t text_length;
} pthread_pattern_arg_t;

typedef struct PThreadTextArg {
  size_t start;
  size_t end;

  char *text;
  size_t text_length;

  size_t pattern_length;
  int pattern_hash;
  char *pattern;

  output_t *output;
  int pattern_idx;

  pthread_mutex_t *lock;

} pthread_text_arg_t;

#endif