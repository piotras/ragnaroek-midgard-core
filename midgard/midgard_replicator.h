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

#ifndef MIDGARD_REPLICATOR_H
#define MIDGARD_REPLICATOR_H

/**
 * \defgroup replicator Midgard Replicator
 *
 * The main purpose of Midgard Replicator is to serialize and unserialize
 * objects and export or import them among different databases.
 *
 * References:
 *  - http://www.midgard-project.org/development/mrfc/view/0030.html
 *  - http://www.midgard-project.org/development/mrfc/view/0033.html
 *
 * Object Hierarchy
 * 	- GObject
 * 		- MidgardReplicator
 */ 

#include <midgard/types.h>

/* convention macros */
#define MIDGARD_TYPE_REPLICATOR (midgard_replicator_get_type())
#define MIDGARD_REPLICATOR(object)  \
	(G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_REPLICATOR, MidgardReplicator))
#define MIDGARD_REPLICATOR_CLASS(klass)  \
	(G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_REPLICATOR, MidgardReplicatorClass))
#define MIDGARD_IS_REPLICATOR(object)   \
	(G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_REPLICATOR))
#define MIDGARD_IS_REPLICATOR_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_REPLICATOR))
#define MIDGARD_REPLICATOR_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_REPLICATOR, MidgardReplicatorClass))

/** 
 * \ingroup replicator 
 *
 * Private structure for Midgard Replicator.
 */ 
typedef struct _MidgardReplicatorPrivate MidgardReplicatorPrivate;

typedef struct MidgardReplicator MidgardReplicator;

/**
 * \ingroup replicator
 * The opaque Midgard Replicator type.
 */
struct MidgardReplicator{
	GObject parent;
	
	/* < private > */
	MidgardReplicatorPrivate *private;
};


typedef struct MidgardReplicatorClass MidgardReplicatorClass;

/**
 * \ingroup replicator
 * The opaque Midgard Replicator Class type.
 */
struct MidgardReplicatorClass{
	GObjectClass parent;
	
	/* class members */
};

/** 
 * \ingroup replicator
 *
 * Returns MidgardReplicator type.
 * Registers the type as  a fundamental GType unless already registered.
 */ 
extern GType midgard_replicator_get_type(void);

/**
 * \ingroup replicator 
 * 
 * Creates new MidgardReplicator instance for the given Midgard Object.
 *
 * \param object, Midgard Object instance
 *
 * \return MidgardReplicator instance, or NULL
 *
 * MidgardObject \c object argument can be ommited and NULL can be passed
 * to constructor. If NULL is passed as argument then all methods like 
 * export or serialize will require not NULL object argument. 
 *
 * For example , there is no need to instatiate MidgardReplicator if you need
 * to export many objects identified by guid.
 *
 * Such code can be executed as static method calls on higher levels.
 *
 * <pre>
 * const gchar *guid_a = "000bf7425e8e11dba4a397c673110db70db7";
 * const gchar *guid_b = "038b351e6a6311dba1c1719fddb8579a579a";
 *
 * midgard_replicator_export(NULL, guid_a, NULL);
 * midgard_replicator_export(NULL, guid_b, NULL);
 * </pre>
 *
 * And this example shows how particular object instance is assigned to 
 * replicator object.
 *
 * <pre>
 * 
 * MgdObject *object = midgard_object_new(mgd, "midgard_article", 1);
 * MidgardReplicator *replicator = midgard_replicator_new(object);
 *
 * gchar *xml = midgard_replicator_serialize(replicator, NULL);
 * // Do something with xml string  
 * midgard_replicator_export(replicator, NULL);
 *
 * </pre>
 *
 * Function will return NULL if object's type is not registered through MgdSchema.
 *
 */
extern MidgardReplicator *midgard_replicator_new	(MgdObject *object);

/**
 * \ingroup replicator 
 *
 * Returns serialized objects as xml content.
 *
 * \param self , MidgardReplicator instance  
 * \param object , Midgard Object instance
 * 
 * \return newly allocated xml string which should be freed when no longer needed
 *
 * For statically called methods , NULL should be passed instead of \c self.
 *
 * NULL can be passed instead of MgdObject instance , if replicator is already 
 * instatiated for the given object.
 *
 * NULL is returned if object can not be serialized.
 */ 
extern gchar *midgard_replicator_serialize(	MidgardReplicator *self, 
						GObject *object);

/**
 * \ingroup replicator 
 * 
 * Export object identified by the given guid.
 *
 * \param self, MidgardReplicator instance
 * \param object, MgdObject instance  
 * 
 * \return TRUE on success, FALSE otherwise
 *
 * For statically called methods , NULL should be passed instead of \c self.
 *
 * NULL can be passed instead of  \c object if particular object is assigned to 
 * replicator instance ( via constructor ).
 * This method simply updates object's metadata exported property.
 */
extern gboolean midgard_replicator_export(	MidgardReplicator *self, 
						MidgardDBObject *object);

