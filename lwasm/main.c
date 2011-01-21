/*
main.c

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

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>
#include <lw_stringlist.h>
#include <lw_expr.h>

#include "lwasm.h"
#include "input.h"

extern int parse_pragma_string(asmstate_t *as, char *str, int ignoreerr);

/* command line option handling */
const char *argp_program_version = "lwasm from " PACKAGE_STRING;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;
char *program_name;

static struct argp_option options[] =
{
	{ "output",		'o',	"FILE",		0,						"Output to FILE"},
	{ "debug",		'd',	"LEVEL",	OPTION_ARG_OPTIONAL,	"Set debug mode"},
	{ "format",		'f',	"TYPE",		0,						"Select output format: decb, raw, obj, os9"},
	{ "list",		'l',	"FILE",		OPTION_ARG_OPTIONAL,	"Generate list [to FILE]"},
	{ "symbols",	's',	0,			OPTION_ARG_OPTIONAL,	"Generate symbol list in listing, no effect without --list"},
	{ "decb",		'b',	0,			0,						"Generate DECB .bin format output, equivalent of --format=decb"},
	{ "raw",		'r',	0,			0,						"Generate raw binary format output, equivalent of --format=raw"},
	{ "obj",		0x100,	0,			0,						"Generate proprietary object file format for later linking, equivalent of --format=obj" },
	{ "depend",		0x101,	0,			0,						"Output a dependency list to stdout; do not do any actual output though assembly is completed as usual" },
	{ "pragma",		'p',	"PRAGMA",	0,						"Set an assembler pragma to any value understood by the \"pragma\" pseudo op"},
	{ "6809",		'9',	0,			0,						"Set assembler to 6809 only mode" },
	{ "6309",		'3',	0,			0,						"Set assembler to 6309 mode (default)" },
	{ "includedir",	'I',	"PATH",		0,						"Add entry to include path" },
	{ 0 }
};


static error_t parse_opts(int key, char *arg, struct argp_state *state)
{
	asmstate_t *as = state -> input;
	
	switch (key)
	{
	case 'I':
		lw_stringlist_addstring(as -> include_list, arg);
		break;

	case 'o':
		if (as -> output_file)
			lw_free(as -> output_file);
		as -> output_file = lw_strdup(arg);
		break;

	case 'd':
		if (!arg)
			as -> debug_level = 50;
		else
			as -> debug_level = atoi(arg);
		break;

	case 'l':
		if (as -> list_file)
			lw_free(as -> list_file);
		if (!arg)
			as -> list_file = NULL;
		else
			as -> list_file = lw_strdup(arg);
		as -> flags |= FLAG_LIST;
		break;

	case 's':
		as -> flags |= FLAG_SYMBOLS;
		break;
		
	case 'b':
		as -> output_format = OUTPUT_DECB;
		break;

	case 'r':
		as -> output_format = OUTPUT_RAW;
		break;

	case 0x100:
		as -> output_format = OUTPUT_OBJ;
		break;

	case 0x101:
		as -> flags |= FLAG_DEPEND;
		break;

	case 'f':
		if (!strcasecmp(arg, "decb"))
			as -> output_format = OUTPUT_DECB;
		else if (!strcasecmp(arg, "raw"))
			as -> output_format = OUTPUT_RAW;
		else if (!strcasecmp(arg, "obj"))
			as -> output_format = OUTPUT_OBJ;
		else if (!strcasecmp(arg, "os9"))
		{
			as -> pragmas |= PRAGMA_DOLLARNOTLOCAL;
			as -> output_format = OUTPUT_OS9;
		}
		else
		{
			fprintf(stderr, "Invalid output format: %s\n", arg);
			exit(1);
		}
		break;
		
	case 'p':
		if (parse_pragma_string(as, arg, 0) == 0)
		{
			fprintf(stderr, "Unrecognized pragma string: %s\n", arg);
			exit(1);
		}
		break;

	case '9':
		as -> target = TARGET_6809;
		break;

	case '3':
		as -> target = TARGET_6309;
		break;

	case ARGP_KEY_END:
		break;
	
	case ARGP_KEY_ARG:
		lw_stringlist_addstring(as -> input_files, arg);
		break;
		
	default:
		return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

static struct argp argp =
{
	options,
	parse_opts,
	"<input file>",
	"LWASM, a HD6309 and MC6809 cross-assembler"
};

/*
main function; parse command line, set up assembler state, and run the 
assembler on the first file
*/
extern void do_pass1(asmstate_t *as);
extern void do_pass2(asmstate_t *as);
extern void do_pass3(asmstate_t *as);
extern void do_pass4(asmstate_t *as);
extern void do_pass5(asmstate_t *as);
extern void do_pass6(asmstate_t *as);
extern void do_pass7(asmstate_t *as);
extern void do_output(asmstate_t *as);
extern void do_list(asmstate_t *as);
extern lw_expr_t lwasm_evaluate_special(int t, void *ptr, void *priv);
extern lw_expr_t lwasm_evaluate_var(char *var, void *priv);
extern lw_expr_t lwasm_parse_term(char **p, void *priv);

struct passlist_s
{
	char *passname;
	void (*fn)(asmstate_t *as);
	int fordep;
} passlist[] = {
	{ "parse", do_pass1, 1 },
	{ "symcheck", do_pass2 },
	{ "resolve1", do_pass3 },
	{ "resolve2", do_pass4 },
	{ "addressresolve", do_pass5 },
	{ "finalize", do_pass6 },
	{ "emit", do_pass7 },
	{ NULL, NULL }
};


int main(int argc, char **argv)
{
	int passnum;

	/* assembler state */
	asmstate_t asmstate = { 0 };
	program_name = argv[0];

	lw_expr_set_special_handler(lwasm_evaluate_special);
	lw_expr_set_var_handler(lwasm_evaluate_var);
	lw_expr_set_term_parser(lwasm_parse_term);

	/* initialize assembler state */
	asmstate.include_list = lw_stringlist_create();
	asmstate.input_files = lw_stringlist_create();
	asmstate.nextcontext = 1;

	/* parse command line arguments */	
	argp_parse(&argp, argc, argv, 0, 0, &asmstate);

	if (!asmstate.output_file)
	{
		asmstate.output_file = lw_strdup("a.out");
	}

	input_init(&asmstate);

	for (passnum = 0; passlist[passnum].fn; passnum++)
	{
		if ((asmstate.flags & FLAG_DEPEND) && passlist[passnum].fordep == 0)
			continue;
		debug_message(&asmstate, 50, "Doing pass %d (%s)\n", passnum, passlist[passnum].passname);
		(passlist[passnum].fn)(&asmstate);
		debug_message(&asmstate, 50, "After pass %d (%s)\n", passnum, passlist[passnum].passname);
		dump_state(&asmstate);
		
		if (asmstate.errorcount > 0)
		{
			lwasm_show_errors(&asmstate);
			exit(1);
		}
	}

	if (asmstate.flags & FLAG_DEPEND)
	{
		// output dependencies
		char *n;
		
		while ((n = lw_stack_pop(asmstate.includelist)))
		{
			fprintf(stdout, "%s\n", n);
			lw_free(n);
		}
	}	
	else
	{
		debug_message(&asmstate, 50, "Doing output");
		do_output(&asmstate);
	}
	
	debug_message(&asmstate, 50, "Done assembly");
	
	do_list(&asmstate);
	
	exit(0);
}
