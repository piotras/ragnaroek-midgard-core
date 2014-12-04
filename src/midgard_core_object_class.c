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

#include "midgard_core_object.h"
#include "midgard_core_object_class.h"
#include "midgard/midgard_error.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "midgard/midgard_reflection_property.h"
#include "midgard/midgard_dbobject.h"
#include "midgard/midgard_metadata.h"
#include "midgard/midgard_object.h"

#define get_varchar_size(str) \
	        if(str == NULL) \
                size++; \
        else \
                size = strlen((const gchar *)str) + 1 + size; \
        g_free(str);

#define get_datetime_size(str) \
	        size = 8 + size; \
        g_free(str);


gboolean _midgard_core_object_prop_type_is_valid(GType src_type, GType dst_type)
{
	if(src_type == dst_type)
		return TRUE;

	return FALSE;
}

gboolean _midgard_core_object_prop_link_is_valid(GType ltype)
{
	if(ltype == MGD_TYPE_UINT) {

		return TRUE;

	} else if(ltype == MGD_TYPE_GUID) {

		return TRUE;

	} else if(ltype == MGD_TYPE_STRING) {

		return TRUE;

	} else {

		g_warning("Invalid property type. Expected uint, guid or string");
		return FALSE;
	}
}

gboolean _midgard_core_object_prop_parent_is_valid(GType ptype)
{
	return  _midgard_core_object_prop_link_is_valid(ptype);
}

gboolean _midgard_core_object_prop_up_is_valid(GType ptype)
{
	return  _midgard_core_object_prop_link_is_valid(ptype);
}

GType _midgard_core_object_get_property_parent_type(MidgardObjectClass *klass)
{
	g_assert(klass != NULL);

	const gchar *prop = midgard_object_class_get_property_parent(klass);

	if(prop == NULL) {

		g_warning("Failed to get parent property");
		return 0;
	}

	GParamSpec *pspec = g_object_class_find_property(G_OBJECT_CLASS(klass), prop);

	if(pspec == NULL) {

		g_error("Failed to find GParamSpec for parent '%s' property", prop);
		return 0;
	}

	if(!_midgard_core_object_prop_parent_is_valid(pspec->value_type))
		return 0;

	return pspec->value_type;
}

GType _midgard_core_object_get_property_up_type(MidgardObjectClass *klass)
{
	g_assert(klass != NULL);

	const gchar *prop = midgard_object_class_get_property_up(klass);

	if(prop == NULL) {

		g_warning("Failed to get up property");
		return 0;
	}

	GParamSpec *pspec = g_object_class_find_property(G_OBJECT_CLASS(klass), prop);

	if(pspec == NULL) {

		g_error("Failed to find GParamSpec for up '%s' property", prop);
		return 0;
	}

	if(!_midgard_core_object_prop_parent_is_valid(pspec->value_type))
		return 0;

	return pspec->value_type;
}

gboolean _midgard_core_object_prop_parent_is_set(MgdObject *object)
{
	g_assert(object != NULL);

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	GType ptype = _midgard_core_object_get_property_parent_type(klass);

	if(ptype == 0) 
		return FALSE;

	GValue gval = {0, };
	g_value_init(&gval, ptype);
	const gchar *prop = midgard_object_class_get_property_parent(klass);

	g_object_get_property(G_OBJECT(object), prop, &gval);

	if(ptype == G_TYPE_UINT) {
		
		guint i = g_value_get_uint(&gval);
		g_value_unset(&gval);
		if(i == 0) 
			return FALSE;

		return TRUE;
	
	} else if(ptype == G_TYPE_STRING) {

		const gchar *s = g_value_get_string(&gval);
		if(s == NULL || *s == '\0') {
			g_value_unset(&gval);
			return FALSE;
		}

		g_value_unset(&gval);
		return TRUE;
	}

	return FALSE;
}

gboolean _midgard_core_object_prop_up_is_set(MgdObject *object)
{
	g_assert(object != NULL);

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	GType ptype = _midgard_core_object_get_property_up_type(klass);

	if(ptype == 0) 
		return FALSE;

	GValue gval = {0, };
	g_value_init(&gval, ptype);
	const gchar *prop = midgard_object_class_get_property_up(klass);

	g_object_get_property(G_OBJECT(object), prop, &gval);

	if(ptype == G_TYPE_UINT) {
		
		guint i = g_value_get_uint(&gval);
		if(i == 0)
			return FALSE;

		return TRUE;
	
	} else if(ptype == G_TYPE_STRING) {

		const gchar *s = g_value_get_string(&gval);
		if(s == NULL || *s == '\0')
			return FALSE;

		return TRUE;
	}

	return FALSE;
}

