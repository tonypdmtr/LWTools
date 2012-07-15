/*
output.c
Copyright © 2009, 2010 William Astle

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


Contains the code for actually outputting the assembled code
*/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <lw_alloc.h>
#include <lw_expr.h>

#include "lwasm.h"

void write_code_raw(asmstate_t *as, FILE *of);
void write_code_decb(asmstate_t *as, FILE *of);
void write_code_rawrel(asmstate_t *as, FILE *of);
void write_code_obj(asmstate_t *as, FILE *of);
void write_code_os9(asmstate_t *as, FILE *of);

// this prevents warnings about not using the return value of fwrite()
// r++ prevents the "set but not used" warnings; should be optimized out
#define writebytes(s, l, c, f)	do { int r; r = fwrite((s), (l), (c), (f)); r++; } while (0)

void do_output(asmstate_t *as)
{
	FILE *of;
	
	if (as -> errorcount > 0)
	{
		fprintf(stderr, "Not doing output due to assembly errors.\n");
		return;
	}
	
	of = fopen(as -> output_file, "wb");
	if (!of)
	{
		fprintf(stderr, "Cannot open '%s' for output", as -> output_file);
		perror("");
		return;
	}

	switch (as -> output_format)
	{
	case OUTPUT_RAW:
		write_code_raw(as, of);
		break;
		
	case OUTPUT_DECB:
		write_code_decb(as, of);
		break;
		
	case OUTPUT_RAWREL:
		write_code_rawrel(as, of);
		break;
	
	case OUTPUT_OBJ:
		write_code_obj(as, of);
		break;

	case OUTPUT_OS9:
		write_code_os9(as, of);
		break;

	default:
		fprintf(stderr, "BUG: unrecognized output format when generating output file\n");
		fclose(of);
		unlink(as -> output_file);
		return;
	}

	fclose(of);
}

/*
rawrel output treats an ORG directive as an offset from the start of the
file. Undefined results will occur if an ORG directive moves the output
pointer backward. This particular implementation uses "fseek" to handle
ORG requests and to skip over RMBs.

This simple brain damanged method simply does an fseek before outputting
each instruction.
*/
void write_code_rawrel(asmstate_t *as, FILE *of)
{
	line_t *cl;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		if (cl -> outputl <= 0)
			continue;
		
		fseek(of, lw_expr_intval(cl -> addr), SEEK_SET);
		writebytes(cl -> output, cl -> outputl, 1, of);
	}
}

/*
raw merely writes all the bytes directly to the file as is. ORG is just a
reference for the assembler to handle absolute references. Multiple ORG
statements will produce mostly useless results
*/
void write_code_raw(asmstate_t *as, FILE *of)
{
	line_t *cl;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		if (cl -> len > 0 && cl -> outputl == 0)
		{
			int i;
			for (i = 0; i < cl -> len; i++)
				writebytes("\0", 1, 1, of);
			continue;
		}
		else if (cl -> outputl > 0)
			writebytes(cl -> output, cl -> outputl, 1, of);
	}
}


/*
OS9 target also just writes all the bytes in order. No need for anything
else.
*/
void write_code_os9(asmstate_t *as, FILE *of)
{
	line_t *cl;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
//		if (cl -> inmod == 0)
//			continue;
		if (cl -> len > 0 && cl -> outputl == 0)
		{
			int i;
			for (i = 0; i < cl -> len; i++)
				writebytes("\0", 1, 1, of);
			continue;
		}
		else if (cl -> outputl > 0)
			writebytes(cl -> output, cl -> outputl, 1, of);
	}
}

