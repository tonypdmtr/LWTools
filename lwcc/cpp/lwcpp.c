/*

This is the main source for lwcpp, the C preprocessor

Copyright © 2012 William Astle

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

/* command line option handling */
#define PROGVER "lwcpp from " PACKAGE_STRING
char *program_name;

/* global state */
char *output_file = NULL;
int debug_level = 0;

static void do_help(void);
static void do_usage(void);

/*
NOTE:

We can't actually use a standard option parser here due to a raft of weird
command line syntax and we want to be somewhat compatible with various
other tools. That means we have a built-in help text that is preformatted.

*/

#define OPTARG(dest,src)	do { char *___s = (src); if (!*___s) { if (i < argc) ___s = argv[i++]; else { fprintf(stderr, "Option %s requires an argument\n", arg); } } (dest) = ___s; } while (0)
static void parse_cmdline(int argc, char **argv)
{
	int i = 1;
	int eargs = 0;
	char *arg;
	
	while (i < argc)
	{
		arg = argv[i++];
		if (!eargs && arg[0] == '-' && arg[1] != 0)
		{
			/* we have an option here */
			if (arg[1] == '-' && arg[2] == 0)
			{
				eargs = 1;
				continue;
			}
			
			/* consume the '-' */
			arg++;
			if (!strcmp(arg, "-help") || !strcmp(arg, "?"))
			{
				/* --help */
				do_help();
				exit(0);
			}
			else if (!strcmp(arg, "-usage"))
			{
				/* --usage */
				do_usage();
				exit(0);
			}
			else if (!strcmp(arg, "version") || !strcmp(arg, "-version"))
			{
				/* --version */
				printf("%s\n", PROGVER);
				exit(0);
			}
			
			switch (*arg)
			{
			case 'o':
				if (output_file)
					lw_free(output_file);
				OPTARG(output_file, arg + 1);
				continue;
			
			case 'd':
				if (!arg[1])
					debug_level = 50;
				else
					debug_level = atoi(arg + 1);
				continue;
			}
			
			fprintf(stderr, "Unknown option: %s\n", arg);
		}
		else
		{
			/* we have an input file here */
			printf("Input file: %s\n", arg);
		}
	}
}

/*
static struct lw_cmdline_parser cmdline_parser =
{
	options,
	parse_opts,
	"INPUTFILE",
	"lwcc, a HD6309 and MC6809 cross-compiler\vPlease report bugs to lost@l-w.ca.",
	PROGVER
};
*/
int main(int argc, char **argv)
{
	program_name = argv[0];

	parse_cmdline(argc, argv);

	if (!output_file)
	{
		output_file = lw_strdup("a.out");
	}

	exit(0);
}

void do_usage(void)
{
	printf(
		"Usage: %1$s [options] <input file>\n"
		"       %1$s --help\n"
		"       %1$s --version\n"
		"       %1$s --usage\n",
		program_name
	);
}

void do_help(void)
{
	printf(
		"Usage: %s [options] <input file>\n"
		"lwcpp, the lwtools C preprocessor\n"
		"\n"
		"  -d[LEVEL]                   enable debug output, optionally set verbosity\n"
		"                              level to LEVEL\n"
		"  -o FILE                     specify the output file name\n"
		"  -?, --help                  give this help message\n"
		"  --usage                     print a short usage message\n"
		"  -version, --version         print program version\n"
		"\n"
		"Please report bugs to lost@l-w.ca.\n",
		program_name
	);
}
