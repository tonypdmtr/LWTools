/*
lwcc/cpp/preproc.c

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "cpp.h"


int munch_comment(void);
char *parse_str_lit(void);
char *parse_chr_lit(void);
char *parse_num_lit(int);
char *parse_identifier(int);
void preprocess_identifier(char *);
void preprocess_directive(void);
void next_token(void);
void next_token_nws(void);
int eval_expr(void);

int skip_level = 0;
int found_level = 0;
int else_level = 0;
int else_skip_level = 0;

struct token curtok = { .ttype = TOK_NONE, .strval = NULL };

/*
Notes:

Rather than tokenize the entire file, we run through it interpreting
things only as much as we need to in order to identify the following:

preprocessing directives (#...)
identifiers which might need to be replaced with macros

We have to interpret strings, character constants, and numbers to prevent
false positives in those situations.

When we find a preprocessing directive, it is handled with a more
aggressive tokenization process and then intepreted accordingly.

nlws is used to record the fact that only whitespace has occurred at the
start of a line. Whitespace is defined as comments or isspace(c). It gets
reset to 1 after each EOL character. If a non-whitespace character is
encountered, it is set to -1. If the character processing decides it really
is a whitespace character, it will set nlws back to 1 (block comment).
Elsewise, it will get set to 0 if it is still -1 when the loop starts again.

This is needed so we can identify whitespace interposed before a
preprocessor directive. This is the only case where it matters for
the preprocessor.

*/
void preprocess_file()
{
	int c;
	int nlws = 1;
	
	preprocess_output_location(1);
	for (;;)
	{
		c = fetch_byte();
		// if we had non-whitespace that wasn't munched (comment), set flag correctly
		if (nlws == -1)
			nlws = 0;
		if (c == CPP_EOF)
		{
			// end of input - make sure newline is present
			outchr('\n');
			return;
		}
		if (c == CPP_EOL)
		{
			// flag that we just hit the start of a new line
			nlws = 1;
			outchr(CPP_EOL);
			continue;
		}
		
		/* if we have a non-whitespace character, flag it as such */
		if (!is_whitespace(c))
			nlws = -1;
		
		if (c == '#' && nlws)
		{
			// we have a preprocessor directive here - this call will do
			// everything including outputting the blank line, if appropriate
			preprocess_directive();
			continue;
		}
		else if (c == '\'')
		{
			// we have a character constant here
			outstr(parse_chr_lit());
			continue;
		}
		else if (c == '"')
		{
			// we have a string constant here
			outstr(parse_str_lit());
			continue;
		}
		else if (c == '.')
		{
			// we might have a number here
			outchr('.');
			c = fetch_byte();
			if (is_dec(c))
				outstr(parse_num_lit(c));
			continue;
		}
		else if (is_dec(c))
		{
			// we have a number here
			outstr(parse_num_lit(c));
		}
		else if (c == '/')
		{
			// we might have a comment here
			c = munch_comment();
			if (c < 0)
			{
				outchr('/');
				continue;
			}
			// comments are white space - count them as such at start of line
			if (nlws == -1)
				nlws = 0;
			/* c is the number of EOL characters the comment spanned */
			while (c--)
				outchr(CPP_EOL);
			continue;
		}
		else if (c == 'L')
		{
			// wide character string or wide character constant, or identifier
			c = fetch_byte();
			if (c == '"')
			{
				outchr('L');
				outstr(parse_str_lit());
				continue;
			}
			else if (c == '\'')
			{
				outchr('L');
				outstr(parse_chr_lit());
				continue;
			}
			unfetch_byte(c);
			preprocess_identifier(parse_identifier('L'));
			continue;
		}
		else if (is_sidchr(c))
		{
			// identifier of some kind
			char *s;
			s = parse_identifier(c);
			preprocess_identifier(s);
			continue;
		}
		else
		{
			// random character - pass through
			outchr(c);
		}
	}	
}

char *parse_identifier(int c)
{
	static char *ident = NULL;
	int idlen = 0;
	static int idbufl = 0;

	do
	{
		if (idlen >= idbufl)
		{
			idbufl += 50;
			ident = lw_realloc(ident, idbufl);
		}
		ident[idlen++] = c;
		c = fetch_byte();
	} while (is_idchr(c));

	ident[idlen++] = 0;
	unfetch_byte(c);

	return ident;
}

void preprocess_identifier(char *s)
{
	/* do something with the identifier here  - macros, etc. */
	outstr(s);
}

