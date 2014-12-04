/* 
Copyright (C) 2004,2005,2006, 2007,2008 Piotr Pokora <piotrek.pokora@gmail.com>
Copyright (C) 2004 Alexander Bokovoy <ab@samba.org>

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include <stdlib.h>
#include "midgard/midgard_type.h"
#include "midgard/midgard_object.h"
#include "midgard/midgard_metadata.h"
#include "midgard/query.h"
#include "midgard/midgard_legacy.h"
#include "midgard/query_builder.h"
#include "midgard/midgard_timestamp.h"
#include "midgard/midgard_datatypes.h"
#include "midgard/midgard_quota.h"
#include "schema.h"
#include "midgard_mysql.h"
#include "midgard/midgard_reflection_property.h"
#include "midgard/midgard_error.h"
#include "midgard_core_object.h"
#include "midgard_core_object_class.h"
#include <sys/stat.h>
#include "midgard/midgard_blob.h"
#include "midgard/midgard_dbus.h"
#include "midgard_core_query.h"
#include "midgard_core_query_builder.h"

GType _midgard_attachment_type = 0;
static gboolean signals_registered = FALSE;

enum {
	MIDGARD_PROPERTY_NULL = 0,
	MIDGARD_PROPERTY_GUID,
	MIDGARD_PROPERTY_SITEGROUP,
	MIDGARD_PROPERTY_METADATA
};

enum {
	OBJECT_IN_TREE_NONE = 0,
	OBJECT_IN_TREE_DUPLICATE,
	OBJECT_IN_TREE_OTHER_LANG
};

enum {
	_SQL_QUERY_CREATE = 0,
	_SQL_QUERY_UPDATE
};

static void  __dbus_send(MgdObject *obj, const gchar *action)
{
	if (!obj)
		return;

	MidgardConnection *mgd = MGD_OBJECT_CNC (obj);
	if (!mgd)
		return;

	if (MGD_CNC_DBUS (mgd)) {
		gchar *_dbus_path = g_strconcat("/mgdschema/",
				G_OBJECT_TYPE_NAME(G_OBJECT(obj)),
				"/", action, NULL);
		_midgard_core_dbus_send_serialized_object(obj, _dbus_path);
		g_free(_dbus_path);
	}
}

static void
__add_core_properties(MgdSchemaTypeAttr *type_attr)
{
	/* GUID */
	MgdSchemaPropertyAttr *prop_attr =
		_mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 0;
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("guid");
	prop_attr->table = g_strdup(type_attr->table);
	prop_attr->tablefield =
		g_strjoin(".", type_attr->table, "guid", NULL);
	g_hash_table_insert(type_attr->prophash,
		g_strdup((gchar *)"guid"), prop_attr);
	
	/* SITEGROUP */
	MgdSchemaPropertyAttr *sg_attr =
		_mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 1;
	sg_attr->gtype = MGD_TYPE_UINT;
	sg_attr->field = g_strdup("sitegroup");
	sg_attr->table = g_strdup(type_attr->table);
	sg_attr->tablefield =
		g_strjoin(".", type_attr->table, "sitegroup", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"sitegroup"), sg_attr);
	
	return;
}

static GParamSpec **_midgard_object_class_paramspec()
{
	/* Hardcode MidgardRepligardClass parameters. Those will be inherited with MgdObject 
	 * and thus do not bother about Midgard basic and core parameters later.
	 * We also should hardcode these parameters to keep one real Midgard object
	 * which has "one logic" properties and methods.
	 */
  
	/* Last value is 'params[n]+1' */
	GParamSpec **params = g_malloc(sizeof(GParamSpec*)*5);
	params[0] = g_param_spec_string ("guid", "", "GUID",
			" ", G_PARAM_READWRITE);
	params[1] = g_param_spec_uint ("sitegroup", "", 
			"Sitegroup which object belongs to.",
			0, G_MAXUINT32, 0, G_PARAM_READWRITE); 
	params[2] = g_param_spec_object ("metadata", "",
			"Property with Midgard metadata object",
			G_TYPE_OBJECT, G_PARAM_READWRITE);
	params[3] = g_param_spec_string ("action", "", "What was done with object",
			" ", G_PARAM_READWRITE);    
	params[4] = NULL;
	  
	return params;     
}

/* Initialize instance for all types that are not MidgardRepligardClass type. */
static void 
_object_instance_init(GTypeInstance *instance, gpointer g_class)
{
    GType own_type = G_TYPE_FROM_INSTANCE(instance);
    MgdSchemaTypeAttr *priv = G_TYPE_INSTANCE_GET_PRIVATE (instance, own_type, MgdSchemaTypeAttr);

    MgdObject *self = (MgdObject *) instance;

    self->private = g_new0(MidgardTypePrivate, 1);
    self->priv = self->private;
    self->private->guid = NULL;
    self->private->sg = -1;
    self->private->action = NULL;
    self->private->exported = NULL;
    self->private->imported = NULL;
    self->private->read_only = FALSE;
    self->private->_mysql_row = NULL;
    
    /* Initialize metadata object */
    self->metadata = (MidgardMetadata *) g_object_new(MIDGARD_TYPE_METADATA, NULL);
    /* LEAK! */
    /* Piotras: I do not understand this, there is object_unref call for metadata
     * object in MgdObject finalize. I am completely blind here */ 

    /* Set metadata's object, we can not do this with midgard_metadata_new */
    self->metadata->private->object = self;
    
    self->private->parameters = NULL;	
    self->private->_params = NULL;
	
    priv->base_index = 1;
    
    guint propn;
    GParamSpec **props =
        g_object_class_list_properties(g_type_class_peek(own_type), &propn);
    g_free(props);
   
    self->mgd = NULL;

    priv->num_properties = propn;

    /* g_debug("New object instance for class '%s'", g_type_name(own_type)); */
  
    /* TODO 
     * Add functionality to get properties from self class instead of looking 
     * for cached type. We can not store GTypeInfo structure in MgdSchema as this require making 
     * schema real global value.
     */ 
 
    /* Allocate properties storage for this instance  */
    priv->properties = 
        priv->num_properties ? g_new0 (MgdSchemaPropertyAttr*, priv->num_properties) : NULL;
    if (priv->properties) {
        guint idx;
        for (idx = 0; idx < priv->num_properties; idx++) {
            priv->properties[idx] = g_new0 (MgdSchemaPropertyAttr, 1);
        }
    }
}
                
/* AB: This is shortcut macro for traversing through class hierarchy (from child to ancestor)
 * until we reach GObject. On each iteration we fetch private data storage for the object
 * according to the current class type and run supplied code.
 * This allows us to implement dynamic subclassing cast which is done usually via compiled 
 * time structure casts in appropriate class-specific hooks for each class in a hierarchy.
 * As our object classes have the same structure at each level of inheritance, we are able
 * to use the same triple of functions for get_property(), set_property(), and finalize()
 * for each class.
 */
#define G_MIDGARD_LOOP_HIERARCHY_START              \
	do {                      \
		priv = G_TYPE_INSTANCE_GET_PRIVATE (object, current_type, MgdSchemaTypeAttr);  \

#define G_MIDGARD_LOOP_HIERARCHY_STOP             \
		current_type = g_type_parent(current_type);           \
	} while (current_type != MIDGARD_TYPE_DBOBJECT );

/* AB: Handle property assignments. Despite its simplicity, this is *very* important function */
static void
_object_set_property (GObject *object, guint prop_id, 
    const GValue *value, GParamSpec   *pspec)
{	
	gint prop_id_local = 0;
	MgdSchemaTypeAttr *priv;
	GType current_type = G_TYPE_FROM_INSTANCE(object);
	MgdObject *self = (MgdObject *) object;
	MgdSchemaPropertyAttr *prop;
	GObject *metadata_object = NULL;

	if (self->private->read_only) {
		g_warning ("Can not write property. Object in read only mode");
		return;
	}

	switch(prop_id) {
			
		case MIDGARD_PROPERTY_GUID:
			g_debug("Setting guid property is not allowed");
			break;
		
		case MIDGARD_PROPERTY_SITEGROUP:
			/* This can not be enabled due to PHP4 usage
			g_warning("sitegroup property can not be set"); */
			//g_warning("Setting sitegroup property");

			/* Allow sitegroup property set , only when this is empty 
			 * ( not fetched from database )instance. */ 
			if(self->private->guid == NULL) {

				/* Somehow this is fatal */
				if(self->mgd == NULL)
					return;
				/* Then , allow only for SG0 admin and anonymous user */
				if(mgd_isroot(self->mgd) 
					|| (self->mgd->current_user->id == 0)) {
					self->private->sg = g_value_get_uint(value);
				}
			}
			break;
		
		case MIDGARD_PROPERTY_METADATA:	
			metadata_object = g_value_get_object(value);
			if(metadata_object == NULL) {
				g_warning("Can not set NULL metadata object explicitly");
				return;
			}
			if(!MIDGARD_IS_METADATA(metadata_object)) {
				g_warning("Expected metadata object instead of '%s'", 
						G_OBJECT_TYPE_NAME(metadata_object));
				return;
			}
			self->metadata = (MidgardMetadata*)metadata_object;
			break;
			
		default:
			G_MIDGARD_LOOP_HIERARCHY_START
				prop_id_local = prop_id - priv->base_index - 1;
			if ((prop_id_local >= 0) && (prop_id_local < priv->num_properties)) {
				if (priv->num_properties) {
					prop = priv->properties[prop_id_local];
					if (!G_IS_VALUE(&prop->value)) {
						g_value_init(&prop->value, G_VALUE_TYPE(value));
						/* g_debug(" Set property %s", pspec->name );  */
					}
					g_value_copy(value, &prop->value);
				}
				return;
			}
			G_MIDGARD_LOOP_HIERARCHY_STOP
	}
}

static gboolean __set_property_from_mysql_row(MgdObject *self, GValue *value, GParamSpec *pspec)
{
	if (!self->private->read_only)
		return FALSE;
	
	MgdSchemaPropertyAttr *attr = _midgard_core_class_get_property_attr (G_OBJECT_GET_CLASS(self), pspec->name);
	if (!attr)
		return FALSE;
		
	/* 0 idx is invalid for object's property */
	gint col_idx = attr->property_col_idx;
	if (col_idx == 0)
		return FALSE;

	switch (G_TYPE_FUNDAMENTAL (pspec->value_type)) {

		case G_TYPE_STRING:
			if (self->private->_mysql_row[col_idx] == NULL)
				g_value_set_string (value, "");
			else
				g_value_set_string(value, (gchar *)self->private->_mysql_row[col_idx]);
			break;
		
		case G_TYPE_UINT:
			if (self->private->_mysql_row[col_idx] == NULL)
				g_value_set_uint (value, 0);
			else 
				g_value_set_uint(value, atoi(self->private->_mysql_row[col_idx]));
			break;
		
		case G_TYPE_INT:
			if (self->private->_mysql_row[col_idx] == NULL)
				g_value_set_int (value, 0);
			else 
				g_value_set_int(value, atoi(self->private->_mysql_row[col_idx]));
			break;

		case G_TYPE_BOOLEAN:
			if (self->private->_mysql_row[col_idx] == NULL)
				g_value_set_boolean (value, FALSE);
			else 
				g_value_set_boolean(value, atoi(self->private->_mysql_row[col_idx]));
			break;

		case G_TYPE_FLOAT:
			if (self->private->_mysql_row[col_idx] == NULL)
				g_value_set_float (value, 0.0000);
			else 
				g_value_set_float (value, g_ascii_strtod(self->private->_mysql_row[col_idx], NULL));
			break;

		default:
			g_warning ("Unhandled %s value type for %s property", G_VALUE_TYPE_NAME (value), pspec->name);
			break;
	}

	return TRUE;
}

/* Get object's property */
static void
_object_get_property (GObject *object, guint prop_id,
		GValue *value, GParamSpec   *pspec)
{
	gint prop_id_local = 0;
	MgdSchemaTypeAttr *priv;
	GType current_type = G_TYPE_FROM_INSTANCE(object);
	MgdObject *self = (MgdObject *) object;

	switch(prop_id) {

		case MIDGARD_PROPERTY_GUID:
			g_value_set_string (value, self->private->guid);
			break;
		
		case MIDGARD_PROPERTY_SITEGROUP:
			g_value_set_uint (value, (guint)self->private->sg);
			break;
		
		case MIDGARD_PROPERTY_METADATA:
			g_value_set_object(value,
					(MidgardMetadata *) self->metadata);
			break;
		
		default:
			if (__set_property_from_mysql_row(self, value, pspec))
				return;
			G_MIDGARD_LOOP_HIERARCHY_START
			prop_id_local = prop_id - priv->base_index - 1;
			if ((prop_id_local >= 0) && (
						prop_id_local < priv->num_properties)) {
				if (priv->num_properties) {
					if (priv->properties && G_IS_VALUE(
								&priv->properties[prop_id_local]->value)) {
						g_value_copy(&priv->properties[prop_id_local]->value, 
								value);
					}
				}
				return;
			}
			G_MIDGARD_LOOP_HIERARCHY_STOP
	}
}

/* 
 * Finalizer for GMidgardObject instance.
 * Cleans up all allocated data. As optimization, it handles all data
 * which belongs to its ancestors up to but not inluding GObject.
 * It is really makes no sense to call this function recursively
 * for each ancestor because we already know object's structure.
 * For GObject we call its finalizer directly.
 */

