/*
lwcc/preproc.c

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

#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "cpp.h"
#include "strbuf.h"
#include "symbol.h"
#include "token.h"

static int expand_macro(struct preproc_info *, char *);
static void process_directive(struct preproc_info *);
static long eval_expr(struct preproc_info *);

struct token *preproc_next_processed_token(struct preproc_info *pp)
{
	struct token *ct;
	
again:
	ct = preproc_next_token(pp);
	if (ct -> ttype == TOK_EOF)
		return ct;
	if (ct -> ttype == TOK_EOL)
		pp -> ppeolseen = 1;
		
	if (ct -> ttype == TOK_HASH && pp -> eolseen == 1)
	{
		// preprocessor directive 
		process_directive(pp);
	}
	// if we're in a false section, don't return the token; keep scanning
	if (pp -> skip_level)
		goto again;

	if (ct -> ttype != TOK_WSPACE)
		pp -> ppeolseen = 0;
	
	if (ct -> ttype == TOK_IDENT)
	{
		// possible macro expansion
		if (expand_macro(pp, ct -> strval))
	 		goto again;
	}
	
	return ct;
}

static struct token *preproc_next_processed_token_nws(struct preproc_info *pp)
{
	struct token *t;
	
	do
	{
		t = preproc_next_processed_token(pp);
	} while (t -> ttype == TOK_WSPACE);
	return t;
}

static struct token *preproc_next_token_nws(struct preproc_info *pp)
{
	struct token *t;
	
	do
	{
		t = preproc_next_token(pp);
	} while (t -> ttype == TOK_WSPACE);
	return t;
}

static void skip_eol(struct preproc_info *pp)
{
	struct token *t;
	
	if (pp -> curtok && pp -> curtok -> ttype == TOK_EOL)
		return;
	do
	{
		t = preproc_next_token(pp);
	} while (t -> ttype != TOK_EOL);
}

static void check_eol(struct preproc_info *pp)
{
	struct token *t;

	t = preproc_next_token(pp);
	if (t -> ttype != TOK_EOL)
		preproc_throw_warning(pp, "Extra text after preprocessor directive");
	skip_eol(pp);
}

static void dir_ifdef(struct preproc_info *pp)
{
	struct token *ct;
	
	if (pp -> skip_level)
	{
		pp -> skip_level++;
		skip_eol(pp);
		return;
	}
	
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	if (ct -> ttype != TOK_IDENT)
	{
		preproc_throw_error(pp, "Bad #ifdef");
		skip_eol(pp);
	}
	
	if (symtab_find(pp, ct -> strval) == NULL)
	{
		pp -> skip_level++;
	}
	else
	{
		pp -> found_level++;
	}
	check_eol(pp);
}

static void dir_ifndef(struct preproc_info *pp)
{
	struct token *ct;
	
	if (pp -> skip_level)
	{
		pp -> skip_level++;
		skip_eol(pp);
		return;
	}
	
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	if (ct -> ttype != TOK_IDENT)
	{
		preproc_throw_error(pp, "Bad #ifdef");
		skip_eol(pp);
	}
	
	if (symtab_find(pp, ct -> strval) != NULL)
	{
		pp -> skip_level++;
	}
	else
	{
		pp -> found_level++;
	}
	check_eol(pp);
}

static void dir_if(struct preproc_info *pp)
{
	if (pp -> skip_level || !eval_expr(pp))
		pp -> skip_level++;
	else
		pp -> found_level++;
}

static void dir_elif(struct preproc_info *pp)
{
	if (pp -> skip_level == 0)
		pp -> else_skip_level = pp -> found_level;
	if (pp -> skip_level)
	{
		if (pp -> else_skip_level > pp -> found_level)
			;
		else if (--(pp -> skip_level) != 0)
			pp -> skip_level++;
		else if (eval_expr(pp))
			pp -> found_level++;
		else
			pp -> skip_level++;
	}
	else if (pp -> found_level)
	{
		pp -> skip_level++;
		pp -> found_level--;
	}
	else
		preproc_throw_error(pp, "#elif in non-conditional section");
}

static void dir_else(struct preproc_info *pp)
{
	if (pp -> skip_level)
	{
		if (pp -> else_skip_level > pp -> found_level)
			;
		else if (--(pp -> skip_level) != 0)
			pp -> skip_level++;
		else
			pp -> found_level++;
	}
	else if (pp -> found_level)
	{
		pp -> skip_level++;
		pp -> found_level--;
	}
	else
	{
		preproc_throw_error(pp, "#else in non-conditional section");
	}
	if (pp -> else_level == pp -> found_level + pp -> skip_level)
	{
		preproc_throw_error(pp, "Too many #else");
	}
	pp -> else_level = pp -> found_level + pp -> skip_level;
	check_eol(pp);
}

static void dir_endif(struct preproc_info *pp)
{
	if (pp -> skip_level)
		pp -> skip_level--;
	else if (pp -> found_level)
		pp -> found_level--;
	else
		preproc_throw_error(pp, "#endif in non-conditional section");
	if (pp -> skip_level == 0)
		pp -> else_skip_level = 0;
	pp -> else_level = 0;
	check_eol(pp);
}

static void dir_define(struct preproc_info *pp)
{
	struct token *tl = NULL;
	struct token *ttl;
	struct token *ct;
	int nargs = -1;
	int vargs = 0;
	char *mname = NULL;

	char **arglist = NULL;
	
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}

	ct = preproc_next_token_nws(pp);
	if (ct -> ttype != TOK_IDENT)
		goto baddefine;
	
	mname = lw_strdup(ct -> strval);
	ct = preproc_next_token(pp);
	
	if (ct -> ttype == TOK_WSPACE)
	{
		/* object like macro */
	}
	else if (ct -> ttype == TOK_EOL)
	{
		/* object like macro - empty value */
		goto out;
	}
	else if (ct -> ttype == TOK_OPAREN)
	{
		/* function like macro - parse args */
		nargs = 0;
		vargs = 0;
		for (;;)
		{
			ct = preproc_next_token_nws(pp);
			if (ct -> ttype == TOK_EOL)
			{
				goto baddefine;
			}
			if (ct -> ttype == TOK_CPAREN)
				break;
			
			if (ct -> ttype == TOK_IDENT)
			{
				/* parameter name */
				nargs++;
				/* record argument name */
				arglist = lw_realloc(arglist, sizeof(char *) * nargs);
				arglist[nargs - 1] = lw_strdup(ct -> strval);
				
				/* check for end of args or comma */
				ct = preproc_next_token_nws(pp);
				if (ct -> ttype == TOK_CPAREN)
					break;
				else if (ct -> ttype == TOK_COMMA)
					continue;
				else
					goto baddefine;
			}
			else if (ct -> ttype == TOK_ELLIPSIS)
			{
				/* variadic macro */
				vargs = 1;
				ct = preproc_next_token_nws(pp);
				if (ct -> ttype != TOK_CPAREN)
					goto baddefine;
				break;
			}
			else
				goto baddefine;
		}
	}
	else
	{
baddefine:
		preproc_throw_error(pp, "bad #define");
baddefine2:
		skip_eol(pp);
		lw_free(mname);
		while (nargs > 0)
			lw_free(arglist[--nargs]);
		lw_free(arglist);
		return;
	}
	
	for (;;)
	{
		ct = preproc_next_token(pp);
		if (ct -> ttype == TOK_EOL)
			break;
		if (!tl)
			tl = ct;
		else
			ttl -> next = ct;
		ttl = ct;
		pp -> curtok = NULL;			// tell *_next_token* not to clobber token
	}
