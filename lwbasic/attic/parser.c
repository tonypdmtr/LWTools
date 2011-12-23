/*
compiler.c

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
This is the actual compiler bit; it drives the parser and code generation
*/

#include <stdio.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwbasic.h"
#include "symtab.h"

static void expect(cstate *state, int tt)
{
	if (state -> lexer_token != tt)
		lwb_error("Expecting %s, got %s\n", lexer_token_name(tt), lexer_return_token(state));
	lexer(state);
}


/* size of a type */
static int sizeof_type(int type)
{
	/* everything is an "int" right now; 2 bytes */
	return 2;
}

/* parse a type; the next token will be acquired as a result */
/* the token advancement is to provide consistency */
static int parse_type(cstate *state)
{
	int pt = -1;
	
	switch (state -> lexer_token)
	{
	case token_kw_integer:
		pt = 1;
		break;

	default:
		lwb_error("Invalid type specification");
	}
	lexer(state);
	/* look for "unsigned" modifier for integer types */
	return pt;
}

static void parse_expr(cstate *state, int prec);
static void parse_term(cstate *state);
static int parse_expression(cstate *state)
{
	state -> expression = 1;
	
	parse_expr(state, 0);
	
	state -> expression = 0;
	return 1;
}

static void parse_decls(cstate *state)
{
	/* declarations */
	/* the first thing that doesn't look like a declaration is assumed */
	/* to be a statement and will trigger a bailout */
	int vt;
	char *vn;
	symtab_entry_t *se;
	
	for (;;)
	{
		switch (state -> lexer_token)
		{
		/* DIM keyword */
		case token_kw_dim:
			lexer(state);
			if (state -> lexer_token != token_identifier)
			{
				lwb_error("Expecting identifier, got %s\n", lexer_return_token(state));
			}
			vn = lw_strdup(state -> lexer_token_string);
			lexer(state);
			if (state -> lexer_token != token_kw_as)
			{
				lwb_error("Expecting AS, got %s\n", lexer_return_token(state));
			}
			lexer(state);
			vt = parse_type(state);
			
			se = symtab_find(state -> local_syms, vn);
			if (se)
			{
				lwb_error("Multiply defined local variable %s", vn);
			}
			state -> framesize += sizeof_type(vt);
			symtab_register(state -> local_syms, vn, -(state -> framesize), symtype_var, NULL);
			
			lw_free(vn);
			break;
		
		/* blank lines allowed */
		case token_eol:
			break;
			
		default:
			return;
		}
		if (state -> lexer_token != token_eol)
			lwb_error("Expecting end of line; got %s\n", lexer_return_token(state));
		lexer(state);
	}
}

static void parse_statements(cstate *state)
{
	symtab_entry_t *se;
	int et;
	
	for (;;)
	{
		switch (state -> lexer_token)
		{
		/* blank lines allowed */
		case token_eol:
			break;
		
		/* variable assignment */
		case token_identifier:
			se = symtab_find(state -> local_syms, state -> lexer_token_string);
			if (!se)
			{
				se = symtab_find(state -> global_syms, state -> lexer_token_string);
			}
			if (!se)
				lwb_error("Unknown variable %s\n", state -> lexer_token_string);
			lexer(state);
			/* ensure the first token of the expression will be parsed correctly */
			state -> expression = 1;
			expect(state, token_op_assignment);

			/* parse the expression */
			et = parse_expression(state);
			
			/* check type compatibility */
			
			/* actually do the assignment */
			
			break;
		
		/* anything we don't recognize as a statement token breaks out */
		default:
			return;
		}
		if (state -> lexer_token != token_eol)
			lwb_error("Expecting end of line; got %s\n", lexer_return_token(state));
		lexer(state);
	}
}


/* issub means RETURNS is not allowed; !issub means RETURNS is required */

