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
 *   */

#ifndef MIDGARD_CORE_QUERY_H
#define MIDGARD_CORE_QUERY_H

#include "midgard/midgard_connection.h"
#include "midgard_core_object.h"
#include "midgard/types.h"

typedef struct _MidgardDBColumn MidgardDBColumn;

struct _MidgardDBColumn {
	const gchar *table_name;
	const gchar *column_name;
	const gchar *column_desc;
	const gchar *dbtype;
	GType gtype;
	gboolean index;
	gboolean unique;
	gboolean autoinc;
	gboolean primary;
	GValue *gvalue;
	const gchar *dvalue;
};

#define MIDGARD_GET_UINT_FROM_VALUE(__prop, __value) \
	if(G_VALUE_HOLDS_UINT(__value)) { \
		__prop = \
			g_value_get_uint(__value); \
	} \
	if(G_VALUE_HOLDS_INT(__value)) { \
		__prop = \
			(guint)g_value_get_int(__value); \
	}  


extern gboolean _midgard_core_query_create_table(
			MidgardConnection *mgd,
			const gchar *descr, 
			const gchar *tablename, 
			const gchar *primary);

extern gboolean _midgard_core_query_add_column(
			MidgardConnection *mgd,
			MidgardDBColumn *mdc);

extern gboolean _midgard_core_query_add_index(
			MidgardConnection *mgd,
			MidgardDBColumn *mdc);

extern gboolean _midgard_core_query_create_metadata_columns(
			MidgardConnection *mgd, 
			const gchar *tablename);

extern gboolean _midgard_core_query_create_basic_db(
			MidgardConnection *mgd);

extern gboolean _midgard_core_query_create_class_storage(
			MidgardConnection *mgd, 
			MidgardObjectClass *klass);

extern gboolean _midgard_core_query_update_class_storage(
			MidgardConnection *mgd, 
			MidgardObjectClass *klass);

extern gboolean _midgard_core_query_table_exists(
			MidgardConnection *mgd, 
			const gchar *tablename);

extern gboolean _midgard_core_query_update_object_fields(
			MgdObject *object, 
			const gchar *field, ...);

extern gboolean _midgard_core_query_create_repligard(
			MidgardConnection *mgd,
			const gchar *guid,
			guint id,
			const gchar *typename,
			const gchar *action_date,
			guint action,
			guint lang);

extern gboolean _midgard_core_query_update_repligard(
			MidgardConnection *mgd,
			const gchar *guid,
			guint id,
			const gchar *typename,
			const gchar *action_date,
			guint action,
			guint lang);

#endif /* MIDGARD_CORE_QUERY_H */