void write_code_decb(asmstate_t *as, FILE *of)
{
	long preambloc;
	line_t *cl;
	int blocklen = -1;
	int nextcalc = -1;
	unsigned char outbuf[5];
	int caddr;
	
	for (cl = as -> line_head; cl; cl = cl -> next)
	{
		if (cl -> outputl < 0)
			continue;
		caddr = lw_expr_intval(cl -> addr);
		if (caddr != nextcalc && cl -> outputl > 0)
		{
			// need preamble here
			if (blocklen > 0)
			{
				// update previous preamble if needed
				fseek(of, preambloc, SEEK_SET);
				outbuf[0] = (blocklen >> 8) & 0xFF;
				outbuf[1] = blocklen & 0xFF;
				writebytes(outbuf, 2, 1, of);
				fseek(of, 0, SEEK_END);
			}
			blocklen = 0;
			nextcalc = caddr;
			outbuf[0] = 0x00;
			outbuf[1] = 0x00;
			outbuf[2] = 0x00;
			outbuf[3] = (nextcalc >> 8) & 0xFF;
			outbuf[4] = nextcalc & 0xFF;
			preambloc = ftell(of) + 1;
			writebytes(outbuf, 5, 1, of);
		}
		nextcalc += cl -> outputl;
		writebytes(cl -> output, cl -> outputl, 1, of);
		blocklen += cl -> outputl;
	}
	if (blocklen > 0)
	{
		fseek(of, preambloc, SEEK_SET);
		outbuf[0] = (blocklen >> 8) & 0xFF;
		outbuf[1] = blocklen & 0xFF;
		writebytes(outbuf, 2, 1, of);
		fseek(of, 0, SEEK_END);
	}
	
	// now write postamble
	outbuf[0] = 0xFF;
	outbuf[1] = 0x00;
	outbuf[2] = 0x00;
	outbuf[3] = (as -> execaddr >> 8) & 0xFF;
	outbuf[4] = (as -> execaddr) & 0xFF;
	writebytes(outbuf, 5, 1, of);
}

void write_code_obj_sbadd(sectiontab_t *s, unsigned char b)
{
	if (s -> oblen >= s -> obsize)
	{
		s -> obytes = lw_realloc(s -> obytes, s -> obsize + 128);
		s -> obsize += 128;
	}
	s -> obytes[s -> oblen] = b;
	s -> oblen += 1;
}


int write_code_obj_expraux(lw_expr_t e, void *of)
{
	int tt;
	int v;
	int count = 1;
	unsigned char buf[16];
	
	tt = lw_expr_type(e);

	switch (tt)
	{
	case lw_expr_type_oper:
		buf[0] =  0x04;
		
		switch (lw_expr_whichop(e))
		{
		case lw_expr_oper_plus:
			buf[1] = 0x01;
			count = lw_expr_operandcount(e) - 1;
			break;
		
		case lw_expr_oper_minus:
			buf[1] = 0x02;
			break;
		
		case lw_expr_oper_times:
			buf[1] = 0x03;
			count = lw_expr_operandcount(e) - 1;
			break;
		
		case lw_expr_oper_divide:
			buf[1] = 0x04;
			break;
		
		case lw_expr_oper_mod:
			buf[1] = 0x05;
			break;
		
		case lw_expr_oper_intdiv:
			buf[1] = 0x06;
			break;
		
		case lw_expr_oper_bwand:
			buf[1] = 0x07;
			break;
		
		case lw_expr_oper_bwor:
			buf[1] = 0x08;
			break;
		
		case lw_expr_oper_bwxor:
			buf[1] = 0x09;
			break;
		
		case lw_expr_oper_and:
			buf[1] = 0x0A;
			break;
		
		case lw_expr_oper_or:
			buf[1] = 0x0B;
			break;
		
		case lw_expr_oper_neg:
			buf[1] = 0x0C;
			break;
		
		case lw_expr_oper_com:
			buf[1] = 0x0D;
			break;

		default:
			buf[1] = 0xff;
		}
		while (count--)
			writebytes(buf, 2, 1, of);
		return 0;

	case lw_expr_type_int:
		v = lw_expr_intval(e);
		buf[0] = 0x01;
		buf[1] = (v >> 8) & 0xff;
		buf[2] = v & 0xff;
		writebytes(buf, 3, 1, of);
		return 0;
		
	case lw_expr_type_special:
		v = lw_expr_specint(e);
		switch (v)
		{
		case lwasm_expr_secbase:
			{
				// replaced with a synthetic symbol
				sectiontab_t *se;
				se = lw_expr_specptr(e);
				
				writebytes("\x03\x02", 2, 1, of);
				writebytes(se -> name, strlen(se -> name) + 1, 1, of);
				return 0;
			}	
		case lwasm_expr_import:
			{
				importlist_t *ie;
				ie = lw_expr_specptr(e);
				buf[0] = 0x02;
				writebytes(buf, 1, 1, of);
				writebytes(ie -> symbol, strlen(ie -> symbol) + 1, 1, of);
				return 0;
			}
		case lwasm_expr_syment:
			{
				struct symtabe *se;
				se = lw_expr_specptr(e);
				buf[0] = 0x03;
				writebytes(buf, 1, 1, of);
				writebytes(se -> symbol, strlen(se -> symbol), 1, of);
				if (se -> context != -1)
				{
					sprintf((char *)buf, "\x01%d", se -> context);
					writebytes(buf, strlen((char *)buf), 1, of);
				}
				writebytes("", 1, 1, of);
				return 0;
			}
		}
			
	default:
		// unrecognized term type - replace with integer 0
//		fprintf(stderr, "Unrecognized term type: %s\n", lw_expr_print(e));
		buf[0] = 0x01;
		buf[1] = 0x00;
		buf[2] = 0x00;
		writebytes(buf, 3, 1, of);
		break;
	}
	return 0;
}

