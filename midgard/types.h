/* 
Copyright (C) 2004,2005,2006 Piotr Pokora <piotr.pokora@infoglob.pl>
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

#ifndef TYPES_H
#define TYPES_H

/** 
 * \defgroup MgdObject Midgard Object
 * 
 * <pre> #include <midgard/midgard.h> </pre>
 *
 * Midgard Object MgdObject is an user defined type and object.
 * Description of an object , its properties and database abstraction 
 * layer is made in xml files. 
 * 
 * MgdObject is represented by MIDGARD_TYPE_OBJECT type and is not associated 
 * with some particular class or type name. It's database and user defined 
 * types ( classes ) abstraction layer.
 *
 * MgdObject provides transparent , not client accessible "methods" for internal
 * midgard core's data like connection handler, sitegroup and MidgardSchema
 * (MgdSchema) data.
 */ 

#include "midgard/midgard_legacy.h"
#include <glib-object.h>
#include "midgard/query_builder.h"
#include "midgard/midgard_dbobject.h"

#define MIDGARD_TYPE_OBJECT midgard_object_get_type()
#define MIDGARD_OBJECT(object)  (G_TYPE_CHECK_INSTANCE_CAST ((object), MIDGARD_TYPE_OBJECT, MgdObject))
#define MIDGARD_OBJECT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_OBJECT, MidgardObjectClass))
#define MIDGARD_IS_OBJECT(object)   (G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_OBJECT))
#define MIDGARD_IS_OBJECT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_OBJECT))
#define MIDGARD_OBJECT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_OBJECT, MidgardObjectClass))
#define MIDGARD_OBJECT_GET_CLASS_BY_NAME(name) ((MidgardObjectClass*) g_type_class_peek(g_type_from_name(name)))

typedef struct _MgdObject MgdObject;

typedef struct _MidgardTypePrivate MidgardTypePrivate;

typedef struct MidgardMetadata MidgardMetadata;
typedef struct MidgardMetadataClass MidgardMetadataClass;

/* Get object by id and by type , only for backwards compatibility */

struct _MgdObject {
	GObject parent;
	
	/* private */
	MidgardTypePrivate *private;
	MidgardTypePrivate *priv;
	MidgardDBObjectPrivate *dbpriv;
	MgdSchemaTypeAttr *data;	

	midgard *mgd;
	GType type;
	const gchar *cname;
	gpointer *klass;
	MidgardMetadata *metadata;	
};

struct _MidgardObjectClass {
	GObjectClass parent;
	
	/* private */
	MidgardDBObjectPrivate *dbpriv;
	MgdSchemaTypeAttr *storage_data;

	/* signals */
	void (*action_create)	  	(MgdObject *object);
	void (*action_create_hook)   	(MgdObject *object);
	void (*action_created)  	(MgdObject *object);
	void (*action_update)   	(MgdObject *object);
	void (*action_update_hook)   	(MgdObject *object);
	void (*action_updated)  	(MgdObject *object);
	void (*action_delete)   	(MgdObject *object);
	void (*action_delete_hook)   	(MgdObject *object);
	void (*action_deleted)  	(MgdObject *object);
	void (*action_purge)    	(MgdObject *object);
	void (*action_purge_hook)   	(MgdObject *object);
	void (*action_purged)   	(MgdObject *object);
	void (*action_import)   	(MgdObject *object);
	void (*action_import_hook)   	(MgdObject *object);
	void (*action_imported) 	(MgdObject *object);
	void (*action_export)   	(MgdObject *object);
	void (*action_export_hook)   	(MgdObject *object);
	void (*action_exported) 	(MgdObject *object);
	void (*action_loaded) 		(MgdObject *object);
	void (*action_loaded_hook)   	(MgdObject *object);
	void (*action_approve)		(MgdObject *object);
	void (*action_approve_hook)	(MgdObject *object);
	void (*action_approved)		(MgdObject *object);
	void (*action_unapprove)	(MgdObject *object);
	void (*action_unapprove_hook)	(MgdObject *object);
	void (*action_unapproved)	(MgdObject *object);
	void (*action_lock)		(MgdObject *object);
	void (*action_lock_hook)	(MgdObject *object);
	void (*action_locked)		(MgdObject *object);
	void (*action_unlock)		(MgdObject *object);
	void (*action_unlock_hook)	(MgdObject *object);
	void (*action_unlocked)		(MgdObject *object);

