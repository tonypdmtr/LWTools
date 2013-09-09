/*
lwcc/cpp/cpp.h

Copyright © 2013 William Astle

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

#ifndef cpp_h_seen___
#define cpp_h_seen___

#include <stdio.h>

enum
{
	CPP_NOUNG = -3,
	CPP_EOL = -2,
	CPP_EOF = -1,
};

struct file_stack_e
{
	const char *fn;
	FILE *fp;
	struct file_stack_e *next;
	int line;
	int col;
	int eolstate;			// end of line state for interpreting \r\n \n\r \n \r
	int ra;					// read ahead byte for trigraph scan
	int qseen;				// number of ? seen during trigraph scan
	int unget;				// character that has been "ungot"
	int curc;				// the most recent character retrieved
};

extern FILE *output_fp;
extern int trigraphs;
extern struct file_stack_e *file_stack;

extern int process_file(const char *);

extern void do_error(const char *, ...);
extern void do_warning(const char *, ...);

#endif // cpp_h_seen___
