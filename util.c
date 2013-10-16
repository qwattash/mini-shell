#include "util.h"

/*
 * remove blanks from beginning and end of a string
 * @param {char*} str string to be elaborated
 */
void stripWhitespaces(char *str) {
  char *start = str;
  while (*start == ' ' || *start == '\t') start++;
  char *end = str + (strlen(str) - 1);
  while (*end == ' ' || *end == '\t' || *end == '\n') end--;
  *(++end) = '\0';
  strcpy(str, start);
}

/*
 * find first unescaped (\) char in the string
 * @param {char*} str str to be evaluated 
 * @param {const char} c char to look for
 * @returns {char*} pointer to matching location
 */
char* findUnescapedChar(char* str, const char c) {
  bool escaped = false;
  while (*(str) != '\0') {
    if (*str == '\\') escaped = !escaped;
    else escaped = false;
    if (!escaped && *str == c) return str;
    str++;
  }
  return NULL;
}

/*
 * unescape (\) string
 * @param {char*} str str to be unescaped
 */
void unescape(char* str) {
  bool escaped = false;
  while (str != NULL) {
    if (*str == '\\') escaped = !escaped;
    else escaped = false;
    if (escaped) {
      //means that escape char is found and must be stripped
      strncpy(str, str + 1, strlen(str + 1));
    }
  }
} 
