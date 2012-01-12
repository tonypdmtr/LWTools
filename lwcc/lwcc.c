/*

This is the front-end program for the C compiler.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>
#include <lw_cmdline.h>

/* command line option handling */
#define PROGVER "lwcc from " PACKAGE_STRING
char *program_name;

/* global state */
char *output_file = NULL;
int debug_level = 0;


static struct lw_cmdline_options options[] =
{
	{ "output",		'o',	"FILE",		0,							"Output to FILE"},
	{ "debug",		'd',	"LEVEL",	lw_cmdline_opt_optional,	"Set debug mode"},
	{ 0 }
};


static int parse_opts(int key, char *arg, void *state)
{
	switch (key)
	{
	case 'o':
		if (output_file)
			lw_free(output_file);
		output_file = lw_strdup(arg);
		break;

	case 'd':
		if (!arg)
			debug_level = 50;
		else
			debug_level = atoi(arg);
		break;

	case lw_cmdline_key_end:
		break;
	
	case lw_cmdline_key_arg:
		printf("Input file: %s\n", arg);
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
	"lwcc, a HD6309 and MC6809 cross-compiler\vPlease report bugs to lost@l-w.ca.",
	PROGVER
};

int main(int argc, char **argv)
{
	program_name = argv[0];

	/* parse command line arguments */	
	lw_cmdline_parse(&cmdline_parser, argc, argv, 0, 0, NULL);

	if (!output_file)
	{
		output_file = lw_strdup("a.out");
	}

	exit(0);
}
