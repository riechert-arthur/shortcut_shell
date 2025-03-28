#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

#define MAX_COMMAND_BYTES 1024

typedef struct {
  char* program_name;
  char** args;
  uint8_t in_background;
} Command;

typedef struct {
  uint8_t n;
  Command* commands[];
} Pipeline;

Command* parse_command(char*); 
Pipeline* parse_pipeline(char*);

#endif
