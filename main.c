#include "shell.h"


int main (int argc, char *argv[], char *envp[]) {
  
  environment_t env = createEnv();
  parseProfile(env);
  //check that PATH and HOME exists and are valid
  if (!checkShellEnv(env)) 
    fatalError("Environment Variable PATH and HOME are incorrect");
  //main command listening loop
  command_t *cmd;
  char buffer[MAX_COMMAND_LENGTH];
  while (true) {
    prompt(env, buffer);
    cmd = parseCommand(buffer, env);
    int i = 0;
    for (; i < cmd->argc; i++) {
      printf("arg %d: %s\n", i, cmd->argv[i]);
    }
    execCommand(cmd, env);
    free(cmd);
  }
  exit(0);
}
