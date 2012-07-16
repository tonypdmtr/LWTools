/*
unicorns.c

Copyright © 2012 William Astle

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
This adds output to lwasm that is suitable for IDEs and other tools
that are interesting in the doings of the assembler.
*/

#include <stdio.h>
#include <string.h>

#include "lwasm.h"
#include "lw_alloc.h"

static void print_urlencoding(FILE *stream, const char *string)
{
	for ( ; *string; string++)
	{
		if (*string < 33 || *string > 126 || strchr("$&+,/:;=?@\"<>#%{}|\\^~[]`", *string))
		{
			fprintf(stream, "%%%02X", *string);
		}
		else
		{
			fputc(*string, stream);
		}
	}
}

void lwasm_do_unicorns(asmstate_t *as)
{
	char *n;
	macrotab_t *me;
	structtab_t *se;
	int i;
			
	/* output file list */	
	while ((n = lw_stack_pop(as -> includelist)))
	{
		fputs("RESOURCE: type=file,filename=", stdout);
		print_urlencoding(stdout, n);
		fputc('\n', stdout);
		lw_free(n);
	}
	
	/* output macro list */
	for (me = as -> macros; me; me = me -> next)
	{
		fprintf(stdout, "RESOURCE: type=macro,name=%s,lineno=%d,filename=", me -> name, me -> definedat -> lineno);
		print_urlencoding(stdout, me -> definedat -> linespec);
		fputs(",flags=", stdout);
		if (me -> flags & macro_noexpand)
			fputs("noexpand", stdout);
		fputs(",def=", stdout);
		for (i = 0; i < me -> numlines; i++)
		{
			if (i)
				fputc(';', stdout);
			print_urlencoding(stdout, me -> lines[i]);
		}
		fputc('\n', stdout);
	}
	
	/* output structure list */
	for (se = as -> structs; se; se = se -> next)
	{
		fprintf(stdout, "RESOURCE: type=struct,name=%s,lineno=%d,filename=", se -> name, se -> definedat -> lineno);
		print_urlencoding(stdout, se -> definedat -> linespec);
		fputc('\n', stdout);
	}
	
}