/**
 * \ingroup replicator
 *
 * Export purged objects.
 *
 * \param self, MidgardReplicator instance
 * \param klass, MidgardObjectClass pointer
 * \param mgd, optional MidgardConnection pointer
 * \param startdate, optional datetime 
 * \param enddate, optional datetime
 *
 * \return xml content 
 *
 * For statically called methods , NULL should be passed instead of \c self.
 *
 * Optional \c startdate and \c enddate datetime defines the period of time 
 * for which purged objects should be exported. If \c startdate is passed and 
 * \c enddate is NULL, all objects purged after startdate will be exported.
 * If only \c startdate is NULL , all objects will be exported which has been 
 * purged till \c enddate.
 *
 * Export all purged objects for the given MidgardObjectClass. 
 * All objects' guids and purge dates are represented as nodes of returned xml file.
 *
 */ 
extern gchar *midgard_replicator_export_purged(	MidgardReplicator *self, 
							MidgardObjectClass *klass,
							MidgardConnection *mgd,
							const gchar *startdate,
							const gchar *enddate);

/**
 * \ingroup replicator 
 *
 * Serialize binary data
 *
 * \param self, MidgardReplicator instance
 * \param object, Midgard Object instace
 * 
 * \return xml content with binary data
 *
 * For statically called methods , NULL should be passed instead of \c self.
 *
 * Given Midgard Object \c object can be any type which can be used as 
 * attachments' ( binary data ) handler.
 * If there is no such type defined in MgdSchema, then only midgard_attachment
 * object instance can be used.
 *
 * Returned xml contains binary data and also guid of an object for which 
 * data are attached. This method doesn't mark object's record as exported.
 *
 */ 
extern gchar *midgard_replicator_serialize_blob( MidgardReplicator *self,
						MgdObject *object);
/**
 * \ingroup replicator
 *
 * Exports binary data.
 *
 * Use serialize_blob instead this method.
 */ 
extern gchar *midgard_replicator_export_blob( MidgardReplicator *self,
						MgdObject *object);

/** 
 * \ingroup replicator
 *
 * Marks as exported the object which is identified by the given guid.
 *
 * \mgd, MidgardConnection instance
 * \param guid
 *
 * \return TRUE on success, FALSE otherwise
 *
 */
extern gboolean midgard_replicator_export_by_guid ( MidgardConnection *mgd,
							const gchar *guid );

/**
 * \ingroup replicator
 *
 * Export binary media file.
 *
 * References:
 * http://www.w3.org/TR/xml-media-types/
 *
 * Not yet implemented.
 *
 */ 
extern gchar *midgard_replicator_export_media( MidgardReplicator *self,
						MgdObject *object);

/**
 * \ingroup replicator
 *
 * Creates object instance from the given xml string.
 *
 * \param self, MidgardReplicator instance
 * \param mgd, MidgardConnection instance
 * \param xml, valid xml content
 * \param force, whether we should ignore dependencies
 *
 * \return NULL terminated array of MgdObject objects , or NULL
 *
 * If force is set to TRUE , then unserialize doesn't map guids to ids
 * and unconditionally set every id ( which is link type ) to 0.
 *
 * For statically called methods , NULL should be passed instead of \c self.
 *
 */ 
extern GObject **midgard_replicator_unserialize(	MidgardReplicator *self,
							MidgardConnection *mgd,
							const gchar *xml,
							gboolean force);

/**
 * \ingroup replicator
 *
 * Imports object to database.
 *
 * \param self, MidgardReplicator instance
 * \param object, MgdObject instance
 * \param force, overwrite exisitng object
 *
 * \return TRUE on success, FALSE otherwise
 *
 * For statically called methods , NULL should be passed instead of \c self.
 * If third \c force argument is set to TRUE, then more recent object in the database
 * will be overwriten and deleted object will be "re created".
 */
extern gboolean midgard_replicator_import_object(	MidgardReplicator *self, 
							MidgardDBObject *object, gboolean force);

/**
 * \ingroup replicator
 *
 * Imports data from the given xml content.
 *
 * \param self, MidgardReplicator instance
 * \param mgd, MidgardConnection instance
 * \param xml, xml content from which data should be imported
 * \param force, boolean set to TRUE if this method should overwrite data
 *
 * For statically called methods , NULL should be passed instead of \c self.
 *
 */ 
extern void midgard_replicator_import_from_xml(		MidgardReplicator *self,
							MidgardConnection *mgd,
							const gchar *xml,
							gboolean force);


/**
 * \ingroup replicator
 *
 * Validates the given xml content.
 *
 * \param xml, xml content
 *
 * \return TRUE if xml is valid, FALSE otherwise
 *
 * You shouldn't use this method before calling unserialize.
 * Simply , unserialize method does validates xml content internally.
 * You should use this method , if you want to store xml contents as files
 * for later replication ( for example ).
 */ 
extern gboolean midgard_replicator_xml_is_valid(const gchar *xml);

#endif /* MIDGARD_REPLICATOR_H */
