#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

typedef struct {
  char* program_name;
  char** args;
  uint8_t in_background;
} Command; 

Command* parse_command(char*); 

#endif
