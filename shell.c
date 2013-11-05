#include "shell.h"

/*
 * @todos
 * [experimental] variable expansion
 * [experimental] quotes parsing and escape sequences
 * above features compiles but hang in a loop
 * [feat] pipes
 */

//name of profile config file
const char PROFILE_FILE_NAME[] = "profile";
//variable assignment special command
const char CMD_ASSIGN[] = "=";
const char CMD_CD[] = "cd";
const char CMD_EXIT[] = "exit";
//buffer for command parsing
const int MAX_COMMAND_LENGTH = 512;
/*
 * determine variables parsing behavior, variable names must start
 * with this symbol
 */
char OPTN_VAR_STARTER = '\0';

//list used to parse commands
typedef struct argv_list {
  struct argv_list* next;
  char *value;
} argv_t;

//parsing functions and data structures

typedef enum {
  PARSE_VAR,
  PARSE_STRING,
  PARSE_NORMAL
} parse_policy_t;

typedef struct State{
  //current parsing policy
  parse_policy_t policy;
  //previous parsing policy, used to come back to string policy
  //after expanding a var in a string
  parse_policy_t prev_policy;
  //list to which append new arguments
  argv_t **argv_list;
  //environment variables list
  environment_t env;
  //temporary parsing buffer for string parsing
  char *parsing_buffer;
  //number of arguments parsed so far
  unsigned int argc;
  //token currently processed
  char *current_token;
  //separator for the token currently processed
  char current_sep;
} parse_state_t;

//parsing functions for each policy
void parseVarState(parse_state_t*);
void parseNormalState(parse_state_t*);
void parseStringState(parse_state_t*);

//"private" functions
var_t* parseVar(char *buffer);
void parseSpecial(command_t *cmd, environment_t env);
char* getFullPath(const char *file, var_t *path);
char* getCurrentWorkingDirectory();
int command2List(argv_t **head, char *cmdString, environment_t env);
void expandEnv(command_t *cmd, environment_t env);
bool checkHome(char *home);
bool checkPath(char *path);

/*
 * parse a profile variable from a string
 * @param {char*} buffer the string to parse
 * @param {const char} pre charachter that notify the beginning of
 * the var name, in some shells there is none (here \0) in others
 * '$' is used
 * @returns {var_t*} new environment variable or NULL
 */
var_t* parseVar(char *buffer) {
  if (buffer == NULL) 
    return NULL;
  //strip leading and trailing spaces and comments
  stripWhitespaces(buffer);
  char *comment = strchr(buffer, '#');
  if (comment != NULL)
    (*comment) = '\0';
  //parse string
  if (strlen(buffer) == 0)
    //empty string or comment, no variable parsed
    return NULL;
  //if a variable starter symbol is defined
  if (OPTN_VAR_STARTER != '\0') {
    //check var name syntax
    char *var_starter = strchr(buffer, OPTN_VAR_STARTER);
    //note that if var_starter == NULL the error is catched anyway
    //because buffer can not be null at this point
    if (var_starter != buffer) { 
      //syntax of variable is incorrect
      return NULL;
    }
    else {
      //remove var starter and parse normally
      //strcpy manually because don't trust strcpy implementation
      //for copying on the same buffer
      char *dst = buffer;
      var_starter++;
      while (*var_starter != '\0')
	*(dst++) = *(var_starter++);
      *dst = '\0';
    }
  }
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
  //if command is valid
  if (cmd->argc == 0) 
    return;
  if (strcmp(cmd->argv[0], CMD_CD) == 0) {
    //parse special meaning for no argument
    //parse special meaning for ~
    if (cmd->argc == 1 || 
	(cmd->argc == 2 && (strcmp(cmd->argv[1], "~") == 0))) {
      var_t *home = getEnvVar(env, "HOME");
      if (cmd->argc == 2) {
	free(cmd->argv[1]);
      }
      else {
	cmd->argc = 2;
	p_string_t tmp = malloc(sizeof(char*) * 3);
	tmp[0] = cmd->argv[0];
	tmp[1] = NULL;
	tmp[2] = NULL;
	free(cmd->argv);
	cmd->argv = tmp;
      }
      cmd->argv[1] = malloc(sizeof(char) * 
      			    (strlen(home->value) + 1));
      strcpy(cmd->argv[1], home->value);
    }
  }
}

/*
 * add token to argv list
 * @param {char*} str string to add
 * @param {argv_t**} head head of the list
 */
