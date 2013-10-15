#include "shell.h"

//name of profile config file
const char PROFILE_FILE_NAME[] = "profile";
//variable assignment special command
const char CMD_ASSIGN[] = "=";
const char CMD_CD[] = "cd";
const char CMD_EXIT[] = "exit";
//buffer for command parsing
const int MAX_COMMAND_LENGTH = 512;

//enum used to parse commands
typedef enum {
  PARSE_ESCAPE, 
  PARSE_STRING, 
  PARSE_NORMAL
} commandParseState_t; 

//list used to parse commands
typedef struct argv_list {
  struct argv_list* next;
  char *value;
} argv_t;

//"private" functions
var_t* parseVar(char *buffer);
void parseSpecial(command_t *cmd, environment_t env);
char* getFullPath(const char *file, var_t *path);
char* getCurrentWorkingDirectory();
int command2List(argv_t **head, char *cmdString);
void expandEnv(command_t *cmd, environment_t env);

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
  var->name = malloc(sizeof(char) * (length + 1));
  strncpy(var->name, buffer, length);
  var->name[length] = '\0';
  stripWhitespaces(var->name);
  length = strlen(var_delim + 1);
  var->value = malloc(sizeof(char) * (length + 1));
  strncpy(var->value, var_delim + 1, length + 1);
  stripWhitespaces(var->value);
  return var;
}

/*
 * parse special commands
 * @param {command_t*} cmd pre-parsed command with argv and envp set
 * @pram {environment_t} env environment variables list
 */
void parseSpecial(command_t *cmd, environment_t env) {
  if (strcmp(cmd->argv[0], CMD_CD) == 0) {
    //parse special meaning for no argument
    //parse special meaning for ~
    if (cmd->argc == 1 || 
	(cmd->argc == 2 && (strcmp(cmd->argv[1], "~") == 0))) 
      {
      var_t *home = getEnvVar(env, "HOME");
      free(cmd->argv[1]);
      cmd->argv[1] = malloc(sizeof(char) * 
			    (strlen(home->value) + 1));
      strcpy(cmd->argv[1], home->value);
    }
  }
}

/*
 * put command arguments in a temporary list
 * @param {argv_t**} head head of the list
 * @param {char*} buffer command to be parsed
 * @returns {int} arguments count
 */
int command2List(argv_t **head, char *buffer) {
  //use a copy of buffer because strtok modifies its input
  char tmp[MAX_COMMAND_LENGTH];
  strcpy(tmp, buffer);
  commandParseState_t state = PARSE_NORMAL; //unused
  char *tok = strtok(tmp, " ");
  int argc = 0;
  while (tok != NULL) {
    argv_t *current;
    //insert token in the list
    if (*(head) == NULL) {
      //empty list case
      *(head) = malloc(sizeof(argv_t));
      current = *(head);
      current->next = NULL;
    }
    else {
      //append to list
      for(current = *(head); current->next != NULL; current = current->next);
      current->next = malloc(sizeof(argv_t));
      current->next->next = NULL;
      current = current->next;
    }
    current->value = malloc(sizeof(char) * (strlen(tok) + 1));
    strcpy(current->value, tok);
    //increment argc
    argc++;
    //go on with parsing the string
    tok = strtok(NULL, " ");
  }
  return argc;
}

/*
 * parse a command
 * @param {char*} buffer input string
 * @param {environment_t} env pointer to head of environment
 * variables list
 * @returns {command_t*} parsed command <malloc>
 */
command_t* parseCommand(char *buffer, environment_t env) {
  command_t *cmd = malloc(sizeof(command_t));
  //init command to avoid random behavior in case of error
  cmd->argc = 0;
  cmd->argv = NULL;
  cmd->exitStatus = 0;
  //check if command inserted is a variable assignment
  var_t *var = parseVar(buffer);
  if (var != NULL) {
    cmd->argc = 3;
    cmd->argv = malloc(cmd->argc * sizeof(char*));
    cmd->argv[0] = malloc(sizeof(char) * (strlen(CMD_ASSIGN) + 1));
    strcpy(cmd->argv[0], CMD_ASSIGN);
    cmd->argv[1] = var->name;
    cmd->argv[2] = var->value;
    free(var);
    return cmd;
  }
  //if it is an actual command
  argv_t *list = NULL;
  cmd->argc = command2List(&list, buffer);
  //build argv array
  //last argv location is set to NULL
  cmd->argv = malloc(sizeof(char*) * (cmd->argc + 1));
  argv_t *current = list;
  int index = 0;
  while (current != NULL) {
    cmd->argv[index] = current->value;
    argv_t *tmp = current->next;
    free(current);
    current = tmp;
    index++;
  }
  cmd->argv[index] = NULL;
  //build envp array
  int envLength = 0;
  profile_t *env_var = *(env);
  for (; env_var != NULL; env_var = env_var->next) envLength++;
  cmd->envp = malloc(sizeof(char*) * (envLength + 1));
  index = 0;
  env_var = *(env);
  while (env_var != NULL) {
    cmd->envp[index] = malloc(sizeof(char) * 
			      (strlen(env_var->var.name) + 
			       strlen(env_var->var.value) + 
			       1));
    cmd->envp[index][0] = '\0';
    strcat(cmd->envp[index], env_var->var.name);
    strcat(cmd->envp[index], "=");
    strcat(cmd->envp[index], env_var->var.value);
    env_var = env_var->next;
    index++;
  }
  cmd->envp[index] = NULL;
  //parsing specific to special commands
  parseSpecial(cmd, env);
  //expand $ variables
  expandEnv(cmd, env);
  return cmd;
}