	/* signals id */
	guint signal_action_loaded;
	guint signal_action_loaded_hook;
	guint signal_action_update;
	guint signal_action_update_hook;
	guint signal_action_updated;
	guint signal_action_create;
	guint signal_action_create_hook;
	guint signal_action_created;
	guint signal_action_delete;
	guint signal_action_delete_hook;
	guint signal_action_deleted;
	guint signal_action_purge;
	guint signal_action_purge_hook;
	guint signal_action_purged;
	guint signal_action_import;
	guint signal_action_import_hook;
	guint signal_action_imported;
	guint signal_action_export;
	guint signal_action_export_hook;
	guint signal_action_exported;
	guint signal_action_approve;
	guint signal_action_approve_hook;
	guint signal_action_approved;
	guint signal_action_unapprove;
	guint signal_action_unapprove_hook;
	guint signal_action_unapproved;
	guint signal_action_lock;
	guint signal_action_lock_hook;
	guint signal_action_locked;
	guint signal_action_unlock;
	guint signal_action_unlock_hook;
	guint signal_action_unlocked;
};

extern void midgard_get_midgard(MgdObject *mobj, midgard *mgd);

/**
 * \ingroup MgdObject
 *
 * Returns Midgard Object type. 
 * Registers the type as a fundamental GType unless already registered.
 */
extern GType midgard_object_get_type(void);

/** 
 * \ingroup MgdObject
 *
 * MidgardObjectAction defines current state of an object if object
 * is instatiated only by guid without type name information.
 *
 * <pre>
 * enum
 * {
 *           MGD_OBJECT_ACTION_NONE = 0,
 *           MGD_OBJECT_ACTION_DELETE,
 *           MGD_OBJECT_ACTION_PURGE,
 *           MGD_OBJECT_ACTION_CREATE,
 *           MGD_OBJECT_ACTION_UPDATE
 * }MidgardObjectAction;
 * </pre>
 * 
 * - MGD_OBJECT_ACTION_NONE , typical object state
 * - MGD_OBJECT_ACTION_DELETE, object has been deleted and can be undeleted
 * - MGD_OBJECT_ACTION_PURGE, object has been purged ( no recovery or undelete )  
 *
 */
extern enum
{
	MGD_OBJECT_ACTION_NONE = 0,
	MGD_OBJECT_ACTION_DELETE,
	MGD_OBJECT_ACTION_PURGE,
	MGD_OBJECT_ACTION_CREATE,
	MGD_OBJECT_ACTION_UPDATE
}MidgardObjectAction;

/**
 * \ingroup MgdObject
 * 
 * Creates new MgdObject object instance.
 *
 * \param mgd, midgard struct connection handler
 * \param name, object's type name 
 * \value , optional value which holds id or guid if object should 
 * be fetched from database.
 *
 * \return New MidgardObjectClass instance 
 *
 * This function is mandatory one for new midgard object initialization.
 * Unlike g_object_new ( which is typical function to create new GObject
 * instance ), midgard_object_new initializes data which are accessible
 * internally by object instance itself or by object's class:
 *
 * - midgard connection handler is associated with object
 * - 'sitegroup' property of an object is being set
 * 
 * Sitegroup value is returned from midgard connection handler and may 
 * be overwritten only by SG0 Midgard Administrator only when object
 * is created. Setting this property is forbidden when object is already
 * fetched from database. 
 *
 * Object's contructor tries to determine third optional parameter value.
 * If it's of G_TYPE_STRING type , then midgard_is_guid is called to check 
 * weather passed string is a guid , in any other case id property is used 
 * with G_TYPE_UINT type.
 * 
 * Any object instance created with this function should be freed using 
 * g_object_unref function.
 */ 
