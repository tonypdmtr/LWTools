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
#include <stdlib.h>
#include <time.h>

#include <lw_alloc.h>

#include "lwasm.h"
#include "instab.h"
#include "input.h"

#include "lw_string.h"

extern void register_struct_entry(asmstate_t *as, line_t *l, int size, structtab_t *ss);

// for "dts"
PARSEFUNC(pseudo_parse_dts)
{
	time_t tp;
	char *t;
	
	skip_operand(p);
	l -> len = 0;

	tp = time(NULL);
	t = ctime(&tp);

	while (*t)
	{
		lwasm_emit(l, *t);
		t++;
		l -> len += 1;
	}
}

EMITFUNC(pseudo_emit_dts)
{
}

// for "dtb"
PARSEFUNC(pseudo_parse_dtb)
{
	skip_operand(p);
	l -> len = 6;
}

EMITFUNC(pseudo_emit_dtb)
{
	time_t tp;
	struct tm *t;
	
	tp = time(NULL);
	t = localtime(&tp);

	lwasm_emit(l, t -> tm_year);
	lwasm_emit(l, t -> tm_mon + 1);
	lwasm_emit(l, t -> tm_mday);
	lwasm_emit(l, t -> tm_hour);
	lwasm_emit(l, t -> tm_min);
	lwasm_emit(l, t -> tm_sec);
}

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
			lwasm_register_error(as, l, "Bad expression (#%d)", i);
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
			lwasm_register_error(as, l, "Bad expression (#%d)", i);
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

static int cstringlen(asmstate_t *as, line_t *ln, char **p, char delim)
{
	int l = 0;
	char *str = NULL;
	int blen = 0;
	int bsize = 0;
	
	if (!(as -> pragmas & PRAGMA_CESCAPES))
	{
		for (; **p && **p != delim; (*p)++)
		{
			l++;
			if (blen >= bsize)
			{
				str = lw_realloc(str, bsize + 32);
				bsize++;
			}
			str[blen++] = **p;
		}
	}
	else
	{
		while (**p && **p != delim)
		{
			int wch = **p;
			if (**p == '\\')
			{
				/* escape sequence */

				(*p)++;
				if (!**p)
					break;

				switch (**p)
				{
					/* octal sequence or NUL */
					/* skip the "0", then skip up to two more digits */
				case '0':
				case '1':
				case '2':
				case '3':
					wch = **p;
					wch -= 0x30;
					if ((*p)[1] >= '0' && (*p)[1] < '8')
					{
						(*p)++;
						wch *= 8;
						wch += **p - 0x30;
					}
					if ((*p)[1] >= '0' && (*p)[1] < '8')
					{
						(*p)++;
						wch *= 8;
						wch += **p -0x30;
					}
					break;			

				/* hexadecimal value */
				case 'x':
					(*p)++;			// ignore "x"
					wch = 0;
					if (**p)		// skip digit 1
					{
						wch = **p - 0x30;
						if (wch > 9)
							wch -= 7;
						if (wch > 9)
							wch -= 32;
						(*p)++;
					}
					if (**p)
					{
						int i;
						i = **p - 0x30;
						if (i > 9)
							i -= 7;
						if (i > 9)
							i -= 32;
						wch = wch * 16 + i;
					}
					break;

				case 'a':
					wch = 7;
					break;
				
				case 'b':
					wch = 8;
					break;
				
				case 't':
					wch = 9;
					break;
				
				case 'n':
					wch = 10;
					break;
				
				case 'v':
					wch = 11;
					break;
				
				case 'f':
					wch = 12;
					break;
				
				case 'r':
					wch = 13;
				
				/* everything else represents itself */
				default:
					break;
				}
			}
			/* now "wch" is the character to write out */
			l++;
			(*p)++;
			if (blen >= bsize)
			{
				str = lw_realloc(str, bsize + 32);
				bsize += 32;
			}
			str[blen++] = wch;
		}
	}
	/* do something with the string here */
	/* l is the string length, str is the string itself */
	ln -> lstr = str;
	return l;
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
	
	
	i = cstringlen(as, l, p, delim);
	
	if (**p != delim)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	(*p)++;	
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
	
	i = cstringlen(as, l, p, delim);
	
	if (**p != delim)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	(*p)++;
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
	
	i = cstringlen(as, l, p, delim);

	if (**p != delim)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	(*p)++;
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
	else
	{
		if (l -> inmod)
		{
			l -> dlen = -1;
			l -> len = 0;
		}
	}
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_rmb)
{
	lw_expr_t expr;
	
	if (l -> lint)
		return;
	
	if (l -> inmod)
	{
		if (l -> dlen >= 0)
			return;
	}
	else
	{
		if (l -> len >= 0)
			return;
	}
			
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		if (lw_expr_intval(expr) < 0)
		{
			lwasm_register_error(as, l, "Negative reservation sizes make no sense!");
			l -> len = 0;
			l -> dlen = 0;
			return;
		}
		if (l -> inmod)
			l -> dlen = lw_expr_intval(expr);
		else
			l -> len = lw_expr_intval(expr);
	}
}

