/*
symtab.c

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
Symbol table handling
*/

#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#define __symtab_c_seen__
#include "symtab.h"

symtab_t *symtab_init(void)
{
	symtab_t *st;
	
	st = lw_alloc(sizeof(symtab_t));
	st -> head = NULL;
	return st;
}

void symtab_destroy(symtab_t *st)
{
	symtab_entry_t *se;
	
	while (st -> head)
	{
		se = st -> head;
		st -> head = se -> next;
		lw_free(se -> name);
		lw_free(se -> privdata);
		lw_free(se);
	}
	lw_free(st);
}

symtab_entry_t *symtab_find(symtab_t *st, char *name)
{
	symtab_entry_t *se;
	
	for (se = st -> head; se; se = se -> next)
	{
		if (strcmp(se -> name, name) == 0)
			return se;
	}
	return NULL;
}

void symtab_register(symtab_t *st, char *name, int addr, int symtype, void *privdata)
{
	symtab_entry_t *se;
	
	se = lw_alloc(sizeof(symtab_entry_t));
	se -> name = lw_strdup(name);
	se -> addr = addr;
	se -> symtype = symtype;
	se -> privdata = privdata;
	se -> next = st -> head;
	st -> head = se;
}
