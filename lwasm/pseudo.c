/*
pseudo.c
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

*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <lw_alloc.h>

#include "lwasm.h"
#include "instab.h"
#include "input.h"

#include "lw_string.h"

extern void register_struct_entry(asmstate_t *as, line_t *l, int size, structtab_t *ss);

// for "end"
PARSEFUNC(pseudo_parse_end)
{
	lw_expr_t addr;
	
	as -> endseen = 1;
	l -> len = 0;
	
	if (as -> output_format != OUTPUT_DECB)
	{
		skip_operand(p);
		return;
	}
	
	if (!**p)
	{
		addr = lw_expr_build(lw_expr_type_int, 0);
	}
	else
	{
		addr = lwasm_parse_expr(as, p);
	}
	if (!addr)
	{
		lwasm_register_error(as, l, "Bad expression");
		addr = lw_expr_build(lw_expr_type_int, 0);
	}
	lwasm_save_expr(l, 0, addr);
}

EMITFUNC(pseudo_emit_end)
{
	lw_expr_t addr;
	
	addr = lwasm_fetch_expr(l, 0);
	
	if (addr)
	{
		if (!lw_expr_istype(addr, lw_expr_type_int))
			lwasm_register_error(as, l, "Exec address not constant!");
		else
			as -> execaddr = lw_expr_intval(addr);
	}
	as -> endseen = 1;
}

PARSEFUNC(pseudo_parse_fcb)
{
	int i = 0;
	lw_expr_t e;
	
	for (;;)
	{
		e = lwasm_parse_expr(as, p);
		if (!e)
		{
			lwasm_register_error(as, l, "Bad expression (#%s)", i);
			break;
		}
		lwasm_save_expr(l, i++, e);
		if (**p != ',')
			break;
		(*p)++;
	}
	
	l -> len = i;
}

EMITFUNC(pseudo_emit_fcb)
{
	int i;
	lw_expr_t e;
//	int v;
	
	for (i = 0; i < l -> len; i++)
	{
		e = lwasm_fetch_expr(l, i);
		lwasm_emitexpr(l, e, 1);
	}
}

PARSEFUNC(pseudo_parse_fdb)
{
	int i = 0;
	lw_expr_t e;
	
	for (;;)
	{
		e = lwasm_parse_expr(as, p);
		if (!e)
		{
			lwasm_register_error(as, l, "Bad expression (#%d)", i);
			break;
		}
		lwasm_save_expr(l, i++, e);
		if (**p != ',')
			break;
		(*p)++;
	}
	
	l -> len = i * 2;
}

EMITFUNC(pseudo_emit_fdb)
{
	int i;
	lw_expr_t e;
//	int v;
	
	for (i = 0; i < (l -> len)/2; i++)
	{
		e = lwasm_fetch_expr(l, i);
		lwasm_emitexpr(l, e, 2);
	}
}

PARSEFUNC(pseudo_parse_fqb)
{
	int i = 0;
	lw_expr_t e;
	
	for (;;)
	{
		e = lwasm_parse_expr(as, p);
		if (!e)
		{
			lwasm_register_error(as, l, "Bad expression (#%s)", i);
			break;
		}
		lwasm_save_expr(l, i++, e);
		if (**p != ',')
			break;
		(*p)++;
	}
	
	l -> len = i * 4;
}

EMITFUNC(pseudo_emit_fqb)
{
	int i;
	lw_expr_t e;
//	int v;
	
	for (i = 0; i < (l -> len)/4; i++)
	{
		e = lwasm_fetch_expr(l, i);
		lwasm_emitexpr(l, e, 4);
	}
}

PARSEFUNC(pseudo_parse_fcc)
{
	char delim;
	int i;
	
	if (!**p)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	delim = **p;
	(*p)++;
	
	for (i = 0; (*p)[i] && (*p)[i] != delim; i++)
		/* do nothing */ ;
	
	if ((*p)[i] != delim)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	l -> lstr = lw_strndup(*p, i);
	(*p) += i + 1;
	
	l -> len = i;
}