out:
	if (strcmp(mname, "defined") == 0)
	{
		preproc_throw_warning(pp, "attempt to define 'defined' as a macro not allowed");
		goto baddefine2;
	}
	else if (symtab_find(pp, mname) != NULL)
	{
		/* need to do a token compare between the old value and the new value
		   to decide whether to complain */
		preproc_throw_warning(pp, "%s previous defined", mname);
		symtab_undef(pp, mname);
	}
	symtab_define(pp, mname, tl, nargs, arglist, vargs);
	lw_free(mname);
	while (nargs > 0)
		lw_free(arglist[--nargs]);
	lw_free(arglist);
	/* no need to check for EOL here */
}

static void dir_undef(struct preproc_info *pp)
{
	struct token *ct;
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}
	
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	if (ct -> ttype != TOK_IDENT)
	{
		preproc_throw_error(pp, "Bad #undef");
		skip_eol(pp);
	}
	
	symtab_undef(pp, ct -> strval);
	check_eol(pp);
}

char *streol(struct preproc_info *pp)
{
	struct strbuf *s;
	struct token *ct;
	int i;
		
	s = strbuf_new();
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	while (ct -> ttype != TOK_EOL)
	{
		for (i = 0; ct -> strval[i]; i++)
			strbuf_add(s, ct -> strval[i]);
		ct = preproc_next_token(pp);
	}
	return strbuf_end(s);
}

