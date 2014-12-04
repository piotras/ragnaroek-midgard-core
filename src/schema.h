/* 
 * Copyright (C) 2006 Piotr Pokora <piotr.pokora@infoglob.pl>
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

#ifndef _PRIVATE_SCHEMA_H
#define _PRIVATE_SCHEMA_H

#include "midgard/midgard_schema.h"

typedef struct _MgdSchemaTypeQuery MgdSchemaTypeQuery;

struct _MgdSchemaTypeQuery {
	gchar *select;
	gchar *select_full;
	gchar *link;
	gboolean use_lang;
	const gchar *tables;
	const gchar *primary;
	const gchar *parentfield;
	const gchar *upfield;
};

typedef struct _MgdSchemaPropertyAttr MgdSchemaPropertyAttr;

struct _MgdSchemaPropertyAttr{
	GType gtype;
	MidgardObjectClass *klass;
	GValue value;
	guint8 access_flags;
	const gchar *type;
	const gchar *dbtype;
	const gchar *field;
	gboolean dbindex;
	const gchar *table;
	const gchar *tablefield;
	const gchar *upfield;
	const gchar *parentfield;
	const gchar *primaryfield;
	gboolean is_multilang;
	const gchar *link;
	const gchar *link_target;
	gboolean is_link;
	gboolean is_linked;
	gboolean is_primary;
	gboolean is_reversed;
	gchar *description;
	GHashTable *user_fields;
	gint property_col_idx;
};

struct _MgdSchemaTypeAttr{
	guint base_index;
	guint num_properties;
	GParamSpec **params;
	MgdSchemaPropertyAttr **properties;
	GHashTable *prophash;
	GHashTable *metadata;
	GHashTable *tables;
	const gchar *table;
	const gchar *parent;
	const gchar *primary;
	const gchar *property_up;
	const gchar *property_parent;
	gboolean use_lang;
	GSList *childs;
	guint property_count;	
	MgdSchemaTypeQuery *query;
	GHashTable *user_values;
	gint cols_idx;	
};

/* MgdSchema storage utilities */
MgdSchemaPropertyAttr *_mgd_schema_property_attr_new();
void _mgd_schema_property_attr_free(gpointer data);

MgdSchemaTypeQuery *_mgd_schema_type_query_new();
void _mgd_schema_type_query_free(MgdSchemaTypeQuery *query);

MgdSchemaTypeAttr *_mgd_schema_type_attr_new();
void _mgd_schema_type_attr_free(MgdSchemaTypeAttr *type);

extern MgdSchemaTypeAttr * midgard_schema_lookup_type (MidgardSchema *schema, gchar *type);

extern GType
midgard_type_register(const gchar *class_name, MgdSchemaTypeAttr *data);

#endif /* _PRIVATE_SCHEMA_H */
