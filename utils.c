/*********************************************************
*
* utils.c - utilities for grafit - graphite NEB module
*
*	ehl0 (Laszlo Kollar)
*    http://kollarlaszlo.hu
*    daddy@kollarlaszlo.hu)
*
*
***********************************************************/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>

#define FALSE 0
#define TRUE  1

#include "utils.h"

#include "../include/common.h"

/* replace a character by another one */
void replace_char(char *str, char c, char r)
{
  char *pos;
	if(str != NULL)
	  while( (pos = strchr(str,c)) )
			if(pos !=NULL || *pos!='\0')
				*pos = r;
}

/* remove a specific character from a string */
void remchar(char *str, char c)
{
  char *pos;
	if(str !=NULL)
	  while( (pos = strchr(str,c)) )
  	  memmove(pos, pos + 1, strlen(pos));
}

char *strip_metric(char *str)
{
  char *p=str;
  char *begin=str;
  char *tempstr=NULL;
  char d=0;

  while(*p!='\0')
  {
    if(isdigit(*p) || *p=='.' || *p=='-')
      p++;
    else
      break;
  }
  tempstr=strndup(begin,p-begin);
  return tempstr;
}

void parse_quote(char *str,char delim)
{
	int 		in_quote=FALSE;

	while(*str != '\0')
	{
		if(*str==delim)
		{
			if(in_quote)
				in_quote=FALSE;
			else
				in_quote=TRUE;
			str++;
			continue;
		}
		if(in_quote)
			if(isspace(*str))
				*str='_';
		str++;
	}	
}

/* tokenize a string -- return a string array*/
char **split(char *string, char delim){
  char **tokens=NULL;
  int i=0;
  char *begin;
  char *p=NULL;

	if(string==NULL) return NULL;

  tokens  = malloc(sizeof(char *) * MAXTOKENS);
  if(tokens)
    for(i = 0; i < MAXTOKENS; i++)
      tokens[i] = NULL;

  i=0;
	p=string;
  begin=p;
  while(*p != '\0')
  {
    if(*p==delim){
      tokens[i]=strndup(begin,p-begin);
      i++;
      p++;
      begin=p;
    }
    else{
      p++;
    }
  }
  tokens[i++]=strndup(begin,p-begin);
  tokens[i]=NULL;
  return tokens;
}

/* free the token array */
void freetokens(char **tokens){
    int idx;
		if(tokens==NULL) return;

    for(idx = 0; *(tokens + idx); idx++){
      if(tokens[idx]){
        free(tokens[idx]);
      }
    }
    if(tokens)
      free(tokens);
}