static void dir_error(struct preproc_info *pp)
{
	char *s;
	
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}
	
	s = streol(pp);
	preproc_throw_error(pp, "%s", s);
	lw_free(s);
}

static void dir_warning(struct preproc_info *pp)
{
	char *s;
	
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}
	
	s = streol(pp);
	preproc_throw_warning(pp, "%s", s);
	lw_free(s);
}

static void dir_include(struct preproc_info *pp)
{
}

static void dir_line(struct preproc_info *pp)
{
}

static void dir_pragma(struct preproc_info *pp)
{
	if (pp -> skip_level)
	{
		skip_eol(pp);
		return;
	}
	
	preproc_throw_warning(pp, "Unsupported #pragma");
	skip_eol(pp);
}

struct { char *name; void (*fn)(struct preproc_info *); } dirlist[] =
{
	{ "ifdef", dir_ifdef },
	{ "ifndef", dir_ifndef },
	{ "if", dir_if },
	{ "else", dir_else },
	{ "elif", dir_elif },
	{ "endif", dir_endif },
	{ "define", dir_define },
	{ "undef", dir_undef },
	{ "include", dir_include },
	{ "error", dir_error },
	{ "warning", dir_warning },
	{ "line", dir_line },
	{ "pragma", dir_pragma },
	{ NULL, NULL }
};

static void process_directive(struct preproc_info *pp)
{
	struct token *ct;
	int i;
	
	do
	{
		ct = preproc_next_token(pp);
	} while (ct -> ttype == TOK_WSPACE);
	
	// NULL directive
	if (ct -> ttype == TOK_EOL)
		return;
	
	if (ct -> ttype != TOK_IDENT)
		goto baddir;
	
	for (i = 0; dirlist[i].name; i++)
	{
		if (strcmp(dirlist[i].name, ct -> strval) == 0)
		{
			(*(dirlist[i].fn))(pp);
			return;
		}
	}
baddir:
		preproc_throw_error(pp, "Bad preprocessor directive");
		while (ct -> ttype != TOK_EOL)
			ct = preproc_next_token(pp);
		return;
}

/*
Evaluate a preprocessor expression
*/

/* same as skip_eol() but the EOL token is not consumed */
static void skip_eoe(struct preproc_info *pp)
{
	skip_eol(pp);
	preproc_unget_token(pp, pp -> curtok);
}

static long eval_expr_real(struct preproc_info *, int);
static long preproc_numval(struct token *);

static long eval_term_real(struct preproc_info *pp)
{
	long tval = 0;
	struct token *ct;
	
eval_next:
	ct = preproc_next_processed_token_nws(pp);
	if (ct -> ttype == TOK_EOL)
	{
		preproc_throw_error(pp, "Bad expression");
		return 0;
	}
	
	switch (ct -> ttype)
	{
	case TOK_OPAREN:
		tval = eval_expr_real(pp, 0);
		ct = preproc_next_processed_token_nws(pp);
		if (ct -> ttype != ')')
		{
			preproc_throw_error(pp, "Unbalanced () in expression");
			skip_eoe(pp);
			return 0;
		}
		return tval;

	case TOK_ADD: // unary +
		goto eval_next;

	case TOK_SUB: // unary -	
		tval = eval_expr_real(pp, 200);
		return -tval;

	/* NOTE: we should only get "TOK_IDENT" from an undefined macro */		
	case TOK_IDENT: // some sort of function, symbol, etc.
		if (strcmp(ct -> strval, "defined"))
		{
			/* the defined operator */
			/* any number in the "defined" bit will be
			   treated as a defined symbol, even zero */
			ct = preproc_next_token_nws(pp);
			if (ct -> ttype == TOK_OPAREN)
			{
				ct = preproc_next_token_nws(pp);
				if (ct -> ttype != TOK_IDENT)
				{
					preproc_throw_error(pp, "Bad expression");
					skip_eoe(pp);
					return 0;
				}
				if (symtab_find(pp, ct -> strval) == NULL)
					tval = 0;
				else
					tval = 1;
				ct = preproc_next_token_nws(pp);
				if (ct -> ttype != TOK_CPAREN)
				{
					preproc_throw_error(pp, "Bad expression");
					skip_eoe(pp);
					return 0;
				}
				return tval;
			}
			else if (ct -> ttype == TOK_IDENT)
			{
				return (symtab_find(pp, ct -> strval) != NULL) ? 1 : 0;
			}
			preproc_throw_error(pp, "Bad expression");
			skip_eoe(pp);
			return 0;
		}
		/* unknown identifier - it's zero */
		return 0;
	
	/* numbers */
	case TOK_NUMBER:
		return preproc_numval(ct);	
		
	default:
		preproc_throw_error(pp, "Bad expression");
		skip_eoe(pp);
		return 0;
	}
	return 0;
}

