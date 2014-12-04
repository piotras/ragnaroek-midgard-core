/* 
 * Copyright (C) 2005 Piotr Pokora <piotr.pokora@infoglob.pl>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *   
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *   */

#ifndef MIDGARD_DATATYPES_H
#define MIDGARD_DATATYPES_H

#include "midgard/midgard.h"

/**
 * \defgroup midgard_datatypes Midgard Datatypes
 */ 

/**
 * \ingroup midgard_datatypes
 *
 * Creates new GHashTable
 *
 * This function is a wrapper for g_hash_table_new_full.
 * Returned GHashTable should contain only strings as keys
 * and as values. Destructor for returned GHashaTable is 
 * automatically registered, so caller is responsible to call
 * g_hash_table_destroy only. 
 *
 * @return new GhashTable
 */ 

GHashTable *midgard_hash_strings_new(void);

/**
 * \ingroup midgard_datatypes
 *
 * MidgardTypeHolder is an opaque holder for different datatypes.
 * You should use this struct when for example integers values
 * should be passed to function which accepts only gpointer ones.
 * There are no strict limitations how this struct should be used.
 * If particular function requires this type, its documentation should
 * clearly describe which member of MidgardTypeHolder is filled and used.
 *
 */ 
struct MidgardTypeHolder{
	guint elements;
	guint level;
};


#endif /* MIDGARD_DATATYPES_H */