#define to_buf(c) do { if (idlen >= idbufl) { idbufl += 100; ident = lw_realloc(ident, idbufl); } ident[idlen++] = (c); } while (0)
char *parse_num_lit(int c)
{
	static char *ident = NULL;
	int idlen = 0;
	static int idbufl = 0;
	
	do
	{
		to_buf(c);
		c = fetch_byte();
		if (is_ep(c))
		{
			to_buf(c);
			c = fetch_byte();
			if (c == '-' || c == '+')
			{
				to_buf(c);
				c = fetch_byte();
			}
		}
	} while ((is_idchr(c)) || (c == '.'));
	to_buf(0);
	
	return ident;
}

char *parse_chr_lit(void)
{
	static char *ident = NULL;
	int idlen = 0;
	static int idbufl = 0;
	int c;
		
	to_buf('\'');
	while ((c = fetch_byte()) != '\'')
	{
		if (c == CPP_EOL || c == CPP_EOF)
		{
			unfetch_byte(c);
			to_buf(0);
			do_warning("Unterminated character constant");
			return ident;
		}
		if (c == '\\')
		{
			to_buf(c);
			c = fetch_byte();
			if (c == CPP_EOL || c == CPP_EOF)
			{
				unfetch_byte(c);
				to_buf(0);
				do_warning("Unterminated character constant");
				return ident;
			}
		}
		to_buf(c);
	}
	to_buf(c);
	to_buf(0);
	return ident;
}

char *parse_str_lit(void)
{
	static char *ident = NULL;
	int idlen = 0;
	static int idbufl = 0;
	int c;
	
	to_buf('"');
	while ((c = fetch_byte()) != '"')
	{
		if (c == CPP_EOL || c == CPP_EOF)
		{
			unfetch_byte(c);
			to_buf(0);
			do_warning("Unterminated string literal");
			return ident;
		}
		if (c == '\\')
		{
			to_buf(c);
			c = fetch_byte();
			if (c == CPP_EOL || c == CPP_EOF)
			{
				unfetch_byte(c);
				to_buf(0);
				do_warning("Unterminated string literal");
				return ident;
			}
		}
		to_buf(c);
	}
	to_buf(c);
	to_buf(0);
	return ident;
}

int munch_comment(void)
{
	int nlc = 0;
	int c;
	
	c = fetch_byte();
	if (c == '/')
	{
		// single line comment
		for (;;)
		{
			c = fetch_byte();
			if (c == CPP_EOL)
				nlc = 1;
			if (c == CPP_EOL || c == CPP_EOF)
				return nlc;
		}
	}
	else if (c == '*')
	{
		// block comment
		for (;;)
		{
			c = fetch_byte();
			if (c == CPP_EOL)
				nlc++;
			if (c == CPP_EOF)
				return nlc;
			if (c == '*')
			{
				c = fetch_byte();
				if (c == '/' || c == CPP_EOF)
					return nlc;
				if (c == CPP_EOL)
					nlc++;
			}
		}
		return nlc;
	}
	else
	{
		unfetch_byte(c);
		return -1;
	}
	
	return nlc;
}

/* Output a location directive to synchronize the compiler with the correct
   input line number and file. This is of the form:

# <linenum> <filename> <flag>

where <linenum> is the line number inside the file, <filename> is the
filename (as a C string), and <flag> is the specified flag argument which
should be 1 for the start of a new file or 2 for returning to the file from
another file. <linenum> is the line number the following line came from.
 */
void preprocess_output_location(int flag)
{
	fprintf(output_fp, "# %d \"%s\" %d\n", file_stack -> line, file_stack -> fn, flag);
}

void preproc_ifndef(void);
void preproc_ifdef(void);
void preproc_if(void);
void preproc_include(void);
void preproc_else(void);
void preproc_endif(void);
void preproc_error(void);
void preproc_warning(void);
void preproc_define(void);
void preproc_undef(void);
void preproc_line(void);
void preproc_pragma(void);
void preproc_elif(void);

struct { char *name; void (*fn)(void); } directive_list[] = {
	{ "ifndef",			preproc_ifndef },
	{ "ifdef",			preproc_ifdef },
	{ "if",				preproc_if },
	{ "include",		preproc_include },
	{ "else",			preproc_else },
	{ "endif",			preproc_endif },
	{ "error",			preproc_error },
	{ "warning",		preproc_warning },
	{ "define",			preproc_define },
	{ "undef",			preproc_undef },
	{ "line",			preproc_line },
	{ "pragma",			preproc_pragma },
	{ "elif",			preproc_elif },
	{ NULL, NULL }
};

