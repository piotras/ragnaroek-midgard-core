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
 */

#include <stdlib.h>
#include <midgard/midgard_quota.h>
#include <midgard/query.h>
#include <midgard/midgard_error.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "schema.h"
#include "midgard_core_object.h"
#include "midgard/midgard_metadata.h"

#define get_varchar_size(str) \
	if(str == NULL) \
		size++; \
	else \
		size = strlen(str) + 1 + size; \
	g_free(str);
	
#define get_datetime_size(str) \
	size = 8 + size; \
	g_free(str);

gboolean midgard_quota_size_is_reached(MgdObject *object, gint size)
{
	GString *query;
	gint limit_tmp_size, tmp_size;
	GList *list;
	midgard *mgd = object->mgd;
	MidgardConnection *new_mgd = mgd->_mgd;
	if (!MGD_CNC_QUOTA (new_mgd))
		return FALSE;

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	const gchar *table = midgard_object_class_get_table(klass);

	/* First we check old quota */
	if (mgd->quota && mgd_sitegroup(mgd) > 0) {
		if (!mgd_check_quota(mgd, table)) {
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_QUOTA);
			return TRUE;
		}
	}		

	/* Check global quota */
	guint sgid;
	g_object_get(G_OBJECT(object), "sitegroup", &sgid, NULL);
	query = g_string_new("SELECT limit_sg_size, sg_size FROM quota ");
	g_string_append_printf(query,
			"WHERE typename='' and sitegroup=%d",
			sgid);

	list = midgard_query_get_single_row(g_string_free(query, FALSE), object);
	
	/* This means that quota is enabled but quota limits are not set
	 * return FALSE then*/
	if(!list)
		return FALSE;
	
	limit_tmp_size = atoi(list->data);
	g_free(list->data);
	list = list->next;
	tmp_size = atoi(list->data);
	g_free(list->data);
	g_list_free(list);

	if(((tmp_size + size) > limit_tmp_size) && limit_tmp_size > 0){
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_QUOTA);
		return TRUE;
	}
	
	/* Check quota for type size */
	query = g_string_new("SELECT limit_type_size, type_size FROM quota ");
	g_string_append_printf(query,
			"WHERE typename='%s' and sitegroup=%d",
			G_OBJECT_TYPE_NAME(object), 
			midgard_sitegroup_get_id(object->mgd));

	list = midgard_query_get_single_row(g_string_free(query, FALSE), object);

	/* This means that quota is enabled and quota limits are not set
	 * return FALSE then*/
	if(!list)
		return FALSE;
	
	limit_tmp_size = atoi(list->data);
	g_free(list->data);
	list = list->next;
	tmp_size = atoi(list->data);
	g_free(list->data);
	
	g_list_free(list);
	
	if(((tmp_size + size) > limit_tmp_size) && limit_tmp_size > 0){
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_QUOTA);
		return TRUE;
	}

	return FALSE;
}


/*
 * Returns object's disk usage size 
 */ 
guint midgard_quota_get_object_size(MgdObject *object){

	g_assert(object != NULL);
	MidgardConnection *mgd = MGD_OBJECT_CNC (object);

	if (!MGD_CNC_QUOTA (mgd))
		return 0;

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	GString *select = g_string_new("SELECT metadata_size");
	g_string_append_printf(select, 
			" FROM %s WHERE %s=",
			midgard_object_class_get_table(klass),				
			midgard_object_class_get_primary_field(klass));
			
	GParamSpec *prop = 
		g_object_class_find_property(
				G_OBJECT_GET_CLASS(G_OBJECT(object)),
				midgard_object_class_get_primary_property(klass));

	if(prop == NULL)
		g_error("Primary property not found for %s", G_OBJECT_TYPE_NAME(object));

	GValue pval = {0, };
	g_value_init(&pval, prop->value_type);
	g_object_get_property(G_OBJECT(object),
			midgard_object_class_get_primary_property(klass),
			&pval);

	if(prop->value_type == G_TYPE_UINT) {
		g_string_append_printf(select, 
				"%d",
				g_value_get_uint(&pval));				
	} else {
		g_string_append_printf(select,
				"'%s'",
				g_value_get_string(&pval));
	}
	g_value_unset(&pval);
	
	/* there is no need to get SG0 size */
	guint sgid;
	g_object_get(G_OBJECT(object), "sitegroup", &sgid, NULL);
	g_string_append_printf(select, 
			" AND sitegroup=%d",
			sgid);
	
	gchar *query = g_string_free(select, FALSE);
	GList *list = midgard_query_get_single_row(query, object);
	
	if(list == NULL)
		return 0;

	guint size = atoi(list->data);

	g_free(list->data);
	g_list_free(list);

	return size;
}