static void parse_subfunc(cstate *state, int issub)
{
	int pt, rt;
	char *subname, *pn;
	int vis = 0;
	symtab_entry_t *se;
	int paramsize = 0;
	
	state -> local_syms = symtab_init();
	state -> framesize = 0;
	
	lexer(state);
	if (state -> lexer_token != token_identifier)
	{
		lwb_error("Invalid sub name '%s'", state -> lexer_token_string);
	}
	
	subname = lw_strdup(state -> lexer_token_string);
	
	lexer(state);
	if (state -> lexer_token == token_kw_public || state -> lexer_token == token_kw_private)
	{
		if (state -> lexer_token == token_kw_public)
			vis = 1;
		lexer(state);
	}

	/* ignore the "PARAMS" keyword if present */
	if (state -> lexer_token == token_kw_params)
		lexer(state);
	
	if (state -> lexer_token == token_eol || state -> lexer_token == token_kw_returns)
		goto noparms;

paramagain:
	if (state -> lexer_token != token_identifier)
	{
		lwb_error("Parameter name expected, got %s\n", lexer_return_token(state));
	}
	pn = lw_strdup(state -> lexer_token_string);
	lexer(state);
	
	if (state -> lexer_token != token_kw_as)
		lwb_error("Expecting AS\n");
	lexer(state);
	
	pt = parse_type(state);

	se = symtab_find(state -> local_syms, pn);
	if (se)
	{
		lwb_error("Duplicate parameter name %s\n", pn);
	}
	symtab_register(state -> local_syms, pn, paramsize, symtype_param, NULL);
	paramsize += sizeof_type(pt);
	lw_free(pn);
	
	if (state -> lexer_token == token_char && state -> lexer_token_string[0] == ',')
	{
		lexer(state);
		goto paramagain;
	}

noparms:
	rt = -1;
	if (!issub)
	{
		if (state -> lexer_token != token_kw_returns)
		{
			lwb_error("FUNCTION must have RETURNS\n");
		}
		lexer(state);
/*		if (state -> lexer_token == token_identifier)
		{
			printf("Return value named: %s\n", state -> lexer_token_string);
			
			lexer(state);
			if (state -> lexer_token != token_kw_as)
				lwb_error("Execting AS after RETURNS");
			lexer(state);
		}
*/
		rt = parse_type(state);
	}
	else
	{
		if (state -> lexer_token == token_kw_returns)
		{
			lwb_error("SUB cannot specify RETURNS\n");
		}
	}

	
	if (state -> lexer_token != token_eol)
	{
		lwb_error("EOL expected; found %s\n", lexer_return_token(state));
	}

	
	se = symtab_find(state -> global_syms, subname);
	if (se)
	{
		lwb_error("Multiply defined symbol %s\n", subname);
	}

	symtab_register(state -> global_syms, subname, -1, issub ? symtype_sub : symtype_func, NULL);

	state -> currentsub = subname;
	state -> returntype = rt;
	/* consume EOL */
	lexer(state);
	
	/* variable declarations */
	parse_decls(state);
	
	/* output function/sub prolog */
	emit_prolog(state, vis);
	
	/* parse statement block  */
	parse_statements(state);
	
	if (issub)
	{
		if (state -> lexer_token != token_kw_endsub)
		{
			lwb_error("Expecting ENDSUB, got %s\n", lexer_return_token(state));
		}
	}
	else
	{
		if (state -> lexer_token != token_kw_endfunction)
		{
			lwb_error("Expecting ENDFUNCTION, got %s\n", lexer_return_token(state));
		}
	}
	/* output function/sub epilog */
	emit_epilog(state);
	
	lw_free(state -> currentsub);
	state -> currentsub = NULL;
	symtab_destroy(state -> local_syms);
	state -> local_syms = NULL;
}

void parser(cstate *state)
{
	state -> lexer_curchar = -1;
	state -> global_syms = symtab_init();
		
	/* now look for a global declaration */
	for (;;)
	{
		state -> parser_state = parser_state_global;
		lexer(state);
		switch (state -> lexer_token)
		{
		case token_kw_function:
			printf("Function\n");
			parse_subfunc(state, 0);
			break;
			
		case token_kw_sub:
			printf("Sub\n");
			parse_subfunc(state, 1);
			break;

		/* blank lines are allowed */
		case token_eol:
			continue;
		
		/* EOF is allowed - end of parsing */
		case token_eof:
			return;

		default:
			lwb_error("Invalid token '%s' in global state\n", lexer_return_token(state));
		}
	}	
}

static void parse_expr(cstate *state, int prec)
{
	static const struct operinfo {
		int opernum;
		int operprec;
	} operators[] =
	{
		{ token_op_plus, 100 },
		{ token_op_minus, 100 },
		{ token_op_times, 150 },
		{ token_op_divide, 150 },
		{ token_op_modulus, 150 },
		{ token_op_and,	25 },
		{ token_op_or, 20 },
		{ token_op_xor, 20 },
		{ token_op_band, 50 },
		{ token_op_bor, 45 },
		{ token_op_bxor, 45 },
		{ -1, -1 }
	};
	int opern;
	
	parse_term(state);
	
eval_next:
	for (opern = 0; operators[opern].opernum != -1; opern++)
	{
		if (operators[opern].opernum == state -> lexer_token)
			break;
	}
	if (operators[opern].opernum == -1)
		return;
	
	if (operators[opern].operprec <= prec)
		return;
	
	lexer(state);
	
	parse_expr(state, operators[opern].operprec);
	
	/* push operator */
	
	goto eval_next;
}

static void parse_term(cstate *state)
{
eval_next:
	/* parens */
	if (state -> lexer_token == token_op_oparen)
	{
		lexer(state);
		parse_expr(state, 0);
		expect(state, token_op_cparen);
		return;
	}
	
	/* unary plus; ignore it */
	if (state -> lexer_token == token_op_plus)
	{
		lexer(state);
		goto eval_next;
	}
	
	/* unary minus, precision 200 */
	if (state -> lexer_token == token_op_minus)
	{
		lexer(state);
		parse_expr(state, 200);
		
		/* push unary negation */
	}
	
	/* BNOT, NOT */
	if (state -> lexer_token == token_op_not || state -> lexer_token == token_op_bnot)
	{
		lexer(state);
		parse_expr(state, 200);
		
		/* push unary operator */
	}
	
	/* integer */
	if (state -> lexer_token == token_int)
	{
	}
	
	/* unsigned integer */
	if (state -> lexer_token == token_uint)
	{
	}
	
	/* variable or function call */
	if (state -> lexer_token == token_identifier)
	{
		lexer(state);
		if (state -> lexer_token == token_op_oparen)
		{
			/* function call */
			return;
		}
		/* variable */
		return;
	}
	
	lwb_error("Invalid input in expression; got %s\n", lexer_return_token(state));
}
