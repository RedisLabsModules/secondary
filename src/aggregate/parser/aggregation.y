
%left LP RP.
%nonassoc COMMA.
//%left PLUS MINUS.
//%right EXP NOT.

%token_type {Token}  
   

%syntax_error {  

    //yyerror(yytext);

    int len =strlen(yytext)+100; 
    char msg[len];

    snprintf(msg, len, "Syntax error in AGGREGATE line %d near '%s'", yylineno, yytext);

    ctx->ok = 0;
    ctx->errorMsg = strdup(msg);
}   
   
%include {   

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "token.h"
#include "aggregation.h"
#include "ast.h"
#include "../../rmutil/alloc.h"

extern int yylineno;
extern char *yytext;

typedef struct {
    ParseNode *root;
    int ok;
    char *errorMsg;
} parseCtx;


void yyerror(char *s);
    
} // END %include  

%extra_argument { parseCtx *ctx }


query ::= func(A). { ctx->root = A; }

%type func {ParseNode*}
%destructor func { ParseNode_Free($$); }

func(A) ::= ident(B) LP vallist(C) RP. { 
    /* Terminal condition of a single predicate */
    A = NewFuncNode(B, C);
}

%type value {ParseNode *}

// raw value tokens - int / string / float
value(A) ::= INTEGER(B). {  A = NewLiteralNode(SI_LongVal(B.intval)); }
value(A) ::= STRING(B). {  A = NewLiteralNode(SI_StringValC(strdup(B.strval))); }
value(A) ::= FLOAT(B). {  A = NewLiteralNode(SI_DoubleVal(B.dval)); }
value(A) ::= TRUE. { A = NewLiteralNode(SI_BoolVal(1)); }
value(A) ::= FALSE. { A = NewLiteralNode(SI_BoolVal(0)); }

%type vallist {Vector *}
%type multivals {Vector *}
%destructor vallist {Vector_Free($$);}
%destructor multivals {Vector_Free($$);}

vallist(A) ::= LP multivals(B) RP. {
    A = B;
}

vallist(A) ::= LP value(B) RP. {
    A = NewVector(ParseNode *, 1);
    Vector_Push(A, B);
}

multivals(A) ::= value(B) COMMA value(C). {
      A = NewVector(ParseNode *, 2);
      Vector_Push(A, B);
      Vector_Push(A, C);
}

multivals(A) ::= multivals(B) COMMA value(C). {
    Vector_Push(B, C);
    A = B;
}


%type ident {ParseNode *}
%destructor ident {
   ParseNode_Free($$);
}
// property enumerator
ident(A) ::= ENUMERATOR(B). { A = NewIdentifierNode(NULL, B.intval);  }
ident(A) ::= IDENT(B). { A = NewIdentifierNode(B.strval, 0);  }
value(A) ::= ident(B). { A = B; } 


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