static void _object_finalize (GObject *object) 
{
	guint idx;
	MgdSchemaTypeAttr *priv;
	
	if(object == NULL)
		return;
	
	MgdObject *self = (MgdObject *)object;
	
	/* Free private struct members and MidgardTypePrivate */
	g_free((gchar *)self->private->guid); 
	g_free((gchar *)self->private->action);
	g_free(self->private->exported);
	g_free(self->private->imported);
	
	/* Free object's parameters */
	if(self->private->parameters != NULL) {
		
		GSList *_param = self->private->parameters;
		for (; _param; _param = _param->next) {
			g_object_unref(_param->data);	
		}
		g_slist_free(_param);
	}
	self->private->parameters = NULL;

	if(self->private->_params != NULL)
		g_hash_table_destroy(self->private->_params);
	self->private->_params = NULL;

	/* Decrease "reference count" of underlying mysql res */
	if (self->private->read_only && 
			(self->private->_mysql &&self->private->_mysql->res_ref_count > 0 )) {
		self->private->_mysql->res_ref_count--;
		if (self->private->_mysql->res_ref_count == 0 && self->private->_mysql->res) {
			mysql_free_result(self->private->_mysql->res);
			self->private->_mysql->res = NULL;
			g_free(self->private->_mysql);
		}
	}
	g_free(self->private);
	self->private = NULL;

	/* FIXME emmit signal?*/
	g_object_run_dispose(G_OBJECT(self->metadata));
	g_object_unref(self->metadata);
	
	GType current_type = G_TYPE_FROM_INSTANCE(object); 

	G_MIDGARD_LOOP_HIERARCHY_START
		for (idx = 0; idx < priv->num_properties; idx++) {
			if (priv->properties[idx]) {
				if (G_IS_VALUE(&priv->properties[idx]->value)) {
					g_value_unset(&priv->properties[idx]->value);
				}
				g_free (priv->properties[idx]);
				priv->properties[idx] = NULL;
			}
		}
	if (priv->properties) {
		g_free (priv->properties);
		priv->properties = NULL;
	}
	G_MIDGARD_LOOP_HIERARCHY_STOP
		
	/* Call parent's finalizer if it is there */
	{
		GObjectClass *parent_class = g_type_class_ref (current_type);		
		if (parent_class->finalize) {
			parent_class->finalize (object);
		}
		g_type_class_unref (parent_class);
	}
}

static GHashTable *_build_create_or_update_query(MgdObject *object, guint build_or_update)
{
	GParamSpec **props;
	guint propn, i;
	GValue pval = {0,};
	GHashTable *sqls = midgard_hash_strings_new();
	gchar **table, *sqlset = "", *sqlsetf = "";
	const gchar *nick, *strval;
	GString *tmpsql;
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	const gchar *primary_property = midgard_object_class_get_primary_property(klass);

	if(( props = g_object_class_list_properties(
					G_OBJECT_GET_CLASS(G_OBJECT(object)), &propn)) == NULL)
		return NULL;

	g_hash_table_insert(sqls, g_strdup(
				midgard_object_class_get_table(
					MIDGARD_OBJECT_GET_CLASS(object))), g_strdup(sqlset));
	
	for (i = 0; i < propn; i++) {
		
		nick =  g_param_spec_get_nick (props[i]);
	
		if ((strlen(nick)) > 0) {
			
			table = g_strsplit_set(nick, ".", 0);

			/* We need to add sid field a bit later, after we can 
			 * get original multilanged object id.
			 * We do not do this later to avoid duplicated query. */
			if(build_or_update == _SQL_QUERY_UPDATE) {
				if(g_str_equal(table[1], "sid")) {
					g_strfreev(table);
					continue;
				}
			}

			g_value_init(&pval,props[i]->value_type);
			g_object_get_property(G_OBJECT(object), (gchar*)props[i]->name, &pval);

			if ((sqlsetf = g_hash_table_lookup(sqls, table[0])) == NULL) {
				sqlset = g_strdup("");        
				g_hash_table_insert(sqls, g_strdup(table[0]), sqlset);
			}

			tmpsql = g_string_new(sqlsetf);
			gchar *lstring;
			GValue fval = {0, };
			
			switch (props[i]->value_type) {
				
				case MGD_TYPE_STRING:
					strval = g_value_get_string(&pval);
					if(strval == NULL)
						strval = "";
					guint length = strlen(strval);
					gchar *escaped = g_new(gchar, 2 * length + 1);
					mysql_real_escape_string(
							object->mgd->msql->mysql, 
							escaped, strval, length);
					g_string_append_printf(tmpsql,
							"%s='%s', ",
							table[1],
							escaped);
					g_free(escaped);
					break;

					
				case MGD_TYPE_UINT:
					/* Avoid using primary field with integer type */
					if(!g_str_equal(props[i]->name, primary_property)) {
						g_string_append_printf(tmpsql,
								"%s=%d, ",
								table[1],
								g_value_get_uint(&pval));
					}
					break;
					
				case MGD_TYPE_INT:
					if(!g_str_equal(props[i]->name, primary_property)) {
						g_string_append_printf(tmpsql,
								"%s=%d, ",
								table[1],
								g_value_get_int(&pval));
					}
					break;					
				
				case MGD_TYPE_FLOAT:
					lstring = setlocale(LC_NUMERIC, "0");
					setlocale(LC_NUMERIC, "C");
					g_value_init(&fval, G_TYPE_FLOAT);
					g_value_copy(&pval, &fval);
					g_string_append_printf(tmpsql,
							"%s=%f, ",
							table[1], 
							g_value_get_float(&fval));
					g_value_unset(&fval);
					setlocale(LC_ALL, lstring);
					break;
				
				case MGD_TYPE_BOOLEAN:
					g_string_append_printf(tmpsql,
							"%s=%d, ",
							table[1],
							g_value_get_boolean(&pval));
					break;						
			}

			g_hash_table_insert(sqls, g_strdup(table[0]), g_string_free(tmpsql, FALSE));
			g_value_unset(&pval);  
			g_strfreev(table);
		}
	}
	
	g_free(props);
	return sqls;    
}

void _insert_records(gpointer key, gpointer value, gpointer userdata)
{
	gchar *table = (gchar *) key;
	gchar *sql = (gchar *) value;
	MgdSchemaTypeQuery *query;
	gint qr, rid;
	MgdObject *object = (MgdObject *) userdata;
	guint object_sitegroup = object->private->sg;
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);

	if(!g_str_equal(midgard_object_class_get_table(klass), table)){
		query = g_hash_table_lookup(klass->storage_data->tables, table);
		if (query->use_lang) {
			//langsql = mgd_format(cmo->mobj->mgd, pool,
			//    ", lang=$d ", cmo->mobj->mgd->lang );
			//sqlf = g_strconcat(sql, langsql, NULL);
		}

		GString *fquery = g_string_new("INSERT INTO ");
		g_string_append_printf(fquery,
				"%s SET %s sitegroup=%d",
				table, 
				sql,
				object_sitegroup);
		gchar *tmpstr = g_string_free(fquery, FALSE);
		
		qr = mysql_query(object->mgd->msql->mysql, tmpstr);
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", tmpstr);
		
		if (qr != 0) {
			g_warning("query failed: %s \n %s", 
					tmpstr, 
					mysql_error(object->mgd->msql->mysql));
		} else {
			/* Keep backward compatibility ( repligard's table )*/
			/* No need to define typename here. It's only type's
			 * "external" data */
			if ((query->use_lang == 1) && (qr == 0)) {
				rid = mysql_insert_id(object->mgd->msql->mysql);
				gchar *guid = midgard_guid_new(object->mgd);
				fquery =  g_string_new("INSERT INTO repligard ");
				g_string_append_printf(fquery,
						"(realm,guid,id,changed,action,sitegroup)"
						" VALUES ('%s', '%s', %d , NULL, 'create', %d)", 
						table, guid, rid, object_sitegroup);
				gchar *rtmpstr = g_string_free(fquery, FALSE);
				g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", rtmpstr);
				qr = mysql_query(object->mgd->msql->mysql, rtmpstr);
				if (qr != 0) 
					g_warning("query failed: %s \n %s",
							rtmpstr,
							mysql_error(object->mgd->msql->mysql));
				g_free(guid);
				g_free(rtmpstr);
			}
		}		
		g_free(tmpstr);
	}
}

void _update_records(gpointer key, gpointer value, gpointer userdata)
{
	gchar *table = (gchar *) key;
	gchar *sql = (gchar *) value, *tmpstr = NULL;
	const gchar *otable;
	MgdSchemaTypeQuery *query;
	gint qr, sq;
	GString *where, *ml_select;
	MgdObject *object = (MgdObject *) userdata;
	guint id = 0, rid = 0;
	gchar *new_sql = NULL;
	MYSQL_RES *results;
	MYSQL_ROW row;


	MidgardObjectClass *klass = 
		MIDGARD_OBJECT_GET_CLASS(object);
	const gchar *main_table = midgard_object_class_get_table(klass);

	g_object_get(G_OBJECT(object), "id",
			&id, NULL);
	
	otable = midgard_object_class_get_table(
			MIDGARD_OBJECT_GET_CLASS(object));
	
	if(!g_str_equal(otable, table)){
			
		query = g_hash_table_lookup(object->data->tables, table); 
		if(!query)
			return;
		
		if (query->use_lang) {
			ml_select = g_string_new(" SELECT ");
			g_string_append_printf(ml_select,
					"%s.id, %s.id FROM %s,%s WHERE "
					"guid = '%s' AND lang = %d "
					"AND %s.sid = %s.id " 
					"AND %s.sitegroup IN (0, %d)",
					table, main_table, 
					table, main_table,
					object->private->guid,
					mgd_lang(object->mgd),
					table, main_table,  
					main_table,
					object->private->sg);
			tmpstr = g_string_free(ml_select, FALSE);
			sq = mysql_query(object->mgd->msql->mysql, tmpstr);
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", tmpstr);
			g_free(tmpstr);
			
			if(sq == 0) {
				
				guint ret_rows;
				guint source_id;
				results = 
					mysql_store_result(object->mgd->msql->mysql);

				if(results) {

					if ((ret_rows = mysql_num_rows(results)) > 0) {
						row = mysql_fetch_row(results);
						rid = atoi(row[0]);
						source_id = atoi(row[1]);
						g_object_set(G_OBJECT(object), 
								"sid", rid, NULL);
						GString *_nsql = 
							g_string_new(sql);
						g_string_append_printf(_nsql,
								" sid = %d",
								source_id);
						new_sql = g_string_free(_nsql, FALSE);
					}
					mysql_free_result(results);
				}
			}
				
			if(rid > 0) {
				
				where = g_string_new("UPDATE ");	
				g_string_append_printf(where, 
						"%s SET %s "
						"WHERE id=%d AND sitegroup=%d",
						table, 
						new_sql ? new_sql : sql, 
						rid, object->private->sg);
				if(new_sql)
					g_free(new_sql);

			} else {

				GString *insq = g_string_new("SELECT ");
				g_string_append_printf(insq, 
						"id from %s WHERE "
						"guid = '%s' AND "
						"sitegroup = %d ", 
						main_table,
						object->private->guid,
						object->private->sg);
				tmpstr = g_string_free(insq, FALSE);

				sq = mysql_query(object->mgd->msql->mysql, 
						tmpstr);
				g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, 
						"query=%s", tmpstr);
				g_free(tmpstr);

				results = mysql_store_result(
						object->mgd->msql->mysql);

				if(!results)
					g_error("ID for %s guid not found", 
							object->private->guid);

				if (mysql_num_rows (results) == 0) {

					g_warning ("ID for %s guid not found", object->private->guid);
					mysql_free_result (results);
					return;
				}

				row = mysql_fetch_row(results);
				rid = atoi(row[0]);

				where = g_string_new("INSERT INTO ");
				g_string_append_printf(where, 
						"%s SET %s sid=%d, sitegroup=%d",
						table, sql,
						rid, object->private->sg);

				mysql_free_result(results);
			}

		} else {
			/* TODO Add "join" fields */
			where = g_string_new("UPDATE ");
			g_string_append_printf(where,
					"%s SET %s, sitegroup=%d",
					table, 
					new_sql ? new_sql : sql, 
					object->private->sg);
			if(new_sql)
				g_free(new_sql);
		}

		tmpstr = g_string_free(where, FALSE);
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", tmpstr);
		qr = mysql_query(object->mgd->msql->mysql, tmpstr);
						
		g_free(tmpstr);
	}                                                                                       
}

static gboolean _is_circular(MgdObject *object)
{
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);

	GParamSpec *pspec = 
		g_object_class_find_property(G_OBJECT_GET_CLASS(object), "id");

	/* There's no id property so we do not have up or parent reference */ 
	if(!pspec) 
		return FALSE;

	guint oid = 0;
	g_object_get(G_OBJECT(object), "id", &oid, NULL);

	/* Probably it's creation */ 
	if(oid == 0)
		return FALSE;
	
	const gchar *up_prop = 
		midgard_object_class_get_parent_property(klass);

	/* there's goto statement, because we might want to check other 
	 * circular references in a near future */
	if(up_prop == NULL)
		goto _CHECK_IS_UP_CIRCULAR;

_CHECK_IS_UP_CIRCULAR:
	up_prop = midgard_object_class_get_up_property(klass);

	if(up_prop == NULL)
		return FALSE;

	/* we checked up property so "parent" class is of the same type */
	GValue *idval = g_new0(GValue, 1);
	g_value_init(idval , G_TYPE_UINT);
	g_value_set_uint(idval, oid);
	MidgardCollector *mc = 
		midgard_collector_new(object->mgd->_mgd,
				G_OBJECT_TYPE_NAME(object),
				up_prop, idval);
	
	midgard_collector_set_key_property(mc, "id", NULL);

	if(!midgard_collector_execute(mc)){
		g_object_unref(mc);
		return FALSE;
	}

	gchar **keys = 
		midgard_collector_list_keys(mc);

	g_object_unref(mc);

	if(keys == NULL)
		return FALSE;

	guint id_up = 0;
	g_object_get(G_OBJECT(object), up_prop, &id_up, NULL);

	guint i = 0;
	while(keys[i] != NULL) {
		
		if((guint)atol(keys[i]) == id_up) {
		
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_TREE_IS_CIRCULAR);
			return TRUE;
		}

		i++;
	}
	
	return FALSE;
}

