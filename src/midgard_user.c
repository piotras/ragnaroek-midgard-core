/* 
 * Copyright (C) 2007 Piotr Pokora <piotrek.pokora@gmail.com>
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

#include "config.h"
/* stddef included due to bug in openssl package.
 * In theory 0.9.8g fix this, but on some systems stddef is not included.
 * Remove this include when stddef is included in openssl dev headers. */
#include <stddef.h>
#include <openssl/md5.h>
#include "midgard_core_query.h"
#include "midgard_core_object.h"
#include "midgard/midgard_dbobject.h"
#include "midgard_core_query.h"
#include "midgard/midgard_user.h"
#include "midgard/midgard_error.h"
#include "midgard/query.h"
#include "midgard/midgard_replicator.h"
#include "schema.h"

#if HAVE_CRYPT_H
#include <crypt.h>
#else
#define _XOPEN_SOURCE
#endif

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
/* Mac OS X puts PAM headers into /usr/include/pam, not /usr/include/security */
#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif

struct _MidgardUserPrivate {
	MgdObject *person;
	guint user_type;
	gchar *login;
	gchar *password;
	gboolean active;
	guint hashtype;
	gboolean fetched;
};

#define MIDGARD_USER_TYPE_NONE 0
#define MIDGARD_USER_TYPE_USER 1
#define MIDGARD_USER_TYPE_ADMIN 2
#define MIDGARD_USER_TYPE_ROOT 3

#define MIDGARD_USER_TABLE "midgard_user"

/* MidgardUser object properties */
enum {
	MIDGARD_USER_GUID = 1,
	MIDGARD_USER_LOGIN, 
	MIDGARD_USER_PASS, 
	MIDGARD_USER_ACTIVE,
	MIDGARD_USER_SG,
	MIDGARD_USER_HASH
};

/* PAM AUTH DATA */
typedef struct {
	const gchar *username;
	const gchar *password;
} midgard_auth_pam_appdata;

static void __initialize_user(MidgardUser *self)
{
	g_assert(self != NULL);
	self->priv = g_new(MidgardUserPrivate, 1);
	self->priv->person = NULL;
	self->priv->user_type = 0;
	self->priv->fetched = FALSE;
	self->priv->login = NULL;
	self->priv->password = NULL;
	self->priv->active = FALSE;
	self->priv->hashtype = 0;
	
	self->dbpriv = g_new(MidgardDBObjectPrivate, 1);
	self->dbpriv->mgd = NULL;
	self->dbpriv->sg = -1;
	self->dbpriv->guid = NULL;
}

/* Creates new MidgardUser object for the given Midgard Object. */
MidgardUser *midgard_user_new(MgdObject *person)
{
	MidgardConnection *mgd = NULL;

	if(person && !MIDGARD_IS_OBJECT(person)) {
		g_warning("Expected MidgardObject type");
		return NULL;
	}

	if(person && !person->private->guid) {
		g_warning("Midgard user initialized for person with NULL guid!");
		return NULL;
	}

	MidgardUser *self =
		g_object_new(MIDGARD_TYPE_USER, NULL);

	if(person) {

		mgd = person->mgd->_mgd;
	
		self->dbpriv->mgd = mgd;
		self->dbpriv->guid = g_strdup(person->private->guid);
		self->dbpriv->sg = person->private->sg;
		self->priv->person = MIDGARD_OBJECT(person);
		mgd->priv->user = G_OBJECT(self);
	}

	return self;
}

/* Authenticate Midgard User. */
MidgardUser *midgard_user_auth(MidgardConnection *mgd, 
		const gchar *name, const gchar *password, 
		const gchar *sitegroup, gboolean trusted)
{
	g_return_val_if_fail(mgd != NULL, NULL);

	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	/* Empty login name. Not allowed */
	if(!name) {
		g_warning("Can not authenticate user with empty login");
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INVALID_NAME);
		return NULL;
	}

	/* Password and trusted is not set */
	if(!trusted && !password) {
		g_warning("Can not authenticate user with empty password");
		return NULL;
	}

	/* Trusted auth not allowed for SG0 users */
	if(!sitegroup && trusted) {
		g_warning("Can not authenticate SG0 user using \"trusted\" type");
		return NULL;
	}

	gboolean sg0 = FALSE;
	gchar *namewd; 
	
	if(sitegroup != NULL 
			&& (*sitegroup  && sitegroup[0] != '\0'))
		namewd = g_strconcat(name, "+", sitegroup, NULL);
	else 
	{
		sg0 = TRUE;
		namewd = g_strdup(name);
	}
	
	gint uid = 0;
	if (trusted == TRUE)
	{
		uid = mgd_auth_trusted(mgd->mgd, namewd);
	}
	else if (sg0 == TRUE)
	{
		uid = mgd_auth_sg0(mgd->mgd, namewd, password, 1);
	}
	else
	{
		uid = mgd_auth(mgd->mgd, namewd, password, 1);
	}

	g_free(namewd);

	if(uid < 1 )
		return NULL;
	
	MidgardUser *user = midgard_connection_get_user(mgd);	

	if(user == NULL) {

		user = midgard_user_new(mgd->mgd->person);
		mgd->priv->user = G_OBJECT(user);	
	}

	g_signal_emit_by_name(mgd, "auth_changed");

	return user;
}

