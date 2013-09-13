/*
lwcc/cpp.c

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
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "cpp.h"

struct token *preproc_lex_next_token(struct preproc_info *);

struct preproc_info *preproc_init(const char *fn)
{
	FILE *fp;
	struct preproc_info *pp;
	
	if (!fn || (fn[0] == '-' && fn[1] == '0'))
	{
		fp = stdin;
	}
	else
	{
		fp = fopen(fn, "rb");
	}
	if (!fp)
		return NULL;
	
	pp = lw_alloc(sizeof(struct preproc_info));
	memset(pp, 0, sizeof(struct preproc_info));
	pp -> fn = lw_strdup(fn);
	pp -> fp = fp;
	pp -> ra = CPP_NOUNG;
	return pp;
}

struct token *preproc_next_token(struct preproc_info *pp)
{
	struct token *t;
	
	if (pp -> tokqueue)
	{
		t = pp -> tokqueue;
		pp -> tokqueue = t -> next;
		if (pp -> tokqueue)
			pp -> tokqueue -> prev = NULL;
		t -> next = NULL;
		t -> prev = NULL;
		return t;
	}
	return(preproc_lex_next_token(pp));
}

void preproc_finish(struct preproc_info *pp)
{
	lw_free((void *)(pp -> fn));
	fclose(pp -> fp);
	lw_free(pp);
}

void preproc_register_error_callback(struct preproc_info *pp, void (*cb)(const char *))
{
	pp -> errorcb = cb;
}

void preproc_register_warning_callback(struct preproc_info *pp, void (*cb)(const char *))
{
	pp -> warningcb = cb;
}

static void preproc_throw_error_default(const char *m)
{
	fprintf(stderr, "ERROR: %s\n", m);
}

static void preproc_throw_warning_default(const char *m)
{
	fprintf(stderr, "WARNING: %s\n", m);
}

static void preproc_throw_message(void (*cb)(const char *), const char *m, va_list args)
{
	int s;
	char *b;
	
	s = vsnprintf(NULL, 0, m, args);
	b = lw_alloc(s + 1);
	vsnprintf(b, s + 1, m, args);
	(*cb)(b);
	lw_free(b);
}

void preproc_throw_error(struct preproc_info *pp, const char *m, ...)
{
	va_list args;
	va_start(args, m);
	preproc_throw_message(pp -> errorcb ? pp -> errorcb : preproc_throw_error_default, m, args);
	va_end(args);
	exit(1);
}

void preproc_throw_warning(struct preproc_info *pp, const char *m, ...)
{
	va_list args;
	va_start(args, m);
	preproc_throw_message(pp -> warningcb ? pp -> warningcb : preproc_throw_warning_default, m, args);
	va_end(args);
}
