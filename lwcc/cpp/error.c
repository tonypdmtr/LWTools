/*
lwcc/cpp/error.c

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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "cpp.h"

static void show_file_pos(void)
{
	if (file_stack == NULL)
		return;
	
	fprintf(stderr, "(%s:%d): ", file_stack -> fn, file_stack -> line);
}

void do_error(const char *f, ...)
{
	va_list arg;
	
	va_start(arg, f);
	fprintf(stderr, "ERROR: ");
	show_file_pos();
	vfprintf(stderr, f, arg);
	fprintf(stderr, "\n");
	va_end(arg);
	exit(1);
}

void do_warning(const char *f, ...)
{
	va_list arg;
	
	va_start(arg, f);
	fprintf(stderr, "WARNING: ");
	show_file_pos();
	vfprintf(stderr, f, arg);
	fprintf(stderr, "\n");
	va_end(arg);
}