/*
 * Updates object's storage setting object's size
 * WARNING, this is MySQL optimized!	
 */
gboolean midgard_quota_update(MgdObject *object)
{
	g_assert(object != NULL);
	g_assert(object->mgd != NULL);

	MidgardConnection *mgd = MGD_OBJECT_CNC (object);
	if (!MGD_CNC_QUOTA (mgd))
		return TRUE;

	guint size = 0;
	guint32 qsize = 0;
	const gchar *typename = G_OBJECT_TYPE_NAME(object);
	GValue pval = {0, };

	g_object_get(G_OBJECT(object->metadata), "size", &size, NULL);
		
	/* nothing to update, this should never happen */
	if(size == 0) return TRUE;

	/* Get difference */
	/* difference is important to allow multiple updates without wrong size 
	 * diff_size may be less then , equal or greater than 0 */
	qsize = midgard_quota_get_object_size(object);
	gint diff_size = size - qsize;	

	if(midgard_quota_size_is_reached(object, diff_size)){
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_QUOTA);
		return FALSE;
	}

	guint32 opsize;
	opsize = diff_size + qsize;
	
	/* diff size is 0 , so no need update record without changing its value */
	if(diff_size == 0) 
		return TRUE;

	object->metadata->private->size = opsize;
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);	
	
	GString *query = g_string_new("UPDATE ");
	g_string_append_printf(query, 
			"%s SET metadata_size=metadata_size+%d "
			"WHERE %s=",
			midgard_object_class_get_table(klass),
			diff_size,
			midgard_object_class_get_primary_field(klass));
	
	GParamSpec *prop = g_object_class_find_property(
			G_OBJECT_GET_CLASS(G_OBJECT(object)),
			midgard_object_class_get_primary_property(klass));
	
	g_value_init(&pval, prop->value_type);
	g_object_get_property(G_OBJECT(object),
			midgard_object_class_get_primary_property(klass),
			&pval);
	
	if(prop->value_type == G_TYPE_UINT) {
		g_string_append_printf(query,
				"%d",
				g_value_get_uint(&pval));
	} else {
		g_string_append_printf(query,
				"'%s'",
				g_value_get_string(&pval));
	}
	g_value_unset(&pval);	
	midgard_query_execute(object->mgd, g_string_free(query, FALSE), NULL);

	/* Update type size */
	GString *quota_query;
	guint sgid;
	g_object_get(G_OBJECT(object), "sitegroup", &sgid, NULL);
	quota_query = g_string_new("UPDATE quota ");
	g_string_append_printf(quota_query, 
			"SET type_size=type_size+%d "
			"WHERE typename='%s' AND sitegroup=%d",
			diff_size,
			typename,
			sgid);
	midgard_query_execute(object->mgd, g_string_free(quota_query, FALSE), NULL);

	/* Update global sitegroup size */
	quota_query = g_string_new("UPDATE quota ");
	g_string_append_printf(quota_query,
			"SET sg_size=sg_size+%d "
			"WHERE typename='' AND sitegroup=%d",
			diff_size,
			sgid);
	midgard_query_execute(object->mgd, g_string_free(quota_query, FALSE), NULL);


	return TRUE;
}

