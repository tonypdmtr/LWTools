/*
instab.h
Copyright © 2008 William Astle

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


Contains definitions for the instruction table
*/

#ifndef __instab_h_seen__
#define __instab_h_seen__

#include "lwasm.h"

typedef struct
{
	char *opcode;				/* the mneumonic */
	int ops[4];					/* opcode values for up to four addr modes */
	void (*parse)(asmstate_t *as, line_t *l, char **optr);	/* parse operand for insn */
	void (*resolve)(asmstate_t *as, line_t *l, int force);				/* resolve instruction to code */
	void (*emit)(asmstate_t *as, line_t *l);				/* resolve instruction to code */
	int flags;					/* flag for this instruction */
} instab_t;

enum
{
	lwasm_insn_cond = 1,		/* conditional instruction */
	lwasm_insn_endm = 2,		/* end of macro */
	lwasm_insn_setsym = 4,		/* insn sets symbol address */
	lwasm_insn_is6309 = 8,		/* insn is 6309 only */
	lwasm_insn_struct = 16,		/* insn allowed in a struct */
	lwasm_insn_setdata = 32,	/* insn uses the data address for symbols */
	lwasm_insn_normal = 0
};


#define PARSEFUNC(fn)	void (fn)(asmstate_t *as, line_t *l, char **p)
#define RESOLVEFUNC(fn)	void (fn)(asmstate_t *as, line_t *l, int force)
#define EMITFUNC(fn)	void (fn)(asmstate_t *as, line_t *l)

#ifndef __instab_c_seen__
extern instab_t instab[];
#endif //__instab_c_seen__

#endif //__instab_h_seen__
