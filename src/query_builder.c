/*
 * Copyright (c) 2005 Jukka Zitting <jz@yukatan.fi>
 * Copyright (c) 2005,2008 Piotr Pokora <piotrek.pokora@gmail.com>
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

#include <config.h>
#include "midgard/midgard_schema.h"
#include "midgard/query_builder.h"
#include "midgard/midgard_legacy.h"
#include "query_constraint.h"
#include "simple_constraint.h"
#include "group_constraint.h"
#include "query_order.h"
#include "midgard/midgard_object_property.h"
#include "midgard/midgard_object.h"
#include "schema.h"
#include "midgard_mysql.h"
#include "midgard_core_query_builder.h"
#include "midgard/midgard_error.h"
#include "midgard_core_object.h"
#include "midgard/midgard_datatypes.h"

/* Internal prototypes , I am not sure if should be included in API */
gchar *midgard_query_builder_get_object_select(MidgardQueryBuilder *builder, guint select_type);

static gboolean _mqb_is_grouping(MidgardQueryBuilder *builder)
{
	return FALSE;

	if(builder->priv->is_grouping) {

		g_warning("Group condition not correctly closed ( missed 'end_group' ?) ");
		return TRUE;
	}

	return FALSE;
}

static gboolean __type_is_valid(GType type)
{
	GType _type = type;
	if(!_type)
		return FALSE;
	
	/* check MIDGARD_TYPE_OBJECT */
	if(g_type_parent(_type) != MIDGARD_TYPE_DBOBJECT) {
		
		/* if fails, try MIDGARD_TYPE_DBOBJECT */
		if(g_type_parent(_type) != MIDGARD_TYPE_OBJECT) {
			
			return FALSE;
		
		} else {

			return TRUE;
		}
	}

	return TRUE;
}

MidgardQueryBuilder *midgard_query_builder_new(
        midgard *mgd, const gchar *classname)
{
        g_assert(mgd != NULL);
        g_assert(classname != NULL);

	GType class_type = g_type_from_name(classname);
	
	if(!__type_is_valid(class_type)) {

		g_warning("Can not initialize Midgard Query Builder for '%s'. It's not registered GType system class", classname);
		MIDGARD_ERRNO_SET(mgd, MGD_ERR_INVALID_OBJECT);
		return NULL;
	}

        MidgardQueryBuilder *builder = 
		g_object_new(MIDGARD_TYPE_QUERY_BUILDER, NULL);
	
	builder->priv = midgard_query_builder_private_new();

        builder->priv->mgd = mgd;
        builder->priv->type = g_type_from_name(classname);
        builder->priv->lang = -1;
	builder->priv->unset_lang = FALSE;
	builder->priv->include_deleted = FALSE;
        builder->priv->error = 0;	
	
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS_BY_NAME(classname);
	builder->priv->schema = klass->dbpriv->storage_data;
       	
	gchar **tables = g_strsplit(midgard_object_class_get_tables(klass), ",", 0);
	guint i = 0;
	while(tables[i] != NULL) {	
		_midgard_core_qb_add_table(builder, (const gchar *)tables[i]);
		i++;
	}
	g_strfreev(tables);

        builder->priv->offset = 0;
        builder->priv->limit = G_MAXUINT;
	builder->priv->read_only = FALSE;

        if (builder->priv->type && builder->priv->schema) {
                return builder;
        } else {
		g_object_unref(builder);
                return NULL;
        }


}

void midgard_query_builder_free(MidgardQueryBuilder *builder) 
{
	g_assert(builder != NULL);

	midgard_query_builder_private_free(builder->priv);
}

