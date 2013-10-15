#include "shell.h"

//name of profile config file
const char PROFILE_FILE_NAME[] = "profile";
//variable assignment special command
const char CMD_ASSIGN[] = "=";
//buffer for command parsing
const int MAX_COMMAND_LENGTH = 512;

typedef enum {
  PARSE_ESCAPE, 
  PARSE_STRING, 
  PARSE_NORMAL
} commandParseState_t; 

/*
 * parse a profile variable from a string
 * @param {char*} buffer the string to parse
 * @param {profileVar_t*} var where to return the parsed variable
 *  note that strings in var are dynamically allocated and need to
 *  be freed
 * @returns {var_t*} new environment variable or NULL
 */
var_t* parseVar(char *buffer) {
  //strip leading and trailing spaces and comments
  stripWhitespaces(buffer);
  char *comment = strchr(buffer, '#');
  if (comment != NULL)
    (*comment) = '\0';
  //parse string
  if (strlen(buffer) == 0)
    //empty string or comment, no variable parsed
    return NULL;
  char *var_delim = strchr(buffer, '=');
  if (var_delim == NULL) {
    return NULL;
  }
  var_t *var = malloc(sizeof(var_t));
  int length = (var_delim - buffer);
  var->name = malloc(sizeof(char) * length + 1);
  strncpy(var->name, buffer, length);
  var->name[length] = '\0';
  stripWhitespaces(var->name);
  length = strlen(var_delim + 1);
  var->value = malloc(sizeof(char) * length + 1);
  strncpy(var->value, var_delim + 1, length + 1);
  stripWhitespaces(var->value);
  return var;
}

/*
 * parse a command
 * @param {char*} buffer input string
 * @param {environment_t} env pointer to head of environment
 * variables list
 * @returns {command_t*} parsed command dynamically allocated
 */
command_t* parseCommand(char *buffer, environment_t env) {
  command_t *cmd = malloc(sizeof(command_t));
  //init command to avoid random behavior in case of error
  cmd->argc = 0;
  cmd->argv = NULL;
  cmd->exitVal = 0;
  cmd->output = NULL;
  cmd->error = NULL;
  //check if command inserted is a variable assignment
  var_t *var = parseVar(buffer);
  if (var != NULL) {
    cmd->argc = 3;
    cmd->argv = malloc(cmd->argc * sizeof(char*));
    cmd->argv[0] = malloc(sizeof(char) * strlen(CMD_ASSIGN));
    strcpy(cmd->argv[0], CMD_ASSIGN);
    cmd->argv[1] = var->name;
    cmd->argv[2] = var->value;
    free(var);
    return cmd;
  }
  //if it is an actual command
  /*parse command, arguments are put in a temporary list
   * and then moved into an array
   */
  typedef struct argv_list {
    struct argv_list* next;
    char *value;
  } argv_t;
  argv_t *list = NULL;
  //use a copy of buffer because strtok modifies its input
  char tmp[MAX_COMMAND_LENGTH];
  strcpy(tmp, buffer);
  commandParseState_t state = PARSE_NORMAL; //unused
  char *tok = strtok(tmp, " ");
  int argc = 0;
  while (tok != NULL) {
    argv_t *current;
    //insert token in the list
    if (list == NULL) {
      //empty list case
      list = malloc(sizeof(argv_t));
      current = list;
      current->next = NULL;
    }
    else {
      //append to list
      for(current = list; current->next != NULL; current = current->next);
      current->next = malloc(sizeof(argv_t));
      current->next->next = NULL;
      current = current->next;
    }
    current->value = malloc(sizeof(char) * strlen(tok));
    strcpy(current->value, tok);
    //increment argc
    argc++;
    //go on with parsing the string
    tok = strtok(NULL, " ");
  }
  //move the list to argv array
  cmd->argc = argc;
  cmd->argv = malloc(sizeof(char*) * cmd->argc);
  argv_t *current = list;
  int index = 0;
  while (current != NULL) {
    cmd->argv[index] = current->value;
    argv_t *tmp = current->next;
    free(current);
    current = tmp;
    index++;
  }
  //expand variables here @todo
  //expandVar(cmd);
  return cmd;
}

/*
 * prompt the user for a command
 * @param {environment_t} env evironment variables list
 * @param {char*} env list of environment variables
 */
void prompt(environment_t env, char *cmd) {
  var_t *home = getEnvVar(env, "HOME");
  printf("%s>", home->value);
  fgets(cmd, MAX_COMMAND_LENGTH, stdin);
}

/*
 * parse profile file
 * @param {environment_t} env environment variables list
 */