gboolean midgard_quota_create(MgdObject *object)
{
	g_assert(object != NULL);

	MidgardConnection *mgd = MGD_OBJECT_CNC (object);
	if (!MGD_CNC_QUOTA (mgd))
		return TRUE;

	GString *query;
	gint tmp_type_limit, tmp_limit;
	GList *list;
	guint object_sitegroup;

	g_object_get(G_OBJECT(object), "sitegroup", &object_sitegroup, NULL);
	
	/* Check quota for global records limit*/
	query = g_string_new("SELECT limit_sg_records, sg_records FROM quota ");
	g_string_append_printf(query,
			"WHERE typename='' and sitegroup=%d",
			object_sitegroup);

	list = midgard_query_get_single_row(g_string_free(query, FALSE), object);
	
	/* This means that quota is enable but quota limits are not set 
	 * return TRUE then */
	if(!list)
		return TRUE;
	
	
	tmp_type_limit = atoi(list->data);
	g_free(list->data);
	list = list->next;
	tmp_limit = atoi(list->data);
	g_free(list->data);
	
	g_list_free(list);
	
	if(((tmp_limit + 1) > tmp_type_limit) && tmp_type_limit > 0){
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_QUOTA);
		return FALSE;
	}

	/* Check quota for type's records limit*/
	query = g_string_new("SELECT limit_type_records, type_records FROM quota ");
	g_string_append_printf(query,
			"WHERE typename='%s' and sitegroup=%d",
			G_OBJECT_TYPE_NAME(object), 
			object_sitegroup);

	list = midgard_query_get_single_row(g_string_free(query, FALSE) , object);
	
	/* This means that quota is enabled but quota limits are not set 
	 * return TRUE then */
	if(!list)
		return TRUE;
	
	tmp_type_limit = atoi(list->data);
	g_free(list->data);
	list = list->next;
	tmp_limit = atoi(list->data);
	g_free(list->data);
	
	g_list_free(list);
	
	if(((tmp_limit + 1) > tmp_type_limit) && tmp_type_limit > 0){
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_QUOTA);
		return FALSE;
	}

	guint32 size;
	g_object_get(G_OBJECT(object->metadata), "size", &size, NULL);
	if(midgard_quota_size_is_reached(object, size)){
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_QUOTA);
		return FALSE;
	}

	/* Update type's records */
	query = g_string_new("UPDATE quota ");
	g_string_append_printf(query,
			"SET type_records=type_records+1"
			" WHERE typename='%s' AND sitegroup=%d",
			G_OBJECT_TYPE_NAME(object), 
			object_sitegroup);

	midgard_query_execute(object->mgd, g_string_free(query, FALSE), NULL);

	/* Update global sitegroup records */
	query = g_string_new("UPDATE quota ");
	g_string_append_printf(query,
			"SET sg_records=sg_records+1"
			" WHERE typename='' AND sitegroup=%d",
			object_sitegroup);

	midgard_query_execute(object->mgd, g_string_free(query, FALSE), NULL);

	return TRUE;
}