/* TODO, replace execute with distinct if needed */  
guint _object_in_tree(MgdObject *object)
{	
	GParamSpec *name_prop, *up_prop;
	GValue pval = {0,};

	if(_is_circular(object))
		return OBJECT_IN_TREE_DUPLICATE;

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	const gchar *upname = midgard_object_class_get_parent_property(klass);
	if(upname == NULL)
		upname = midgard_object_class_get_up_property(klass);

	if(upname == NULL)
		return OBJECT_IN_TREE_NONE;
	
	up_prop = g_object_class_find_property(
			G_OBJECT_GET_CLASS(G_OBJECT(object)), upname);
	name_prop = g_object_class_find_property(
			G_OBJECT_GET_CLASS(G_OBJECT(object)), "name");

	if((name_prop == NULL) || (up_prop == NULL))
		return OBJECT_IN_TREE_NONE;
	
	MidgardQueryBuilder *builder =
		midgard_query_builder_new(object->mgd,
				G_OBJECT_TYPE_NAME(object));
	/* Add up or parent constraint */
	g_value_init(&pval,up_prop->value_type);
	g_object_get_property(G_OBJECT(object), upname, &pval);
	midgard_query_builder_add_constraint(builder,
			upname,
			"=", &pval);
	g_value_unset(&pval);
			
	/* Add name property constraint */
	g_value_init(&pval,name_prop->value_type);
	g_object_get_property(G_OBJECT(object), "name", &pval);
	
	/* Name property declared non string, consider it empty and thus return IN_TREE_NONE */
	if (name_prop->value_type != G_TYPE_STRING) {

		g_object_unref (builder);
		g_value_unset (&pval);
		return OBJECT_IN_TREE_NONE;
	}

	/* Check empty name */	
	const gchar *tmpstr = NULL;
	tmpstr = g_value_get_string(&pval);
	if (tmpstr == NULL)
		tmpstr = "";
	if (g_str_equal(tmpstr, "")){
		g_object_unref(builder);
		g_value_unset(&pval);
		return OBJECT_IN_TREE_NONE;
	}
	midgard_query_builder_add_constraint(builder,
			"name",
			"=", &pval);
	g_value_unset(&pval);

	/* Add sitegroup property constraint if user is root.
	 * We check if record exists in object's sitegroup scope!
	 * Not in application's one*/
	if(mgd_isroot(object->mgd)){
		
		GValue gval = {0, }, sgval = {0, };

		g_value_init(&gval,G_TYPE_VALUE_ARRAY);
		GValueArray *array =
			g_value_array_new(2);
		
		/* Sitegroup 0 */
		g_value_init(&sgval, G_TYPE_UINT);
		g_value_set_uint(&sgval, 0);
		g_value_array_append(array, &sgval);
		g_value_unset(&sgval);
		
		/* Object's sitegroup */
		g_value_init(&sgval, G_TYPE_UINT);
		g_value_set_uint(&sgval, (guint) object->private->sg);
		g_value_array_append(array, &sgval);
		g_value_unset(&sgval);
		
		g_value_take_boxed(&gval, array);
		midgard_query_builder_add_constraint(builder,
				"sitegroup",
				"IN", &gval);
		g_value_unset(&gval);	
	}
		
	GObject **ret_object = 
		midgard_query_builder_execute(
				builder, NULL);

	/* Name is not duplicated, return */
	if(!ret_object){
		g_object_unref(G_OBJECT(builder));
		return OBJECT_IN_TREE_NONE;
	}
			
	gboolean duplicate = TRUE;

	const gchar *retguid = MIDGARD_OBJECT(ret_object[0])->private->guid;
	const gchar *guid = object->private->guid;
	
	if(guid == NULL)
		duplicate = FALSE;

	if(retguid == NULL) {
		duplicate = FALSE;
		g_warning("Database object has NULL guid. Possible database corruption");
	}

	if(guid != NULL && retguid != NULL) {		
		if(g_str_equal(retguid, guid))
		duplicate = FALSE;	
	}

	/* We must compare langs here.
	 * If database object with name and up exists and have different lang 
	 * we HAVE TO return OBJECT_IN_TREE_OTHER_LANG.
	 * Later we have to switch to update instead of create */
	gint object_lang, dbobject_lang;
	if(midgard_object_class_is_multilang(klass)){
		
		g_object_get(G_OBJECT(object), "lang", &object_lang, NULL);
		g_object_get(G_OBJECT(ret_object[0]), "lang", &dbobject_lang, NULL);

		if(object_lang != dbobject_lang) {
			g_object_unref(ret_object[0]);
			g_object_unref(builder);
			return OBJECT_IN_TREE_OTHER_LANG;
		}
	}

	g_object_unref(ret_object[0]);
	g_free(ret_object);
	g_object_unref(builder);
		
	if(duplicate){

		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_DUPLICATE);
		return OBJECT_IN_TREE_DUPLICATE;
	}
	
	/* PP:I am not sure if we really need this for every object. 
	 * Static path for dynamic applications is needless.
	 * However path should be build or used for applications 
	 * which do not change paths for libraries snippets code
	 * */
	/* (void *) midgard_object_build_path(mobj); */
  
	return OBJECT_IN_TREE_NONE;
}

gboolean midgard_object_set_guid(MgdObject *self, const gchar *guid)
{
	g_assert(self != NULL);
	g_assert(guid != NULL);

	MIDGARD_ERRNO_SET(self->mgd, MGD_ERR_OK);

	if(self->private->guid != NULL) {
		
		MIDGARD_ERRNO_SET(self->mgd, MGD_ERR_INVALID_PROPERTY_VALUE);
		return FALSE;
	}

	if(!midgard_is_guid(guid)) {
		
		MIDGARD_ERRNO_SET(self->mgd, MGD_ERR_INVALID_PROPERTY_VALUE);
		return FALSE;
	}

	MgdObject *dbobject = 
		midgard_object_class_get_object_by_guid(self->mgd->_mgd, guid);

	if(dbobject) {

		MIDGARD_ERRNO_SET(self->mgd, MGD_ERR_DUPLICATE);
		g_object_unref(dbobject);
		return FALSE;
	}

	self->private->guid = g_strdup(guid);

	return TRUE;
}

gboolean _midgard_object_update(MgdObject *gobj, 
		_ObjectActionUpdate replicate)
{
	g_assert(gobj != NULL);
	g_assert(gobj->mgd != NULL);
	
	gchar *sqlsetf = "", *fquery = "";
	guint  qr, oid;
	GHashTable *sqls;
	const gchar *table;
	MidgardConnection *mgd = MGD_OBJECT_CNC (gobj);

	if(_midgard_object_violates_sitegroup(gobj))
		return FALSE;

	if(!_midgard_core_object_is_valid(gobj))
		return FALSE;

	MIDGARD_ERRNO_SET(gobj->mgd, MGD_ERR_OK);

	/* Disable person check */
	/*
	if(gobj->mgd->person == NULL) {
		MIDGARD_ERRNO_SET(gobj->mgd, MGD_ERR_ACCESS_DENIED);
		return FALSE;
	}
	*/
	
	if (gobj->data == NULL) {
		MIDGARD_ERRNO_SET(gobj->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	}

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(gobj);
	
	table = midgard_object_class_get_table(klass);
    
	if(table  == NULL) {
		/* Object has no storage defined. Return FALSE as there is nothing to update */
		g_warning("Object '%s' has no table defined!", G_OBJECT_TYPE_NAME(gobj));
		MIDGARD_ERRNO_SET(gobj->mgd, MGD_ERR_OBJECT_NO_STORAGE);
		return FALSE;
	}

	/* Check duplicates and parent field */
	if(_object_in_tree(gobj) == OBJECT_IN_TREE_DUPLICATE) 
		return FALSE;	
	
	/* Set sid and lang property */    
	if (midgard_object_class_is_multilang(klass)) {
		g_object_get((GObject *) gobj, "id", &oid , NULL);
		g_object_set((GObject *) gobj, "sid", oid , NULL);

		if(replicate == OBJECT_UPDATE_NONE) {
			g_object_set(G_OBJECT(gobj), "lang",
				mgd_lang(gobj->mgd), NULL);
		}
	}

	sqls = _build_create_or_update_query(gobj, _SQL_QUERY_UPDATE);
	sqlsetf = g_hash_table_lookup(sqls, gobj->data->table);

	GString *sql = g_string_new("UPDATE ");

	/* Do not make typical update if we're in exported mode at least */
	switch(replicate) {

		case OBJECT_UPDATE_EXPORTED:
			g_string_append_printf(sql, "%s SET ", table);
			break;

		default:
			g_string_append_printf(sql, 
					"%s SET %s ",
					table, sqlsetf);
			break;
	}
	
	/* METADATA START */
	gchar *person_guid ;
	MgdObject *person = (MgdObject *)gobj->mgd->person;		
	if(person){
		if(G_IS_OBJECT(person))
			g_object_get(G_OBJECT(person), "guid", &person_guid, NULL);
	}else {
		/* g_info("Anonymous mode"); */
		person_guid = g_strdup("");
	}
		
	GValue tval = {0, };
	g_value_init(&tval, midgard_timestamp_get_type());
	midgard_timestamp_set_time(&tval, time(NULL));
	gchar *timeupdated = midgard_timestamp_dup_string(&tval);

	/* If object is imported it's revised time should be 
	 * untouched as property value is copied from imported 
	 * object */
	gchar *revised_time;
	gint revision_count = gobj->metadata->private->revision;
	
	if(replicate == OBJECT_UPDATE_IMPORTED){
		revised_time = gobj->metadata->private->revised;
	} else {
		revised_time = timeupdated;
		revision_count++;
	}

	gchar *metasql;

	switch(replicate){

		case OBJECT_UPDATE_NONE:			
			g_string_append_printf(sql,
					"metadata_revisor='%s', metadata_revised='%s',"
					"metadata_revision=%d, metadata_isapproved=0 ",
					person_guid, revised_time, revision_count);
			
			metasql = midgard_metadata_get_sql(gobj->metadata);
			g_string_append_printf(sql, " %s ", metasql);
			g_free(metasql);
		
			g_free(gobj->metadata->private->revisor);
			gobj->metadata->private->revisor = g_strdup(person_guid);
			g_free(gobj->metadata->private->revised);
			gobj->metadata->private->revised = g_strdup(timeupdated);
			gobj->metadata->private->revision = revision_count;
			
			gobj->metadata->private->approve_is_set = TRUE;
			gobj->metadata->private->is_approved = FALSE;

			break;
		
		case OBJECT_UPDATE_EXPORTED:
			/* Set defaults for some old objects */
			if(!gobj->metadata->private->revised
					&& !gobj->metadata->private->created){
				gobj->metadata->private->created = 
					g_strdup(timeupdated);
				gobj->metadata->private->revised = 
					g_strdup(timeupdated);
				g_string_append_printf(sql,
						"metadata_revised='%s', "
						"metadata_created='%s', ",
						timeupdated, timeupdated);
			}

			if(!gobj->metadata->private->revised
					&& gobj->metadata->private->created){
				gobj->metadata->private->revised = 
					g_strdup(gobj->metadata->private->created);
				g_string_append_printf(sql,
						"metadata_revised='%s', "
						"metadata_revision=1, ", 
						gobj->metadata->private->created);
				gobj->metadata->private->revision = 1;
			}

			if(!gobj->metadata->private->created
					&& gobj->metadata->private->revised){
				gobj->metadata->private->created = 
					g_strdup(gobj->metadata->private->revised);
				g_string_append_printf(sql,
						"metadata_created='%s', ",
						gobj->metadata->private->revised);
			}

			g_string_append_printf(sql,
					"metadata_exported='%s'",
					timeupdated);
			g_free(gobj->metadata->private->exported);
			gobj->metadata->private->exported = g_strdup(timeupdated);
			break;

		case OBJECT_UPDATE_IMPORTED:
			g_string_append_printf(sql,
					"metadata_imported='%s'"
					",metadata_exported='%s'"
					",metadata_revisor='%s' "
					",metadata_revised='%s' "
					",metadata_revision=%d  "
					",metadata_creator='%s' ",
					timeupdated, 
					gobj->metadata->private->exported,
					gobj->metadata->private->revisor,
					revised_time,
					revision_count,
					gobj->metadata->private->creator);
			
			metasql = midgard_metadata_get_sql(gobj->metadata);
			g_string_append_printf(sql, " %s ", metasql);
			g_free(metasql);

			g_free(gobj->metadata->private->imported);
			gobj->metadata->private->imported = g_strdup(timeupdated);
			break;
	}
	/* METADATA END */
	
	g_free(timeupdated);
	g_free(person_guid);

	/* Identify object by guid only */
	if(gobj->private->guid == NULL)
		g_critical("Object's guid is NULL. Can not update");
	g_string_append_printf(sql,
			" WHERE guid = '%s'", gobj->private->guid);

	/* Always add sitegroup, even if user is root.
	 * It guarantees that we update correct object identified by guid
	 * in correct sitegroup */ 
	g_string_append_printf(sql,
			" AND %s.sitegroup = %d",
			table,
			gobj->private->sg);	
	
	guint object_size;
	g_object_get(G_OBJECT(gobj->metadata), "size", &object_size, NULL);
	fquery = g_string_free(sql, FALSE);
	if(midgard_quota_size_is_reached(gobj, object_size)){
		MIDGARD_ERRNO_SET(gobj->mgd, MGD_ERR_QUOTA);
		g_free(fquery);
		return FALSE;
	}
    
	/* Execute update query */
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", fquery);
	qr = mysql_query(gobj->mgd->msql->mysql, fquery);
	g_free(fquery);
	
	if (qr != 0) {

		g_warning("query failed: %s", 
				mysql_error(gobj->mgd->msql->mysql));
		MIDGARD_ERRNO_SET(gobj->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	
	} else {
		
		/* Get record's id in additional SELECT.
		 * Object's id can be incorrectly set here by application 
		 * or reset to default 0 during replication's import */

		GString *idsql = g_string_new("SELECT id FROM ");
		g_string_append_printf(idsql, 
				"%s WHERE guid = '%s' "
				"AND %s.sitegroup = %d", 
				table, gobj->private->guid,
				table, gobj->private->sg);
		
		midgard_res *res;
		gchar *idquery = g_string_free(idsql, FALSE);
		res = mgd_query(gobj->mgd, idquery);
		if(res && mgd_fetch(res)){
			guint _oid = (guint)mgd_sql2id(res,0);
			if(res) mgd_release(res);
			g_object_set(G_OBJECT(gobj), "id", _oid, NULL);
		}
		g_free(idquery);
	
		midgard_quota_update(gobj);   
	
		if (MGD_CNC_REPLICATION (mgd)) {	
			sql = g_string_new(
					"UPDATE repligard SET changed=NULL,action='update',");
			g_string_append_printf(sql,
					"typename='%s', object_action=%d "
					" WHERE guid='%s' AND realm='%s' "
					" AND sitegroup = %d",
					G_OBJECT_TYPE_NAME(gobj),
					MGD_OBJECT_ACTION_UPDATE,
					gobj->private->guid,
					table, gobj->private->sg);
			fquery = g_string_free(sql, FALSE);
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", fquery);
			mysql_query(gobj->mgd->msql->mysql, fquery);
			g_free(fquery);
			/* Should I worry about error in repligard table here? */		
		}
	}

	/* sqls = _build_create_or_update_query(gobj); */
	g_hash_table_foreach(sqls, _update_records, gobj);
	g_hash_table_destroy(sqls);   
	MIDGARD_ERRNO_SET(gobj->mgd, MGD_ERR_OK);

	/* Success, emit done signals */
	switch(replicate){
		
		case OBJECT_UPDATE_NONE:
			g_signal_emit(gobj, MIDGARD_OBJECT_GET_CLASS(gobj)->signal_action_updated, 0);
			break;
			
		case OBJECT_UPDATE_EXPORTED:
			g_signal_emit(gobj, MIDGARD_OBJECT_GET_CLASS(gobj)->signal_action_exported, 0);
			break;
			
		case OBJECT_UPDATE_IMPORTED:
			g_signal_emit(gobj, MIDGARD_OBJECT_GET_CLASS(gobj)->signal_action_imported, 0);
			break;
	}
	
	return TRUE;
}

gboolean midgard_object_update(MgdObject *self)
{	
	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_update, 0);
	gboolean rv = _midgard_object_update(self, OBJECT_UPDATE_NONE);
	
	if(rv)	{
		__dbus_send(self, "update");
	}

	return rv;
}

/* Create object's data in storage */
gboolean _midgard_object_create(	MgdObject *object, 
					const gchar *create_guid, 
					_ObjectActionUpdate replicate)
{
	gchar *sqlsetf = "", *fquery;
	guint  qr, rid;
	GHashTable *sqls;
	GString *query;
	
	if (object->data == NULL)
		return FALSE;

	if (replicate == OBJECT_UPDATE_NONE && object->private->guid != NULL) {
		 midgard_set_error(object->mgd->_mgd,
  				 MGD_GENERIC_ERROR,
				 MGD_ERR_DUPLICATE,
				 "Object already created.");
		 return FALSE;
	}

	if(_midgard_object_violates_sitegroup(object))
		return FALSE;

	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);	
	
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS (object);
	const gchar *table = midgard_object_class_get_table (klass);
	MidgardConnection *mgd = MGD_OBJECT_CNC (object);
	/* Disable person check */
	/*
	if(object->mgd->person == NULL) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_ACCESS_DENIED);
		return FALSE;
	}
	*/
	
	if (table == NULL) {

		/* Object has no storage defined. Return FALSE as there is nothing to create */
		g_warning("Object '%s' has no table or storage defined!", G_OBJECT_TYPE_NAME(object));
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OBJECT_NO_STORAGE);    
		return FALSE;  
	}

	/* Create object's guid, only if it's not already set */
	gchar *guid = NULL;
	if (object->private->guid == NULL) {
		
		if(create_guid == NULL)
			guid = midgard_guid_new(object->mgd);
		else
			guid = g_strdup(create_guid);
		
		object->private->guid = guid;
	}

	/* Force multilang property set */
	if (midgard_object_class_is_multilang (klass)) {

		g_object_set (object, "lang", mgd_lang (object->mgd), NULL);
	}

	if (!_midgard_core_object_is_valid(object))
		return FALSE;

	/* If Object has upfield set , check if property which points to up is set and has some value. 
	 * In other case, create Object and do INSERT, 
	 * as object is not "treeable" and may be created without 
	 * any parent object(typical for parameters or attachments).
	 */
	gint is_dup = _object_in_tree(object);
	if (is_dup == OBJECT_IN_TREE_DUPLICATE) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_DUPLICATE);
		return FALSE;
	}
	if (is_dup == OBJECT_IN_TREE_OTHER_LANG) {
		return midgard_object_update(object);
	}

	sqls = _build_create_or_update_query(object, _SQL_QUERY_CREATE);
	sqlsetf = g_hash_table_lookup(sqls, object->data->table);

	/* Create object record in its table */
	query = g_string_new("INSERT INTO ");
	g_string_append_printf(query,
			"%s SET %s guid='%s',",
			table, sqlsetf, object->private->guid);
	
	g_string_append_printf(query,
			"sitegroup=%d,", object->private->sg);
	
	g_hash_table_destroy(sqls); 
	
	/* Set metadata */
	gchar *person_guid ;
	MgdObject *person = (MgdObject *)object->mgd->person;
	if(person){
		if(G_IS_OBJECT(person))
			g_object_get(G_OBJECT(person), "guid", &person_guid, NULL);
	} else {
		/* g_info("Anonymous mode"); */
		person_guid = g_strdup("");
	}
	
	/* Set metadata fields creator and created values*/
	GValue tval = {0, };    
	time_t utctime = time(NULL);
	g_value_init(&tval, MIDGARD_TYPE_TIMESTAMP);    
	midgard_timestamp_set_time(&tval, utctime);
	gchar *timecreated = midgard_timestamp_dup_string(&tval);
	g_value_unset(&tval);	
	guint object_size;
	g_object_get(G_OBJECT(object->metadata), "size", &object_size, NULL);
	
	switch(replicate){

		case OBJECT_UPDATE_NONE:
			
			g_string_append_printf(query,   
					"metadata_creator='%s', metadata_created='%s', "
					"metadata_revised='%s', metadata_revision=0, "
					"metadata_revisor='%s', metadata_size=%d ",      
					person_guid, timecreated,
					timecreated, person_guid,
					object_size);
			
			/* Set metadata properties */
			g_free(object->metadata->private->creator);
			object->metadata->private->creator = g_strdup(person_guid);
			g_free(object->metadata->private->created);
			object->metadata->private->created = g_strdup(timecreated);
			g_free(object->metadata->private->revised);
			object->metadata->private->revised = g_strdup(timecreated);
			g_free(object->metadata->private->revisor);
			object->metadata->private->revisor = g_strdup(person_guid);
			object->metadata->private->revision = 0;
			/* Set metadata published only if it's not set by application */
			if (object->metadata->private->published == NULL) {
				g_free(object->metadata->private->published);
				object->metadata->private->published = g_strdup(timecreated);	
			}
			break;

		case OBJECT_UPDATE_IMPORTED:

			g_string_append_printf(query,   
					"metadata_created='%s', metadata_deleted=%d, "
					"metadata_revised='%s', metadata_revision=%d, "
					"metadata_imported='%s', metadata_size=%d ",      
					object->metadata->private->created, 
					object->metadata->private->deleted,
					object->metadata->private->revised,
					object->metadata->private->revision,
					timecreated, object_size);
			break;

		case OBJECT_UPDATE_EXPORTED:
			/* Do nothing, satisfy compiler */
			break;
	}

	gchar *metasql = midgard_metadata_get_sql(object->metadata); 
	g_string_append_printf(query, " %s ", metasql);

	g_free(metasql);
	g_free(timecreated);
	g_free(person_guid);
	/* metadata end */
	
	fquery = g_string_free(query, FALSE);
	
	if(!midgard_quota_create(object)){
		g_free(fquery);
		return FALSE;
	}
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", fquery);
	
	qr = mysql_query(object->mgd->msql->mysql, fquery);
	g_free(fquery);
	
	if (qr != 0) {
		g_warning("\n\n QUERY FAILED: %s \n\n", mysql_error(object->mgd->msql->mysql));
		g_free(guid);
		object->private->guid = NULL;
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	} else {
		/* Create repligard and other tables entries */
		if ((rid = mysql_insert_id(object->mgd->msql->mysql))){
			g_object_set(G_OBJECT(object), "id", rid, NULL); /* FIXME */		
			midgard_quota_update(object);		
			
			if (MGD_CNC_REPLICATION (mgd)) {
				query = g_string_new("INSERT INTO repligard SET ");
				g_string_append_printf(query, 
						"realm='%s', guid='%s', changed=NULL, "
						"action='create', typename='%s', id=%d, "
						" sitegroup=%d, object_action = %d ",
						table, object->private->guid, 
						G_OBJECT_TYPE_NAME(object), rid, 
						object->private->sg,
						MGD_OBJECT_ACTION_CREATE);
				fquery = g_string_free(query, FALSE);
				g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", fquery);
				qr = mysql_query(object->mgd->msql->mysql, fquery);
				g_free(fquery);
	
				if (qr != 0) {
					g_warning("query failed: %s", mysql_error(object->mgd->msql->mysql));
					MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);                
				}
			}
			
			/* Set sid property to newly created object's id if object has such property */
			if (g_object_class_find_property(
						G_OBJECT_GET_CLASS(G_OBJECT(object)), "sid") != NULL) 
				g_object_set(G_OBJECT(object), "sid", rid, NULL);
			/* Set lang property */
			if (g_object_class_find_property(
						G_OBJECT_GET_CLASS(G_OBJECT(object)), "lang") != NULL)
				g_object_set(G_OBJECT(object), 
						"lang", mgd_lang(object->mgd), NULL);
			
			sqls = _build_create_or_update_query(object, _SQL_QUERY_CREATE);
			g_hash_table_foreach(sqls, _insert_records, object);
			
			g_hash_table_destroy(sqls);		
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,"Object %s created with id=%d", 
					G_OBJECT_TYPE_NAME(object), rid);

			/* Emit done signals */
			switch(replicate){
				
				case OBJECT_UPDATE_NONE:
					g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_created, 0);
					break;
					
				case OBJECT_UPDATE_IMPORTED:
					g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_imported, 0);
					break;
					
				default:
					/* do nothing */
					break;
			}
	
			return TRUE;
		}
	}
	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
	return FALSE;   
}

