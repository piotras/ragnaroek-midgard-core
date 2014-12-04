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

#include "midgard/midgard_replicator.h"
#include "midgard_core_object.h"
#include "midgard/midgard_object.h"
#include "midgard_mysql.h"
#include <sys/stat.h>
#include "midgard/midgard_blob.h"
#include "midgard/midgard_timestamp.h"
#include "midgard/midgard_error.h"

struct _MidgardReplicatorPrivate 
{ 
	MidgardDBObject *object;
	MidgardObjectClass *klass;
	GType type;
	const gchar *typename;	
	MidgardConnection *mgd;
};

/* Creates new MidgardReplicator instance for the given Midgard Object. */
MidgardReplicator *midgard_replicator_new(MgdObject *object)
{
	g_assert(object != NULL);

	MidgardReplicator *self = 
		g_object_new(MIDGARD_TYPE_REPLICATOR, NULL);

	if(object) {
		
		g_assert(object->mgd->_mgd != NULL);

		if(MIDGARD_IS_OBJECT(object)){
			self->private->object = MIDGARD_DBOBJECT(object);
			self->private->klass = 
				MIDGARD_OBJECT_GET_CLASS(object);
			self->private->type = 
				G_OBJECT_TYPE(G_OBJECT(object));
			self->private->typename = 
				G_OBJECT_TYPE_NAME(G_OBJECT(object));
			self->private->mgd = object->mgd->_mgd;
		} else {
			g_warning("Replicator constructor: object is not Midgard Object");
			g_object_unref(self);
			return NULL;
		}
	
	} else {

		/* self->private->mgd = mgd; */
	}

	return self;
}

/* Returns serialized objects as xml content. */ 
gchar *midgard_replicator_serialize(MidgardReplicator *self, 
		GObject *object) 
{
	if(!self) {
		
		if(object && G_IS_OBJECT(object))
			return _midgard_core_object_to_xml(G_OBJECT(object));
		else 
			g_warning("Can not serialize. Given object is either NULL or not GObject derived");
	
	} else {

		if(object) {
			g_warning("Ignoring object argument. Another object already assigned with replicator");
			return NULL;
		}

		return _midgard_core_object_to_xml(G_OBJECT(object));
	}

	return NULL;
}

/* Export object identified by the given guid. */
gboolean midgard_replicator_export(	MidgardReplicator *self, 
					MidgardDBObject *object) 
{
	if(self && object){
		g_warning("midgard_replicator already initialized with %s",
				self->private->typename);
		return FALSE;
	}

	MidgardDBObject *_object;
	MidgardConnection *mgd = NULL;
	const gchar *guid = NULL;

	if(!object)
		_object = self->private->object;
	else 
		_object = object;

	if(_object){
		
		mgd = MGD_OBJECT_CNC(_object);
		guid = MGD_OBJECT_GUID(_object);

		if(guid == NULL){
			midgard_set_error(mgd,
					MGD_GENERIC_ERROR,
					MGD_ERR_INVALID_PROPERTY_VALUE,
					"Empty guid value! ");
			g_warning("%s", mgd->errstr);
			g_clear_error(&mgd->err);
			return FALSE;
		}
	}

	g_signal_emit(_object, MIDGARD_OBJECT_GET_CLASS(_object)->signal_action_export, 0);
	
	/* FIXME, invoke class method here */
	if(MIDGARD_IS_OBJECT(_object))
		return _midgard_object_update(MIDGARD_OBJECT(_object), OBJECT_UPDATE_EXPORTED);

	return FALSE;
}

