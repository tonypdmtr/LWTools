/*
lw_alloc.h

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

#ifndef ___lw_alloc_h_seen___
#define ___lw_alloc_h_seen___


#ifdef ___lw_alloc_c_seen___

#define ___E

#else /* def ___lw_alloc_c_seen___ */

#define ___E extern

#endif /* def ___lw_alloc_c_seen___ */

___E void lw_free(void *P);
___E void *lw_alloc(int S);
___E void *lw_realloc(void *P, int S);

#undef ___E

#endif /* ___lw_alloc_h_seen___ */