gboolean midgard_object_create(MgdObject *object)
{
	g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_create, 0);
	gboolean rv =  _midgard_object_create(object, NULL, OBJECT_UPDATE_NONE);

	if(rv) {
		__dbus_send(object, "create");
	}

	return rv;
}

void _object_copy_properties(GObject *src, GObject *dest)
{
	g_assert(src != NULL && dest != NULL);

	guint nprop, i;
	GValue pval = {0, };
	GValue doval = {0, };
	GParamSpec **props = g_object_class_list_properties(
			G_OBJECT_GET_CLASS(src),
			&nprop);

	/* sitegroup and guid properties are set directly */
	if(MIDGARD_IS_OBJECT(src) && MIDGARD_IS_OBJECT(dest)) {
		MIDGARD_OBJECT(dest)->private->guid = 
			g_strdup(MIDGARD_OBJECT(src)->private->guid);
		MIDGARD_OBJECT(dest)->private->sg = 
			MIDGARD_OBJECT(src)->private->sg;
	}

	for(i = 0; i <+ nprop; i++){		
		
		g_value_init(&pval, props[i]->value_type);	

		if(props[i]->value_type == G_TYPE_OBJECT){			
		
			g_object_get_property(src,
					props[i]->name,
					&pval);
			
			g_value_init(&doval, props[i]->value_type);
			g_object_get_property(dest,
					props[i]->name,
					&doval);

			_object_copy_properties(
					G_OBJECT(g_value_get_object(&pval)),
					G_OBJECT(g_value_get_object(&doval))
					);
			g_value_unset(&doval);

		} else {

			g_object_get_property(G_OBJECT(src),
					props[i]->name,
					&pval);
			g_object_set_property(G_OBJECT(dest),
					props[i]->name,
					&pval);
		}

		g_value_unset(&pval);
	}

	g_free(props);
}


/* Get object by its id property, id must be uint type */
gboolean  midgard_object_get_by_id(MgdObject *object, guint id)
{
	g_assert(object != NULL);
	g_assert(object->mgd != NULL);

	/* Reset errno */
	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);
	
	GParamSpec *prop;
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	
	/* Find "primary" property , just in case of fatal error */
	prop = g_object_class_find_property(
			(GObjectClass*)klass,
			"id");
	if(prop == NULL){
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		g_warning("Primary property id not found for object's klass '%s'",
				G_OBJECT_TYPE_NAME(object));
		return FALSE;
	}

	/* Stop when property is not uint type */
	g_assert((prop->value_type == G_TYPE_UINT));
	
	/* Initialize QB */
	MidgardQueryBuilder *builder =
		midgard_query_builder_new(object->mgd, 
				G_OBJECT_TYPE_NAME(object));
	
	if(!builder) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		g_warning("Invalid query builder configuration (%s)",
				G_OBJECT_TYPE_NAME(object));
		return FALSE;
	}

	/* Get primary's property value */
	GValue pval = {0,};
	g_value_init(&pval,prop->value_type);	
	g_value_set_uint(&pval, id);
	if (midgard_query_builder_add_constraint(builder,
				"id",
				"=", &pval)) {
		
		GList *olist = 
			_midgard_core_qb_set_object_from_query(builder, MQB_SELECT_OBJECT, object);

		g_value_unset(&pval);	
		g_object_unref(G_OBJECT(builder));
				
		if(!olist) {
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
			return FALSE;
		}
	
		g_list_free(olist);	

		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);
		__dbus_send(object, "get");
		return TRUE;
	}
	return FALSE;
}

GObject **midgard_object_find(MgdObject *object, MidgardTypeHolder *holder)
{
	GParamSpec **props;
	guint propn, i;
	GValue pval = {0,};
	
	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);

	props = g_object_class_list_properties(
			G_OBJECT_CLASS(MIDGARD_OBJECT_GET_CLASS(object)), &propn);
	if(!props){
		g_warning("No properties found for %s class!",
				object->cname);
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		return NULL;
	}
	
	MidgardQueryBuilder *builder =
			midgard_query_builder_new(object->mgd, object->cname);
	if(!builder){
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		return NULL;
	}

	for(i = 0; i < propn; i++) {

		g_value_init(&pval,props[i]->value_type);
		g_object_get_property(G_OBJECT(object),props[i]->name, &pval);
		
		switch (props[i]->value_type) {
			
			case G_TYPE_STRING:
				if(g_value_get_string(&pval))
					midgard_query_builder_add_constraint(builder,
							props[i]->name, "=", &pval);
				break;

			case G_TYPE_UINT:
				if(g_value_get_uint(&pval))
					midgard_query_builder_add_constraint(builder,
							props[i]->name, "=", &pval);
				break;
			
			case G_TYPE_INT:
				if(g_value_get_int(&pval))
					midgard_query_builder_add_constraint(builder,
							props[i]->name, "=", &pval);
				break;

			case G_TYPE_FLOAT:
				if(g_value_get_float(&pval))
					midgard_query_builder_add_constraint(builder,
							props[i]->name, "=", &pval);
				break;

			case G_TYPE_BOOLEAN:
				if(g_value_get_boolean(&pval))
					midgard_query_builder_add_constraint(builder,
							props[i]->name, "=", &pval);
				break;

			case G_TYPE_OBJECT:
				/* TODO */
				break;

			default:
				if(g_value_get_string(&pval))
					midgard_query_builder_add_constraint(builder,
							props[i]->name, "=", &pval);
				break;			
		}
		g_value_unset(&pval);
	}
	
	GObject **objects = midgard_query_builder_execute(builder, holder);
	g_object_unref(builder);
	return objects;
}


/* Initialize class. 
 * Properties setting follow data in class_data.
 */ 
