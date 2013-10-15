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
  p_string_t argv;
  int argc;
  int exitVal;
  p_string_t output;
  p_string_t error;
} command_t;

extern const int MAX_COMMAND_LENGTH;

/*
 * prompt the user and parse the command
 * @param {var_t} home home variable
 * @param {char*} buffer where to put command string
 */
void prompt(var_t home, char *buffer);

/*
 * parse profile file and fill given structure accordingly
 */
void parseProfile(profile_t **);

/*
 * parse shellCommand
 * @param {char*} buffer input command to parse
 * @param {profile_t*} profileList list of environment variables 
 * for expansion
 * @returns {command_t*} parsed command
 */
command_t* parseCommand(char *buffer, profile_t **profileList);
void execCommand(command_t*);

#endif