EMITFUNC(pseudo_emit_fcc)
{
	int i;
	
	for (i = 0; i < l -> len; i++)
		lwasm_emit(l, l -> lstr[i]);
}

PARSEFUNC(pseudo_parse_fcs)
{
	char delim;
	int i;
	
	if (!**p)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	delim = **p;
	(*p)++;
	
	for (i = 0; (*p)[i] && (*p)[i] != delim; i++)
		/* do nothing */ ;
	
	if ((*p)[i] != delim)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	l -> lstr = lw_strndup(*p, i);
	(*p) += i + 1;
	
	l -> len = i;
}

EMITFUNC(pseudo_emit_fcs)
{
	int i;
	
	for (i = 0; i < l -> len - 1; i++)
		lwasm_emit(l, l -> lstr[i]);
	lwasm_emit(l, l -> lstr[i] | 0x80);
}

PARSEFUNC(pseudo_parse_fcn)
{
	char delim;
	int i;
	
	if (!**p)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	delim = **p;
	(*p)++;
	
	for (i = 0; (*p)[i] && (*p)[i] != delim; i++)
		/* do nothing */ ;
	
	if ((*p)[i] != delim)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	l -> lstr = lw_strndup(*p, i);
	(*p) += i + 1;
	
	l -> len = i + 1;
}

EMITFUNC(pseudo_emit_fcn)
{
	int i;
	
	for (i = 0; i < (l -> len - 1); i++)
		lwasm_emit(l, l -> lstr[i]);
	lwasm_emit(l, 0);
}

PARSEFUNC(pseudo_parse_rmb)
{
	lw_expr_t expr;
	
	expr = lwasm_parse_expr(as, p);
	if (!expr)
	{
		lwasm_register_error(as, l, "Bad expression");
	}

	l -> lint = 0;
	if (as -> instruct)
	{
		lwasm_reduce_expr(as, expr);
		if (!lw_expr_istype(expr, lw_expr_type_int))
		{
			lwasm_register_error(as, l, "Expression must be constant at parse time");
		}
		else
		{
			int e;
			e = lw_expr_intval(expr);
			register_struct_entry(as, l, e, NULL);
			l -> len = 0;
			l -> lint = 1;
			l -> symset = 1;
		}
	}
	
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_rmb)
{
	lw_expr_t expr;
	
	if (l -> lint)
		return;
	
	if (l -> len >= 0)
		return;
	
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		l -> len = lw_expr_intval(expr);
	}
}

EMITFUNC(pseudo_emit_rmb)
{
	if (l -> lint)
		return;

	if (l -> len < 0)
		lwasm_register_error(as, l, "Expression not constant");
}

PARSEFUNC(pseudo_parse_rmd)
{
	lw_expr_t expr;
	
	l -> lint = 0;
	expr = lwasm_parse_expr(as, p);
	if (!expr)
	{
		lwasm_register_error(as, l, "Bad expression");
	}
	
	if (as -> instruct)
	{
		lwasm_reduce_expr(as, expr);
		if (!lw_expr_istype(expr, lw_expr_type_int))
		{
			lwasm_register_error(as, l, "Expression must be constant at parse time");
		}
		else
		{
			int e;
			e = lw_expr_intval(expr) * 2;
			register_struct_entry(as, l, e, NULL);
			l -> len = 0;
			l -> symset = 1;
			l -> lint = 1;
		}
	}
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_rmd)
{
	lw_expr_t expr;
	
	if (l -> lint)
		return;
	
	if (l -> len >= 0)
		return;
	
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		l -> len = lw_expr_intval(expr) * 2;
	}
}

EMITFUNC(pseudo_emit_rmd)
{
	if (l -> lint)
		return;

	if (l -> len < 0)
		lwasm_register_error(as, l, "Expression not constant");
}


