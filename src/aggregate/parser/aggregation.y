%nonassoc LP RP.
%nonassoc COMMA.
%left IDENT.

//%left PLUS MINUS.
//%right EXP NOT.

%token_type {Token}  
   

%syntax_error {  

    //yyerror(yytext);

    int len =strlen(yyaggtext)+100; 
    char msg[len];

    snprintf(msg, len, "Syntax error in AGGREGATE line %d near '%s'", yyagglineno, yyaggtext);

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

extern int yyagglineno;
extern char *yyaggtext;

typedef struct {
    AggParseNode *root;
    int ok;
    char *errorMsg;
} parseCtx;


void yyerror(char *s);
    

} // END %include  

%extra_argument { parseCtx *ctx }

%default_type {AggParseNode*}
%default_destructor { AggParseNode_Free($$); }

query ::= func(A). { ctx->root = A; }

func(A) ::= ident(B) arglist(C). { 
    /* Terminal condition of a single predicate */
    A = NewAggFuncNode(B->ident.name, C);
}

// property enumerator
// TODO: separate ident and enum
ident(A) ::= ENUMERATOR(B). { A = NewAggIdentifierNode(NULL, B.intval);  }
// string identifier
ident(A) ::= IDENTT(B). { A = NewAggIdentifierNode(B.strval, 0);  }


// function argument can be anything
arg(A) ::= value(B). { A = B; }
arg(A) ::= func(B). { A = B; }
arg(A) ::= ident(B). { A = B; }


// raw value tokens - int / string / float
value(A) ::= INTEGER(B). {  A = NewAggLiteralNode(SI_LongVal(B.intval)); }
value(A) ::= STRING(B). {  A = NewAggLiteralNode(SI_StringValC(strdup(B.strval))); }
value(A) ::= FLOAT(B). {  A = NewAggLiteralNode(SI_DoubleVal(B.dval)); }
value(A) ::= TRUE. { A = NewAggLiteralNode(SI_BoolVal(1)); }
value(A) ::= FALSE. { A = NewAggLiteralNode(SI_BoolVal(0)); }

%type arglist {Vector *}
%type multivals {Vector *}
%destructor arglist {Vector_Free($$);}
%destructor multivals {Vector_Free($$);}

arglist(A) ::= LP multivals(B) RP. {
    A = B;
}

arglist(A) ::= LP arg(B) RP. {
    A = NewVector(AggParseNode *, 1);
    Vector_Push(A, B);
}

multivals(A) ::= arg(B) COMMA arg(C). {
      A = NewVector(AggParseNode *, 2);
      Vector_Push(A, B);
      Vector_Push(A, C);
}

multivals(A) ::= multivals(B) COMMA arg(C). {
      Vector_Push(B, C);
      A = B;
}


%code {

  /* Definitions of flex stuff */
 // extern FILE *yyin;
  typedef struct yyagg_buffer_state *YY_BUFFER_STATE;
  int             yyagglex( void );
  YY_BUFFER_STATE yyagg_scan_string( const char * );
  YY_BUFFER_STATE yyagg_scan_bytes( const char *, size_t );
  void            yyagg_delete_buffer( YY_BUFFER_STATE );
  
  

AggParseNode *Agg_ParseQuery(const char *c, size_t len, char **err)  {

    //printf("Parsing query %s\n", c);
    yyagg_scan_bytes(c, strlen(c));
    void* pParser = agg_ParseAlloc (malloc);        
    int t = 0;

    parseCtx ctx = {.root = NULL, .ok = 1, .errorMsg = NULL };
    //AggParseNode *ret = NULL;
    //ParserFree(pParser);
    while (ctx.ok && 0 != (t = yyagglex())) {
        agg_Parse(pParser, t, tok, &ctx);                
    }
    if (ctx.ok) {
        agg_Parse (pParser, 0, tok, &ctx);
        if (ctx.root)
            AggParseNode_print(ctx.root, 0);
    }
            

    agg_ParseFree(pParser, free);
    if (err) {
        *err = ctx.errorMsg;
    }
    printf("Err: %s\n", *err);
    return ctx.root;
  }
   


}
