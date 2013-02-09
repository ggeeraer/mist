// vim:sw=4:ts=4
/*
   This file is part of mist2.

   mist2 is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   mist2 is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mist2; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

   Copyright 2002, Anthony Piron
 */

#include "error.h"
#include "xmalloc.h"
#include "stdlib.h"
#include "tbsymbolinfo.h"

T_PTR_tbsymbol_info 
tbsymbol_info_new() {
  T_PTR_tbsymbol_info info;
    
  info = (T_PTR_tbsymbol_info)xmalloc(sizeof(T_tbsymbol_info));
  return info;
}


void
tbsymbol_info_destroy(T_PTR_tbsymbol_info* info) {
  xfree(info);
  info = NULL;
}