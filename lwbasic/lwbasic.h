/*
lwbasic.h

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
definitions used throughout lwbasic
*/

#ifndef __lwbasic_h_seen__
#define __lwbasic_h_seen__

#include <stdint.h>

#include "symtab.h"

/* note: integer and uinteger will be the same for positive values from 0
through 0x7FFFFFFF; the unsigned type should be used for doing ascii
conversions and then if a negative value was discovered, it should be
negated IFF it is in range. */

union lexer_numbers
{
	uint32_t uinteger;
	int32_t integer;
};

typedef struct
{
	char *output_file;
	char *input_file;
	
	int debug_level;

	char *lexer_token_string;
	union lexer_numbers lexer_token_number;
	int lexer_token;
	int lexer_curchar;
	int lexer_ignorechar;
	int expression;	
	int parser_state;
	
	void *input_state;
	
	char *currentsub;
	symtab_t *global_syms;
	symtab_t *local_syms;
	int returntype;
	int framesize;
} cstate;

/* parser states */
enum
{
	parser_state_global = 0,			/* only global decls allowed */
	parser_state_error
};

/* token types */
enum
{
	token_kw_sub,				/* SUB keyword */
	token_kw_function,			/* FUNCTION keyword */
	token_kw_as,				/* AS keyword */
	token_kw_public,			/* PUBLIC keyword */
	token_kw_private,			/* PRIVATE keyword */
	token_kw_params,			/* PARAMS keyword */
	token_kw_returns,			/* RETURNS keyword */
	token_kw_integer,			/* INTEGER keyword */
	token_kw_endsub,			/* ENDSUB keyword */
	token_kw_endfunction,		/* ENDFUNCTION keyword */
	token_kw_dim,				/* DIM keyword */
	token_op_assignment,		/* assignment operator */
	token_op_equality,			/* equality test */
	token_op_greater,			/* greater than */
	token_op_less,				/* less than */
	token_op_greaterequal,		/* greater or equal */
	token_op_lessequal,			/* less or equal */
	token_op_notequal,			/* not equal */
	token_op_and,				/* boolean and */
	token_op_or,				/* boolean or */
	token_op_xor,				/* boolean exlusive or */
	token_op_band,				/* bitwise and */
	token_op_bor,				/* bitwise or */
	token_op_bxor,				/* bitwise xor */
	token_op_plus,				/* plus */
	token_op_minus,				/* minus */
	token_op_times,				/* times */
	token_op_divide,			/* divide */
	token_op_modulus,			/* modulus */
	token_identifier,			/* an identifier (variable, function, etc. */
	token_char,					/* single character; fallback */
	token_uint,					/* unsigned integer up to 32 bits */
	token_int,					/* signed integer up to 32 bits */
	token_eol,					/* end of line */
	token_eof					/* end of file */
};

/* symbol types */
enum
{
	symtype_sub,				/* "sub" (void function) */
	symtype_func,				/* function (nonvoid) */
	symtype_param,				/* function parameter */
	symtype_var					/* variable */
};

#ifndef __input_c_seen__
extern int input_getchar(cstate *state);
#endif

#ifndef __main_c_seen__
extern void lwb_error(const char *fmt, ...);
#endif

#ifndef __lexer_c_seen__
extern void lexer(cstate *state);
extern char *lexer_return_token(cstate *state);
extern char *lexer_token_name(int token);
#endif

#ifndef __emit_c_seen__
extern void emit_prolog(cstate *state, int vis);
extern void emit_epilog(cstate *state);
#endif


#endif /* __lwbasic_h_seen__ */
