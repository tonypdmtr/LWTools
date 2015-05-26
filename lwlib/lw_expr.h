/*
lwexpr.h

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

#ifndef ___lw_expr_h_seen___
#define ___lw_expr_h_seen___

#include <stdio.h>

#ifdef _MSC_VER
#include "lw_win.h"	// windows build
#endif

enum
{
	lw_expr_type_oper,			// operator term
	lw_expr_type_int,			// integer
	lw_expr_type_var,			// a "variable" (string for the name)
	lw_expr_type_special		// a "special" reference (user defined)
};

enum
{
	lw_expr_oper_plus = 1,		// addition
	lw_expr_oper_minus,			// subtraction
	lw_expr_oper_times,			// multiplication
	lw_expr_oper_divide,		// division
	lw_expr_oper_mod,			// modulus
	lw_expr_oper_intdiv,		// integer division
	lw_expr_oper_bwand,			// bitwise and
	lw_expr_oper_bwor,			// bitwise or
	lw_expr_oper_bwxor,			// bitwise xor
	lw_expr_oper_and,			// boolean and
	lw_expr_oper_or,			// boolean or
	lw_expr_oper_neg,			// unary negation, 2's complement
	lw_expr_oper_com,			// unary 1's complement
	lw_expr_oper_none = 0
};

#ifdef ___lw_expr_c_seen___

typedef struct lw_expr_priv * lw_expr_t;

struct lw_expr_opers
{
	lw_expr_t p;
	struct lw_expr_opers *next;
};

struct lw_expr_priv
{
	int type;							// type of term
	int value;							// integer value
	void *value2;						// misc pointer value
	struct lw_expr_opers *operands;		// ptr to list of operands (for operators)
};

typedef lw_expr_t lw_expr_fn_t(int t, void *ptr, void *priv);
typedef lw_expr_t lw_expr_fn2_t(char *var, void *priv);
typedef lw_expr_t lw_expr_fn3_t(char **p, void *priv);
typedef int lw_expr_testfn_t(lw_expr_t e, void *priv);

#else /* def ___lw_expr_c_seen___ */

typedef void * lw_expr_t;

extern lw_expr_t lwexpr_create(void);
extern void lw_expr_destroy(lw_expr_t E);
extern lw_expr_t lw_expr_copy(lw_expr_t E);
extern void lw_expr_add_operand(lw_expr_t E, lw_expr_t O);
extern lw_expr_t lw_expr_build(int exprtype, ...);
extern char *lw_expr_print(lw_expr_t E);
extern int lw_expr_compare(lw_expr_t E1, lw_expr_t E2);
extern void lw_expr_simplify(lw_expr_t E, void *priv);

typedef lw_expr_t lw_expr_fn_t(int t, void *ptr, void *priv);
typedef lw_expr_t lw_expr_fn2_t(char *var, void *priv);
typedef lw_expr_t lw_expr_fn3_t(char **p, void *priv);

extern void lw_expr_set_special_handler(lw_expr_fn_t *fn);
extern void lw_expr_set_var_handler(lw_expr_fn2_t *fn);
extern void lw_expr_set_term_parser(lw_expr_fn3_t *fn);

extern lw_expr_t lw_expr_parse(char **p, void *priv);
extern int lw_expr_istype(lw_expr_t e, int t);
extern int lw_expr_intval(lw_expr_t e);
extern int lw_expr_specint(lw_expr_t e);
extern void *lw_expr_specptr(lw_expr_t e);
extern int lw_expr_whichop(lw_expr_t e);

extern int lw_expr_type(lw_expr_t e);

typedef int lw_expr_testfn_t(lw_expr_t e, void *priv);

extern int lw_expr_operandcount(lw_expr_t e);

// run a function on all terms in an expression; if the function
// returns non-zero for any term, return non-zero, else return
// zero
extern int lw_expr_testterms(lw_expr_t e, lw_expr_testfn_t *fn, void *priv);

extern void lw_expr_setdivzero(void (*fn)(void *priv));

#endif /* def ___lw_expr_c_seen___ */

#endif /* ___lw_expr_h_seen___ */
