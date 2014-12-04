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

#ifndef MIDGARD_REFLECTION_PROPERTY_H
#define MIDGARD_REFLECTION_PROPERTY_H

/**
 *
 * \defgroup midgard_reflection_property  MidgardReflectionProperty
 *
 * Midgard Reflection is designed for developers who need to write "schema aware" 
 * applications without any knowledge about schema registered for application.
 * Property's or type's attributes defined in schema files are easily accessible 
 * with Midgard Reflection objects. 
 */ 

#include "midgard/midgard_type.h"
#include "midgard/types.h"

/* convention macros */
#define MIDGARD_TYPE_REFLECTION_PROPERTY (midgard_reflection_property_get_type())
#define MIDGARD_REFLECTION_PROPERTY(object)  \
	(G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_REFLECTION_PROPERTY, MidgardReflectionProperty))
#define MIDGARD_REFLECTION_PROPERTY_CLASS(klass)  \
	(G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_REFLECTION_PROPERTY, MidgardReflectionPropertyClass))
#define MIDGARD_IS_REFLECTION_PROPERTY(object)   \
	(G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_REFLECTION_PROPERTY))
#define MIDGARD_IS_REFLECTION_PROPERTY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_REFLECTION_OBJECT_PROPERTY))
#define MIDGARD_REFLECTION_PROPERTY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_REFLECTION_PROPERTY, MidgardReflectionPropertyClass))

typedef struct _MidgardReflectionProperty MidgardReflectionProperty;
typedef struct _MidgardReflectionPropertyClass MidgardReflectionPropertyClass;

/**
 *
 * \ingroup midgard_reflection_property 
 *
 * The MidgardReflectionPropertyClass class structure. 
 *
 */
struct _MidgardReflectionPropertyClass{
	GObjectClass parent;

	/* public class members */
	gboolean (*is_multilang) (MidgardReflectionProperty *object, const gchar *property);
	gboolean (*is_link) (MidgardReflectionProperty *object, const gchar *property);
};

/**
 * 
 * \ingroup midgard_reflection_property
 * 
 * Returns the MidgardReflectionProperty  value type. Registers the type as
 * a fundamental GType unless already registered.
 */
extern GType midgard_reflection_property_get_type(void);

/** 
 * /ingroup midgard_reflection_property
 *
 * Creates new MidgardReflectionProperty instance for the given MidgardObjectClass.
 *
 * g_object_unref should be used to free memory used by object.
 */ 
extern MidgardReflectionProperty *midgard_reflection_property_new(
		MidgardObjectClass *klass);

/**
 * 
 * \ingroup midgard_reflection_property
 *
 * Get Midgard Type of the property.
 * 
 * \param object MidgardReflectionProperty 
 * \param[in] name property name registered for the given MidgardObjectClass
 * 
 * \return Gtype 
 * 
 * Returned Gtype is registered as Midgard type.
 *
 * MGD_TYPE_NONE is returned if property is not found as 
 * member of a class or if class is not registered within GType system.
 */
extern GType 
midgard_reflection_property_get_midgard_type(
		MidgardReflectionProperty *object, const gchar *name);

/**
 *
 * \ingroup midgard_reflection_property
 *
 * Checks if property is a link to another type.
 * 
 * \param object MidgardReflectionProperty
 * \param[in] name property name registered for the given MidgardObjectClass 
 *  
 * \return TRUE when property is a link to another type, FALSE otherwise 
 * 
 * FALSE is returned when property is not found as
 * member of a class or if class is not registered within GType system. 
 */
extern gboolean 
midgard_reflection_property_is_link(
		MidgardReflectionProperty *object, const gchar *name);

/** 
 *
 * \ingroup midgard_reflection_property
 *
 * Checks if property is linked with another type.
 *
 * \param object MidgardReflectionProperty
 * \param[in] name property name registered for the given MidgardObjectClass
 *
 * \return TRUE when property is linked to another type, FALSE otherwise
 *
 * FALSE is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 * 
 */
extern gboolean
midgard_reflection_property_is_linked(
		MidgardReflectionProperty *object, const gchar *name);

/**
 * \ingroup midgard_reflection_property
 *
 * Returns the class that the named property is linked to. 
 * Returns NULL if the named property is not a link.
 * 
 * \param object MidgardReflectionProperty
 * \param[in] name property name registered for the given MidgardObjectClass
 * 
 * \return the link target class, or \c NULL
 *
 * NULL is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 *  
 */
extern MidgardObjectClass 
*midgard_reflection_property_get_link_class(
		MidgardReflectionProperty *object, const gchar *name);

/**
 * 
 * \ingroup midgard_reflection_property
 *
 * Gets the class name of type which property is linked to.
 * 
 * \param object MidgardReflectionProperty
 * \param[in] name property name registered for the given MidgardObjectClass
 * 
 * \return class name or NULL if property is not linked with other type. 
 *
 * NULL is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 */
extern const gchar
*midgard_reflection_property_get_link_name(
		MidgardReflectionProperty *object, const gchar *name);

/**
 * 
 * \ingroup midgard_reflection_property
 *
 * Gets the name of target property.
 *
 * \param object MidgardReflectionProperty instance
 * \param[in] name property name registered for the given MidgardObjectClass
 *
 * \return target property name for which given property name is linked to.
 *
 * NULL is returned if:
 *  - property name is not member of the class
 *  - property name is not defined as link in MgdSchema
 */
extern const gchar 
*midgard_reflection_property_get_link_target(
		MidgardReflectionProperty *object, const gchar *name);

/**
 *
 * \ingroup midgard_reflection_property
 *
 * Gets description of the property.
 *
 * \param object MidgardReflectionProperty
 * \param[in] name property name registered for the given MidgardObjectClass
 * 
 * \return description of the property. 
 *
 * Description should be defined in application's xml schema file.
 * This function is a simple wrapper to g_param_spec_get_blurb.
 * 
 * NULL is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 * 
 */
extern const gchar 
*midgard_reflection_property_description(
		MidgardReflectionProperty *object, const gchar *name);

/**
 *
 * \ingroup midgard_reflection_property
 *
 * Checks if property is multilingual.
 * 
 * \param object MidgardReflectionProperty
 * \param[in] name property name registered for the given MidgardObjectClass
 * 
 * \return TRUE if property is multilingual , FALSE otherwise 
 *
 * FALSE is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 * 
 * Property's multilang attribute should be defined in xml schema file.
 */
extern gboolean
midgard_reflection_property_is_multilang(
		MidgardReflectionProperty *object, const gchar *name); 

extern const gchar*
midgard_reflection_property_get_user_value (MidgardReflectionProperty *self, const gchar *property, const gchar *name);


#endif /* MIDGARD_REFLECTION_PROPERTY_H */