void addToken(char *str, argv_t **head) {
  if (strlen(str) == 0) 
    return;
  //use temporary string to avoid modifying str
  char *tmp = malloc(sizeof(char) * strlen(str));
  strcpy(tmp, str);
  //unescape given argument
  unescape(tmp);
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
    //iterate to the end
    for(current = *(head); current->next != NULL; current = current->next);
    //insert at the end
    current->next = malloc(sizeof(argv_t));
    current->next->next = NULL;
    current = current->next;
  }
  //copy value string to new malloc string buffer
  current->value = malloc(sizeof(char) * (strlen(str) + 1));
  strcpy(current->value, tmp);
  free(tmp);
}

/*
 * function for parsing of tokens that match the separator '"'
 * @param {parse_state_t*} current parsing state
 */
void parseNormalState(parse_state_t* state) {
  printf("normal: %s\n", state->current_token);
  //add token as an argument and change handler
  addToken(state->current_token, state->argv_list);
  state->prev_policy = PARSE_NORMAL;
  switch (state->current_sep) {
  case '"':
    state->policy = PARSE_STRING;
    break;
  case '$':
    state->policy = PARSE_VAR;
    break;
  case ' ':
    //stay in the same state
    break;
  }
}

/*
 * function for parsing a var after the $ separator
 * @param {parse_state_t*} current parsing state
 */
void parseVarState(parse_state_t* state) {
  printf("var: %s\n", state->current_token);
  var_t *var = getEnvVar(state->env, state->current_token);
  if (state->prev_policy == PARSE_STRING) {
    //save old buffer pointer
    char* tmp_free_holder = state->parsing_buffer;
    int grow_length = strlen(var->value);
    //add extra space for the space
    if (state->current_sep == ' ')
      grow_length++;
    //grow buffer size, old content is copied automatically
    state->parsing_buffer = strgrow(state->parsing_buffer,
				    grow_length);
   //free old string
   if (tmp_free_holder != NULL) 
     free(tmp_free_holder);
   //append var to parsed string
   strcat(state->parsing_buffer, var->value);
   //preserve space sparator
   if (state->current_sep == ' ')
     strcat(state->parsing_buffer, " ");
   }
  else {
    //add var as arg
    if (var != NULL) addToken(var->value, state->argv_list);
  }
  switch (state->current_sep) {
  case '"':
    if (state->prev_policy == PARSE_STRING) {
      state->policy = PARSE_NORMAL;
      //flush parsing buffer
      addToken(state->parsing_buffer, state->argv_list);
      free(state->parsing_buffer);
      state->parsing_buffer = NULL;
    }
    else {
      state->policy = PARSE_STRING;
    }
    state->prev_policy = PARSE_VAR;
    break;
  case ' ':
    //switch back to previous state
    state->prev_policy = PARSE_VAR;
    if (state->prev_policy = PARSE_STRING) {
      state->policy = PARSE_STRING;
    }
    else {
      state->policy = PARSE_NORMAL;
    }
  }
}

/*
 * function for parsing of tokens that follow a " char
 * @param {parse_state_t*} current parsing state
 */
void parseStringState(parse_state_t* state) {
  printf("string: '%s'\n", state->current_token);
  //append token to parsing_buffer
  //save old buffer pointer
  char* tmp_free_holder = state->parsing_buffer;
  int grow_length = strlen(state->current_token);
  //add extra space for the space
  if (state->current_sep == ' ')
    grow_length++;
  //grow buffer size, old content is copied automatically
  state->parsing_buffer = strgrow(state->parsing_buffer,
				  grow_length);
  //free old string
  if (tmp_free_holder != NULL) 
    free(tmp_free_holder);
  //append var to parsed string
  strcat(state->parsing_buffer, state->current_token);
  if (state->current_sep == ' ')
    strcat(state->parsing_buffer, " ");
  switch (state->current_sep) {
  case '"':
    //end of string, flush buffer
    state->prev_policy = PARSE_STRING;
    state->policy = PARSE_NORMAL;
    addToken(state->parsing_buffer, state->argv_list);
    //free temporary buffer
    free(state->parsing_buffer);
    state->parsing_buffer = NULL;
    break;
  case '$':
    //switch to parsing variable inside string
    state->prev_policy = PARSE_STRING;
    state->policy = PARSE_VAR;
    break;
  }
}

/*
 * put command arguments in a temporary list
 * @param {argv_t**} head head of the list
 * @param {char*} buffer command to be parsed
 * @returns {int} arguments count
 */
