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

#include "midgard/midgard_connection.h"
#include "midgard/midgard_error.h"
#include "schema.h"
#include "midgard/midgard_legacy.h"
#include "midgard_core_object.h"
#include "fmt_russian.h"
#include "midgard/midgard_user.h"

static void _midgard_connection_finalize(GObject *object)
{
	g_assert(object != NULL);
	MidgardConnection *self = (MidgardConnection *) object;

	if(self->errstr != NULL)
		g_free(self->errstr);

	self->errstr = NULL;

	self->priv->loghandler = 0;
	if(self->priv->sitegroup)
		g_free((gchar *) self->priv->sitegroup);
	if(self->priv->lang)
		g_free((gchar *) self->priv->lang);
	g_clear_error(&self->err);

	if(self->priv->configname)
		g_free(self->priv->configname);
	self->priv->configname = NULL;

	if(self->priv->free_config) {
		g_object_unref(self->priv->config);
		self->priv->config = NULL;
		self->priv->free_config = FALSE;
	}

	midgard_connection_unref_implicit_user(self);

	if (!self->priv->is_copy) {
		if(self->priv->sg_ids)
			id_list_free(self->priv->sg_ids);
		self->priv->sg_ids = NULL;
	}
	
	/* Free legacy memory pool, as we do not need to keep it filled all the time */
	if (self->mgd) {
		if(self->mgd->tmp != NULL) {
			mgd_free_pool(self->mgd->tmp);
			self->mgd->tmp = mgd_alloc_pool();
		}
		if(self->mgd->pool != NULL) {
			mgd_free_pool(self->mgd->pool);
			self->mgd->pool = mgd_alloc_pool();
		}
	}

	g_free(self->priv);

	self->priv = NULL;
}

static void _midgard_connection_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardConnectionClass *klass = MIDGARD_CONNECTION_CLASS (g_class);

	gobject_class->finalize = _midgard_connection_finalize;
	klass->open = midgard_connection_open;
	klass->open_config = midgard_connection_open_config;
	klass->close = midgard_connection_close;
	klass->set_sitegroup = midgard_connection_set_sitegroup;
	klass->get_sitegroup = midgard_connection_get_sitegroup;
	klass->set_lang = midgard_connection_set_lang; 
	klass->get_lang = midgard_connection_get_lang;
	klass->set_loglevel = midgard_connection_set_loglevel;
	klass->get_loglevel = midgard_connection_get_loglevel;
	klass->set_loghandler = midgard_connection_set_loghandler;
	klass->get_loghandler = midgard_connection_get_loghandler;

	 /* signals */
	g_signal_new("error",
			G_TYPE_FROM_CLASS(g_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			G_STRUCT_OFFSET (MidgardConnectionClass, error),
			NULL, /* accumulator */
			NULL, /* accu_data */
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);

	g_signal_new("sitegroup-changed",
			G_TYPE_FROM_CLASS(g_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			G_STRUCT_OFFSET (MidgardConnectionClass, sitegroup_changed),
			NULL, /* accumulator */
			NULL, /* accu_data */
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	
	g_signal_new("lang-changed",
			G_TYPE_FROM_CLASS(g_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			G_STRUCT_OFFSET (MidgardConnectionClass, lang_changed),
			NULL, /* accumulator */
			NULL, /* accu_data */
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
	
	g_signal_new("auth-changed",
			G_TYPE_FROM_CLASS(g_class),
			G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
			G_STRUCT_OFFSET (MidgardConnectionClass, auth_changed),
			NULL, /* accumulator */
			NULL, /* accu_data */
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0);
}

static void _midgard_connection_instance_init(
		GTypeInstance *instance, gpointer g_class)
{
	MidgardConnection *self = (MidgardConnection *) instance;
	self->priv = g_new(MidgardConnectionPrivate, 1);

	self->priv->config = NULL;
	self->priv->free_config = FALSE;
	self->priv->loghandler = 0;
	self->priv->loglevel = 0; 
	self->priv->sitegroup = NULL;
	self->priv->lang = NULL;
	self->priv->lang_id = 0;
	self->priv->default_lang = NULL;
	self->priv->default_lang_id = 0;
	self->priv->configname = 0;
	self->priv->user = NULL;
	self->priv->implicit_user = FALSE;
	self->priv->is_copy = FALSE;

	self->priv->enable_replication = TRUE;
	self->priv->enable_dbus = TRUE;
	self->priv->enable_quota = TRUE;

	/* Sitegroup cache */
	self->priv->sg_ids = NULL;
	
	self->err = MGD_ERR_OK;
	self->errstr = g_strdup("MGD_ERR_OK");
	self->mgd = NULL;
}

/* Registers the type as  a fundamental GType unless already registered. */
GType midgard_connection_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardConnectionClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_connection_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardConnection),
			0,              /* n_preallocs */
			(GInstanceInitFunc) _midgard_connection_instance_init /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
				"midgard_connection",
				&info, 0);
	}
	return type;
}

