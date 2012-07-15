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

void lwasm_do_unicorns(asmstate_t *as)
{
	char *n;
	macrotab_t *me;
	
	/* output file list */	
	while ((n = lw_stack_pop(as -> includelist)))
	{
		fprintf(stdout, "RESOURCE: file:%s\n", n);
			lw_free(n);
	}
	
	/* output macro list */
	for (me = as -> macros; me; me = me -> next)
	{
		fprintf(stdout, "RESOURCE: macro:%s,%d,%s\n", me -> name, me -> definedat -> lineno, me -> definedat -> linespec);
	}
}
