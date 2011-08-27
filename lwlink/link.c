/*
link.c
Copyright © 2009 William Astle

This file is part of LWLINK.

LWLINK is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.


Resolve section and symbol addresses; handle incomplete references
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>

#include "expr.h"
#include "lwlink.h"

void check_os9(void);

struct section_list *sectlist = NULL;
int nsects = 0;
static int nforced = 0;

void check_section_name(char *name, int *base, fileinfo_t *fn)
{
	int sn;

	if (fn -> forced == 0)
		return;

	for (sn = 0; sn < fn -> nsections; sn++)
	{
		if (!strcmp(name, (char *)(fn -> sections[sn].name)))
		{
			// we have a match
			sectlist = lw_realloc(sectlist, sizeof(struct section_list) * (nsects + 1));
			sectlist[nsects].ptr = &(fn -> sections[sn]);
					
			fn -> sections[sn].processed = 1;
			fn -> sections[sn].loadaddress = *base;
			*base += fn -> sections[sn].codesize;
			nsects++;
		}
	}
	for (sn = 0; sn < fn -> nsubs; sn++)
	{
		check_section_name(name, base, fn -> subs[sn]);
	}
}

void add_matching_sections(char *name, int yesflags, int noflags, int *base);
void check_section_flags(int yesflags, int noflags, int *base, fileinfo_t *fn)
{
	int sn;

	if (fn -> forced == 0)
		return;

	for (sn = 0; sn < fn -> nsections; sn++)
	{
		// ignore if the noflags tell us to
		if (noflags && (fn -> sections[sn].flags & noflags))
			continue;
		// ignore unless the yesflags tell us not to
		if (yesflags && ((fn -> sections[sn].flags & yesflags) == 0))
			continue;
		// ignore it if already processed
		if (fn -> sections[sn].processed)
			continue;

		// we have a match - now collect *all* sections of the same name!
		add_matching_sections((char *)(fn -> sections[sn].name), 0, 0, base);
		
		// and then continue looking for sections
	}
	for (sn = 0; sn < fn -> nsubs; sn++)
	{
		check_section_flags(yesflags, noflags, base, fn -> subs[sn]);
	}
}



void add_matching_sections(char *name, int yesflags, int noflags, int *base)
{
	int fn;
	if (name)
	{
		// named section
		// look for all instances of a section by the specified name
		// and resolve base addresses and add to the list
		for (fn = 0; fn < ninputfiles; fn++)
		{
			check_section_name(name, base, inputfiles[fn]);
		}
	}
	else
	{
		// wildcard section
		// named section
		// look for all instances of a section by the specified name
		// and resolve base addresses and add to the list
		for (fn = 0; fn < ninputfiles; fn++)
		{
			check_section_flags(yesflags, noflags, base, inputfiles[fn]);
		}
	}
}

// work out section load order and resolve base addresses for each section
// make a list of sections to load in order
void resolve_sections(void)
{
	int laddr = 0;
	int ln, sn, fn;
	
	for (ln = 0; ln < linkscript.nlines; ln++)
	{
		if (linkscript.lines[ln].loadat >= 0)
			laddr = linkscript.lines[ln].loadat;
		add_matching_sections(linkscript.lines[ln].sectname, linkscript.lines[ln].yesflags, linkscript.lines[ln].noflags, &laddr);
		
		if (linkscript.lines[ln].sectname)
		{
		}
		else
		{
			// wildcard section
			// look for all sections not yet processed that match flags

			int f = 0;
			int fn0, sn0;
			char *sname;
			
			// named section
			// look for all instances of a section by the specified name
			// and resolve base addresses and add to the list
			for (fn0 = 0; fn0 < ninputfiles; fn0++)
			{
				for (sn0 = 0; sn0 < inputfiles[fn0] -> nsections; sn0++)
				{
					// ignore if the "no flags" bit says to
					if (linkscript.lines[ln].noflags && (inputfiles[fn0] -> sections[sn0].flags & linkscript.lines[ln].noflags))
						continue;
					// ignore unless the yes flags tell us not to
					if (linkscript.lines[ln].yesflags && ((inputfiles[fn0] -> sections[sn0].flags & linkscript.lines[ln].yesflags) == 0))
						continue;
					if (inputfiles[fn0] -> sections[sn0].processed == 0)
					{
						sname = (char *)(inputfiles[fn0] -> sections[sn0].name);
						for (fn = 0; fn < ninputfiles; fn++)
						{
							for (sn = 0; sn < inputfiles[fn] -> nsections; sn++)
							{
								if (!strcmp(sname, (char *)(inputfiles[fn] -> sections[sn].name)))
								{
									// we have a match
									sectlist = lw_realloc(sectlist, sizeof(struct section_list) * (nsects + 1));
									sectlist[nsects].ptr = &(inputfiles[fn] -> sections[sn]);
						
									inputfiles[fn] -> sections[sn].processed = 1;
									if (!f && linkscript.lines[ln].loadat >= 0)
									{
										f = 1;
										sectlist[nsects].forceaddr = 1;
										laddr = linkscript.lines[ln].loadat;
									}
									else
									{
										sectlist[nsects].forceaddr = 0;
									}
									inputfiles[fn] -> sections[sn].loadaddress = laddr;
									laddr += inputfiles[fn] -> sections[sn].codesize;
									nsects++;
								}
							}
						}
					}
				}
			}
		}
	}
	
	// theoretically, all the base addresses are set now
}

lw_expr_stack_t *find_external_sym_recurse(char *sym, fileinfo_t *fn)
{
	int sn;
	lw_expr_stack_t *r;
	lw_expr_term_t *term;
	symtab_t *se;
	int val;
	
	for (sn = 0; sn < fn -> nsections; sn++)
	{
		for (se = fn -> sections[sn].exportedsyms; se; se = se -> next)
		{
			if (!strcmp(sym, (char *)(se -> sym)))
			{
				if (!(fn -> forced))
				{
					fn -> forced = 1;
					nforced = 1;
				}
				val = se -> offset + fn -> sections[sn].loadaddress;
				r = lw_expr_stack_create();
				term = lw_expr_term_create_int(val & 0xffff);
				lw_expr_stack_push(r, term);
				lw_expr_term_free(term);
				return r;
			}
		}
	}
		
	for (sn = 0; sn < fn -> nsubs; sn++)
	{
		r = find_external_sym_recurse(sym, fn -> subs[sn]);
		if (r)
		{
			if (!(fn -> forced))
			{
				nforced = 1;
				fn -> forced = 1;
			}
			return r;
		}
	}
	return NULL;
}

// resolve all incomplete references now
// anything that is unresolvable at this stage will throw an error
// because we know the load address of every section now
lw_expr_stack_t *resolve_sym(char *sym, int symtype, void *state)
{
	section_t *sect = state;
	lw_expr_term_t *term;
	int val = 0, i, fn;
	lw_expr_stack_t *s;
	symtab_t *se;
	fileinfo_t *fp;

	if (symtype == 1)
	{
		// local symbol
		if (!sym)
		{
			val = sect -> loadaddress;
			goto out;
		}
		
		// start with this section
		for (se = sect -> localsyms; se; se = se -> next)
		{
			if (!strcmp((char *)(se -> sym), sym))
			{
				val = se -> offset + sect -> loadaddress;
				goto out;
			}
		}
		// not in this section - check all sections in this file
		for (i = 0; i < sect -> file -> nsections; i++)
		{
			for (se = sect -> file -> sections[i].localsyms; se; se = se -> next)
			{
				if (!strcmp((char *)(se -> sym), sym))
				{
					val = se -> offset + sect -> file -> sections[i].loadaddress;
					goto out;
				}
			}
		}
		// not found
		symerr = 1;
		fprintf(stderr, "Local symbol %s not found in %s:%s\n", sanitize_symbol(sym), sect -> file -> filename, sect -> name);
		goto outerr;
	}
	else
	{
		// external symbol
		// read all files in order until found (or not found)
		if (sect)
		{
			for (fp = sect -> file; fp; fp = fp -> parent)
			{
				s = find_external_sym_recurse(sym, fp);
				if (s)
					return s;
			}
		}

		for (fn = 0; fn < ninputfiles; fn++)
		{
			s = find_external_sym_recurse(sym, inputfiles[fn]);
			if (s)
				return s;
		}
		if (sect)
		{
			fprintf(stderr, "External symbol %s not found in %s:%s\n", sanitize_symbol(sym), sect -> file -> filename, sect -> name);
		}
		else
		{
			fprintf(stderr, "External symbol %s not found\n", sym);
		}
		symerr = 1;
		goto outerr;
	}
	fprintf(stderr, "Shouldn't ever get here!!!\n");
	exit(88);
out:
	s = lw_expr_stack_create();
	term = lw_expr_term_create_int(val & 0xffff);
	lw_expr_stack_push(s, term);
	lw_expr_term_free(term);
	return s;
outerr:
	return NULL;
}

void resolve_references(void)
{
	int sn;
	reloc_t *rl;
	int rval;

	// resolve entry point if required
	// this must resolve to an *exported* symbol and will resolve to the
	// first instance of that symbol
	if (linkscript.execsym)
	{
		lw_expr_stack_t *s;
		
		s = resolve_sym(linkscript.execsym, 0, NULL);
		if (!s)
		{
			fprintf(stderr, "Cannot resolve exec address '%s'\n", linkscript.execsym);
			symerr = 1;
		}
		else
		{
			linkscript.execaddr = lw_expr_get_value(s);
			lw_expr_stack_free(s);
		}
	}
	
	for (sn = 0; sn < nsects; sn++)
	{
		for (rl = sectlist[sn].ptr -> incompletes; rl; rl = rl -> next)
		{
			// do a "simplify" on the expression
			rval = lw_expr_reval(rl -> expr, resolve_sym, sectlist[sn].ptr);

			// is it constant? error out if not
			if (rval != 0 || !lw_expr_is_constant(rl -> expr))
			{
				fprintf(stderr, "Incomplete reference at %s:%s+%02X\n", sectlist[sn].ptr -> file -> filename, sectlist[sn].ptr -> name, rl -> offset);
				symerr = 1;
			}
			else
			{
				// put the value into the relocation address
				rval = lw_expr_get_value(rl -> expr);
				if (rl -> flags & RELOC_8BIT)
				{
					sectlist[sn].ptr -> code[rl -> offset] = rval & 0xff;
				}
				else
				{
					sectlist[sn].ptr -> code[rl -> offset] = (rval >> 8) & 0xff;
					sectlist[sn].ptr -> code[rl -> offset + 1] = rval & 0xff;
				}
			}
		}
	}
	
	if (symerr)
		exit(1);
	
	if (outformat == OUTPUT_OS9)
		check_os9();	
}

/*
This is just a pared down version of the algo for resolving references.
*/
void resolve_files(void)
{
	int sn;
	int fn;
	reloc_t *rl;
	lw_expr_stack_t *te;
	
	int rval;

	// resolve entry point if required
	// this must resolve to an *exported* symbol and will resolve to the
	// first instance of that symbol
	if (linkscript.execsym)
	{
		lw_expr_stack_t *s;
		
		s = resolve_sym(linkscript.execsym, 0, NULL);
		if (!s)
		{
			fprintf(stderr, "Cannot resolve exec address '%s'\n", sanitize_symbol(linkscript.execsym));
			symerr = 1;
		}
	}
	
	do
	{
		nforced = 0;
		for (fn = 0; fn < ninputfiles; fn++)
		{
			if (inputfiles[fn] -> forced == 0)
				continue;
	
			for (sn = 0; sn < inputfiles[fn] -> nsections; sn++)
			{
				for (rl = inputfiles[fn] -> sections[sn].incompletes; rl; rl = rl -> next)
				{
					// do a "simplify" on the expression
					te = lw_expr_stack_dup(rl -> expr);
					rval = lw_expr_reval(te, resolve_sym, &(inputfiles[fn] -> sections[sn]));
					
					// is it constant? error out if not
					if (rval != 0 || !lw_expr_is_constant(te))
					{
						fprintf(stderr, "Incomplete reference at %s:%s+%02X\n", inputfiles[fn] -> filename, inputfiles[fn] -> sections[sn].name, rl -> offset);
						symerr = 1;
					}
					lw_expr_stack_free(te);
				}
			}
		}
	}
	while (nforced == 1);

	if (symerr)
		exit(1);
	
	// theoretically, all files referenced by other files now have "forced" set to 1
	for (fn = 0; fn < ninputfiles; fn++)
	{
		if (inputfiles[fn] -> forced == 1)
			continue;
		
		fprintf(stderr, "Warning: %s (%d) does not resolve any symbols\n", inputfiles[fn] -> filename, fn);
	}
}
void find_section_by_name_once_aux(char *name, fileinfo_t *fn, section_t **rval, int *found);
void find_section_by_name_once_aux(char *name, fileinfo_t *fn, section_t **rval, int *found)
{
	int sn;
	
	if (fn -> forced == 0)
		return;

	for (sn = 0; sn < fn -> nsections; sn++)
	{
		if (!strcmp(name, (char *)(fn -> sections[sn].name)))
		{
			(*found)++;
			*rval = &(fn -> sections[sn]);
		}
	}
	for (sn = 0; sn < fn -> nsubs; sn++)
	{
		find_section_by_name_once_aux(name, fn -> subs[sn], rval, found);
	}
}