static long eval_expr_real(struct preproc_info *pp, int p)
{
	static const struct operinfo
	{
		int tok;
		int prec;
	} operators[] =
	{
		{ TOK_ADD, 100 },
		{ TOK_SUB, 100 },
		{ TOK_STAR, 150 },
		{ TOK_DIV, 150 },
		{ TOK_MOD, 150 },
		{ TOK_LT, 75 },
		{ TOK_LE, 75 },
		{ TOK_GT, 75 },
		{ TOK_GE, 75 },
		{ TOK_EQ, 70 },
		{ TOK_NE, 70 },
		{ TOK_BAND, 30 },
		{ TOK_BOR, 25 },
		{ TOK_NONE, 0 }
	};
	
	int op;
	long term1, term2, term3;
	struct token *ct;
	
	term1 = eval_term_real(pp);
eval_next:
	ct = preproc_next_processed_token_nws(pp);
	for (op = 0; operators[op].tok != TOK_NONE; op++)
	{
		if (operators[op].tok == ct -> ttype)
			break;
	}
	/* if it isn't a recognized operator, assume end of expression */
	if (operators[op].tok == TOK_NONE)
	{
		preproc_unget_token(pp, ct);
		return term1;
	}

	/* if new operation is not higher than the current precedence, let the previous op finish */
	if (operators[op].prec <= p)
		return term1;

	/* get the second term */
	term2 = eval_expr_real(pp, operators[op].prec);
	
	switch (operators[op].tok)
	{
	case TOK_ADD:
		term3 = term1 + term2;
		break;
	
	case TOK_SUB:
		term3 = term1 - term2;
		break;
	
	case TOK_STAR:
		term3 = term1 * term2;
		break;
	
	case TOK_DIV:
		if (!term2)
		{
			preproc_throw_warning(pp, "Division by zero");
			term3 = 0;
			break;
		}
		term3 = term1 / term2;
		break;
	
	case TOK_MOD:
		if (!term2)
		{
			preproc_throw_warning(pp, "Division by zero");
			term3 = 0;
			break;
		}
		term3 = term1 % term2;
		break;
		
	case TOK_BAND:
		term3 = (term1 && term2);
		break;
	
	case TOK_BOR:
		term3 = (term1 || term2);
		break;
	
	case TOK_EQ:
		term3 = (term1 == term2);
		break;
	
	case TOK_NE:
		term3 = (term1 != term2);
		break;
	
	case TOK_GT:
		term3 = (term1 > term2);
		break;
	
	case TOK_GE:
		term3 = (term1 >= term2);
		break;
	
	case TOK_LT:
		term3 = (term1 < term2);
		break;
	
	case TOK_LE:
		term3 = (term1 <= term2);
		break;

	default:
		term3 = 0;
		break;
	}
	term1 = term3;
	goto eval_next;
}

static long eval_expr(struct preproc_info *pp)
{
	long rv;
	
	rv = eval_expr_real(pp, 0);
	if (pp -> curtok -> ttype != TOK_EOL)
	{
		preproc_throw_error(pp, "Bad expression");
		skip_eol(pp);
	}
	return rv;
}

/* convert a numeric string to a number */
long preproc_numval(struct token *t)
{
	return 0;
}

