/*
lwasm.c

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

#define ___lwasm_c_seen___

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include <lw_expr.h>
#include <lw_alloc.h>
#include <lw_string.h>

#include "lwasm.h"

void lwasm_register_error(asmstate_t *as, line_t *l, const char *msg, ...);

int lwasm_expr_exportable(asmstate_t *as, lw_expr_t expr)
{
	return 0;
}

int lwasm_expr_exportval(asmstate_t *as, lw_expr_t expr)
{
	return 0;
}

lw_expr_t lwasm_evaluate_var(char *var, void *priv)
{
	asmstate_t *as = (asmstate_t *)priv;
	lw_expr_t e;
	importlist_t *im;
	struct symtabe *s;
	
	s = lookup_symbol(as, as -> cl, var);
	if (s)
	{
		e = lw_expr_build(lw_expr_type_special, lwasm_expr_syment, s);
		return e;
	}
	
	// undefined here is undefied unless output is object
	if (as -> output_format != OUTPUT_OBJ)
		goto nomatch;
	
	// check for import
	for (im = as -> importlist; im; im = im -> next)
	{
		if (!strcmp(im -> symbol, var))
			break;
	}
	
	// check for "undefined" to import automatically
	if ((as -> passno != 0) && !im && CURPRAGMA(as -> cl, PRAGMA_UNDEFEXTERN))
	{
		im = lw_alloc(sizeof(importlist_t));
		im -> symbol = lw_strdup(var);
		im -> next = as -> importlist;
		as -> importlist = im;
	}
	
	if (!im)
		goto nomatch;

	e = lw_expr_build(lw_expr_type_special, lwasm_expr_import, im);
	return e;

nomatch:
	if (as -> badsymerr)
	{
		lwasm_register_error(as, as -> cl, "Undefined symbol %s", var);
	}
	return NULL;
}

lw_expr_t lwasm_evaluate_special(int t, void *ptr, void *priv)
{
	switch (t)
	{
	case lwasm_expr_secbase:
		{
//			sectiontab_t *s = priv;
			asmstate_t *as = priv;
			if (as -> exportcheck && ptr == as -> csect)
				return lw_expr_build(lw_expr_type_int, 0);
			return NULL;
		}
			
	case lwasm_expr_linelen:
		{
			line_t *cl = ptr;
			if (cl -> len == -1)
				return NULL;
			return lw_expr_build(lw_expr_type_int, cl -> len);
		}
		break;
		
	case lwasm_expr_lineaddr:
		{
			line_t *cl = ptr;
			if (cl -> addr)
				return lw_expr_copy(cl -> addr);
			else
				return NULL;
		}
	
	case lwasm_expr_syment:
		{
			struct symtabe *sym = ptr;
			return lw_expr_copy(sym -> value);
		}
	
	case lwasm_expr_import:
		{
			return NULL;
		}
	
	case lwasm_expr_nextbp:
		{
			line_t *cl = ptr;
			for (cl = cl -> next; cl; cl = cl -> next)
			{
				if (cl -> isbrpt)
					break;
			}
			if (cl)
			{
				return lw_expr_copy(cl -> addr);
			}
			return NULL;
		}
	
	case lwasm_expr_prevbp:
		{
			line_t *cl = ptr;
			for (cl = cl -> prev; cl; cl = cl -> prev)
			{
				if (cl -> isbrpt)
					break;
			}
			if (cl)
			{
				return lw_expr_copy(cl -> addr);
			}
			return NULL;
		}
	}
	return NULL;
}

void lwasm_register_error(asmstate_t *as, line_t *l, const char *msg, ...)
{
	lwasm_error_t *e;
	va_list args;
	char errbuff[1024];
	int r;
	
	if (!l)
		return;

	va_start(args, msg);
	
	e = lw_alloc(sizeof(lwasm_error_t));
	
	e -> next = l -> err;
	l -> err = e;
	
	as -> errorcount++;
	
	r = vsnprintf(errbuff, 1024, msg, args);
	e -> mess = lw_strdup(errbuff);
	
	va_end(args);
}

void lwasm_register_warning(asmstate_t *as, line_t *l, const char *msg, ...)
{
	lwasm_error_t *e;
	va_list args;
	char errbuff[1024];
	int r;
	
	if (!l)
		return;

	va_start(args, msg);
	
	e = lw_alloc(sizeof(lwasm_error_t));
	
	e -> next = l -> err;
	l -> err = e;
	
	as -> errorcount++;
	
	r = vsnprintf(errbuff, 1024, msg, args);
	e -> mess = lw_strdup(errbuff);
	
	va_end(args);
}

int lwasm_next_context(asmstate_t *as)
{
	int r;
	r = as -> nextcontext;
	as -> nextcontext++;
	return r;
}

void lwasm_emit(line_t *cl, int byte)
{
	if (cl -> as -> output_format == OUTPUT_OBJ && cl -> csect == NULL)
	{
		lwasm_register_error(cl -> as, cl, "Instruction generating output outside of a section");
		return;
	}
	if (cl -> outputl < 0)
		cl -> outputl = 0;

	if (cl -> outputl == cl -> outputbl)
	{
		cl -> output = lw_realloc(cl -> output, cl -> outputbl + 8);
		cl -> outputbl += 8;
	}
	cl -> output[cl -> outputl++] = byte & 0xff;
	
	if (cl -> inmod)
	{
		asmstate_t *as = cl -> as;
		// update module CRC
		// this is a direct transliteration from the nitros9 asm source
		// to C; it can, no doubt, be optimized for 32 bit processing  
		byte &= 0xff;

		byte ^= (as -> crc)[0];
		(as -> crc)[0] = (as -> crc)[1];
		(as -> crc)[1] = (as -> crc)[2];
		(as -> crc)[1] ^= (byte >> 7);
		(as -> crc)[2] = (byte << 1); 
		(as -> crc)[1] ^= (byte >> 2);
		(as -> crc)[2] ^= (byte << 6);
		byte ^= (byte << 1);
		byte ^= (byte << 2);
		byte ^= (byte << 4);
		if (byte & 0x80) 
		{
			(as -> crc)[0] ^= 0x80;
		    (as -> crc)[2] ^= 0x21;
		}
	}
}

void lwasm_emitop(line_t *cl, int opc)
{
	if (opc > 0x100)
		lwasm_emit(cl, opc >> 8);
	lwasm_emit(cl, opc);
}

lw_expr_t lwasm_parse_term(char **p, void *priv)
{
	asmstate_t *as = priv;
	int neg = 1;
	int val;
	
	if (!**p)
		return NULL;
	
	if (**p == '*' || (
			**p == '.'
			&& !((*p)[1] >= 'A' && (*p)[1] <= 'Z')
			&& !((*p)[1] >= 'a' && (*p)[1] <= 'z')
			&& !((*p)[1] >= '0' && (*p)[1] <= '9')
		))
	{
		// special "symbol" for current line addr (*, .)
		(*p)++;
		return lw_expr_build(lw_expr_type_special, lwasm_expr_lineaddr, as -> cl);
	}
	
	// branch points
	if (**p == '<')
	{
		(*p)++;
		return lw_expr_build(lw_expr_type_special, lwasm_expr_prevbp, as -> cl);
	}
	if (**p == '>')
	{
		(*p)++;
		return lw_expr_build(lw_expr_type_special, lwasm_expr_nextbp, as -> cl);
	}
	
	// double ascii constant
	if (**p == '"')
	{
		int v;
		(*p)++;
		if (!**p)
			return NULL;
		if (!*((*p)+1))
			return NULL;
		v = (unsigned char)**p << 8 | (unsigned char)*((*p)+1);
		(*p) += 2;
		return lw_expr_build(lw_expr_type_int, v);
	}
	
	if (**p == '\'')
	{
		int v;
		
		(*p)++;
		if (!**p)
			return NULL;
		
		v = (unsigned char)**p;
		(*p)++;
		return lw_expr_build(lw_expr_type_int, v);
	}
	
	if (**p == '&')
	{
		// decimal constant
		int v = 0;
		(*p)++;
		
		if (**p == '-')
		{
			(*p)++;
			neg = -1;
		}

		if (!strchr("0123456789", **p))
			return NULL;

		while (**p && strchr("0123456789", **p))
		{
			val = val * 10 + (**p - '0');
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, val * neg);
	}

	if (**p == '%')
	{
		// binary constant
		(*p)++;

		if (**p == '-')
		{
			(*p)++;
			neg = -1;
		}

		if (**p != '0' && **p != '1')
			return NULL;

		while (**p && (**p == '0' || **p == '1'))
		{
			val = val * 2 + (**p - '0');
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, val * neg);
	}
	
	if (**p == '$')
	{
		// hexadecimal constant
		int v = 0, v2;
		(*p)++;
		if (**p == '-')
		{
			(*p)++;
			neg = -1;
		}

		if (!strchr("0123456789abcdefABCDEF", **p))
			return NULL;

		while (**p && strchr("0123456789abcdefABCDEF", **p))
		{
			v2 = toupper(**p) - '0';
			if (v2 > 9)
				v2 -= 7;
			v = v * 16 + v2;
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, v * neg);
	}
	
	if (**p == '0' && (*((*p)+1) == 'x' || *((*p)+1) == 'X'))
	{
		// hexadecimal constant, C style
		int v = 0, v2;
		(*p)+=2;

		if (!strchr("0123456789abcdefABCDEF", **p))
			return NULL;

		while (**p && strchr("0123456789abcdefABCDEF", **p))
		{
			v2 = toupper(**p) - '0';
			if (v2 > 9)
				v2 -= 7;
			v = v * 16 + v2;
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, v);
	}
	
	if (**p == '@' && (*((*p)+1) >= '0' && *((*p)+1) <= '7'))
	{
		// octal constant
		int v = 0;
		(*p)++;
		if (**p == '-')
		{
			(*p)++;
			neg = -1;
		}


		if (!strchr("01234567", **p))
			return NULL;

		while (**p && strchr("01234567", **p))
		{
			v = v * 8 + (**p - '0');
			(*p)++;
		}
		return lw_expr_build(lw_expr_type_int, v * neg);
	}
	

	// symbol or bare decimal or suffix constant here
	do
	{
		int havedol = 0;
		int l = 0;
		
		while ((*p)[l] && strchr(SYMCHARS, (*p)[l]))
		{
			if ((*p)[l] == '$')
				havedol = 1;
			l++;
		}
		if (l == 0)
			return NULL;

		if ((*p)[l] == '{')
		{
			while ((*p)[l] && (*p)[l] != '}')
				l++;
			l++;
		}
		
		if (havedol || **p < '0' || **p > '9')
		{
			// have a symbol here
			char *sym;
			lw_expr_t term;
			
			sym = lw_strndup(*p, l);
			(*p) += l;
			term = lw_expr_build(lw_expr_type_var, sym);
			lw_free(sym);
			return term;
		}
	} while (0);
	
	if (!**p)
		return NULL;
	
	// we have a numeric constant here, either decimal or postfix base notation
	{
		int decval = 0, binval = 0, hexval = 0, octval = 0;
		int valtype = 15; // 1 = bin, 2 = oct, 4 = dec, 8 = hex
		int bindone = 0;
		int val;
		int dval;
		
		while (1)
		{
			if (!**p || !strchr("0123456789ABCDEFabcdefqhoQHO", **p))
			{
				// we can legally be bin or decimal here
				if (bindone)
				{
					// just finished a binary value
					val = binval;
					break;
				}
				else if (valtype & 4)
				{
					val = decval;
					break;
				}
				else
				{
					// bad value
					return NULL;
				}
			}
			
			dval = toupper(**p);
			(*p)++;
			
			if (bindone)
			{
				// any characters past "B" means it is not binary
				bindone = 0;
				valtype &= 14;
			}
			
			switch (dval)
			{
			case 'Q':
			case 'O':
				if (valtype & 2)
				{
					val = octval;
					valtype = -1;
					break;
				}
				else
				{
					return NULL;
				}
				/* can't get here */
			
			case 'H':
				if (valtype & 8)
				{
					val = hexval;
					valtype = -1;
					break;
				}
				else
				{
					return NULL;
				}
				/* can't get here */
			
			case 'B':
				// this is a bit of a sticky one since B may be a
				// hex number instead of the end of a binary number
				// so it falls through to the digit case
				if (valtype & 1)
				{
					// could still be binary of hex
					bindone = 1;
					valtype = 9;
				}
				/* fall through intented */
			
			default:
				// digit
				dval -= '0';
				if (dval > 9)
					dval -= 7;
				if (valtype & 8)
					hexval = hexval * 16 + dval;
				if (valtype & 4)
				{
					if (dval > 9)
						valtype &= 11;
					else
						decval = decval * 10 + dval;
				}
				if (valtype & 2)
				{
					if (dval > 7)
						valtype &= 13;
					else
						octval = octval * 8 + dval;
				}
				if (valtype & 1)
				{
					if (dval > 1)
						valtype &= 14;
					else
						binval = binval * 2 + dval;
				}
			}
			if (valtype == -1)
				break;
			
			// return if no more valid types
			if (valtype == 0)
				return NULL;
			
			val = decval; // in case we fall through	
		} 
		
		// get here if we have a value
		return lw_expr_build(lw_expr_type_int, val);
	}
	// can't get here
}

