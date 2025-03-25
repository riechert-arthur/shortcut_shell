#ifndef UTILS_H
#define UTILS_H

typedef struct {
  char* program_name;
  char** args;
} Command; 

Command* parse_command(char*); 

#endif