void parseProfile(environment_t env) {
  char buffer[1024];
  if (getcwd(buffer, sizeof(buffer)) == NULL) {
    printf("Error, could not get current working directory");
    exit(1);
  }
  char *profilePath;
  profilePath = malloc(sizeof(char) * 
		       (strlen(buffer) + strlen(PROFILE_FILE_NAME) + 1));
  memset(profilePath, '\0', sizeof(char) * 
	 (strlen(buffer) + strlen(PROFILE_FILE_NAME)) + 1);
  strcat(profilePath, buffer);
  strcat(profilePath, "/");
  strcat(profilePath, PROFILE_FILE_NAME);
  FILE *profileFile;
  //open profile file in current working dir
  profileFile = fopen(profilePath, "r");
  if (profileFile == NULL) {
    printf("Error, could not open profile file at %s\n", profilePath);
    free(profilePath);
    exit(1);
  }
  free(profilePath);
  //parse profile file content
  var_t *var = NULL;
  while (!feof(profileFile)) {
    fgets(buffer, sizeof(buffer), profileFile);
    var = parseVar(buffer);
    if (var == NULL) continue;
    if (var->name != NULL && var->value != NULL) {
      updateEnvVar(env, *var);
      free(var);
    }
    else {
      //unused var
      free(var->name);
      free(var->value);
      free(var);
    }
  }
}

/*
 * generate full path if file is found in path
 * @param {char*} file filename to look for
 * @param {var_t*} path path environment variable
 * @returns {char*} full path string dynamically allocated
 */
char* getFullPath(const char *file, var_t *path) {
  char *tmp_path = malloc(sizeof(char) * strlen(path->value));
  strcpy(tmp_path, path->value);
  char *full_path = NULL;
  char *tok = strtok(tmp_path, ":");
  while (tok != NULL) {
    DIR *dir = opendir(tok);
    if (dir == NULL) fatalError("PATH directory does not exist");
    struct dirent *dir_entity = readdir(dir);
    while (dir_entity != NULL) {
      if (strcmp(dir_entity->d_name, file) == 0) {
	//found file in this directory
	full_path = malloc(sizeof(char) * (strlen(file) + strlen(tok)+ 2));
	full_path[0] = '\0';
	strcat(full_path, tok);
	strcat(full_path, "/");
	strcat(full_path, file);
	break;
      }
      dir_entity = readdir(dir);
    }
    tok = strtok(NULL, ":");
  }
  return full_path;
}

void execCommand(command_t *cmd, environment_t env) {
  if (cmd->argc <= 0) {
    //empty or unparsed command
    return;
  }
  //check for special commands
  if (strcmp(cmd->argv[0], CMD_ASSIGN) == 0) {
    //variable assignment command
    var_t var;
    var.name = cmd->argv[1];
    var.value = cmd->argv[2];
    updateEnvVar(env, var);
  }
  //normal command execution
      char *full_path = getFullPath(cmd->argv[0], 
				    getEnvVar(env,"PATH"));
}

//couple of helpers to deal with environment variables

/*
 * create environment variables list and init it
 * @returns {environment_t}
 */
environment_t createEnv() {
  environment_t head = malloc(sizeof(profile_t*));
  *(head) = NULL;
  return head;
}

/*
 * check that PATH and HOME are set correctly
 * @param {environment_t} env environment var list
 * @returns {bool}
 */
bool checkShellEnv(environment_t env) {
  //particular env vars that are required
  var_t *home = getEnvVar(env, "HOME"); 
  var_t *path = getEnvVar(env, "PATH");
  //find PATH and HOME
  if (home == NULL || path == NULL) return false;
  if (opendir(home->value) == NULL)
    return false;
  char *tmp_path = malloc(sizeof(char) * strlen(path->value));
  strcpy(tmp_path, path->value);
  char *tok = strtok(tmp_path, ":");
  while (tok != NULL) {
    if (opendir(tok) == NULL) return false;
    tok = strtok(NULL, ":");
  }
  return true;
}

/*
 * get environment variable from name
 * @param {environment_t} env environment var list
 * @param {const char*} name name of the var to look for
 * @returns {var_t*} pointer to env var
 */
var_t* getEnvVar(environment_t env, const char *name) {
  profile_t *current = *(env);
  for (; current != NULL; current = current->next) {
    if (strcmp(current->var.name, name) == 0) {
      return &(current->var);
    }
  }
  return NULL;
}

/*
 * update environment variable or add unexisting one
 * @param {environment_t} env list of environment variables
 * @param {var_t} var variable structure
 */
void updateEnvVar(environment_t env, var_t var) {
if (var.name != NULL && var.value != NULL) {
  //search for already declared variable
  profile_t *found = *(env);
  for (; found != NULL; found = found->next) {
    if (strcmp(found->var.name, var.name) == 0) break;
  }
  if (found != NULL) {
    free(found->var.value);
    found->var.value = var.value;
  }
  else {
    //append var in the profileList
    profile_t *tmp = malloc(sizeof(profile_t));
    tmp->var.name = var.name;
    tmp->var.value = var.value;
    tmp->next = *(env);
    *(env) = tmp;
  }
 }
}
