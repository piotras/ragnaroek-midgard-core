/* 
 * Copyright (C) 2007 Piotr Pokora <piotrek.pokora@gmail.com>
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

#include "midgard/midgard_connection.h"
#include "midgard/midgard_sitegroup.h"
#include "midgard/midgard_metadata.h"
#include "midgard/midgard_error.h"
#include "midgard_core_query.h"
#include "midgard_core_object.h"
#include "midgard_core_object_class.h"
#include "midgard/midgard_user.h"
#include "midgard/guid.h"
#include "midgard/query_builder.h"
#include "midgard/midgard_dbobject.h"
#include "midgard/midgard_datatypes.h"

/* Sitegroup object properties */
enum {
	MIDGARD_SITEGROUP_GUID = 1,
	MIDGARD_SITEGROUP_NAME,
	MIDGARD_SITEGROUP_REALM,
	MIDGARD_SITEGROUP_ADMINGROUP,
	MIDGARD_SITEGROUP_PUBLIC,
	MIDGARD_SITEGROUP_ID,
	MIDGARD_SITEGROUP_METADATA,
	MIDGARD_SITEGROUP_ADMINID
};

#define SITEGROUP_TABLE "sitegroup"

/* Creates new Midgard Sitegroup instance for the given Midgard Connection. */
MidgardSitegroup *midgard_sitegroup_new(MidgardConnection *mgd, const GValue *value) 
{
	g_return_val_if_fail(mgd != NULL, NULL);

	midgard_res *res = NULL;

	if (G_VALUE_HOLDS_STRING(value) || G_VALUE_HOLDS_INT(value) || value == NULL) {

		/* This is OK */

	} else {
			
		g_warning("Expected GValue of type string or int");
		return NULL;
	}

	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	MidgardSitegroup *self;

	if (value == NULL) {

		self = (MidgardSitegroup *)g_object_new(MIDGARD_TYPE_SITEGROUP, NULL);	
		self->dbpriv->mgd = mgd;
		self->dbpriv->guid = midgard_guid_new(mgd->mgd);
		
		return self;
	}
	
	if (G_VALUE_HOLDS_STRING(value)) {
		
		res = mgd_ungrouped_select(mgd->mgd, "id, admingroup, guid, realm, name", "sitegroup",
				"name=$q", NULL, g_value_get_string(value));
	
	} else if (G_VALUE_HOLDS_INT(value)) {
		
		res = mgd_ungrouped_select(mgd->mgd, "id, admingroup, guid, realm, name", "sitegroup",
				"id=$d", NULL, g_value_get_int(value));
	}

	
	if (!res) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_NOT_EXISTS);
		return NULL;
	}
	
	guint j = mgd_rows(res);

	if(j > 1) {

		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		g_warning("Database inconsistency. Found %d sitegroups with the same identifier", j);
		mgd_release(res);

		return NULL;
	}

	if(j == 0) {
		
		mgd_release(res);
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_NOT_EXISTS);
		return NULL;
	}

	mgd_fetch(res);

	self = (MidgardSitegroup *)g_object_new(MIDGARD_TYPE_SITEGROUP, NULL);
	self->dbpriv->mgd = mgd;

	self->id = mgd_sql2id(res, 0);
	self->adminid = mgd_sql2id(res, 1);
	self->dbpriv->guid = g_strdup((char *)mgd_colvalue(res, 2));
	self->realm = g_strdup((char *)mgd_colvalue(res, 3));
	self->name = g_strdup((char *)mgd_colvalue(res, 4));

	mgd_release(res);

	return self;
}

/* Fetch all sitegroups from database. */
gchar **midgard_sitegroup_list(MidgardConnection *mgd)
{
	g_return_val_if_fail(mgd != NULL, NULL);

	const gchar *query = 
		"SELECT name from sitegroup";

	midgard_res *res = mgd_query(mgd->mgd, query);	
	guint i = 0, j;

	 if (!res)
		 return NULL;	

	j = mgd_rows(res);

	if(j == 0) {

		mgd_release(res);
		return NULL;
	}

	gchar **names = g_new(gchar*, j+1);

	while(mgd_fetch(res)){ 
	
		names[i] = g_strdup((char *)mgd_colvalue(res, 0));
		i++;
	}

	names[i++] = NULL;
	mgd_release(res);

	return names;
}

