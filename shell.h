#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

typedef char** lpString;

typedef struct {
  lpString path;
  char *home;
} profile_t;

typedef struct {
  char *commandString;
  lpString *arguments;
  int exitVal;
  lpString *output;
  lpString *error;
} command_t;

command_t* prompt();
void parseProfile(profile_t*);
void exec(command_t*);
