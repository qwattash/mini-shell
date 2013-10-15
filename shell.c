#include "shell.h"
#include "util.h"

const char PROFILE_FILE_NAME[] = "profile";
const int MAX_COMMAND_LENGTH = 512;

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
 */
bool parseVar(char *buffer, var_t *var) {
  var->name = NULL;
  var->value = NULL;
  //strip leading and trailing spaces and comments
  stripWhitespaces(buffer);
  char *comment = strchr(buffer, '#');
  if (comment != NULL)
    (*comment) = '\0';
  //parse string
  if (strlen(buffer) == 0)
    //empty string or comment, no variable parsed
    return true;
  char *var_delim = strchr(buffer, '=');
  if (var_delim == NULL) {
    return false;
  }
  int length = (var_delim - buffer);
  var->name = malloc(sizeof(char) * length + 1);
  strncpy(var->name, buffer, length);
  var->name[length] = '\0';
  stripWhitespaces(var->name);
  length = strlen(var_delim + 1);
  var->value = malloc(sizeof(char) * length + 1);
  strncpy(var->value, var_delim + 1, length + 1);
  stripWhitespaces(var->value);
  return true;
}

command_t* parseCommand(char *buffer) {
  


}

command_t *prompt(var_t home, profile_t **profileList) {
  char buffer[MAX_COMMAND_LENGTH + 1];
  var_t var;
  command_t *cmd;
  printf("%s>", home.value);
  fgets(buffer, MAX_COMMAND_LENGTH, stdin);
  if (parseVar(buffer, &var)) {
    //if user entered variable assignment
    updateVar(var, profileList);
  }
  else if((cmd = parseCommand(buffer)) != NULL){
    return NULL;
  }
  else {
    //error
  }
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
    printf("Error,could not open profile file at %s\n", profilePath);
    free(profilePath);
    exit(1);
  }
  free(profilePath);
  //parse profile file content
  var_t var;
  while (!feof(profileFile)) {
    fgets(buffer, sizeof(buffer), profileFile);
    if (! parseVar(buffer, &var)) {
      //error while parsing
      printf("Error parsing profile file");
      exit(1);
    };
    if (var.name != NULL && var.value != NULL) {
      updateVar(var, profileList);
    }
    else {
      //unused var
      free(var.name);
      free(var.value);
    }
  }
}

void execCommand(command_t *command) {}
