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

//#include "midgard/schema.h"
#include "midgard/midgard_object_property.h"
#include "schema.h"

#define _MGD_GET_PRIVATE_CLASS_DATA \
	g_assert(classname != NULL); \
	g_assert(property != NULL); \
        MidgardObjectClass *oc = \
		(MidgardObjectClass*) g_type_class_peek(g_type_from_name(classname)); \
        MgdSchemaTypeAttr *type_attr = oc->storage_data ;\

/* Initialize class */
static void _midgard_object_property_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardObjectPropertyClass *klass = MIDGARD_OBJECT_PROPERTY_CLASS (g_class);
	
	gobject_class->set_property = NULL;
	gobject_class->get_property = NULL;
	gobject_class->finalize = NULL;

	klass->is_multilang = midgard_object_property_is_multilang; 
}

/* Register MidgardObjectProperty type */
GType
midgard_object_property_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardObjectPropertyClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_object_property_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardObjectProperty),
			0,              /* n_preallocs */
			NULL            /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
				"MidgardObjectProperty",
				&info, 0);
	}
	return type;
}


/* Get Midgard Type of the property. */
GType 
midgard_object_property_get_midgard_type(const gchar *classname, const gchar *property){

	_MGD_GET_PRIVATE_CLASS_DATA;

	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(type_attr->prophash, property)) == NULL)
		return MGD_TYPE_NONE;

	return prop_attr->gtype;
}

/* Checks if property is a link to another type. */
gboolean 
midgard_object_property_is_link(const gchar *classname, const gchar *property){

	_MGD_GET_PRIVATE_CLASS_DATA;

	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(type_attr->prophash, property)) == NULL)
		return FALSE;
	
	return prop_attr->is_link;
}

/* Checks if property is linked with another type. */
gboolean
midgard_object_property_is_linked(const gchar *classname, const gchar *property){

	_MGD_GET_PRIVATE_CLASS_DATA;

	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(type_attr->prophash, property)) == NULL)
		return FALSE;

	return prop_attr->is_linked;
	
}

/* Gets the class name of type which property is linked to. */
const gchar
*midgard_object_property_get_link_name(const gchar *classname, const gchar *property){

	_MGD_GET_PRIVATE_CLASS_DATA;
	
	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(type_attr->prophash, property)) == NULL)
		return NULL;
	
	return prop_attr->link;					
}

/* Gets description of the property. */
const gchar 
*midgard_object_property_description(const gchar *classname, const gchar *property){

	g_assert(classname != NULL);
	g_assert(property != NULL);
	
	GObject *object = g_object_new(
			g_type_from_name(classname), NULL);
	
	GParamSpec *prop = g_object_class_find_property(
			G_OBJECT_GET_CLASS(G_OBJECT(object)), property);

	if(!prop)
		return NULL;

	return  g_param_spec_get_blurb(prop);
}

/*  Checks if property is multilingual. */
gboolean
midgard_object_property_is_multilang(const gchar *classname, const gchar *property){

	_MGD_GET_PRIVATE_CLASS_DATA;
	
	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(type_attr->prophash, property)) == NULL)
		return FALSE;
	
	return prop_attr->is_multilang;
}

/*  Returns the class that the named property is linked to. */
extern MidgardObjectClass
*midgard_object_property_get_link_class(MidgardObjectClass *klass, const gchar *property)
{
	g_assert(klass != NULL);
	g_assert(property != NULL);

	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(klass->storage_data->prophash, property)) == NULL)
		return NULL;
	
	if(prop_attr->is_link) {
		
		MidgardObjectClass *link_klass = 
			g_type_class_peek(g_type_from_name(prop_attr->link)); 
		return link_klass;
	}
	return NULL;
}
