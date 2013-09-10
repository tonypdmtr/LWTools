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

*/

#include <errno.h>
#include <stdio.h>
#include <string.h> 

#include <lw_alloc.h>

#include "cpp.h"

struct file_stack_e *file_stack = NULL;

/* output a byte to the current output stream as long as we aren't in the
   middle of a false conditional. CPP_EOL will be converted to '\n'
   on output. */
void outchr(int c)
{
	if (skip_level)
		return;
	if (c == CPP_EOL)
		c = '\n';
	fputc(c, output_fp);
}

/* output a string to the current output stream as long as we aren't in the
   middle of a false conditional */
void outstr(char *s)
{
	if (skip_level)
		return;
	while (*s)
		outchr(*s++);
}

/* fetch a raw input byte from the current file. Will return CPP_EOF if
   EOF is encountered and CPP_EOL if an end of line sequence is encountered.
   End of line is defined as either CR, CRLF, LF, or LFCR. CPP_EOL is
   returned on the first CR or LF encountered. The complementary CR or LF
   is munched, if present, when the *next* character is read. This always
   operates on file_stack.

   This function also accounts for line numbers in input files and also
   character columns.
*/
int fetch_byte_ll(void)
{
	int c;

	if (file_stack -> eolstate != 0)	
	{
		file_stack -> line++;
		file_stack -> col = 0;
	}
	c = getc(file_stack -> fp);
	file_stack -> col++;
	if (file_stack -> eolstate == 1)
	{
		// just saw CR, munch LF
		if (c == 10)
			c = getc(file_stack -> fp);
		file_stack -> eolstate = 0;
	}
	else if (file_stack -> eolstate == 2)
	{
		// just saw LF, much CR
		if (c == 13)
			c = getc(file_stack -> fp);
		file_stack -> eolstate = 0;
	}
	
	if (c == 10)
	{
		// we have LF - end of line, flag to munch CR
		file_stack -> eolstate = 2;
		c = CPP_EOL;
	}
	else if (c == 13)
	{
		// we have CR - end of line, flag to munch LF
		file_stack -> eolstate = 1;
		c = CPP_EOL;
	}
	else if (c == EOF)
	{
		c = CPP_EOF;
	}
	return c;
}

/* This function takes a sequence of bytes from the _ll function above
   and does trigraph interpretation on it, but only if the global
   trigraphs is nonzero. */
int fetch_byte_tg(void)
{
	int c;
	
	if (!trigraphs)
	{
		c = fetch_byte_ll();
	}
	else
	{
		/* we have to do the trigraph shit here */
		if (file_stack -> ra != CPP_NOUNG)
		{
			if (file_stack -> qseen > 0)
			{
				c = '?';
				file_stack -> qseen -= 1;
				return c;
			}
			else
			{
				c = file_stack -> ra;
				file_stack -> ra = CPP_NOUNG;
				return c;
			}
		}
	
		c = fetch_byte_ll();
		while (c == '?')
		{
			file_stack -> qseen++;
			c = fetch_byte_ll();
		}
	
		if (file_stack -> qseen >= 2)
		{
			// we have a trigraph
			switch (c)
			{
			case '=':
				c = '#';
				file_stack -> qseen -= 2;
				break;
			
			case '/':
				c = '\\';
				file_stack -> qseen -= 2;
				break;
		
			case '\'':
				c = '^';
				file_stack -> qseen -= 2;
				break;
		
			case '(':
				c = '[';
				file_stack -> qseen -= 2;
				break;
		
			case ')':
				c = ']';
				file_stack -> qseen -= 2;
				break;
		
			case '!':
				c = '|';
				file_stack -> qseen -= 2;
				break;
		
			case '<':
				c = '{';
				file_stack -> qseen -= 2;
				break;
		
			case '>':
				c = '}';
				file_stack -> qseen -= 2;
				break;
		
			case '~':
				c = '~';
				file_stack -> qseen -= 2;
				break;
			}
			if (file_stack -> qseen > 0)
			{
				file_stack -> ra = c;
				c = '?';
				file_stack -> qseen--;
			}
		}
		else if (file_stack -> qseen > 0)
		{
			file_stack -> ra = c;
			c = '?';
			file_stack -> qseen--;
		}
	}
	return c;
}

/* This function puts a byte back onto the front of the input stream used
   by fetch_byte(). Theoretically, an unlimited number of characters can
   be unfetched. Line and column counting may be incorrect if unfetched
   characters cross a token boundary. */
void unfetch_byte(int c)
{
	if (file_stack -> ungetbufl >= file_stack -> ungetbufs)
	{
		file_stack -> ungetbufs += 100;
		file_stack -> ungetbuf = lw_realloc(file_stack -> ungetbuf, file_stack -> ungetbufs);
	}
	file_stack -> ungetbuf[file_stack -> ungetbufl++] = c;
}

/* This function retrieves a byte from the input stream. It performs
   backslash-newline splicing on the returned bytes. Any character
   retrieved from the unfetch buffer is presumed to have already passed
   the backslash-newline filter. */
int fetch_byte(void)
{
	int c;

	if (file_stack -> ungetbufl > 0)
	{
		file_stack -> ungetbufl--;
		c = file_stack -> ungetbuf[file_stack -> ungetbufl];
		if (file_stack -> ungetbufl == 0)
		{
			lw_free(file_stack -> ungetbuf);
			file_stack -> ungetbuf = NULL;
			file_stack -> ungetbufs = 0;
		}
		return c;
	}
	
again:
	if (file_stack -> unget != CPP_NOUNG)
	{
		c = file_stack -> unget;
		file_stack -> unget = CPP_NOUNG;
	}
	else
	{
		c = fetch_byte_tg();
	}
	if (c == '\\')
	{
		int c2;
		c2 = fetch_byte_tg();
		if (c2 == CPP_EOL)
			goto again;
		else
			file_stack -> unget = c2;
	}
	file_stack -> curc = c;
	return c;
}

/* This function opens (if not stdin) the file f and pushes it onto the
   top of the input file stack. It then proceeds to process the file
   and return. Nonzero return means the file could not be opened. */
int process_file(const char *f)
{
	struct file_stack_e nf;
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
	nf.fn = f;
	nf.fp = fp;
	nf.next = file_stack;
	nf.line = 1;
	nf.col = 0;
	nf.qseen = 0;
	nf.ra = CPP_NOUNG;
	nf.unget = CPP_NOUNG;
	file_stack = &nf;
	nf.ungetbuf = NULL;
	nf.ungetbufs = 0;
	nf.ungetbufl = 0;
	
	/* go preprocess the file */
	preprocess_file();
	
	if (nf.fp != stdin)
		fclose(nf.fp);
	file_stack = nf.next;
	return 0;
}