/*
 * prompt the user for a command
 * @param {environment_t} env evironment variables list
 * @param {char*} env list of environment variables
 */
void prompt(char *cmd) {
  char *cwd = getCurrentWorkingDirectory();
  if (cwd != NULL) 
    printf("%s>", cwd);
  else 
    printf("unknown>");
  fgets(cmd, MAX_COMMAND_LENGTH, stdin);
  free(cwd);
}

/*
 * parse profile file
 * @param {environment_t} env environment variables list
 */
void parseProfile(environment_t env) {
  char *cwd = getCurrentWorkingDirectory();
  if (cwd == NULL) {
    printf("Error, could not get current working directory");
    exit(1);
  }
  char *profilePath;
  profilePath = malloc(sizeof(char) * 
		       (strlen(cwd) + 
			strlen(PROFILE_FILE_NAME) + 1));
  profilePath[0] = '\0';
  strcat(profilePath, cwd);
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
  free(cwd);
  //parse profile file content
  var_t *var = NULL;
  char buffer[1024];
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
  //set cwd to home
  var_t *home = getEnvVar(env, "HOME");
  if (home != NULL) {
    chdir(home->value);
  }
  //else leave cwd as-is
}

/*
 * generate full path if file is found in path
 * @param {char*} file filename to look for
 * @param {var_t*} path path environment variable
 * @returns {char*} full path string <malloc>
 */
char* getFullPath(const char *file, var_t *path) {
  char *tmp_path = malloc(sizeof(char) * (strlen(path->value) + 1));
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
	full_path = malloc(sizeof(char) * 
			   (strlen(file) + strlen(tok) + 2));
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
    return;
  }
  if (strcmp(cmd->argv[0], CMD_CD) == 0) {
    chdir(cmd->argv[1]);
    return;
  }
  if (strcmp(cmd->argv[0], CMD_EXIT) == 0) {
    exit(0);
  }
  //normal command execution
  char *full_path = getFullPath(cmd->argv[0], getEnvVar(env,"PATH"));
  if (full_path == NULL) {
    consoleError("Unexisting command");
  }
  pid_t pid = fork();
  if (pid < 0) {
    //error
    fatalError("Fork failed while executing command");
  }
  else if (pid == 0){
    //child
    execve(full_path, cmd->argv, cmd->envp);
  }
  else {
    //parent
    waitpid(pid, &cmd->exitStatus, 0);
  }
  free(full_path);
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
  char *tmp_path = malloc(sizeof(char) * (strlen(path->value) + 1));
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

/*
 * expand $ variables between unescaped expansion operators {}
 * @param {command_t*} cmd expand variables in cmd->argv
 * @param {environment_t} env used to read variables
 */
void expandEnv(command_t *cmd, environment_t env) {
  p_string_t current_arg = cmd->argv;
  char *token = NULL;
  var_t *var = NULL;
  char *tmp = NULL;
  int pre_token_length = 0;
  while (*(current_arg) != NULL) {
    //look for multiple specal char $
    token = findUnescapedChar(*(current_arg), '$');
    while (token != NULL) {
      /*
       * calculate length before skipping the $
       * so when the string before $ will be copied 
       * the $ symbol will not be in
       */
      pre_token_length = (token - *(current_arg));
      token++;
      //if special symbol $ is found in the string
      if ((var = getEnvVar(env, token)) != NULL) {
	tmp = malloc(sizeof(char) * 
		     (pre_token_length + strlen(var->value) + 1));
	tmp[0] = '\0';
	strncpy(tmp, *(current_arg), pre_token_length);
	strcat(tmp, var->value);
	free(*(current_arg));
	*(current_arg) = tmp;
      }
      token = findUnescapedChar(token, '$');
    }
    current_arg++;
  }
}

//memory free helpers

void deleteCommand(command_t *cmd) {
  int i;
  p_string_t tmp = cmd->argv;
  while (*tmp != NULL) {
    free(*tmp);
    tmp = tmp + 1;
  }
  free(cmd->argv);
  tmp = cmd->envp;
  while (*tmp != NULL) {
    free(*tmp);
    tmp = tmp + 1;
  }
  free(cmd->envp);
  //@todo free other fields
  free(cmd);
}

/*
 * free environment variables list
 * @param {environment_t} env environment variable list
 */
void deleteEnv(environment_t env) {
  profile_t *current = *(env);
  while (current != NULL) {
    profile_t *tmp = current->next;
    free(current->var.name);
    free(current->var.value);
    free(current);
    current = tmp;
  }
  free(env);
}

//helper for getting current working directory
/*
 * get current working dir path independently from its size
 * @retruns {char*} cwd path <malloc>
 */
char* getCurrentWorkingDirectory() {
  const int size_step = 128;
  int current_size = size_step;
  char *cwd = malloc(sizeof(char) * size_step);
  while (getcwd(cwd, current_size) == NULL) {
    free(cwd);
    if (errno == ERANGE) {
      //cwd string is too big
      current_size += size_step;
      cwd = malloc(sizeof(char) * current_size);
    }
    else {
      return NULL;
    }
  }
  return cwd;
}