/* Creates new MidgardConnection object instance. */ 
MidgardConnection *midgard_connection_new(void){
	
	MidgardConnection *self = 
		g_object_new(MIDGARD_TYPE_CONNECTION, NULL);
	
	return self;
}

gboolean __midgard_connection_open(
		MidgardConnection *mgd, MidgardConfig *config, GError *err)
{
	gchar *host, *dbname, *dbuser, *dbpass, *schemafile, *loglevel;
	g_object_get(G_OBJECT(mgd->priv->config),
			"host", &host,
			"database", &dbname,
			"dbuser", &dbuser,
			"dbpass", &dbpass,
			"schema", &schemafile,
			"loglevel", &loglevel,
			NULL);
	
	(void) mgd_parser_create("latin1", "latin1", 1);
	(void) mgd_parser_init_raw("russian", "KOI8-R");
	(void) mgd_parser_init_raw("utf-8", "UTF-8");

	/* FIXME when we switch to libgda usage */
	midgard *legacy_mgd = mgd_connect(
			host,
			dbname,
			dbuser,
			dbpass);

	if(!legacy_mgd) {
		midgard_set_error(mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_NOT_CONNECTED,
				"Database %s@%s.", dbname, host);
		if(err)
			g_warning("%s", err->message);
		g_clear_error(&err); 
		return FALSE;
	}

	if(g_type_from_name("midgard_article") <= 0) {
		MidgardSchema *schema = g_object_new(MIDGARD_TYPE_SCHEMA, NULL);
		midgard_schema_init(schema);
		midgard_schema_read_dir(schema, MIDGARD_LSCHEMA_DIR);
		mgd->priv->schema = schema;
	}
	
	midgard_connection_set_loglevel(mgd, loglevel, NULL);
		
	/* Set legacy midgard connection handler's data*/
	gchar *mgduser = NULL, *mgdpass = NULL, *blobdir;
	g_object_get(mgd->priv->config,
			"midgardusername", &mgduser,
			"midgardpassword", &mgdpass,
			"blobdir", &blobdir,
			NULL);

	legacy_mgd->blobdir = g_strdup(blobdir);
	legacy_mgd->loglevel = midgard_connection_get_loglevel(mgd);
	mgd->mgd = legacy_mgd;
	
	/* Acceptable workaround */
	if (G_IS_OBJECT(legacy_mgd->_mgd))
		g_object_unref(legacy_mgd->_mgd);
	legacy_mgd->_mgd = mgd;
	mgd_legacy_load_sitegroup_cache(legacy_mgd);
	
	/* This is commented:
	 1. I am not sure if this is application specific
	 	-  requires special config object  management
		( like check for root id when getting some secret data )
	 2. Core's loghandler should be initialized before to avoid 
	 unconditional debug logs before we switch to application's log handler
	 and log function
	*/ 
	/* if((mgduser != NULL) && (mgdpass != NULL))
		mgd_auth(legacy_mgd, mgduser, mgdpass, 1);
	*/
	g_free(host);
	g_free(dbname);
	g_free(dbuser);
	g_free(dbpass);
	g_free(schemafile);
	g_free(loglevel);
	g_free(mgduser);
	g_free(mgdpass);
	g_free(blobdir);

	mgd->priv->configname = g_strdup(config->private->configname);

	return TRUE;
}

/* Opens a connection to the named Midgard database. */
gboolean midgard_connection_open(
		MidgardConnection *mgd, const char *name, GError *err)
{	
	g_assert(mgd != NULL);
	g_assert (name != NULL);
	g_assert(err == NULL);
	
	if(mgd->priv->config != NULL){
		midgard_set_error(mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_USER_DATA,
				"MidgardConfig already associated with "
				"MidgardConnection");
		g_warning("%s", err->message);
		return FALSE;
	}
	
	MidgardConfig *config = midgard_config_new();
	if(!midgard_config_read_file(config, name, FALSE))
		return FALSE;
	
	mgd->priv->config = config;	
	mgd->priv->free_config = TRUE;

	if(!__midgard_connection_open(mgd, config, err))
		return FALSE;

	return TRUE;	
}