extern MgdObject *midgard_object_new(midgard *mgd, const gchar *name, GValue *value);

/** 
 * \ingroup MgdObject
 *
 * DEPRECATED: use midgard_object_new with GValue instead
 *
 * Creates new Midgard object instance and fetch record(s) from database 
 * using object's primary property.
 *
 * \param mgd Midgard structure
 * \param name name for registered type(class)
 * \param identifier value
 *
 * \return new MgdObject instance
 * 
 * The main purpose of this function is to query object's record(s) from 
 * database without any knowledge which object's property holds value of 
 * primary field and what is the type of this property.
 * midgard_object_new_by_id also minimize the need to write full reflected
 * code which could be used for the same purpose.
 *
 * Third prameter's value must be always type casted to gchar ( even if it's
 * integer type ). Internally check for primary property and its type is made
 * and midgard_object_get_by_id or midgard_object_get_by_guid is called to 
 * return correct object's instance.
 *
 * NULL is returned when object can not be queried from database.
 * 
 */ 
extern MgdObject *midgard_object_new_by_id(midgard *mgd, const gchar *name, gchar *id);

/**
 * \ingroup MgdObject
 *
 * Fetch object's record(s) from database using 'id' property.
 *
 * \param object,  MgdObject instance
 * \param id, object's integer identifier
 *
 * \return TRUE if object's record(s) is successfully fetched from database.
 * FALSE otherwise.
 * 
 * This is common practice to use 'id' property with integer type when table's
 * id column stores unique value which identifies object and its record(s).
 * However primary property with integer type is freely defined by user.
 * 
 * MgdObject object instance must be created with midgard_object_new function.
 * When midgard connection handler is not associated with object instance, 
 * application is terminated with 'assertion fails' error message being logged. 
 *
 * Object instance created with this function should be freed using g_object_unref.
 *  
 */ 
extern gboolean midgard_object_get_by_id(MgdObject *object, guint id);

/**
 * \ingroup MgdObject
 *
 * Update object's record(s).
 *
 * \param object MgdObject
 *
 * \return TRUE when object's record(s) is successfully updated.
 * FALSE otherwise.
 *
 * This function list all object's properties and creates query using 
 * properties values which are already initialized. Value of property
 * which is not initialized is ignored. 
 * 
 * Internally such properties are being set (overwritten):
 *
 *  - sitegroup
 *  - revisor ( object's metadata property ) 
 *  - revision ( object's metadata property ) 
 *  - revised ( object's metadata property )
 *  
 * FALSE is returned in such cases:
 *
 * - There is no midgard_person object associated with connection handler.
 * - Midgard Schema is not set for this object ( this is internal fatal error )  
 * - Object's has no storage defined.
 * - Object has property 'name' defined and such named record already exists 
 *   in object's table
 * - Quota size is reached  
 * - Database query failed ( this is internal fatal error )
 * - Sitegroup property of an object was changed ( critical error )   
 * 
 * Midgard error set by this method:
 * 
 * - MGD_ERR_OK
 * 	-# Object's record has been updated successfully.
 * 	Returns TRUE
 * - MGD_ERR_SITEGROUP_VIOLATION
 * 	-# Sitegroup of an object has been chnaged  
 * - MGD_ERR_DUPLICATE
 * 	-# Another object with the same name already exists in tree  
 * - MGD_ERR_QUOTA
 * 	-# Quota limit is reached
 * - MGD_ERR_INTERNAL
 * 	-# SQL query failed
 * 	-# No table defined for object's type
 * - MGD_ERR_INVALID_PROPERTY_VALUE
 * 	-# Some property holds incorrect value.
 */ 
extern gboolean midgard_object_update(MgdObject *object);

