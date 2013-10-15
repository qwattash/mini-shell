#include "error.h"

/*
 * fatal error, exit after reporting
 */
void fatalError(const char *message) {
  printf("[x]fatal: %s\n", message);
  exit(1);
}

void consoleError(const char *message) {
  printf("[-]console error: %s\n", message);
}