void write_code_obj_auxsym(asmstate_t *as, FILE *of, sectiontab_t *s, struct symtabe *se2)
{
	struct symtabe *se;
	unsigned char buf[16];
		
	if (!se2)
		return;
	write_code_obj_auxsym(as, of, s, se2 -> left);
	
	for (se = se2; se; se = se -> nextver)
	{
		lw_expr_t te;
		
		debug_message(as, 200, "Consider symbol %s (%p) for export in section %p", se -> symbol, se -> section, s);
		
		// ignore symbols not in this section
		if (se -> section != s)
			continue;
		
		debug_message(as, 200, "  In section");
			
		if (se -> flags & symbol_flag_set)
			continue;
			
		debug_message(as, 200, "  Not symbol_flag_set");
			
		te = lw_expr_copy(se -> value);
		debug_message(as, 200, "  Value=%s", lw_expr_print(te));
		as -> exportcheck = 1;
		as -> csect = s;
		lwasm_reduce_expr(as, te);
		as -> exportcheck = 0;

		debug_message(as, 200, "  Value2=%s", lw_expr_print(te));
			
		// don't output non-constant symbols
		if (!lw_expr_istype(te, lw_expr_type_int))
		{
			lw_expr_destroy(te);
			continue;
		}

		writebytes(se -> symbol, strlen(se -> symbol), 1, of);
		if (se -> context >= 0)
		{
			writebytes("\x01", 1, 1, of);
			sprintf((char *)buf, "%d", se -> context);
			writebytes(buf, strlen((char *)buf), 1, of);
		}
		// the "" is NOT an error
		writebytes("", 1, 1, of);
			
		// write the address
		buf[0] = (lw_expr_intval(te) >> 8) & 0xff;
		buf[1] = lw_expr_intval(te) & 0xff;
		writebytes(buf, 2, 1, of);
		lw_expr_destroy(te);
	}
	write_code_obj_auxsym(as, of, s, se2 -> right);
}