/* Marks as exported the object which is identified by the given guid. */
gboolean midgard_replicator_export_by_guid (MidgardConnection *mgd, const gchar *guid)
{
	if(guid == NULL) {
		g_warning("Can not identify object by NULL guid");
		MIDGARD_ERRNO_SET(mgd->mgd,
				MGD_ERR_INVALID_PROPERTY_VALUE);
		return FALSE;
	}

	midgard_res *res;	
	
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);
	GString *sql = g_string_new("SELECT ");
	g_string_append_printf(sql,
			"typename, object_action FROM repligard "
			"WHERE guid = '%s' ",
			guid);

	if(!mgd_isroot(mgd->mgd)){
		g_string_append_printf(sql,
				" AND sitegroup = %d",
				mgd_sitegroup(mgd->mgd));
	}

	gchar *query = g_string_free(sql, FALSE);
	res = mgd_query(mgd->mgd, query);
	g_free(query);

	if (!res || !mgd_fetch(res)) {
		if(res) mgd_release(res);
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_NOT_EXISTS);
		return FALSE;
	}
	
	if(mgd_isroot(mgd->mgd) 
			&& (mgd_rows(res) > 1)) {
		
		mgd_release(res);
		MIDGARD_ERRNO_SET(mgd->mgd, 
				MGD_ERR_SITEGROUP_VIOLATION);
		return FALSE;
	}

	MidgardObjectClass *klass = 
		MIDGARD_OBJECT_GET_CLASS_BY_NAME((gchar *)mgd_colvalue(res, 0));

	if(klass == NULL) {
		
		g_warning("Failed to get class pointer for '%s', an object identified with '%s'", (gchar *)mgd_colvalue(res, 0), guid);
		if(res)
			mgd_release(res);
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		return FALSE; 
	}

	const gchar *table = 
		midgard_object_class_get_table(klass);
	GValue tval = {0, };
	gchar *timeupdated, *tmpstr;
	gint qr;

	switch((gint)atoi(mgd_colvalue(res, 1))) {
		
		case MGD_OBJECT_ACTION_PURGE:
			mgd_release(res);
			MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OBJECT_PURGED);
			return FALSE;
			break;

		default:
			mgd_release(res);
			sql = g_string_new("UPDATE ");
			g_string_append_printf(sql,
					"%s SET ",
					table);

			g_value_init(&tval, midgard_timestamp_get_type());
			midgard_timestamp_set_time(&tval, time(NULL));
			timeupdated = midgard_timestamp_dup_string(&tval);
			
			g_string_append_printf(sql,
					"metadata_exported='%s' "
					"WHERE guid = '%s' AND sitegroup = %d", 
					timeupdated,
					guid, mgd_sitegroup(mgd->mgd));

			tmpstr = g_string_free(sql, FALSE);

			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", tmpstr);
			qr = mysql_query(mgd->mgd->msql->mysql, tmpstr);
			g_free(tmpstr);
			
			if (qr != 0) {
				g_warning("query failed: %s",
						mysql_error(mgd->mgd->msql->mysql));
				MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
				return FALSE;
			}

			return TRUE;

			break;
	}

	return FALSE;
}

/* Export purged objects. */ 
gchar *midgard_replicator_export_purged(	MidgardReplicator *self, 
						MidgardObjectClass *klass, 
						MidgardConnection *mgd,
						const gchar *startdate, 
						const gchar *enddate)
{
	if(self && klass){
		g_warning("midgard_replicator already initialized with %s",
				self->private->typename);
		return NULL;
	}

	MidgardConnection *_mgd;
	MidgardObjectClass *_klass;

	if(self == NULL){
		_klass = klass;
		_mgd = mgd;
	} else {
		_klass = self->private->klass;
		_mgd = self->private->mgd;
	}

	GString *sql = g_string_new(" SELECT guid, object_action_date ");
	g_string_append_printf(sql, " FROM repligard where typename = '%s' "
			" AND object_action = %d",
			G_OBJECT_CLASS_NAME(_klass),
			MGD_OBJECT_ACTION_PURGE);

	if(!mgd_isroot(_mgd->mgd)) {
		g_string_append_printf(sql, " AND sitegroup = %d ",
				mgd_sitegroup(_mgd->mgd));
	}
	
	if(startdate != NULL){
		g_string_append_printf(sql, " AND object_action_date > '%s' ",
				startdate);
	}

	if(enddate != NULL) {
		g_string_append_printf(sql, " AND object_action_date < '%s' ",
				enddate);
	}

	gchar *tmpstr = g_string_free(sql, FALSE);
	
	gint sq = mysql_query(_mgd->mgd->msql->mysql, tmpstr);	
	if (sq != 0) {
		g_warning("\n\nQUERY: \n %s \n\n FAILED: \n %s",
				tmpstr,
				mysql_error(_mgd->mgd->msql->mysql));
		midgard_set_error(_mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INTERNAL,
				" SQL query failed. ");
		g_clear_error(&_mgd->err);
		g_free(tmpstr);
		return NULL;
	}
	g_free(tmpstr);

	MYSQL_ROW row;
	gint ret_rows, i;
	MYSQL_RES *results = mysql_store_result(_mgd->mgd->msql->mysql);
	if (!results)
		return NULL;
	
	if ((ret_rows = mysql_num_rows(results)) == 0) {
		mysql_free_result(results);
		return NULL;
	}

	xmlNode *object_node;
	xmlDoc *doc = 
		_midgard_core_object_create_xml_doc();
	xmlNode *root_node = 
		xmlDocGetRootElement(doc);

	for(i = 0; i < ret_rows; i++){
		row = mysql_fetch_row(results);
		
		object_node =
			xmlNewNode(NULL, BAD_CAST G_OBJECT_CLASS_NAME(_klass));
		xmlNewProp(object_node, BAD_CAST "purge", BAD_CAST "yes");
		xmlNewProp(object_node, BAD_CAST "guid", BAD_CAST row[0]);
		xmlNewProp(object_node, BAD_CAST "purged", BAD_CAST row[1]);
		xmlAddChild(root_node, object_node);
	}

	xmlChar *buf;
	gint size;
	xmlDocDumpFormatMemoryEnc(doc, &buf, &size, "UTF-8", 1);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	
	return (gchar*) buf;
}

