#include "utils.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

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

int main() {

  char* line = NULL;
  size_t line_len = 0;
  Pipeline* pipeline;

  while(poll_user_input("shortcut> ", &line, &line_len) > 0) {
    pipeline = parse_pipeline(line);
    if (!pipeline || !pipeline->commands[0] || !pipeline->commands[0]->program_name) {
      if (pipeline) {
        deallocate_pipeline(pipeline);  
      }
      continue;
    }

    if (pipeline->n == 1 && strcmp(pipeline->commands[0]->program_name, "cd") == 0) {
      if (pipeline->commands[0]->args[1]) {
        chdir(pipeline->commands[0]->args[1]);
      }
    }
    else {
      execute_pipeline(pipeline); 
    }
    deallocate_pipeline(pipeline);  
  }

  return 0;
}
