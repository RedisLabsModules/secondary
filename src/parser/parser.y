
%left AND.
%left OR.
%nonassoc EQ NE GT GE LT LE.
//%left PLUS MINUS.
//%right EXP NOT.

%token_type {Token}  
   

%syntax_error {  
yyerror("WAT?");
}   
   
%include {   

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "token.h"
#include "parser.h"
#include "ast.h"

void yyerror(char *s);

} // END %include  

%extra_argument { ParseNode **root }


query ::= cond(A). { *root = A; }

%type op {int}
op(A) ::= EQ. { A = EQ; }
op(A) ::= GT. { A = GT; }
op(A) ::= LT. { A = LT; }
op(A) ::= LE. { A = LE; }
op(A) ::= GE. { A = GE; }
op(A) ::= NE. { A = NE; } 

%type cond {ParseNode*}
%destructor cond { ParseNode_Free($$); }

cond(A) ::= prop(B) op(C) value(D). { 
    /* Terminal condition of a single predicate */
    A = NewPredicateNode(B, C, D);
}

cond(A) ::= LP cond(B) RP. { 
  A = B;
}
                      
cond(A) ::= cond(B) AND cond(C). {
  A = NewConditionNode(B, AND, C);
}

cond(A) ::= cond(B) OR cond(C). {
  A = NewConditionNode(B, OR, C);
}

%type value {SIValue}

// raw value tokens - int / string / float
value(A) ::= INTEGER(B). {  A = SI_IntVal(B.intval); }
value(A) ::= STRING(B). {  A = SI_StringValC(B.strval); }
value(A) ::= FLOAT(B). {  A = SI_FloatVal(B.dval); }


%type prop {int}
// property enumerator
prop(A) ::= ENUMERATOR(B). { A = B.intval; }


%code {

  /* Definitions of flex stuff */
 // extern FILE *yyin;
  typedef struct yy_buffer_state *YY_BUFFER_STATE;
  int             yylex( void );
  YY_BUFFER_STATE yy_scan_string( const char * );
  YY_BUFFER_STATE yy_scan_bytes( const char *, size_t );
  void            yy_delete_buffer( YY_BUFFER_STATE );
  extern int yylineno;
  extern char *yytext;
  

ParseNode *ParseQuery(const char *c, size_t len)  {

    //printf("Parsing query %s\n", c);
    yy_scan_bytes(c, len);
    void* pParser = ParseAlloc (malloc);        
    int t = 0;

    ParseNode *ret = NULL;
    //ParserFree(pParser);
    while (0 != (t = yylex())) {
      printf("Token %d\n", t);
        Parse(pParser, t, tok, &ret);                
    }
    Parse (pParser, 0, tok, &ret);
    ParseFree(pParser, free);

    return ret;
  }
   


  // int main( int argc, char **argv )   {
    
  //   yy_scan_string("$1 = \"foo bar\" AND $2 = 1337");
  //   void* pParser = ParseAlloc (malloc);        
  //   int t = 0;

  //   //ParserFree(pParser);
  //   while (0 != (t = yylex())) {
  //       Parse(pParser, t, tok);                
  //   }
  //   Parse (pParser, 0, tok);
  //   ParseFree(pParser, free);
  // }
      
}