/**
 * \ingroup MgdObject
 *
 * Creates new database record(s) for object.
 *
 * \param object MgdObject
 *
 * This function list all object's properties and creates query using 
 * properties values which are already initialized. Value of property
 * which is not initialized is set to its default value.
 *
 * Internally such properties are being set (overwritten):
 * 
 * - sitegroup
 * - guid (since Midgard 1.7.5)
 * - id (if set as primary property)
 * - metadata.creator (since Midgard 1.8)
 * - metadata.created (since Midgard 1.8)
 * - metadata.revisor (since Midgard 1.8)
 * - metadata.revised (since Midgard 1.8)
 * - metadata.revision (since Midgard 1.8)
 * - metadata.published (since Midgard 1.8)
 * 
 * Midgard error set by this method:
 *
 * - MGD_ERR_OK
 *   -# Object's record has been created successfully
 *   	Returns TRUE
 * - MGD_ERR_NOT_EXISTS
 *   -# Storage table not defined in schema
 *   -# SQL query failed ( database query error is populated with warn log level ) 
 *   	Returns FALSE
 * - MGD_ERR_DUPLICATE
 *   -# Object with the same name already exists in tree
 *   	Returns FALSE
 * - MGD_ERR_INTERNAL
 *   	-# Repligard record hasn't been created for object's record
 *   	-# Critical unknown reason
 *   	   Returns FALSE
 * - MGD_ERR_INVALID_PROPERTY_VALUE
 * 	-# Some property holds incorrect value. ( For example property with MGD_TYPE_GUID type
 * 	holds string which doesn't contain at least 21 and at most 80 characters).
 * 		-# GUID and UUID reference http://www.midgard-project.org/development/mrfc/0018.html
 *
 * Since Midgard 1.8 , Midgard Administrator (Sitegroup 0) is able to specify 
 * sitegroup of created object.
 */
extern gboolean midgard_object_create(MgdObject *object);

/**
 * \ingroup MgdObject 
 *
 * Find object(s) record(s) in database for the properties being set
 * for object instance.
 *
 * \param object MgdObject
 * \param[out] holder pointer to MidgardTypeHolder which stores number of returned objects
 *
 * \return objects
 *
 * This function is a simple wrapper for Midgard Query Builder.
 * Unlike QB it only uses equal operators while looking for object records
 * in database storage. It should be usefull if used with properties like 
 * up and name ( get by name ), up or parent ( list ), or similiar ones.
 *
 * You should free type holder structure if not longer needed.
 * This parameter is optional.
 * Number of objects is available as holder->elements.
 *
 */ 
extern GObject **midgard_object_find(MgdObject *object, MidgardTypeHolder *holder);

extern gchar * midgard_object_build_path(MgdObject *mobj);

/**
 * \ingroup MgdObject
 *
 * Checks whether object exists in parent's type tree.
 *
 * \param object MgdObject
 * \param rootid guint which identifies parent's type primary property
 * \param id guint which identify object's primary property
 *
 * \return TRUE when object exists in parent tree, FALSE otherwise.
 *
 * Midgard tree in MgdSchema reference:
 * http://www.midgard-project.org/midcom-permalink-a4e185a08fb2d0e278ef1ba3a739f77e
 *
 * Parent type in midgard tree is a type which can be "container" with object nodes
 * of different types. 
 * 
 */ 
extern gboolean midgard_object_is_in_parent_tree(MgdObject *object, guint rootid, guint id);

/**
 * \ingroup MgdObject 
 *
 * Checks whether object exists in tree ( of the same type ).
 *
 * \param object MgdObject
 * \param rootid guint value which identifies type's primary property
 * \param id guint value which identifies object's primary property
 *
 * \return TRUE when object exists in tree, FALSE otherwise.
 *
 * Midgard tree in MgdSchema reference:
 * http://www.midgard-project.org/midcom-permalink-a4e185a08fb2d0e278ef1ba3a739f77e
 * 
 * This function checks only objects of the same type.
 * 
 */
extern gboolean midgard_object_is_in_tree(MgdObject *object, guint rootid, guint id);

extern gchar * midgard_object_get_tree(MgdObject *object, GSList *tnodes);