/* Serialize midgard_blob binary data */
gchar *midgard_replicator_serialize_blob(MidgardReplicator *self,
					MgdObject *object)
{
	g_assert(object != NULL);

	MidgardConnection *_mgd;
	gchar *content;
	gsize bytes_read = 0;

	if(self == NULL)
		_mgd = object->mgd->_mgd;
	else
		_mgd = self->private->mgd;
	
	MidgardBlob *blob = 
		midgard_blob_new(object, NULL);

	if(!blob)
		return NULL;

	content = midgard_blob_read_content(blob, &bytes_read);

	gchar *encoded =
		g_base64_encode((const guchar *)content, bytes_read);
	g_free(content);

	xmlDoc *doc = _midgard_core_object_create_xml_doc();
	xmlNode *root_node =
		xmlDocGetRootElement(doc);
	xmlNode *blob_node = 
		xmlNewTextChild(root_node, NULL,
				(const xmlChar*)
				"midgard_blob",
				BAD_CAST encoded);
	xmlNewProp(blob_node, BAD_CAST "guid",
			BAD_CAST object->private->guid);

	g_free(encoded);

	xmlChar *buf;
	gint size;
	xmlDocDumpFormatMemoryEnc(doc, &buf, &size, "UTF-8", 1);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	
	return (gchar*) buf;
}

gchar *midgard_replicator_export_blob(MidgardReplicator *self,
		MgdObject *object)
{
	return midgard_replicator_serialize_blob(self, object);
}

/*  Creates object instance from the given xml string. */
GObject **midgard_replicator_unserialize(	MidgardReplicator *self, 
						MidgardConnection *mgd, 
						const gchar *xml,
						gboolean force)
{
	MidgardConnection *_mgd;
	if(self == NULL)
		_mgd = mgd;
	else 
		_mgd = self->private->mgd;

	return _midgard_core_object_from_xml(_mgd, xml, force); 
}