gboolean midgard_query_builder_add_constraint(
        MidgardQueryBuilder *builder,
        const gchar *name, const gchar *op, const GValue *value) 
{
	g_assert(builder);
        g_assert(name);
        g_assert(op);
        g_assert(value);
     
	MidgardQueryConstraint *constraint = midgard_query_constraint_new();	

	if(!midgard_query_constraint_add_operator(constraint, op))
		return FALSE;

	if(!_midgard_core_qb_parse_property(builder, &constraint, name))
		return FALSE;

	if(!midgard_query_constraint_add_value(constraint, value))
		return FALSE;
	
	if(!midgard_query_constraint_build_condition(constraint))
		return FALSE;

	/* FIXME, table should be stored per every constraint, order, whatever */
	_midgard_core_qb_add_table(builder, constraint->priv->prop_left->table);
	
	if(builder->priv->grouping_ref > 0) {

		MidgardGroupConstraint *group = 
			(MidgardGroupConstraint *)builder->priv->group_constraint;
		midgard_group_constraint_add_constraint(group, constraint);
		return TRUE;
	
	} else {
		
		_midgard_core_qb_add_constraint(builder, constraint);
	}

	return TRUE;
}

gboolean midgard_query_builder_add_constraint_with_property(
		MidgardQueryBuilder *builder, const gchar *property_a,
		const gchar *op, const gchar *property_b)
{
	g_assert(builder != NULL);
	g_assert(property_a != NULL);
	g_assert(op != NULL);
	g_assert(property_b != NULL);

	MidgardQueryConstraint *constraint = midgard_query_constraint_new();

	if(!_midgard_core_qb_parse_property(builder, &constraint, property_a))
		return FALSE;
	constraint->priv->current = constraint->priv->prop_right;
	if(!_midgard_core_qb_parse_property(builder, &constraint, property_b))
		return FALSE;

	constraint->priv->condition_operator = g_strdup(op);

	if(!midgard_query_constraint_build_condition(constraint))
		return FALSE;

	_midgard_core_qb_add_table(builder, constraint->priv->prop_left->table);
	_midgard_core_qb_add_table(builder, constraint->priv->prop_right->table);

	if(builder->priv->grouping_ref > 0) {
	
		MidgardGroupConstraint *group = builder->priv->group_constraint;
		midgard_group_constraint_add_constraint(group, constraint);
		return TRUE;
	
	} else {
		
		_midgard_core_qb_add_constraint(builder, constraint);
	}

	return TRUE;
}

gboolean midgard_query_builder_begin_group(
                MidgardQueryBuilder *builder, const gchar *type) 
{
	g_assert(builder != NULL);

	const gchar *group_op = type;

	if(group_op == NULL)
		group_op = "OR";

        MidgardGroupConstraint *group = 
		midgard_group_constraint_new(group_op);

	if(!group)
		return FALSE;

	/* Check if this is nested grouping */
	if(builder->priv->grouping_ref > 0) {

		MidgardGroupConstraint *pgroup = builder->priv->group_constraint; 
		pgroup->nested = g_slist_prepend(pgroup->nested, group);
		group->parent_group = pgroup;

	} else { 
		
		builder->priv->groups = 
			g_slist_prepend(builder->priv->groups, group);
	}

	builder->priv->group_constraint = group;

	builder->priv->grouping_ref++;
	builder->priv->is_grouping = TRUE;

	return TRUE;
}

gboolean midgard_query_builder_end_group(MidgardQueryBuilder *builder) 
{
	g_assert(builder != NULL);

	if(builder->priv->grouping_ref == 0) {
		
		g_warning("Missed 'begin_group' method");
		return FALSE;
	}

	MidgardGroupConstraint *group = NULL;

	if(builder->priv->group_constraint)
		group = MIDGARD_GROUP_CONSTRAINT(builder->priv->group_constraint);
	
	if(group && group->parent_group) {

		builder->priv->group_constraint = group->parent_group;

	} else {
	
		builder->priv->group_constraint = NULL;
	}
	
	builder->priv->grouping_ref--;	

	return TRUE;
}