EMITFUNC(pseudo_emit_rmb)
{
	if (l -> lint)
		return;

	if (l -> len < 0 || l -> dlen < 0)
		lwasm_register_error(as, l, "Expression not constant: %d %d", l -> len, l -> dlen);
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
	else
	{
		if (l -> inmod)
		{
			l -> dlen = -1;
			l -> len = 0;
		}
	}
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_rmd)
{
	lw_expr_t expr;
	
	if (l -> lint)
		return;
	
	if (l -> inmod)
	{
		if (l -> dlen >= 0)
			return;
	}
	else
	{
		if (l -> len >= 0)
			return;
	}
	
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		if (lw_expr_intval(expr) < 0)
		{
			lwasm_register_error(as, l, "Negative reservation sizes make no sense!");
			l -> len = 0;
			l -> dlen = 0;
			return;
		}
		if (l -> inmod)
			l -> dlen = lw_expr_intval(expr) * 2;
		else
			l -> len = lw_expr_intval(expr) * 2;
	}
}

EMITFUNC(pseudo_emit_rmd)
{
	if (l -> lint)
		return;

	if (l -> len < 0 || l -> dlen < 0)
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
	else
	{
		if (as -> inmod)
		{
			l -> dlen = -1;
			l -> len = 0;
		}
	}
	lwasm_save_expr(l, 0, expr);
}

RESOLVEFUNC(pseudo_resolve_rmq)
{
	lw_expr_t expr;

	if (l -> lint)
		return;
	
	if (l -> inmod)
	{
		if (l -> dlen >= 0)
			return;
	}
	else
	{
		if (l -> len >= 0)
			return;
	}
	
	expr = lwasm_fetch_expr(l, 0);
	
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		if (lw_expr_intval(expr) < 0)
		{
			lwasm_register_error(as, l, "Negative reservation sizes make no sense!");
			l -> len = 0;
			l -> dlen = 0;
			return;
		}
		if (l -> inmod)
			l -> dlen = lw_expr_intval(expr) * 4;
		else 
			l -> len = lw_expr_intval(expr) * 4;
	}
}

EMITFUNC(pseudo_emit_rmq)
{
	if (l -> lint)
		return;

	if (l -> len < 0 || l -> dlen < 0)
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
		if (lw_expr_intval(expr) < 0)
		{
			lwasm_register_error(as, l, "Negative block sizes make no sense!");
			l -> len = 0;
			return;
		}
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
		if (lw_expr_intval(expr) < 0)
		{
			lwasm_register_error(as, l, "Negative block sizes make no sense!");
			l -> len = 0;
			return;
		}
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
		if (lw_expr_intval(expr) < 0)
		{
			lwasm_register_error(as, l, "Negative block sizes make no sense!");
			l -> len = 0;
			return;
		}
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
	
	lw_expr_destroy(l -> daddr);
	l -> daddr = e;
	
	if (l -> inmod == 0)
	{
		lw_expr_destroy(l -> addr);
		l -> addr = e;
	}
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
	lw_expr_destroy(e);
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
	lw_expr_destroy(e);
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
		lw_expr_destroy(e);
		return;
	}
	l -> dpval = lw_expr_intval(e) & 0xff;
	lw_expr_destroy(e);
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
	skip_operand(p);
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

again:
	for (i = 0; (*p)[i] && !isspace((*p)[i]) && (*p)[i] != '|' && (*p)[i] != '&'; i++)
		/* do nothing */ ;
	
	sym = lw_strndup(*p, i);
	(*p) += i;
	
	s = lookup_symbol(as, l, sym);
	
	lw_free(sym);
	
	if (!s)
	{
		if (**p == '|')
		{
			(*p)++;
			goto again;
		}
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
	skip_operand(p);
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
	for (i = 0; (*p)[i] && !isspace((*p)[i]) && (*p)[i] != '&' && (*p)[i] != '|'; i++)
		/* do nothing */ ;
	
	sym = lw_strndup(*p, i);
	(*p) += i;
	
	s = lookup_symbol(as, l, sym);
	
	lw_free(sym);

	if (s)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
		return;
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

		for (p2 = *p; *p2 && (*p2 != delim); p2++)
			/* do nothing */ ;
	}
	else
	{
		for (p2 = *p; *p2 && !isspace(*p2); p2++)
			/* do nothing */ ;
	}
	fn = lw_strndup(*p, p2 - *p);
	*p = p2;
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
	lw_free(fn);
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
		(*p)++;
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

