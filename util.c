#include "util.h"

void stripWhitespaces(char *str) {
  char *start = str;
  while (*start == ' ' || *start == '\t') start++;
  char *end = str + (strlen(str) - 1);
  while (*end == ' ' || *end == '\t' || *end == '\n') end--;
  *(++end) = '\0';
  strcpy(str, start);
}
