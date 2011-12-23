/*
lexer.c

Copyright © 2011 William Astle

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

/*
This handles the gritty details of parsing tokens
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#define __lexer_c_seen__
#include "lwbasic.h"

/*
A token idenfier is returned by lexer(). The actual string value
is found in state->lexer_lexer_token_string; if the token as an integer value,
it will be found in state->lexer_token_number in the appropriate "value"
slot.
*/

struct token_list
{
	char *string;
	int token;
};

/* keywords that appear as part of normal expressions */
static struct token_list lexer_global_tokens[] = 
{
	{ "function",		token_kw_function },
	{ "sub",			token_kw_sub },
	{ "public",			token_kw_public },
	{ "private",		token_kw_private },
	{ "as",				token_kw_as },
	{ "params",			token_kw_params },
	{ "returns",		token_kw_returns },
	{ "integer",		token_kw_integer },
	{ "endsub",			token_kw_endsub },
	{ "endfunction",	token_kw_endfunction },
	{ "dim",			token_kw_dim },
	{ NULL }
};

/* contains "built in" function names */
static struct token_list lexer_expr_tokens[] =
{
	{ "and",			token_op_and },
	{ "or",				token_op_or },
	{ "band",			token_op_band },
	{ "bor", 			token_op_bor },
	{ "bxor",			token_op_bxor },
	{ "xor",			token_op_xor },
	{ "not",			token_op_not },
	{ "bnot",			token_op_bnot },
	{ NULL }
};

static char *lexer_token_names[] =
{
	"SUB",
	"FUNCTION",
	"AS",
	"PUBLIC",
	"PRIVATE",
	"PARAMS",
	"RETURNS",
	"INTEGER",
	"ENDSUB",
	"ENDFUNCTION",
	"DIM",
	"<assignment>",
	"<equality>",
	"<greater>",
	"<less>",
	"<greaterequal>",
	"<lessequal>",
	"<notequal>",
	"<and>",
	"<or>",
	"<xor>",
	"<bitwiseand>",
	"<bitwiseor>",
	"<bitwisexor>",
	"<plus>",
	"<minus>",
	"<times>",
	"<divide>",
	"<modulus>",
	"<openparen>",
	"<closeparen>",
	"<not>",
	"<bitwisenot>",
	"<identifier>",
	"<char>",
	"<uint>",
	"<int>",
	"<eol>",
	"<eof>"
};

char *lexer_token_name(int token)
{
	if (token > token_eol)
		return "???";
	return lexer_token_names[token];
}

static int lexer_getchar(cstate *state)
{
	int c;
	c = input_getchar(state);
	if (c == -2)
	{
		lwb_error("Error reading input stream.");
	}
	return c;
}

static void lexer_nextchar(cstate *state)
{
	state -> lexer_curchar = lexer_getchar(state);
	if (state -> lexer_curchar == state -> lexer_ignorechar)
		state -> lexer_curchar = lexer_getchar(state);
	state -> lexer_ignorechar = 0;
}

static int lexer_curchar(cstate *state)
{
	if (state -> lexer_curchar == -1)
	{
		lexer_nextchar(state);
	}
	
	return state -> lexer_curchar;
}

static void lexer_skip_white(cstate *state)
{
	int c;
	
	for (;;)
	{
		c = lexer_curchar(state);
		if (!(c == 0 || c == ' ' || c == '\t'))
			return;
		lexer_nextchar(state);
	}
}

/* must not be called unless the word will be non-zero length */
static void lexer_word(cstate *state)
{
	int wordlen = 0;
	int wordpos = 0;
	char *word = NULL;
	int c;
	struct token_list *tok = NULL;
	
	for (;;) {
		c = lexer_curchar(state);
		if (c == '_' || (c >= '0' && c <= '9' ) || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c >= 0x80)
		{
			/* character is part of word */
			if (wordpos >= wordlen)
			{
				word = lw_realloc(word, wordlen + 32);
				wordlen += 32;
			}
			word[wordpos++] = c;
		}
		else
			break;
		
		lexer_nextchar(state);
	}
	
	word[wordpos] = 0;
	lw_free(state -> lexer_token_string);
	state -> lexer_token_string = lw_strdup(word);
	
	switch (state -> parser_state)
	{
	default:
		tok = lexer_global_tokens;
	}
	
	if (state -> expression)
	{
		tok = lexer_expr_tokens;
	}
	
	/* check for tokens if appropriate */
	/* force uppercase */
	if (tok)
	{
		for (c = 0; word[c]; c++)
			if (word[c] >= 'A' && word[c] <= 'Z')
				word[c] = word[c] + 0x20;

		while (tok -> string)
		{
			if (strcmp(tok -> string, word) == 0)
				break;
			tok++;
		}
	}
	
	lw_free(word);
	if (tok && tok -> string)
		state -> lexer_token = tok -> token;
	else
		state -> lexer_token = token_identifier;
}

