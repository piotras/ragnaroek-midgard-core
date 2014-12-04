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
 * */

#ifndef MIDGARD_METADATA_H
#define MIDGARD_METADATA_H

/**
 * \defgroup midgard_metadata Midgard Metadata 
 *
 * Midgard Metadata Class provides unified common data for all classes 
 * registered in Midgard Schema ( MgdSchema ).
 * Midgard Metadata objects are automatically created when any  Midgard 
 * Schema class instance is created. However it is possible to  create 
 * standalone metadata object , it's not used in practice.
 * Midgard Metadata object are designed to be fully usefull with any object 
 * instance. 
 * 
 */

#include <glib-object.h>
#include "midgard/types.h"
#include "midgard/midgard_dbobject.h"

/* convention macros */
#define MIDGARD_TYPE_METADATA        (midgard_metadata_get_type ())
#define MIDGARD_METADATA(obj)        (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    MIDGARD_TYPE_METADATA, MidgardMetadata))
#define MIDGARD_METADATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                    MIDGARD_TYPE_METADATA, MidgardMetadataClass))
#define MIDGARD_IS_METADATA(obj)     (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MIDGARD_TYPE_METADATA))
#define MIDGARD_IS_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_METADATA))
#define MIDGARD_METADATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                                    MIDGARD_TYPE_METADATA, MidgardMetadataClass))

/**
 * \ingroup midgard_metadata
 * 
 * Returns the MidgardMetadata  value type. 
 * Registers the type as a fundamental GType unless already registered.
 */
GType midgard_metadata_get_type (void);

/** 
 * \ingroup midgard_metadata
 *
 * Midgard Metadata Class structure.
 *
 */
struct MidgardMetadataClass {
	GObjectClass parent;

	MidgardDBObjectPrivate *dbpriv;
	guint signal_set_created;
	guint signal_set_updated;
	
	void (*set_created) (MgdObject *self);
	void (*set_updated) (MgdObject *self);
};

/**
 * \ingroup midgard_metadata
 *
 * Private Midgard Metadata structure.
 */
typedef struct _MidgardMetadataPrivate MidgardMetadataPrivate;

/**
 * \ingroup midgard_metadata
 *
 * Midgard Metadata object structure.
 */
struct MidgardMetadata {
	GObject parent;
	
	/* private */
	MidgardDBObjectPrivate *dbpriv;
	MidgardMetadataPrivate *private;
};

/*
 * \ingroup midgard_metadata
 *
 * Creates new midgard_metadata instance for the given MgdObject instance.
 *
 * \param[in] klass MidgardObjectClass pointer
 *
 * \return newly allocated MidgardMetadata object
 *
 * Function g_object_new may be used as well as object's constructor however
 * it's recommended to use this constructor. MgdObject pointer is internally 
 * assigned as a pointer to midgard object for which particular metadata object 
 * instance was created.
 * 
 * Midgard Metadata object has two "kinds" of properties.
 * The first one is settable ( and overwritten ) only by metadata implementation.
 * The second one is freely settable by application. In this case midgard core
 * keep value of such property "as is". There is no change or update made for such 
 * property even if its value hasn't been changed by application.
 * 
 * Properties of an object:
 *
 * - creator
 *   Holds a string value with midgard_person object guid.
 *   This property can not be overwritten by application. 
 *   It is set once when particular schema object's record(s) is created.
 * - created
 *   Holds an ISO date value ( stored in UTC ).
 *   This property can not be overwritten by application. 
 *   It is set once when particular schema object's record(s) is created.
 * - revisor
 *   Holds a string value with midgard_person object guid.
 *   This property can not be overwritten by application.
 *   It is set when update method is invoked for particular schema object.
 * - revised
 *   Holds an ISO date value ( stored in UTC ).
 *   This property can not be overwritten by application.
 *   It is set when update method is invoked for particular schema object.
 * - revision
 *   Holds unsigned integer value with revision number.
 *   This property can not be overwritten by application.
 *   It is set ( increased by one ) when update method is invoked for 
 *   particular schema object.
 * - locker
 *   Holds a string value with midgard_person object guid.
 *   This property can be set by application.
 * - locked
 *   Holds an ISO date value ( stored in UTC ).
 *   This property can be set by application.
 * - approver
 *   Holds a string value with midgard_person object guid.
 *   This property can be set by application.
 * - approved
 *   Holds an ISO date value ( stored in UTC ).
 *   This property can be set by application.
 * - author
 *   Holds a string value with midgard_person object guid.
 *   This property can be set by application.
 * - owner
 *   Holds a string value with midgard_person object guid.
 *   This property can be set by application.
 * - schedulestart
 *   Holds an ISO date value ( stored in UTC ).
 *   This property can be set by application.
 * - scheduleend
 *   Holds an ISO date value ( stored in UTC ).
 *   This property can be set by application.
 * - hidden
 *   Holds boolean value.
 *   This property can be set by application.
 * - navnoentry
 *   Holds boolean value.
 *   This property can be set by application.
 * - size
 *   Holds unsigned integer value of object's size ( in bytes ).
 *   This property can not be set by application. 
 *   Object's size may vary depending on database storage and its optimized
 *   for MySQL database engine now. When object is midgard_attachment type
 *   then size is file size + object's database record size.
 * - published
 *   Holds an ISO date value ( stored in UTC ).
 *   This property can be set by application.
 *
 * There is no need to free MidgardMetadata object's memory as it is automatically
 * freed when particular object's instance memory is freed. 
 *  
 */ 
MidgardMetadata *midgard_metadata_new(MgdObject *object);

/* 
 * \ingroup midgard_metadata
 *
 * Returns MidgardObjectClass pointer.
 *
 * \param object MidgardMetadata instance
 *
 * \return MidgardObjectClass which the given MidgardMetadata object was created for.
 *
 */
MidgardObjectClass *midgard_metadata_get_class(MidgardMetadata *object);


/* OBSOLETE */
GParamSpec **midgard_class_list_properties(MgdObject *object, guint *n);

GParamSpec *midgard_class_find_property(MgdObject *object, gchar *propertyname);

void midgard_class_get_metadata(MgdObject *object, gint mmt, GValue *val);

GSList *midgard_class_metadata_types();

MidgardMetadata *midgard_object_metadata_get(MgdObject *object);

/* Returns sql string 
 *
 * @param object MidgardMetadata type object
 *
 * @return sql query used for MgdObject update or create method
 */ 

gchar *midgard_metadata_get_sql(MidgardMetadata *object);

/* Set internal midgard metadata properties when object is created
 *
 * @param object MgdObject  
 * 
 */
void midgard_metadata_set_create(MgdObject *object);

/* Set internal midgard metadata properties when object is updated
 *
 * @param object MgdObject 
 * 
 */
void midgard_metadata_set_update(MgdObject *object);

#endif /* MIDGARD_METADATA_H */
