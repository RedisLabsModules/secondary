
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

func(A) ::= ident(B) vallist(C). { 
    /* Terminal condition of a single predicate */
    A = NewFuncNode(B->ident.name, C);
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
    printf("VALLIST!\n");
    A = B;
}

vallist(A) ::= LP value(B) RP. {
    printf("Got single value!\n");
    A = NewVector(ParseNode *, 1);
    Vector_Push(A, B);
}

vallist(A) ::= LP ident(B) RP. {
    printf("Got single ident!\n");
    A = NewVector(ParseNode *, 1);
    Vector_Push(A, B);
}

multivals(A) ::= value(B) COMMA value(C). {
      A = NewVector(ParseNode *, 2);
      Vector_Push(A, B);
      Vector_Push(A, C);
}


multivals(A) ::= ident(B) COMMA ident(C). {
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

//value(A) ::= ident(B). { printf("IDENT NODE!\n"); A = B; } 
// property enumerator
ident(A) ::= ENUMERATOR(B). { A = NewIdentifierNode(NULL, B.intval);  }
ident(A) ::= IDENTT(B). { A = NewIdentifierNode(B.strval, 0);  }



%code {

  /* Definitions of flex stuff */
 // extern FILE *yyin;
  typedef struct yy_buffer_state *YY_BUFFER_STATE;
  int             yylex( void );
  YY_BUFFER_STATE yy_scan_string( const char * );
  YY_BUFFER_STATE yy_scan_bytes( const char *, size_t );
  void            yy_delete_buffer( YY_BUFFER_STATE );
  
  


int main(int argc, char **argv) {

    const char *c = argc > 1 ? argv[1] :  "avg(1)";
    char *e = NULL;
    char **err = &e;

    //printf("Parsing query %s\n", c);
    yy_scan_bytes(c, strlen(c));
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

        ParseNode_print(ctx.root, 0);
    }
            

    ParseFree(pParser, free);
    if (err) {
        *err = ctx.errorMsg;
    }
    printf("Err: %s\n", *err);
  }
   


}