static void
_object_class_init(gpointer g_class, gpointer class_data)
{
	MgdSchemaTypeAttr *data = (MgdSchemaTypeAttr *) class_data;
	GObjectClass *gobject_class = G_OBJECT_CLASS(g_class);
	MidgardObjectClass *mklass = (MidgardObjectClass *) g_class;	
	guint idx;

	if(mklass) {
		
		mklass->dbpriv = g_new(MidgardDBObjectPrivate, 1);
		mklass->dbpriv->storage_data = data;
		mklass->dbpriv->set_from_xml_node = NULL;
	}


	gobject_class->set_property = _object_set_property;
	gobject_class->get_property = _object_get_property;
	gobject_class->finalize = _object_finalize;

	if(!signals_registered && mklass) {
		
		mklass->signal_action_create = 
			g_signal_new("action-create",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_create),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE,
					0);

		mklass->signal_action_create_hook =
			g_signal_new("action-create-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_create_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,
					G_TYPE_NONE,
					0);

		mklass->signal_action_created =
			g_signal_new("action-created",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_created),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);
	
		mklass->signal_action_update =
			g_signal_new("action-update",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,	
					G_STRUCT_OFFSET (MidgardObjectClass, action_update),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);
	
		mklass->signal_action_update_hook =
			g_signal_new("action-update-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,	
					G_STRUCT_OFFSET (MidgardObjectClass, action_update_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_updated =
			g_signal_new("action-updated",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_updated),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_delete =
			g_signal_new("action-delete",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_delete),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_delete_hook =
			g_signal_new("action-delete-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_delete_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_deleted =
			g_signal_new("action-deleted",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_deleted),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);
		
		mklass->signal_action_purge =
			g_signal_new("action-purge",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_purge),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_purge_hook =
			g_signal_new("action-purge-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_purge_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_purged =
			g_signal_new("action-purged",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_purged),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_import =
			g_signal_new("action-import",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_import),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_import_hook =
			g_signal_new("action-import-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_import_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_imported =
			g_signal_new("action-imported",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_imported),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_export =
			g_signal_new("action-export",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_export),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_export_hook =
			g_signal_new("action-export-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_export_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_exported =
			g_signal_new("action-exported",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_exported),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_loaded =
			g_signal_new("action-loaded",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_loaded),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_loaded_hook =
			g_signal_new("action-loaded-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_loaded_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_approve =
			g_signal_new("action-approve",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_approve),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);
	
		mklass->signal_action_approve_hook =
			g_signal_new("action-approve_hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_approve_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_approved =
			g_signal_new("action-approved",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_approved),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);
		
		mklass->signal_action_unapprove =
			g_signal_new("action-unapprove",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_unapprove),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_unapprove_hook =
			g_signal_new("action-unapprove_hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_unapprove_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_unapproved =
			g_signal_new("action-unapproved",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_unapproved),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_lock =
			g_signal_new("action-lock",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_lock),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_lock_hook =
			g_signal_new("action-lock-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_lock_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_locked =
			g_signal_new("action-locked",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_locked),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_unlock =
			g_signal_new("action-unlock",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_unlock),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		mklass->signal_action_unlock_hook =
			g_signal_new("action-unlock-hook",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_unlock_hook),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);
		
		mklass->signal_action_unlocked =
			g_signal_new("action-unlocked",
					G_TYPE_FROM_CLASS(g_class),
					G_SIGNAL_ACTION,
					G_STRUCT_OFFSET (MidgardObjectClass, action_unlocked),
					NULL, /* accumulator */
					NULL, /* accu_data */
					g_cclosure_marshal_VOID__VOID,	
					G_TYPE_NONE,
					0);

		signals_registered = TRUE;
	}

	if(mklass)
		mklass->storage_data = data;

	if(G_OBJECT_CLASS_TYPE(g_class) != MIDGARD_TYPE_OBJECT) {
		__add_core_properties(mklass->storage_data);
	}

	g_type_class_add_private (g_class, 
			sizeof(MgdSchemaTypeAttr));
	if(data)
		data->properties = 
			g_malloc(sizeof(MgdSchemaPropertyAttr*)
					*(data->num_properties+1));
		
	/* Note, that we start numbering from 1 , not from 0. property_id must be > 0 */
	for (idx = 1; idx <= data->num_properties; idx++) {
		/* g_debug("Installing property id %d :: %s",
				idx, data->params[idx-1]->name); */
		g_object_class_install_property(
				gobject_class, 
				data->base_index + idx , 
				data->params[idx-1]);
	}  
}


GType
midgard_type_register(const gchar *class_name, MgdSchemaTypeAttr *data)
{
        if (g_type_from_name(class_name)) {
                return g_type_from_name(class_name);
        }
        {
                GTypeInfo *midgard_type_info = g_new0 (GTypeInfo, 1);
		if(data == NULL)
			data = g_new(MgdSchemaTypeAttr, 1);

                data->base_index = 4; /* TODO: refactor this! */
                /* PP: refactor, refactor , registered types hash where used for this 
                 * because GTypyInfo can not be used other way to extend types 
                 * Currently we have one extending level.
                 * Any _midgard_object_class_paramspec GParamSpec change will break "child" types.
                 * There is nothing to do if it was already done.
                 */

                /* our own class size is 0 but it should include space for a parent, therefore add it */
                midgard_type_info->class_size = sizeof(MidgardObjectClass);
                midgard_type_info->base_init = NULL;
                midgard_type_info->base_finalize = NULL;
                midgard_type_info->class_init  = _object_class_init;
                midgard_type_info->class_finalize  = NULL;
                midgard_type_info->class_data = data;
                /* our own instance size is 0 but it should include space for a parent,
                 * therefore add it */
                midgard_type_info->instance_size = sizeof(MgdObject);
                midgard_type_info->n_preallocs = 0;
                midgard_type_info->instance_init = _object_instance_init;
                midgard_type_info->value_table = NULL;
                
		GType type = g_type_register_static(
                                MIDGARD_TYPE_OBJECT, class_name, midgard_type_info, 0);
                
		if(g_str_equal(class_name, "midgard_attachment"))
			_midgard_attachment_type = type;

                g_free(midgard_type_info);
                return type;   
        }                      

}

/* Workaround for midgard_attachment created as MgdSchema class and type */
GType midgard_attachment_get_type(void)
{
	g_assert(_midgard_attachment_type != 0);

	return _midgard_attachment_type;
}

GType midgard_object_get_type(void) 
{
        static GType type = 0;
        if (type == 0) {
                MgdSchemaTypeAttr *class_data = g_new(MgdSchemaTypeAttr, 1);
                class_data->params = _midgard_object_class_paramspec();
                class_data->base_index = 0;
                class_data->num_properties = 0;
                while (class_data->params[class_data->num_properties]) {
                        class_data->num_properties++;
                }

                GTypeInfo info = {
                        sizeof(MidgardObjectClass),
                        NULL,                       /* base_init */
                        NULL,                       /* base_finalize */
                        _object_class_init,          /* class_init */
                        NULL,                       /* class_finalize */
                        class_data,	                    /* class_data */
                        sizeof(MgdObject),
                        0,                          /* n_preallocs */
                        NULL       /* instance_init */
                };
                type = g_type_register_static(
                        MIDGARD_TYPE_DBOBJECT, "MidgardObjectClass", &info,
                        G_TYPE_FLAG_ABSTRACT);
        }
        return type;
}

void midgard_init()
{
	g_type_init();

	GType type;

	type = MIDGARD_TYPE_BLOB;
	g_assert(type != 0);

	type = MIDGARD_TYPE_USER;
	g_assert(type != 0);
	
	type = MIDGARD_TYPE_CONNECTION;
	g_assert(type != 0);
	
	type = MIDGARD_TYPE_CONFIG;
	g_assert(type != 0);
	
	type = MIDGARD_TYPE_COLLECTOR;
	g_assert(type != 0);
	
	type = MIDGARD_TYPE_QUERY_BUILDER;
	g_assert(type != 0);

	type = MIDGARD_TYPE_SITEGROUP;
	g_assert(type != 0);
}

void mgdlib_init()
{
	midgard_init();
}

MgdObject *
midgard_object_new(midgard *mgd, const gchar *name, GValue *value)
{
	g_return_val_if_fail (mgd != NULL, NULL);
	g_return_val_if_fail (name != NULL, NULL);

	GType type;	
	MgdObject *self;
	const gchar *field = "id";

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS_BY_NAME (name);

	if (!klass) {

		g_warning ("%s class is not registered as midgard_object derived one", name);
		return NULL;
	}

	type = G_OBJECT_CLASS_TYPE (G_OBJECT_CLASS (klass));

	if (!type) {
		
		g_warning ("Failed to retrieve %s class type from GType system", name);
		return NULL;
	}

	/* Empty object instance */
	if (value == NULL) {	
		
		self = g_object_new(type, NULL);	
		self->private->sg = mgd_sitegroup(mgd);
	
		if (midgard_object_class_is_multilang (klass)) 
			g_object_set (self, "lang", mgd_lang (mgd), NULL);

	} else {
	
		MidgardQueryBuilder *builder = midgard_query_builder_new(mgd, name);
			
		if (G_VALUE_TYPE(value) == G_TYPE_STRING) {

			if (g_value_get_string(value) == NULL) {
				
				midgard_set_error(mgd->_mgd,
						MGD_GENERIC_ERROR,
						MGD_ERR_INVALID_PROPERTY_VALUE,
						"NULL string value is not a guid.");
				g_clear_error(&mgd->err);
				g_object_unref(builder);
				return NULL;
			}

			if (!midgard_is_guid(g_value_get_string(value))) {

				midgard_set_error(mgd->_mgd,
						MGD_GENERIC_ERROR,
						MGD_ERR_INVALID_PROPERTY_VALUE,
						"String value '%s' is not a guid.",
						g_value_get_string(value));
				g_clear_error(&mgd->err);

				g_object_unref(builder);
				return NULL;
			}
	
			field = "guid";
		} 

		midgard_query_builder_add_constraint (builder, field, "=", value);
		
		MidgardTypeHolder *holder = g_new(MidgardTypeHolder, 1);
		GObject **objects = midgard_query_builder_execute(builder, holder);

		guint n_objects = holder->elements;
		g_free(holder);
		g_object_unref(builder);

		if (!objects) {

			MIDGARD_ERRNO_SET(mgd, MGD_ERR_NOT_EXISTS);
			return NULL;
		}

		guint i = 0;
		if (mgd_isroot(mgd) && n_objects > 1) {

			while(objects[i]) {
				g_object_unref(objects[i]);
				i++;
			}

			g_free(objects);
			MIDGARD_ERRNO_SET(mgd, MGD_ERR_SITEGROUP_VIOLATION);
	
			return NULL;
		}
	
		self = MIDGARD_OBJECT(objects[0]);
		g_free(objects);

		__dbus_send(self, "get");
	}

	if(!self)
		return NULL;

	self->type = type;
	self->data = MIDGARD_OBJECT_GET_CLASS(self)->storage_data;
		
	self->klass = (gpointer) klass;
	self->mgd = mgd;
	self->cname = name;

	return self;
}

MgdObject *midgard_object_new_by_id(
		midgard *mgd, const gchar *name, gchar *id)
{
	MgdObject *object = midgard_object_new(mgd, name, NULL);
	
	if(object == NULL)
		return NULL;
	
	GParamSpec *prop = g_object_class_find_property(
			G_OBJECT_GET_CLASS(G_OBJECT(object)),
			object->data->primary);
	if(prop) {
		switch (prop->value_type) {
			
			case G_TYPE_STRING:
				g_object_set(G_OBJECT(object), 
						object->data->primary , id, NULL);
				if(!midgard_object_get_by_guid(object, (const gchar *)id))
					return NULL;
				break;
				
			case G_TYPE_UINT:
				return NULL;
				break;
		}
		return object;
	}
	g_object_unref(object);
	return NULL;
}

MgdObject * midgard_object_get_parent(MgdObject *mobj)
{
	g_assert(mobj != NULL);
	
	MgdObject *pobj;
	const gchar *pcstring;
	guint puint = 0;
	gint pint = 0;
	GParamSpec *fprop = NULL;
	GValue pval = {0,};
	gboolean ret_object = FALSE;

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(mobj);
	
	if (klass->storage_data->parent == NULL)
		return NULL;

	pobj =  midgard_object_new(mobj->mgd , G_OBJECT_TYPE_NAME(mobj), NULL);
	
	if (pobj == NULL)
		return NULL;

	const gchar *up_property = 
		midgard_object_class_get_up_property(klass);
	
	if(up_property != NULL) {
		fprop = g_object_class_find_property(
				(GObjectClass *) mobj->klass,
				midgard_object_class_get_up_property(klass));
	}

	if (fprop != NULL){
		
		g_value_init(&pval,fprop->value_type);
		g_object_get_property(G_OBJECT(mobj), 
				midgard_object_class_get_up_property(klass), &pval);
		
		switch(fprop->value_type) {
			
			case G_TYPE_STRING:
				
				if ((pcstring = g_value_get_string(&pval)) != NULL) {
					
					ret_object = TRUE;
					if(!midgard_object_get_by_guid(pobj, pcstring)) {
						g_object_unref(pobj);
						pobj = NULL;
					}
				}
				break;

			case G_TYPE_UINT:
				
				if ((puint = g_value_get_uint(&pval))) {

					ret_object = TRUE;
					if(!midgard_object_get_by_id(pobj, puint)) {
						g_object_unref(pobj);
						pobj = NULL;
					}
				}				
				break;

				
			case G_TYPE_INT:

				if ((pint = g_value_get_int(&pval))) {

					ret_object = TRUE;
					if(!midgard_object_get_by_id(pobj, pint)) {
						g_object_unref(pobj);
						pobj = NULL;
					}
				}
				break;
		}
		
		g_value_unset(&pval);

		if(ret_object) {
			return pobj;
		} else {
			g_object_unref(pobj);
			pobj = NULL;
		}
	}
        
        /* I do make almost the same for property_parent, because I want to 
	 * avoid plenty of warnings after G_VALUE_HOLDS which could be used 
	 * with  value returned for mobj->data->tree->property_up 
	 */ 

	if (midgard_object_class_get_parent_property(klass) == NULL)
		return NULL;

	pobj =  midgard_object_new(mobj->mgd , mobj->data->parent, NULL);
	
	if (pobj == NULL)
		return NULL;
	
	fprop = g_object_class_find_property(
			(GObjectClass *) mobj->klass,
			midgard_object_class_get_parent_property(klass));

	if(!fprop)
		return NULL;
       
        g_value_init(&pval,fprop->value_type);
        g_object_get_property(G_OBJECT(mobj), 
			midgard_object_class_get_parent_property(klass) , &pval);

        switch(fprop->value_type) {
            
            case G_TYPE_STRING:
		    
		    if ((pcstring = g_value_get_string(&pval)) != NULL) {
			    
			    if(!midgard_object_get_by_guid(pobj, pcstring)) {
				    g_object_unref(pobj);
				    pobj = NULL;
			    }
		    }
		    break;

            case G_TYPE_UINT:
		    
		    if ((puint = g_value_get_uint(&pval))) {

			    if(!midgard_object_get_by_id(pobj, puint)) {
				    g_object_unref(pobj);
				    pobj = NULL;
			    }
		    }
		    break;

	    case G_TYPE_INT:
		    
		    if ((pint = g_value_get_int(&pval))) {

			    if(!midgard_object_get_by_id(pobj, pint)) {
				    g_object_unref(pobj);
				    pobj = NULL;
			    }
		    }
		    break;


        }
        
        g_value_unset(&pval);

	__dbus_send(pobj, "get");

        return pobj;
}

gboolean midgard_object_get_by_guid(MgdObject *object, const gchar *guid)
{
	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);
	
	if (object->data == NULL) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
		return FALSE; 
	}
	
	if(!midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(object))){
		/* Object has no storage defined. Return NULL as there is nothing to get */
		g_warning("Object '%s' has no table or storage defined!", G_OBJECT_TYPE_NAME(object));
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OBJECT_NO_STORAGE);
		return FALSE;
	}
	
	MidgardQueryBuilder *builder =
		midgard_query_builder_new(object->mgd,
				G_OBJECT_TYPE_NAME(object));
	if(!builder) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	}

	GValue pval = {0,};	
	g_value_init(&pval,G_TYPE_STRING);
	g_value_set_string(&pval, guid);
	if (midgard_query_builder_add_constraint(builder,
				"guid",
				"=", &pval)) {

		GList *olist =
			_midgard_core_qb_set_object_from_query(builder, MQB_SELECT_OBJECT, object);

		g_value_unset(&pval);
		g_object_unref(G_OBJECT(builder));
	
		if(!olist) {
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
			return FALSE;
		}
		
		g_list_free(olist);
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);
		__dbus_send(object, "get");
		return TRUE;
	} 
	
	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
	return FALSE;
	
}

