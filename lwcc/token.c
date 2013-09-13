/*
lwcc/token.c

Copyright © 2013 William Astle

This file is part of LWTOOLS.

LWTOOLS is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "token.h"

struct token *token_create(int ttype, char *strval, int row, int col, const char *fn)
{
	struct token *t;
	
	t = lw_alloc(sizeof(struct token));
	t -> ttype = ttype;
	if (strval)
		t -> strval = lw_strdup(strval);
	else
		strval = NULL;
	t -> lineno = row;
	t -> column = col;
	t -> fn = fn;
	t -> next = NULL;
	t -> prev = NULL;
	return t;
}

void token_free(struct token *t)
{
	lw_free(t -> strval);
	lw_free(t);
}

struct token *token_dup(struct token *t)
{
	struct token *t2;
	
	t2 = lw_alloc(sizeof(struct token));
	(*t2) = (*t);
	t2 -> next = NULL;
	t2 -> prev = NULL;
	if (t -> strval)
		t2 -> strval = lw_strdup(t -> strval);
	return t2;
}

static struct { int ttype; char *tstr; } tok_strs[] =
{
	{ TOK_WSPACE, " " },
	{ TOK_EOL, "\n" },
	{ TOK_DIV, "/" },
	{ TOK_ADD, "+" },
	{ TOK_SUB, "-" },
	{ TOK_OPAREN, "(" },
	{ TOK_CPAREN, ")" },
	{ TOK_NE, "!=" },
	{ TOK_EQ, "==" },
	{ TOK_LE, "<=" },
	{ TOK_LT, "<" },
	{ TOK_GE, ">=" },
	{ TOK_GT, ">" },
	{ TOK_BAND, "&&" },
	{ TOK_BOR, "||" },
	{ TOK_BNOT, "!" },
	{ TOK_MOD, "%"},
	{ TOK_COMMA, "," },
	{ TOK_ELLIPSIS, "..." },
	{ TOK_QMARK, "?" },
	{ TOK_COLON, ":" },
	{ TOK_OBRACE, "{" },
	{ TOK_CBRACE, "}" },
	{ TOK_OSQUARE, "[" },
	{ TOK_CSQUARE, "]" },
	{ TOK_COM, "~" },
	{ TOK_EOS, ";" },
	{ TOK_HASH, "#" },
	{ TOK_DBLHASH, "##" },
	{ TOK_XOR, "^" },
	{ TOK_XORASS, "^=" },
	{ TOK_STAR, "*" },
	{ TOK_MULASS, "*=" },
	{ TOK_DIVASS, "/=" },
	{ TOK_ASS, "=" },
	{ TOK_MODASS, "%=" },
	{ TOK_SUBASS, "-=" },
	{ TOK_DBLSUB, "--" },
	{ TOK_ADDASS, "+=" },
	{ TOK_DBLADD, "++" },
	{ TOK_BWAND, "&" },
	{ TOK_BWANDASS, "&=" },
	{ TOK_BWOR, "|" },
	{ TOK_BWORASS, "|=" },
	{ TOK_LSH, "<<" },
	{ TOK_LSHASS, "<<=" },
	{ TOK_RSH, ">>" },
	{ TOK_RSHASS, ">>=" },
	{ TOK_DOT, "." },
	{ TOK_ARROW, "->" },
	{ TOK_NONE, "" }
};

void token_print(struct token *t, FILE *f)
{
	int i;
	for (i = 0; tok_strs[i].ttype != TOK_NONE; i++)
	{
		if (tok_strs[i].ttype == t -> ttype)
		{
			fprintf(f, "%s", tok_strs[i].tstr);
			break;
		}
	}
	if (t -> strval)
		fprintf(f, "%s", t -> strval);
}
