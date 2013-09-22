/*
lwcc/tree.h

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

#ifndef tree_h_seen___
#define tree_h_seen___

#include <stdio.h>

/* the various node types */
#define NODE_NONE		0	// a node with no type
#define NODE_PROGRAM	1	// the whole program
#define NODE_NUMTYPES	2	// the number of node types

typedef struct node_s node_t;

struct node_s
{
	int type;				// node type
	char *strval;			// any string value associated with the node
	unsigned char ival[8];	// any 64 bit integer value associated with the node, signed or unsigned
	node_t *children;		// pointer to list of child nodes
	node_t *next_child;		// pointer to next child in the list
	node_t *parent;			// pointer to parent node
};

extern node_t *node_create(int, ...);
extern void node_destroy(node_t *);
extern void node_addchild(node_t *, node_t *);
extern void node_removechild(node_t *, node_t *);
extern void node_display(node_t *, FILE *);
extern void node_removechild_destroy(node_t *, node_t *);

#endif // tree_h_seen___