gboolean midgard_query_builder_add_order(
        MidgardQueryBuilder *builder, const gchar *name, const gchar *dir)
{
        g_assert(builder != NULL);
        g_assert(name != NULL);

	MidgardQueryOrder *order = 
		midgard_core_query_order_new(builder, name, dir);

	if (order) {
		
		_midgard_core_qb_add_order(builder, order);
		return TRUE;
	
	} else {

		g_warning("Skipping a ordering constraint specification");
		return FALSE;
	}
}

void midgard_query_builder_set_limit(
        MidgardQueryBuilder *builder, guint limit)
{
	g_assert(builder != NULL);

        builder->priv->limit = limit;
}

void midgard_query_builder_set_offset(
        MidgardQueryBuilder *builder, guint offset)
{
        g_assert(builder != NULL);

        builder->priv->offset = offset;
}

void midgard_query_builder_set_lang(MidgardQueryBuilder *builder, gint lang)
{
	g_assert(builder != NULL);
	
	builder->priv->lang = lang;
}

void midgard_query_builder_unset_languages(MidgardQueryBuilder *builder)
{
	g_assert(builder);
	
	builder->priv->unset_lang = TRUE;
}

void midgard_query_builder_toggle_read_only(MidgardQueryBuilder *builder, gboolean toggle)
{
	g_return_if_fail(builder != NULL);

	builder->priv->read_only = toggle;
}

gint _compare_ml_guid(gconstpointer a, gconstpointer b){

        if(g_ascii_strcasecmp(a,b) == 0) return 0;
        return 1;
}

static GList *midgard_query_builder_execute_or_count(
        MidgardQueryBuilder *builder, MidgardTypeHolder *holder, guint select_type)
{
        g_assert(builder != NULL);

	if(builder->priv->grouping_ref > 0) {
		
		g_warning("Incorrect constraint grouping. Missed 'end_group'?");
		return NULL;
	}

	if(builder->priv->type == MIDGARD_TYPE_SITEGROUP) {
		
		if(!mgd_isroot(builder->priv->mgd)) {
			
			MIDGARD_ERRNO_SET(builder->priv->mgd, MGD_ERR_ACCESS_DENIED);
			g_warning("Type incompatible with Midgard Query Builder");
			return NULL;
		}
	}
        
        GList *list = _midgard_core_qb_set_object_from_query(builder, select_type, NULL);
        if(list == NULL){
                if(holder)
			holder->elements = 0;
                return NULL; 
        }
        
	if(holder)
		holder->elements = g_list_length(list);    

	return list;	 
}

GObject **midgard_query_builder_execute(
        MidgardQueryBuilder *builder, MidgardTypeHolder *holder)
{
	g_assert(builder != NULL);

	if(_mqb_is_grouping(builder))
		return NULL;

	guint i = 0;
	gboolean free_holder = FALSE;
	MIDGARD_ERRNO_SET(builder->priv->mgd, MGD_ERR_OK);
	
	if(!holder){
		free_holder = TRUE;
		holder = g_new(MidgardTypeHolder, 1);
	}

        GList *list = 
		midgard_query_builder_execute_or_count(builder, holder, MQB_SELECT_OBJECT);
	if(list == NULL) {
		if(free_holder) g_free(holder);
		return NULL;
	}

	MgdObject **objects = g_new(MgdObject *, holder->elements+1);
	
	for( ; list; list = list->next){
		objects[i] = list->data;
		i++;
	}	
	
	objects[i] = NULL;

	g_list_free(list);
	if(free_holder)	g_free(holder);

	return (GObject **)objects;	
}

guint midgard_query_builder_count(MidgardQueryBuilder *builder)
{
	g_assert(builder != NULL);
	
	MIDGARD_ERRNO_SET(builder->priv->mgd, MGD_ERR_OK);
	
	GList *list =
		midgard_query_builder_execute_or_count(builder, NULL, MQB_SELECT_GUID);

	if(list == NULL)
		return 0;

	MidgardTypeHolder *holder = (MidgardTypeHolder *)list->data;

	if(!holder)
		return 0;

	guint elements = holder->elements;
	g_free(holder);

	g_list_free(list);

	return elements;
}

