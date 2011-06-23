/*
insn_indexed.c
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
for handling indexed mode instructions
*/

#include <ctype.h>
#include <string.h>

#include <lw_expr.h>

#include "lwasm.h"
#include "instab.h"

/*
l -> lint: size of operand (0, 1, 2, -1 if not determined)
l -> pb: actual post byte (from "resolve" stage) or info passed
	forward to the resolve stage (if l -> line is -1); 0x80 is indir
	bits 0-2 are register number
*/
void insn_parse_indexed_aux(asmstate_t *as, line_t *l, char **p)
{
	struct opvals { char *opstr; int pb; };
	
	static const char *regs = "X  Y  U  S  W  PCRPC ";
	static const struct opvals simpleindex[] =
	{
		{",x", 0x84},		{",y", 0xa4},		{",u", 0xc4},		{",s", 0xe4},
		{",x+", 0x80},		{",y+", 0xa0},		{",u+", 0xc0},		{",s+", 0xe0},
		{",x++", 0x81},		{",y++", 0xa1},		{",u++", 0xc1},		{",s++", 0xe1},
		{",-x", 0x82},		{",-y", 0xa2},		{",-u", 0xc2},		{",-s", 0xe2},
		{",--x", 0x83},		{",--y", 0xa3},		{",--u", 0xc3},		{",--s", 0xe3},
		{"a,x", 0x86},		{"a,y", 0xa6},		{"a,u", 0xc6},		{"a,s", 0xe6},
		{"b,x", 0x85},		{"b,y", 0xa5},		{"b,u", 0xc5},		{"b,s", 0xe5},
		{"e,x", 0x87},		{"e,y", 0xa7},		{"e,u", 0xc7},		{"e,s", 0xe7},
		{"f,x",	0x8a},		{"f,y",	0xaa},		{"f,u", 0xca},		{"f,s", 0xea},
		{"d,x", 0x8b},		{"d,y", 0xab},		{"d,u", 0xcb},		{"d,s", 0xed},
		{"w,x", 0x8e},		{"w,y", 0xae},		{"w,u", 0xce},		{"w,s", 0xee},
		{",w", 0x8f},							{",w++", 0xcf},		{",--w", 0xef},
		
		{"[,x]", 0x94},		{"[,y]", 0xb4},		{"[,u]", 0xd4},		{"[,s]", 0xf4},
		{"[,x++]", 0x91},	{"[,y++]", 0xb1},	{"[,u++]", 0xd1},	{"[,s++]", 0xf1},
		{"[,--x]", 0x93},	{"[,--y]", 0xb3},	{"[,--u]", 0xd3},	{"[,--s]", 0xf3},
		{"[a,x]", 0x96},	{"[a,y]", 0xb6},	{"[a,u]", 0xd6},	{"[a,s]", 0xf6},
		{"[b,x]", 0x95},	{"[b,y]", 0xb5},	{"[b,u]", 0xd5},	{"[b,s]", 0xf5},
		{"[e,x]", 0x97},	{"[e,y]", 0xb7},	{"[e,u]", 0xd7},	{"[e,s]", 0xf7},
		{"[f,x]", 0x9a},	{"[f,y]", 0xba},	{"[f,u]", 0xda},	{"[f,s]", 0xfa},
		{"[d,x]", 0x9b},	{"[d,y]", 0xbb},	{"[d,u]", 0xdb},	{"[d,s]", 0xfd},
		{"[w,x]", 0x9e},	{"[w,y]", 0xbe},	{"[w,u]", 0xde},	{"[w,s]", 0xfe},
		{"[,w]", 0x90},							{"[,w++]", 0xd0},	{"[,--w]", 0xf0},
		
		{ "", -1 }
	};

	static const char *regs9 = "X  Y  U  S     PCRPC ";
	static const struct opvals simpleindex9[] =
	{
		{",x", 0x84},		{",y", 0xa4},		{",u", 0xc4},		{",s", 0xe4},
		{",x+", 0x80},		{",y+", 0xa0},		{",u+", 0xc0},		{",s+", 0xe0},
		{",x++", 0x81},		{",y++", 0xa1},		{",u++", 0xc1},		{",s++", 0xe1},
		{",-x", 0x82},		{",-y", 0xa2},		{",-u", 0xc2},		{",-s", 0xe2},
		{",--x", 0x83},		{",--y", 0xa3},		{",--u", 0xc3},		{",--s", 0xe3},
		{"a,x", 0x86},		{"a,y", 0xa6},		{"a,u", 0xc6},		{"a,s", 0xe6},
		{"b,x", 0x85},		{"b,y", 0xa5},		{"b,u", 0xc5},		{"b,s", 0xe5},
		{"d,x", 0x8b},		{"d,y", 0xab},		{"d,u", 0xcb},		{"d,s", 0xed},
		
		{"[,x]", 0x94},		{"[,y]", 0xb4},		{"[,u", 0xd4},		{"[,s]", 0xf4},
		{"[,x++]", 0x91},	{"[,y++]", 0xb1},	{"[,u++]", 0xd1},	{"[,s++]", 0xf1},
		{"[,--x]", 0x93},	{"[,--y]", 0xb3},	{"[,--u]", 0xd3},	{"[,--s]", 0xf3},
		{"[a,x]", 0x96},	{"[a,y]", 0xb6},	{"[a,u]", 0xd6},	{"[a,s]", 0xf6},
		{"[b,x]", 0x95},	{"[b,y]", 0xb5},	{"[b,u]", 0xd5},	{"[b,s]", 0xf5},
		{"[d,x]", 0x9b},	{"[d,y]", 0xbb},	{"[d,u]", 0xdb},	{"[d,s]", 0xfd},
		
		{ "", -1 }
	};
	char stbuf[25];
	int i, j, rn;
	int indir = 0;
	int f0 = 1;
	const struct opvals *simples;
	const char *reglist;
	lw_expr_t e;
		
	if (as -> target == TARGET_6809)
	{
		simples = simpleindex9;
		reglist = regs9;
	}
	else
	{
		simples = simpleindex;
		reglist = regs;
	}
	
	// fetch out operand for lookup
	for (i = 0; i < 24; i++)
	{
		if (*((*p) + i) && !isspace(*((*p) + i)))
			stbuf[i] = *((*p) + i);
		else
			break;
	}
	stbuf[i] = '\0';
	
	// now look up operand in "simple" table
	if (!*((*p) + i) || isspace(*((*p) + i)))
	{
		// do simple lookup
		for (j = 0; simples[j].opstr[0]; j++)
		{
			if (!strcasecmp(stbuf, simples[j].opstr))
				break;
		}
		if (simples[j].opstr[0])
		{
			l -> pb = simples[j].pb;
			l -> lint = 0;
			(*p) += i;
			return;
		}
	}

	// now do the "hard" ones

	// is it indirect?
	if (**p == '[')
	{
		indir = 1;
		(*p)++;
	}
	
	// look for a "," - all indexed modes have a "," except extended indir
	rn = 0;
	for (i = 0; (*p)[i] && !isspace((*p)[i]); i++)
	{
		if ((*p)[i] == ',')
		{
			rn = 1;
			break;
		}
	}

	// if no "," and indirect, do extended indir
	if (!rn && indir)
	{
		// extended indir
		l -> pb = 0x9f;
		e = lwasm_parse_expr(as, p);
		if (!e || **p != ']')
		{
			lwasm_register_error(as, l, "Bad operand");
			return;
		}
		lwasm_save_expr(l, 0, e);
		
		(*p)++;
		l -> lint = 2;
		return;
	}

	if (**p == '<')
	{
		l -> lint = 1;
		(*p)++;
	}
	else if (**p == '>')
	{
		l -> lint = 2;
		(*p)++;
	}

	if (**p == '0' && *(*p+1) == ',')
	{
		f0 = 1;
	}
	
	// now we have to evaluate the expression
	e = lwasm_parse_expr(as, p);
	if (!e)
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	lwasm_save_expr(l, 0, e);

	// now look for a comma; if not present, explode
	if (*(*p)++ != ',')
	{
		lwasm_register_error(as, l, "Bad operand");
		return;
	}
	
	// now get the register
	rn = lwasm_lookupreg3(reglist, p);
	if (rn < 0)
	{
		lwasm_register_error(as, l, "Bad register");
		return;
	}
	
	if (indir)
	{
		if (**p != ']')
		{
			lwasm_register_error(as, l, "Bad operand");
			return;
		}
		else
			(*p)++;
	}

	// nnnn,W is only 16 bit (or 0 bit)
	if (rn == 4)
	{
		if (l -> lint == 1)
		{
			lwasm_register_error(as, l, "n,W cannot be 8 bit");
			return;
		}

		if (l -> lint == 2)
		{
			l -> pb = indir ? 0xb0 : 0xcf;
			l -> lint = 2;
			return;
		}
		
		l -> pb = (0x80 * indir) | rn;

/* [,w] and ,w
			if (indir)
				*b1 = 0x90;
			else
				*b1 = 0x8f;
*/
		return;
	}
	
	// PCR? then we have PC relative addressing (like B??, LB??)
	if (rn == 5 || (rn == 6 && CURPRAGMA(l, PRAGMA_PCASPCR)))
	{
		lw_expr_t e1, e2;
		// external references are handled exactly the same as for
		// relative addressing modes
		// on pass 1, adjust the expression for a subtraction of the
		// current address
		// e - (addr + linelen) => e - addr - linelen
		
		e2 = lw_expr_build(lw_expr_type_special, lwasm_expr_linelen, l);
		e1 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, e, e2);
		lw_expr_destroy(e2);
		e2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_minus, e1, l -> addr);
		lw_expr_destroy(e1);
		lwasm_save_expr(l, 0, e2);
		if (l -> lint == 1)
		{
			l -> pb = indir ? 0x9C : 0x8C;
			return;
		}
		if (l -> lint == 2)
		{
			l -> pb = indir ? 0x9D : 0x8D;
			return;
		}
	}
	
	if (rn == 6)
	{
		if (l -> lint == 1)
		{
			l -> pb = indir ? 0x9C : 0x8C;
			return;
		}
		if (l -> lint == 2)
		{
			l -> pb = indir ? 0x9D : 0x8D;
			return;
		}
	}

	l -> pb = (indir * 0x80) | rn | (f0 * 0x40);
}

