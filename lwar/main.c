/*
main.c
Copyright © 2009 William Astle

This file is part of LWAR.

LWAR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <http://www.gnu.org/licenses/>.


Implements the program startup code

*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <lw_cmdline.h>
#include <lw_version.h>

#include "lwar.h"

// command line option handling
#define PROGVER "lwar from " PACKAGE_STRING
char *program_name;

static int parse_opts(int key, char *arg, void *state)
{
	switch (key)
	{
	case 'd':
		// debug
		debug_level++;
		break;
	
	case 'a':
		// add members
		operation = LWAR_OP_ADD;
		break;
	
	case 'c':
		// create archive
		operation = LWAR_OP_CREATE;
		break;
	
	case 'm':
		mergeflag = 1;
		break;

	case 'r':
		// replace members
		operation = LWAR_OP_REPLACE;
		break;
	
	case 'l':
		// list members
		operation = LWAR_OP_LIST;
		break;
	
	case 'x':
		// extract members
		operation = LWAR_OP_EXTRACT;
		break;

	case lw_cmdline_key_arg:
		if (archive_file)
		{
			// add archive member to list
			add_file_name(arg);
		}
		else
			archive_file = arg;
		break;
		
	default:
		return lw_cmdline_err_unknown;
	}
	return 0;
}

static struct lw_cmdline_options options[] =
{
	{ "replace",	'r',	0,		0,
				"Add or replace archive members" },
	{ "extract",	'x',	0,		0,
				"Extract members from the archive" },
	{ "add",		'a',	0,		0,
				"Add members to the archive" },
	{ "list",		'l',	0,		0,
				"List the contents of the archive" },
	{ "create",		'c',	0,		0,
				"Create new archive (or truncate existing one)" },
	{ "merge",		'm',	0,		0,
				"Add the contents of archive arguments instead of the archives themselves" },
	{ "debug",		'd',	0,		0,
				"Set debug mode"},
	{ 0 }
};

static struct lw_cmdline_parser argparser =
{
	options,
	parse_opts,
	"ARCHIVE [FILE ...]",
	"lwar, a library file manager for lwlink",
	PROGVER
};

extern void do_list(void);
extern void do_add(void);
extern void do_remove(void);
extern void do_replace(void);
extern void do_extract(void);

// main function; parse command line, set up assembler state, and run the
// assembler on the first file
int main(int argc, char **argv)
{
	program_name = argv[0];
	lw_cmdline_parse(&argparser, argc, argv, 0, 0, NULL);
	if (archive_file == NULL)
	{
		fprintf(stderr, "You must specify an archive file.\n");
		exit(1);
	}

	if (operation == 0)
	{
		fprintf(stderr, "You must specify an operation.\n");
		exit(1);
	}

	if (operation == LWAR_OP_LIST || operation == LWAR_OP_REMOVE || operation == LWAR_OP_EXTRACT)
	{
		struct stat stbuf;
		// make sure the archive exists
		if (stat(archive_file, &stbuf) < 0)
		{
			fprintf(stderr, "Cannot open archive file %s:\n", archive_file);
			perror("");
			exit(2);
		}
	}
	if (operation == LWAR_OP_CREATE)
	{
		struct stat stbuf;
		// check if the archive exists
		if (stat(archive_file, &stbuf) < 0)
		{
			if (errno != ENOENT)
			{
				fprintf(stderr, "Cannot create archive file %s:\n", archive_file);
				perror("");
				exit(2);
			}
		}
		else
		{
			if (unlink(archive_file) < 0)
			{
				fprintf(stderr, "Cannot create archive file %s:\n", archive_file);
				perror("");
				exit(2);
			}
				
		}
	}
	
	switch (operation)
	{
	case LWAR_OP_LIST:
		do_list();
		break;
	
	case LWAR_OP_ADD:
	case LWAR_OP_CREATE:
		do_add();
		break;
	
	case LWAR_OP_REMOVE:
		do_remove();
		break;
	
	case LWAR_OP_REPLACE:
		do_replace();
		break;
	
	case LWAR_OP_EXTRACT:
		do_extract();
		break;
	}

	exit(0);
}
