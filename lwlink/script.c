/*
script.c
Copyright © 2009 William Astle

This file is part of LWLINK.

LWLINK is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.


Read and parse linker scripts
*/

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwlink.h"

// the built-in DECB target linker script
static char *decb_script =
	"section init load 2000\n"
	"section code\n"
	"section *,!bss\n"
	"section *,bss\n"
	"entry 2000\n"
	;

// the built-in RAW target linker script
static char *raw_script = 
	"section init load 0000\n"
	"section code\n"
	"section *,!bss\n"
	"section *,bss\n"
	;

static char *lwex0_script =
	"section init load 0100\n"
	"section .text\n"
	"section .data\n"
	"section .ctors_start\n"
	"section .ctors\n"
	"section .ctors_end\n"
	"section .dtors_start\n"
	"section .dtors\n"
	"section .dtors_end\n"
	"section *,!bss\n"
	"section *,bss\n"
	"entry __start\n"
	"stacksize 0100\n"		// default 256 byte stack
	;

// the "simple" script
static char *simple_script = 
	"section *,!bss\n"
	"section *,bss\n"
	;

linkscript_t linkscript = { 0, NULL, -1 };

void setup_script()
{
	char *script, *oscript;
	long size;

	// read the file if needed
	if (scriptfile)
	{
		FILE *f;
		long bread;
		
		f = fopen(scriptfile, "rb");
		if (!f)
		{
			fprintf(stderr, "Can't open file %s:", scriptfile);
			perror("");
			exit(1);
		}
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		rewind(f);
		
		script = lw_alloc(size + 2);
		
		bread = fread(script, 1, size, f);
		if (bread < size)
		{
			fprintf(stderr, "Short read on file %s (%ld/%ld):", scriptfile, bread, size);
			perror("");
			exit(1);
		}
		fclose(f);
		
		script[size] = '\n';
		script[size + 1] = '\0';
	}
	else
	{
		// fetch defaults based on output mode
		switch (outformat)
		{
		case OUTPUT_RAW:
			script = raw_script;
			break;
		
		case OUTPUT_DECB:
			script = decb_script;
			break;
		
		case OUTPUT_LWEX0:
			script = lwex0_script;
			break;
			
		default:
			script = simple_script;
			break;
		}
		
		size = strlen(script);
		if (nscriptls)
		{
			char *rscript;
			int i;
			// prepend the "extra" script lines
			for (i = 0; i < nscriptls; i++)
				size += strlen(scriptls[i]) + 1;
			
			rscript = lw_alloc(size + 1);
			oscript = rscript;
			for (i = 0; i < nscriptls; i++)
			{
				oscript += sprintf(oscript, "%s\n", scriptls[i]);
			}
			strcpy(oscript, script);
			script = rscript;
		}
	}

	if (outformat == OUTPUT_LWEX0)
		linkscript.stacksize = 0x100;

	oscript = script;
	// now parse the script file
	while (*script)
	{
		char *ptr, *ptr2, *line;

		for (ptr = script; *ptr && *ptr != '\n' && *ptr != '\r'; ptr++)
			/* do nothing */ ;
		
		line = lw_alloc(ptr - script + 1);
		memcpy(line, script, ptr - script);
		line[ptr - script] = '\0';

		// skip line terms
		for (script = ptr + 1; *script == '\n' || *script == '\r'; script++)
			/* do nothing */ ;
		
		// ignore leading whitespace
		for (ptr = line; *ptr && isspace(*ptr); ptr++)
			/* do nothing */ ;
		
		// ignore blank lines
		if (!*ptr)
			continue;
		
		for (ptr = line; *ptr && !isspace(*ptr); ptr++)
			/* do nothing */ ;
		
		// now ptr points to the char past the first word
		// NUL it out
		if (*ptr)
			*ptr++ = '\0';
		
		// skip spaces after the first word
		for ( ; *ptr && isspace(*ptr); ptr++)
			/* do nothing */ ;
		
		if (!strcmp(line, "pad"))
		{
			// padding
			// parse the hex number and stow it
			linkscript.padsize = strtol(ptr, NULL, 16);
			if (linkscript.padsize < 0)
				linkscript.padsize = 0;
		}
		else if (!strcmp(line, "stacksize"))
		{
			// stack size for targets that support it
			// parse the hex number and stow it
			linkscript.padsize = strtol(ptr, NULL, 16);
			if (linkscript.stacksize < 0)
				linkscript.stacksize = 0x100;
		}
		else if (!strcmp(line, "entry"))
		{
			int eaddr;
			
			eaddr = strtol(ptr, &ptr2, 16);
			if (*ptr2)
			{
				linkscript.execaddr = -1;
				linkscript.execsym = lw_strdup(ptr);
			}
			else
			{
				linkscript.execaddr = eaddr;
				linkscript.execsym = NULL;
			}
		}
		else if (!strcmp(line, "section"))
		{
			// section
			// parse out the section name and flags
			for (ptr2 = ptr; *ptr2 && !isspace(*ptr2); ptr2++)
				/* do nothing */ ;
			
			if (*ptr2)
				*ptr2++ = '\0';
			
			while (*ptr2 && isspace(*ptr2))
				ptr2++;
				
			// ptr now points to the section name and flags and ptr2
			// to the first non-space character following
			
			// then look for "load <addr>" clause
			if (*ptr2)
			{
				if (!strncmp(ptr2, "load", 4))
				{
					ptr2 += 4;
					while (*ptr2 && isspace(*ptr2))
						ptr2++;
					
				}
				else
				{
					fprintf(stderr, "%s: bad script\n", scriptfile);
					exit(1);
				}
			}
			
			// now ptr2 points to the load address if there is one
			// or NUL if not
			linkscript.lines = lw_realloc(linkscript.lines, sizeof(struct scriptline_s) * (linkscript.nlines + 1));

			linkscript.lines[linkscript.nlines].noflags = 0;
			linkscript.lines[linkscript.nlines].yesflags = 0;
			if (*ptr2)
				linkscript.lines[linkscript.nlines].loadat = strtol(ptr2, NULL, 16);
			else
				linkscript.lines[linkscript.nlines].loadat = -1;
			for (ptr2 = ptr; *ptr2 && *ptr2 != ','; ptr2++)
				/* do nothing */ ;
			if (*ptr2)
			{
				*ptr2++ = '\0';
				if (!strcmp(ptr2, "!bss"))
				{
					linkscript.lines[linkscript.nlines].noflags = SECTION_BSS;
				}
				else if (!strcmp(ptr2, "bss"))
				{
					linkscript.lines[linkscript.nlines].yesflags = SECTION_BSS;
				}
				else
				{
					fprintf(stderr, "%s: bad script\n", scriptfile);
					exit(1);
				}
			}
			if (ptr[0] == '*' && ptr[1] == '\0')
				linkscript.lines[linkscript.nlines].sectname = NULL;
			else
				linkscript.lines[linkscript.nlines].sectname = lw_strdup(ptr);
			linkscript.nlines++;
		}
		else
		{
			fprintf(stderr, "%s: bad script line: %s\n", scriptfile, line);
			exit(1);
		}
		lw_free(line);
	}
	
	if (scriptfile || nscriptls)
		lw_free(oscript);
}
