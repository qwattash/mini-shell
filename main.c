#include "shell.h"


int main (int argc, char *argv[], char *envp[]) {
  
  profile_t *profileList = NULL;
  parseProfile(&profileList);
  //particular env vars that are required
  profile_t *home = NULL, *path = NULL;
  //find HOME
  profile_t *current = profileList;
  for (; current != NULL; current = current->next) {
    if (strcmp(current->var.name, "HOME") == 0) home = current;
    if (strcmp(current->var.name, "PATH") == 0) path = current;
  }
  if (home == NULL) fatalError("HOME variable is not defined");
  if (home == NULL) fatalError("PATH variable is not defined");
  //find PATH
  //main command listening loop
  command_t *cmd;
  char buffer[MAX_COMMAND_LENGTH];
  while (true) {
    prompt(home->var, buffer);
    cmd = parseCommand(buffer, &profileList);
    int i = 0;
    for (; i < cmd->argc; i++) {
      printf("arg %d: %s\n", i, cmd->argv[i]);
    }
    break;
    }
  exit(0);
}
