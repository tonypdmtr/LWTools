/*
emit.c

Copyright © 2011 William Astle

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
This is the actual compiler bit; it drives the parser and code generation
*/

#include <stdio.h>

#define __emit_c_seen__
#include "lwbasic.h"

void emit_prolog(cstate *state, int vis)
{
	if (vis)
	{
		printf("\texport _%s\n", state -> currentsub);
	}
	printf("_%s\n", state -> currentsub);
	if (state -> framesize > 0)
	{
		printf("\tleas %d,s\n", -(state -> framesize));
	}
}

void emit_epilog(cstate *state)
{
	if (state -> framesize > 0)
	{
		printf("\tleas %d,s\n", state -> framesize);
	}
	printf("\trts\n");
}