/*  Imports object  to database. */
gboolean midgard_replicator_import_object (	MidgardReplicator *self, 
						MidgardDBObject *import_object, gboolean force)
{
	if(self && import_object){
		g_warning("midgard_replicator already initialized with %s",
				self->private->typename);
		return FALSE;
	}

	MgdObject *object = MIDGARD_OBJECT(import_object);

	GType class_type = G_OBJECT_TYPE(object);
	if(!class_type || g_type_parent(class_type) != MIDGARD_TYPE_OBJECT ) {
		
		g_warning("Can not import object with invalid type");
		
		/* FIXME, there might be midgard object or not midgard one
		 * we should check if object is MgdDBObjectClass derived */
		/* MIDGARD_ERRNO_SET(mgd, MGD_ERR_INVALID_OBJECT); */
		return FALSE;
	}



	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);
	g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_import, 0);

	MidgardConnection *mgd;
	if(self) {
		object = MIDGARD_OBJECT(self->private->object);
		mgd = self->private->mgd;
	} else {
		mgd = object->mgd->_mgd;
	}
	
	/* Get object from database */
	MidgardQueryBuilder *builder = 
		midgard_query_builder_new(mgd->mgd, 
				G_OBJECT_TYPE_NAME(object));
	if(!builder)
		return FALSE;

	/* Set internal language */
	gint init_lang, init_dlang, object_lang, dbobject_lang;
	init_lang = mgd_lang(object->mgd);
	init_dlang = mgd_get_default_lang(object->mgd);
	gboolean multilang = FALSE;
	gboolean ret_val;

	MidgardObjectClass *klass =
		MIDGARD_OBJECT_GET_CLASS(object);
	if(midgard_object_class_is_multilang(klass)) {
		multilang = TRUE;
		g_object_get(G_OBJECT(object), "lang", &object_lang, NULL);
		mgd_internal_set_lang(object->mgd, object_lang);
		mgd_set_default_lang(object->mgd, object_lang);
	}

	GValue pval = {0, };
	/* Add guid constraint */
	g_value_init(&pval,G_TYPE_STRING);
	g_object_get_property(G_OBJECT(object), "guid", &pval);
	midgard_query_builder_add_constraint(builder,
			"guid",
			"=", &pval);
	g_value_unset(&pval);
	
	/* Get db object which is "elder" than imported one */
	/*
	g_value_init(&pval,G_TYPE_STRING);
	g_object_get_property(G_OBJECT(object->metadata), "revised", &pval);
	midgard_query_builder_add_constraint(builder, 
			"metadata.revised", "<", &pval);
	g_value_unset(&pval);
	*/

	/* Unset languages if we have multilang object */
	if(multilang) 
		midgard_query_builder_unset_languages(builder);

	/* Get deleted or undeleted object */
	midgard_query_builder_include_deleted(builder);

	GObject **_dbobjects = 
		midgard_query_builder_execute(builder, NULL);
	MgdObject *dbobject;

	g_object_unref (builder);

	if(_dbobjects && multilang){
		g_object_get(G_OBJECT(_dbobjects[0]), "lang", &dbobject_lang, NULL);	
		g_object_get(G_OBJECT(object), "lang", &object_lang, NULL);
		mgd_internal_set_lang(object->mgd, object_lang);
		mgd_set_default_lang(object->mgd, object_lang);
	}

	if(!_dbobjects){

		dbobject = midgard_object_class_get_object_by_guid(
				mgd, object->private->guid);
		
		/* Error is already set by get_by_guid.
		 * This is not thread safe */
		if(dbobject) {
			if(multilang) {
				mgd_internal_set_lang(object->mgd, init_lang);
				mgd_set_default_lang(object->mgd, init_dlang);
			}
			return FALSE;	
		}

		if((mgd->errnum == MGD_ERR_OBJECT_PURGED)
			&& force) {

			/* we need to delete repligard entry here 
			 * In any other case we need to change core's API a bit
			 * and make create method more complicated with 
			 * additional needless cases */

			GString *sql = g_string_new("DELETE from repligard WHERE ");
			g_string_append_printf(sql,
					"typename = '%s' AND guid = '%s' "
					"AND sitegroup = %d",
					G_OBJECT_TYPE_NAME(object),
					object->private->guid,
					object->private->sg);
			gchar *tmpstr = g_string_free(sql, FALSE);
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", tmpstr);
			mysql_query(object->mgd->msql->mysql, tmpstr);
			g_free(tmpstr);

			ret_val =  _midgard_object_create(object, 
					object->private->guid, 
					OBJECT_UPDATE_IMPORTED);		

			if(multilang) {
				mgd_internal_set_lang(object->mgd, init_lang);
				mgd_set_default_lang(object->mgd, init_dlang);
			}

			return ret_val;
		}

		if(mgd->errnum == MGD_ERR_NOT_EXISTS) {

			ret_val =  _midgard_object_create(object,
					object->private->guid, 
					OBJECT_UPDATE_IMPORTED);

			if(multilang) {
				mgd_internal_set_lang(object->mgd, init_lang);
				mgd_set_default_lang(object->mgd, init_dlang);
			}

			return ret_val;
		}

	} else {

		gchar *updated, *dbupdated;
		dbobject = (MgdObject *)_dbobjects[0];
		g_free (_dbobjects);

		/* Compare revised datetimes. We must know if imported 
		 * object is newer than that one which exists in database */
		g_object_get(G_OBJECT(object->metadata),
				"revised", &updated, NULL);
		g_object_get(G_OBJECT(dbobject->metadata),
				"revised", &dbupdated, NULL);
		/* We can use g_ascii_strcasecmp as it type cast every single 
		 * pointer to integer and unsigned char returning substract result */		
		gint datecmp;
	
		if (updated == NULL) {
		
			g_warning ("Trying to import ivalid object. metadata.revised property holds NULL (Object: %s)", 
					MGD_OBJECT_GUID (object));
			
			if (dbupdated)
				g_free (dbupdated);

			g_object_unref (dbobject);

			return FALSE;
		}

		if (dbupdated == NULL) {
		
			g_warning ("Trying to import object for invalid object stored in database. metadata.revised property holds NULL (Database object:%s)", MGD_OBJECT_GUID (object));
			
			if (updated)
				g_free (updated);

			g_object_unref (dbobject);

			return FALSE;
		}

		/* We can not check objects' lang now.
		 * Both uses the same revised metadata and only one object might be 
		 * imported when import from xml file is done.
		 * It affects only multilang objects, so those must be imported 
		 * always unconditionally */
		/* if(force || (multilang
		 *            && (dbobject_lang != object_lang))) { */
		if(force || multilang) {

			datecmp = -1;
		
		} else {
			
			datecmp = g_ascii_strcasecmp(
					(const gchar *)dbupdated, 
					(const gchar *)updated);
		}

		g_free(updated);
		g_free(dbupdated);

		gboolean deleted;
		gboolean ret;

		if(datecmp > 0 || datecmp == 0) {
			
			/* Database object is more recent or exactly the same */
			g_object_unref(dbobject);
			MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OBJECT_IMPORTED);
	
			if(multilang) {
				mgd_internal_set_lang(object->mgd, init_lang);
				mgd_set_default_lang(object->mgd, init_dlang);
			}

			return FALSE;

		} else if (datecmp < 0) {
			
			/* Database object is elder */	
			
			/* DELETE */
			g_object_get(G_OBJECT(object->metadata), 
					"deleted", &deleted, NULL);
			/* Imported object is marked as deleted , so 
			 * * we delete object from database */
			if(deleted){
				ret = midgard_object_delete(dbobject);
				g_object_unref(dbobject);
	
				if(multilang) {
					mgd_internal_set_lang(object->mgd, init_lang);
					mgd_set_default_lang(object->mgd, init_dlang);
				}

				return ret;
			}

			/* UPDATE */
		
			/* Check if dbobject is deleted */
			g_object_get(G_OBJECT(dbobject->metadata),
					"deleted", &deleted, NULL);

			guint undelete;
			g_object_get(G_OBJECT(object->metadata), 
					"deleted", &undelete, NULL);

			if((deleted && !undelete)) {
				ret = midgard_object_undelete(mgd, 
						dbobject->private->guid);
				goto _update_object;
			}

			if(deleted && !force) {
				
				MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OBJECT_DELETED);
				g_object_unref(dbobject);

				if(multilang) {
					mgd_internal_set_lang(object->mgd, init_lang);
					mgd_set_default_lang(object->mgd, init_dlang);
				}

				return FALSE;
			}

			_update_object:
			ret = _midgard_object_update(object, 
					OBJECT_UPDATE_IMPORTED);
			g_object_unref(dbobject);

			if(multilang) {
				mgd_internal_set_lang(object->mgd, init_lang);
				mgd_set_default_lang(object->mgd, init_dlang);
			}

			return ret;
		}		
	}
	return FALSE;
}


