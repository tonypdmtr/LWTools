/*
struct.c
Copyright © 2010 William Astle

This file is part of LWASM.

LWASM is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.

Contains stuff associated with structure processing
*/

#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "lwasm.h"
#include "instab.h"

PARSEFUNC(pseudo_parse_struct)
{
	structtab_t *s;
	
	if (as -> instruct)
	{
		lwasm_register_error(as, l, "Attempt to define a structure inside a structure");
		return;
	}
	
	if (l -> sym == NULL)
	{
		lwasm_register_error(as, l, "Structure definition with no effect - no symbol");
		return;
	}
	
	for (s = as -> structs; s; s = s -> next)
	{
		if (!strcmp(s -> name, l -> sym))	
			break;
	}
	
	if (s)
	{
		lwasm_register_error(as, l, "Duplicate structure definition");
		return;
	}
	
	as -> instruct = 1;
	
	s = lw_alloc(sizeof(structtab_t));
	s -> name = lw_strdup(l -> sym);
	s -> next = as -> structs;
	s -> fields = NULL;
	s -> size = 0;
	as -> structs = s;
	as -> cstruct = s;
	
	skip_operand(p);
	
	l -> len = 0;
	l -> symset = 1;
}

void pseudo_endstruct_aux(asmstate_t *as, line_t *l, structtab_field_t *e, const char *prefix, int *coff)
{
	char *symname = NULL;
	lw_expr_t te1, te2;
	int len;
	
	while (e)
	{
		if (e -> name)
		{
			len = strlen(prefix) + strlen(e -> name) + 1;
			symname = lw_alloc(len + 1);
			sprintf(symname, "%s.%s", prefix, e -> name);
		}
		else
		{
			len = strlen(prefix) + 30;
			symname = lw_alloc(len + 1);
			sprintf(symname, "%s.____%d", prefix, *coff);
		}
		
		// register the symbol
		te1 = lw_expr_build(lw_expr_type_int, *coff);
		te2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, te1, l -> addr);
		register_symbol(as, l, symname, te2, symbol_flag_nocheck);
		lw_expr_destroy(te2);
		lw_expr_destroy(te1);
		
		if (e -> substruct)
		{
			char *t;
			len = strlen(symname) + 8;
			t = lw_alloc(len + 1);
			sprintf(t, "sizeof{%s}", symname);
			te1 = lw_expr_build(lw_expr_type_int, e -> substruct -> size);
			register_symbol(as, l, t, te1, symbol_flag_nocheck);
			lw_expr_destroy(te1);
			lw_free(t);
			pseudo_endstruct_aux(as, l, e -> substruct -> fields, symname, coff);
		}
		else
		{
			*coff += e -> size;
		}
		e = e -> next;
	}
}


PARSEFUNC(pseudo_parse_endstruct)
{
	char *t;
	int coff = 0;
	lw_expr_t te;
	int len;
		
	if (as -> instruct == 0)
	{
		lwasm_register_warning(as, l, "endstruct without struct");
		skip_operand(p);
		return;
	}

	len = strlen(as -> cstruct -> name) + 8;
	t = lw_alloc(len + 1);
	sprintf(t, "sizeof{%s}", as -> cstruct -> name);
	te = lw_expr_build(lw_expr_type_int, as -> cstruct -> size);
	register_symbol(as, l, t, te, symbol_flag_nocheck);
	lw_expr_destroy(te);
	lw_free(t);
	
	l -> soff = as -> cstruct -> size;
	as -> instruct = 0;
	
	skip_operand(p);
	
	pseudo_endstruct_aux(as, l, as -> cstruct -> fields, as -> cstruct -> name, &coff);
	
	l -> len = 0;
}

void register_struct_entry(asmstate_t *as, line_t *l, int size, structtab_t *ss)
{
	structtab_field_t *e, *e2;
	
	l -> soff = as -> cstruct -> size;
	e = lw_alloc(sizeof(structtab_field_t));
	e -> next = NULL;
	e -> size = size;
	if (l -> sym)
		e -> name = lw_strdup(l -> sym);
	else
		e -> name = NULL;
	e -> substruct = ss;
	if (as -> cstruct -> fields)
	{
		for (e2 = as -> cstruct -> fields; e2 -> next; e2 = e2 -> next)
			/* do nothing */ ;
		e2 -> next = e;
	}
	else
	{
		as -> cstruct -> fields = e;
	}
	as -> cstruct -> size += size;
}

int expand_struct(asmstate_t *as, line_t *l, char **p, char *opc)
{
	structtab_t *s;
	char *t;
	lw_expr_t te;
	int addr = 0;
	int len;
	
	debug_message(as, 200, "Checking for structure expansion: %s", opc);

	for (s = as -> structs; s; s = s -> next)
	{
		if (!strcmp(opc, s -> name))
			break;
	}
	
	if (!s)
		return -1;
	
	debug_message(as, 10, "Expanding structure: %s", opc);
	
	if (!(l -> sym))
	{
		lwasm_register_error(as, l, "Cannot declare a structure without a symbol name.");
		return -1;
	}

	if (as -> instruct)
	{
		lwasm_register_error(as, l, "Nested structures not currently supported");
		return -1;
	}
	
	l -> len = s -> size;

	if (as -> instruct)
	{
//		len = strlen(as -> cstruct -> name) + strlen(l -> sym) + 9;
//		t = lw_alloc(len + 1);
//		sprintf(t, "sizeof{%s.%s}", as -> cstruct -> name, l -> sym);
	}
	else
	{
		len = strlen(l -> sym) + 8;
		t = lw_alloc(len + 1);
		sprintf(t, "sizeof{%s}", l -> sym);
		te = lw_expr_build(lw_expr_type_int, s -> size);
		register_symbol(as, l, t, te, symbol_flag_nocheck);
		lw_expr_destroy(te);
		lw_free(t);
	}
	
	if (as -> instruct)
	{
//		len = strlen(as -> cstruct -> name) + strlen(l -> sym) + 1;
//		t = lw_alloc(len + 1);
//		sprintf(t, "%s.%s", as -> cstruct -> name, l -> sym);
	}
	else
	{
		t = lw_strdup(l -> sym);
		pseudo_endstruct_aux(as, l, s -> fields, t, &addr);
		lw_free(t);
	}

	if (as -> instruct)
		l -> symset = 1;
	if (as -> instruct)
		register_struct_entry(as, l, s -> size, s);
	return 0;
}