/* Creates new record for the given MidgardSitegroup object. */
gboolean midgard_sitegroup_create(MidgardSitegroup *self)
{
	g_return_val_if_fail(self != NULL, FALSE);

	MidgardConnection *mgd = self->dbpriv->mgd;	
	
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);
	
	if(!mgd_isroot(mgd->mgd)) {
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_ACCESS_DENIED);
		return FALSE;
	}

	if(self->name == NULL) {
		midgard_set_error(mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_NAME,
				" Can not create sitegroup with empty name");
		return FALSE;
	}

        midgard_res *res = mgd_ungrouped_select(mgd->mgd, "id", "sitegroup",
			"name=$q", NULL, self->name);

	if(res && mgd_fetch(res)) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OBJECT_NAME_EXISTS);
		mgd_release(res);

		return FALSE;
	}

	if(res)
		mgd_release(res);

	gint id = mgd_create(mgd->mgd, 
			"sitegroup", 
			"name, realm, admingroup, guid", 
			"$q, $q, $d, $q", 
			self->name, self->realm, self->adminid, self->dbpriv->guid);

	if(id == 0){
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	}

	self->id = id;	

	/* TODO, do we need repligard entry for sitegroup? I guess not */		

	mgd_legacy_load_sitegroup_cache(mgd->mgd);

	return TRUE;
}

/* Updates record(s) for the given Midgard Sitegroup object. */ 
gboolean midgard_sitegroup_update(MidgardSitegroup *self)
{
	g_return_val_if_fail(self != NULL, FALSE);

	MidgardConnection *mgd = self->dbpriv->mgd;	
	
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);
	
	if(!mgd_isroot(mgd->mgd)) {
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_ACCESS_DENIED);
		return FALSE;
	}

	if(self->name == NULL) {
		midgard_set_error(mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_NAME,
				" Can not update sitegroup with empty name");
		return FALSE;
	}

	if(self->id == 0) {
		midgard_set_error(mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_NAME,
				" Can not update sitegroup with id 0");
		return FALSE;
	}

	gint id = mgd_update(mgd->mgd, 
			"sitegroup", self->id,
			"name=$q, realm=$q, admingroup=$d",
			self->name, self->realm, self->adminid);

	if(id == 0){
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	}

	/* TODO, do we need repligard entry for sitegroup? I guess not */		

	return TRUE;
}

/**
* Delete record(s) for the given Midgard Sitegroup object.
*/ 
gboolean midgard_sitegroup_delete(MidgardSitegroup *self)
{
	g_warning("Not implemented");
	return FALSE;
}


/* GOBJECT ROUTINES */

static void __set_sitegroup_from_xml_node(MidgardDBObject *object, xmlNode *node)
{
	g_assert(object != NULL);
	MidgardSitegroup *self = MIDGARD_SITEGROUP(object);
	xmlNode *lnode = NULL;

	lnode = __dbobject_xml_lookup_node(node, "name");
	self->name = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "realm");
	self->realm = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "id");
	self->id = __get_node_content_uint(lnode);

	lnode = __dbobject_xml_lookup_node(node, "adminid");
	self->adminid = __get_node_content_uint(lnode);

	lnode = __dbobject_xml_lookup_node(node, "group");
	self->group = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "public");
	self->public = __get_node_content_bool(lnode);
}

