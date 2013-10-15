#include "error.h"

/*
 * fatal error, exit after reporting
 */
void fatalError(const char *message) {
  printf("[-]fatal: %s", message);
  exit(1);
}

void consoleError(const char *message) {
  printf("[-]console error: %s", message);
}
