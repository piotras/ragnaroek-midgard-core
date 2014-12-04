/* 
 * Copyright (C) 2008 Piotr Pokora <piotrek.pokora@gmail.com>
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

#include "midgard/midgard_dbus.h"
#include "src/midgard_core_object.h"
#include "midgard/midgard_replicator.h"

#define MIDGARD_DBUS_SERVICE	"org.midgardproject"
#define MIDGARD_DBUS_IFACE	"org.midgardproject.interface"
#define MIDGARD_DBUS_PATH	"/org/midgardproject"	

/* for auto generated interfaces */
gboolean midgard_dbus_notify(MidgardDbus *self, const char *message, GError **error);
#include "midgard_dbus_interface.h"

static DBusGConnection *system_bus = NULL;

struct _MidgardDbusPrivate{

	DBusGConnection *dbus_connection;
	gchar *path;
	gchar *fullpath;
	gchar *message;
	MidgardConnection *mgd;
};

static void __initialize_system_bus()
{
	if(system_bus != NULL){
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "System D-Bus already exists");
		return;
	}

	GError *error = NULL;

	system_bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);	

	if (!system_bus) {

		g_warning("Couldn't connect to system bus. %s", error->message);
		return;
	}

	g_debug("Create new %s service", MIDGARD_DBUS_SERVICE);

	DBusGProxy *bus_proxy = dbus_g_proxy_new_for_name (system_bus, "org.freedesktop.DBus",
			"/org/freedesktop/DBus",
			"org.freedesktop.DBus");

	guint request_name_result;

	if (!dbus_g_proxy_call (bus_proxy, "RequestName", &error,
				G_TYPE_STRING, MIDGARD_DBUS_SERVICE,
				G_TYPE_UINT, 0,
				G_TYPE_INVALID,
				G_TYPE_UINT, &request_name_result,
				G_TYPE_INVALID)){

		g_warning("Failed to create proxy for midgard service. %s", error->message);
	}

	return;
}

gboolean midgard_dbus_notify(MidgardDbus *self, const char *message, GError **error)
{

#ifndef MIDGARD_DBUS_SUPPORT
return FALSE;
#endif

	if(self->priv->message != NULL)
		g_free(self->priv->message);
	self->priv->message = g_strdup(message);

	g_signal_emit(self, MIDGARD_DBUS_GET_CLASS(self)->signal_notified, 0);

	return TRUE;
}

gchar *__modify_path(MidgardConnection *mgd, const gchar *path)
{
	g_assert(mgd != NULL);

	if(*path != '/') {
		g_warning("Invalid path. Should start with '/'.");
		return NULL;
	}

	if(!mgd->priv->configname) {
		g_warning("No configuration associated with Midgard connection");
		return NULL;
	}

	gchar *new_path = g_strconcat("/", mgd->priv->configname, path, NULL);

	return new_path;
}

/* API */
MidgardDbus *midgard_dbus_new(MidgardConnection *mgd, const gchar *path)
{

#ifndef MIDGARD_DBUS_SUPPORT
return NULL;
#endif

	g_assert(path != NULL);

	gchar *new_path = __modify_path(mgd, path);
	
	if(!new_path)
		return NULL;

	MidgardDbus *self = g_object_new(MIDGARD_TYPE_DBUS, NULL);

	if(self == NULL)
		return NULL;

	if(system_bus == NULL) {
		
		g_warning("System bus not initialized");
		g_object_unref(self);
		return NULL;
	}

	self->priv->path = g_strdup((gchar *)new_path);
	self->priv->mgd = mgd;

	dbus_g_connection_register_g_object(system_bus, new_path, G_OBJECT(self));

	g_free(new_path);

	return self;
}

