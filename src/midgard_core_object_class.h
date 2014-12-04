/* 
 * Copyright (C) 2006 Piotr Pokora <piotrek.pokora@gmail.com>
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
 */

#ifndef MIDGARD_CORE_OBJECT_CLASS_H
#define MIDGARD_CORE_OBJECT_CLASS_H

#include "midgard/types.h"
#include "schema.h"

/** 
 *
 * \defgroup core_object_class Midgard Core Object Class
 *
 * This part of core is not a public API. It is used only by midgard-core
 * and functions or methods references are not distributed in public packages.
 */

/**
 *
 * \ingroup core_object_class
 *
 * Performs object's internal data validation.
 *
 * \param object MgdObject object
 * 
 * \return TRUE if all object's properties and data are valid, FALSE otherwise
 *
 * This function validates object's properties types and its internal data.
 * Also it computes object's size which is set as object's metadata size property.
 *
 */
extern gboolean _midgard_core_object_is_valid(MgdObject *object);

/**
 * \ingroup core_object_class
 * 
 * Get object's property attribute pointer
 */
MgdSchemaPropertyAttr *_midgard_core_class_get_property_attr(
                GObjectClass *klass, const gchar *name);

/**
 * \ingroup core_object_class
 * 
 * Get object's property table
 */
const gchar *_midgard_core_class_get_property_table(
                GObjectClass *klass, const gchar *name);
		
/**
 * \ingroup core_object_class
 * 
 * Get object's property column name
 */
const gchar *_midgard_core_class_get_property_colname(
		GObjectClass *klass, const gchar *name);

/**
 * \ingroup core_object_class
 * 
 * Get object's property table.column name
 */
const gchar *_midgard_core_class_get_property_tablefield(
		GObjectClass *klass, const gchar *name);

#endif /* MIDGARD_CORE_OBJECT_CLASS_H */
