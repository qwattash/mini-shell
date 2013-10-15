#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "util.h"
#include "error.h"

#ifndef SHELL_H
#define SHELL_H

typedef char** p_string_t;

typedef struct {
  char *name;
  char *value;
} var_t;

typedef struct profile_elem{
  struct profile_elem *next;
  var_t var;
} profile_t;

typedef struct {
  char *commandString;
  p_string_t *arguments;
  int exitVal;
  p_string_t *output;
  p_string_t *error;
} command_t;


/*
 * prompt the user and parse the command
 * @param {var_t} home home variable
 * @param {profile_t**} profileList environment variables list
 */
command_t* prompt(var_t home);

/*
 * parse profile file and fill given structure accordingly
 */
void parseProfile(profile_t **);
void execCommand(command_t*);

#endif