guint midgard_object_get_size(MgdObject *object)
{
	g_assert(object != NULL);
	g_assert(object->mgd != NULL);

	MidgardConnection *mgd = MGD_OBJECT_CNC (object);
	if (!MGD_CNC_QUOTA (mgd))
		return 0;

	/* This is overloaded function 
	 * Should be absolutely changed when midgard type system is done 
	 */
	guint propn, size = 0;
	const gchar *schematype, *blobdir, *strprop;
	const gchar *typename = G_OBJECT_TYPE_NAME(object);
	gchar *meta_strprop = NULL, *blobpath, *location;
	guint meta_intprop, i;
	GValue pval = {0, };
	struct stat statbuf; /* GLib 2.6 is not available everywhere, g_stat not used */

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	
	if(g_ascii_strcasecmp(typename, "midgard_attachment") == 0){
		
		blobdir = mgd_get_blobdir(object->mgd);

		if (!blobdir || (*blobdir != '/')
				|| (stat(blobdir, &statbuf) != 0)
				|| !S_ISDIR(statbuf.st_mode)) {
		g_error("Invalid blobdir or blobdir is not set");
		}
				
		g_object_get(G_OBJECT(object), "location", &location, NULL);
	
		/* I "think" it's update method now. It poor solution 
		 * till there's no real attachment support in core */
		if(strlen(location) > 1) {

			blobpath = g_strconcat(
					blobdir,
					"/",
					location,
					NULL);
			
			if((stat(blobpath, &statbuf) == 0) || (!S_ISDIR(statbuf.st_mode)))
				size = statbuf.st_size;
			
			g_free(blobpath);
		}
	}
		
	GParamSpec **props = g_object_class_list_properties(
			G_OBJECT_GET_CLASS(G_OBJECT(object)), &propn);
	
	if(props == NULL){
		g_warning("Object size: Object %s has no properties",
				G_OBJECT_TYPE_NAME(object));
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INVALID_OBJECT);
		return FALSE;
	}
	
	for(i = 0; i < propn; i++){

		/* midgard_metadata object */
		if(g_ascii_strcasecmp(props[i]->name, "metadata") == 0){

			/* creator */
			g_object_get(G_OBJECT(object->metadata), "creator", &meta_strprop, NULL);
			get_varchar_size(meta_strprop);
			/* created */
			g_object_get(G_OBJECT(object->metadata), "created", &meta_strprop, NULL);
			get_datetime_size(meta_strprop);
			/* revisor */
			g_object_get(G_OBJECT(object->metadata), "revisor", &meta_strprop, NULL);
			get_varchar_size(meta_strprop);
			/* revised */
			g_object_get(G_OBJECT(object->metadata), "revised", &meta_strprop, NULL);
			get_datetime_size(meta_strprop);
     			/* revision */
			g_object_get(G_OBJECT(object->metadata), "revision", &meta_intprop, NULL);
			size = 4 + size;
			/* locker */
			g_object_get(G_OBJECT(object->metadata), "locker", &meta_strprop, NULL);
			get_varchar_size(meta_strprop);
			/* locked */
			g_object_get(G_OBJECT(object->metadata), "locked", &meta_strprop, NULL);
			get_datetime_size(meta_strprop);
			/* approver */
			g_object_get(G_OBJECT(object->metadata), "approved", &meta_strprop, NULL);
			get_varchar_size(meta_strprop);
			/* approved */
			g_object_get(G_OBJECT(object->metadata), "approved", &meta_strprop, NULL);
			get_datetime_size(meta_strprop);
			/* authors */
			g_object_get(G_OBJECT(object->metadata), "authors", &meta_strprop, NULL);
			if(meta_strprop == NULL)
				meta_strprop = g_strdup("");
			size = strlen(meta_strprop) + 4 + size;
			g_free(meta_strprop);
			/* owner */
			g_object_get(G_OBJECT(object->metadata), "owner", &meta_strprop, NULL);
			get_varchar_size(meta_strprop);
			/* schedule-start */
			g_object_get(G_OBJECT(object->metadata), "schedulestart", &meta_strprop, NULL);
			get_datetime_size(meta_strprop);
			/* schedule-end */
			g_object_get(G_OBJECT(object->metadata), "scheduleend", &meta_strprop, NULL);
			get_datetime_size(meta_strprop);
     			/* hidden */
			g_object_get(G_OBJECT(object->metadata), "hidden", &meta_intprop, NULL);
			size = size;
     			/* nav-noentry */
			g_object_get(G_OBJECT(object->metadata), "navnoentry", &meta_intprop, NULL);
			size =  size;	

		} else {	

			/* Follow all custom properties */
			g_value_init(&pval, props[i]->value_type);
			
			switch (props[i]->value_type){

				case G_TYPE_FLOAT:
				case G_TYPE_UINT:
					size =  4 + size;
					break;

				case G_TYPE_BOOLEAN:
					size = size; /* Piotras: I didn't find bool size */
					break;
					
				case G_TYPE_STRING:

					g_object_get_property(G_OBJECT(object), props[i]->name, &pval);
					
					/* FIXME , move to data->gtype if midgard fundamental 
					 * types can not do this work */
					MgdSchemaPropertyAttr *prop_attr = 
						g_hash_table_lookup(klass->storage_data->prophash,
								props[i]->name);
					schematype = NULL;
					if(prop_attr != NULL)
						schematype = prop_attr->type; 
											
					/* default one is string (varchar) */
					if((schematype == NULL)){

						strprop = g_value_get_string(&pval);
						if(strprop == NULL)
							size++;
						else
							size = (strlen(strprop)) + 1 + size;

						schematype = "";					
					}
						
					if(g_ascii_strcasecmp(schematype, "longtext") == 0){
						strprop = g_value_get_string(&pval);
						if(strprop == NULL)
							strprop = "";
						size = (strlen(strprop)) + 4 + size;
					}

					if(g_ascii_strcasecmp(schematype, "text") == 0){
						strprop = g_value_get_string(&pval);
						if(strprop == NULL)
							strprop = "";
						size = (strlen(strprop)) + 1 + size;
					}
					
					if(g_ascii_strcasecmp(schematype, "string") == 0){
						strprop = g_value_get_string(&pval);                                                                       if(strprop == NULL)
							strprop = "";
						size = (strlen(strprop)) + 1 + size;
					}
										
					

					if(g_ascii_strcasecmp(schematype, "date") == 0)
						size =  3 + size;
					
					if(g_ascii_strcasecmp(schematype, "datetime") == 0)
						size =  8 + size;
					
					break;


			}
			g_value_unset(&pval);
		}	
	}

	g_free(props);

	return size;
}