section_t *find_section_by_name_once(char *name)
{
	int fn;
	int found = 0;
	section_t *rval = NULL;
	
	for (fn = 0; fn < ninputfiles; fn++)
	{
		find_section_by_name_once_aux(name, inputfiles[fn], &rval, &found);
	}
	if (found != 1)
	{
		rval = NULL;
		fprintf(stderr, "Warning: multiple instances of section %s found; ignoring all of them which is probably not what you want\n", name);
	}
	return rval;
}

void check_os9(void)
{
	section_t *s;
	symtab_t *sym;
	
	s = find_section_by_name_once("__os9");
	linkscript.name = outfile;
	
	if (!s)
	{
		// use defaults if not found
		return;
	}	

	// this section is special
	// several symbols may be defined as LOCAL symbols:
	// type: module type
	// lang: module language
	// attr: module attributes
	// rev: module revision
	//
	// the symbols are not case sensitive
	//
	// any unrecognized symbols are ignored
	
	// the contents of the actual data to the first NUL
	// is assumed to be the module name unless it is an empty string
	if (s -> codesize > 0 && (s -> flags & SECTION_BSS) == 0 && s -> code[0] != 0)
	{
		linkscript.name = (char *)(s -> code);
	}
	
	for (sym = s -> localsyms; s; sym = sym -> next)
	{
		char *sm = (char *)(sym -> sym);
		if (!strcasecmp(sm, "type"))
		{
			linkscript.modtype = sym -> offset;
		}
		else if (!strcasecmp(sm, "lang"))
		{
			linkscript.modlang = sym -> offset;
		}
		else if (!strcasecmp(sm, "attr"))
		{
			linkscript.modlang = sym -> offset;
		}
		else if (!strcasecmp(sm, "rev"))
		{
			linkscript.modrev = sym -> offset;
		}
	}
	if (linkscript.modtype > 15)
		linkscript.modtype = linkscript.modtype >> 4;
	
	if (linkscript.modattr > 15)
		linkscript.modattr = linkscript.modattr >> 4;
	
	linkscript.modlang &= 15;
	linkscript.modtype &= 15;
	linkscript.modrev &= 15;
	linkscript.modattr &= 15;
}