static void
_midgard_sitegroup_set_property (GObject *object, guint property_id,
		const GValue *value, GParamSpec *pspec)
{
	MidgardSitegroup *self = (MidgardSitegroup *) object;

	switch (property_id) {
		
		case MIDGARD_SITEGROUP_GUID:
			g_warning("Setting guid property forbidden");
			break;
			
		case MIDGARD_SITEGROUP_NAME:
			g_free((gchar *)self->name);
			self->name = g_value_dup_string(value);
			break;

		case MIDGARD_SITEGROUP_REALM:
			g_free((gchar *)self->realm);
			self->realm = g_value_dup_string(value);
			break;
			
		case MIDGARD_SITEGROUP_ADMINGROUP:
			g_free((gchar *)self->group);
			self->group = g_value_dup_string(value);
			break;
			
		case MIDGARD_SITEGROUP_PUBLIC:
			self->public = g_value_get_boolean(value);
			break;
			
		case MIDGARD_SITEGROUP_ID:
			g_warning("Setting ID property forbidden");
			break;

		case MIDGARD_SITEGROUP_METADATA:
			self->metadata = g_value_get_object(value);
			break;
			
		case MIDGARD_SITEGROUP_ADMINID:
			self->adminid = g_value_get_uint(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
			break;
	}
}

static void
_midgard_sitegroup_get_property (GObject *object, guint property_id,
		GValue *value, GParamSpec *pspec)
{
	MidgardSitegroup *self = (MidgardSitegroup *) object;
	
	switch (property_id) {

		case MIDGARD_SITEGROUP_GUID:
			g_value_set_string(value, self->dbpriv->guid);
			break;
			
		case MIDGARD_SITEGROUP_NAME:
			g_value_set_string(value, self->name);
			break;

		case MIDGARD_SITEGROUP_REALM:
			g_value_set_string(value, self->realm);
			break;

		case MIDGARD_SITEGROUP_ADMINGROUP:
			g_value_set_string(value, self->group);
			break;

		case MIDGARD_SITEGROUP_PUBLIC:
			g_value_set_boolean(value, self->public);
			break;

		case MIDGARD_SITEGROUP_ID:
			g_value_set_uint(value, self->id);
			break;

		case MIDGARD_SITEGROUP_METADATA:
			g_value_set_object(value, self->metadata);
			break;

		case MIDGARD_SITEGROUP_ADMINID:
			g_value_set_uint(value, self->adminid);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
			break;
	}
}

static void _midgard_sitegroup_finalize(GObject *object)
{
	g_assert(object != NULL);
	
	MidgardSitegroup *self = (MidgardSitegroup *) object;

	g_free((gchar *) self->dbpriv->guid);
	g_free((gchar *) self->name);
	g_free((gchar *) self->realm);
	g_free((gchar *) self->group);
	
	/* g_object_unref(self->metadata); */

	g_free(self->dbpriv);
	
	if(self->priv != NULL)
		g_free(self->priv);

	self->priv = NULL;
}

static void _midgard_sitegroup_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardSitegroupClass *klass = MIDGARD_SITEGROUP_CLASS (g_class);
	
	gobject_class->finalize = _midgard_sitegroup_finalize;
	gobject_class->set_property = _midgard_sitegroup_set_property;
	gobject_class->get_property = _midgard_sitegroup_get_property;

	MgdSchemaPropertyAttr *prop_attr;
	MgdSchemaTypeAttr *type_attr = _mgd_schema_type_attr_new();
	
	GParamSpec *pspec;
	
	/* GUID */
	pspec = g_param_spec_string ("guid",
			"Object's guid",
			"",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_SITEGROUP_GUID,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("guid");
	prop_attr->table = g_strdup(SITEGROUP_TABLE);
	prop_attr->tablefield = g_strjoin(".", SITEGROUP_TABLE, "guid", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"guid"), prop_attr);

	/* NAME */
	pspec = g_param_spec_string ("name",
			"Sitegroup's name",
			"Unique name",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_SITEGROUP_NAME,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_STRING;
	prop_attr->field = g_strdup("name");
	prop_attr->table = g_strdup(SITEGROUP_TABLE);
	prop_attr->tablefield = g_strjoin(".", 
			SITEGROUP_TABLE, "name", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"name"), prop_attr);
	
	/* REALM */
	pspec = g_param_spec_string ("realm",
			"Sitegroup's realm",
			"",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_SITEGROUP_REALM,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_STRING;
	prop_attr->field = g_strdup("realm");
	prop_attr->table = g_strdup(SITEGROUP_TABLE);
	prop_attr->tablefield = g_strjoin(".", 
			SITEGROUP_TABLE, "realm", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"realm"), prop_attr);

	/* GROUP */	
	pspec = g_param_spec_string ("group",
			"Sitegroup's admin group",
			"",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_SITEGROUP_ADMINGROUP,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("admingroup_guid");
	prop_attr->table = g_strdup(SITEGROUP_TABLE);
	prop_attr->tablefield = g_strjoin(".", 
			SITEGROUP_TABLE, "admingroup_guid", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"group"), prop_attr);

	/* PUBLIC */
	pspec = g_param_spec_boolean("public",
			"Sitegroup's public info",
			"",
			FALSE, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_SITEGROUP_PUBLIC,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_BOOLEAN;
	prop_attr->field = g_strdup("public");
	prop_attr->table = g_strdup(SITEGROUP_TABLE);
	prop_attr->tablefield = g_strjoin(".", 
			SITEGROUP_TABLE, "public", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"public"), prop_attr);

	/* ID */
	pspec = g_param_spec_uint ("id",
			"Sitegroup's ID",
			"",
			0, G_MAXUINT32, 0, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_SITEGROUP_ID,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_UINT;
	prop_attr->field = g_strdup("id");
	prop_attr->table = g_strdup(SITEGROUP_TABLE);
	prop_attr->tablefield = g_strjoin(".", 
			SITEGROUP_TABLE, "id", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"id"), prop_attr);

	/* METADATA */
	/*pspec = g_param_spec_object ("metadata", 
			"",
			"Property with Midgard metadata object",
			G_TYPE_OBJECT, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_SITEGROUP_METADATA,
			pspec);*/
	
	/* LEGACY ADMIN ID */
	pspec = g_param_spec_uint ("adminid",
			"Legacy ",
			"Legacy admingroup id",
			0, G_MAXUINT32, 0, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_SITEGROUP_ADMINID,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_UINT;
	prop_attr->field = g_strdup("admingroup");
	prop_attr->table = g_strdup(SITEGROUP_TABLE);
	prop_attr->tablefield = g_strjoin(".", 
			SITEGROUP_TABLE, "admingroup", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"adminid"), prop_attr);

	/* Set storage data */
	type_attr->table = g_strdup(SITEGROUP_TABLE);
	/* type_attr->tables = g_strdup(SITEGROUP_TABLE); */
	
	/* This must be replaced with GDA classes */
	MgdSchemaTypeQuery *_query = _mgd_schema_type_query_new();
	_query->select_full = g_strdup("name, realm, public, admingroup_guid AS guid, admingroup AS adminid");
	type_attr->query = _query;

	/* Initialize private member */
	klass->dbpriv = g_new(MidgardDBObjectPrivate, 1);
	klass->dbpriv->storage_data = type_attr;

	klass->dbpriv->set_from_xml_node = __set_sitegroup_from_xml_node;
}

static void _midgard_sitegroup_instance_init(
		GTypeInstance *instance, gpointer g_class)
{
	MidgardSitegroup *self = (MidgardSitegroup *) instance;

	self->name = NULL;
	self->realm = NULL;
	self->group = NULL;
	self->public = FALSE;
	self->id = 0;
	self->adminid = 0;
	/* self->metadata = g_object_new(MIDGARD_TYPE_METADATA, NULL); */

	self->dbpriv = g_new(MidgardDBObjectPrivate, 1);
	self->dbpriv->mgd = NULL;
	self->dbpriv->guid = NULL;
}

GType midgard_sitegroup_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardSitegroupClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_sitegroup_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardSitegroup),
			0,              /* n_preallocs */
			(GInstanceInitFunc) _midgard_sitegroup_instance_init/* instance_init */
		};
		type = g_type_register_static (MIDGARD_TYPE_DBOBJECT,
				"midgard_sitegroup",
				&info, 0);
	}
	return type;
}