int command2List(argv_t **head, char *buffer, environment_t env) {
  //init parsing state
  parse_state_t state;
  state.policy = PARSE_NORMAL;
  state.prev_policy = PARSE_NORMAL;
  state.argv_list = head;
  state.env = env;
  state.parsing_buffer = NULL;
  state.argc = 0;
  state.current_token = NULL;
  state.current_sep = '\0';
  //begin parsing
  //token string <malloc> by findFirstUnescapedChar
  //note that token refers to everything BEFORE the separator
  state.current_token = nextUnescapedTok(buffer, " \"$", 
					 &state.current_sep);
  while(state.current_token != NULL) {
    //handle parsing of current token
    switch (state.policy) {
    case PARSE_NORMAL:
      parseNormalState(&state);
      break;
    case PARSE_STRING:
      parseStringState(&state);
      break;
    case PARSE_VAR:
      parseVarState(&state);
      break;
    }
    buffer += strlen(state.current_token); //skip token length
    if (state.current_sep != '\0') buffer++; //skip separator char
    state.current_token = nextUnescapedTok(buffer, " \"$", 
					   &state.current_sep);
  }
  //check that parsing have finished correctly
  if (state.policy == PARSE_STRING) {
    consoleError("Syntax Error");
    return 0;
  }
  //DEBUGG
  argv_t *c = *(head);
  int argc = 0;
  for (; c != NULL; c = c->next) {
    printf("[d] arg %s\n", c->value);
    argc++;
  }
  //END DEBUG
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
  cmd->envp = NULL;
  cmd->exitStatus = 0;
  //check if command inserted is a variable assignment
  var_t *var = parseVar(buffer);
  if (var != NULL) {
    cmd->argc = 4; 
    //one argv slot is for NULL pointer signalisng 
    //the end of the argv array
    cmd->argv = malloc(cmd->argc * sizeof(char*));
    cmd->argv[0] = malloc(sizeof(char) * (strlen(CMD_ASSIGN) + 1));
    *(cmd->argv[0]) = '\0';
    strcpy(cmd->argv[0], CMD_ASSIGN);
    cmd->argv[1] = var->name;
    cmd->argv[2] = var->value;
    cmd->argv[3] = NULL;
    free(var);
    return cmd;
  }
  //if it is an actual command
  argv_t *list = NULL;
  cmd->argc = command2List(&list, buffer, env);
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
  //expandEnv(cmd, env);
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
    //if assigning to HOME or PATH verify assigment first
    if (strcmp(cmd->argv[1], "HOME") == 0 && 
	! checkHome(cmd->argv[2])) {
      consoleError("invalid HOME");
      return;
    }
    if (strcmp(cmd->argv[1], "PATH") == 0 &&
	! checkPath(cmd->argv[2])) {
      consoleError("invalid PATH");
      return;
    }
    //variable assignment command
    var_t var;
    //copy vars from cmd so if cmd is free'd it doesn't matter
    var.name = malloc(sizeof(char) * (strlen(cmd->argv[1]) + 1));
    strcpy(var.name, cmd->argv[1]);
    var.value = malloc(sizeof(char) * (strlen(cmd->argv[2]) + 1));
    strcpy(var.value, cmd->argv[2]);
    updateEnvVar(env, var);
    return;
  }
  if (strcmp(cmd->argv[0], CMD_CD) == 0) {
    if(chdir(cmd->argv[1]) == -1) {
      //error
      consoleError("can not cd there!");
    }
    return;
  }
  if (strcmp(cmd->argv[0], CMD_EXIT) == 0) {
    int exitval = 0;
    if (cmd->argc == 2) exitval = atoi(cmd->argv[1]);
    exit(exitval);
  }
  //normal command execution
  char *full_path = getFullPath(cmd->argv[0], getEnvVar(env,"PATH"));
  if (full_path == NULL) {
    consoleError("Unexisting command");
    return;
  }
  //fork
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
  if (! checkHome(home->value)) return false;
  if (! checkPath(path->value)) return false;
  return true;
}

/*
 * check that HOME var is set to a valid value
 * @param {char*} home pointer to env var HOME value
 * @returns {boolean}
 */
bool checkHome(char *home) {
  if (home != NULL && opendir(home) != NULL)
    return true;
  return false;
}

/*
 * check that PATH var is set to a valid value
 * @param {char*} path pointer to env var PATH value
 * @returns {boolean}
 */
bool checkPath(char *path) {
  //strtok modify its input so make a copy
  char *tmp_path = malloc(sizeof(char) * (strlen(path) + 1));
  strcpy(tmp_path, path);
  char *tok = strtok(tmp_path, ":");
  bool ok = true;
  while (tok != NULL) {
    if (opendir(tok) == NULL) {
      ok = false;
      break;
    }
    tok = strtok(NULL, ":");
  }
  free(tmp_path);
  return ok;
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
  p_string_t tmp;
  if (cmd->argv != NULL) {
    tmp = cmd->argv;
    while (*tmp != NULL) {
      free(*tmp);
      tmp = tmp + 1;
    }
    free(cmd->argv);
  }
  if (cmd->envp != NULL) { 
    tmp = cmd->envp;
    while (*tmp != NULL) {
      free(*tmp);
      tmp = tmp + 1;
    }
    free(cmd->envp);
  }
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
