
%left AND.
%left OR.
%nonassoc EQ NE GT GE LT LE.
//%left PLUS MINUS.
//%right EXP NOT.

%token_type {Token}  
   

%syntax_error {  
  printf("Syntax error!\n");  
}   
   
%include {   
  #include <stdlib.h>
  #include <stdio.h>  
  #include <assert.h>
  #include "token.h"
  #include "parser.h"
  #include "../value.h"

  #define OP_EQ 1

  typedef struct {
    int propId;
    int op;
    SIValue val;
  } PredicateNode;

  

} // END %include  
     
query ::= pred(A).   { printf("Query: property %d == %s\n", A.propId, A.val.stringval.str); }

//pred_list(A) ::= pred(B) AND pred(C). { }
//%type pred_list {PredicateNode}
//pred_list(A) ::= pred(B). { A = B; }
//pred_list(A) ::= pred(B) OR pred(C). { }

%type pred {PredicateNode}

pred(A) ::= prop(B) EQ value(C). { 
  printf("Predicate!\n");
                        A.propId = B;
                        A.op = OP_EQ;
                        A.val = C;
                      }

%type value {SIValue}

value(A) ::= INTEGER(B). { printf("INTVAL\n"); A = SI_IntVal(B.intval); }
value(A) ::= STRING(B). {  printf("STRVAL\n");A = SI_StringValC(B.strval); }
value(A) ::= FLOAT(B). {  printf("FLOATVAL\n"); A = SI_FloatVal(B.dval); }


%type prop {int}
prop(A) ::= ENUMERATOR(B). { printf("ENUM %d!\n", B.intval); A = B.intval; }


%code {


extern FILE *yyin;
typedef struct yy_buffer_state *YY_BUFFER_STATE;

int             yylex( void );
YY_BUFFER_STATE yy_scan_string( const char * );
void            yy_delete_buffer( YY_BUFFER_STATE );
extern int yylineno;
extern char *yytext;
  



int main( int argc, char **argv )   {
  ++argv, --argc;  /* skip over program name */
  if ( argc > 0 )
          yyin = fopen( argv[0], "r" );
  else
          yyin = stdin;
        
  void* pParser = ParseAlloc (malloc);        
  int t = 0;



  //ParserFree(pParser);
  while (0 != (t = yylex())) {
      Parse(pParser, t, tok);                
      
      printf("%d) tok: %d (%s)\n", yylineno, t, yytext);
  }
  Parse (pParser, 0, tok);
}
     
}