static gboolean __import_blob_from_xml(	MidgardConnection *mgd,
					xmlDoc *doc, 
					xmlNode *node)
{
	gchar *content;
	struct stat statbuf;
	const gchar *guid = 
		(const gchar *)xmlGetProp(node, BAD_CAST "guid");
	
	if(!guid) {
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		g_warning("Object's guid is empty. Can not import blob file.");
		g_free((gchar *)guid);
		xmlFreeDoc(doc);
		return FALSE;
	}

	if(!midgard_is_guid((const gchar *)guid)) {
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		g_warning("'%s' is not a valid guid", guid);
		g_free((gchar *)guid);
		xmlFreeDoc(doc);
		return FALSE;
	}
	
	MgdObject *object = 
		midgard_object_class_get_object_by_guid(mgd, guid);
	
	/* TODO , Add more error messages to inform about object state. 
	 * One is already set by midgard_object_class_get_object_by_guid */
	if(!object) {
		g_free((gchar *)guid);
		xmlFreeDoc(doc);
		return FALSE;
	}

	gchar *blobdir = object->mgd->blobdir;

	if (!blobdir || (*blobdir != '/')
			|| (stat(blobdir, &statbuf) != 0)
			|| !S_ISDIR(statbuf.st_mode)) {
		g_warning("Blobs directory is not set");
		g_free((gchar *)guid);
		xmlFreeDoc(doc);
		return FALSE;
	}

	gchar *location;
	gchar *blobpath = NULL;
	g_object_get(G_OBJECT(object), "location", &location, NULL);
	if(strlen(location) > 1) {
		blobpath = g_strconcat(blobdir, "/", location, NULL);
	}

	/* TODO, Find the way to get content and not its copy */
	/* node->content doesn't seem to hold it */
	content = (gchar *)xmlNodeGetContent(node);
	
	gsize content_length =
		(gsize) strlen(content);
	guchar *decoded =
		g_base64_decode(content, &content_length);
	g_free(content);

	FILE *fp = fopen(blobpath, "w+");
	fwrite(decoded, sizeof(char),
			content_length, fp);	

	fclose(fp);	
	g_free(decoded);
	g_free((gchar *)guid);
	if(blobpath)
		g_free(blobpath);
	xmlFreeDoc(doc);

	return TRUE;
}

