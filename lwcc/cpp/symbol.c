/*
lwcc/cpp/symbol.c

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
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "cpp.h"

struct symtab_e *symtab_head = NULL;

struct symtab_e *symbol_find(const char *s)
{
	struct symtab_e *r;
	
	for (r = symtab_head; r; r = r -> next)
		if (strcmp(r -> name, s) == 0)
			return r;
	return NULL;
}

void symbol_free(struct symtab_e *r)
{
	lw_free(r -> name);
	lw_free(r -> strval);
	lw_free(r);
}

void symbol_undef(const char *s)
{
	struct symtab_e *r, **p;
	
	p = &symtab_head;
	for (r = symtab_head; r; r = r -> next)
	{
		if (strcmp(r -> name, s) == 0)
		{
			*p = r -> next;
			symbol_free(r);
			return;
		}
		p = &(r -> next);
	}
}

struct symtab_e *symbol_add(const char *s, const char *str, int nargs, int vargs)
{
	struct symtab_e *r;

	r = lw_alloc(sizeof (struct symtab_e));
	*r = (struct symtab_e){
		.name = lw_strdup(s),
		.strval = lw_strdup(str),
		.nargs = nargs,
		.vargs = vargs,
		.next = symtab_head };
	symtab_head = r;
	return r;
}
