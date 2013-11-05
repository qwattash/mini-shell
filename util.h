#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
 * remove blanks from beginning and end of a string
 * @param {char*} str string to be elaborated
 */
void stripWhitespaces(char * str);

/*
 * find first unescaped (\) char in the string
 * @param {char*} str str to be evaluated 
 * @param {const char} c char to look for
 * @returns {char*} pointer to matching location
 */
char* findUnescapedChar(char* str, const char c);

/*
 * find first unescaped (\) char in the string, the char to look
 * for are passed as a string, this function leaves the given
 * string intact but returns an additional length of token int
 * @param {char*} str str to be evaluated 
 * @param {const char*} delim chars to look for
 * @param {char*} match where to pot matching char
 * @returns {char*} pointer to matching token <malloc>
 */
char* nextUnescapedTok(char *str, const char *delim, char *match);

/*
 * unescape (\) string
 * @param {char*} str str to be unescaped
 */
void unescape(char* str);

//dynamic string helpers
/*
 * grows string to given size
 * @param {char*} str string to grow
 * @param {int} increment increment of size
 * @returns {char*} new string <malloc>
 */
char* strgrow(char* str, int increment);