static void
__delete_multilang_content_hack (MidgardConnection *mgd, xmlNode *root_node)
{
	if (!mgd || !root_node)
		return;

	xmlChar *guid_attr;
	MgdObject *object = NULL;
	xmlNode *child = _get_type_node(root_node->children);
	gboolean revert_lang;
	gint init_lang;
	gint init_dlang;

	/* Try to find guid of an object */
	/* WARNING! We assume there's instances of the same class identified by the same guid in xml! */
	for (; child; child = _get_type_node(child->next)) {

		guid_attr = xmlGetProp(child, BAD_CAST "guid");

		revert_lang = FALSE;

		if (guid_attr) {

			/* Take into account the case when object has no lang0 entries, but has many languages */
			xmlChar *lang_attr = xmlGetProp (child, BAD_CAST "lang");
			if (lang_attr && mgd_lang (mgd->mgd) == 0) {
		
				/* Get language ids, so we know how to revert */
				init_lang = mgd_lang (mgd->mgd);
				init_dlang = mgd_get_default_lang (mgd->mgd);			

				GValue pval = {0, };
				g_value_init(&pval, G_TYPE_STRING);
				g_value_set_string(&pval, (gchar *)lang_attr);
				MidgardQueryBuilder *builder = 
					midgard_query_builder_new(mgd->mgd, "midgard_language");
				midgard_query_builder_add_constraint(builder, "code", "=", &pval);
				g_value_unset(&pval);

				GObject **objects = midgard_query_builder_execute(builder, NULL);
				g_object_unref(builder);

				guint langid;
				if(objects) {

					g_object_get(objects[0], "id", &langid, NULL);
					mgd_internal_set_lang(mgd->mgd, langid);
					mgd_set_default_lang(mgd->mgd, langid);
					g_object_unref(objects[0]);
				}

				g_free(objects);

				revert_lang = TRUE;
				xmlFree (lang_attr);
			}
	
			object = midgard_object_class_get_object_by_guid(mgd, (const gchar *)guid_attr);

			if (revert_lang) {

				/* Revert to previous - initial state */
				mgd_internal_set_lang (mgd->mgd, init_lang);
				mgd_set_default_lang (mgd->mgd, init_dlang);	
			}

			xmlFree (guid_attr);

			if (object)
				break;
		}
	}

	if (!object)
		return;

	GType object_type = G_OBJECT_TYPE (object);
	if (!g_type_is_a (object_type, MIDGARD_TYPE_OBJECT))
		return;

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS (object);
	if (!midgard_object_class_is_multilang (klass))
		return;

	child = _get_type_node(root_node->children);
	GObject **langs = midgard_object_get_languages (MIDGARD_OBJECT(object), NULL);

	if (!langs)
		return;

	xmlChar *attr = NULL;
	guint i;
	gchar *lang_code;
	GHashTable *lang_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	/* Get first node again */
	child = _get_type_node(root_node->children);

	/* Fill hash table so we know what language contents given object has */
	for (; child; child = _get_type_node(child->next)) {

		i = 0;
		attr = xmlGetProp(child, BAD_CAST "lang");
		if((!attr) || (attr && (*attr == '\0')))
				continue;

		g_hash_table_insert (lang_table, g_strdup ((gchar *) attr), "");

		xmlFree (attr);
	}

	/* Count number of languages, so we know if to delete witj own query or to 
	  use API delete() fallback */
	guint j = 0;
	while (langs[j] != NULL)
		j++;

	for ( i = 0; langs[i] != NULL; i++) {

		g_object_get (langs[i], "code", &lang_code, NULL);	

		/* We assume this is lang0, so special case that we can not handle this one atm */
		if (!lang_code || (lang_code && *lang_code == '\0')) 
			continue;	
	
		/* Given language is found in hash table, so continue as language translation is still valid */		
		if (!lang_table || (lang_code && g_hash_table_lookup (lang_table, lang_code))) {
			g_free (lang_code);
			continue;
		}
		
		g_free (lang_code);

		guint temp_lang = 0;
		g_object_get (langs[i], "id", &temp_lang, NULL);

		/* Get language ids, so we know how to revert */
		init_lang = mgd_lang (MIDGARD_OBJECT (object)->mgd);
		init_dlang = mgd_get_default_lang (MIDGARD_OBJECT (object)->mgd);			

		/* Set language explicitly so only lang translation record is deleted */
		mgd_internal_set_lang (MIDGARD_OBJECT (object)->mgd, temp_lang);
		mgd_set_default_lang (MIDGARD_OBJECT (object)->mgd, temp_lang);	

		if (j > 0) {
			guint oid;
			MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS (object);
			g_object_get (G_OBJECT (object), "id", &oid, NULL);
			GString *del = g_string_new("DELETE FROM ");
			g_string_append_printf(del,
				"%s_i WHERE lang = %d AND sid = %d",
				midgard_object_class_get_table(klass),
				temp_lang,  oid);
                        g_debug ("query=%s", del->str);
			mysql_query (object->mgd->msql->mysql, del->str);
			g_string_free (del, TRUE);
		} else {
			midgard_object_delete (MIDGARD_OBJECT (object));
		}

		/* Revert to previous - initial state */
		mgd_internal_set_lang (MIDGARD_OBJECT (object)->mgd, init_lang);
		mgd_set_default_lang (MIDGARD_OBJECT (object)->mgd, init_dlang);	
	}
}