/* string conditional argument parser */
/*
argument syntax:

a bare word ended by whitespace, comma, or NUL
a double quote delimited string containing arbitrary printable characters
a single quote delimited string containing arbitrary printable characters

In a double quoted string, a double quote cannot be represented.
In a single quoted string, a single quote cannot be represented.

*/
char *strcond_parsearg(char **p)
{
	char *arg;
	char *tstr;
	int i;
	tstr = *p;

	if (!**p || isspace(**p))
	{
		return lw_strdup("");
	}
	
	if (*tstr == '"')
	{
		// double quote delim
		tstr++;
		for (i = 0; tstr[i] && tstr[i] != '"'; i++)
			/* do nothing */ ;
		
		arg = lw_alloc(i + 1);
		strncpy(arg, tstr, i);
		arg[i] = 0;
		
		if (tstr[i])
			i++;
		
		*p += i;
		return arg;
	}
	else if (*tstr == '\'')
	{
		// single quote delim
		tstr++;
		for (i = 0; tstr[i] && tstr[i] != '\''; i++)
			/* do nothing */ ;
		
		arg = lw_alloc(i + 1);
		strncpy(arg, tstr, i);
		arg[i] = 0;
		
		if (tstr[i])
			i++;
		
		*p += i;
		return arg;
	}
	else
	{
		// bare word - whitespace or comma delim
		for (i = 0; tstr[i] && !isspace(tstr[i]) && tstr[i] != ','; i++)
			/* do nothing */ ;
		
		arg = lw_alloc(i + 1);
		strncpy(arg, tstr, i);
		arg[i] = 0;
		if (tstr[i] == ',')
			i++;
		
		*p += i;
		return arg;
	}
}

