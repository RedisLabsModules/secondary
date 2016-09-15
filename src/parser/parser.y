
%left AND.
%left OR.
%nonassoc EQ NE GT GE LT LE IN.
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

/* special case to make sure LIKE does not occur with non-strings */
cond(A) ::= prop(B) LIKE STRING(C). { 
    A = NewPredicateNode(B, LIKE, SI_StringValC(C.strval));
}

cond(A) ::= prop(B) IN vallist(D). { 
    /* Terminal condition of a single IN predicate */
    A = NewInPredicateNode(B, IN, D);
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
value(A) ::= TRUE. { A = SI_BoolVal(1); }
value(A) ::= FALSE. { A = SI_BoolVal(0); }

%type vallist {SIValueVector}
%type multivals {SIValueVector}
%destructor vallist {SIValueVector_Free(&$$);}
%destructor multivals {SIValueVector_Free(&$$);}

vallist(A) ::= LP multivals(B) RP. {
    A = B;
    
}
multivals(A) ::= value(B) COMMA value(C). {
      A = SI_NewValueVector(2);
      SIValueVector_Append(&A, B);
      SIValueVector_Append(&A, C);
}

multivals(A) ::= multivals(B) COMMA value(C). {
    SIValueVector_Append(&B, C);
    A = B;
}


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

  // IDX.HGETALL FROM <index_name> WHERE ....
  // IDX.HMSET index_nmae key elem vale elem value

  // RQL.CREATE_TABLE ... 

  //   //ParserFree(pParser);
  //   while (0 != (t = yylex())) {
  //       Parse(pParser, t, tok);                
  //   }
  //   Parse (pParser, 0, tok);
  //   ParseFree(pParser, free);
  // }
      
}