lw_expr_t lwasm_parse_expr(asmstate_t *as, char **p)
{
	lw_expr_t e;
	
	e = lw_expr_parse(p, as);
	
	return e;
}

int lwasm_reduce_expr(asmstate_t *as, lw_expr_t expr)
{
	if (expr)
		lw_expr_simplify(expr, as);
	return 0;
}

void lwasm_save_expr(line_t *cl, int id, lw_expr_t expr)
{
	struct line_expr_s *e;
	
	for (e = cl -> exprs; e; e = e -> next)
	{
		if (e -> id == id)
		{
			lw_expr_destroy(e -> expr);
			e -> expr = expr;
			return;
		}
	}
	
	e = lw_alloc(sizeof(struct line_expr_s));
	e -> expr = expr;
	e -> id = id;
	e -> next = cl -> exprs;
	cl -> exprs = e;
}

lw_expr_t lwasm_fetch_expr(line_t *cl, int id)
{
	struct line_expr_s *e;
	
	for (e = cl -> exprs; e; e = e -> next)
	{
		if (e -> id == id)
		{
			return e -> expr;
		}
	}
	return NULL;
}

void skip_operand(char **p)
{
	for (; **p && !isspace(**p); (*p)++)
		/* do nothing */ ;
}

int lwasm_emitexpr(line_t *l, lw_expr_t expr, int size)
{
	int v = 0;
	int ol;
	
	ol = l -> outputl;
	if (ol == -1)
		ol = 0;
		
	if (lw_expr_istype(expr, lw_expr_type_int))
	{
		v = lw_expr_intval(expr);
	}
	// handle external/cross-section/incomplete references here
	else
	{
		if (l -> as -> output_format == OUTPUT_OBJ)
		{
			reloctab_t *re;
			lw_expr_t te;

			if (l -> csect == NULL)
			{
				lwasm_register_error(l -> as, l, "Instruction generating output outside of a section");
				return -1;
			}
			
			if (size == 4)
			{
				// create a two part reference because lwlink doesn't
				// support 32 bit references
				lw_expr_t te2;
				te = lw_expr_build(lw_expr_type_int, 0x10000);
				te2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_divide, expr, te);
				lw_expr_destroy(te);
				
				re = lw_alloc(sizeof(reloctab_t));
				re -> next = l -> csect -> reloctab;
				l -> csect -> reloctab = re;
				te = lw_expr_build(lw_expr_type_int, ol);
				re -> offset = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, l -> addr, te);
				lw_expr_destroy(te);
				lwasm_reduce_expr(l -> as, re -> offset);
				re -> expr = te2;
				re -> size = 2;

				te = lw_expr_build(lw_expr_type_int, 0xFFFF);
				te2 = lw_expr_build(lw_expr_type_oper, lw_expr_oper_bwand, expr, te);
				lw_expr_destroy(te);
				
				re = lw_alloc(sizeof(reloctab_t));
				re -> next = l -> csect -> reloctab;
				l -> csect -> reloctab = re;
				te = lw_expr_build(lw_expr_type_int, ol + 2);
				re -> offset = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, l -> addr, te);
				lw_expr_destroy(te);
				lwasm_reduce_expr(l -> as, re -> offset);
				re -> expr = te2;
				re -> size = 2;
			}
			else
			{
				// add "expression" record to section table
				re = lw_alloc(sizeof(reloctab_t));
				re -> next = l -> csect -> reloctab;
				l -> csect -> reloctab = re;
				te = lw_expr_build(lw_expr_type_int, ol);
				re -> offset = lw_expr_build(lw_expr_type_oper, lw_expr_oper_plus, l -> addr, te);
				lw_expr_destroy(te);
				lwasm_reduce_expr(l -> as, re -> offset);
				re -> size = size;
				re -> expr = lw_expr_copy(expr);
			}
			for (v = 0; v < size; v++)
				lwasm_emit(l, 0);
			return 0;
		}
		lwasm_register_error(l -> as, l, "Expression not fully resolved");
		return -1;
	}
	
	switch (size)
	{
	case 4:
		lwasm_emit(l, v >> 24);
		lwasm_emit(l, v >> 16);
		/* fallthrough intended */
			
	case 2:
		lwasm_emit(l, v >> 8);
		/* fallthrough intended */
		
	case 1:
		lwasm_emit(l, v);
	}
	
	return 0;
}

