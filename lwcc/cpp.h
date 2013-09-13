/*
lwcc/cpp.h

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

#include "token.h"

#define TOKBUFSIZE 32

struct preproc_info
{
	const char *fn;
	FILE *fp;
	struct token *tokbuf[TOKBUFSIZE];
	struct token *tokqueue;
	int tokbuf_ptr;
	void (*errorcb)(const char *);
	void (*warningcb)(const char *);
	int eolstate;
	int lineno;
	int column;
	int trigraphs;
	int ra;
	int qseen;
	int ungetbufl;
	int ungetbufs;
	int *ungetbuf;
	int unget;
	int eolseen;
	int nlseen;
};

extern struct preproc_info *preproc_init(const char *);
extern struct token *preproc_next_token(struct preproc_info *);
extern void preproc_finish(struct preproc_info *);
extern void preproc_register_error_callback(struct preproc_info *, void (*)(const char *));
extern void preproc_register_warning_callback(struct preproc_info *, void (*)(const char *));
extern void preproc_throw_error(struct preproc_info *, const char *, ...);
extern void preproc_throw_warning(struct preproc_info *, const char *, ...);
#endif // cpp_h_seen___
