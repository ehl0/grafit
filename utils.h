/*********************************************************
*
* utils.h - header file for utils.c
*
*	ehl0 (Laszlo Kollar)
*    http://kollarlaszlo.hu
*    daddy@kollarlaszlo.hu)
*
***********************************************************/

#define MAXBUF 2048 
#define MAXTOKENS 1024
#define FALSE 0
#define TRUE  1

void replace_char(char *str, char c, char r);
void remchar(char *str, char c);
void parse_quote(char *str,char delim);
char **split(char *input, char delim);
void freetokens(char **tokens);
char *concat(const char *s1, ...);
char *strip_metric(char *str);