/* process a preprocessor directive */
#define DIRBUFLEN 20
void preprocess_directive(void)
{
	static char dirbuf[DIRBUFLEN+1];
	int c;
	int dl = 0;
	
	for (;;)
	{
		c = fetch_byte();
		if (is_whitespace(c))
			continue;
		if (c == '/')
		{
			c = munch_comment();
			if (c < 0)
				goto baddir;
			if (c > 0)
			{
				while (c--)
					outchr(CPP_EOL);
			}
			continue;
		}
		if (c == CPP_EOL)
		{
			// NULL directive - do nothing
			outchr(CPP_EOL);
			return;
		}
		break;
	}	


	dl = 0;
	while (((c >= 'a' && c <= 'z') || c == '_') && dl < DIRBUFLEN)
	{
		dirbuf[dl++] = c;
		c = fetch_byte();
	}
	dirbuf[dl] = 0;

commagain:
	if (c == '/')
	{
		c = munch_comment();
		if (c < 0)
			c = '/';
		else
		{
			while (c--)
			{
				outchr(CPP_EOL);
			}
			c = fetch_byte();
			goto commagain;
		}
	}
	
	if (!is_whitespace(c) && c != CPP_EOL && c != CPP_EOF)
		goto baddir;
	
	for (dl = 0; directive_list[dl].name; dl++)
	{
		if (strcmp(directive_list[dl].name, dirbuf) == 0)
		{
			(*(directive_list[dl].fn))();
			outchr(CPP_EOL);
			return;
		}
	}

baddir:
	dirbuf[dl] = 0;
	if (skip_level == 0)
		do_error("Bad preprocessor directive %s", dirbuf);
	outchr(CPP_EOL);
}

void check_eol(void)
{
	next_token_nws();
	if (curtok.ttype == TOK_EOL)
		return;
	if (curtok.ttype == TOK_EOF)
		return;
	do_warning("Extra text after preprocessor directive");
	skip_eol();
}

void preproc_ifndef(void)
{
	if (skip_level)
	{
		skip_level++;
		skip_eol();
		return;
	}
	next_token_nws();
	if (curtok.ttype != TOK_IDENT)
	{
		do_error("Bad #ifndef");
		skip_eol();
	}
	
	if (symbol_find(curtok.strval))
	{
		skip_level++;
	}
	else
	{
		found_level++;
	}
	check_eol();
}

void preproc_ifdef(void)
{
	if (skip_level)
	{
		skip_level++;
		skip_eol();
		return;
	}
	next_token_nws();
	if (curtok.ttype != TOK_IDENT)
	{
		do_error("Bad #ifdef");
		skip_eol();
	}
	
	if (symbol_find(curtok.strval) == NULL)
	{
		skip_level++;
	}
	else
	{
		found_level++;
	}
	check_eol();
}

void preproc_if(void)
{
	skip_eol();
}

void preproc_include(void)
{
	skip_eol();
}

void preproc_else(void)
{
	if (skip_level)
	{
		if (else_skip_level > found_level)
			;
		else if (--skip_level != 0)
			skip_level++;
		else
			found_level++;
	}
	else if (found_level)
	{
		skip_level++;
		found_level--;
	}
	else
	{
		do_error("#else in non-conditional section");
	}
	if (else_level == found_level + skip_level)
	{
		do_error("Too many #else");
	}
	else_level = found_level + skip_level;
	check_eol();
}

void preproc_endif(void)
{
	if (skip_level)
		skip_level--;
	else if (found_level)
		found_level--;
	else
		do_error("#endif in non-conditional section");
	if (skip_level == 0)
		else_skip_level = 0;
	else_level = 0;
	check_eol();
}

void preproc_error(void)
{
	skip_eol();
}

void preproc_warning(void)
{
	skip_eol();
}

void preproc_define(void)
{
	skip_eol();
}

void preproc_undef(void)
{
	if (skip_level)
	{
		skip_eol();
		return;
	}
	
	next_token_nws();
	if (curtok.ttype != TOK_IDENT)
	{
		do_error("Bad #undef");
		symbol_undef(curtok.strval);
	}
	check_eol();
}

void preproc_line(void)
{
	skip_eol();
}

void preproc_pragma(void)
{
	if (skip_level || !eval_expr())
		skip_level++;
	else
		found_level++;
}

void preproc_elif(void)
{
	if (skip_level == 0)
		else_skip_level = found_level;
	if (skip_level)
	{
		if (else_skip_level > found_level)
			;
		else if (--skip_level != 0)
			skip_level++;
		else if (eval_expr())
			found_level++;
		else
			skip_level++;
	}
	else if (found_level)
	{
		skip_level++;
		found_level--;
	}
	else
		do_error("#elif in non-conditional section");
}



