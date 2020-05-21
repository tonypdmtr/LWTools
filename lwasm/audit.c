/*
audit.c

Copyright © 2020 William Astle

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
#include <lw_expr.h>
#include <lw_string.h>

#include "lwasm.h"
#include "instab.h"

void do_audit(asmstate_t* as)
{
	line_t* cl, * nl;
	FILE* of = NULL;

	if (!(as->flags & FLAG_AUDIT))
		return;

	if (as->audit_file)
	{
		if (strcmp(as->audit_file, "-") == 0)
		{
			of = stdout;
		}
		else
		{
			of = fopen(as->audit_file, "w");
		}
	}
	else
	{
		of = stdout;
	}

	if (!of)
	{
		fprintf(stderr, "Cannot open audit file; feature list not generated\n");
		return;
	}

	for (cl = as->line_head; cl; cl = nl)
	{
		nl = cl->next;

		int flags = instab[cl->insn].flags;
		if (flags & lwasm_insn_is6309)
		{
			if (flags & lwasm_insn_used) continue;
			instab[cl->insn].flags |= lwasm_insn_used;
			fprintf(of, "%s\n", instab[cl->insn].opcode);
		}
	}
}
