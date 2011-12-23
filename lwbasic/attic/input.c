/*
input.c

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
handle reading input for the rest of the system
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_error.h>

#define __input_c_seen__
#include "lwbasic.h"

struct input_state
{
	FILE *fp;
	int error;
};

static void input_init(cstate *state)
{
	struct input_state *sp;
	
	sp = lw_alloc(sizeof(struct input_state));
	sp -> error = 0;
	
	if (!(state -> input_file) || strcmp(state -> input_file, "-"))
	{
		sp -> fp = stdin;
	}
	else
	{
		sp -> fp = fopen(state -> input_file, "rb");
		if (!(sp -> fp))
		{
			lwb_error("Cannot open input file\n");
		}
	}
	
	state -> input_state = sp;
}

int input_getchar(cstate *state)
{
	int r;
	struct input_state *sp;
	
	if (!(state -> input_state))
		input_init(state);
	sp = state -> input_state;
	

	if (sp -> error)
		return -2;
	
	if (feof(sp -> fp))
		return -1;
	
	r = fgetc(sp -> fp);
	if (r == EOF)
		return -1;
	return r;	
}
