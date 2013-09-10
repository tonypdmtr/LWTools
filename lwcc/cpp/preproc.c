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

#include <lw_alloc.h>

#include "cpp.h"


int munch_comment(void);
char *parse_str_lit(void);
char *parse_chr_lit(void);
char *parse_num_lit(int);
void preprocess_identifier(int);
void preprocess_directive(void);


int skip_level;

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
			preprocess_identifier('L');
			continue;
		}
		else if (is_sidchr(c))
		{
			// identifier of some kind
			preprocess_identifier(c);
			continue;
		}
		else
		{
			// random character - pass through
			outchr(c);
		}
	}	
}

void preprocess_identifier(int c)
{
	char *ident = NULL;
	int idlen = 0;
	int idbufl = 0;

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
	
	/* do something with the identifier here  - macros, etc. */
	outstr(ident);
	lw_free(ident);
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
	} while ((is_dec(c)) || (c == '.'));
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

/* process a preprocessor directive */
void preprocess_directive(void)
{
	outchr('>');
	outchr('#');
}
