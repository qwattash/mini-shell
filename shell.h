#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

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

typedef profile_t** environment_t;

extern const int MAX_COMMAND_LENGTH;

/*
 * prompt the user and parse the command
 * @param {environment_t} env environment variables list
 * @param {char*} buffer where to put command string
 */
void prompt(environment_t env, char *buffer);

/*
 * parse profile file and fill given structure accordingly
 * @param {environment_t} environment list
 */
void parseProfile(environment_t);

/*
 * parse shellCommand
 * @param {char*} buffer input command to parse
 * @param {profile_t*} env list of environment variables 
 * for expansion
 * @returns {command_t*} parsed command
 */
command_t* parseCommand(char *buffer, environment_t env);

/*
 * execute given command
 * @param {command_t*} cmd command to execute
 * @param {environment_t} env environment var list
 */
void execCommand(command_t *cmd, environment_t env);

//utilities for env variables handling

/*
 * check that PATH and HOME are set correctly
 * @param {environment_t} env environment variables list
 * list
 * @returns {bool}
 */
bool checkShellEnv(environment_t env);

/*
 * get environment variable from name
 * @param {environment_t} env environment var list
 * @param {const char*} name name of the var to look for
 * @returns {var_t*} pointer to env var
 */
var_t* getEnvVar(environment_t env, const char *name);

/*
 * update environment variable or add unexisting one
 * @param {environment_t} env list of environment variables
 * @param {var_t} var variable structure
 */
void updateEnvVar(environment_t env, var_t var);

/*
 * create environment variables list and init it
 * @returns {environment_t}
 */
environment_t createEnv();

#endif
