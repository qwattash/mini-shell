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
 * find first unescaped (\) char in the string, the char to look
 * for are passed as a string, this function leaves the given
 * string intact but returns the matched token or \0
 * this is a custom implementation of strtok
 * @param {char*} str str to be evaluated 
 * @param {const char*} delim chars to look for
 * @param {char*} match matching delimiter, \0 in case of no match 
 * or end of string
 * @returns {char*} pointer to matching token <malloc>
 */
char* nextUnescapedTok(char *str, const char *delim, char *match) {
  if (*str == '\0') {
    *match = '\0';
    return NULL;
  }
  char *end = str + (strlen(str));
  char *tmp = NULL;
  const char *curr_delim = delim;
  while (*(curr_delim) != '\0') {
    tmp = findUnescapedChar(str, *(curr_delim));
    if (tmp != NULL && end > tmp) end = tmp;
    curr_delim++;
  }
  //else return new string with token inside
  tmp = malloc(sizeof(char) * (end - str + 1));
  strncpy(tmp, str, (end - str));
  tmp[(end - str)] = '\0';
  *match = *end;
  return tmp;
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
    if (escaped) escaped = false;
    else {
      if (*str == '\\') escaped = true;
      else if (*str == c) return str;
    }
    str++;
  }
  return NULL;
}

/*
 * unescape (\) string
 * @param {char*} str str to be unescaped
 */
void unescape(char* str) {
  char *dst = str;
  char *curr = str;
  while (*(curr) != '\0') {
    if (*curr != '\\') *(dst++) = *curr;
    curr++;
  }
  *dst = '\0';
} 

//dynamic string helpers
/*
 * grows string to given size, copy old content into the new
 * buffer
 * @param {char*} str string to grow
 * @param {int} increment increment of size
 * @returns {char*} new string <malloc>
 */
char* strgrow(char* str, int increment) {
  char *tmp = NULL;
  if (str == NULL) {
    tmp = malloc(sizeof(char) * (increment + 1));
    tmp[0] = '\0';
  }
  else {
    tmp = malloc(sizeof(char) * 
		       (strlen(str) + increment + 1));
    strcpy(tmp, str);
  }
  return tmp;
}
