#include "shell.h"


int main (int argc, char *argv[], char *envp[]) {
  
  profile_t *profileList = NULL;
  parseProfile(&profileList);
  //check that PATH and HOME exists and are valid
  if (!checkShellEnv(profileList)) 
    fatalError("Environment Variable PATH and HOME are incorrect");
  //main command listening loop
  command_t *cmd;
  char buffer[MAX_COMMAND_LENGTH];
  while (true) {
    prompt(*(getEnvVar(profileList, "HOME")), buffer);
    cmd = parseCommand(buffer, &profileList);
    int i = 0;
    for (; i < cmd->argc; i++) {
      printf("arg %d: %s\n", i, cmd->argv[i]);
    }
    execCommand(cmd, getEnvVar(profileList, "PATH"));
    free(cmd);
  }
  exit(0);
}
