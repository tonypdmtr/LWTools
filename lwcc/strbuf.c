/*
lwcc/strbuf.c

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

#include <stdlib.h>

#include <lw_alloc.h>

#include "strbuf.h"

struct strbuf *strbuf_new(void)
{
	struct strbuf *strbuf;
	
	strbuf = lw_alloc(sizeof(struct strbuf));
	strbuf -> str = NULL;
	strbuf -> bo = 0;
	strbuf -> bl = 0;
	return strbuf;
}

void strbuf_add(struct strbuf *strbuf, int c)
{
	if (strbuf -> bo >= strbuf -> bl)
	{
		strbuf -> bl += 100;
		strbuf -> str = lw_realloc(strbuf -> str, strbuf -> bl);
	}
	strbuf -> str[strbuf -> bo++] = c;
}

char *strbuf_end(struct strbuf *strbuf)
{
	char *rv;

	strbuf_add(strbuf, 0);
	rv = strbuf -> str;
	lw_free(strbuf);
	return rv;
}