PARSEFUNC(insn_parse_indexed)
{
	l -> lint = -1;
	insn_parse_indexed_aux(as, l, p);

	if (l -> lint != -1)
	{
		l -> len = OPLEN(instab[l -> insn].ops[0]) + l -> lint + 1;
	}
}

void insn_resolve_indexed_aux(asmstate_t *as, line_t *l, int force, int elen)
{
	// here, we have an expression which needs to be
	// resolved; the post byte is determined here as well
	lw_expr_t e, e2;
	int pb = -1;
	int v;
	
	if (l -> len != -1)
		return;
	
	e = lwasm_fetch_expr(l, 0);
	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		// temporarily set the instruction length to see if we get a
		// constant for our expression; if so, we can select an instruction
		// size
		e2 = lw_expr_copy(e);
		// magic 2 for 8 bit (post byte + offset)
		l -> len = OPLEN(instab[l -> insn].ops[0]) + elen + 2;
		lwasm_reduce_expr(as, e2);
//		l -> len += 1;
//		e3 = lw_expr_copy(e);
//		lwasm_reduce_expr(as, e3);
		l -> len = -1;
		if (lw_expr_istype(e2, lw_expr_type_int))
		{
			v = lw_expr_intval(e2);
			// we have a reducible expression here which depends on
			// the size of this instruction
			if (v < -128 || v > 127)
			{
				l -> lint = 2;
				switch (l -> pb & 0x07)
				{
				case 0:
				case 1:
				case 2:
				case 3:
					pb = 0x89 | ((l -> pb & 0x03) << 5) | ((l -> pb & 0x80) ? 0x10 : 0);
					break;
			
				case 4: // W
					pb = (l -> pb & 0x80) ? 0xD0 : 0xCF;
					break;
				
				case 5: // PCR
				case 6: // PC
					pb = (l -> pb & 0x80) ? 0x9D : 0x8D;
					break;
				}
				
				l -> pb = pb;
				lw_expr_destroy(e2);
//				lw_expr_destroy(e3);
				return;
			}
			else if ((l -> pb & 0x80) || ((l -> pb & 0x07) > 3) || v < -16 || v > 15)
			{
				// if not a 5 bit value, is indirect, or is not X,Y,U,S
				l -> lint = 1;
				switch (l -> pb & 0x07)
				{
				case 0:
				case 1:
				case 2:
				case 3:
					pb = 0x88 | ((l -> pb & 0x03) << 5) | ((l -> pb & 0x80) ? 0x10 : 0);
					break;
			
				case 4: // W
					// use 16 bit because W doesn't have 8 bit, unless 0
					if (v == 0 && !(CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) || l -> pb & 0x40))
					{
						pb = (l -> pb & 0x80) ? 0x90 : 0x8F;
						l -> lint = 0;
					}
					else
					{
						pb = (l -> pb & 0x80) ? 0xD0 : 0xCF;
						l -> lint = 2;
					}
					break;
				
				case 5: // PCR
				case 6: // PC
					pb = (l -> pb & 0x80) ? 0x9C : 0x8C;
					break;
				}
			
				l -> pb = pb;
				lw_expr_destroy(e2);
				return;
			}
			else
			{
				// we have X,Y,U,S and a possible 16 bit here
				l -> lint = 0;
				
				if (v == 0 && !(CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) || l -> pb & 0x40))
				{
					pb = (l -> pb & 0x03) << 5 | 0x84;
				}	
				else
				{
					pb = (l -> pb & 0x03) << 5 | (v & 0x1F);
				}
				l -> pb = pb;
				lw_expr_destroy(e2);
				return;
			}
		}
		lw_expr_destroy(e2);
	}
		
	if (lw_expr_istype(e, lw_expr_type_int))
	{
		// we know how big it is
		v = lw_expr_intval(e);
		if (v < -128 || v > 127)
		{
		do16bit:
			l -> lint = 2;
			switch (l -> pb & 0x07)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				pb = 0x89 | (l -> pb & 0x03) << 5 | ((l -> pb & 0x80) ? 0x10 : 0);
				break;
			
			case 4: // W
				pb = (l -> pb & 0x80) ? 0xD0 : 0xCF;
				break;
				
			case 5: // PCR
			case 6: // PC
				pb = (l -> pb & 0x80) ? 0x9D : 0x8D;
				break;
			}
			
			l -> pb = pb;
			return;
		}
		else if ((l -> pb & 0x80) || ((l -> pb & 0x07) > 3) || v < -16 || v > 15)
		{
			// if not a 5 bit value, is indirect, or is not X,Y,U,S
			l -> lint = 1;
			switch (l -> pb & 0x07)
			{
			case 0:
			case 1:
			case 2:
			case 3:
				pb = 0x88 | (l -> pb & 0x03) << 5 | ((l -> pb & 0x80) ? 0x10 : 0);
				break;
			
			case 4: // W
				// use 16 bit because W doesn't have 8 bit, unless 0
				if (v == 0 && !(CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) || l -> pb & 0x40))
				{
					pb = (l -> pb & 0x80) ? 0x90 : 0x8F;
					l -> lint = 0;
				}
				else
				{
					pb = (l -> pb & 0x80) ? 0xD0 : 0xCF;
					l -> lint = 2;
				}
				break;
				
			case 5: // PCR
			case 6: // PC
				pb = (l -> pb & 0x80) ? 0x9C : 0x8C;
				break;
			}
			
			l -> pb = pb;
			return;
		}
		else
		{
			// we have X,Y,U,S and a possible 16 bit here
			l -> lint = 0;
			
			if (v == 0 && !(CURPRAGMA(l, PRAGMA_NOINDEX0TONONE) || l -> pb & 0x40))
			{
				pb = (l -> pb & 0x03) << 5 | 0x84;
			}
			else
			{
				pb = (l -> pb & 0x03) << 5 | (v & 0x1F);
			}
			l -> pb = pb;
			return;
		}
	}
	else
	{
		// we don't know how big it is
		if (!force)
			return;
		// force 16 bit if we don't know
		l -> lint = 2;
		goto do16bit;
	}
}

RESOLVEFUNC(insn_resolve_indexed)
{
	if (l -> lint == -1)
		insn_resolve_indexed_aux(as, l, force, 0);
	
	if (l -> lint != -1 && l -> pb != -1)
	{
		l -> len = OPLEN(instab[l -> insn].ops[0]) + l -> lint + 1;
	}
}

void insn_emit_indexed_aux(asmstate_t *as, line_t *l)
{
	lw_expr_t e;
	
	lwasm_emitop(l, instab[l -> insn].ops[0]);
	lwasm_emitop(l, l -> pb);
	if (l -> lint > 0)
	{
		e = lwasm_fetch_expr(l, 0);
		lwasm_emitexpr(l, e, l -> lint);
	}
}

EMITFUNC(insn_emit_indexed)
{
	insn_emit_indexed_aux(as, l);
}
