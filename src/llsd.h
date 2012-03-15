/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with main.c; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef LLSD_H
#define LLSD_H

#include <stdlib.h>

#include "llsd_const.h"
#include "llsd_binary.h"
#include "llsd_xml.h"

typedef union
{
	llsd_bin_t b;
	llsd_xml_t x;
} llsd_t;

/* serialize/deserialize interface */
llsd_t * llsd_parse( uint8_t * p, size_t len );

#endif /*LLSD_H*/