PARSEFUNC(pseudo_parse_rmq)
{
	lw_expr_t expr;
	
	l -> lint = 0;
	expr = lwasm_parse_expr(as, p);
	if (!expr)
	{
		lwasm_register_error(as, l, "Bad expression");
	}
	if (as -> instruct)
	{
		lwasm_reduce_expr(as, expr);
		if (!lw_expr_istype(expr, lw_expr_type_int))
		{
			lwasm_register_error(as, l, "Expression must be constant at parse time");
		}
		else
		{
			int e;
			e = lw_expr_intval(expr) * 4;
			register_struct_entry(as, l, e, NULL);
			l -> len = 0;
			l -> symset = 1;
			l -> lint = 1;
		}
	}
	
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_rmq)
{
	lw_expr_t expr;

	if (l -> lint)
		return;
	
	if (l -> len >= 0)
		return;
	
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		l -> len = lw_expr_intval(expr) * 4;
	}
}

EMITFUNC(pseudo_emit_rmq)
{
	if (l -> lint)
		return;

	if (l -> len < 0)
		lwasm_register_error(as, l, "Expression not constant");
}


PARSEFUNC(pseudo_parse_zmq)
{
	lw_expr_t expr;
	
	expr = lwasm_parse_expr(as, p);
	if (!expr)
	{
		lwasm_register_error(as, l, "Bad expression");
	}
	
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_zmq)
{
	lw_expr_t expr;
	
	if (l -> len >= 0)
		return;
	
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		l -> len = lw_expr_intval(expr) * 4;
	}
}

EMITFUNC(pseudo_emit_zmq)
{
	int i;

	if (l -> len < 0)
	{
		lwasm_register_error(as, l, "Expression not constant");
		return;
	}

	for (i = 0; i < l -> len; i++)
		lwasm_emit(l, 0);	
}


PARSEFUNC(pseudo_parse_zmd)
{
	lw_expr_t expr;
	
	expr = lwasm_parse_expr(as, p);
	if (!expr)
	{
		lwasm_register_error(as, l, "Bad expression");
	}
	
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_zmd)
{
	lw_expr_t expr;
	
	if (l -> len >= 0)
		return;
	
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		l -> len = lw_expr_intval(expr) * 2;
	}
}

EMITFUNC(pseudo_emit_zmd)
{
	int i;

	if (l -> len < 0)
	{
		lwasm_register_error(as, l, "Expression not constant");
		return;
	}

	for (i = 0; i < l -> len; i++)
		lwasm_emit(l, 0);	
}

PARSEFUNC(pseudo_parse_zmb)
{
	lw_expr_t expr;
	
	expr = lwasm_parse_expr(as, p);
	if (!expr)
	{
		lwasm_register_error(as, l, "Bad expression");
	}
	
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_zmb)
{
	lw_expr_t expr;
	
	if (l -> len >= 0)
		return;
	
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		l -> len = lw_expr_intval(expr);
	}
}

EMITFUNC(pseudo_emit_zmb)
{
	int i;

	if (l -> len < 0)
	{
		lwasm_register_error(as, l, "Expression not constant");
		return;
	}

	for (i = 0; i < l -> len; i++)
		lwasm_emit(l, 0);	
}

PARSEFUNC(pseudo_parse_org)
{
	lw_expr_t e;
	
	l -> len = 0;
	
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	lw_expr_destroy(l -> addr);
	l -> addr = e;
	l -> len = 0;
}

PARSEFUNC(pseudo_parse_equ)
{
	lw_expr_t e;
	
	l -> len = 0;
	
	if (!(l -> sym))
	{
		lwasm_register_error(as, l, "Missing symbol");
		return;
	}
	
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	register_symbol(as, l, l -> sym, e, symbol_flag_none);
	l -> symset = 1;
	l -> dptr = lookup_symbol(as, l, l -> sym);
}

PARSEFUNC(pseudo_parse_set)
{
	lw_expr_t e;
	
	l -> len = 0;
	
	if (!(l -> sym))
	{
		lwasm_register_error(as, l, "Missing symbol");
		return;
	}
	
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	register_symbol(as, l, l -> sym, e, symbol_flag_set);
	l -> symset = 1;
	l -> dptr = lookup_symbol(as, l, l -> sym);
}

