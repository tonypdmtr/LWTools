/*
pragma.c

Copyright © 2010 William Astle

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
#include <ctype.h>

#include <lw_string.h>
#include <lw_alloc.h>

#include "lwasm.h"
#include "instab.h"

struct pragma_list
{
	const char *str;
	int flag;
};

static const struct pragma_list set_pragmas[] =
{
	{ "dollarnotlocal", PRAGMA_DOLLARNOTLOCAL },
	{ "noindex0tonone", PRAGMA_NOINDEX0TONONE },
	{ "undefextern", PRAGMA_UNDEFEXTERN },
	{ "cescapes", PRAGMA_CESCAPES },
	{ "importundefexport", PRAGMA_IMPORTUNDEFEXPORT },
	{ "pcaspcr", PRAGMA_PCASPCR },
	{ "shadow", PRAGMA_SHADOW },
	{ "nolist", PRAGMA_NOLIST },
	{ 0, 0 }
};

static const struct pragma_list reset_pragmas[] =
{
	{ "nodollarnotlocal", PRAGMA_DOLLARNOTLOCAL },
	{ "index0tonone", PRAGMA_NOINDEX0TONONE },
	{ "noundefextern", PRAGMA_UNDEFEXTERN },
	{ "nocescapes", PRAGMA_CESCAPES },
	{ "noimportundefexport", PRAGMA_IMPORTUNDEFEXPORT },
	{ "nopcaspcr", PRAGMA_PCASPCR },
	{ "noshadow", PRAGMA_SHADOW },
	{ "list", PRAGMA_NOLIST },
	{ 0, 0 }
};

int parse_pragma_string(asmstate_t *as, char *str, int ignoreerr)
{
	char *p;
	int i;
	const char *np = str;
	int pragmas = as -> pragmas;

	while (np)
	{
		p = lw_token(np, ',', &np);
		for (i = 0; set_pragmas[i].str; i++)
		{
			if (!strcasecmp(p, set_pragmas[i].str))
			{
				pragmas |= set_pragmas[i].flag;
				goto out;
			}
		}
		for (i = 0; reset_pragmas[i].str; i++)
		{
			if (!strcasecmp(p, reset_pragmas[i].str))
			{
				pragmas &= ~(reset_pragmas[i].flag);
				goto out;
			}
		}
		/* unrecognized pragma here */
		if (!ignoreerr)
		{
			lw_free(p);
			return 0;
		}
	out:	
		lw_free(p);
	}
	as -> pragmas = pragmas;
	return 1;
}

PARSEFUNC(pseudo_parse_pragma)
{
	char *ps, *t;
	
	for (t = *p; *t && !isspace(*t); t++)
		/* do nothing */ ;
	
	ps = lw_strndup(*p, t - *p);
	*p = t;
	
	l -> len = 0;

	if (parse_pragma_string(as, ps, 0) == 0)
	{
		lwasm_register_error(as, l, "Unrecognized pragma string");
	}
	if (as -> pragmas & PRAGMA_NOLIST)
		l -> pragmas |= PRAGMA_NOLIST;
	lw_free(ps);
}

PARSEFUNC(pseudo_parse_starpragma)
{
	char *ps, *t;
	
	for (t = *p; *t && !isspace(*t); t++)
		/* do nothing */ ;
	
	ps = lw_strndup(*p, t - *p);
	*p = t;

	l -> len = 0;
	
	// *pragma must NEVER throw an error
	parse_pragma_string(as, ps, 1);
	if (as -> pragmas & PRAGMA_NOLIST)
		l -> pragmas |= PRAGMA_NOLIST;
	lw_free(ps);
}
