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

#include "midgard/midgard_object.h"
#include "midgard/types.h"
#include "midgard/midgard_error.h"
#include "schema.h"
#include "midgard_core_object.h"

static MgdSchemaPropertyAttr *__get_property_attr(
		MidgardObjectClass *klass, const gchar *name)
{
	return g_hash_table_lookup(klass->dbpriv->storage_data->prophash, name);
}

/* Returns table name used for object. */
const gchar *midgard_object_class_get_table(MidgardObjectClass *klass)
{
	g_assert(klass != NULL);
	return klass->dbpriv->storage_data->table;
}

/* Returns table names used for object. */  
const gchar *midgard_object_class_get_tables(MidgardObjectClass *klass)
{
	g_assert(klass != NULL);
	
	if(klass->dbpriv->storage_data->query == NULL)
		return midgard_object_class_get_table(klass);

	if(klass->dbpriv->storage_data->query->tables == NULL)
		return  midgard_object_class_get_table(klass);

	return klass->dbpriv->storage_data->query->tables;
}

/* Returns primary field name used for object's class. */
const gchar *midgard_object_class_get_primary_field(MidgardObjectClass *klass)
{	
	g_assert(klass);
	return klass->storage_data->query->primary;
}

/* Returns table name used for property. */ 
const gchar *midgard_object_class_get_property_table(MidgardObjectClass *klass, 
		const gchar *property_name)
{
	g_assert(klass != NULL);
	
	MgdSchemaPropertyAttr *prop_attr = 
		__get_property_attr(klass, property_name);

	if(!prop_attr)
		return NULL;

	/* this is workaround, if prop_attr->table is NULL , 
	 * fallback to class table */
	if(prop_attr->table == NULL)
		return klass->storage_data->table;

	return prop_attr->table;
}

/* Returns field name used for property name. */
const gchar *midgard_object_class_get_property_field(
        MidgardObjectClass *klass, const gchar *property_name)
{
	g_assert(klass != NULL);
	
	MgdSchemaPropertyAttr *prop_attr = 
		__get_property_attr(klass, property_name);

	if(!prop_attr)
		return NULL;

	return prop_attr->field;
}

/* Returns MidgardObjectClass for linked object. */
MidgardObjectClass *midgard_object_class_get_link_target(
        MidgardObjectClass *klass, const gchar *property_name)
{	
	g_assert(klass);
	/* FIXME */
	return NULL;
}

/* Returns the name of object's primary property. */
const gchar *midgard_object_class_get_primary_property(
		MidgardObjectClass *klass)
{	
	g_assert(klass);
	return klass->storage_data->primary;
} 

/* Returns the name of object's parent property. */  
const gchar *midgard_object_class_get_property_parent(
		MidgardObjectClass *klass)
{
	g_assert(klass);
	return klass->storage_data->property_parent;	
}

const gchar *midgard_object_class_get_parent_property(
		                MidgardObjectClass *klass)
{
	return midgard_object_class_get_property_parent(klass);
}


/* Returns the name of object's up property. */
const gchar *midgard_object_class_get_property_up(
		MidgardObjectClass *klass)
{	
	g_assert(klass);
	return klass->storage_data->property_up;
}

const gchar *midgard_object_class_get_up_property(
		                MidgardObjectClass *klass)
{ 
	return midgard_object_class_get_property_up(klass);
}

/* Returns class' lang usage information. */
gboolean midgard_object_class_is_multilang(
		MidgardObjectClass *klass)
{
    g_assert(klass != NULL);

    if(klass->dbpriv->storage_data->use_lang == 0)
        return FALSE;

    return TRUE;
}

/* Returns children ( in tree ) classes' pointers */
MidgardObjectClass **midgard_object_class_list_children(
		        MidgardObjectClass *klass)
{
	g_assert(klass);

	if(!klass->storage_data->childs)
		return NULL;
	
	GSList *slist = klass->storage_data->childs;
	guint i = 0;
	MidgardObjectClass **children = 
		g_new(MidgardObjectClass *, g_slist_length(slist)+1);

	for(; slist; slist = slist->next){
		children[i] = 
			MIDGARD_OBJECT_GET_CLASS_BY_NAME(
					g_type_name((GType)slist->data));
		i++;

	}
	children[i] = NULL;

	return children;
}