/*
Below here is the logic for expanding a macro
*/
static int expand_macro(struct preproc_info *pp, char *mname)
{
	struct symtab_e *s;
	struct token *t, *t2, *t3;
	struct token **arglist = NULL;
	int nargs = 0;
	struct expand_e *e;
	struct token **exparglist = NULL;
	int i;
	int pcount;
		
	s = symtab_find(pp, mname);
	if (!s)
		return 0;
	
	for (e = pp -> expand_list; e; e = e -> next)
	{
		/* don't expand if we're already expanding the same macro */
		if (e -> s == s)
			return 0;
	}

	if (s -> nargs == -1)
	{
		/* short circuit NULL expansion */
		if (s -> tl == NULL)
			return 1;

		goto expandmacro;
	}
	
	// look for opening paren after optional whitespace
	t2 = NULL;
	t = NULL;
	for (;;)
	{
		t = preproc_next_token(pp);
		if (t -> ttype != TOK_WSPACE && t -> ttype != TOK_EOL)
			break;
		t -> next = t2;
		t2 = t2;
	}
	if (t -> ttype != TOK_OPAREN)
	{
		// not a function-like invocation
		while (t2)
		{
			t = t2 -> next;
			preproc_unget_token(pp, t2);
			t2 = t;
		}
		return 0;
	}
	
	// parse parameters here
	t = preproc_next_token_nws(pp);
	nargs = 1;
	arglist = lw_alloc(sizeof(struct token *));
	arglist[0] = NULL;
	t2 = NULL;
	
	while (t -> ttype != TOK_CPAREN)
	{
		pcount = 0;
		if (t -> ttype == TOK_EOF)
		{
			preproc_throw_error(pp, "Unexpected EOF in macro call");
			break;
		}
		if (t -> ttype == TOK_EOL)
			continue;
		if (t -> ttype == TOK_OPAREN)
			pcount++;
		else if (t -> ttype == TOK_CPAREN && pcount)
			pcount--;
		if (t -> ttype == TOK_COMMA && pcount == 0)
		{
			if (!(s -> vargs) || (nargs > s -> nargs))
			{
				nargs++;
				arglist = lw_realloc(arglist, sizeof(struct token *) * nargs);
				arglist[nargs - 1] = NULL;
				t2 = NULL;
				continue;
			}
		}
		if (t2)
		{
			t2 -> next = token_dup(t);
			t2 = t2 -> next;
		}
		else
		{
			t2 = token_dup(t);
			arglist[nargs - 1] = t2;
		}
	}

	if (s -> vargs)
	{
		if (nargs <= s -> nargs)
		{
			preproc_throw_error(pp, "Wrong number of arguments (%d) for variadic macro %s which takes %d arguments", nargs, mname, s -> nargs);
		}
	}
	else
	{
		if (s -> nargs != nargs && !(s -> nargs == 0 && nargs == 1 && arglist[nargs - 1]))
		{
			preproc_throw_error(pp, "Wrong number of arguments (%d) for macro %s which takes %d arguments", nargs, mname, s -> nargs);
		}
	}

	/* now calculate the pre-expansions of the arguments */
	exparglist = lw_alloc(nargs);
	for (i = 0; i < nargs; i++)
	{
		t2 = NULL;
		exparglist[i] = NULL;
		// NOTE: do nothing if empty argument
		if (arglist[i] == NULL)
			continue;
		pp -> sourcelist = arglist[i];
		for (;;)
		{
			t = preproc_next_processed_token(pp);
			if (t -> ttype == TOK_EOF)
				break;
			if (t2)
			{
				t2 -> next = token_dup(t);
				t2 = t2 -> next;
			}
			else
			{
				t2 = token_dup(t);
				exparglist[i] = t2;
			}
		}
	}

expandmacro:
	t2 = NULL;
	t3 = NULL;
	
	for (t = s -> tl; t; t = t -> next)
	{
		if (t -> ttype == TOK_IDENT)
		{
			/* identifiers might need expansion to arguments */
			if (strcmp(t -> strval, "__VA_ARGS__") == 0)
			{
				i = s -> nargs;
			}
			else
			{
				for (i = 0; i < nargs; i++)
				{
					if (strcmp(t -> strval, s -> params[i]) == 0)
						break;
				}
			}
			if ((i == s -> nargs) && !(s -> vargs))
			{
				struct token *te;
				// expand argument
				// FIXME: handle # and ##
				for (te = exparglist[i]; te; te = te -> next)
				{
					if (t2)
					{
						t2 -> next = token_dup(te);
						t2 = t2 -> next;
					}
					else
					{
						t3 = token_dup(te);
						t2 = t2;
					}
				}
				continue;
			}
		}
		if (t2)
		{
			t2 -> next = token_dup(t);
			t2 = t2 -> next;
		}
		else
		{
			t3 = token_dup(t);
			t2 = t3;
		}
	}

	/* put the new expansion in front of the input, if relevant; if we
	   expanded to nothing, no need to create an expansion record or
	   put anything into the input queue */
	if (t3)
	{
		t2 -> next = token_create(TOK_ENDEXPAND, "", -1, -1, "");
		t2 -> next -> next = pp -> tokqueue;
		pp -> tokqueue = t3;

		/* set up expansion record */
		e = lw_alloc(sizeof(struct expand_e));
		e -> next = pp -> expand_list;
		pp -> expand_list = e;
		e -> s = s;
	}
	
	/* now clean up */
	for (i = 0; i < nargs; i++)
	{
		lw_free(arglist[i]);
		lw_free(exparglist[i]);
	}
	lw_free(arglist);
	lw_free(exparglist);
	
	return 1;
}
