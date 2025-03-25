#include "utils.h"
#include <stdio.h>

/*
 * Appends a prefix each time a user is prompted for an input and retrieves the user's input.
 */ 
ssize_t poll_user_input(const char* prompt, char** line, size_t* n) {
  fputs(prompt, stdout);
  return getline(line, n, stdin);
}

int main() {

  char* line = NULL;
  size_t line_len = 0;

  while(poll_user_input("shortcut> ", &line, &line_len) > 0) {
     printf(line, "\n");
  }

  return 0;
}
