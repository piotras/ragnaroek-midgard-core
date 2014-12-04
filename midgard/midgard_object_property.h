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

#ifndef MIDGARD_OBJECT_PROPERTY_H
#define MIDGARD_OBJECT_PROPERTY_H

#include "midgard/midgard_type.h"
#include "midgard/types.h"

/* MidgardObjectProperty , reflection */

/* convention macros */
#define MIDGARD_TYPE_OBJECT_PROPERTY (midgard_object_property_get_type())
#define MIDGARD_OBJECT_PROPERTY(object)  \
	(G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_OBJECT_PROPERTY, midgard))
#define MIDGARD_OBJECT_PROPERTY_CLASS(klass)  \
	(G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_OBJECT_PROPERTY, MidgardObjectPropertyClass))
#define MIDGARD_IS_OBJECT_PROPERTY(object)   \
	(G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_OBJECT_PROPERTY))
#define MIDGARD_IS_OBJECT_PROPERTY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_OBJECT_PROPERTY))
#define MIDGARD_OBJECT_PROPERTY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_OBJECT_PROPERTY, MidgardObjectPropertyClass))

typedef struct _MidgardObjectProperty MidgardObjectProperty;
typedef struct _MidgardObjectPropertyClass MidgardObjectPropertyClass;

/**
 *
 * \defgroup objectproperty  Midgard Object Property
 *
 * The MidgardObjectPropertyClass class structure . 
 *
 */
struct _MidgardObjectPropertyClass{
	GObjectClass parent;

	/* public class members */
	gboolean (*is_multilang) (const gchar *classname, const gchar *property);
	gboolean (*is_link) (const gchar *classname, const gchar *property);
};

/**
 *
 * \ingroup objectproperty
 *
 * The MidgardObjectProperty structure.
 * 
 * */
struct _MidgardObjectProperty{
	GObject parent;
};

/**
 * 
 * \ingroup objectproperty
 * 
 * Returns the MidgardObjectProperty  value type. Registers the type as
 * a fundamental GType unless already registered.
 */
extern GType 
midgard_object_property_get_type(void);

/**
 * 
 * \ingroup objectproperty
 *
 * Get Midgard Type of the property.
 * 
 * \param classname name of the registered class
 * \param name property name registered for klass
 * 
 * \return Gtype 
 * 
 * Returned Gtype is registered as Midgard type.
 *
 * MGD_TYPE_NONE is returned if property is not found as 
 * member of a class or if class is not registered within GType system.
 */
extern GType 
midgard_object_property_get_midgard_type(const gchar *classname, const gchar *name);

/**
 *
 * \ingroup objectproperty
 *
 * Checks if property is a link to another type.
 * 
 * \param classname name of the registered class
 * \param name property name registered for klass
 *
 *  
 * \return TRUE when property is a link to another type, FALSE otherwise 
 * 
 * FALSE is returned when property is not found as
 * member of a class or if class is not registered within GType system. 
 */
extern gboolean 
midgard_object_property_is_link(const gchar *classname, const gchar *name);

/** 
 *
 * \ingroup objectproperty
 *
 * Checks if property is linked with another type.
 *
 * \param classname name of the registered class
 * \param name property name registered for klass
 *
 * \return TRUE when property is linked to another type, FALSE otherwise
 *
 * FALSE is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 * 
 */
extern gboolean
midgard_object_property_is_linked(const gchar *classname, const gchar *name);

/**
 *
 * \ingroup objectproperty
 *
 * Returns the class that the named property is linked to. 
 * Returns NULL if the named property is not a link.
 * 
 * \param klass the class containing the named link property
 * \param name  name of the link property
 * 
 * \return the link target class, or \c NULL
 *
 * NULL is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 *  
 */
extern MidgardObjectClass 
*midgard_object_property_get_link_class(MidgardObjectClass *klass, const gchar *name);

/**
 * 
 * \ingroup objectproperty
 *
 * Gets the class name of type which property is linked to.
 * 
 * \param classname name of the registered class
 * \param name property name registered for klass
 * 
 * \return class name or NULL if property is not linked with other type. 
 *
 * NULL is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 */
extern const gchar
*midgard_object_property_get_link_name(const gchar *classname, const gchar *name);

/**
 *
 * \ingroup objectproperty
 *
 * Gets description of the property.
 *
 * \param classname name of the registered class
 * \param name property name registered for klass
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
*midgard_object_property_description(const gchar *classname, const gchar *name);

/**
 *
 * \ingroup objectproperty
 *
 * Checks if property is multilingual.
 * 
 * \param classname name of the registered class
 * \param name property name registered for klass
 * 
 * \return TRUE if property is multilingual , FALSE otherwise 
 *
 * FALSE is returned when property is not found as
 * member of a class or if class is not registered within GType system.
 * 
 * Property's multilang attribute should be defined in xml schema file.
 */
extern gboolean
midgard_object_property_is_multilang(const gchar *classname, const gchar *name); 

#endif /* MIDGARD_OBJECT_PROPERTY_H */
