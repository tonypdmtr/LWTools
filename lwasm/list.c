/*
list.c

Copyright © 2010 William Astle

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
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwasm.h"
#include "instab.h"

void list_symbols(asmstate_t *as, FILE *of);

/*
Do listing
*/
void do_list(asmstate_t *as)
{
	line_t *cl;
	FILE *of;
	int i;

	char *tc;
		
	if (!(as -> flags & FLAG_LIST))
		return;

	if (as -> list_file)
		of = fopen(as -> list_file, "w");
	else
		of = stdout;
	if (!of)
	{
		fprintf(stderr, "Cannot open list file; list not generated\n");
		return;
	}
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		if (cl -> len < 1)
		{
			if (cl -> soff >= 0)
			{
				fprintf(of, "%04X                  ", cl -> soff & 0xffff);
			}
			else if (cl -> dshow >= 0)
			{
				if (cl -> dsize == 1)
				{
					fprintf(of, "     %02X               ", cl -> dshow & 0xff);
				}
				else
				{
					fprintf(of, "     %04X               ", cl -> dshow & 0xff);
				}
			}
			else if (cl -> dptr)
			{
				lw_expr_t te;
				te = lw_expr_copy(cl -> dptr -> value);
				as -> exportcheck = 1;
				as -> csect = cl -> csect;
				lwasm_reduce_expr(as, te);
				as -> exportcheck = 0;
				if (lw_expr_istype(te, lw_expr_type_int))
				{
					fprintf(of, "     %04X             ", lw_expr_intval(te) & 0xffff);
				}
				else
				{
					fprintf(of, "     ????             ");
				}
				lw_expr_destroy(te);
			}
			else
			{
				fprintf(of, "                      ");
			}
		}
		else
		{
			lw_expr_t te;
			te = lw_expr_copy(cl -> addr);
			as -> exportcheck = 1;
			as -> csect = cl -> csect;
			lwasm_reduce_expr(as, te);
			as -> exportcheck = 0;
//			fprintf(of, "%s\n", lw_expr_print(te));
			fprintf(of, "%04X ", lw_expr_intval(te) & 0xffff);
			lw_expr_destroy(te);
			for (i = 0; i < cl -> outputl && i < 8; i++)
			{
				fprintf(of, "%02X", cl -> output[i]);
			}
			for (; i < 8; i++)
			{
				fprintf(of, "  ");
			}
			fprintf(of, " ");
		}
		/* the 32.32 below is deliberately chosen so that the start of the line text is at
		   a multiple of 8 from the start of the list line */
		fprintf(of, "(%32.32s):%05d ", cl -> linespec, cl -> lineno);
		i = 0;
		for (tc = cl -> ltext; *tc; tc++)
		{
			if ((*tc) == '\t')
			{
				if (i % 8 == 0)
				{
					i += 8;
					fprintf(of, "        ");
				}
				else
				{
					while (i % 8)
					{
						fputc(' ', of);
						i++;
					}
				}
			}
			else
			{
				fputc(*tc, of);
				i++;
			}
		}
		fputc('\n', of);
		if (cl -> outputl > 8)
		{
			for (i = 8; i < cl -> outputl; i++)
			{
				if (i % 8 == 0)
				{
					if (i != 8)
						fprintf(of, "\n     ");
					else
						fprintf(of, "     ");
				}
				fprintf(of, "%02X", cl -> output[i]);
			}
			if (i > 8)
				fprintf(of, "\n");
		}
	}
	
	if (as -> flags & FLAG_SYMBOLS)
		list_symbols(as, of);
}
