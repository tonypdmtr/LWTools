/*
lwcc/strbuf.h

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

#ifndef strbuf_h_seen___
#define strbuf_h_seen___

struct strbuf
{
	char *str;
	int bl;
	int bo;
};

extern struct strbuf *strbuf_new(void);
extern void strbuf_add(struct strbuf *, int);
extern char *strbuf_end(struct strbuf  *);

#endif // strbufh_seen___