gboolean midgard_object_delete(MgdObject *object)
{
	g_assert(object);

	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);
	g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_delete, 0);

	gchar *query;

	if(_midgard_object_violates_sitegroup(object))
		return FALSE;

	if (object->data == NULL){
		g_warning("No schema attributes for class %s",
				G_OBJECT_TYPE_NAME(object));
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	}

	const gchar *table = 
		midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(object));
	MidgardConnection *mgd = MGD_OBJECT_CNC (object);

	if(table  == NULL) {
		/* Object has no storage defined. Return FALSE as there is nothing to update */
		g_warning("Object '%s' has no table defined!", G_OBJECT_TYPE_NAME(object));
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OBJECT_NO_STORAGE);
		return FALSE;
	}
	
	const gchar *guid = object->private->guid;
	
	if(!guid){
		
		midgard_set_error(MGD_OBJECT_CNC(object),
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_PROPERTY_VALUE,
				"Guid property is NULL. ");
		return FALSE;
	}	

	/* Delete multilingual content if current language != lang0 
           and there's more than one language. */

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	if(midgard_object_class_is_multilang(klass)) {

		guint olang = mgd_lang (MGD_OBJECT_CNC (object)->mgd);
		guint dlang = mgd_get_default_lang (MGD_OBJECT_CNC (object)->mgd);
		if (olang != dlang) {

			guint oid;
			g_object_get(G_OBJECT(object), "id", &oid, NULL);

			GValue *gval = g_new0(GValue, 1);
			g_value_init(gval, G_TYPE_STRING);
			g_value_set_string(gval, object->private->guid);
			MidgardCollector *mc = midgard_collector_new (
				object->mgd->_mgd, G_OBJECT_TYPE_NAME(object),
				"guid", gval);
		
			if(!mc)
				return FALSE;

			midgard_collector_set_key_property(mc, "lang", NULL);
			midgard_collector_unset_languages(mc);

			midgard_collector_execute(mc);		
			gchar **keys = midgard_collector_list_keys(mc);

			guint lang_i = 0;
			if (keys) {
				while (keys[lang_i] != NULL) 
					lang_i++;
			}

			if (keys && lang_i > 1) {

				GString *del = g_string_new("DELETE FROM ");
				g_string_append_printf(del,
					"%s_i WHERE lang = %d "
					"AND sid = %d",
					midgard_object_class_get_table(klass),
					olang,	oid);

				g_debug ("query=%s", del->str);
				mysql_query (object->mgd->msql->mysql, del->str);
				g_string_free (del, TRUE);

				g_free(keys);
				g_object_unref (mc);

				g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_deleted, 0);
				return TRUE;
			}

			if (keys) g_free (keys);
			g_object_unref (mc);		
		}
	}

	GString *sql = g_string_new("UPDATE ");
	g_string_append_printf(sql, "%s SET ", table);
	
	gchar *person_guid;
	MgdObject *person = (MgdObject *)object->mgd->person;
	if(person){

		if(G_IS_OBJECT(person)) {
			person_guid = (gchar *)MGD_OBJECT_GUID(person);
		} else {
			g_warning("Expected person object associated with current connection. Probably found garbage!");
			person_guid = "";
		}
	} else {
		/* g_info("Anonymous mode"); */
		person_guid = "";
	}

	GValue tval = {0, };
	g_value_init(&tval, midgard_timestamp_get_type());
	midgard_timestamp_set_time(&tval, time(NULL));
	gchar *timeupdated = midgard_timestamp_dup_string(&tval);
	object->metadata->private->revision++;
	g_string_append_printf(sql,
			"metadata_revisor='%s', metadata_revised='%s',"
			"metadata_revision=%d, "
			"metadata_deleted=TRUE ",
			person_guid, timeupdated, 
			object->metadata->private->revision);

	g_string_append_printf(sql,
			" WHERE guid = '%s'"
			" AND %s.sitegroup = %d",
			object->private->guid,
			table, object->private->sg);
	
	query = g_string_free(sql, FALSE);
        g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", query);
	gint qr = mysql_query(object->mgd->msql->mysql, query);
	
	if (qr != 0) {

		g_warning("\n\n QUERY %s \n\n FAILED : \n\n %s",
				query,
				mysql_error(object->mgd->msql->mysql));
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		g_free(query);
		g_free(timeupdated);
		object->metadata->private->revision--;
		return FALSE;
	
	} else {
	
		g_free(query);
		
		if (MGD_CNC_REPLICATION (mgd)) {
			sql = g_string_new("UPDATE repligard SET ");
			g_string_append_printf(sql,
					"object_action = %d "
					"WHERE guid = '%s' AND typename = '%s'"
					" AND sitegroup = %d",
					/* FIXME , lang column should be updated when 
					 * we move lang data to object's table again */
					MGD_OBJECT_ACTION_DELETE,
					guid,
					G_OBJECT_TYPE_NAME(G_OBJECT(object)),
					object->private->sg);
		
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", sql->str);
			mysql_query(object->mgd->msql->mysql, sql->str);
			g_string_free(sql, TRUE);
		}
	}

	/* Set object properties */
	g_free(object->metadata->private->revised);
	object->metadata->private->revised = timeupdated;
	g_free(object->metadata->private->revisor);
	object->metadata->private->revisor = g_strdup(person_guid);
	object->metadata->private->deleted = TRUE;

	g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_deleted, 0);

	__dbus_send(object, "delete");

	return TRUE;
}

gboolean midgard_object_purge(MgdObject *object)
{
	gint rv = 0, id = 0;
	GString *dsql;
	gchar *mlt = "";
	midgard_res *res;
	const gchar *guid, *table;
	gchar *tmpstr;

	if(_midgard_object_violates_sitegroup(object))
		return FALSE;

	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);
	g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_purge, 0);

	if (object->data == NULL){
		g_warning("No schema attributes for class %s", 
				G_OBJECT_TYPE_NAME(object));
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		return FALSE; 
	}
	
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	MidgardConnection *mgd = MGD_OBJECT_CNC (object);
	table = midgard_object_class_get_table(klass);   
	if(table == NULL) {
		/* Object has no storage defined. Return -1 as there is nothing to delete */
		g_warning("Object '%s' has no table or storage defined!", G_OBJECT_TYPE_NAME(object));
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OBJECT_NO_STORAGE);    
		return FALSE;
	}

	guid = object->private->guid;
	
	if(!guid){

		midgard_set_error(MGD_OBJECT_CNC(object),
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_PROPERTY_VALUE,
				"Guid property is NULL. ");
		return FALSE;
	}

	if (midgard_object_class_is_multilang(klass)) {
		g_object_get(G_OBJECT(object), "id", &id, NULL);
		if(id < 1) {
			g_warning("Object id < 1. Can not purge language content");
			MIDGARD_ERRNO_SET(object->mgd, 
					MGD_ERR_INVALID_PROPERTY_VALUE);
			return FALSE;
		}
	}

	/* unlink midgard_attachment's file */
	/*const gchar *cname = g_type_name(G_OBJECT_TYPE(object));
	if(g_str_equal(cname, "midgard_attachment")) {
		
		const gchar *blobdir = mgd_get_blobdir(object->mgd);
		gchar *blobpath = NULL, *location;
		struct stat statbuf;

		if (!blobdir || (*blobdir != '/')
				|| (stat(blobdir, &statbuf) != 0)
				|| !S_ISDIR(statbuf.st_mode)) {
			g_error("Invalid blobdir or blobdir is not set");
		}
		
		g_object_get(G_OBJECT(object), "location", &location, NULL);

		if(strlen(location) > 1)
			blobpath = g_strconcat(blobdir,	"/", location, NULL);

		if(!blobpath) {
			g_warning("Couldn't resolve attachment's absolute location");
			g_free(location);
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
			return FALSE;
		}

		if(remove(blobpath) != 0) {
			g_free(blobpath);
			g_warning("Couldn't remove %s file", location);
			g_free(location);
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
			return FALSE;
		}
		
		g_free(location);
		g_free(blobpath);
	}*/

	dsql = g_string_new("DELETE ");
	g_string_append_printf(dsql,
			"FROM %s WHERE guid='%s'",
			table,
			guid);
		
	g_string_append_printf(dsql,
			" AND sitegroup=%d ",
				object->private->sg);

	guint size = midgard_quota_get_object_size(object);
	
	/* TODO , libgda, libgda */
	tmpstr = g_string_free(dsql, FALSE);
	mgd_exec(object->mgd, tmpstr);
	g_free(tmpstr);
	
	rv = mysql_affected_rows(object->mgd->msql->mysql);
	
	if (rv == 0) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
		return FALSE;
	}
	
	midgard_quota_remove(object, size);

	GValue tval = {0, };
	time_t utctime = time(NULL);
	g_value_init(&tval, MIDGARD_TYPE_TIMESTAMP);
	midgard_timestamp_set_time(&tval, utctime);
	gchar *timedeleted = midgard_timestamp_dup_string(&tval);
	g_value_unset(&tval);

	if (MGD_CNC_REPLICATION (mgd)) {
		dsql = g_string_new("UPDATE repligard SET ");
		g_string_append_printf(dsql,
				"updated = 0, action = 'delete', "
				"object_action = %d, object_action_date = '%s' "
				" WHERE typename='%s' AND guid='%s' AND sitegroup = %d",
				MGD_OBJECT_ACTION_PURGE,
				timedeleted,
				G_OBJECT_TYPE_NAME(object),
				object->private->guid, object->private->sg);
		tmpstr = g_string_free(dsql, FALSE);
		mgd_exec(object->mgd, tmpstr);
		g_free(tmpstr);
	}
	
	g_free (timedeleted);

	/* Only support for multilang now */
	if (midgard_object_class_is_multilang(klass)) { 
		
		dsql = g_string_new("DELETE ");
		g_string_append_printf(dsql,
				"FROM %s_i WHERE sid=%d",
				table,
				id);
		
		if(!mgd_isroot(object->mgd))
			g_string_append_printf(dsql,
					" AND sitegroup=%d ",
					object->private->sg);

		/* Get multilang table name */
		mlt = g_strconcat(table, "_i", NULL);
		
		/* Get id from table_i table , backwards compatible */
		res = mgd_ungrouped_select(object->mgd, "id", mlt, "sid=$d", NULL, id);
		
		tmpstr = g_string_free(dsql, FALSE);
		rv = mgd_exec(object->mgd, tmpstr);
		g_free(tmpstr);
		
		if (res) {
			while (mgd_fetch(res)) {
				id = atol(mgd_colvalue(res,0));
				DELETE_REPLIGARD(object->mgd, mlt, id);
			}
			mgd_release(res);
		}		
		g_free(mlt);
	}
		
	if (rv == 0) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
		return FALSE; 
	}

	g_signal_emit(object, MIDGARD_OBJECT_GET_CLASS(object)->signal_action_purged, 0);

	__dbus_send(object, "purge");

	return TRUE;  
}