GList *midgard_query_builder_get_guid(
		MidgardQueryBuilder *builder)
{
	g_assert(builder != NULL);
	

	GList *list = 
		midgard_query_builder_execute_or_count(builder, NULL, MQB_SELECT_GUID);
	
	return list;	
}


gchar *midgard_query_builder_get_object_select(
        MidgardQueryBuilder *builder, guint select_type){

        g_assert(builder != NULL);
        g_assert(builder->priv->type);

        MidgardObjectClass *klass = (MidgardObjectClass*) g_type_class_peek(builder->priv->type);
	const gchar *table = midgard_object_class_get_table(klass);

        if (midgard_object_class_get_table(klass) == NULL){
                g_warning("Object '%s' has no table or storage defined!", 
                                g_type_name(builder->priv->type));
                return NULL;
        }
	        
        GString *select = g_string_new("");

	/* We are getting only guids */
	if(select_type == MQB_SELECT_GUID){
		
		if (midgard_object_class_is_multilang (klass) && !builder->priv->unset_lang) 
			g_string_append_printf(select, " %s_i.sid AS sid, %s_i.lang AS lang ", table, table);
		else 
			g_string_append_printf(select, "COUNT(%s.guid) ", table);

		return g_string_free(select, FALSE);
	}
	
        /* guid , sitegroup hardcoded ( inherited from MgdObjectClass )
         * metadata properties hardcoded ( defined in midgard_metadata )
         */ 
        /* TODO , set switch to disable metadata ( for midgard-infectant) */
        g_string_append_printf(select, 
                        "%s.guid, %s.sitegroup, "
                        "%s.metadata_creator, "
			"IF(%s.metadata_created = '0000-00-00 00:00:00', NULL, %s.metadata_created) AS metadata_created, "
			"%s.metadata_revisor, "
                        "IF(%s.metadata_revised = '0000-00-00 00:00:00', NULL, %s.metadata_revised) AS metadata_revised, "
			"%s.metadata_revision, "
                        "%s.metadata_locker, "
			"IF(%s.metadata_locked = '0000-00-00 00:00:00', NULL, %s.metadata_locked) AS metadata_locked, "
			"%s.metadata_approver, "
			"IF(%s.metadata_approved = '0000-00-00 00:00:00', NULL, %s.metadata_approved) AS metadata_approved, "
			"%s.metadata_authors, %s.metadata_owner, "
                        "IF(%s.metadata_schedule_start = '0000-00-00 00:00:00', NULL, %s.metadata_schedule_start) AS metadata_schedule_start, "
			"IF(%s.metadata_schedule_end = '0000-00-00 00:00:00', NULL, %s.metadata_schedule_end) AS metadata_schedule_end, "
			"%s.metadata_hidden, "
                        "%s.metadata_nav_noentry, %s.metadata_size, %s.metadata_published, "
			"IF(%s.metadata_exported = '0000-00-00 00:00:00', NULL, %s.metadata_exported) AS metadata_exported, "
			"IF(%s.metadata_imported = '0000-00-00 00:00:00', NULL, %s.metadata_imported) AS metadata_imported,"
			"%s.metadata_deleted, %s.metadata_score, "
			"%s.metadata_islocked, %s.metadata_isapproved, %s",
			table, table, table, table, table, table, table, table, table,
			table, table, table, table, table, table, table, table, table,
			table, table, table, table, table, table, table, table, table,
			table, table, table, table, table, table, 
                        klass->dbpriv->storage_data->query->select_full); /* TODO, Set select which suits better */
        
        return g_string_free(select, FALSE);        
}

