/*
lwcc/strpool.h

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

#ifndef strpool_h_seen___
#define strpool_h_seen___

struct strpool
{
	int nstrs;
	char **strs;
};

extern struct strpool *strpool_create(void);
extern void strpool_free(struct strpool *);
extern char *strpool_strdup(struct strpool *, const char *);

#endif // strpool.h_seen___