/* Opens a Midgard connection with the given configuration. */
gboolean midgard_connection_open_config(
		MidgardConnection *mgd, MidgardConfig *config, GError *err)
{
	g_assert(mgd != NULL);	
	g_assert(config != NULL);
	g_assert(err == NULL);

	mgd->priv->config = config;
	
	if(!__midgard_connection_open(mgd, config, err))
		return FALSE;

	return TRUE;
}

/* Closes database connection. */ 
void midgard_connection_close(MidgardConnection *mgd)
{
	g_assert(mgd != NULL);
	
	/* DO NOT unref MidgardConnection. It's done by mgd_close. */
	if(mgd->mgd)
		mgd_close(mgd->mgd);	
	
	return;
}

/* Sets sitegroup's value for the given MidgardConnection. */ 
gboolean midgard_connection_set_sitegroup(
		MidgardConnection *self, const gchar *name)
{
	g_assert(self != NULL);

	if(!name || *name == '\0') {

		g_debug("Setting sitegroup 0");
		self->priv->sitegroup_id = 0;
		self->priv->sitegroup = NULL;
		g_signal_emit_by_name (self, "sitegroup-changed");

		return TRUE;
	}	

	if (self->priv->sg_ids == NULL)
		return FALSE;

	guint id = id_list_lookup(self->priv->sg_ids, name);

	if(id == 0)
		return FALSE;
	
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
			"Setting sitegroup '%s' (id: %d)",
			name, id);
	
	self->priv->sitegroup_id = id;
	g_free((gchar *)self->priv->sitegroup);
	self->priv->sitegroup = (const gchar *)g_strdup(name);

	/* Legacy... */
	self->mgd->current_user->sitegroup = id;

	g_signal_emit_by_name (self, "sitegroup-changed");

	return TRUE;
}

/* Return the sitegroup of the given MidgardConnection. */ 
const gchar *midgard_connection_get_sitegroup(
		MidgardConnection *mgd)
{
	g_assert(mgd != NULL);
	/* g_assert(mgd->priv->sitegroup != NULL); */

	return mgd->priv->sitegroup;
}

/* Sets the language of the given Midgard connection. */
gboolean midgard_connection_set_lang(
		MidgardConnection *mgd, const char *language)
{
	g_assert(mgd != NULL);
	g_assert(language != NULL);
	
	if(language == NULL || *language == '\0') {

		/* set default language */
		if(mgd->priv->default_lang != NULL)
			g_free((gchar *)mgd->priv->default_lang);
		mgd->priv->default_lang = (const gchar *)g_strdup("");
		mgd->priv->default_lang_id = 0;

		/* set fallback language */
		if(mgd->priv->lang != NULL)
			g_free((gchar *)mgd->priv->lang);
		mgd->priv->lang = (const gchar *)g_strdup("");
		mgd->priv->lang_id = 0;
		mgd->mgd->lang = 0;

		g_signal_emit_by_name(mgd, "lang-changed");
		return TRUE;
	}

	guint lang_id = 0;
	
	midgard_res *res =
		mgd_query(mgd->mgd,
				"SELECT id FROM "
				"midgard_language "
				"WHERE code=$q",
				language);
	if(res){
		if(mgd_fetch(res)) {
			
			lang_id = atoi(mgd_colvalue(res, 0));
			
			mgd_release(res);
			
			if(mgd->priv->lang != NULL)
				g_free((gchar *)mgd->priv->lang);
			mgd->priv->lang = (const gchar *)g_strdup(language);
			mgd->priv->lang_id = lang_id;
			mgd->mgd->lang = lang_id;
			
			g_signal_emit_by_name(mgd, "lang-changed");
			
			return TRUE;
		}

		mgd_release(res);
	}
	
	return FALSE;
}

/* Returns language of the given MidgardConnection. */
const gchar *midgard_connection_get_lang(MidgardConnection *mgd)
{
	g_assert(mgd != NULL);

	if (mgd->priv->lang_id == 0)
		return NULL;

	midgard_res *res =
		mgd_query(mgd->mgd,
				"SELECT code FROM "
				"midgard_language "
				"WHERE id=$d",
				mgd->priv->lang_id);

	gchar *lang = NULL;

	if(res){

		if(mgd_fetch(res)) {
			
			lang = g_strdup((gchar *)mgd_colvalue(res, 0));
		}

		mgd_release(res);
	}

	if (lang == NULL)
		return NULL;

	mgd->priv->lang = (const gchar *) lang;
	return mgd->priv->lang;
}