/* tokenizing stuff here */
#undef to_buf
#define to_buf(c) do { if (strlen >= strbufl) { strbufl += 100; strbuf = lw_realloc(strbuf, strbufl); } strbuf[strlen++] = (c); strbuf[strlen] = 0; } while (0)
void next_token(void)
{
	int strbufl = 0;
	int strlen = 0;
	char *strbuf = NULL;
	int c;
	int ttype;
		
	lw_free(curtok.strval);
	curtok.strval = NULL;
	curtok.ttype = TOK_NONE;
	
	c = fetch_byte();
	if (c == CPP_EOL)
	{
		curtok.ttype = TOK_EOL;
		return;
	}
	
	if (c == CPP_EOF)
	{
		curtok.ttype = TOK_EOF;
		return;
	}
	
	if (is_whitespace(c))
	{
		do
		{
			to_buf(c);
			c = fetch_byte();
		} while (is_whitespace(c));
		unfetch_byte(c);
		ttype = TOK_WSPACE;
		goto out;
	}
	if (c == '/')
	{
		c = munch_comment();
		if (c >= 0)
		{
			to_buf(' ');
			while (c--)
				outchr(CPP_EOL);
			ttype = TOK_WSPACE;
			goto out;
		}
		c = '/';
	}

	if (c == '\'')
	{
		// we have a character constant here
		ttype = TOK_NUMBER;
		strbuf = lw_strdup(parse_chr_lit());
		goto out;
	}
	else if (c == '"')
	{
		// we have a string constant here
		ttype = TOK_STRING;
		strbuf = lw_strdup(parse_str_lit());
		goto out;
	}
	else if (c == '.')
	{
		// we might have a number here
		c = fetch_byte();
		if (is_dec(c))
		{
			unfetch_byte(c);
			ttype = TOK_NUMBER;
			strbuf = lw_strdup(parse_num_lit('.'));
			goto out;
		}
		else
		{
			goto ttypegen;
		}
	}
	else if (is_dec(c))
	{
		// we have a number here
		ttype = TOK_NUMBER;
		strbuf = lw_strdup(parse_num_lit(c));
	}
	else if (c == 'L')
	{
		// wide character string or wide character constant, or identifier
		c = fetch_byte();
		if (c == '"')
		{
			char *s;
			to_buf('L');
			s = parse_str_lit();
			while (*s)
				to_buf(*s++);
			ttype = TOK_STRING;
			goto out;
		}
		else if (c == '\'')
		{
			char *s;
			to_buf('L');
			s = parse_chr_lit();
			while (*s)
				to_buf(*s++);
			ttype = TOK_NUMBER;
			goto out;
		}
		unfetch_byte(c);
		ttype = TOK_IDENT;
		strbuf = lw_strdup(parse_identifier('L'));
		goto out;
	}
	else if (is_sidchr(c))
	{
		// identifier of some kind
		strbuf = lw_strdup(parse_identifier(c));
		ttype = TOK_IDENT;
	}
	else
	{
ttypegen:
		ttype = TOK_CHAR;
		to_buf(c);
		
		switch (c)
		{
		case '/':
			ttype = TOK_DIV;
			break;
		
		case '*':
			ttype = TOK_MUL;
			break;
		
		case '+':
			ttype = TOK_ADD;
			break;
		
		case '-':
			ttype = TOK_SUB;
			break;
		
		case '<':
			c = fetch_byte();
			if (c == '=')
				ttype = TOK_LE;
			else
			{
				ttype = TOK_LT;
				unfetch_byte(c);
			}
			break;
		
		case '>':
			c = fetch_byte();
			if (c == '=')
				ttype = TOK_GE;
			else
			{
				ttype = TOK_GT;
				unfetch_byte(c);
			}
			break;
		
		case '=':
			c = fetch_byte();
			if (c == '=')
				ttype = TOK_EQ;
			else
				unfetch_byte(c);
			break;
			
		case '!':
			c = fetch_byte();
			if (c == '=')
				ttype = TOK_NE;
			else
			{
				ttype = TOK_BNOT;
				unfetch_byte(c);
			}
			break;
			
		case '&':
			c = fetch_byte();
			if (c == '&')
				ttype = TOK_BAND;
			else
				unfetch_byte(c);
			break;
		
		case '|':
			c = fetch_byte();
			if (c == '|')
				ttype = TOK_BOR;
			else
				unfetch_byte(c);
			break;
				
		case '(':
			ttype = TOK_OPAREN;
			break;
			
		case ')':
			ttype = TOK_CPAREN;
			break;
			
		}
		goto out;
	}
	
out:
	curtok.ttype = ttype;
	curtok.strval = strbuf;
}

void next_token_nws(void)
{
	do
	{
		next_token();
	} while (curtok.ttype == TOK_WSPACE);
}


/*
evaluate an expression. Return true if expression is true, false if it
is false. Expression ends at the end of the line. Enter at eval_expr().
   
eval_term_real() evaluates a term in the expression. eval_expr_real() is
the main expression evaluator.
*/

int eval_expr(void)
{
	skip_eol();
	return 0;
}