int lwasm_lookupreg2(const char *regs, char **p)
{
	int rval = 0;
	
	while (*regs)
	{
		if (toupper(**p) == *regs)
		{
			if (regs[1] == ' ' && !isalpha(*(*p + 1)))
				break;
			if (toupper(*(*p + 1)) == regs[1])
				break;
		}
		regs += 2;
		rval++;
	}
	if (!*regs)
		return -1;
	if (regs[1] == ' ')
		(*p)++;
	else
		(*p) += 2;
	return rval;
}

int lwasm_lookupreg3(const char *regs, char **p)
{
	int rval = 0;
	
	while (*regs)
	{
		if (toupper(**p) == *regs)
		{
			if (regs[1] == ' ' && !isalpha(*(*p + 1)))
				break;
			if (toupper(*(*p + 1)) == regs[1])
			{
				if (regs[2] == ' ' && !isalpha(*(*p + 2)))
					break;
				if (toupper(*(*p + 2)) == regs[2])
					break;
			}
		}
		regs += 3;
		rval++;
	}
	if (!*regs)
		return -1;
	if (regs[1] == ' ')
		(*p)++;
	else if (regs[2] == ' ')
		(*p) += 2;
	else
		(*p) += 3;
	return rval;
}

void lwasm_show_errors(asmstate_t *as)
{
	line_t *cl;
	lwasm_error_t *e;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		if (!(cl -> err) && !(cl -> warn))
			continue;
		for (e = cl -> err; e; e = e -> next)
		{
			fprintf(stderr, "ERROR: %s\n", e -> mess);
		}
		for (e = cl -> warn; e; e = e -> next)
		{
			fprintf(stderr, "WARNING: %s\n", e -> mess);
		}
		fprintf(stderr, "%s:%05d %s\n\n", cl -> linespec, cl -> lineno, cl -> ltext);
	}
}