void midgard_replicator_import_from_xml(	MidgardReplicator *self,
						MidgardConnection *mgd,
						const gchar *xml, 
						gboolean force)
{	
	MidgardConnection *_mgd;
	
	if(self	== NULL)
		_mgd = mgd;
	else 
		_mgd = self->private->mgd;
	
	xmlDoc *doc = NULL;
	xmlNode *root_node = NULL;
	gint init_lang = mgd_lang(_mgd->mgd);
	gint init_dlang = mgd_get_default_lang(_mgd->mgd);
	_midgard_core_object_get_xml_doc(_mgd, xml, &doc, &root_node);
	
	if(doc == NULL || root_node == NULL)
		return;

	xmlNodePtr child = _get_type_node(root_node->children);
	if(!child) {
		g_warning("Can not get midgard type name from the given xml");
		xmlFreeDoc(doc);
		return;
	}

	GType object_type = g_type_from_name((const gchar *)child->name);
	
	if(object_type == MIDGARD_TYPE_BLOB) {
		/* it will destroy xmlDoc */
		__import_blob_from_xml(_mgd, doc, child);
		return;
	}

	xmlChar *attr, *guid_attr;
	MgdObject *dbobject;
	guint langid = 0;
	MgdObject *object = NULL;

	for (; child; child = _get_type_node(child->next)) {
		
		attr = xmlGetProp(child, BAD_CAST "purge");
		guid_attr = xmlGetProp(child, BAD_CAST "guid");

		if(g_str_equal(attr, "yes")) {
			
			dbobject = midgard_object_class_get_object_by_guid(
					_mgd, (const gchar *)guid_attr);

			if(dbobject || 
					( !dbobject && 
					 (mgd->mgd->errn == MGD_ERR_OBJECT_DELETED)
					 )) {
				midgard_object_purge(dbobject);
				if(dbobject)
					g_object_unref(dbobject);
				xmlFree(attr);
				xmlFree(guid_attr);
				continue;
			}
		}

		xmlFree(attr);

		object = midgard_object_new(_mgd->mgd, (const gchar *)child->name, NULL);
		if(!object) {
			g_warning("Can not create %s instance", child->name);
			xmlFreeDoc(doc);
			xmlFree(attr);
			xmlFree(guid_attr);
			continue;
		}

		if (guid_attr) {
			object->private->guid = (const gchar *)g_strdup((gchar *)guid_attr);
		}

		if(!_nodes2object(G_OBJECT(object), child->children, force)) {
			xmlFree(guid_attr);
			g_object_unref(object);
			continue;
		}		
		
		MidgardObjectClass *klass = 
			MIDGARD_OBJECT_GET_CLASS(object);
		if(midgard_object_class_is_multilang(klass)){
			
			gboolean langnz = TRUE;
			attr =  xmlGetProp(child, BAD_CAST "lang");

			if((!attr) || (attr && g_str_equal(attr, "")))
				langnz = FALSE;
			
			if(langnz) {
				
				GValue pval = {0, };
				g_value_init(&pval, G_TYPE_STRING);
				g_value_set_string(&pval, (gchar *)attr);
				MidgardQueryBuilder *builder = 
					midgard_query_builder_new(_mgd->mgd,
							"midgard_language");
				midgard_query_builder_add_constraint(builder,
						"code", "=", &pval);
				g_value_unset(&pval);
				GObject **objects = 
					midgard_query_builder_execute(builder, NULL);
				g_object_unref(builder);
				if(objects) {
					g_object_get(objects[0], "id", &langid, NULL);
					mgd_internal_set_lang(_mgd->mgd, langid);
					mgd_set_default_lang(_mgd->mgd, langid);
					g_object_set(object, "lang", langid, NULL);
					g_object_unref(objects[0]);
				}
				g_free(objects);
			
			} else {

				/* Set default 0 language */
				mgd_internal_set_lang(_mgd->mgd, 0);
				mgd_set_default_lang(_mgd->mgd, 0);
			}
		}

		/* TODO */
		/* Add lang property ! */

		if(!midgard_replicator_import_object(NULL, MIDGARD_DBOBJECT(object), force)){
			xmlFree(guid_attr);
			mgd_internal_set_lang(object->mgd, init_lang);
			mgd_set_default_lang(object->mgd, init_dlang);
			g_object_unref(object);
			continue;
		} else {
			xmlFree(guid_attr);
			mgd_internal_set_lang(object->mgd, init_lang);  
			mgd_set_default_lang(object->mgd, init_dlang);
			g_object_unref(object);
		}
	}

	__delete_multilang_content_hack (_mgd, root_node);

	xmlFreeDoc(doc);
}

/* GOBJECT ROUTINES */

static void _midgard_replicator_instance_init(
		GTypeInstance *instance, gpointer g_class)
{
	MidgardReplicator *self = (MidgardReplicator *) instance;
	self->private = g_new(MidgardReplicatorPrivate, 1);

	self->private->object = NULL;
	self->private->klass = NULL;
	self->private->type = 0;
	self->private->typename = NULL;
}

static void _midgard_replicator_finalize(GObject *object)
{
	g_assert(object != NULL);
	
	MidgardReplicator *self = (MidgardReplicator *) object;

	g_free(self->private);
}

static void _midgard_replicator_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	/* MidgardReplicatorClass *klass = MIDGARD_REPLICATOR_CLASS (g_class); */
	
	gobject_class->finalize = _midgard_replicator_finalize;
}

/* Returns MidgardReplicator type. */
GType midgard_replicator_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardReplicatorClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_replicator_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardReplicator),
			0,              /* n_preallocs */
			(GInstanceInitFunc) _midgard_replicator_instance_init /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
				"midgard_replicator",
				&info, 0);
	}
	return type;
}
