#include "utils.h"
#include <string.h>
#include <stdlib.h>

#define MAX_COMMAND_BYTES 1024
#define DELIMITERS " \t\n\r"

/*
 * Returns the next non-empty token based on each byte in DELIMITERS. If no tokens remain, it
 * returns NULL.
 */
char* next_non_empty_token(char** line) {
  char* token;

  while ((token = strsep(line, DELIMITERS)) && !*token);
 
  return token;
}

Command* parse_command(char* command_str) {
  char* command_str_copy = strndup(command_str, MAX_COMMAND_BYTES);
  char* token;
  int i = 0;

  Command* command;
  if (!(command = (Command*) malloc(sizeof(Command)))) {
    return NULL;
  }

  if (!(command->args = (char**) calloc(MAX_COMMAND_BYTES, sizeof(char*)))) {
    free(command);
    return NULL;
  }

  while ((token = next_non_empty_token(&command_str_copy))) {
    command->args[i++] = token;
  }
  command->program_name = command->args[0];
  command->in_background = 0;

  if (i > 0 && strcmp(command->args[i - 1], "&") == 0) {
    command->in_background = 1;
    command->args[i - 1] = NULL;
  }

  return command;
}
