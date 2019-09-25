/*
lwcc/cc-gencode.c

Copyright © 2019 William Astle

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
#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "tree.h"

void generate_code(node_t *n, FILE *output)
{
    node_t *nn;
    switch (n -> type)
    {
    // function definition - output prologue, then statements, then epilogue
    case NODE_FUNDEF:
        fprintf(output, "_%s\n", n->children->next_child->strval);
        generate_code(n->children->next_child->next_child->next_child, output);
        fprintf(output, "\trts\n");
        break;
    
    case NODE_CONST_INT:
        fprintf(output, "\tldd #%s\n", n->strval);
        break;

    case NODE_OPER_PLUS:
        generate_code(n->children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n->children->next_child, output);
        fprintf(output, "\taddd ,s++\n");
        break;
    
    case NODE_OPER_MINUS:
        generate_code(n->children, output);
        fprintf(output, "\tpshs d,x\n");
        generate_code(n->children->next_child, output);
        fprintf(output, "\tstd 2,s\n\tpuls d\n\tsubd ,s++\n");
        break;

    case NODE_OPER_TIMES:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n->children->next_child, output);
        fprintf(output, "\tjsr ___mul16i\n");
        break;

    case NODE_OPER_DIVIDE:
        generate_code(n -> children, output);
        fprintf(output, "\tpshs d\n");
        generate_code(n->children->next_child, output);
        fprintf(output, "\tjsr ___div16i\n");
        break;

    default:
        for (nn = n -> children; nn; nn = nn -> next_child)
            generate_code(nn, output);
        break;
    }
}
