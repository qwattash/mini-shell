#include <string.h>
#include <stdbool.h>

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
