
%left AND.
%left OR.
%nonassoc EQ NE GT GE LT LE IN IS NOW.
//%left PLUS MINUS.
//%right EXP NOT.

%token_type {Token}  
   

%syntax_error {  

    //yyerror(yytext);

    int len =strlen(yytext)+100; 
    char msg[len];

    snprintf(msg, len, "Syntax error in WHERE line %d near '%s'", yylineno, yytext);

    ctx->ok = 0;
    ctx->errorMsg = strdup(msg);
}   
   
%include {   

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include "token.h"
#include "parser.h"
#include "ast.h"
#include "../rmutil/alloc.h"

extern int yylineno;
extern char *yytext;

typedef struct {
    ParseNode *root;
    int ok;
    char *errorMsg;
}parseCtx;


void yyerror(char *s);
    
} // END %include  

%extra_argument { parseCtx *ctx }


query ::= cond(A). { ctx->root = A; }

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

/* special case to make sure LIKE does not occur with non-strings */
cond(A) ::= prop(B) IS TK_NULL. { 
    A = NewPredicateNode(B, IS, SI_NullVal());
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
value(A) ::= INTEGER(B). {  A = SI_LongVal(B.intval); }
value(A) ::= STRING(B). {  A = SI_StringValC(strdup(B.strval)); }
value(A) ::= FLOAT(B). {  A = SI_DoubleVal(B.dval); }
value(A) ::= TRUE. { A = SI_BoolVal(1); }
value(A) ::= FALSE. { A = SI_BoolVal(0); }
value(A) ::= timestamp(B). { A = SI_TimeVal(B); }

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


%type prop {property}
%destructor prop {
     
    if ($$.name != NULL) { 
        free($$.name); 
        $$.name = NULL;
    } 
}
// property enumerator
prop(A) ::= ENUMERATOR(B). { A.id = B.intval; A.name = NULL;  }
prop(A) ::= IDENT(B). { A.name = B.strval; A.id = 0;  }

%type timestamp {time_t}

timestamp(A) ::= NOW. {
    A = time(NULL);
}

timestamp(A) ::= TODAY. {
    time_t t = time(NULL);
    A = t - t % 86400;
}

timestamp(A) ::= TIME LP INTEGER(B) RP. {
    A = (time_t)B.intval;
}

timestamp(A) ::= UNIXTIME LP INTEGER(B) RP. {
    A = (time_t)B.intval;
}

timestamp(A) ::= TIME_ADD LP timestamp(B) COMMA duration(C) RP. {
    A = B + C;
}

timestamp(A) ::= TIME_SUB LP timestamp(B) COMMA duration(C) RP. {
    A = B - C;
}


%type duration {int}
duration(A) ::= DAYS LP INTEGER(B) RP. {
    A = B.intval * 86400;
}
duration(A) ::= HOURS LP INTEGER(B) RP. {
    A = B.intval * 3600;
}
duration(A) ::= MINUTES LP INTEGER(B) RP. {
    A = B.intval * 60;
}
duration(A) ::= SECONDS LP INTEGER(B) RP. {
    A = B.intval;
}    

%code {

  /* Definitions of flex stuff */
 // extern FILE *yyin;
  typedef struct yy_buffer_state *YY_BUFFER_STATE;
  int             yylex( void );
  YY_BUFFER_STATE yy_scan_string( const char * );
  YY_BUFFER_STATE yy_scan_bytes( const char *, size_t );
  void            yy_delete_buffer( YY_BUFFER_STATE );
  
  


ParseNode *ParseQuery(const char *c, size_t len, char **err)  {

    //printf("Parsing query %s\n", c);
    yy_scan_bytes(c, len);
    void* pParser = ParseAlloc (malloc);        
    int t = 0;

    parseCtx ctx = {.root = NULL, .ok = 1, .errorMsg = NULL };
    //ParseNode *ret = NULL;
    //ParserFree(pParser);
    while (ctx.ok && 0 != (t = yylex())) {
        Parse(pParser, t, tok, &ctx);                
    }
    if (ctx.ok) {
        Parse (pParser, 0, tok, &ctx);
    }
    ParseFree(pParser, free);
    if (err) {
        *err = ctx.errorMsg;
    }
    return ctx.root;
  }
   


}
