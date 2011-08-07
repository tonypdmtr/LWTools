/*
symbol.c

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
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_expr.h>
#include <lw_string.h>

#include "lwasm.h"

struct symtabe *symbol_findprev(asmstate_t *as, struct symtabe *se)
{
	struct symtabe *se1, *se2;
	int i;
	
	for (se2 = NULL, se1 = as -> symtab.head; se1; se1 = se1 -> next)
	{
		debug_message(as, 200, "Sorting; looking at symbol %s (%p) for %s", se1 -> symbol, se1, se -> symbol);
		/* compare se with se1 */
		i = strcasecmp(se -> symbol, se1 -> symbol);
		
		/* if the symbol sorts before se1, we just need to return */
		if (i < 0)
			return se2;
		
		if (i == 0)
		{
			/* symbol name matches; compare other things */
			
			/*if next version is greater than this one, return */
			if (se -> version > se1 -> version)
				return se2;
			/* if next context is great than this one, return */ 
			if (se -> context > se1 -> context)
				return se2;
			
			/* if section name is greater, return */
			/* if se has no section but se1 does, we go first */
			if (se -> section == NULL && se1 -> section != NULL)
				return se2;
			if (se -> section != NULL && se1 -> section != NULL)
			{
				/* compare section names and if se < se1, return */
				i = strcasecmp(se -> section -> name, se1 -> section -> name);
				if (i < 0)
					return se2;
			}
		}
		
		se2 = se1;
	}
	return se2;
}

struct symtabe *register_symbol(asmstate_t *as, line_t *cl, char *sym, lw_expr_t val, int flags)
{
	struct symtabe *se;
	struct symtabe *sprev;
	int islocal = 0;
	int context = -1;
	int version = -1;
	char *cp;

	debug_message(as, 200, "Register symbol %s (%02X), %s", sym, flags, lw_expr_print(val));

	if (!(flags & symbol_flag_nocheck))
	{
		if (!sym || !*sym)
		{
			lwasm_register_error(as, cl, "Bad symbol (%s)", sym);
			return NULL;
		}
		if (*sym < 0x80 && (!strchr(SSYMCHARS, *sym) && !strchr(sym + 1, '$') && !strchr(sym + 1, '@') && !strchr(sym + 1, '?')))
		{
			lwasm_register_error(as, cl, "Bad symbol (%s)", sym);
			return NULL;
		}

		if ((*sym == '$' || *sym == '@') && (sym[1] >= '0' && sym[1] <= '9'))
		{
			lwasm_register_error(as, cl, "Bad symbol (%s)", sym);
			return NULL;
		}
	}

	for (cp = sym; *cp; cp++)
	{
		if (*cp == '@' || *cp == '?')
			islocal = 1;
		if (*cp == '$' && !(CURPRAGMA(cl, PRAGMA_DOLLARNOTLOCAL)))
			islocal = 1;
		
		// bad symbol
		if (!(flags & symbol_flag_nocheck) && *cp < 0x80 && !strchr(SYMCHARS, *cp))
		{
			lwasm_register_error(as, cl, "Bad symbol (%s)", sym);
			return NULL;
		}
	}

	if (islocal)
		context = cl -> context;
	
	// first, look up symbol to see if it is already defined
	for (se = as -> symtab.head; se; se = se -> next)
	{
		debug_message(as, 300, "Symbol add lookup: %p, %p", se, se -> next);
		if (!strcmp(sym, se -> symbol))
		{
			if (se -> context != context)
				continue;
			if ((flags & symbol_flag_set) && (se -> flags & symbol_flag_set))
			{
				if (version < se -> version)
					version = se -> version;
					continue;
			}
			break;
		}
	}

	if (se)
	{
		// multiply defined symbol
		lwasm_register_error(as, cl, "Multiply defined symbol (%s)", sym);
		return NULL;
	}

	if (flags & symbol_flag_set)
	{
		version++;
	}
	
	// symplify the symbol expression - replaces "SET" symbols with
	// symbol table entries
	lwasm_reduce_expr(as, val);

