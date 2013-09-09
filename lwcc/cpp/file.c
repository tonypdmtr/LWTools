/*
lwcc/cpp/file.c

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


NOTES:

The function fetch_byte() grabs a byte from the input file. It returns
CPP_EOF if end of file has been reached. The resulting byte has passed
through three filters, in order:

* All CRLF, LFCR, LF, and CR have been converted to CPP_EOL
* If enabled (--trigraphs), trigraphs have been interpreted
* \\n (backslash-newline) has been processed (eliminated)

To obtain a byte without processing \\n, call fetch_byte_tg().

*/

#include <errno.h>
#include <stdio.h>
#include <string.h> 

#include <lw_alloc.h>

#include "cpp.h"

struct file_stack_e *file_stack = NULL;

int is_whitespace(int c)
{
	switch (c)
	{
	case ' ':
	case '\t':
	case '\r':
	case '\n':
		return 1;
	}
	return 0;
}

int is_sidchr(c)
{
	if (c == '_' || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
		return 1;
	return 0;
}

int is_idchr(int c)
{
	if (c >= '0' && c <= '9')
		return 1;
	return is_sidchr(c);
}

int is_ep(int c)
{
	if (c == 'e' || c == 'E' || c == 'p' || c == 'P')
		return 1;
	return 0;
}

int is_hex(int c)
{
	if (c >= 'a' && c <= 'f')
		return 1;
	if (c >= 'A' && c <= 'F')
		return 1;
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

int is_dec(int c)
{
	if (c >= '0' && c <= '9')
		return 1;
	return 0;
}

static void outchr(int c)
{
	fputc(c, output_fp);
}

static void outstr(char *s)
{
	while (*s)
		outchr(*s++);
}

int fetch_byte_ll(struct file_stack_e *f)
{
	int c;

	if (f -> eolstate != 0)	
	{
		f -> line++;
		f -> col = 0;
	}
	c = getc(f -> fp);
	f -> col++;
	if (f -> eolstate == 1)
	{
		// just saw CR, munch LF
		if (c == 10)
			c = getc(f -> fp);
		f -> eolstate = 0;
	}
	else if (f -> eolstate == 2)
	{
		// just saw LF, much CR
		if (c == 13)
			c = getc(f -> fp);
		f -> eolstate = 0;
	}
	
	if (c == 10)
	{
		// we have LF - end of line, flag to munch CR
		f -> eolstate = 2;
		c = CPP_EOL;
	}
	else if (c == 13)
	{
		// we have CR - end of line, flag to munch LF
		f -> eolstate = 1;
		c = CPP_EOL;
	}
	else if (c == EOF)
	{
		c = CPP_EOF;
	}
	return c;
}

int fetch_byte_tg(struct file_stack_e *f)
{
	int c;

	if (!trigraphs)
	{
		c = fetch_byte_ll(f);
	}
	else
	{
		/* we have to do the trigraph shit here */
		if (f -> ra != CPP_NOUNG)
		{
			if (f -> qseen > 0)
			{
				c = '?';
				f -> qseen -= 1;
				return c;
			}
			else
			{
				c = f -> ra;
				f -> ra = CPP_NOUNG;
				return c;
			}
		}
	
		c = fetch_byte_ll(f);
		while (c == '?')
		{
			f -> qseen++;
			c = fetch_byte_ll(f);
		}
	
		if (f -> qseen >= 2)
		{
			// we have a trigraph
			switch (c)
			{
			case '=':
				c = '#';
				f -> qseen -= 2;
				break;
			
			case '/':
				c = '\\';
				f -> qseen -= 2;
				break;
		
			case '\'':
				c = '^';
				f -> qseen -= 2;
				break;
		
			case '(':
				c = '[';
				f -> qseen -= 2;
				break;
		
			case ')':
				c = ']';
				f -> qseen -= 2;
				break;
		
			case '!':
				c = '|';
				f -> qseen -= 2;
				break;
		
			case '<':
				c = '{';
				f -> qseen -= 2;
				break;
		
			case '>':
				c = '}';
				f -> qseen -= 2;
				break;
		
			case '~':
				c = '~';
				f -> qseen -= 2;
				break;
			}
			if (f -> qseen > 0)
			{
				f -> ra = c;
				c = '?';
				f -> qseen--;
			}
		}
		else if (f -> qseen > 0)
		{
			f -> ra = c;
			c = '?';
			f -> qseen--;
		}
	}
	return c;
}

int fetch_byte(struct file_stack_e *f)
{
	int c;
	
again:
	if (f -> unget != CPP_NOUNG)
	{
		c = f -> unget;
		f -> unget = CPP_NOUNG;
	}
	else
	{
		c = fetch_byte_tg(f);
	}
	if (c == '\\')
	{
		int c2;
		c2 = fetch_byte_tg(f);
		if (c2 == CPP_EOL)
			goto again;
		else
			f -> unget = c2;
	}
	f -> curc = c;
	return c;
}

static void skip_line(struct file_stack_e *f)
{
	int c;
	while ((c = fetch_byte(f)) != CPP_EOL && c != CPP_EOF)
		/* do nothing */ ;
}


struct
{
	char *name;
	void (*fn)(struct file_stack_e *);
} directives[] =
{
	{ NULL, NULL },
	{ NULL, NULL }
};

/*
This handles a preprocessing directive. Such a directive goes from the
next character to be retrieved from f until the first instance of CPP_EOL
or CPP_EOF.
*/
void handle_directive(struct file_stack_e *f)
{
	int c, i;
	char kw[20];
	
again:
	while ((c = fetch_byte(f)) == ' ' || c == '\t')
		/* do nothing */ ;
	if (c == '/')
	{
		// maybe a comment //
		c = fetch_byte(f);
		if (c == '/')
		{
			// line comment
			skip_line(f);
			return;
		}
		if (c == '*')
		{
			// block comment
			while (1)
			{
				c = fetch_byte(f);
				if (c == CPP_EOF)
					return;
				if (c == '*')
				{
					c = fetch_byte(f);
					if (c == '/')
					{
						// end of comment - try again for directive
						goto again;
					}
					if (c == CPP_EOF)
						return;
				}
			}
		}
	}
	
	// empty directive - do nothing
	if (c == CPP_EOL)
		return;
	
	if (c < 'a' || c > 'z')
		goto out;
	
	i = 0;
	do
	{
		kw[i++] = c;
		if (i == sizeof(kw) - 1)
			goto out;	// keyword too long
		c = fetch_byte(f);
	} while ((c >= 'a' && c <= 'z') || (c == '_'));
	kw[i++] = '\0';
	
	/* we have a keyword here */
	for (i = 0; directives[i].name; i++)
	{
		if (strcmp(directives[i].name, kw) == 0)
		{
			(*directives[i].fn)(f);
			return;
		}
	}

/* if we fall through here, we have an unknown directive */
out:
	do_error("invalid preprocessor directive");
	skip_line(f);
}

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
void preprocess_file(struct file_stack_e *f)
{
	int c;
	int nlws = 1;
	
	while (1)
	{
		c = fetch_byte(f);
again:
		if (nlws == -1)
			nlws = 0;
		if (c == CPP_EOF)
		{
			outchr('\n');
			return;
		}
		if (c == CPP_EOL)
		{
			nlws = 1;
			outchr('\n');
			continue;
		}
		
		if (!is_whitespace(c))
			nlws = -1;

		if (is_sidchr(c))
		{
			// have identifier here - parse it off
			char *ident = NULL;
			int idlen = 0;
			
			do
			{
				ident = lw_realloc(ident, idlen + 1);
				ident[idlen++] = c;
				ident[idlen] = '\0';
				c = fetch_byte(f);
			} while (is_idchr(c));
			
			/* do something with the identifier here  - macros, etc. */
			outstr(ident);
			lw_free(ident);
			
			goto again;
		}
		
		switch (c)
		{
		default:
			outchr(c);
			break;

		case '.':	// a number - to prevent seeing an identifier in middle of number
			outchr(c);
			c = fetch_byte(f);
			if (!is_dec(c))
				goto again;
			/* fall through */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			do
			{
				outchr(c);
				c = fetch_byte(f);
				if (c == CPP_EOF)
					return;
				if (is_ep(c))
				{
					outchr(c);
					c = fetch_byte(f);
					if (c == '-' || c == '+')
					{
						outchr(c);
						c = fetch_byte(f);
					}
				}
			} while ((is_idchr(c)) || (c == '.'));
			goto again;

		case '#':
			if (nlws)
			{
				handle_directive(f);
				/* note: no need to reset nlws */
			}
			else
				outchr('#');
			break;
		
		case '\'':	// character constant
			outchr('\'');
			while ((c = fetch_byte(f)) != '\'')
			{
				if (c == '\\')
				{
					outchr('\\');
					c = fetch_byte(f);
				}
				if (c == CPP_EOL)
				{
					do_warning("Unterminated character constant");
					goto again;
				}
				if (c == CPP_EOF)
					return;
				outchr(c);
			}
			outchr(c);
			break;
			
		case '"':	// strings
			outchr(c);
			while ((c = fetch_byte(f)) != '"')
			{
				if (c == '\\')
				{
					outchr('\\');
					c = fetch_byte(f);
				}
				if (c == CPP_EOL)
				{
					do_warning("unterminated string literal");
					goto again;
				}
				if (c == CPP_EOF)
					return;
				outchr(c);
			}
			outchr(c);
			break;
			
		case '/':	// comments
			c = fetch_byte(f);
			if (c == '/')
			{
				// line comment
				outchr(' ');
				do
				{
					c = fetch_byte(f);
				} while (c != CPP_EOF && c != CPP_EOL);
			}
			else if (c == '*')
			{
				// block comment
				for (;;)
				{
					c = fetch_byte(f);
					if (c == CPP_EOF)
					{
						break;
					}
					if (c == CPP_EOL)
					{
						continue;
					}
					if (c == '*')
					{
						// maybe end of comment
						c = fetch_byte(f);
						if (c == '/')
						{
							// end of comment
							break;
						}
					}
				}
				// replace comment with a single space
				outchr(' ');
				if (nlws == -1)
					nlws = 1;
				continue;
			}
			else
			{
				// restore eaten '/'
				outchr('/');
				// process the character we just fetched
				goto again;
			}
		} // switch
	} // processing loop
}

int process_file(const char *f)
{
	struct file_stack_e *nf;
	FILE *fp;

	fprintf(stderr, "Processing %s\n", f);
	
	if (strcmp(f, "-") == 0)
		fp = stdin;
	else
		fp = fopen(f, "rb");
	if (fp == NULL)
	{
		do_warning("Cannot open %s: %s", f, strerror(errno));
		return -1;
	}

	/* push the file onto the file stack */	
	nf = lw_alloc(sizeof(struct file_stack_e));
	nf -> fn = f;
	nf -> fp = fp;
	nf -> next = file_stack;
	nf -> line = 1;
	nf -> col = 0;
	nf -> qseen = 0;
	nf -> ra = CPP_NOUNG;
	nf -> unget = CPP_NOUNG;
	file_stack = nf;

	/* go preprocess the file */
	preprocess_file(nf);
	
	if (nf -> fp != stdin)
		fclose(nf -> fp);
	file_stack = nf -> next;
	lw_free(nf);
	return 0;
}