gboolean midgard_connection_set_default_lang(MidgardConnection *mgd, const gchar *language)
{
	g_assert(mgd != NULL);
	g_assert(language != NULL);

	if(language == NULL || *language == '\0') {

		/* set default language */
		if(mgd->priv->default_lang != NULL)
			g_free((gchar *)mgd->priv->default_lang);
		mgd->priv->default_lang = (const gchar *)g_strdup("");
		mgd->priv->default_lang_id = 0;

		/* set fallback language */
		if(mgd->priv->lang != NULL)
			g_free((gchar *)mgd->priv->lang);
		mgd->priv->lang = (const gchar *)g_strdup("");
		mgd->priv->lang_id = 0;

		if (mgd->mgd) {
			mgd->mgd->default_lang = 0;
			mgd->mgd->lang = 0;
		}

		g_signal_emit_by_name(mgd, "lang-changed");
		return TRUE;
	}

	guint lang_id = 0;
	
	midgard_res *res =
		mgd_query(mgd->mgd,
				"SELECT id FROM "
				"midgard_language "
				"WHERE code=$q",
				language);
	if(res){

		if(mgd_fetch(res)) {
			
			lang_id = atoi(mgd_colvalue(res, 0));
			
			mgd_release(res);
		
			/* set default language */
			if(mgd->priv->default_lang != NULL)
				g_free((gchar *)mgd->priv->default_lang);
			mgd->priv->default_lang = (const gchar *)g_strdup(language);
			mgd->priv->default_lang_id = lang_id;

			/* set fallback language */
			if(mgd->priv->lang != NULL)
				g_free((gchar *)mgd->priv->lang);
			mgd->priv->lang = (const gchar *)g_strdup(language);
			mgd->priv->lang_id = lang_id;

			if (mgd->mgd) {
				mgd->mgd->default_lang = lang_id;
				mgd->mgd->lang = lang_id;
			}

			g_signal_emit_by_name(mgd, "lang-changed");
			
			return TRUE;
		}

		mgd_release(res);
	}
	
	return FALSE;
}

const gchar *midgard_connection_get_default_lang(MidgardConnection *self)
{
	g_assert(self != NULL);

	if (self->priv->default_lang_id == 0)
		return NULL;

	midgard_res *res =
		mgd_query(self->mgd,
				"SELECT code FROM "
				"midgard_language "
				"WHERE id=$d",
				self->priv->default_lang_id);

	gchar *lang = NULL;

	if(res){

		if(mgd_fetch(res)) {
			
			lang = (gchar *)mgd_colvalue(res, 0);
		}

		mgd_release(res);
	}

	self->priv->default_lang = (const gchar *) lang;
	return self->priv->default_lang;
}

/* Sets log level of the given MidgardConnection. */
gboolean midgard_connection_set_loglevel(
		MidgardConnection *mgd, const gchar *levelstring, GLogFunc log_func)
{
	g_assert(mgd != NULL);

	GLogFunc _func = log_func;
	if(_func == NULL)
		_func = midgard_error_default_log;
	
	guint loghandler = midgard_connection_get_loghandler(mgd);
	if(loghandler)
		g_log_remove_handler(G_LOG_DOMAIN, loghandler);

	guint level = 0, mask = 0;
	guint len = strlen(levelstring);
	gchar *newlevel = g_ascii_strdown(levelstring, len);

	if(g_str_equal(newlevel, "error")) 
		level = G_LOG_LEVEL_ERROR;
	if(g_str_equal(newlevel, "critical"))
		level = G_LOG_LEVEL_CRITICAL;
	if(g_str_equal(newlevel, "warn"))
		level = G_LOG_LEVEL_WARNING;
	if(g_str_equal(newlevel, "warning"))
		level = G_LOG_LEVEL_WARNING;
	if(g_str_equal(newlevel, "message"))	
		level = G_LOG_LEVEL_MESSAGE;
	if(g_str_equal(newlevel, "info"))
		level = G_LOG_LEVEL_INFO;
	if(g_str_equal(newlevel, "debug"))
		level = G_LOG_LEVEL_DEBUG;
	
	switch(level){

		case G_LOG_LEVEL_ERROR:
		case G_LOG_LEVEL_CRITICAL:
		case G_LOG_LEVEL_WARNING:
		case G_LOG_LEVEL_MESSAGE:
		case G_LOG_LEVEL_INFO:
		case G_LOG_LEVEL_DEBUG:
			for (; level && level > G_LOG_FLAG_FATAL; level >>= 1) {
				mask |= level;
			}
			mgd->priv->loglevel = mask;
			break;

		default:
			midgard_set_error(mgd,
					MGD_GENERIC_ERROR,
					MGD_ERR_INTERNAL,
					" Unknown log level (%s) in configuration.", 
					newlevel);
			g_warning("%s", mgd->err->message);
			g_free(newlevel);

			return FALSE;
	}

	g_free(newlevel);

	loghandler = g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK,
			_func, (gpointer)mgd);

	if(loghandler == 0)
		return FALSE;
	
	midgard_connection_set_loghandler(mgd, loghandler);
	return TRUE;
}

