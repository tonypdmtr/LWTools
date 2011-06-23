/*
insn_rel.c
Copyright © 2009 William Astle

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

/*
for handling relative mode instructions
*/

#include <stdlib.h>

#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"

PARSEFUNC(insn_parse_rel8)
{
//	int v;
	lw_expr_t t, e1, e2;
//	int r;

	// sometimes there is a "#", ignore if there
	if (**p == '#')
		(*p)++;

	t = lwasm_parse_expr(as, p);
	if (!t)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	l -> len = OPLEN(instab[l -> insn].ops[0]) + 1;
	
	e1 = lw_expr_build(lw_expr_type_special, lwasm_expr_linelen, l);
	e2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, e1, l -> addr);
	lw_expr_destroy(e1);
	e1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, t, e2);
	lw_expr_destroy(e2);
	lw_expr_destroy(t);
	lwasm_save_expr(l, 0, e1);
}

EMITFUNC(insn_emit_rel8)
{
	lw_expr_t e;
	int offs;
	
	e = lwasm_fetch_expr(l, 0);
	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		lwasm_register_error(as, l, "Illegal non-constant expression");
		return;
	}
	
	offs = lw_expr_intval(e);
	if (offs < -128 || offs > 127)
	{
		lwasm_register_error(as, l, "Byte overflow");
		return;
	}
	
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	lwasm_emit(l, offs);
}

PARSEFUNC(insn_parse_rel16)
{
//	int v;
	lw_expr_t t, e1, e2;
//	int r;

	// sometimes there is a "#", ignore if there
	if (**p == '#')
		(*p)++;

	t = lwasm_parse_expr(as, p);
	if (!t)
	{
		lwasm_register_error(as, l, "Bad operand");
		lw_expr_destroy(t);
		return;
	}
	l -> len = OPLEN(instab[l -> insn].ops[0]) + 2;
	
	e1 = lw_expr_build(lw_expr_type_special, lwasm_expr_linelen, l);
	e2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, e1, l -> addr);
	lw_expr_destroy(e1);
	e1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, t, e2);
	lw_expr_destroy(e2);
	lw_expr_destroy(t);
	lwasm_save_expr(l, 0, e1);
}

EMITFUNC(insn_emit_rel16)
{
	lw_expr_t e;
//	int offs;
	
	e = lwasm_fetch_expr(l, 0);
	
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	lwasm_emitexpr(l, e, 2);
}
