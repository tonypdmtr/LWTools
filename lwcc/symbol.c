/*
lwcc/symbol.c

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

#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "cpp.h"
#include "symbol.h"
#include "token.h"

void symbol_free(struct symtab_e *s)
{
	int i;
	struct token *t;
	
	lw_free(s -> name);
	
	for (i = 0; i < s -> nargs; i++)
		lw_free(s -> params[i]);
	lw_free(s -> params);
	while (s -> tl)
	{
		t = s -> tl;
		s -> tl = t -> next;
		token_free(t);
	}
}

struct symtab_e *symtab_find(struct preproc_info *pp, char *name)
{
	struct symtab_e *s;
	
	for (s = pp -> sh; s; s = s -> next)
	{
		if (strcmp(s -> name, name) == 0)
		{
			return s;
		}
	}
	return NULL;
}

void symtab_undef(struct preproc_info *pp, char *name)
{
	struct symtab_e *s, **p;
	
	p = &(pp -> sh);
	for (s = pp -> sh; s; s = s -> next)
	{
		if (strcmp(s -> name, name) == 0)
		{
			(*p) -> next = s -> next;
			symbol_free(s);
			return;
		}
		p = &((*p) -> next);
	}
}

void symtab_define(struct preproc_info *pp, char *name, struct token *def, int nargs, char **params, int vargs)
{
	struct symtab_e *s;
	
	s = lw_alloc(sizeof(struct symtab_e));
	s -> name = lw_strdup(name);
	s -> tl = def;
	s -> nargs = nargs;
	s -> params = params;
	s -> vargs = vargs;
	s -> next = pp -> sh;
	pp -> sh = s;
}
