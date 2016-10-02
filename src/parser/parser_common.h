#ifndef __PARSER_COMMON_H__
#define __PARSER_COMMON_H__
#include <stdlib.h>
#include "ast.h"

ParseNode *ParseQuery(const char *c, size_t len, char **msg);
#endif