gint midgard_object_get(MgdObject *object)
{
    GParamSpec **props;
    guint propn, i;
    GValue pval = {0,};
    const gchar *pcchar, *nick, *table;
    guint puint;
    gint pint;
    gboolean pbool;
    gfloat pfloat;
    GString *where, *sql;
    midgard_res *res;

    MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);
    
    if (object->data == NULL) {
        MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
        return -1;
    }

    table = midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(object));            
    if(table == NULL) {
        /* Object has no storage defined. Return NULL as there is nothing to select */
        g_warning("Object '%s' has no table or storage defined!", G_OBJECT_TYPE_NAME(object));
        MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OBJECT_NO_STORAGE);
        return -1;
    }
    
    if((props = g_object_class_list_properties(
                    (GObjectClass *)object->klass, 
                    &propn)) != NULL) {

        where = g_string_new(" ");
        
        for (i = 0; i < propn; i++) {
            
            g_value_init(&pval,props[i]->value_type);
            nick = g_param_spec_get_nick(props[i]);

	    if(g_str_equal(nick, "")) {
		    g_value_unset(&pval);
		    continue;
	    }

            g_object_get_property((GObject*)object, (gchar*)props[i]->name, &pval);

            switch (props[i]->value_type) {

                case G_TYPE_STRING:

                    if ((pcchar = g_strdup(g_value_get_string(&pval))) != NULL ) {
                        if (strlen(pcchar) > 0) {
                            g_string_append_printf(where, "%s='%s' AND ", nick, pcchar);
                        }
                    }                 
                    break;

                case G_TYPE_INT:

                    if((pint = g_value_get_int(&pval))){ 
                        g_string_append_printf(where, "%s=%d AND ", nick, pint);
                    }
                    break;

                case G_TYPE_UINT:

                    if((puint = g_value_get_uint(&pval))){ 
                        g_string_append_printf(where, "%s=%d AND ", nick, puint);
                    }
                    break;

                case G_TYPE_FLOAT:

                    if ((pfloat = g_value_get_float(&pval))){
                        g_string_append_printf(where, "%s=%f AND ", nick, pfloat);
                    }
                    break;

                case G_TYPE_BOOLEAN:

                    if ((pbool = g_value_get_boolean(&pval))){
                        g_string_append_printf(where, "%s=%d AND ", nick, pbool);
                    }
                    break;
            }
            g_value_unset(&pval);
        }

        sql = g_string_new(" SELECT ");

        if (object->data->use_lang == 1) {

            g_string_append_printf(sql," %s.id FROM %s WHERE %s \
                    %s.sitegroup IN (%d, 0) AND %s.id=%s_i.sid LIMIT 0,1",
                    table,
                    object->data->query->tables,
                    g_string_free(where, FALSE),
                    table,
                    mgd_sitegroup(object->mgd),
                    table,
                    table);
        } else {

            g_string_append_printf(sql," %s.id FROM %s WHERE %s \
                    %s.sitegroup IN (%d, 0) LIMIT 0,1",
                    table,
                    object->data->query->tables,
                    g_string_free(where, FALSE),
                    table,
                    mgd_sitegroup(object->mgd));
        }
            
        res = mgd_query(object->mgd, g_string_free(sql, FALSE));
        
        if (!res || !mgd_fetch(res)) {
            if (res) mgd_release(res);
            MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
            return -1;
        }
         
        if (!midgard_object_get_by_id(object, atoi(mgd_colvalue(res, 0))))
            return -1;

        return 0;

    }

    return -1;
}

/* List all "child" objects of the same type which have
 * up set to object identifier */
GObject **midgard_object_list(MgdObject *object, MidgardTypeHolder *holder)
{
	g_assert(object != NULL);
	GParamSpec *fprop, *uprop;

	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);	
	const gchar *primary_prop = 
		midgard_object_class_get_primary_property(klass);

	if(midgard_object_class_get_up_property(klass) == NULL) 
		return NULL;
	
	if ((fprop = g_object_class_find_property(
					G_OBJECT_GET_CLASS(G_OBJECT(object)),
					primary_prop)) != NULL){
		
		if((uprop = g_object_class_find_property(
						G_OBJECT_GET_CLASS(G_OBJECT(object)),
						midgard_object_class_get_up_property(klass))) == NULL ) {
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
			return NULL;
		}

		MidgardQueryBuilder *builder =
			midgard_query_builder_new(object->mgd, 
					G_OBJECT_TYPE_NAME(object));

		if(!builder) {
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
			return NULL;
		}

		GValue pval = {0,};
		g_value_init(&pval,fprop->value_type);
		g_object_get_property(G_OBJECT(object), primary_prop, &pval);
		
		midgard_query_builder_add_constraint(builder,
				midgard_object_class_get_up_property(klass), "=", &pval);
		
		g_value_unset(&pval);
		GObject **objects = midgard_query_builder_execute(builder, holder);
		g_object_unref(builder);
				
		return objects;
	}
	return NULL;
}


/* List all child objects */
GObject **midgard_object_list_children(MgdObject *object, 
		const gchar *childcname, MidgardTypeHolder *holder)
{
	GParamSpec *fprop ;
	const gchar *primary_prop = object->data->primary;

	MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_OK);

	GSList *childs = NULL;
	childs = object->data->childs;

	if ((childcname == NULL) || (object->data->childs == NULL)) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);    
		return NULL;
	}
	
	if(!g_slist_find(object->data->childs, (gpointer)g_type_from_name(childcname))) {
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				"Child type (%s) is not a child type of (%s)", 
				childcname, object->cname);
		return NULL;
	}
	
	MgdObject *child = midgard_object_new(object->mgd, childcname, NULL);
	MidgardObjectClass *child_klass = MIDGARD_OBJECT_GET_CLASS(child);

	if(midgard_object_class_get_parent_property(child_klass) == NULL) {
		g_object_unref(child);
		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_NOT_EXISTS);
		return NULL;
	}
	
	if ((fprop = g_object_class_find_property(
					(GObjectClass *) object->klass,
					primary_prop)) != NULL){
		
		MidgardQueryBuilder *builder =
			midgard_query_builder_new(object->mgd, child->cname);
		
		if(!builder) {
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, 
					"Invalid query builder configuration (%s)",
					object->cname);
			return NULL;
		}
		
		GValue pval = {0,};
		g_value_init(&pval,fprop->value_type);
		g_object_get_property(G_OBJECT(object), primary_prop, &pval);
		
		if (midgard_query_builder_add_constraint(builder, 
					midgard_object_class_get_parent_property(child_klass)
					, "=", &pval)) {

			g_value_unset(&pval);
			GObject **objects = midgard_query_builder_execute(builder, holder);
			g_object_unref(builder);
						
			return objects;
			
		} else {
			
			MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_INTERNAL);
			g_value_unset(&pval);         
			return NULL;                        
		}                            
	}
	return NULL;
}

#define _GET_TYPE_ATTR(__klass) \
	MgdSchemaTypeAttr *type_attr = \
	_midgard_core_class_get_type_attr(MIDGARD_DBOBJECT_CLASS(__klass))

static void __get_parentclass_name(const gchar *classname, GList **list)
{
	MidgardObjectClass *klass =
		MIDGARD_OBJECT_GET_CLASS_BY_NAME(classname);
	
	MgdSchemaTypeAttr *type_attr = klass->dbpriv->storage_data;
	
	const gchar *pname = type_attr->parent;
	
	if(pname != NULL) {
		
		*list = g_list_prepend(*list, (gpointer) pname);
		__get_parentclass_name(pname, list);
	}

	//*list = g_list_prepend(*list, NULL);
	
	return;
}

static gboolean __get_id_from_path_element(MidgardConnection *mgd,
		const gchar *table, const gchar *name, const gchar *field, 
		guint val, guint sitegroup, guint *rval)
{
	GString *cmd = g_string_new("SELECT id FROM ");
	g_string_append_printf(cmd, 
			"%s WHERE name = '%s' AND %s = %d "
			"AND sitegroup IN (0, %d) AND metadata_deleted = 0 ",
			table, name, field, val, sitegroup);

	midgard_res *res = mgd_vquery(mgd->mgd, cmd->str, NULL);
	
	g_string_free(cmd, TRUE);

	if (!res || !mgd_fetch(res)) {
		
		if(res) mgd_release(res);
		return FALSE;
	}
	
	*rval = (guint)atoi(mgd_colvalue(res, 0));	
	mgd_release(res);

	return TRUE;
}

MgdObject *midgard_object_get_by_path(midgard *mgd, const gchar *classname , const gchar *object_path)
{
	g_assert(mgd != NULL);
	g_assert(classname != NULL);
	g_assert(object_path != NULL);

	MIDGARD_ERRNO_SET(mgd, MGD_ERR_OK);

	gchar *path = NULL;

	if(g_str_has_prefix(object_path, "//")){
		g_warning("Empty element in object's path '%s'", object_path);
	}    
	    
	if(g_str_has_prefix(object_path, "/")){
		
		path = g_strdup(object_path); 
	
	} else  {
		
		path = g_strconcat("/", object_path, NULL);
	}

	/* Add classname to list */
	GList *plist = NULL;
	plist = g_list_prepend(plist, (gpointer) classname);

	/* "Walk" tree till NULL parent found */
	__get_parentclass_name(classname, &plist);

	gchar **pelts = g_strsplit(path, "/", 0);
	g_free(path);
	
	guint i = 1;	
	const gchar *tablename;
	gchar *_cname;
	MidgardObjectClass *klass;
	const gchar *property_up, *property_parent;
	guint oid, _oid;
	guint sitegroup = mgd_sitegroup(mgd);
	gboolean id_exists = FALSE;
	
	do {
		if(i == 1)
			oid = 0;
		else 
			oid = _oid;

		if(plist->data == NULL) {			
			g_strfreev(pelts);
			g_list_free(plist);
			MIDGARD_ERRNO_SET(mgd, MGD_ERR_NOT_EXISTS);
			return NULL;
		}

		_cname = (gchar *)plist->data;	
		klass = MIDGARD_OBJECT_GET_CLASS_BY_NAME(_cname);
		tablename = midgard_object_class_get_table(MIDGARD_OBJECT_CLASS(klass));
		property_parent = midgard_object_class_get_parent_property(klass);
		property_up = midgard_object_class_get_up_property(klass);
		
		if(!property_parent && !property_up) {
			
			g_strfreev(pelts);
			g_list_free(plist);
			g_warning("%s doesn't support tree functionality", _cname);
			MIDGARD_ERRNO_SET(mgd, MGD_ERR_INTERNAL);
			return NULL;
		}

		GParamSpec *namespec, *idspec;
		namespec = g_object_class_find_property(G_OBJECT_CLASS(klass), "name");
		idspec = g_object_class_find_property(G_OBJECT_CLASS(klass), "id");

		if(!namespec || !idspec) {
			
			 g_strfreev(pelts);
			 g_list_free(plist);
			 g_warning("%s class has no 'name' or 'id' member registered", _cname);
			 MIDGARD_ERRNO_SET(mgd, MGD_ERR_INTERNAL);
			 return NULL;
		}

		if(property_parent) {
			
			id_exists = __get_id_from_path_element(mgd->_mgd,
					tablename, (const gchar *)pelts[i], 
					property_parent, oid, sitegroup, &_oid);
		
			if(id_exists)
				goto _GET_NEXT_PATH_ELEMENT;
		}

		/* Try to get object using up property */
		if(property_up) {
			
			id_exists = __get_id_from_path_element(mgd->_mgd,
					tablename, (const gchar *)pelts[i],
					property_up, oid, sitegroup, &_oid);
		
			if(id_exists > 0)
				goto _GET_NEXT_PATH_ELEMENT;
		}

		if(!id_exists) {
			
			if(!plist->next)
				plist->data = NULL;
			else
				plist = plist->next;
			continue;
		}

_GET_NEXT_PATH_ELEMENT:

		i++;

		if(pelts[i] == NULL) {
			
			GValue idval = {0, };
			g_value_init(&idval, G_TYPE_UINT);
			g_value_set_uint(&idval, _oid);

			MgdObject *object = midgard_object_new(mgd, _cname, &idval);
			
			g_value_unset(&idval);
			g_list_free(plist);
			g_strfreev(pelts);

			return object;
		}

	} while(pelts[i] != NULL);

	return NULL;
}

GObject **midgard_object_get_languages(MgdObject *self, MidgardTypeHolder *holder)
{
	g_assert(self != NULL);

	gboolean free_holder = FALSE;

	if(!holder) {
		free_holder = TRUE;
		holder = g_new(MidgardTypeHolder, 1);
	}

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(self);
	if(!midgard_object_class_is_multilang(klass)){
		midgard_set_error(self->mgd->_mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_USER_DATA,
				"Class %s is not multilang! ", 
				G_OBJECT_TYPE_NAME(self));
		g_warning("%s", self->mgd->_mgd->err->message);
		g_clear_error(&self->mgd->_mgd->err);
		return NULL;
	}

	GValue *pval = g_new0(GValue, 1);
	g_value_init(pval, G_TYPE_STRING);	
	/* GValue is owned by collector now, do not free it */
	g_object_get_property(G_OBJECT(self), "guid", pval);
	const gchar *guid = g_value_get_string(pval);
	
	if(!guid){
		midgard_set_error(self->mgd->_mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_PROPERTY_VALUE,
				"Guid property is NULL. ");
		g_warning("%s", self->mgd->_mgd->err->message);
		g_clear_error(&self->mgd->_mgd->err);
		return NULL;
	}
	MidgardCollector *mc = midgard_collector_new(
			self->mgd->_mgd, G_OBJECT_TYPE_NAME(self), 
			"guid", pval);
	if(!mc)
		return NULL;

	midgard_collector_set_key_property(mc, "lang", NULL);
	midgard_collector_unset_languages(mc);
	
	if(!midgard_collector_execute(mc))
		return NULL;
	
	guint i = 0;
	gchar **keys = midgard_collector_list_keys(mc);
	
	if(!keys)
		return NULL;
	
	/* Compute returned **GObjects size */
	while(keys[i]){
		i++;
	}

	holder->elements = i;
	/* Return them */
	MgdObject **objects = g_new(MgdObject *, i+1);
	MgdObject *tmpobj = NULL;
	i = 0;
	while(keys[i]){
	
		/* We could fetch objects with QB (lang IN (x,y,z)), but we need to get also special lang0 object */
		if(atoi(keys[i]) == 0) {
			
			/* Ensure we get language 0 */
			tmpobj = midgard_object_new(self->mgd, "midgard_language", NULL);

		} else {
			
			GValue lang_id = {0, };
			g_value_init(&lang_id, G_TYPE_UINT);
			g_value_set_uint(&lang_id, atoi(keys[i]));
			tmpobj = midgard_object_new(self->mgd, "midgard_language", &lang_id);
			g_value_unset(&lang_id);
		}

		if(tmpobj){

			objects[i] = tmpobj;

		} else {
			
			g_warning("No language object with ID:%d", atoi(keys[i]));
			objects[i] = NULL;
		}	

		i++;
	}
	objects[i] = NULL; /* terminate by NULL */

	if(free_holder) g_free(holder);
	g_object_unref(mc);

	return (GObject**) objects;
}

const gchar *midgard_object_parent(MgdObject *object)
{
	g_assert(object != NULL);
	
	if (object->data->parent)
		return object->data->parent;
	
	return NULL;    
}

gboolean midgard_object_undelete(MidgardConnection *mgd, const gchar *guid)
{
	return midgard_object_class_undelete(mgd, guid);
}