gboolean _midgard_core_object_is_valid(MgdObject *object)
{
	guint propn, i, size = 0;
	GType prop_type;
	gchar *prop_str;
	const gchar *blobdir;
	gchar *blobpath, *location = NULL;
	struct stat statbuf; /* GLib 2.6 is not available everywhere, g_stat not used */
	const gchar *typename = G_OBJECT_TYPE_NAME(object);
	GParamSpec **props = g_object_class_list_properties(
			G_OBJECT_GET_CLASS(G_OBJECT(object)), &propn);
	if(!props){
		midgard_set_error(object->mgd->_mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_OBJECT,
				"Object %s has no properties",
				G_OBJECT_TYPE_NAME(object));
		g_warning("%s", object->mgd->_mgd->err->message);
		g_clear_error(&object->mgd->_mgd->err);
		g_free(props);
		return FALSE; 
	}
	
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);

	/* Check if object is dependent */
	const gchar *parent_prop = midgard_object_class_get_parent_property(klass);
	const gchar *up = midgard_object_class_get_up_property(klass);

	/* Make sure parent and up are integers */
	GParamSpec *parent_pspec = NULL;
	GParamSpec *up_pspec = NULL;
	if(parent_prop)	
		parent_pspec = g_object_class_find_property(G_OBJECT_CLASS(klass), parent_prop);
	if(up_pspec)
		up_pspec = g_object_class_find_property(G_OBJECT_CLASS(klass), up);
	
	/* Parent declared, up not. Parent can not be 0 */
	if(parent_pspec && !up_pspec) {
			
		if(!_midgard_core_object_prop_parent_is_set(object)) {
				
			midgard_set_error(object->mgd->_mgd,
					MGD_GENERIC_ERROR,
					MGD_ERR_INVALID_PROPERTY_VALUE,
					"(%s) parent '%s' property's value is 0. ",
					G_OBJECT_TYPE_NAME(object), parent_prop);
			g_free(props);
			return FALSE;
		}
	}

	/* Parent declared, up declared. Parent might be 0, only if up is not 0. */
	if(parent_pspec && up_pspec) {
			
		if(!_midgard_core_object_prop_parent_is_set(object)
				&& !_midgard_core_object_prop_up_is_set(object)) {
				
			midgard_set_error(object->mgd->_mgd,
					MGD_GENERIC_ERROR,
					MGD_ERR_INVALID_PROPERTY_VALUE,
					"Parent '%s' and up '%s' property's value is 0. ",
					parent_prop, up);
			g_free(props);
		}
	}

	/* Attachment, we need to get file size */
	if(g_str_equal(typename, "midgard_attachment")){
		
		/* FIXME , remove legacy blobdir get */
		blobdir = mgd_get_blobdir(object->mgd);
		
		if (!blobdir || (*blobdir != '/')
				|| (stat(blobdir, &statbuf) != 0)
				|| !S_ISDIR(statbuf.st_mode)) {
			g_free(props);
			g_error("Invalid blobdir or blobdir is not set");
		}
		g_object_get(G_OBJECT(object), "location", &location, NULL);

		/* FIXME, get blobdir location from MidgardConfig */
		if(location) {
			if(strlen(location) > 1) {
				blobpath = g_strconcat(
						blobdir,
						"/",
						location,
						NULL);
				
				if((stat(blobpath, &statbuf) == 0) || (!S_ISDIR(statbuf.st_mode)))
					size = statbuf.st_size;
				g_free(blobpath);
				g_free(location);
			}
		}
	}

	MidgardReflectionProperty *mrp = 
		midgard_reflection_property_new(klass);
	
	for(i = 0; i < propn; i++){
		prop_type = 
			midgard_reflection_property_get_midgard_type(
					mrp,
					props[i]->name);
		
		if(prop_type == MGD_TYPE_GUID) {
			g_object_get(G_OBJECT(object), props[i]->name, &prop_str, NULL);

			if(prop_str != NULL && *prop_str != '\0') {
				if(!midgard_is_guid(prop_str)){
					midgard_set_error(object->mgd->_mgd,
							MGD_GENERIC_ERROR,
							MGD_ERR_INVALID_PROPERTY_VALUE,
							"'%s' property's value is not a guid. ",
							props[i]->name);
					g_warning("%s", object->mgd->_mgd->err->message);
					g_clear_error(&object->mgd->_mgd->err);
					g_free(props);
					g_free(prop_str);
					g_object_unref(mrp);
					return FALSE;
				}
			}
			get_varchar_size(prop_str);
		
		} else if(prop_type == MGD_TYPE_LONGTEXT){
			g_object_get(G_OBJECT(object), props[i]->name,
					&prop_str, NULL);			
			if(prop_str == NULL) prop_str = g_strdup("");
			size = (strlen(prop_str)) + 4 + size;
			g_free(prop_str);
		
		} else if(prop_type == MGD_TYPE_TIMESTAMP) {
			g_object_get(G_OBJECT(object), props[i]->name,
					&prop_str, NULL);
			/* TODO, Check if timestamp value is valid */
			size =  8 + size;
			g_free(prop_str);
		
		} else if(prop_type == MGD_TYPE_STRING) {
			g_object_get(G_OBJECT(object), props[i]->name,
					&prop_str, NULL);
			get_varchar_size(prop_str);

		} else if(prop_type == MGD_TYPE_UINT 
				&& prop_type == MGD_TYPE_INT
				&& prop_type == MGD_TYPE_FLOAT
				&& prop_type == MGD_TYPE_BOOLEAN){
			size = size + 4;
		
		} else {
			/* Do nothing now */
		}
	}
	g_free(props);

	/* Object metadata */
	/* creator */
	g_object_get(G_OBJECT(object->metadata), "creator", &prop_str, NULL);
	get_varchar_size(prop_str);
	/* created */
	g_object_get(G_OBJECT(object->metadata), "created", &prop_str, NULL);
	get_datetime_size(prop_str);
	/* revisor */
	g_object_get(G_OBJECT(object->metadata), "revisor", &prop_str, NULL);
	get_varchar_size(prop_str);
	/* revised */
	g_object_get(G_OBJECT(object->metadata), "revised", &prop_str, NULL);
	get_datetime_size(prop_str);
	/* locker */
	g_object_get(G_OBJECT(object->metadata), "locker", &prop_str, NULL);
	get_varchar_size(prop_str);
	/* locked */
	g_object_get(G_OBJECT(object->metadata), "locked", &prop_str, NULL);
	get_datetime_size(prop_str);
	/* approver */
	g_object_get(G_OBJECT(object->metadata), "approved", &prop_str, NULL);
	get_varchar_size(prop_str);
	/* approved */
	g_object_get(G_OBJECT(object->metadata), "approved", &prop_str, NULL);
	get_datetime_size(prop_str);
	/* published */
	g_object_get(G_OBJECT(object->metadata), "published", &prop_str, NULL);
	get_datetime_size(prop_str);
	/* authors */
	g_object_get(G_OBJECT(object->metadata), "authors", &prop_str, NULL);
	if(prop_str == NULL)
		prop_str = g_strdup("");
	size = strlen(prop_str) + 4 + size;
	g_free(prop_str);
	
	/* owner */
	g_object_get(G_OBJECT(object->metadata), "owner", &prop_str, NULL);
	get_varchar_size(prop_str);
	/* schedule-start */
	g_object_get(G_OBJECT(object->metadata), "schedulestart", &prop_str, NULL);
	get_datetime_size(prop_str);
	/* schedule-end */
	g_object_get(G_OBJECT(object->metadata), "scheduleend", &prop_str, NULL);
	get_datetime_size(prop_str);
	/* hidden, nav-noentry , revision ( 3 x 4 ) */
	size = size + 12;
	
	object->metadata->private->size = size;

	return TRUE;
}

MgdSchemaPropertyAttr *_midgard_core_class_get_property_attr(
		GObjectClass *klass, const gchar *name)
{
	g_assert(klass != NULL);
	g_assert(name != NULL);
	
	MgdSchemaPropertyAttr *prop_attr = NULL;
	
	MidgardDBObjectClass *_klass = MIDGARD_DBOBJECT_CLASS(klass);
	prop_attr = g_hash_table_lookup(_klass->dbpriv->storage_data->prophash, name);
	
	if(prop_attr == NULL)
		return NULL;
	
	return prop_attr;
}
