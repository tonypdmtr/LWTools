/*
main.c

Copyright © 2011 William Astle

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

/*
main program startup handling for lwbasic
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <lw_cmdline.h>
#include <lw_string.h>
#include <lw_alloc.h>

#define __main_c_seen__
#include "lwbasic.h"

#define PROGVER "lwbasic from " PACKAGE_STRING

static struct lw_cmdline_options options[] =
{
	{ "output",			'o',		"FILE",		0,							"Output to FILE"},
	{ "debug",			'd',		"LEVEL",	lw_cmdline_opt_optional,	"Set debug mode"},
	{ 0 }
};

static int parse_opts(int key, char *arg, void *data)
{
	cstate *state = data;
	
	switch (key)
	{
	case 'o':
		if (state -> output_file)
			lw_free(state -> output_file);
		state -> output_file = lw_strdup(arg);
		break;

	case 'd':
		if (!arg)
			state -> debug_level = 50;
		else
			state -> debug_level = atoi(arg);
		break;
	
	case lw_cmdline_key_end:
		return 0;
		
	case lw_cmdline_key_arg:
		if (state -> input_file)
		{
			fprintf(stderr, "Already have an input file; ignoring %s\n", arg);
		}
		else
		{
			state -> input_file = lw_strdup(arg);
		}
		break;
		
	default:
		return lw_cmdline_err_unknown;
	}
	
	return 0;
}

static struct lw_cmdline_parser cmdline_parser =
{
	options,
	parse_opts,
	"INPUTFILE",
	"lwbasic, a compiler for a dialect of Basic\vPlease report bugs to lost@l-w.ca.",
	PROGVER
};

extern void parser(cstate *state);

int main(int argc, char **argv)
{
	cstate state = { 0 };

	lw_cmdline_parse(&cmdline_parser, argc, argv, 0, 0, &state);

	parser(&state);

	exit(0);
}

void lwb_error(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	exit(1);
}
