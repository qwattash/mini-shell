#include "shell.h"

const char PROFILE_FILE_NAME[] = "profile";
const int MAX_COMMAND_LENGTH = 512;

typedef enum {
  PARSE_ESCAPE, 
  PARSE_STRING, 
  PARSE_NORMAL
} commandParseState_t; 

/*
 * update environment variable or add unexisting one
 * @param {var_t} var variable structure
 * @param {profile_t*} profileList list of environment variables
 */
void updateVar(var_t var, profile_t **profileList) {
if (var.name != NULL && var.value != NULL) {
  //search for already declared variable
  profile_t *found = *(profileList);
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
    tmp->next = *(profileList);
    *(profileList) = tmp;
  }
 }
}


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

command_t* parseCommand(char *buffer, profile_t **profileList) {
  //check if command inserted is a variable assignment
  var_t *var = parseVar(buffer);
  if (var != NULL) {
    updateVar(*var, profileList);
    return NULL;
  }
  command_t *cmd = malloc(sizeof(command_t));
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
 * @param {var_t} home home evironment variable
 * @param {profile_t**} profileList list of environment variables
 */
void prompt(var_t home, char *cmd) {
  printf("%s>", home.value);
  fgets(cmd, MAX_COMMAND_LENGTH, stdin);
}

void parseProfile(profile_t **profileList) {
  char buffer[1024];
  if (getcwd(buffer, sizeof(buffer)) == NULL) {
    printf("Error, could not get current working directory");
    exit(1);
  }
  char *profilePath;
  profilePath = malloc(sizeof(char) * 
		       (strlen(buffer) + strlen(PROFILE_FILE_NAME)));
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
      updateVar(*var, profileList);
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

void execCommand(command_t *command) {}
