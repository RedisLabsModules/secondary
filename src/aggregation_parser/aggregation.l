%{

#include "token.h"
#include "parser.h"
#include <string.h>
#include "../rmutil/alloc.h"

Token tok;

%}

%%

\$[0-9]{1} {    
    tok.intval = atoi(yytext+1); 
    return ENUMERATOR; 
}


[\-\+]?[0-9]*\.[0-9]+    {  
    tok.dval = atof(yytext); 
    return FLOAT; 
}

[\-\+]?[0-9]+    {   
  tok.intval = atoi(yytext); 
  return INTEGER; 
}

(\"(\\.|[^\"])*\")|('(\\.|[^'])*')    {
  /* String literals, with escape sequences - enclosed by "" or '' */
  *(yytext+strlen(yytext)-1) = '\0';
  tok.strval = yytext+1; 
  //TODO: pre-process string to unescape shit
  return STRING; 
}

                                                                                               
"("           { return LP; }
")"           { return RP; }
","           { return COMMA; }

[A-Za-z_][A-Za-z0-9_]* {	
  	tok.strval = strdup(yytext);
  	return IDENT;
}
 
[ \t\n]+ /* ignore whitespace */
%%


/**
 * yyerror() is invoked when the lexer or the parser encounter
 * an error. The error message is passed via *s
 *
 *
 */
void yyerror(char *s)
{
  printf("error: '%s' at line: %d\n",s,yylineno);
}

int yywrap(void)
{
  return 1;
}

