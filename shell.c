#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_NUM_SHORTCUT_COMMANDS 128

/*
 * Appends a prefix each time a user is prompted for an input and retrieves the user's input.
 */ 
ssize_t poll_user_input(const char* prompt, char** line, size_t* n) {
  fputs(prompt, stdout);
  return getline(line, n, stdin);
}

void fork_and_execute_with_io(Command* command, int in_fd, int out_fd) {
  pid_t pid = fork();
  if (pid == 0) {
    if (in_fd != STDIN_FILENO) {
      dup2(in_fd, STDIN_FILENO);
    }
    if (out_fd != STDOUT_FILENO) {
      dup2(out_fd, STDOUT_FILENO);
    }

    execvp(command->program_name, command->args);
    exit(EXIT_FAILURE);
  }
}

void execute_pipeline(Pipeline* pipeline) {
  int n = pipeline->n;
  int in_fd = STDIN_FILENO;
  int pipe_fd[2];

  for (int i = 0; i < n; i++) {
    if (i < n - 1) {
      pipe(pipe_fd);
    } else {
      pipe_fd[1] = STDOUT_FILENO; 
    }
    
    fork_and_execute_with_io(pipeline->commands[i], in_fd, (i < n - 1) ? pipe_fd[1] : STDOUT_FILENO);

    // In the parent, close the write end of the pipe
    if (i < n - 1) {
      close(pipe_fd[1]);
    }

    if (in_fd != STDIN_FILENO) {
      close(in_fd);
    }

    if (i < n - 1) {
      in_fd = pipe_fd[0];
    }

  }
 
  // If the last command is non-blocking assume the whole pipeline is non-blocking.
  for (int i = 0; i < n && !pipeline->commands[n - 1]->in_background; i++) {
    wait(NULL);
  } 
}

void deallocate_pipeline(Pipeline* pipeline) {
  for (int i = 0; i < pipeline->n; i++) {
    free(pipeline->commands[i]->args);
    free(pipeline->commands[i]);
  }
  free(pipeline);
}

/*
 * The shortcuts are stored in the ~/.shortcut_shell/ directory.
 */
char* get_shortcut_directory() {
  const char *home = getenv("HOME");
  if (!home) return NULL;
  char *dir = malloc(PATH_MAX);
  if (!dir) return NULL;
  snprintf(dir, PATH_MAX, "%s/.shortcut_shell", home);
  return dir;
}

void create_shortcut_directory() {
  char *dir = get_shortcut_directory();
  if (!dir) return;
  
  if (mkdir(dir, 0755) != -1) {
    printf("Created directory: %s\n", dir);
  } 
  free(dir);
}

int execute_pipeline_wrapper(Pipeline* pipeline) {
  
  // error if no pipeline, so we return -1
  if (!pipeline || !pipeline->commands[0] || !pipeline->commands[0]->program_name) {
    if (pipeline) {
      deallocate_pipeline(pipeline);  
    }
    return -1; 
  }
  
  // Builtin handling of cd, which requires us to change process's directory
  if (pipeline->n == 1 && strcmp(pipeline->commands[0]->program_name, "cd") == 0) {
    if (pipeline->commands[0]->args[1]) {
      chdir(pipeline->commands[0]->args[1]);
    }
  }
  else {
    execute_pipeline(pipeline); 
  }

  return 1;
}

int main() {

  create_shortcut_directory();

  char* line = NULL;
  size_t line_len = 0;
  Pipeline* pipeline;
  int recording = 0;
  char* recorded_commands[MAX_NUM_SHORTCUT_COMMANDS];
  int n_recorded = 0;

  while(poll_user_input("shortcut> ", &line, &line_len) > 0) {

    // Records commands for the shortcut
    if (strcmp(line, "r\n") == 0) {
      printf("Capturing commands...\n");
      recording = 1;
      n_recorded = 0;
      continue;
    }

    // Save a shortcut
    if (recording && strcmp(line, "s\n") == 0) {
      printf("Stopping capture. Enter shortcut name: ");
      char name[256];
      if (fgets(name, sizeof(name), stdin) == NULL) {
        recording = 0;
        continue;
      }
      name[strcspn(name, "\n")] = '\0';

      char *dir = get_shortcut_directory();
      if (!dir) {
        recording = 0;
        continue;
      }

      char filepath[PATH_MAX];
      snprintf(filepath, sizeof(filepath), "%s/%s", dir, name);
      free(dir);

      FILE *fp = fopen(filepath, "w");
      if (!fp) {
        recording = 0;
        continue;
      }

      for (int i = 0; i < n_recorded; i++) {
        fprintf(fp, "%s", recorded_commands[i]);
        free(recorded_commands[i]);
      }
      fclose(fp);
      printf("Shortcut saved to %s\n", filepath);
      recording = 0;
      n_recorded = 0;
      continue;
    }

    // Executes a shortcut
    if (line[0] == 'q' && line[1] == ' ') {
      char* shortcut_name = line + 2;
      shortcut_name[strcspn(shortcut_name, "\n")] = '\0';

      char* dir = get_shortcut_directory();
      if (!dir) {
        continue;
      }

      char filepath[PATH_MAX];
      snprintf(filepath, sizeof(filepath), "%s/%s", dir, shortcut_name);
      free(dir);

      FILE* fp = fopen(filepath, "r");
      if (!fp) {
        continue;
      }

      char command_line[MAX_COMMAND_BYTES];
      while (fgets(command_line, sizeof(command_line), fp)) {
        if (strlen(command_line) <= 1) continue;
        Pipeline* p = parse_pipeline(command_line);
        if (execute_pipeline_wrapper(pipeline) < 0) continue; 
        deallocate_pipeline(p);
      }
      fclose(fp);
      continue;
    }

    if (recording) {
      if (n_recorded < MAX_NUM_SHORTCUT_COMMANDS) {
        recorded_commands[n_recorded++] = strdup(line);
      } else {
        printf("Max number of commands reached!");
      }
      continue;
    }

    pipeline = parse_pipeline(line);
    if (execute_pipeline_wrapper(pipeline) < 0) continue;    
    deallocate_pipeline(pipeline);  
  }

  free(line);
  return 0;
}
