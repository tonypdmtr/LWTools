/*
lwcc/tree.c

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
#include <string.h>
#include <lw_alloc.h>
#include <lw_string.h>

#include "tree.h"

static char *node_names[] = {
	"NONE",
	"PROGRAM",
	"DECL",
	"TYPE_CHAR",
	"TYPE_SHORT",
	"TYPE_INT",
	"TYPE_LONG",
	"TYPE_LONGLONG",
	"IDENT",
	"TYPE_PTR",
	"TYPE_SCHAR",
	"TYPE_UCHAR",
	"TYPE_USHORT",
	"TYPE_UINT",
	"TYPE_ULONG",
	"TYPE_ULONGLONG",
	"TYPE_VOID",
	"TYPE_FLOAT",
	"TYPE_DOUBLE",
	"TYPE_LDOUBLE",
	"FUNDEF",
	"FUNDECL",
	"FUNARGS",
	"BLOCK",
};



node_t *node_create(int type, ...)
{
	node_t *r;
	int nargs = 0;
	va_list args;
	
	va_start(args, type);
	r = lw_alloc(sizeof(node_t));
	memset(r, 0, sizeof(node_t));
	r -> type = type;
	
	switch (type)
	{
	case NODE_DECL:
		nargs = 2;
		break;
	
	case NODE_TYPE_PTR:
		nargs = 1;
		break;
		
	case NODE_IDENT:
		r -> strval = lw_strdup(va_arg(args, char *));
		break;
	
	case NODE_FUNDEF:
		nargs = 4;
		break;
	
	case NODE_FUNDECL:
		nargs = 3;
		break;
	}
	
	while (nargs--)
	{
		node_addchild(r, va_arg(args, node_t *));
	}
	va_end(args);
	return r;
}

void node_destroy(node_t *node)
{
	node_t *n;
	
	while (node -> children)
	{
		n = node -> children -> next_child;
		node_destroy(node -> children);
		node -> children = n;
	}
	lw_free(node -> strval);
	lw_free(node);
}

void node_addchild(node_t *node, node_t *nn)
{
	node_t *tmp;
	
	if (!nn)
		return;
	
	nn -> parent = node;
	nn -> next_child = NULL;
	if (node -> children)
	{
		for (tmp = node -> children; tmp -> next_child; tmp = tmp -> next_child)
			/* do nothing */ ;
		tmp -> next_child = nn;
	}
	else
	{
		node -> children = nn;
	}
}

void node_removechild(node_t *node, node_t *nn)
{
	node_t **pp;
	node_t *np;
	
	if (!node)
		node = nn -> parent;
	
	pp = &(node -> children);
	for (np = node -> children; np; np = np -> next_child)
	{
		if (np -> next_child == nn)
			break;
		pp = &((*pp) -> next_child);
	}
	if (!np)
		return;
	
	*pp = nn -> next_child;
	nn -> parent = NULL;
	nn -> next_child = NULL;
}

void node_removechild_destroy(node_t *node, node_t *nn)
{
	node_removechild(node, nn);
	node_destroy(nn);
}

static void node_display_aux(node_t *node, FILE *f, int level)
{
	node_t *nn;
	int i;
	
	for (i = 0; i < level * 4; i++)
		fputc(' ', f);
	fprintf(f, "(%s ", node_names[node -> type]);
	if (node -> strval)
		fprintf(f, "\"%s\" ", node -> strval);
	fputc('\n', f);
	for (nn = node -> children; nn; nn = nn -> next_child)
		node_display_aux(nn, f, level + 1);
	for (i = 0; i < level * 4; i++)
		fputc(' ', f);
	fputc(')', f);
	fputc('\n', f);
}

void node_display(node_t *node, FILE *f)
{
	node_display_aux(node, f, 0);
}
