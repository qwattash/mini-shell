#include "shell.h"


int main (int argc, char *argv[], char *envp[]) {
  environment_t env = createEnv();
  parseProfile(env);
  OPTN_VAR_STARTER = '$';
  //check that PATH and HOME exists and are valid
  if (!checkShellEnv(env)) 
    fatalError("Environment Variable PATH and HOME are incorrect");
  //main command listening loop
  command_t *cmd;
  char buffer[MAX_COMMAND_LENGTH];
  while (true) {
    prompt(buffer);
    cmd = parseCommand(buffer, env);
    execCommand(cmd, env);
    deleteCommand(cmd);
  }
  deleteEnv(env);
  exit(0);
}
