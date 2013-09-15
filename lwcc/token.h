/*
lwcc/token.h

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

#ifndef token_h_seen___
#define token_h_seen___

#include <stdio.h>

enum
{
	CPP_NOUNG = -3,
	CPP_EOL = -2,
	CPP_EOF = -1,
};

enum
{
	TOK_NONE = 0,
	TOK_EOF,
	TOK_EOL,
	TOK_WSPACE,
	TOK_IDENT,
	TOK_NUMBER,
	TOK_STRING,
	TOK_CHAR,
	TOK_DIV,
	TOK_ADD,
	TOK_SUB,
	TOK_OPAREN,
	TOK_CPAREN,
	TOK_NE,
	TOK_EQ,
	TOK_LE,
	TOK_LT,
	TOK_GE,
	TOK_GT,
	TOK_BAND,
	TOK_BOR,
	TOK_BNOT,
	TOK_MOD,
	TOK_COMMA,
	TOK_ELLIPSIS,
	TOK_QMARK,
	TOK_COLON,
	TOK_OBRACE,
	TOK_CBRACE,
	TOK_OSQUARE,
	TOK_CSQUARE,
	TOK_COM,
	TOK_EOS,
	TOK_HASH,
	TOK_DBLHASH,
	TOK_XOR,
	TOK_XORASS,
	TOK_STAR,
	TOK_MULASS,
	TOK_DIVASS,
	TOK_ASS,
	TOK_MODASS,
	TOK_SUBASS,
	TOK_DBLSUB,
	TOK_ADDASS,
	TOK_DBLADD,
	TOK_BWAND,
	TOK_BWANDASS,
	TOK_BWOR,
	TOK_BWORASS,
	TOK_LSH,
	TOK_LSHASS,
	TOK_RSH,
	TOK_RSHASS,
	TOK_DOT,
	TOK_CHR_LIT,
	TOK_STR_LIT,
	TOK_ARROW,
	TOK_ENDEXPAND,
	TOK_STARTEXPAND,
	TOK_MAX
};

struct token
{
	int ttype;				// token type
	char *strval;			// the token value if relevant
	struct token *prev;		// previous token in a list
	struct token *next;		// next token in a list
	int lineno;				// line number token came from
	int column;				// character column token came from
	const char *fn;			// file name token came from
};

extern void token_free(struct token *);
extern struct token *token_create(int, char *strval, int, int, const char *);
extern struct token *token_dup(struct token *);
/* add a token to the end of a list */
extern void token_append(struct token *, struct token *);
/* add a token to the start of a list */
extern void token_prepend(struct token *, struct token *);
/* remove individual token from whatever list it is on */
extern void token_remove(struct token *);
/* replace token with list of tokens specified */
extern void token_replace(struct token *, struct token *);
/* print a token out */
extern void token_print(struct token *, FILE *);

#endif // token_h_seen___