/* string conditional helpers */
/* return "1" for true, "0" for false */
int strcond_eq(char **p)
{
	char *arg1;
	char *arg2;
	int c = 0;
		
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	if (strcmp(arg1, arg2) == 0)
		c = 1;
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_ieq(char **p)
{
	char *arg1;
	char *arg2;
	int c = 0;
		
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	if (strcasecmp(arg1, arg2) == 0)
		c = 1;
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_ne(char **p)
{
	char *arg1;
	char *arg2;
	int c = 0;
		
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	if (strcmp(arg1, arg2) != 0)
		c = 1;
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_ine(char **p)
{
	char *arg1;
	char *arg2;
	int c = 0;
		
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	if (strcasecmp(arg1, arg2) != 0)
		c = 1;
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_peq(char **p)
{
	char *arg0;
	char *arg1;
	char *arg2;
	int plen;
	int c = 0;
	
	arg0 = strcond_parsearg(p);
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	plen = strtol(arg0, NULL, 10);
	if (strlen(arg1) > plen)
		arg1[plen] = 0;
	if (strlen(arg2) > plen)
		arg2[plen] = 0;

	if (strcmp(arg1, arg2) == 0)
		c = 1;
	lw_free(arg0);
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_ipeq(char **p)
{
	char *arg0;
	char *arg1;
	char *arg2;
	int plen;
	int c = 0;
	
	arg0 = strcond_parsearg(p);
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	plen = strtol(arg0, NULL, 10);
	if (strlen(arg1) > plen)
		arg1[plen] = 0;
	if (strlen(arg2) > plen)
		arg2[plen] = 0;

	if (strcasecmp(arg1, arg2) == 0)
		c = 1;
	lw_free(arg0);
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_pne(char **p)
{
	char *arg0;
	char *arg1;
	char *arg2;
	int plen;
	int c = 0;
	
	arg0 = strcond_parsearg(p);
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	plen = strtol(arg0, NULL, 10);
	if (strlen(arg1) > plen)
		arg1[plen] = 0;
	if (strlen(arg2) > plen)
		arg2[plen] = 0;

	if (strcmp(arg1, arg2) != 0)
		c = 1;
	lw_free(arg0);
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_ipne(char **p)
{
	char *arg0;
	char *arg1;
	char *arg2;
	int plen;
	int c = 0;
	
	arg0 = strcond_parsearg(p);
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);

	plen = strtol(arg0, NULL, 10);
	if (strlen(arg1) > plen)
		arg1[plen] = 0;
	if (strlen(arg2) > plen)
		arg2[plen] = 0;

	if (strcasecmp(arg1, arg2) != 0)
		c = 1;
	lw_free(arg0);
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_seq(char **p)
{
	char *arg0;
	char *arg1;
	char *arg2;
	char *rarg1;
	char *rarg2;
	
	int plen;
	int c = 0;
	
	arg0 = strcond_parsearg(p);
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	rarg1 = arg1;
	rarg2 = arg2;

	plen = strtol(arg0, NULL, 10);
	if (strlen(arg1) > plen)
	{
		rarg1 += strlen(arg1) - plen;
	}
	if (strlen(arg2) > plen)
	{
		rarg2 += strlen(arg2) - plen;
	}
	if (strcmp(rarg1, rarg2) == 0)
		c = 1;
	lw_free(arg0);
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_iseq(char **p)
{
	char *arg0;
	char *arg1;
	char *arg2;
	char *rarg1;
	char *rarg2;
	
	int plen;
	int c = 0;
	
	arg0 = strcond_parsearg(p);
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	rarg1 = arg1;
	rarg2 = arg2;
		
	plen = strtol(arg0, NULL, 10);
	if (strlen(arg1) > plen)
	{
		rarg1 += strlen(arg1) - plen;
	}
	if (strlen(arg2) > plen)
	{
		rarg2 += strlen(arg2) - plen;
	}
	
	if (strcasecmp(rarg1, rarg2) == 0)
		c = 1;
	lw_free(arg0);
	lw_free(arg1);
	lw_free(arg2);
	return c;
}


int strcond_sne(char **p)
{
	char *arg0;
	char *arg1;
	char *arg2;
	char *rarg1;
	char *rarg2;
	
	int plen;
	int c = 0;
	
	arg0 = strcond_parsearg(p);
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	rarg1 = arg1;
	rarg2 = arg2;
		
	plen = strtol(arg0, NULL, 10);
	if (strlen(arg1) > plen)
	{
		rarg1 += strlen(arg1) - plen;
	}
	if (strlen(arg2) > plen)
	{
		rarg2 += strlen(arg2) - plen;
	}
	
	if (strcmp(rarg1, rarg2) != 0)
		c = 1;
	lw_free(arg0);
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

int strcond_isne(char **p)
{
	char *arg0;
	char *arg1;
	char *arg2;
	char *rarg1;
	char *rarg2;
	
	int plen;
	int c = 0;
	
	arg0 = strcond_parsearg(p);
	arg1 = strcond_parsearg(p);
	arg2 = strcond_parsearg(p);
	
	rarg1 = arg1;
	rarg2 = arg2;
		
	plen = strtol(arg0, NULL, 10);
	if (strlen(arg1) > plen)
	{
		rarg1 += strlen(arg1) - plen;
	}
	if (strlen(arg2) > plen)
	{
		rarg2 += strlen(arg2) - plen;
	}
	
	if (strcasecmp(rarg1, rarg2) != 0)
		c = 1;
	lw_free(arg0);
	lw_free(arg1);
	lw_free(arg2);
	return c;
}

/* string conditionals */
PARSEFUNC(pseudo_parse_ifstr)
{
	static struct strconds
	{
		char *str;
		int (*fn)(char **ptr);
	} strops[] = {
		{ "eq", strcond_eq },
		{ "ieq", strcond_ieq },
		{ "ne", strcond_ne },
		{ "ine", strcond_ine },
		{ "peq", strcond_peq },
		{ "ipeq", strcond_ipeq },
		{ "pne", strcond_pne },
		{ "ipne", strcond_ipne },
		{ "seq", strcond_seq },
		{ "iseq", strcond_iseq },
		{ "sne", strcond_sne },
		{ "isne", strcond_isne },
		{ NULL, 0 }
	};
	int tv = 0;
	char *tstr;
	int strop;
	
	l -> len = 0;
	
	if (as -> skipcond && !(as -> skipmacro))
	{
		as -> skipcount++;
		skip_operand(p);
		return;
	}

	tstr = strcond_parsearg(p);
	if (!**p || isspace(**p))	
	{
		lwasm_register_error(as, l, "Bad string condition");
		return;
	}
		
	for (strop = 0; strops[strop].str != NULL; strop++)
		if (strcasecmp(strops[strop].str, tstr) == 0)
			break;
	
	lw_free(tstr);
	
	if (strops[strop].str == NULL)
	{
		lwasm_register_error(as, l, "Bad string condition");
	}

	tv = (*(strops[strop].fn))(p);
	
	if (!tv)
	{
		as -> skipcond = 1;
		as -> skipcount = 1;
	}
}