void write_code_obj(asmstate_t *as, FILE *of)
{
	line_t *l;
	sectiontab_t *s;
	reloctab_t *re;
	exportlist_t *ex;

	int i;
	unsigned char buf[16];

	// output the magic number and file header
	// the 8 is NOT an error
	writebytes("LWOBJ16", 8, 1, of);
	
	// run through the entire system and build the byte streams for each
	// section; at the same time, generate a list of "local" symbols to
	// output for each section
	// NOTE: for "local" symbols, we will append \x01 and the ascii string
	// of the context identifier (so sym in context 1 would be "sym\x011"
	// we can do this because the linker can handle symbols with any
	// character other than NUL.
	// also we will generate a list of incomplete references for each
	// section along with the actual definition that will be output
	
	// once all this information is generated, we will output each section
	// to the file
	
	// NOTE: we build everything in memory then output it because the
	// assembler accepts multiple instances of the same section but the
	// linker expects only one instance of each section in the object file
	// so we need to collect all the various pieces of a section together
	// (also, the assembler treated multiple instances of the same section
	// as continuations of previous sections so we would need to collect
	// them together anyway.
	
	for (l = as -> line_head; l; l = l -> next)
	{
		if (l -> csect)
		{
			// we're in a section - need to output some bytes
			if (l -> outputl > 0)
				for (i = 0; i < l -> outputl; i++)
					write_code_obj_sbadd(l -> csect, l -> output[i]);
			else if (l -> outputl == 0 || l -> outputl == -1)
				for (i = 0; i < l -> len; i++)
					write_code_obj_sbadd(l -> csect, 0);
		}
	}
	
	// run through the sections
	for (s = as -> sections; s; s = s -> next)
	{
		// write the name
		writebytes(s -> name, strlen(s -> name) + 1, 1, of);
		
		// write the flags
		if (s -> flags & section_flag_bss)
			writebytes("\x01", 1, 1, of);
		if (s -> flags & section_flag_constant)
			writebytes("\x02", 1, 1, of);
			
		// indicate end of flags - the "" is NOT an error
		writebytes("", 1, 1, of);
		
		// now the local symbols
		
		// a symbol for section base address
		if ((s -> flags & section_flag_constant) == 0)
		{
			writebytes("\x02", 1, 1, of);
			writebytes(s -> name, strlen(s -> name) + 1, 1, of);
			// address 0; "\0" is not an error
			writebytes("\0", 2, 1, of);
		}
		
		write_code_obj_auxsym(as, of, s, as -> symtab.head);
		// flag end of local symbol table - "" is NOT an error
		writebytes("", 1, 1, of);
		
		// now the exports -- FIXME
		for (ex = as -> exportlist; ex; ex = ex -> next)
		{
			int eval;
			lw_expr_t te;
			line_t tl;
			
			if (ex -> se == NULL)
				continue;
			if (ex -> se -> section != s)
				continue;
			te = lw_expr_copy(ex -> se -> value);
			as -> csect = ex -> se -> section;
			as -> exportcheck = 1;
			tl.as = as;
			as -> cl = &tl;
			lwasm_reduce_expr(as, te);
			as -> exportcheck = 0;
			as -> cl = NULL;
			if (!lw_expr_istype(te, lw_expr_type_int))
			{
				lw_expr_destroy(te);
				continue;
			}
			eval = lw_expr_intval(te);
			lw_expr_destroy(te);
			writebytes(ex -> symbol, strlen(ex -> symbol) + 1, 1, of);
			buf[0] = (eval >> 8) & 0xff;
			buf[1] = eval & 0xff;
			writebytes(buf, 2, 1, of);
		}
	
		// flag end of exported symbols - "" is NOT an error
		writebytes("", 1, 1, of);
		
		// FIXME - relocation table
		for (re = s -> reloctab; re; re = re -> next)
		{
			int offset;
			lw_expr_t te;
			line_t tl;
			
			tl.as = as;
			as -> cl = &tl;
			as -> csect = s;
			as -> exportcheck = 1;
			
			if (re -> expr == NULL)
			{
				// this is an error but we'll simply ignore it
				// and not output this expression
				continue;
			}
			
			// work through each term in the expression and output
			// the proper equivalent to the object file
			if (re -> size == 1)
			{
				// flag an 8 bit relocation (low 8 bits will be used)
				buf[0] = 0xFF;
				buf[1] = 0x01;
				writebytes(buf, 2, 1, of);
			}
			
			te = lw_expr_copy(re -> offset);
			lwasm_reduce_expr(as, te);
			if (!lw_expr_istype(te, lw_expr_type_int))
			{
				lw_expr_destroy(te);
				continue;
			}
			offset = lw_expr_intval(te);
			lw_expr_destroy(te);
			
			// output expression
			lw_expr_testterms(re -> expr, write_code_obj_expraux, of);
			
			// flag end of expressions
			writebytes("", 1, 1, of);
			
			// write the offset
			buf[0] = (offset >> 8) & 0xff;
			buf[1] = offset & 0xff;
			writebytes(buf, 2, 1, of);
		}

		// flag end of incomplete references list
		writebytes("", 1, 1, of);
		
		// now blast out the code
		
		// length
		if (s -> flags & section_flag_constant)
		{
			buf[0] = 0;
			buf[1] = 0;
		}
		else
		{
			buf[0] = s -> oblen >> 8 & 0xff;
			buf[1] = s -> oblen & 0xff;
		}
		writebytes(buf, 2, 1, of);
		
		
		if (!(s -> flags & section_flag_bss) && !(s -> flags & section_flag_constant))
		{
			writebytes(s -> obytes, s -> oblen, 1, of);
		}
	}
	
	// flag no more sections
	// the "" is NOT an error
	writebytes("", 1, 1, of);
}