/* Check if user is a user */
gboolean midgard_user_is_user(MidgardUser *self)
{
	if(!self)
		return FALSE;

	if(!self->dbpriv->mgd->mgd->current_user->is_root
			&& !self->dbpriv->mgd->mgd->current_user->is_admin)
		return TRUE;

	return FALSE;
}

/* Check if user is an admin */
gboolean midgard_user_is_admin(MidgardUser *self)
{
	if(!self)
		return FALSE;

	if(self->dbpriv->mgd->mgd->current_user->is_admin)
		return TRUE;

	return FALSE;
}

/* Check if user is root */
gboolean midgard_user_is_root(MidgardUser *self)
{
	if(!self)
		return FALSE;

	if(self->dbpriv->mgd->mgd->current_user->is_root)
		return TRUE;

	return FALSE;
}

/* Set active flag */
gboolean midgard_user_set_active(MidgardUser *self, gboolean flag)
{
	g_warning("set_active is not supported");

	return FALSE;
}

static gboolean __set_legacy_password(MidgardUser *self, const gchar *login, 
		const gchar *password, guint hashtype) 
{
	if(self->dbpriv->guid == NULL)
		return FALSE;

	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, self->dbpriv->guid);
	MgdObject *person = 
		midgard_object_new(self->dbpriv->mgd->mgd, "midgard_person", &gval);
	g_value_unset(&gval);

	if(!person)
		return FALSE;
	
	GString *sql = g_string_new("UPDATE person SET ");
	g_string_append_printf(sql, "username = '%s', password = ", login);
	
	if(hashtype == MIDGARD_USER_HASH_LEGACY) {
		
		g_string_append_printf(sql, "Encrypt('%s') ", password);

	} else {

		g_string_append_printf(sql, "'**%s' ", password);
	}

	g_string_append_printf(sql, "WHERE guid = '%s' AND sitegroup = %d ",
			self->dbpriv->guid, self->dbpriv->sg);

	gint up = midgard_query_execute(self->dbpriv->mgd->mgd, sql->str, NULL);

	g_string_free(sql, FALSE);

	if(up > 0) {

		midgard_replicator_export(NULL, MIDGARD_DBOBJECT(person));
		return TRUE;
	} 

	return FALSE;	
}

/* Sets user's password. */
gboolean midgard_user_password(MidgardUser *self, const gchar *login,
				const gchar *password, guint hashtype)
{
	g_assert(self != NULL);

	MidgardConnection *mgd = self->dbpriv->mgd;
	MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_OK);

	if(!mgd->mgd->person) {

		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_ACCESS_DENIED);
		return FALSE;
	}

	if(self->priv->person == NULL) {
		g_warning("Can not create user account. Person object is not assigned");
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	}
	
	if(self->dbpriv->guid == NULL) {
		g_warning("Can not change user account. Person object assigned with NULL guid");
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INTERNAL);
		return FALSE;
	}

	/* MidgardUser *user = MIDGARD_USER(mgd->priv->user); */
	MidgardUser *user = self;

	gint anon = 3;
	
	if(!midgard_user_is_root(user))
		anon--;

	if(!midgard_user_is_admin(user))
		anon--;

	if(!midgard_user_is_user(user)) {	
		anon--;
	} else {
		if(!g_str_equal(self->dbpriv->guid, 
					user->dbpriv->guid)) 
			anon--;
	}

	if(anon < 1) {
		MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_ACCESS_DENIED);
		return FALSE;
	}

	/* Login can not be empty */
	if(login == NULL || g_str_equal(login, "")) {
		
		 midgard_set_error(mgd,
				 MGD_GENERIC_ERROR,
				 MGD_ERR_INVALID_NAME,
				 " Can not set empty login");
		 return FALSE;
	}

	/* Password can not be empty, unless we use PAM */
	if((password == NULL || g_str_equal(password, ""))
			&& (hashtype != MIDGARD_USER_HASH_PAM)) {
		
		midgard_set_error(mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_NAME,
				" Can not set empty password");
		return FALSE;
	}

	if(hashtype == MIDGARD_USER_HASH_LEGACY
			|| hashtype == MIDGARD_USER_HASH_LEGACY_PLAIN) {
		
		return __set_legacy_password(self, login, password, hashtype);
	}

	return FALSE;
}

/* Get person */
MgdObject *midgard_user_get_person(MidgardUser *self)
{
	g_assert(self != NULL);

	return self->priv->person;
}


/* GOBJECT ROUTINES */

static void _midgard_user_finalize(GObject *object)
{
	g_assert(object != NULL);
	
	MidgardUser *self = (MidgardUser *) object;

	g_free(self->priv);

	g_free((gchar *) self->dbpriv->guid);
	g_free(self->dbpriv);
}

