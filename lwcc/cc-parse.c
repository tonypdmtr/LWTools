/*
lwcc/cc-parse.c

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

#include <string.h>

#include <lw_alloc.h>
#include <lw_string.h>

#include "cpp.h"
#include "tree.h"

#define TOK_KW_IF       -1
#define TOK_KW_ELSE     -2
#define TOK_KW_WHILE    -3
#define TOK_KW_DO       -4
#define TOK_KW_FOR      -5
#define TOK_KW_VOID     -6
#define TOK_KW_INT      -7
#define TOK_KW_CHAR     -8
#define TOK_KW_SHORT    -9
#define TOK_KW_LONG     -10
#define TOK_KW_UNSIGNED -11
#define TOK_KW_SIGNED   -12
#define TOK_KW_FLOAT    -13
#define TOK_KW_DOUBLE   -14
#define TOK_KW_STRUCT   -15
#define TOK_KW_UNION    -16
#define TOK_KW_TYPEDEF  -17
#define TOK_KW_STATIC   -18
#define TOK_KW_SWITCH   -19
#define TOK_KW_CASE     -20
#define TOK_KW_DEFAULT  -21
#define TOK_KW_BREAK    -22
#define TOK_KW_CONTINUE -23
#define TOK_KW_CONST    -24
#define TOK_KW_AUTO     -25
#define TOK_KW_ENUM     -26
#define TOK_KW_REGISTER -27
#define TOK_KW_SIZEOF   -28
#define TOK_KW_VOLATILE -29
#define TOK_KW_RETURN   -30
#define TOK_KW_EXTERN   -31
#define TOK_KW_GOTO     -32
#define TOK_TYPENAME    -100
#define TOK_CONST_INT   -150

static struct { int tok; char *word; } keyword_list[] = {
    { TOK_KW_IF, "if" },
    { TOK_KW_ELSE, "else" },
    { TOK_KW_WHILE, "while" },
    { TOK_KW_DO, "do" },
    { TOK_KW_FOR, "for" },
    { TOK_KW_VOID, "void" },
    { TOK_KW_INT, "int" },
    { TOK_KW_CHAR, "char" },
    { TOK_KW_SHORT, "short" },
    { TOK_KW_LONG, "long" },
    { TOK_KW_UNSIGNED, "unsigned" },
    { TOK_KW_SIGNED, "signed" },
    { TOK_KW_FLOAT, "float" },
    { TOK_KW_DOUBLE, "double" },
    { TOK_KW_STRUCT, "struct" },
    { TOK_KW_UNION, "union" },
    { TOK_KW_TYPEDEF, "typedef" },
    { TOK_KW_STATIC, "static" },
    { TOK_KW_SWITCH, "switch" },
    { TOK_KW_CASE, "case" },
    { TOK_KW_DEFAULT, "default" },
    { TOK_KW_BREAK, "break" },
    { TOK_KW_CONTINUE, "continue" },
    { TOK_KW_CONST, "const" },
    { TOK_KW_AUTO, "auto" },
    { TOK_KW_ENUM, "enum" },
    { TOK_KW_REGISTER, "register" },
    { TOK_KW_SIZEOF, "sizeof" },
    { TOK_KW_VOLATILE, "volatile" },
    { TOK_KW_RETURN, "return" },
    { TOK_KW_EXTERN, "extern" },
    { TOK_KW_GOTO, "goto" },
    { TOK_NONE, "" }
};


struct parser_state
{
    struct preproc_info *pp;                // preprocessor data
    struct token *curtok;                   // the current token
};


struct token *parse_next(struct parser_state *ps)
{
    struct token *tok;
    int i;
    
    for (;;)
    {
        tok = preproc_next(ps -> pp);
        if (tok -> ttype == TOK_WSPACE)
            continue;
        if (tok -> ttype == TOK_EOL)
            continue;
        if (tok -> ttype == TOK_CHAR)
        {
            // random character
            fprintf(stderr, "Random character %02x\n", tok -> strval[0]);
            if (tok -> strval[0] < 32 || tok -> strval[0] > 126)
                    continue;
        }
        break;
    }
    if (tok -> ttype == TOK_IDENT)
    {
        // convert identifier tokens to their respective meanings
        for (i = 0; keyword_list[i].tok != TOK_NONE; i++)
        {
            if (strcmp(keyword_list[i].word, tok -> strval) == 0)
            {
                tok -> ttype = keyword_list[i].tok;
                goto out;
            }
        }
        // check for registered types here
    }
    else if (tok -> ttype == TOK_NUMBER)
    {
        // look for anything that isn't 0-9
        for (i = 0; tok -> strval[i]; i++)
        {
            if (tok -> strval[i] < '0' || tok -> strval[i] > '9')
                break;
        }
        if (tok -> strval[i] == 0)
            tok -> ttype = TOK_CONST_INT;
    }
out:
    fprintf(stderr, "Lexed: ");
    token_print(tok, stderr);
    fprintf(stderr, " (%d)\n", tok -> ttype);
    if (ps -> curtok)
        token_free(ps -> curtok);
    ps -> curtok = tok;
    return tok;
}

void parse_generr(struct parser_state *ps, char *tag)
{
    fprintf(stderr, "(%s) Unexpected token (%d): ", tag, ps -> curtok -> ttype);
    token_print(ps -> curtok, stderr);
    fprintf(stderr, "\n");

}

node_t *parse_term_real(struct parser_state *ps)
{
    node_t *rv;

    switch (ps -> curtok -> ttype)
    {
    case TOK_CONST_INT:
        rv = node_create(NODE_CONST_INT, ps -> curtok -> strval);
        parse_next(ps);
        return rv;
    }

    parse_generr(ps, "term");
    return NULL;
}

node_t *parse_expr_real(struct parser_state *ps, int prec)
{
    static struct { int tok; int nodetype; int prec; } operlist[] = {
        { TOK_STAR, NODE_OPER_TIMES, 150 },
        { TOK_DIV, NODE_OPER_DIVIDE, 150 },
        { TOK_ADD, NODE_OPER_PLUS, 100 },
        { TOK_SUB, NODE_OPER_MINUS, 100 },
        { 0, 0, 0 }
    };
    node_t *term1, *term2;
    int i;
    
    term1 = parse_term_real(ps);
    if (!term1)
        return NULL;

nextoper:
    for (i = 0; operlist[i].tok; i++)
        if (operlist[i].tok == ps -> curtok -> ttype)
            break;
    fprintf(stderr, "Matched operator: %d, %d\n", operlist[i].tok, operlist[i].prec);
    // if we hit the end of the expression, return
    if (operlist[i].tok == 0)
        return term1;

    // is the next operator less or same precedence?
    if (operlist[i].prec <= prec)
        return term1;

    parse_next(ps);
    term2 = parse_expr_real(ps, operlist[i].prec);
    if (!term2)
    {
        parse_generr(ps, "expr");
        node_destroy(term2);
    }
    
    term1 = node_create(operlist[i].nodetype, term1, term2);
    term2 = NULL;
    goto nextoper;
}

node_t *parse_expr(struct parser_state *ps)
{
    return parse_expr_real(ps, 0);
}

node_t *parse_statement(struct parser_state *ps)
{
    node_t *rv;
    node_t *n;

    switch (ps -> curtok -> ttype)
    {
    case TOK_KW_RETURN:
        parse_next(ps);
        n = parse_expr(ps);
        if (!n)
        {
            parse_generr(ps, "statement");
            return NULL;
        }
        rv = node_create(NODE_STMT_RETURN);
        node_addchild(rv, n);
        break;

    default:
        return NULL;
    }
    
    if (ps -> curtok -> ttype != TOK_EOS)
        parse_generr(ps, "statement");
    else
        parse_next(ps);
    return rv;
}

node_t *parse_globaldecl(struct parser_state *ps)
{
    node_t *rv = NULL;
    node_t *stmt;
    char *fnname = NULL;
    if (ps -> curtok -> ttype == TOK_KW_INT)
    {
        // variable name
        parse_next(ps);
        if (ps -> curtok -> ttype != TOK_IDENT)
            goto error;
        fnname = lw_strdup(ps -> curtok -> strval);
        parse_next(ps);
        if (ps -> curtok -> ttype != TOK_OPAREN)
            goto error;
        parse_next(ps);
        if (ps -> curtok -> ttype != TOK_CPAREN)
            goto error;
        parse_next(ps);
        if (ps -> curtok -> ttype != TOK_OBRACE)
            goto error;
        parse_next(ps);
        stmt = parse_statement(ps);
        if (!stmt)
            goto error;
        rv = node_create(NODE_FUNDEF, node_create(NODE_TYPE_INT), node_create(NODE_IDENT, fnname), node_create(NODE_FUNARGS), stmt);
        if (ps -> curtok -> ttype != TOK_CBRACE)
            goto error;
        parse_next(ps);
        lw_free(fnname);        
        return rv;
    }        
        

error:
    if (fnname)
        lw_free(fnname);
    parse_generr(ps, "globaldecl");
    return rv;
}

node_t *parse_program(struct preproc_info *pp)
{
    node_t *rv;
    node_t *node;
    struct parser_state ps;
    
    ps.pp = pp;
    ps.curtok = NULL;

    rv = node_create(NODE_PROGRAM);

    // prime the parser
    parse_next(&ps);
    while (ps.curtok -> ttype != TOK_EOF)
    {
        node = parse_globaldecl(&ps);
        if (!node)
            break;
        node_addchild(rv, node);
    }

    return rv;
}
