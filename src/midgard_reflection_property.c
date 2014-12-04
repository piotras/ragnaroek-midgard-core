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

#include "midgard/midgard_reflection_property.h"
#include "schema.h"
#include "midgard_core_object.h"

struct _MidgardReflectionProperty{
	        GObject parent;

		MidgardObjectClass *klass;
};

static void _midgard_reflection_property_finalize(GObject *object)
{
	g_assert(object != NULL);
	return;
}

/* Initialize class */
static void _midgard_reflection_property_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardReflectionPropertyClass *klass = MIDGARD_REFLECTION_PROPERTY_CLASS (g_class);
	
	gobject_class->set_property = NULL;
	gobject_class->get_property = NULL;
	gobject_class->finalize = _midgard_reflection_property_finalize;

	klass->is_multilang = midgard_reflection_property_is_multilang; 
}

/* Register MidgardObjectProperty type */
GType midgard_reflection_property_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardReflectionPropertyClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_reflection_property_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardReflectionProperty),
			0,              /* n_preallocs */
			NULL            /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
				"midgard_reflection_property",
				&info, 0);
	}
	return type;
}

/* Object constructor */
MidgardReflectionProperty *midgard_reflection_property_new(
		MidgardObjectClass *klass)
{
	g_assert(klass != NULL);
	MidgardReflectionProperty *self = 
		g_object_new(MIDGARD_TYPE_REFLECTION_PROPERTY, NULL);
	self->klass = klass;
	return self;
}

/* Get Midgard Type of the property. */
GType midgard_reflection_property_get_midgard_type(
		MidgardReflectionProperty *object, const gchar *property)
{
	g_assert(object != NULL);
	g_assert(property != NULL);
	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(object->klass->dbpriv->storage_data->prophash,
					property)) == NULL)
		return MGD_TYPE_NONE;

	return prop_attr->gtype;
}

/* Checks if property is a link to another type. */
gboolean midgard_reflection_property_is_link(
		MidgardReflectionProperty *object, const gchar *property)
{	
	g_assert(object != NULL);
	g_assert(property != NULL);
	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(object->klass->dbpriv->storage_data->prophash,
					property)) == NULL)
		return FALSE;
	
	return prop_attr->is_link;
}

/* Checks if property is linked with another type. */
gboolean midgard_reflection_property_is_linked(
		MidgardReflectionProperty *object, const gchar *property)
{
	g_assert(object != NULL);
	g_assert(property != NULL);
	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(object->klass->dbpriv->storage_data->prophash,
					property)) == NULL)
		return FALSE;

	return prop_attr->is_linked;
}

/* Gets the class name of type which property is linked to. */
const gchar *midgard_reflection_property_get_link_name(
		MidgardReflectionProperty *object, const gchar *property)
{
	g_assert(object != NULL);
	g_assert(property != NULL);	
	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(object->klass->dbpriv->storage_data->prophash,
					property)) == NULL)
		return NULL;
	
	return prop_attr->link;					
}

/* Gets the name of target property. */
const gchar  *midgard_reflection_property_get_link_target(
		MidgardReflectionProperty *object, const gchar *property)
{
	g_assert(object != NULL);
	g_assert(property != NULL);
	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(object->klass->dbpriv->storage_data->prophash,
					property)) == NULL)
		return NULL;

	return prop_attr->link_target;	
}


/* Gets description of the property. */
const gchar *midgard_reflection_property_description(
		MidgardReflectionProperty *object, const gchar *property)
{
	g_assert(object != NULL);
	g_assert(property != NULL);
	
	GParamSpec *prop = g_object_class_find_property(
			G_OBJECT_CLASS(object->klass), property);

	if(!prop)
		return NULL;

	return  g_param_spec_get_blurb(prop);
}

/* Checks if property is multilingual. */
gboolean midgard_reflection_property_is_multilang(
		MidgardReflectionProperty *object, const gchar *property)
{
	g_assert(object != NULL);
	g_assert(property != NULL);		
	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(object->klass->dbpriv->storage_data->prophash,
					property)) == NULL)
		return FALSE;
	
	return prop_attr->is_multilang;
}

/* Returns the class that the named property is linked to. */
extern MidgardObjectClass *midgard_reflection_property_get_link_class(
		MidgardReflectionProperty *object, const gchar *property)
{
	g_assert(object != NULL);
	g_assert(property != NULL);

	MgdSchemaPropertyAttr *prop_attr;
	if((prop_attr = g_hash_table_lookup(object->klass->dbpriv->storage_data->prophash,
					property)) == NULL)
		return NULL;
	
	if(prop_attr->is_link) {
		MidgardObjectClass *link_klass = 
			g_type_class_peek(g_type_from_name(prop_attr->link)); 
		return link_klass;
	}
	return NULL;
}

const gchar*
midgard_reflection_property_get_user_value (MidgardReflectionProperty *self, const gchar *property, const gchar *name)
{
	g_assert(self != NULL);
	g_return_val_if_fail(property != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);

	MgdSchemaPropertyAttr *prop_attr = 
		g_hash_table_lookup(self->klass->dbpriv->storage_data->prophash, property);

	if (prop_attr == NULL)
		return NULL;

	return (const gchar *) g_hash_table_lookup(prop_attr->user_fields, name);
}

