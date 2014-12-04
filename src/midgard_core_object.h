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

#ifndef _MIDGARD_CORE_OBJECT_H
#define _MIDGARD_CORE_OBJECT_H

#include <midgard/midgard_legacy.h>
#include <libxml/parser.h> 
#include <libxml/tree.h>
#include "midgard/types.h"
#include "midgard_mysql.h"

struct _MidgardDBObjectPrivate {
	const gchar *guid;
	guint sg;
	MgdSchemaTypeAttr *storage_data;
	MidgardConnection *mgd;

	void (*set_from_xml_node) (MidgardDBObject *self, xmlNode *node);
};

/* Private structure for private data of MgdSchema objects */
struct _MidgardTypePrivate{
	const gchar *guid;
	guint sg;
	const gchar *action;
	MgdSchemaTypeAttr *data;
	gchar *exported;
	gchar *imported;
	GSList *parameters;
	GHashTable *_params;
	midgard_object_mysql *_mysql;
	MYSQL_ROW _mysql_row;
	gboolean read_only;
};

#define MGD_OBJECT_GUID(___obj) (MIDGARD_IS_OBJECT(___obj) ? MIDGARD_OBJECT(___obj)->private->guid : MIDGARD_DBOBJECT(___obj)->dbpriv->guid)
#define MGD_OBJECT_CNC(___obj) (MIDGARD_IS_OBJECT(___obj) ? MIDGARD_OBJECT(___obj)->mgd->_mgd : MIDGARD_DBOBJECT(___obj)->dbpriv->mgd)
#define MGD_OBJECT_SG(___obj) ___obj->private->sg

/* Private structure for private data of MgdSchema object's metadata */
struct _MidgardMetadataPrivate {

        /* Object properties */
	gchar *creator;
	gchar *created;
	gchar *revisor;
	gchar *revised;
	guint revision;
	gchar *locker;
	gchar *locked;
	gchar *approver;
	gchar *approved;
	gchar *authors;
	gchar *owner;
	gchar *schedule_start;
	gchar *schedule_end;
	gboolean hidden;
	gboolean nav_noentry;
	guint32 size;
	gchar *published;
	gchar *exported;
	gchar *imported;
	gboolean deleted;
	gint32 score;

	gboolean is_locked;
	gboolean lock_is_set;
	gboolean is_approved;
	gboolean approve_is_set;

	/* Other sruct members */
	MgdObject *object;
};

/* Private structure for Midgard Config object */
struct _MidgardConfigPrivate{

	GKeyFile *keyfile;
	guint dbtype;
	GIOChannel *log_channel;
	gchar *configname;
	
	gchar *host;
	gchar *database;
	gchar *dbuser;
	gchar *dbpass;
	gchar *blobdir;
	gchar *logfilename;
	gchar *schemafile;
	gchar *default_lang;
	gchar *loglevel;
	gboolean tablecreate;
	gboolean tableupdate;
	FILE  *logfile;
	gchar *mgdusername;
	gchar *mgdpassword;
	gboolean testunit;
	guint loghandler;
	midgard_auth_type authtype;
	gchar *pamfile;
};

struct _MidgardConnectionPrivate{
        MidgardConfig *config;
	gboolean free_config;
	guint loghandler;
	guint loglevel;
	const gchar *sitegroup;
	guint sitegroup_id;
	const gchar *lang;
	guint lang_id;
	const gchar *default_lang;
	guint default_lang_id;
	MidgardSchema *schema;
	GObject *user;
	gboolean implicit_user;
	gchar *configname;
	gpointer sg_ids;
	gboolean is_copy;

	gboolean enable_replication;
	gboolean enable_quota;
	gboolean enable_dbus;
};

#define MGD_CNC_QUOTA(_cnc) _cnc->priv->enable_quota
#define MGD_CNC_REPLICATION(_cnc) _cnc->priv->enable_replication
#define MGD_CNC_DBUS(_cnc) _cnc->priv->enable_dbus

typedef enum {
	OBJECT_UPDATE_NONE = 0,
	OBJECT_UPDATE_EXPORTED,
	OBJECT_UPDATE_IMPORTED
} _ObjectActionUpdate;

struct _MidgardBlobPrivate {
	MgdObject *attachment;
	gchar *location;
	MidgardConnection *mgd;
	gchar *blobdir;
	GIOChannel *channel;
	gchar *filepath;
	gchar *parentguid;
	gchar *content;
	gchar *encoding;
};

/* core object */
void midgard_core_metadata_property_attr_new(MgdSchemaTypeAttr *type_attr);

/* Object's xml */
xmlDoc *_midgard_core_object_create_xml_doc(void);
void _midgard_core_object_get_xml_doc(  MidgardConnection *mgd,
					const gchar *xml,
					xmlDoc **doc,
					xmlNode **root_node);
gchar *_midgard_core_object_to_xml(GObject *object);
gboolean _nodes2object(GObject *object, xmlNode *node, gboolean force);
xmlNode *_get_type_node(xmlNode *node);
GObject **_midgard_core_object_from_xml(MidgardConnection *mgd, const gchar *xml, gboolean force);

xmlNode *__dbobject_xml_lookup_node(xmlNode *node, const gchar *name);
gchar *__get_node_content_string(xmlNode *node);
gint __get_node_content_int(xmlNode *node);
guint __get_node_content_uint(xmlNode *node);
gboolean __get_node_content_bool(xmlNode *node);

/* Object's routines */
gboolean _midgard_object_update(MgdObject *object, _ObjectActionUpdate replicate);
gboolean _midgard_object_create(MgdObject *object, const gchar *create_guid, _ObjectActionUpdate replicate);
void _object_copy_properties(GObject *src, GObject *dest);
gboolean _midgard_object_violates_sitegroup(MgdObject *object);

/* Links */
gboolean _midgard_core_object_prop_link_is_valid(GType ltype);

/* Tree related routines */
gboolean _midgard_core_object_prop_type_is_valid(GType src_type, GType dst_type);
gboolean _midgard_core_object_prop_parent_is_valid(GType ptype);
gboolean _midgard_core_object_prop_up_is_valid(GType ptype);
GType _midgard_core_object_get_property_parent_type(MidgardObjectClass *klass);
GType _midgard_core_object_get_property_up_type(MidgardObjectClass *klass);
gboolean _midgard_core_object_prop_parent_is_set(MgdObject *object);
gboolean _midgard_core_object_prop_up_is_set(MgdObject *object);

/* D-Bus */
void _midgard_core_dbus_send_serialized_object(MgdObject *object, const gchar *path);

/* Legacy workarounds */
void id_list_free(gpointer idsptr);
guint id_list_lookup(gpointer idsptr, const gchar *name);

/* MySQL results */
#define MGD_RES_OBJECT_IDX 24

#endif /* _MIDGARD_CORE_OBJECT_H */
