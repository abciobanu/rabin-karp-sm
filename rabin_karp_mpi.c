#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helpers.h"

#define MAPPER_RANK 0
#define REDUCER_RANK 1

#define MAPPING_DONE_MARKER -1

#define HASH_BASE 256
#define HASH_PRIME 101

const int MAPPING_DONE_MARKER_INT = MAPPING_DONE_MARKER;

int check_if_any_worker_busy(int worker_availabilities[], int n_workers) {
  // Skip MAPPER_RANK (0) and REDUCER_RANK (1)
  for (int i = 2; i < n_workers; i++) {
    if (!worker_availabilities[i]) {
      return 1;
    }
  }

  return 0;
}

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

int main(int argc, char *argv[]) {
  // Sanity check for arguments
  if (argc != 3) {
    printf("Usage: %s <tests_directory_path> <number_of_tests>\n", argv[0]);
    return -1;
  }

  // MPI initialization
  int mpi_rank, mpi_size;
  MPI_Status status;
  MPI_Init(&argc, &argv);

  // Get the rank and size of the current process
  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

  // Get the size of the group associated with the communicator
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

  // Sanity check for the number of processes
  if (mpi_size < 3) {
    perror("Error: The number of processes must be at least 3 (mapper, "
           "reducer, and at least one worker)\n");
    MPI_Finalize();
    exit(EXIT_FAILURE);
  }

  // Initialize the worker availabilities array
  int worker_availabilities[mpi_size];
  for (int i = 0; i < mpi_size; i++) {
    worker_availabilities[i] = 1;
  }

  // (Safe guard) Mark the mapper and reducer processes as busy
  worker_availabilities[MAPPER_RANK] = 0;
  worker_availabilities[REDUCER_RANK] = 0;

  // Parse command line arguments
  char *tests_directory_path = argv[1];
  int number_of_tests = atoi(argv[2]);

  if (mpi_rank == MAPPER_RANK) {
    // Master process is responsible for distributing tasks to workers - one
    // task is equivalent to searching for all the patterns in one text
    input_t **inputs =
        parse_all_input_files(tests_directory_path, number_of_tests);

    output_t **ref =
        parse_all_ref_files(tests_directory_path, inputs, number_of_tests);

    // Distribute the tasks to the workers
    for (int i = 0; i < number_of_tests; i++) {
      // Parse input parameters
      input_t *input = inputs[i];
      char *text = input->text;
      int n_patterns = input->n_patterns;
      char **patterns = input->patterns;

      int text_length = strlen(text);

      int is_task_assigned = 0;
      while (!is_task_assigned) {
        // Receives all worker availability updates from REDUCER_RANK
        int available_worker;
        int flag;
        while (MPI_Iprobe(REDUCER_RANK, 0, MPI_COMM_WORLD, &flag, &status) ==
                   MPI_SUCCESS &&
               flag) {
          MPI_Recv(&available_worker, 1, MPI_INT, REDUCER_RANK, 0,
                   MPI_COMM_WORLD, &status);
          worker_availabilities[available_worker] = 1;
        }

        // Get the index of the first available worker (skip MAPPER_RANK (0)
        // and REDUCER_RANK (1))
        int next_worker = -1;
        for (int i = 2; i < mpi_size; i++) {
          if (worker_availabilities[i]) {
            next_worker = i;
            break;
          }
        }

        if (next_worker != -1) {
          // Signal the next available worker that a new text is ready to be
          // processed by sending an UUID of the task
          MPI_Send(&i, 1, MPI_INT, next_worker, 0, MPI_COMM_WORLD);

          // Send the length of the text to the next available worker
          MPI_Send(&text_length, 1, MPI_INT, next_worker, 0, MPI_COMM_WORLD);

          // Send the text to the next available worker
          MPI_Send(text, text_length, MPI_CHAR, next_worker, 0, MPI_COMM_WORLD);

          // Send the number of patterns to the next available worker
          MPI_Send(&n_patterns, 1, MPI_INT, next_worker, 0, MPI_COMM_WORLD);

          // Send the patterns to the next available worker
          for (int pattern_idx = 0; pattern_idx < n_patterns; ++pattern_idx) {
            char *pattern = patterns[pattern_idx];
            int pattern_length = strlen(pattern);

            // Send the length of the current pattern to the next available
            // worker
            MPI_Send(&pattern_length, 1, MPI_INT, next_worker, 0,
                     MPI_COMM_WORLD);

            // Send the current pattern to the next available worker
            MPI_Send(pattern, pattern_length, MPI_CHAR, next_worker, 0,
                     MPI_COMM_WORLD);
          }

          // Mark the worker as unavailable
          MPI_Send(&next_worker, 1, MPI_INT, REDUCER_RANK, 0, MPI_COMM_WORLD);
          worker_availabilities[next_worker] = 0;

          is_task_assigned = 1;
        }
      }
    }

    // Notify all workers that the mapping is done
    for (int i = REDUCER_RANK + 1; i < mpi_size; i++) {
      MPI_Send(&MAPPING_DONE_MARKER_INT, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    }

    // Consume all worker availability updates from REDUCER_RANK; all updates
    // are processed when all workers become available
    int available_worker;
    while (check_if_any_worker_busy(worker_availabilities, mpi_size)) {
      MPI_Recv(&available_worker, 1, MPI_INT, REDUCER_RANK, 0, MPI_COMM_WORLD,
               &status);
      worker_availabilities[available_worker] = 1;
    }

    // Receive all results from REDUCER_RANK
    for (int i = 0; i < number_of_tests; ++i) {
      // Receive the number of patterns from REDUCER_RANK
      int n_patterns = 0;
      MPI_Recv(&n_patterns, 1, MPI_INT, REDUCER_RANK, 0, MPI_COMM_WORLD,
               &status);

      // Initialize output parameters
      output_t *output = (output_t *)(malloc(sizeof(output_t)));
      if (output == NULL) {
        perror("Error allocating memory for output");
        MPI_Finalize();
        exit(EXIT_FAILURE);
      }

      // Allocate memory for the identified patterns array
      output->identified_patterns =
          (pattern_w_idx_t **)(malloc(n_patterns * sizeof(pattern_w_idx_t *)));
      if (output->identified_patterns == NULL) {
        perror("Error allocating memory for output.identified_patterns\n");
        free(output);
        MPI_Finalize();
        exit(EXIT_FAILURE);
      }

      // Allocate memory for each identified pattern
      for (int i = 0; i < n_patterns; ++i) {
        output->identified_patterns[i] = alloc_pattern_w_idx();
        if (output->identified_patterns[i] == NULL) {
          perror("Error allocating memory for output.identified_patterns[i]\n");
          free(output->identified_patterns);
          free(output);
          MPI_Finalize();
          exit(EXIT_FAILURE);
        }
      }

      // Receive the identified patterns from REDUCER_RANK
      for (int pattern_idx = 0; pattern_idx < n_patterns; ++pattern_idx) {
        // Receive the length of the current pattern from REDUCER_RANK
        int pattern_length = 0;
        MPI_Recv(&pattern_length, 1, MPI_INT, REDUCER_RANK, 0, MPI_COMM_WORLD,
                 &status);

        // Receive the current pattern from REDUCER_RANK
        char pattern[pattern_length + 1];
        MPI_Recv(pattern, pattern_length, MPI_CHAR, REDUCER_RANK, 0,
                 MPI_COMM_WORLD, &status);

        // Place the null terminator at the end of the pattern
        pattern[pattern_length] = '\0';

        // Receive the number of times the current pattern has been identified
        // from REDUCER_RANK
        int pattern_occurrences = 0;
        MPI_Recv(&pattern_occurrences, 1, MPI_INT, REDUCER_RANK, 0,
                 MPI_COMM_WORLD, &status);

        // Receive the indexes of the current pattern from REDUCER_RANK
        int pattern_indexes[pattern_occurrences];
        MPI_Recv(pattern_indexes, pattern_occurrences, MPI_INT, REDUCER_RANK, 0,
                 MPI_COMM_WORLD, &status);

        // Store the received data in the output struct
        output->n_patterns = n_patterns;
        strcpy(output->identified_patterns[pattern_idx]->pattern, pattern);
        output->identified_patterns[pattern_idx]->len = pattern_occurrences;
        for (int i = 0; i < pattern_occurrences; i++) {
          output->identified_patterns[pattern_idx]->indexes[i] =
              pattern_indexes[i];
        }
      }

      // Check correctness
      const char *correctness =
          check_correctness(output, ref[i]) ? "FAILED" : "PASSED";
      printf("test %d: %s\n", i, correctness);
    }

    destroy_tests(inputs, ref, number_of_tests);
  } else if (mpi_rank == REDUCER_RANK) {
    // Reducer process is responsible for receiving the results from the workers
    // and combining them to produce the final result to be sent to MAPPER_RANK
    int files_processed = 0;

    // Initialize outputs
    output_t **outputs =
        (output_t **)(malloc(number_of_tests * sizeof(output_t *)));
    if (outputs == NULL) {
      perror("Error allocating memory for outputs");
      MPI_Finalize();
      exit(EXIT_FAILURE);
    }

    // Reducer's work is done when all workers are done and there are no more
    // patterns to be processed
    while (files_processed < number_of_tests) {
      // Receives all available messages from MAPPER_RANK
      int message = 0;
      int flag;
      while (MPI_Iprobe(MAPPER_RANK, 0, MPI_COMM_WORLD, &flag, &status) ==
                 MPI_SUCCESS &&
             flag) {
        MPI_Recv(&message, 1, MPI_INT, MAPPER_RANK, 0, MPI_COMM_WORLD, &status);
        worker_availabilities[message] = 0;
      }

      // For safety, make sure the next messages are not worker availability
      // updates from MAPPER_RANK; otherwise, we will receive them when we try
      // to receive the results from the workers
      while (MPI_Probe(MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status) ==
                 MPI_SUCCESS &&
             status.MPI_SOURCE == MAPPER_RANK) {
        MPI_Recv(&message, 1, MPI_INT, MAPPER_RANK, 0, MPI_COMM_WORLD, &status);
        worker_availabilities[message] = 0;
      }

      // Receive a new result from a worker
      // Receive the UUID of the task from the worker
      int task_uuid = 0;
      MPI_Recv(&task_uuid, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
               &status);

      // Receive the number of patterns from the worker
      int n_patterns = 0;
      MPI_Recv(&n_patterns, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD,
               &status);

      // Initialize output parameters
      output_t *output = (output_t *)(malloc(sizeof(output_t)));
      if (output == NULL) {
        perror("Error allocating memory for output");
        MPI_Finalize();
        exit(EXIT_FAILURE);
      }

      // Allocate memory for the identified patterns array
      output->identified_patterns =
          (pattern_w_idx_t **)(malloc(n_patterns * sizeof(pattern_w_idx_t *)));
      if (output->identified_patterns == NULL) {
        perror("Error allocating memory for output.identified_patterns\n");
        free(output);
        MPI_Finalize();
        exit(EXIT_FAILURE);
      }

      // Allocate memory for each identified pattern
      for (int i = 0; i < n_patterns; ++i) {
        output->identified_patterns[i] = alloc_pattern_w_idx();
        if (output->identified_patterns[i] == NULL) {
          perror("Error allocating memory for output.identified_patterns[i]\n");
          free(output->identified_patterns);
          free(output);
          MPI_Finalize();
          exit(EXIT_FAILURE);
        }
      }

      // Receive the identified patterns from the previous worker
      for (int pattern_idx = 0; pattern_idx < n_patterns; ++pattern_idx) {
        // Receive the length of the current pattern from the worker
        int pattern_length = 0;
        MPI_Recv(&pattern_length, 1, MPI_INT, status.MPI_SOURCE, 0,
                 MPI_COMM_WORLD, &status);

        // Receive the current pattern from the worker
        char pattern[pattern_length + 1];
        MPI_Recv(pattern, pattern_length, MPI_CHAR, status.MPI_SOURCE, 0,
                 MPI_COMM_WORLD, &status);

        // Place the null terminator at the end of the pattern
        pattern[pattern_length] = '\0';

        // Receive the number of times the current pattern has been identified
        // from the worker
        int pattern_occurrences = 0;
        MPI_Recv(&pattern_occurrences, 1, MPI_INT, status.MPI_SOURCE, 0,
                 MPI_COMM_WORLD, &status);

        // Receive the indexes of the current pattern from the worker
        int pattern_indexes[pattern_occurrences];
        MPI_Recv(pattern_indexes, pattern_occurrences, MPI_INT,
                 status.MPI_SOURCE, 0, MPI_COMM_WORLD, &status);

        // Store the received data in the output struct
        output->n_patterns = n_patterns;
        strcpy(output->identified_patterns[pattern_idx]->pattern, pattern);
        output->identified_patterns[pattern_idx]->len = pattern_occurrences;
        for (int i = 0; i < pattern_occurrences; i++) {
          output->identified_patterns[pattern_idx]->indexes[i] =
              pattern_indexes[i];
        }
      }

      // Store the output in the outputs array
      outputs[task_uuid] = output;

      // Increment the number of files processed
      ++files_processed;

      // Mark the worker as available
      worker_availabilities[status.MPI_SOURCE] = 1;

      // Notify MAPPER_RANK that the worker is available
      MPI_Send(&status.MPI_SOURCE, 1, MPI_INT, MAPPER_RANK, 0, MPI_COMM_WORLD);
    }

    // Send all results to MAPPER_RANK
    for (int i = 0; i < number_of_tests; i++) {
      // Send the number of patterns to MAPPER_RANK
      MPI_Send(&outputs[i]->n_patterns, 1, MPI_INT, MAPPER_RANK, 0,
               MPI_COMM_WORLD);

      // Send the identified patterns to MAPPER_RANK
      for (int pattern_idx = 0; pattern_idx < outputs[i]->n_patterns;
           ++pattern_idx) {
        pattern_w_idx_t *pattern_w_idx =
            outputs[i]->identified_patterns[pattern_idx];

        int pattern_length = strlen(pattern_w_idx->pattern);

        // Send the length of the current pattern to MAPPER_RANK
        MPI_Send(&pattern_length, 1, MPI_INT, MAPPER_RANK, 0, MPI_COMM_WORLD);

        // Send the current pattern to MAPPER_RANK
        MPI_Send(pattern_w_idx->pattern, strlen(pattern_w_idx->pattern),
                 MPI_CHAR, MAPPER_RANK, 0, MPI_COMM_WORLD);

        // Send the number of times the current pattern has been identified to
        // MAPPER_RANK
        MPI_Send(&pattern_w_idx->len, 1, MPI_INT, MAPPER_RANK, 0,
                 MPI_COMM_WORLD);

        // Send the indexes of the current pattern to MAPPER_RANK
        MPI_Send(pattern_w_idx->indexes, pattern_w_idx->len, MPI_INT,
                 MAPPER_RANK, 0, MPI_COMM_WORLD);
      }
    }

    MPI_Finalize();
    exit(EXIT_SUCCESS);
  } else {
    // Worker process is responsible for searching for the patterns in the text
    // received from MAPPER_RANK
    int mapper_signal = 0;
    while (1) {
      // Wait for a signal from MAPPER_RANK
      MPI_Recv(&mapper_signal, 1, MPI_INT, MAPPER_RANK, 0, MPI_COMM_WORLD,
               &status);
      if (mapper_signal == MAPPING_DONE_MARKER)
        break; // MAPPER_RANK marked the mapping as done; no more work to do
      else {
        // Received signal from MAPPER_RANK represents the UUID of the task
        int task_uuid = mapper_signal;

        // Receive the length of the text from MAPPER_RANK
        int text_length = 0;
        MPI_Recv(&text_length, 1, MPI_INT, MAPPER_RANK, 0, MPI_COMM_WORLD,
                 &status);

        // Receive the text from MAPPER_RANK
        char *text = (char *)(malloc((text_length + 1) * sizeof(char)));
        if (text == NULL) {
          perror("Error allocating memory for text");
          MPI_Finalize();
          exit(EXIT_FAILURE);
        }
        MPI_Recv(text, text_length, MPI_CHAR, MAPPER_RANK, 0, MPI_COMM_WORLD,
                 &status);

        // Place the null terminator at the end of the text
        text[text_length] = '\0';

        // Receive the number of patterns from MAPPER_RANK
        int n_patterns = 0;
        MPI_Recv(&n_patterns, 1, MPI_INT, MAPPER_RANK, 0, MPI_COMM_WORLD,
                 &status);

        // Receive the patterns from MAPPER_RANK
        char *patterns[n_patterns];
        for (int pattern_idx = 0; pattern_idx < n_patterns; ++pattern_idx) {
          // Receive the length of the current pattern from MAPPER_RANK
          int pattern_length = 0;
          MPI_Recv(&pattern_length, 1, MPI_INT, MAPPER_RANK, 0, MPI_COMM_WORLD,
                   &status);

          // Receive the current pattern from MAPPER_RANK
          char *pattern = (char *)(malloc((pattern_length + 1) * sizeof(char)));
          if (pattern == NULL) {
            perror("Error allocating memory for pattern");
            MPI_Finalize();
            exit(EXIT_FAILURE);
          }

          MPI_Recv(pattern, pattern_length, MPI_CHAR, MAPPER_RANK, 0,
                   MPI_COMM_WORLD, &status);

          // Place the null terminator at the end of the pattern
          pattern[pattern_length] = '\0';

          patterns[pattern_idx] = pattern;
        }

        // Initialize output parameters
        output_t *output = (output_t *)(malloc(sizeof(output_t)));
        if (output == NULL) {
          perror("Error allocating memory for output");
          MPI_Finalize();
          exit(EXIT_FAILURE);
        }

        output->n_patterns = n_patterns;

        // Allocate memory for the identified patterns array
        output->identified_patterns = (pattern_w_idx_t **)(malloc(
            n_patterns * sizeof(pattern_w_idx_t *)));
        if (output->identified_patterns == NULL) {
          perror("Error allocating memory for output.identified_patterns\n");
          free(output);
          MPI_Finalize();
          exit(EXIT_FAILURE);
        }

        // Allocate memory for each identified pattern
        for (int i = 0; i < n_patterns; ++i) {
          output->identified_patterns[i] = alloc_pattern_w_idx();
          if (output->identified_patterns[i] == NULL) {
            perror(
                "Error allocating memory for output.identified_patterns[i]\n");
            free(output->identified_patterns);
            free(output);
            MPI_Finalize();
            exit(EXIT_FAILURE);
          }
        }

        // Do the search for each pattern
        for (int pattern_idx = 0; pattern_idx < n_patterns; ++pattern_idx) {
          char *pattern = patterns[pattern_idx];
          int pattern_length = strlen(pattern);

          strcpy(output->identified_patterns[pattern_idx]->pattern, pattern);
          output->identified_patterns[pattern_idx]->len = 0;

          // Compute the hash of the current pattern
          int pattern_hash = compute_hash(pattern, pattern_length);

          // Move the sliding window over the text
          int sliding_points = text_length - pattern_length;
          for (int text_offset = 0; text_offset <= sliding_points;
               ++text_offset) {
            // Compute the hash of the current window
            int text_window_hash =
                compute_hash(text + text_offset, pattern_length);
            if (text_window_hash == pattern_hash) {
              int is_matching = is_pattern_matching(text, text_offset, pattern,
                                                    pattern_length);
              if (is_matching) {
                output->identified_patterns[pattern_idx]
                    ->indexes[output->identified_patterns[pattern_idx]->len++] =
                    text_offset;
              }
            }
          }
        }

        // Processing is done; send the output to REDUCER_RANK
        // Send the UUID of the task to REDUCER_RANK
        MPI_Send(&task_uuid, 1, MPI_INT, REDUCER_RANK, 0, MPI_COMM_WORLD);

        // Send the number of patterns to REDUCER_RANK
        MPI_Send(&n_patterns, 1, MPI_INT, REDUCER_RANK, 0, MPI_COMM_WORLD);

        // Send the identified patterns to REDUCER_RANK
        for (int pattern_idx = 0; pattern_idx < n_patterns; ++pattern_idx) {
          pattern_w_idx_t *pattern_w_idx =
              output->identified_patterns[pattern_idx];

          int pattern_length = strlen(pattern_w_idx->pattern);

          // Send the length of the current pattern to REDUCER_RANK
          MPI_Send(&pattern_length, 1, MPI_INT, REDUCER_RANK, 0,
                   MPI_COMM_WORLD);

          // Send the current pattern to REDUCER_RANK
          MPI_Send(pattern_w_idx->pattern, strlen(pattern_w_idx->pattern),
                   MPI_CHAR, REDUCER_RANK, 0, MPI_COMM_WORLD);

          // Send the number of times the current pattern has been identified to
          // REDUCER_RANK
          MPI_Send(&pattern_w_idx->len, 1, MPI_INT, REDUCER_RANK, 0,
                   MPI_COMM_WORLD);

          // Send the indexes of the current pattern to REDUCER_RANK
          MPI_Send(pattern_w_idx->indexes, pattern_w_idx->len, MPI_INT,
                   REDUCER_RANK, 0, MPI_COMM_WORLD);
        }

        // Free the memory allocated for the current output
        free_output_struct(output);
      }
    }

    MPI_Finalize();
    exit(EXIT_SUCCESS);
  }

  MPI_Finalize();
  return 0;
}