DBusGProxy *__get_bus_proxy(MidgardConnection *mgd, const gchar *path)
{
	DBusGProxy *remote_object;
	GError *error = NULL;

	g_assert(path != NULL);

	gchar *new_path = __modify_path(mgd, path);

	if(!new_path)
		return NULL;

	DBusGConnection *system_bus = 
		dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);

	if(!system_bus || error) {
		g_debug("System bus error. %s.", error->message);
		g_error_free(error);
		return NULL;
	}
	
	remote_object = dbus_g_proxy_new_for_name_owner(system_bus,
			MIDGARD_DBUS_SERVICE,
			new_path,
			MIDGARD_DBUS_IFACE, &error);

	g_free(new_path);

	if(error) {

		g_error_free(error);
		return NULL;
	}

	return remote_object;
}

void midgard_dbus_send(MidgardConnection *mgd, const gchar *path, const gchar *message)
{

#ifndef MIDGARD_DBUS_SUPPORT
	return;
#endif

	DBusGProxy *remote_object = __get_bus_proxy(mgd, path);

	if(!remote_object) 
		return;
	
	g_debug("%s, %s, %s", 
			dbus_g_proxy_get_bus_name(remote_object),
			dbus_g_proxy_get_path(remote_object),
			dbus_g_proxy_get_interface(remote_object));

	dbus_g_proxy_call_no_reply(remote_object, "notify", 
			G_TYPE_STRING, message, G_TYPE_INVALID);
	
	return;
}

void _midgard_core_dbus_send_serialized_object(MgdObject *object, const gchar *path)
{

#ifndef MIDGARD_DBUS_SUPPORT
	return;
#endif

	g_assert(object != NULL);
	g_assert(path != NULL);

	DBusGProxy *remote_object = __get_bus_proxy(object->mgd->_mgd, path);

	if(!remote_object)
		return;

	gchar *xml = midgard_replicator_serialize(NULL, G_OBJECT(object));

	dbus_g_proxy_call_no_reply(remote_object, "notify",
			G_TYPE_STRING, xml, G_TYPE_INVALID);

	g_free(xml);

	return;
}

const gchar *midgard_dbus_get_message(MidgardDbus *self)
{
	g_assert(self != NULL);

	return self->priv->message;
}

extern const gchar *midgard_dbus_get_name(void)
{
	return NULL;
}

/* GOBJECT ROUTINES */

static void _midgard_dbus_finalize(GObject *object)
{
	g_assert(object != NULL);

	MidgardDbus *self = (MidgardDbus *) object;

	if(self->priv->path != NULL)
		g_free(self->priv->path);
	self->priv->path = NULL;
	
	if(self->priv->fullpath != NULL)
		g_free(self->priv->fullpath);
	self->priv->fullpath = NULL;
	
	if(self->priv->message != NULL)
		g_free(self->priv->message);
	self->priv->message = NULL;

	self->priv->mgd = NULL;

	g_free(self->priv);
	self->priv = NULL;
}

static void _midgard_dbus_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(g_class);
	MidgardDbusClass *klass = MIDGARD_DBUS_CLASS(g_class);
	
	gobject_class->finalize = _midgard_dbus_finalize;

	klass->signal_notified = 
		g_signal_new("notified",
				G_TYPE_FROM_CLASS(g_class),
				G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
				G_STRUCT_OFFSET (MidgardDbusClass, notified),
				NULL, /* accumulator */
				NULL, /* accu_data */
				g_cclosure_marshal_VOID__VOID,
				G_TYPE_NONE,
				0);

	dbus_g_object_type_install_info(MIDGARD_TYPE_DBUS, &dbus_glib_midgard_dbus_object_info);
}

static void _midgard_dbus_instance_init(
		GTypeInstance *instance, gpointer g_class)
{
	MidgardDbus *self = MIDGARD_DBUS(instance);

	 __initialize_system_bus();

	self->priv = g_new(MidgardDbusPrivate, 1);
	self->priv->path = NULL;
	self->priv->fullpath = NULL;
	self->priv->message = NULL;
}

GType midgard_dbus_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardDbusClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_dbus_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardDbus),
			0,              /* n_preallocs */
			(GInstanceInitFunc) _midgard_dbus_instance_init/* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
				"midgard_dbus",
				&info, 0);
	}
	
	return type;
}