static void __safe_string_from_field(GValue *val, gchar *row)
{
	if(row == NULL)
		g_value_set_string(val, "");
	else 
		g_value_set_string(val, row);
}

static void __safe_uint_from_field(GValue *val, gchar *row)
{
	if(row == NULL)
		g_value_set_uint(val, 0);
	else 
		g_value_set_uint(val, atoi(row));
}

static void __safe_int_from_field(GValue *val, gchar *row)
{
	if(row == NULL)
		g_value_set_int(val, 0);
	else 
		g_value_set_int(val, atoi(row));
}

static void __safe_float_from_field(GValue *val, gchar *row)
{
	if(row == NULL)
		g_value_set_float(val, 0.0000);
	else 
		g_value_set_float(val, g_ascii_strtod(row, NULL));
}

static void __safe_bool_from_field(GValue *val, gchar *row)
{
	if(row == NULL)
		g_value_set_boolean(val, FALSE);
	else 
		g_value_set_boolean(val, atoi(row));
}

#define __safe_metadata_string(__str, __row) \
	if(__str != NULL) \
		g_free(__str); \
	if(__row == NULL || *__row == '\0') \
		__str = NULL; \
	else \
		__str = g_strdup(__row);

GList *_midgard_core_qb_set_object_from_query(MidgardQueryBuilder *builder, guint select_type, MgdObject *nobject){

        g_assert(builder != NULL);

        GParamSpec *prop;
        MgdObject *object = NULL;
        GValue pval = {0, };
        MidgardObjectClass *klass = (MidgardObjectClass*) g_type_class_peek(builder->priv->type);;
        guint ret_rows, ret_fields, i, j;
       
	gchar *sql = _midgard_core_qb_get_sql(
			builder, select_type, 
			midgard_query_builder_get_object_select(builder, select_type), select_type == MQB_SELECT_GUID ? FALSE : TRUE);

	if(!sql) {
		g_warning("Attempted to execute NULL query");
		return NULL;
	}		

	/* Multilang fallback, wrap default fallback query */
	if (select_type == MQB_SELECT_GUID && midgard_object_class_is_multilang (klass)) {	
		GString *ml = g_string_new ("SELECT COUNT(tmp.sid) FROM ( ");
		g_string_append_printf (ml, "%s ) AS tmp", sql);
		g_free(sql);
		sql = g_string_free (ml, FALSE);
	}

	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", sql);
        gint sq = mysql_query(builder->priv->mgd->msql->mysql, sql);

        if (sq != 0) {
		g_warning("\nQUERY FAILED: \n %s \n QUERY: \n %s",
				mysql_error(builder->priv->mgd->msql->mysql), sql);
		midgard_set_error(builder->priv->mgd->_mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INTERNAL,
				" SQL query failed. ");
		g_clear_error(&builder->priv->mgd->_mgd->err);	
		g_free(sql); 
		return FALSE;
        }
	g_free(sql);
      
        /* We use MySQL API directly, no mgd_query and midgard_res usage */
        MYSQL_ROW row;
        MYSQL_FIELD *field;
        MYSQL_RES *results = mysql_store_result(builder->priv->mgd->msql->mysql);
        if (!results)
                return FALSE;
        
        if ((ret_rows = mysql_num_rows(results)) == 0) {
                mysql_free_result(results);
                return NULL;
        }
       
        /* records found , allocate as many objects as many returned rows */ 
        GList *list = NULL;

	/* We count guids only */
	if(select_type == MQB_SELECT_GUID) {
	
		row = mysql_fetch_row(results);
		guint crows = atoi(row[0]);

		MidgardTypeHolder *holder = g_new(MidgardTypeHolder, 1); 
		holder->elements = crows;

		list = g_list_append(list, holder);

		mysql_free_result(results);
		return list;
	}
         
    	/* Read only mode, so initialize object, set guid and sitegroup only */	
	if (builder->priv->read_only) {

		midgard_object_mysql *__mysql = g_new0(midgard_object_mysql, 1);
		__mysql->res = results;
		__mysql->res_ref_count = ret_rows;

		for(i = 0; i < ret_rows; i++) {

			if(nobject)
				object = nobject;
			else 
				object = midgard_object_new(builder->priv->mgd, 
						g_type_name(builder->priv->type), NULL);

			row = mysql_fetch_row(results);

			object->private->read_only = TRUE;
			object->private->_mysql_row = row;		
			object->private->_mysql = __mysql;
			/* Set private properties */
			object->private->guid = g_strdup((gchar *)row[0]);
			object->private->sg = atoi(row[1]);
			object->private->exported = g_strdup((gchar *)row[19]);
			object->private->imported = g_strdup((gchar *)row[20]);
	  		list = g_list_prepend(list, G_OBJECT(object));                
        	}
        	return g_list_reverse (list);
	}

        /* Get every row */
        for(i = 0; i < ret_rows; i++){
                
                row = mysql_fetch_row(results);
                
		if(nobject)
			object = nobject;
		else 
			object = midgard_object_new(builder->priv->mgd, 
				g_type_name(builder->priv->type), NULL);

		ret_fields =  mysql_num_fields(results); 
               
		/* We set metadata properties directly , but w get 
		 * additional speed. g_object_set looses 10% of performance here */
		__safe_metadata_string(object->metadata->private->creator, (gchar *)row[2]);
		g_free(object->metadata->private->created);
		object->metadata->private->created = g_strdup((gchar *)row[3]);
		__safe_metadata_string(object->metadata->private->revisor, (gchar *)row[4]);
		g_free(object->metadata->private->revised);
		object->metadata->private->revised = g_strdup((gchar *)row[5]);
		if(row[6] != NULL)
			object->metadata->private->revision = atoi(row[6]);	
		__safe_metadata_string(object->metadata->private->locker, (gchar *)row[7]);
		g_free(object->metadata->private->locked);
		object->metadata->private->locked = g_strdup((gchar *)row[8]);
		__safe_metadata_string(object->metadata->private->approver, (gchar *)row[9]);
		g_free(object->metadata->private->approved);
		object->metadata->private->approved = g_strdup((gchar *)row[10]);
		g_free(object->metadata->private->authors);
		object->metadata->private->authors = g_strdup((gchar *)row[11]);
		g_free(object->metadata->private->owner);
		object->metadata->private->owner = g_strdup((gchar *)row[12]);
		g_free(object->metadata->private->schedule_start);
		object->metadata->private->schedule_start = g_strdup((gchar *)row[13]);
		g_free(object->metadata->private->schedule_end);
		object->metadata->private->schedule_end = g_strdup((gchar *)row[14]);
		if(row[15] != NULL)
			object->metadata->private->hidden = atoi(row[15]);
		if(row[16] != NULL)
			object->metadata->private->nav_noentry = atoi(row[16]);
		if(row[17] != NULL)
			object->metadata->private->size = atoi(row[17]);
		g_free(object->metadata->private->published);
		object->metadata->private->published = g_strdup((gchar *)row[18]);
		g_free(object->metadata->private->exported);
		object->metadata->private->exported = g_strdup((gchar *)row[19]);
		g_free(object->metadata->private->imported);
		object->metadata->private->imported = g_strdup((gchar *)row[20]);
		if(row[21] != NULL)
			object->metadata->private->deleted = atoi(row[21]);
		if(row[22] != NULL)
			object->metadata->private->score = atoi(row[22]);
		if(row[23] != NULL) {
			object->metadata->private->is_locked = atoi(row[23]);
			object->metadata->private->lock_is_set = TRUE;
		}
		if(row[24] != NULL) {
			object->metadata->private->is_approved = atoi(row[24]);
			object->metadata->private->approve_is_set = TRUE;
		}
               
		/* Set core's object private data */
		object->private->exported = g_strdup((gchar *)row[19]);
		object->private->imported = g_strdup((gchar *)row[20]);

                /* Get all fields */
                /* We can set guid and sitegroup as row[0] and row[1]
                 * and start loop from 16 to increase speed. 
                 * All in all metadata won't be changed often */
                for (j = 0; j < ret_fields; j++){
                       
			if(j < 24)
				continue;
                        field = mysql_fetch_field_direct(results, j);
                                              
                        if ((prop = g_object_class_find_property(
                                                        (GObjectClass *)klass, field->name)) != NULL) {

                                g_value_init(&pval,prop->value_type);

                                switch(prop->value_type){

                                        case G_TYPE_STRING:				
						__safe_string_from_field(&pval, (gchar *)row[j]);
                                                break; 

                                        case G_TYPE_UINT:
						__safe_uint_from_field(&pval, (gchar *)row[j]);
                                                break;

					case G_TYPE_INT:
						__safe_int_from_field(&pval, (gchar *)row[j]);
						break;

                                        case G_TYPE_FLOAT:
						__safe_float_from_field(&pval, (gchar *)row[j]);
                                                break;
                                        
                                        case G_TYPE_BOOLEAN:
						__safe_bool_from_field(&pval, (gchar *)row[j]);
                                                break;                                     
                                                
                                }
                                g_object_set_property(G_OBJECT(object), field->name, &pval);
                                g_value_unset(&pval);                                
                        }                               
                }
	
		/* Set private guid and sitegrup property */
		object->private->guid = g_strdup((gchar *)row[0]);
		object->private->sg = atoi(row[1]);

                list = g_list_prepend(list, G_OBJECT(object));                
        }

        mysql_free_result(results);
        return g_list_reverse(list);
}