	se = lw_alloc(sizeof(struct symtabe));
	se -> context = context;
	se -> version = version;
	se -> flags = flags;
	if (CURPRAGMA(cl, PRAGMA_NOLIST))
	{
		se -> flags |= symbol_flag_nolist;
	}
	se -> value = lw_expr_copy(val);
	se -> symbol = lw_strdup(sym);
	if (cl)
		se -> section = cl -> csect;
	else
		se -> section = NULL;
	sprev = symbol_findprev(as, se);
	if (!sprev)
	{
		debug_message(as, 200, "Adding symbol at head of symbol table");
		se -> next = as -> symtab.head;
		as -> symtab.head = se;
	}
	else
	{
		debug_message(as, 200, "Adding symbol in middle of symbol table");
		se -> next = sprev -> next;
		sprev -> next = se;
	}
	return se;
}

// for "SET" symbols, always returns the LAST definition of the
// symbol. This works because the lwasm_reduce_expr() call in 
// register_symbol will ensure there are no lingering "var" references
// to the set symbol anywhere in the symbol table; they will all be
// converted to direct references
// NOTE: this means that for a forward reference to a SET symbol,
// the LAST definition will be the one used.
// This arrangement also ensures that any reference to the symbol
// itself inside a "set" definition will refer to the previous version
// of the symbol.
struct symtabe * lookup_symbol(asmstate_t *as, line_t *cl, char *sym)
{
	int local = 0;
	struct symtabe *s, *s2;

	// check if this is a local symbol
	if (strchr(sym, '@') || strchr(sym, '?'))
		local = 1;
	
	if (cl && !CURPRAGMA(cl, PRAGMA_DOLLARNOTLOCAL) && strchr(sym, '$'))
		local = 1;
	if (!cl && !(as -> pragmas & PRAGMA_DOLLARNOTLOCAL) && strchr(sym, '$'))
		local = 1;
	
	// cannot look up local symbol in global context!!!!!
	if (!cl && local)
		return NULL;
	
	for (s = as -> symtab.head, s2 = NULL; s; s = s -> next)
	{
		if (!strcmp(sym, s -> symbol))
		{
			if (local && s -> context != cl -> context)
				continue;
			
			if (s -> flags & symbol_flag_set)
			{
				// look for highest version of symbol
				if (!s2 || (s -> version > s2 -> version))
					s2 = s;
				continue;
			}
			break;
		}
	}
	if (!s && s2)
		s = s2;
	
	return s;
}

struct listinfo
{
	sectiontab_t *sect;
	asmstate_t *as;
	int complex;
};

int list_symbols_test(lw_expr_t e, void *p)
{
	struct listinfo *li = p;
	
	if (li -> complex)
		return 0;
	
	if (lw_expr_istype(e, lw_expr_type_special))
	{
		if (lw_expr_specint(e) == lwasm_expr_secbase)
		{
			if (li -> sect)
			{
				li -> complex = 1;
			}
			else
			{
				li -> sect = lw_expr_specptr(e);
			}
		}
	}
	return 0;
}

void list_symbols(asmstate_t *as, FILE *of)
{
	struct symtabe *s;
	lw_expr_t te;
	struct listinfo li;

	li.as = as;
		
	fprintf(of, "\nSymbol Table:\n");
	
	for (s = as -> symtab.head; s; s = s -> next)
	{
		if (s -> flags & symbol_flag_nolist)
			continue;
		lwasm_reduce_expr(as, s -> value);
		fputc('[', of);
		if (s -> flags & symbol_flag_set)
			fputc('S', of);
		else
			fputc(' ', of);
		if (as -> output_format == OUTPUT_OBJ)
		{
			if (lw_expr_istype(s -> value, lw_expr_type_int))
				fputc('c', of);
			else
				fputc('s', of);
		}
		if (s -> context < 0)
			fputc('G', of);
		else
			fputc('L', of);

		fputc(']', of);
		fputc(' ', of);
		fprintf(of, "%-32s ", s -> symbol);
		
		te = lw_expr_copy(s -> value);
		li.complex = 0;
		li.sect = NULL;
		lw_expr_testterms(te, list_symbols_test, &li);
		if (li.sect)
		{
			as -> exportcheck = 1;
			as -> csect = li.sect;
			lwasm_reduce_expr(as, te);
			as -> exportcheck = 0;
		}
		
		if (lw_expr_istype(te, lw_expr_type_int))
		{
			fprintf(of, "%04X", lw_expr_intval(te));
			if (li.sect)
			{
				fprintf(of, " (%s)", li.sect -> name);
			}
			fprintf(of, "\n");
		}
		else
		{
			fprintf(of, "<<incomplete>>\n");
//			fprintf(of, "%s\n", lw_expr_print(s -> value));
		}
		lw_expr_destroy(te);
	}
}