PARSEFUNC(pseudo_parse_setdp)
{
	lw_expr_t e;

	l -> len = 0;
	
	if (as -> output_format == OUTPUT_OBJ)
	{
		lwasm_register_error(as, l, "SETDP not permitted for object target");
		return;
	}
	
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	// try simplifying the expression and see if it turns into an int
	lwasm_reduce_expr(as, e);
	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		lwasm_register_error(as, l, "SETDP must be constant on pass 1");
		return;
	}
	l -> dpval = lw_expr_intval(e) & 0xff;
	l -> dshow = l -> dpval;
	l -> dsize = 1;
}

PARSEFUNC(pseudo_parse_ifp1)
{
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}
	
	lwasm_register_warning(as, l, "IFP1 if is not supported; ignoring");
	
}

PARSEFUNC(pseudo_parse_ifp2)
{
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}
	
	lwasm_register_warning(as, l, "IFP2 if is not supported; ignoring");
}

PARSEFUNC(pseudo_parse_ifeq)
{
	lw_expr_t e;
	
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	e = lwasm_parse_cond(as, p);
	lwasm_reduce_expr(as, e);
	if (e && lw_expr_intval(e) != 0)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}

PARSEFUNC(pseudo_parse_ifne)
{
	lw_expr_t e;
	
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	e = lwasm_parse_cond(as, p);
	lwasm_reduce_expr(as, e);
	if (e && lw_expr_intval(e) == 0)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}


PARSEFUNC(pseudo_parse_ifgt)
{
	lw_expr_t e;
	
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	e = lwasm_parse_cond(as, p);
	lwasm_reduce_expr(as, e);
	if (e && lw_expr_intval(e) <= 0)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}

PARSEFUNC(pseudo_parse_ifge)
{
	lw_expr_t e;
	
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	e = lwasm_parse_cond(as, p);
	lwasm_reduce_expr(as, e);
	if (e && lw_expr_intval(e) < 0)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}

PARSEFUNC(pseudo_parse_iflt)
{
	lw_expr_t e;
	
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	e = lwasm_parse_cond(as, p);
	lwasm_reduce_expr(as, e);
	if (e && lw_expr_intval(e) >= 0)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}

PARSEFUNC(pseudo_parse_ifle)
{
	lw_expr_t e;

	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	e = lwasm_parse_cond(as, p);
	lwasm_reduce_expr(as, e);
	if (e && lw_expr_intval(e) > 0)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}

PARSEFUNC(pseudo_parse_endc)
{
	l -> len = 0;
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount--;
		if (as -> skipcount <= 0)
			as -> skipcond = 0;
	}
}

PARSEFUNC(pseudo_parse_else)
{
	l -> len = 0;
	
	if (as -> skipmacro)
		return;
	
	if (as -> skipcond)
	{
		if (as -> skipcount == 1)
		{
			as -> skipcount = 0;
			as -> skipcond = 0;
		}
		return;
	}
	as -> skipcond = 1;
	as -> skipcount = 1;
}

PARSEFUNC(pseudo_parse_ifdef)
{
	char *sym;
	int i;
	struct symtabe *s;
	
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	for (i = 0; (*p)[i] && !isspace((*p)[i]); i++)
		/* do nothing */ ;
	
	sym = lw_strndup(*p, i);
	
	s = lookup_symbol(as, l, sym);
	
	lw_free(sym);
	
	if (!s)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}

PARSEFUNC(pseudo_parse_ifndef)
{
	char *sym;
	int i;
	struct symtabe *s;
	
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	for (i = 0; (*p)[i] && !isspace((*p)[i]); i++)
		/* do nothing */ ;
	
	sym = lw_strndup(*p, i);
	(*p) += i;
	
	s = lookup_symbol(as, l, sym);
	
	lw_free(sym);

	if (s)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}

PARSEFUNC(pseudo_parse_error)
{
	lwasm_register_error(as, l, "User error: %s", *p);
	skip_operand(p);
}

PARSEFUNC(pseudo_parse_warning)
{
	lwasm_register_warning(as, l, "User warning: %s", *p);
	skip_operand(p);
}