static void lexer_parse_number(cstate *state, int neg)
{
	unsigned long tint = 0;
	int c;
	
	for (;;)
	{
		c = lexer_curchar(state);
		if (c >= '0' && c <= '9')
		{
			tint *= 10 + (c - '0');
		}
		else
		{
			/* end of the number here */
			if (neg)
			{
				if (tint > 0x80000000)
					lwb_error("Integer overflow\n");
				state -> lexer_token_number.integer = -tint;
				state -> lexer_token = token_int;
			}
			else
			{
				state -> lexer_token = token_uint;
				state -> lexer_token_number.uinteger = tint;
			}
			return;
		}
		lexer_nextchar(state);
	}
}

static void lexer_empty_token(cstate *state)
{
	lw_free(state -> lexer_token_string);
	state -> lexer_token_string = NULL;
}

void lexer(cstate *state)
{
	int c;

	lexer_skip_white(state);
	
	lexer_empty_token(state);
	
	c = lexer_curchar(state);
	if (c == -1)
	{
		state -> lexer_token = token_eof;
		return;
	}

	if (c == '\n')
	{
		/* LF */
		lexer_nextchar(state);
		state -> lexer_ignorechar = '\r';
		state -> lexer_token = token_eol;
		return;
	}
	
	if (c == '\r')
	{
		/* CR */
		lexer_nextchar(state);
		state -> lexer_ignorechar = '\n';
		state -> lexer_token = token_eol;
		return;
	}
	
	if (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c >= 0x80)
	{
		/* we have a word here; identifier, keyword, etc. */
		lexer_word(state);
		return;
	}

	if (state -> expression && c >= '0' && c <= '9')
	{
		/* we have a number */
		lexer_parse_number(state, 0);
		return;
	}
 
	lexer_nextchar(state);	
	if (state -> expression)
	{
		if (c == '-' && lexer_curchar(state) >= '0' && lexer_curchar(state) <= '9')
		{
			/* we have a negative number here */
			lexer_parse_number(state, 1);
			return;
		}
		if (c == '=')
		{
			state -> lexer_token = token_op_equality;
			return;
		}
		if (c == '<')
		{
			if (lexer_curchar(state) == '=')
			{
				lexer_nextchar(state);
				state -> lexer_token = token_op_lessequal;
				return;
			}
			if (lexer_curchar(state) == '>')
			{
				lexer_nextchar(state);
				state -> lexer_token = token_op_notequal;
				return;
			}
			state -> lexer_token = token_op_less;
			return;
		}
		if (c == '>')
		{
			if (lexer_curchar(state) == '>')
			{
				lexer_nextchar(state);
				state -> lexer_token = token_op_greaterequal;
				return;
			}
			if (lexer_curchar(state) == '<')
			{
				state -> lexer_token = token_op_notequal;
				lexer_nextchar(state);
				return;
			}
			state -> lexer_token = token_op_greater;
			return;
		}
		switch(c)
		{
		case '+':
			state -> lexer_token = token_op_plus;
			return;
		
		case '-':
			state -> lexer_token = token_op_minus;
			return;
		
		case '/':
			state -> lexer_token = token_op_divide;
			return;
		
		case '*':
			state -> lexer_token = token_op_times;
			return;
		
		case '%':
			state -> lexer_token = token_op_modulus;
			return;
		
		case '(':
			state -> lexer_token = token_op_oparen;
			return;
		
		case ')':
			state -> lexer_token = token_op_cparen;
			return;
		
		}
	}
	else
	{
		if (c == '=')
		{
			state -> lexer_token = token_op_assignment;
			return;
		}
	}
	
	/* return the character if all else fails */
	state -> lexer_token = token_char;
	state -> lexer_token_string = lw_realloc(state -> lexer_token_string, 2);
	state -> lexer_token_string[0] = c;
	state -> lexer_token_string[1] = 0;
	return;
}

char *lexer_return_token(cstate *state)
{
	static char *buffer = NULL;
	static int buflen = 0;
	int l;
	
	if (buflen == 0)
	{
		buffer = lw_alloc(128);
		buflen = 128;
	}

	l = snprintf(buffer, buflen, "%s (%s)", state -> lexer_token_string, lexer_token_name(state -> lexer_token));
	if (l >= buflen)
	{
		buffer = lw_realloc(buffer, l + 1);
		buflen = l + 1;
		snprintf(buffer, buflen, "%s (%s)", state -> lexer_token_string, lexer_token_name(state -> lexer_token));
	}
	return buffer;
}