gchar *midgard_object_export(MgdObject *self)
{
	g_assert(self);
	
	MIDGARD_ERRNO_SET(self->mgd, MGD_ERR_OK);

	gchar *guid;
	g_object_get(G_OBJECT(self), "guid", &guid, NULL);

	if(!guid){
		midgard_set_error(self->mgd->_mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_PROPERTY_VALUE,
				"Empty guid value! ");
		g_warning("%s", self->mgd->_mgd->errstr);
		g_clear_error(&self->mgd->_mgd->err);

		return NULL;
	}

	/* FIXME , remove this select when PHP language bindings use 1:1
	 * for zend and gobject objects , REMOVE */
	/*
	midgard_res *res;
	MidgardObjectClass *klass = 
		MIDGARD_OBJECT_GET_CLASS(self);
	
	GString *sql = g_string_new("SELECT ");
	g_string_append_printf(sql,
			"NULLIF(metadata_exported,'0000-00-00 00:00:00') "
			"AS metadata_exported FROM %s "
			"WHERE guid = '%s' ",
			midgard_object_class_get_table(klass),
			guid);

	if(!mgd_isroot(self->mgd)){
		g_string_append_printf(sql,
				" AND sitegroup = %d",
				mgd_sitegroup(self->mgd));
	}
	
	gchar *query = g_string_free(sql, FALSE);
	res = mgd_query(self->mgd, query);
	g_free(query);
	
	if (!res || !mgd_fetch(res)) {
		if(res) mgd_release(res);
		MIDGARD_ERRNO_SET(self->mgd, MGD_ERR_NOT_EXISTS);
		return NULL;
	}	
	self->private->exported = g_strdup((gchar *)mgd_colvalue(res, 0));
	mgd_release(res);
	*/
	/* REMOVE end*/
	
	g_free(guid);	
	
	/* Update object */
	if(_midgard_object_update(self, OBJECT_UPDATE_EXPORTED))
		return _midgard_core_object_to_xml(G_OBJECT(self));

	return NULL;
}

gboolean midgard_object_approve(MgdObject *self)
{
	g_assert(self != NULL);
	g_object_ref(self);

	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_approve, 0);

	MidgardConnection *mgd = MGD_OBJECT_CNC(self);
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	MidgardUser *user = MIDGARD_USER(mgd->priv->user);
	MgdObject *person = MIDGARD_OBJECT(mgd->mgd->person);

	if(!user) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_ACCESS_DENIED);
		g_object_unref(self);
		return FALSE;
	}

	if(!person) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		g_object_unref(self);
		return FALSE;
	}

	/* approved time value */
	GValue tval = midgard_timestamp_current_as_value(G_TYPE_STRING);

	/* approver guid value */
	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, MGD_OBJECT_GUID(person));

	/* approved toggle */
	GValue aval = {0, };
	g_value_init(&aval, G_TYPE_BOOLEAN);
	g_value_set_boolean(&aval, TRUE);

	/* increment revision */
	GValue rval = {0, };
	g_value_init(&rval, G_TYPE_UINT);
	g_value_set_uint(&rval, self->metadata->private->revision+1);

	if(midgard_object_is_approved(self)) {

		g_warning("Object is already approved");
		return FALSE;
	}

	gboolean rv = _midgard_core_query_update_object_fields(
			MIDGARD_OBJECT(self), 
			"metadata_approved", &tval,
			"metadata_approver", &gval, 
			"metadata_isapproved", &aval, 
			"metadata_revisor", &gval,
			"metadata_revised", &tval,
			"metadata_revision", &rval,
			NULL);

	/* Set object properties if query succeeded */
	if(rv) {	
		self->metadata->private->approve_is_set = TRUE;
		self->metadata->private->is_approved = TRUE;

		if(self->metadata->private->approved)
			g_free(self->metadata->private->approved);
		self->metadata->private->approved = g_value_dup_string(&tval);
		
		if(self->metadata->private->approver)
			g_free(self->metadata->private->approver);
		self->metadata->private->approver = g_value_dup_string(&gval);

		if(self->metadata->private->revisor)
			g_free(self->metadata->private->revisor);
		self->metadata->private->revisor = g_value_dup_string(&gval);

		if(self->metadata->private->revised)
			g_free(self->metadata->private->revised);
		self->metadata->private->revised = g_value_dup_string(&tval);
	
		self->metadata->private->revision = g_value_get_uint(&rval);
	}

	g_value_unset(&tval);
	g_value_unset(&gval);
	g_value_unset(&aval);
	g_value_unset(&rval);

	g_object_unref(self);
	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_approved, 0);

	return rv;
}

gboolean midgard_object_is_approved(MgdObject *self)
{
	g_assert(self != NULL);

	g_object_ref(self);
	
	MidgardConnection *mgd = MGD_OBJECT_CNC(self);
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	if(self->metadata->private->approve_is_set)
		return self->metadata->private->is_approved;

	GString *query = g_string_new("SELECT metadata_isapproved FROM");
	
	g_string_append_printf(query, " %s WHERE guid = '%s' AND sitegroup = %d ",
			midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(self)),
			MGD_OBJECT_GUID(self), MGD_OBJECT_SG(self));

	midgard_res *res = mgd_query(mgd->mgd, query->str, NULL);

	g_string_free(query, TRUE);

	if(!res || !mgd_fetch(res)) {

		if(res)
			mgd_release(res);

		g_object_unref(self);
		return FALSE;
	}

	gboolean av = FALSE;
	
	guint isap = mgd_sql2int(res, 0);
	if(isap == 1) av = TRUE;

	mgd_release(res);

	self->metadata->private->approve_is_set = TRUE;
	self->metadata->private->is_approved = av;

	g_object_unref(self);

	return av;
}

gboolean midgard_object_unapprove(MgdObject *self)
{
	g_assert(self != NULL);

	g_object_ref(self);
	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_unapprove, 0);

	MidgardConnection *mgd = MGD_OBJECT_CNC(self);
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	MidgardUser *user = MIDGARD_USER(mgd->priv->user);
	MgdObject *person = MIDGARD_OBJECT(mgd->mgd->person);

	if(!user) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_ACCESS_DENIED);
		g_object_unref(self);
		return FALSE;
	}

	if(!person) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		g_object_unref(self);
		return FALSE;
	}						         

	/* approved time value */
	GValue tval = midgard_timestamp_current_as_value(G_TYPE_STRING);

	/* approver guid value */
	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, MGD_OBJECT_GUID(person));

	/* approved toggle */
	GValue aval = {0, };
	g_value_init(&aval, G_TYPE_BOOLEAN);
	g_value_set_boolean(&aval, FALSE);

	/* increment revision */
	GValue rval = {0, };
	g_value_init(&rval, G_TYPE_UINT);
	g_value_set_uint(&rval, self->metadata->private->revision+1);

	if(!midgard_object_is_approved(self)) {

		g_warning("Object is not approved");
		g_object_unref(self);
		return FALSE;
	}

	/* FIXME & TODO , I think we need revisor, revised and revision updates too */
	gboolean rv = _midgard_core_query_update_object_fields(
			MIDGARD_OBJECT(self), 
			"metadata_approved", &tval,
			"metadata_approver", &gval, 
			"metadata_isapproved", &aval, 
			"metadata_revisor", &gval, 
			"metadata_revised", &tval, 
			"metadata_revision", &rval, 
			NULL);

	/* Set object properties if query succeeded */
	if(rv) {
		
		self->metadata->private->approve_is_set = TRUE;
		self->metadata->private->is_approved = FALSE;

		if(self->metadata->private->approved)
			g_free(self->metadata->private->approved);
		self->metadata->private->approved = g_value_dup_string(&tval);
		
		if(self->metadata->private->approver)
			g_free(self->metadata->private->approver);
		self->metadata->private->approver = g_value_dup_string(&gval);

		if(self->metadata->private->revisor)
			g_free(self->metadata->private->revisor);
		self->metadata->private->revisor = g_value_dup_string(&gval);

		if(self->metadata->private->revised)
			g_free(self->metadata->private->revised);
		self->metadata->private->revised = g_value_dup_string(&tval);
		
		self->metadata->private->revision = g_value_get_uint(&rval);
	}

	g_value_unset(&tval);
	g_value_unset(&gval);
	g_value_unset(&aval);
	g_value_unset(&rval);

	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_unapproved, 0);
	g_object_unref(self);

	return rv;
}

gboolean midgard_object_lock(MgdObject *self)
{
	g_assert(self != NULL);

	g_object_ref(self);
	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_lock, 0);

	MidgardConnection *mgd = MGD_OBJECT_CNC(self);
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	MidgardUser *user = MIDGARD_USER(mgd->priv->user);
	MgdObject *person = MIDGARD_OBJECT(mgd->mgd->person);

	if(!user) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_ACCESS_DENIED);
		g_object_unref(self);
		return FALSE;
	}

	if(!person) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		g_object_unref(self);
		return FALSE;
	}

	/* approved time value */
	GValue tval = midgard_timestamp_current_as_value(G_TYPE_STRING);

	/* approver guid value */
	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, MGD_OBJECT_GUID(person));

	/* approved toggle */
	GValue aval = {0, };
	g_value_init(&aval, G_TYPE_BOOLEAN);
	g_value_set_boolean(&aval, TRUE);

	/* increment revision */
	GValue rval = {0, };
	g_value_init(&rval, G_TYPE_UINT);
	g_value_set_uint(&rval, self->metadata->private->revision+1);

	if(midgard_object_is_locked(self)) {

		g_warning("Object is already locked");
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OBJECT_IS_LOCKED);
		g_object_unref(self);
		return FALSE;
	}
	
	gboolean rv = _midgard_core_query_update_object_fields(
			MIDGARD_OBJECT(self), 
			"metadata_locked", &tval,
			"metadata_locker", &gval, 
			"metadata_islocked", &aval, 
			"metadata_revisor", &gval, 
			"metadata_revised", &tval, 
			"metadata_revision", &rval,
			NULL);

	/* Set object properties if query succeeded */
	if(rv) {
		
		self->metadata->private->lock_is_set = TRUE;
		self->metadata->private->is_locked = TRUE;
		
		if(self->metadata->private->locked)
			g_free(self->metadata->private->locked);
		self->metadata->private->locked = g_value_dup_string(&tval);
		
		if(self->metadata->private->locker)
			g_free(self->metadata->private->locker);
		self->metadata->private->locker = g_value_dup_string(&gval);
		
		if(self->metadata->private->revisor)
			g_free(self->metadata->private->revisor);
		self->metadata->private->revisor = g_value_dup_string(&gval);
				
		if(self->metadata->private->revised)
			g_free(self->metadata->private->revised);
		self->metadata->private->revised = g_value_dup_string(&tval);

		self->metadata->private->revision = g_value_get_uint(&rval);
	}

	g_value_unset(&tval);
	g_value_unset(&gval);
	g_value_unset(&aval);
	g_value_unset(&rval);

	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_locked, 0);
	g_object_unref(self);

	return rv;
}

gboolean midgard_object_is_locked(MgdObject *self)
{
	g_assert(self != NULL);

	g_object_ref(self);
	
	MidgardConnection *mgd = MGD_OBJECT_CNC(self);
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	if(self->metadata->private->lock_is_set)
		return self->metadata->private->is_locked;

	GString *query = g_string_new("SELECT metadata_islocked FROM");
	
	g_string_append_printf(query, " %s WHERE guid = '%s' AND sitegroup = %d ",
			midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(self)),
			MGD_OBJECT_GUID(self), MGD_OBJECT_SG(self));
	
	midgard_res *res = mgd_query(mgd->mgd, query->str, NULL);
	
	g_string_free(query, TRUE);
	
	if(!res || !mgd_fetch(res)) {
		
		if(res)
			mgd_release(res);
		
		g_object_unref(self);
		return FALSE;
	}

	gboolean av = FALSE;
	
	guint isap = mgd_sql2int(res, 0);
	if(isap == 1) av = TRUE;

	self->metadata->private->lock_is_set = TRUE;
	self->metadata->private->is_locked = av;

	g_object_unref(self);

	return av;

}

gboolean midgard_object_unlock(MgdObject *self)
{
	g_assert(self != NULL);

	g_object_ref(self);
	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_unlock, 0);

	MidgardConnection *mgd = MGD_OBJECT_CNC(self);
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	MidgardUser *user = MIDGARD_USER(mgd->priv->user);
	MgdObject *person = MIDGARD_OBJECT(mgd->mgd->person);

	if(!user) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_ACCESS_DENIED);
		g_object_unref(self);
		return FALSE;
	}

	if(!person) {
		
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		g_object_unref(self);
		return FALSE;
	}

	/* approved time value */
	GValue tval = midgard_timestamp_current_as_value(G_TYPE_STRING);

	/* approver guid value */
	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, MGD_OBJECT_GUID(person));

	/* lock toggle */
	GValue lval = {0, };
	g_value_init(&lval, G_TYPE_BOOLEAN);
	g_value_set_boolean(&lval, FALSE);

	/* increment revision */
	GValue rval = {0, };
	g_value_init(&rval, G_TYPE_UINT);
	g_value_set_uint(&rval, self->metadata->private->revision+1);

	if(!midgard_object_is_locked(self)) {

		g_warning("Object is not locked");
		g_object_unref(self);
		return FALSE;
	}

	gboolean rv = _midgard_core_query_update_object_fields(
			MIDGARD_OBJECT(self), 
			"metadata_locker", &gval,
			"metadata_locked", &tval, 
			"metadata_islocked", &lval, 
			"metadata_revisor", &gval, 
			"metadata_revised", &tval, 
			"metadata_revision", &rval,
			NULL);

	/* Set object properties if query succeeded */
	if(rv) {
		
		self->metadata->private->lock_is_set = TRUE;
		self->metadata->private->is_locked = FALSE;
		
		if(self->metadata->private->locked)
			g_free(self->metadata->private->locked);
		self->metadata->private->locked = g_value_dup_string(&tval);
		
		if(self->metadata->private->locker)
			g_free(self->metadata->private->locker);
		self->metadata->private->locker = g_value_dup_string(&gval);

		if(self->metadata->private->revisor)
			g_free(self->metadata->private->revisor);
		self->metadata->private->revisor = g_value_dup_string(&gval);

		if(self->metadata->private->revised)
			g_free(self->metadata->private->revised);
		self->metadata->private->revised = g_value_dup_string(&tval);
		
		self->metadata->private->revision = g_value_get_uint(&rval);
	}

	g_value_unset(&tval);
	g_value_unset(&gval);
	g_value_unset(&lval);
	g_value_unset(&rval);

	g_signal_emit(self, MIDGARD_OBJECT_GET_CLASS(self)->signal_action_unlocked, 0);
	g_object_unref(self);

	return rv;
}