/* Get loglevel's value. */
guint midgard_connection_get_loglevel(MidgardConnection *mgd)
{
	g_assert(mgd != NULL);

	return mgd->priv->loglevel;
}

/* Sets loghandler of the given MidgardConnection. */
void midgard_connection_set_loghandler(
		MidgardConnection *mgd, guint loghandler){

	g_assert(mgd != NULL);

	mgd->priv->loghandler = loghandler;

	return;
}

/* Get loghandler associated with MidgardConnection. */
guint midgard_connection_get_loghandler(MidgardConnection *mgd)
{
	g_assert(mgd != NULL);
	
	return mgd->priv->loghandler;
}

/* Get last error id */
gint midgard_connection_get_error(MidgardConnection *mgd)
{
	g_assert(mgd != NULL);

	return mgd->errnum;
}

void midgard_connection_set_error(MidgardConnection *mgd, gint errcode)
{
	g_assert(mgd != NULL);
	g_assert(errcode < 1);

	MIDGARD_ERRNO_SET(mgd->mgd, errcode);
}

/* Get last error string */
const gchar *midgard_connection_get_error_string(MidgardConnection *mgd)
{
	g_return_val_if_fail(mgd != NULL, NULL);

	return (const gchar *)mgd->errstr;
}

/* Get logged in user */
MidgardUser *midgard_connection_get_user(MidgardConnection *mgd)
{
	g_return_val_if_fail(mgd != NULL, NULL);

	if(mgd->priv->user == NULL) {
		return NULL;
	}

	if(!G_IS_OBJECT(mgd->priv->user)){
		return NULL;
	}

	return MIDGARD_USER(mgd->priv->user);
}

void
midgard_connection_enable_quota (MidgardConnection *self, gboolean toggle)
{
	g_return_if_fail (self != NULL);
	self->priv->enable_quota = toggle;
}

void
midgard_connection_enable_replication (MidgardConnection *self, gboolean toggle)
{
	g_return_if_fail (self != NULL);
	self->priv->enable_replication = toggle;
}

void
midgard_connection_enable_dbus (MidgardConnection *self, gboolean toggle)
{
	g_return_if_fail (self != NULL);
	self->priv->enable_dbus = toggle;
}

gboolean
midgard_connection_is_enabled_quota (MidgardConnection *self)
{
	g_return_val_if_fail (self != NULL, FALSE);
	return self->priv->enable_quota;
}

gboolean
midgard_connection_is_enabled_replication (MidgardConnection *self)
{
	g_return_val_if_fail (self != NULL, FALSE);
	return self->priv->enable_replication;
}

gboolean
midgard_connection_is_enabled_dbus (MidgardConnection *self)
{
	g_return_val_if_fail (self != NULL, FALSE);
	return self->priv->enable_dbus;
}

gboolean
midgard_connection_reopen (MidgardConnection *self, guint n_try, guint sleep_seconds)
{
	g_assert (self != NULL);
	MidgardConfig *config = self->priv->config;

	return FALSE;
	/* gboolean reopen = mgd_reopen (self->mgd, 
			(const char *) config->host, 
			(const char *) config->database,
			(const char *) config->dbuser, 
			(const char *) config->dbpass,
			n_try, sleep_seconds);

	return reopen; */
}

/* This is HACK! It's required for legacy mgd_auth_su. */
void midgard_connection_unref_implicit_user(MidgardConnection *mgd)
{
	g_assert(mgd != NULL);

	if(!mgd->priv->implicit_user)
		return;

	if(mgd->priv->user == NULL)
		return;

	g_object_unref(mgd->priv->user);
	mgd->priv->user = NULL;

	mgd->priv->implicit_user = FALSE;
}
