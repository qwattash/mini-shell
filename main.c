#include "shell.h"
#include "error.h"


int main (int argc, char *argv[], char *envp[]) {
  
  profile_t *profileList = NULL;
  parseProfile(&profileList);
  //particular env vars that are required
  profile_t *home, *path;
  //find HOME
  profile_t *current = profileList;
  for (; current != NULL; current = current->next) {
    if (strcmp(home->var.name, "HOME") == 0) home = current;
    if (strcmp(home->var.name, "PATH") == 0) path = current;
  }
  if (home == NULL) fatalError("HOME variable is not defined");
  if (home == NULL) fatalError("PATH variable is not defined");
  //find PATH
  //main command listening loop
  command_t *cmd;
  while (true) {
    cmd = prompt(home, &profileList);
    break;
    }
  exit(0);
}