gboolean midgard_query_builder_join(
		MidgardQueryBuilder *builder, const gchar *prop, 
		const gchar *jobject, const gchar *jprop)
{

	g_assert(prop != NULL);
	g_assert(jobject != NULL);
	g_assert(jprop != NULL);

	g_warning("FIXME, it's not supported");
	
	return FALSE;									
}

 
/* Returns type name of the type which is currently used by Query Builder. */
const gchar *midgard_query_builder_get_type_name(
		MidgardQueryBuilder *builder)
{
	g_assert(builder != NULL);
	return g_type_name(builder->priv->type);
}

void midgard_query_builder_include_deleted(MidgardQueryBuilder *builder)
{
	g_assert(builder);

	builder->priv->include_deleted = TRUE;
}

/* GOBJECT ROUTINES*/

static void _midgard_query_builder_finalize(GObject *object)
{
	midgard_query_builder_free(
			(MidgardQueryBuilder *)object);
}

static void _midgard_query_builder_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardQueryBuilderClass *klass = MIDGARD_QUERY_BUILDER_CLASS (g_class);
	
	gobject_class->finalize = _midgard_query_builder_finalize;
	klass->set_lang = midgard_query_builder_set_lang;
	klass->unset_languages = midgard_query_builder_unset_languages;
	klass->add_constraint = midgard_query_builder_add_constraint;
	klass->begin_group = midgard_query_builder_begin_group;
	klass->end_group = midgard_query_builder_end_group;
	klass->add_order = midgard_query_builder_add_order;
	klass->set_limit = midgard_query_builder_set_limit;
	klass->execute = midgard_query_builder_execute;
	klass->count = midgard_query_builder_count;
}

static void _midgard_query_builder_instance_init(
		GTypeInstance *instance, gpointer g_class)
{

}

/* Registers the type as  a fundamental GType unless already registered. */
GType midgard_query_builder_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardQueryBuilderClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_query_builder_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardQueryBuilder),
			0,              /* n_preallocs */
			(GInstanceInitFunc) _midgard_query_builder_instance_init /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
				"midgard_query_builder",
				&info, 0);
	}
	return type;
}