void midgard_quota_remove(MgdObject *object, guint size){

	g_assert(object != NULL);
	
	MidgardConnection *mgd = MGD_OBJECT_CNC (object);
	if (!MGD_CNC_QUOTA (mgd))
		return;

	GString *query;
	const gchar *typename = G_OBJECT_TYPE_NAME(object);
	
	/* Update type's records */
	query = g_string_new("UPDATE quota ");
	g_string_append_printf(query,
			"SET type_records=type_records-1"
			" WHERE typename='%s' AND sitegroup=%d AND type_records>0",
			typename,
			midgard_sitegroup_get_id(object->mgd));
	midgard_query_execute(object->mgd, g_string_free(query, FALSE), NULL);

	/* Update global sitegroup records */
	query = g_string_new("UPDATE quota ");
	g_string_append_printf(query,
			"SET sg_records=sg_records-1"
			" WHERE typename='' AND sitegroup=%d AND sg_records>0",
			midgard_sitegroup_get_id(object->mgd));
	
	midgard_query_execute(object->mgd, g_string_free(query, FALSE), NULL);

	query = g_string_new("UPDATE quota ");
	g_string_append_printf(query,
			"SET type_size=type_size-%d "
			"WHERE typename='%s' AND sitegroup=%d AND type_size>0",
			size,
			typename,
			midgard_sitegroup_get_id(object->mgd));
	midgard_query_execute(object->mgd, g_string_free(query, FALSE), NULL);

	/* Update global sitegroup size */
	query = g_string_new("UPDATE quota ");
	g_string_append_printf(query,
			"SET sg_size=sg_size-%d "
			"WHERE typename='' AND sitegroup=%d AND sg_size>0",
			size,
			midgard_sitegroup_get_id(object->mgd));
	midgard_query_execute(object->mgd, g_string_free(query, FALSE), NULL);

}

typedef struct{
	guint sg;
	guint32 size;
	midgard *mgd;
}_qts;


static void _get_type_global_size(gpointer k, gpointer v, gpointer ud)
{
	const gchar *typename = (gchar *)k;
	_qts *qts = (_qts *)ud;

	MgdObject *object = midgard_object_new(qts->mgd, typename, NULL);
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);

	guint propn, i;
	GParamSpec **props = g_object_class_list_properties(
			G_OBJECT_GET_CLASS(G_OBJECT(object)), &propn);

	if(props == NULL) return;
	
	GString *tmp = g_string_new("");
	const gchar *nick;
	
	for(i = 0; i < propn; i++){
		
		nick = g_param_spec_get_nick(props[i]);
		if(nick && strlen(nick) > 0){

			g_string_append_printf(tmp,
					"%s,",
					nick);
		}
	}

	g_free(props);
	
	gchar *concat = g_string_free(tmp, FALSE);
	guint conlen = strlen(concat);

	if(conlen < 1){
		g_free(concat);
		return;
	}
	
	concat[conlen-1] = '\0';
	
	tmp = g_string_new("SELECT SUM(LENGTH(CONCAT(");
	g_string_append_printf(tmp,
			"%s))) FROM %s ",
			concat,
			midgard_object_class_get_tables(klass));
	g_string_append(tmp, " WHERE ");
	g_string_append_printf(tmp,
			"%s.sitegroup=%d",
			midgard_object_class_get_table(klass),
			qts->sg);

	if(midgard_object_class_is_multilang(klass)){

		g_string_append_printf(tmp,
				" AND %s.id=%s_i.sid ",
				midgard_object_class_get_table(klass),
				midgard_object_class_get_table(klass));
	}
	
	g_free(concat);
	midgard_res *res;
	gchar *query = g_string_free(tmp, FALSE);
	res = mgd_query(qts->mgd, query);
	if(res && mgd_fetch(res)){
		qts->size = qts->size + (guint)mgd_sql2id(res,0);
		if(res) mgd_release(res);
	}	
	g_free(query);

	if(g_ascii_strcasecmp(typename, "midgard_attachment") == 0){

		struct stat buf;
		gchar *path;
		const gchar *location;
		
		res = mgd_query(qts->mgd, 
				"SELECT location FROM blobs WHERE sitegroup = $d",
				qts->sg);
		if (res) {
			while (mgd_fetch(res)) {
				location = mgd_colvalue(res,0);
				if(location){
					path = g_strconcat(
							mgd_get_blobdir(qts->mgd),
							"/",
							location, NULL);
					stat(path, &buf);
					qts->size = qts->size + buf.st_size;
					g_free(path);
				}
			}
			mgd_release(res);
		}
	}
	g_object_unref(object);
}


guint32 midgard_quota_get_sitegroup_size(midgard *mgd, guint sg)
{

	g_assert(mgd != NULL);
	if(sg == 0) return 0;

	if(!mgd_isroot(mgd))
		sg = mgd_sitegroup(mgd);
	
	guint32 size = 0;
	
	_qts *qts = g_new(_qts,1);
	qts->size = 0;
	qts->sg = sg;
	qts->mgd = mgd;

	g_hash_table_foreach(mgd->schema->types, _get_type_global_size, qts);

	size = qts->size;
	g_free(qts);
	return size;
}