/*
this does any passes and other gymnastics that might be useful
to see if an expression reduces early
*/
extern void do_pass3(asmstate_t *as);
extern void do_pass4_aux(asmstate_t *as, int force);

void lwasm_interim_reduce(asmstate_t *as)
{
	do_pass3(as);
//	do_pass4_aux(as, 0);
}

lw_expr_t lwasm_parse_cond(asmstate_t *as, char **p)
{
	lw_expr_t e;

	debug_message(as, 250, "Parsing condition");
	e = lwasm_parse_expr(as, p);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
	
	if (!e)
	{
		lwasm_register_error(as, as -> cl, "Bad expression");
		return NULL;
	}

	/* we need to simplify the expression here */
	debug_message(as, 250, "Doing interim reductions");
	lwasm_interim_reduce(as);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
	debug_message(as, 250, "Reducing expression");
	lwasm_reduce_expr(as, e);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
/*	lwasm_reduce_expr(as, e);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
	lwasm_reduce_expr(as, e);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
	lwasm_reduce_expr(as, e);
	debug_message(as, 250, "COND EXPR: %s", lw_expr_print(e));
*/

	lwasm_save_expr(as -> cl, 4242, e);

	if (!lw_expr_istype(e, lw_expr_type_int))
	{
		debug_message(as, 250, "Non-constant expression");
		lwasm_register_error(as, as -> cl, "Conditions must be constant on pass 1");
		return NULL;
	}
	debug_message(as, 250, "Returning expression");
	return e;
}