/**
 * \ingroup MgdObject 
 * 
 * Fetch object's record(s) from database using 'guid' property.
 *
 * \param object,  MgdObject instance
 * \param guid, string with guid value
 *
 * \return TRUE if object's record(s) is successfully fetched from database.
 * FALSE otherwise.
 * 
 * MgdObject object instance must be created with midgard_object_new function.
 * When midgard connection handler is not associated with object instance, 
 * application is terminated with 'assertion fails' error message being logged. 
 * 
 * Object instance created with this function should be freed using g_object_unref.
 */ 
extern gboolean midgard_object_get_by_guid(MgdObject *object, const gchar *guid);

/*
 * \ingroup MgdObject
 *
 * Delete object's record(s) from database.
 *
 * \param object MgdObject
 *
 * \return TRUE if object's record(s) is successfully deleted from database.
 * FALSE otherwise.
 *
 * Object's record is not fully deleted from database. Instead, it is marked 
 * as deleted , thus it is possible to undelete object later with static 
 * method midgard_object_undelete.
 *
 * If object's klass is defined as multilingual , and there is more tan one 
 * language content in database , then only particular object's content is
 * removed. In such case object is not marked as deleted.
 * Content itself is not removed if language is set to 0.
 *
 * FALSE is returned in such cases:
 *
 * - Midgard Schema is not set for this object ( MGD_ERR_INTERNAL )
 * - Object's has no storage defined ( MGD_ERR_OBJECT_NO_STORAGE )
 * - Object's property guid is empty ( MGD_ERR_INVALID_PROPERTY_VALUE )
 * - SQL query failed ( MGD_ERR_INTERNAL )
 *  
 */ 
extern gboolean midgard_object_delete(MgdObject *object);

/**
 * \ingroup MgdObject
 *
 * Purge object's record(s) from database.
 *
 * \param object MgdObject
 *
 * \return TRUE if object has been succesfully purged from database.
 * FALSE otherwise.
 *  
 * Caller is responsible to set 'guid' property before calling this function.
 *
 * FALSE is returned in such cases:
 *
 * - Midgard Schema is not set for this object ( MGD_ERR_INTERNAL )
 * - Object's has no storage defined ( MGD_ERR_OBJECT_NO_STORAGE )
 * - Object's property guid is empty ( MGD_ERR_INVALID_PROPERTY_VALUE ) 
 * - Object's property id is less then 1 ( MGD_ERR_INVALID_PROPERTY_VALUE )
 *   ( only if class is multilang )
 * - No record has been deleted from database ( MGD_ERR_NOT_EXISTS )
 *
 */ 
extern gboolean midgard_object_purge(MgdObject *object);

/**
 * \ingroup MgdObject
 *
 * Fetch parent object.
 *
 * \param object MgdObject
 *
 * \return new MgdObject object instance or NULL if parent object 
 * can not be fetched from database.
 *
 * This function fetch "upper" object of the same type, and then parent 
 * object if object's up property holds empty value. 
 * Object is root object in tree when NULL is returned.
 *
 */ 
extern MgdObject *midgard_object_get_parent(MgdObject *object);

extern gint midgard_object_get(MgdObject *object);

/**
 * \ingroup MgdObject
 *
 * Return objects which up property is equal with object primary property
 *
 * \param object MgdObject object instance
 * \param[out] holder pointer  which stores number of returned objects
 *
 * \return newly allocated objects array terminated by NULL
 * 
 * \c holder parameter is optional ( may be passed as NULL ).
 * If used, should be freed when no longer needed.
 * holder->elements stores number of returned objects
 *
 */
extern GObject **midgard_object_list(MgdObject *object, MidgardTypeHolder *holder);

/**
 * \ingroup MgdObject
 *
 * Return child objects
 *
 * \param object MgdObject object instance
 * \param childname of child typename
 * \param[out] holder pointer which stores number of returned objects
 *
 * \return newly allocated child objects array terminated by NULL
 *
 * \c holder parameter is optional ( may be passed as NULL ).
 * If used, should be freed when no longer needed.
 * holder->elements stores number of returned objects
 *
 */
extern GObject **midgard_object_list_children(MgdObject *object, const gchar *childname, 
		MidgardTypeHolder *holder);