/* Returns Midgard Object identified by guid */
MgdObject *midgard_object_class_get_object_by_guid (	MidgardConnection *mgd,
							const gchar *guid)
{
	g_assert(mgd != NULL);
	g_assert(guid);

	if(guid == NULL) {
		g_warning("Can not get object by guid = NULL");
		return NULL;
	}

	midgard_res *res;
	MgdObject *object;
	
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);
	GString *sql = g_string_new("SELECT ");
	g_string_append_printf(sql,
			"typename, object_action FROM repligard "
			"WHERE guid = '%s' ",
			guid);

	if(!mgd_isroot(mgd->mgd)){
		g_string_append_printf(sql,
				" AND sitegroup IN (0, %d)",
				mgd_sitegroup(mgd->mgd));
	}
	
	gchar *query = g_string_free(sql, FALSE);
	res = mgd_query(mgd->mgd, query);
	g_free(query);
	GValue pval = {0, };

	if (!res || !mgd_fetch(res)) {
		if(res) mgd_release(res);
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_NOT_EXISTS);
		return NULL;
	}

	if(mgd_isroot(mgd->mgd)
			&& (mgd_rows(res) > 1)) {
		mgd_release(res);
		MIDGARD_ERRNO_SET(mgd->mgd,
				MGD_ERR_SITEGROUP_VIOLATION);
		return NULL;
	}

	switch((gint)atoi(mgd_colvalue(res, 1))) {
		
		case MGD_OBJECT_ACTION_DELETE:
			mgd_release(res);
			MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OBJECT_DELETED);
			return NULL;
			break;

		case MGD_OBJECT_ACTION_PURGE:
			mgd_release(res);
			MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OBJECT_PURGED);
			return NULL;
			break;

		default:
			g_value_init(&pval, G_TYPE_STRING);
			g_value_set_string(&pval, guid);
			object = midgard_object_new(mgd->mgd, 
					mgd_colvalue(res, 0),
					&pval);
			g_value_unset(&pval);
			mgd_release(res);
			if(object){
				return object;
			} else {
				MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_NOT_EXISTS);
				return NULL;
			}
			break;

	}
	return NULL;
}

gboolean midgard_object_class_undelete(MidgardConnection *mgd, const gchar *guid)
{
	g_assert(mgd);
	g_assert(guid);

	midgard_res *res;
	MidgardObjectClass *klass;
	gint rv;
	
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);
	GString *sql = g_string_new("SELECT ");
	g_string_append_printf(sql,
			"typename, object_action FROM repligard "
			"WHERE guid = '%s' ",
			guid);
	
	if(!mgd_isroot(mgd->mgd)){
		g_string_append_printf(sql,
				" AND sitegroup IN (0, %d)",
				mgd_sitegroup(mgd->mgd));
	}
	
	res = mgd_query(mgd->mgd, sql->str);
	g_string_free(sql, TRUE);
		
	if (!res || !mgd_fetch(res)) {
		if(res) mgd_release(res);
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_NOT_EXISTS);
		return FALSE;
	}
	
	switch((gint)atoi(mgd_colvalue(res, 1))) {

		case MGD_OBJECT_ACTION_DELETE:
			klass = 
				MIDGARD_OBJECT_GET_CLASS_BY_NAME(
						(const gchar *)mgd_colvalue(res, 0));
			sql = g_string_new("UPDATE ");
			g_string_append_printf(sql, 
					"%s SET metadata_deleted=FALSE "
					"WHERE guid = '%s'",
					midgard_object_class_get_table(klass),
					guid);

			if(!mgd_isroot(mgd->mgd)){
				g_string_append_printf(sql,
						" AND sitegroup = %d",
						mgd_sitegroup(mgd->mgd));
			}

			rv = mgd_exec(mgd->mgd, sql->str);
			g_string_free(sql, TRUE);
			
			/* FIXME, move to libgda, this part of legacy API is broken! */
			/* Or MySQL changed its API recently */
			/* 0 means failure. I do not want to fix this in legacy code */
			if(rv == 0) {
				MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
				return FALSE;
			}
			
			sql = g_string_new("UPDATE repligard SET ");
			g_string_append_printf(sql,
					"object_action = %d "
					"WHERE guid = '%s' AND typename = '%s'"
					"AND sitegroup = %d",
					MGD_OBJECT_ACTION_UPDATE,
					guid,
					(gchar *) mgd_colvalue(res, 0),
					mgd_sitegroup(mgd->mgd));
			
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", sql->str);
			mgd_exec(mgd->mgd, sql->str);
			g_string_free(sql, TRUE);

			return TRUE;			
			break;

		case MGD_OBJECT_ACTION_PURGE:
			mgd_release(res);
			MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OBJECT_PURGED);
			return FALSE;

		case MGD_OBJECT_ACTION_NONE:
			mgd_release(res);
			midgard_set_error(mgd,
					MGD_GENERIC_ERROR,
					MGD_ERR_USER_DATA,
					"Requested object is not deleted");
			g_clear_error(&mgd->err);
			return FALSE;

		default:
			MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
			return FALSE;
	}

	return FALSE;
}

const gchar *
midgard_object_class_get_schema_value (MidgardObjectClass *klass, const gchar *name) 
{
	g_return_val_if_fail (klass != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);
	g_return_val_if_fail (MIDGARD_IS_OBJECT_CLASS (klass), NULL);

	MgdSchemaTypeAttr *type_attr = MIDGARD_DBOBJECT_CLASS (klass)->dbpriv->storage_data;

	if (!type_attr)
		return NULL;

	return g_hash_table_lookup (type_attr->user_values, (gpointer) name);
}