static void
_midgard_user_get_property (GObject *object, guint property_id,
		GValue *value, GParamSpec *pspec)
{
	MidgardUser *self = (MidgardUser *) object;
	
	switch (property_id) {
		
		case MIDGARD_USER_GUID:
			g_value_set_string(value, self->dbpriv->guid);
			break;

		case MIDGARD_USER_LOGIN:
			g_value_set_string(value, self->priv->login);
			break;

		case MIDGARD_USER_PASS:
			g_value_set_string(value, self->priv->password);
			break;

		case MIDGARD_USER_ACTIVE:
			g_value_set_boolean(value, self->priv->active);	
			break;

		case MIDGARD_USER_SG:
			g_value_set_uint(value, self->dbpriv->sg);
			break;

		case MIDGARD_USER_HASH:
			g_value_set_uint(value, self->priv->hashtype);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(self, property_id, pspec);
			break;
	}
}

static void _midgard_user_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardUserClass *klass = MIDGARD_USER_CLASS (g_class);
	
	gobject_class->finalize = _midgard_user_finalize;
	gobject_class->get_property = _midgard_user_get_property;
	
	MgdSchemaPropertyAttr *prop_attr;
	MgdSchemaTypeAttr *type_attr = _mgd_schema_type_attr_new();

	GParamSpec *pspec;

	 /* GUID */
	pspec = g_param_spec_string ("guid",
			"midgard_person object's guid",
			"",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_USER_GUID,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("guid");
	prop_attr->table = g_strdup(MIDGARD_USER_TABLE);
	prop_attr->tablefield = g_strjoin(".", MIDGARD_USER_TABLE, "guid", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"guid"), prop_attr);

	/* LOGIN */
	pspec = g_param_spec_string ("login",
			"midgard_user's login",
			"",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_USER_LOGIN,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_STRING;
	prop_attr->field = g_strdup("login");
	prop_attr->table = g_strdup(MIDGARD_USER_TABLE);
	prop_attr->tablefield = g_strjoin(".", MIDGARD_USER_TABLE, "login", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"login"), prop_attr);

	 /* PASSWORD */
	pspec = g_param_spec_string ("password",
			"midgard_user's password",
			"",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_USER_PASS,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_STRING;
	prop_attr->field = g_strdup("hashed");
	prop_attr->table = g_strdup(MIDGARD_USER_TABLE);
	prop_attr->tablefield = g_strjoin(".", MIDGARD_USER_TABLE, "hashed", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"password"), prop_attr);
	
	/* ACTIVE */
	pspec = g_param_spec_boolean ("active",
			"midgard_user's active info",
			"",
			FALSE, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_USER_ACTIVE,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_BOOLEAN;
	prop_attr->field = g_strdup("active");
	prop_attr->table = g_strdup(MIDGARD_USER_TABLE);
	prop_attr->tablefield = g_strjoin(".", MIDGARD_USER_TABLE, "active", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"active"), prop_attr);

	/* SITEGROUP */
	pspec = g_param_spec_uint ("sitegroup",
			"midgard_user's sitegroup",
			"",
			0, G_MAXUINT32, 0, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_USER_SG,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_UINT;
	prop_attr->field = g_strdup("sitegroup");
	prop_attr->table = g_strdup(MIDGARD_USER_TABLE);
	prop_attr->tablefield = g_strjoin(".", MIDGARD_USER_TABLE, "sitegroup", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"sitegroup"), prop_attr);

	/* HASHTYPE */
	pspec = g_param_spec_uint ("hashtype",
			"midgard_user's hash type",
			"",
			0, G_MAXUINT32, 0, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_USER_HASH,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->gtype = MGD_TYPE_UINT;
	prop_attr->field = g_strdup("hashtype");
	prop_attr->table = g_strdup(MIDGARD_USER_TABLE);
	prop_attr->tablefield = g_strjoin(".", MIDGARD_USER_TABLE, "hashtype", NULL);
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"hashtype"), prop_attr);

	/* This must be replaced with GDA classes */
	MgdSchemaTypeQuery *_query = _mgd_schema_type_query_new();   
	_query->select_full = "login, hashed AS password, hashtype, active";
	type_attr->query = _query;

	klass->dbpriv = g_new(MidgardDBObjectPrivate, 1);
	klass->dbpriv->storage_data = type_attr;
	klass->dbpriv->storage_data->table = g_strdup(MIDGARD_USER_TABLE);
	klass->dbpriv->set_from_xml_node = NULL;
	//klass->dbpriv->storage_data->tables = (const gchar *)g_strdup(MIDGARD_USER_TABLE);	
}

static void _midgard_user_instance_init(
		GTypeInstance *instance, gpointer g_class)
{
	MidgardUser *self = MIDGARD_USER(instance);
	
	__initialize_user(self);
}

/* Register midgard_user type */
GType midgard_user_get_type(void) 
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardUserClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_user_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardUser),
			0,              /* n_preallocs */
			(GInstanceInitFunc) _midgard_user_instance_init/* instance_init */
		};
		type = g_type_register_static (MIDGARD_TYPE_DBOBJECT,
				"midgard_user",
				&info, 0);
	}
	
	return type;
}