/**
 * \ingroup MgdObject 
 *
 * Returns midgard_language objects for the given Midgard Object instance.
 *
 * \param self MgdObject instance
 * \param holder, pointer which stores number of returned objects
 *
 * \return midgard_language objects' array terminated by NULL
 *
 * Returned array is limited to midgard_language objects for which content
 * exists for the given Midgard Object instance.
 * 
 * \c holder parameter is optional ( may be passed as NULL ).
 * If used, should be freed when no longer needed.
 * holder->elements stores number of returned objects
 */ 
extern GObject **midgard_object_get_languages(
	MgdObject *self, MidgardTypeHolder *holder);

/** 
 * \ingroup MgdObject
 *
 * Returns object by path
 */ 
extern MgdObject *midgard_object_get_by_path(midgard *mgd, const gchar *classname , const gchar *path);

/**
 * \ingroup MgdObject
 *
 * Returns type name of parent object ( in content tree ).
 */ 
extern const gchar *midgard_object_parent(MgdObject *object);

/**
 * \ingroup MgdObject
 * 
 * Undelete object
 *
 * \param mgd, MidgardConnection pointer
 * \guid, identifier for deleted object
 *
 * \return TRUE if object has been undeleted, FALSE otherwise.
 *
 * Cases to return FALSE:
 *
 * - There is no record in repligard's table with the given guid's value
 *   ( MGD_ERR_NOT_EXISTS )
 * - Object can not be undeleted ( MGD_ERR_INTERNAL )  
 * - Object is already purged ( MGD_ERR_OBJECT_PURGED )   
 * - Object is not deleted ( MGD_ERR_USER_DATA )  
 */
extern gboolean midgard_object_undelete(MidgardConnection *mgd, 
			const gchar *guid);

/**
 * \ingroup MgdObject 
 *	
 * Export given object to xml. 	
 * 
 * \param self , MidgardObject instance
 *
 * \return object serialized to xml format on success , NULL otherwise.
 * 
 * Returns newly allocated string as object with its properties serialized 
 * to xml format, and sets object's metadata exported value to current datetime.
 * UTF-8 character encoding is set for returned xml.
 * 
 * Cases to return NULL:
 *
 * - Object has been recently exported ( MGD_ERR_OBJECT_EXPORTED )  
 */ 
extern gchar *midgard_object_export(MgdObject *self);


/** 
 * \ingroup MgdObject 
 *
 * Import Midgard Object from xml.
 *
 * \param mgd, MidgardConnection handler's pointer
 * \param xml, xml string
 *
 * \return TRUE if import has been successfull, FALSE otherwise.
 * 
 * Imports object to database. 
 * Given xml is used as serialized object's data.
 *
 * Cases to return FALSE:
 *
 * - object is already imported ( MGD_ERR_OBJECT_IMPORTED )
 * 
 * Import method determines whether create or update method should 
 * be invoked for imported object, so all error cases for create
 * and update method are valid for import one.
 */
extern gboolean midgard_import_object(
			MidgardConnection *mgd, gchar *xml);

/**
 * \ingroup MgdObject 
 *
 * Sets object's guid
 *
 * \param self, MgdObject instance
 * \param guid, guid which must be set for given object
 *
 * \return TRUE on success, FALSE otherwise
 *
 * Midgard error set by this method:
 *
 * - MGD_ERR_DUPLICATE
 *   -# given guid already exists in database
 * - MGD_ERR_INVALID_PROPERTY_VALUE
 *   -# given guid is invalid
 *   -# object has already set guid property
 */
extern gboolean midgard_object_set_guid(MgdObject *self, const gchar *guid);

gboolean midgard_object_approve(MgdObject *self);
gboolean midgard_object_is_approved(MgdObject *self);
gboolean midgard_object_unapprove(MgdObject *self);

gboolean midgard_object_lock(MgdObject *self);
gboolean midgard_object_is_locked(MgdObject *self);
gboolean midgard_object_unlock(MgdObject *self);

/*
 * It's located here due to legacy code in midgard.c. 
 */ 
void midgard_init();

#endif /* TYPES_H */
