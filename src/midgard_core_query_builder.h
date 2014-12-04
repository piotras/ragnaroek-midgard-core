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

#ifndef MIDGARD_CORE_QB_H
#define MIDGARD_CORE_QB_H

#include "midgard/types.h"
#include "midgard/midgard_collector.h"
#include "schema.h"
#include "query_constraint.h"
#include "query_order.h"
#include "query_group_constraint.h"
#include "group_constraint.h"

/** 
 *
 * \defgroup core_qb  Midgard Core Query Builder
 *
 * This part of core is not a public API. 
 * It is used by Midgard Query Builder for midgard-core only. 
 * Functions or methods references are not distributed in public packages.
 */

struct _MidgardQueryBuilderPrivate {
	
	GSList *constraints;
	GSList *groups;
	gboolean is_grouping;
	guint grouping_ref;
	MidgardGroupConstraint *group_constraint;
	GSList *joins;
	GHashTable *tables;
	GSList *orders;
	GValue *value;

	/* constants */
	midgard *mgd;
	MgdSchemaTypeAttr *schema;
	GType type;

	guint offset;
	guint limit;
	gint lang;
	gint default_lang;
	gboolean unset_lang;
	gboolean include_deleted;
	gint error;
	gboolean read_only;
};

extern MidgardQueryBuilderPrivate *midgard_query_builder_private_new(void);
extern void midgard_query_builder_private_free(MidgardQueryBuilderPrivate *mqbp);

/*
 * \ingroup core_qb
 *
 * Query builder "modes"
 * 
 * MQB_SELECT_OBJECT,	Select all objects' data from database 
 * MQB_SELECT_GUID,	Select only objects' guids
 * MQB_SELECT_FIELD,	Select only particular fields
 * MQB_SELECT_COUNT,	Count results
 */ 
#define MQB_SELECT_OBJECT 0 
#define MQB_SELECT_GUID 1
#define MQB_SELECT_FIELD 2
#define MQB_SELECT_COUNT  3

/**
 *
 * \ingroup core_qb
 *
 * Adds table string for the given builder instance.
 *
 * \param 	builder	query builder
 * \param[in] 	table	table string	
 * 
 * Appends table string to query builders tables member
 * 
 */
extern void _midgard_core_qb_add_table(MidgardQueryBuilder *builder, 
	const gchar *table);

/**
 * \ingroup core_qb
 *
 * Creates SQL query string
 *
 * \param builder Midgard Query Builder instance
 * \param mode select QB's mode
 * \param select string which defines fields to select
 *
 * Mode is defined using MQB_SELECT constants. Select is appended
 * after SELECT part of SQL query and before WHERE.
 *
 * \return Fully generated SQL query
 */
extern gchar *_midgard_core_qb_get_sql(MidgardQueryBuilder *builder, 
	guint mode, gchar *select, gboolean order_workaround);

/**
 * \ingroup core_qb
 *
 * Returns formatted field part
 *
 * \param builder, Midgard Query Builder instance
 * \param name, property name
 *
 * \return formatted part of SQL 
 *
 * Returns formatted property part of SQL as 'table.column AS property'
 */
extern gchar *_midgard_core_qb_get_sql_as(MidgardQueryBuilder *builder, 
	const gchar *name);

extern void _midgard_core_qb_add_table(MidgardQueryBuilder *builder, 
	const gchar *table);

extern void _midgard_core_qb_add_constraint(MidgardQueryBuilder *builder, 
	MidgardQueryConstraint *constraint);

extern void _midgard_core_qb_add_order(MidgardQueryBuilder *builder,
        MidgardQueryOrder *order);

extern void _midgard_core_qb_add_group(MidgardQueryBuilder *builder,
	MidgardQueryGroupConstraint *group);

extern void _midgard_core_qb_add_group_constraint(MidgardQueryBuilder *builder,
        MidgardQueryConstraint *constraint);

extern gboolean _midgard_core_qb_parse_property(
	MidgardQueryBuilder *builder, MidgardQueryConstraint **constraint, const gchar *name);

extern gboolean _midgard_core_qb_is_grouping(MidgardQueryBuilder *builder);

extern GList *_midgard_core_qb_set_object_from_query(MidgardQueryBuilder *builder, guint select_type, MgdObject *object);
#endif /* MIDGARD_CORE_QB_H */