PARSEFUNC(pseudo_parse_includebin)
{
	char *fn, *p2;
	int delim = 0;
	FILE *fp;
	long flen;
	
	if (!**p)
	{
		lwasm_register_error(as, l, "Missing filename");
		return;
	}
	
	if (**p == '"' || **p == '\'')
	{
		delim = **p;
		(*p)++;

		for (p2 = *p; *p2 && *p2 != delim; p2++)
			/* do nothing */ ;
	}
	else
	{
		for (p2 = *p; *p2 && !isspace(*p2); p2++)
			/* do nothing */ ;
	}
	fn = lw_strndup(*p, p2 - *p);
	
	if (delim && **p)
		(*p)++;
	
	fp = input_open_standalone(as, fn);
	if (!fp)
	{
		lwasm_register_error(as, l, "Cannot open file");
		lw_free(fn);
		return;
	}
	
	l -> lstr = fn;
	
	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);
	fclose(fp);

	l -> len = flen;
}

EMITFUNC(pseudo_emit_includebin)
{
	FILE *fp;
	int c;
	
	fp = input_open_standalone(as, l -> lstr);
	if (!fp)
	{
		lwasm_register_error(as, l, "Cannot open file (emit)!");
		return;
	}
	
	for (;;)
	{
		c = fgetc(fp);
		if (c == EOF)
		{
			fclose(fp);
			return;
		}
		lwasm_emit(l, c);
	}
}

PARSEFUNC(pseudo_parse_include)
{
	char *fn, *p2;
	char *p3;
	int delim = 0;
	int len;
	char buf[110];
		
	if (!**p)
	{
		lwasm_register_error(as, l, "Missing filename");
		return;
	}
	
	if (**p == '"' || **p == '\'')
	{
		delim = **p;
		(*p)++;

		for (p2 = *p; *p2 && *p2 != delim; p2++)
			/* do nothing */ ;
	}
	else
	{
		for (p2 = *p; *p2 && !isspace(*p2); p2++)
			/* do nothing */ ;
	}
	fn = lw_strndup(*p, p2 - *p);
	(*p) = p2;
	if (delim && **p)
		(*p)++;

	/* add a book-keeping entry for line numbers */
	snprintf(buf, 100, "\001\001SETLINENO %d\n", l -> lineno + 1);
	input_openstring(as, "INTERNAL", buf);
	
	len = strlen(fn) + 8;
	p3 = lw_alloc(len + 1);
	sprintf(p3, "include:%s", fn);
	input_open(as, p3);
	lw_free(p3);

	l -> len = 0;
}

PARSEFUNC(pseudo_parse_align)
{
	lw_expr_t e;
	if (!**p)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	e = lwasm_parse_expr(as, p);
	
	if (!e)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	lwasm_save_expr(l, 0, e);
	
	if (**p == ',')
	{
		e = lwasm_parse_expr(as, p);
	}
	else
	{
		e = lw_expr_build(lw_expr_type_int, 0);
	}
	if (!e)
	{
		lwasm_register_error(as, l, "Bad padding");
		return;
	}
	
	lwasm_save_expr(l, 1, e);
}

RESOLVEFUNC(pseudo_resolve_align)
{
	lw_expr_t e;
	int align;

	e = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(e, lw_expr_type_int))
	{
		align = lw_expr_intval(e);
		if (align < 1)
		{
			lwasm_register_error(as, l, "Invalid alignment");
			return;
		}
	}
	
	if (lw_expr_istype(l -> addr, lw_expr_type_int))
	{
		int a;
		a = lw_expr_intval(l -> addr);
		if (a % align == 0)
		{
			l -> len = 0;
			return;
		}
		l -> len = align - (a % align);
		return;
	}
}

EMITFUNC(pseudo_emit_align)
{
	lw_expr_t e;
	int i;
	
	e = lwasm_fetch_expr(l, 1);
	for (i = 0; i < l -> len; i++)
	{
		lwasm_emitexpr(l, e, 1);
	}
}
