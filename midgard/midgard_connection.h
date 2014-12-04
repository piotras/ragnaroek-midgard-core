/* 
 * Copyright (C) 2006 Piotr Pokora <piotr.pokora@infoglob.pl>
 * Copyright (C) 2006 Jukka Zitting <jukka.zitting@gmail.com>
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

#ifndef MIDGARD_CONNECTION_H
#define MIDGARD_CONNECTION_H

#define GETTEXT_PACKAGE "midgard"
#include <glib/gi18n-lib.h>

#include <midgard/midgard_defs.h>
#include <midgard/midgard_config.h>

/* convention macros */
#define MIDGARD_TYPE_CONNECTION (midgard_connection_get_type())
#define MIDGARD_CONNECTION(object)  \
	        (G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_CONNECTION, MidgardConnection))
#define MIDGARD_CONNECTION_CLASS(klass)  \
	        (G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_CONNECTION, MidgardConnectionClass))
#define MIDGARD_IS_CONNECTION(object)   \
	        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_CONNECTION))
#define MIDGARD_IS_CONNECTION_CLASS(klass) \
	        (G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_CONNECTION))
#define MIDGARD_CONNECTION_GET_CLASS(obj) \
	        (G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_CONNECTION, MidgardConnectionClass))

typedef struct MidgardConnectionClass MidgardConnectionClass;
typedef struct _MidgardConnectionPrivate MidgardConnectionPrivate; 

/**
 * 
 * \defgroup midgard_connection Midgard Connection
 *  
 * The MidgardConnectionClass class structure . 
 */
struct MidgardConnectionClass{
	GObjectClass parent;

	/* class members */
	gboolean (*open) (MidgardConnection *mgd, 
			const char *name, GError *err);
	gboolean (*open_config) (MidgardConnection *mgd, 
			MidgardConfig *config, GError *err);
	void (*close) (MidgardConnection *mgd);
	
	gboolean (*set_sitegroup) (MidgardConnection *mgd, 
			const gchar *guid);
	const gchar *(*get_sitegroup) (MidgardConnection *mgd);

	gboolean (*set_lang) (MidgardConnection *mgd,
			const char *language);
	const gchar *(*get_lang) (MidgardConnection *mgd);

	gboolean (*set_loglevel) (MidgardConnection *mgd, 
			const gchar *levelstring, GLogFunc log_func);
	guint (*get_loglevel) (MidgardConnection *mgd);

	void (*set_loghandler) (MidgardConnection *mgd, 
			guint loghandler);
	guint (*get_loghandler) (MidgardConnection *mgd);

	/* signals */
	void(*error) (GObject *object);
	void(*lang_changed) (GObject *object);
	void(*sitegroup_changed) (GObject *object);
	void(*auth_changed) (GObject *object);
};

/**
 *
 * \ingroup midgard_connection
 *
 * The MidgardConnection object structure.
 */ 
struct MidgardConnection{

	GObject parent;
	gint errnum;
	gchar *errstr;
	GError *err;
	
	/* < private > */
	MidgardConnectionPrivate *priv;
	/* This is reserved */
	/* GdaConnection *connection; */
	/* Legacy, FIXME, remove */
	midgard *mgd;
};

/**
 *
 * \ingroup midgard_connection
 *
 * Returns the MidgardConnection value type. 
 * Registers the type as  a fundamental GType unless already registered.
 *
 */
extern GType midgard_connection_get_type(void);

/**
 *
 * \ingroup midgard_connection 
 * 
 * Creates new MidgardConnection object instance.
 *
 * \return pointer to MidgardConnection object or NULL on failure.
 * 
 * Initializes new instance of MidgardConnection object type. 
 * NULL is returned when object can not be initialized.  
 * MidgardConnectionClass has no properties registered as class members. 
 * Every internal data of MidgardConnection object is accessible with API
 * functions, and is not settable or gettable as property's value.
 * Particular methods should be implemented for language bindings.
 *
 * MidgardConnection objects holds runtime ( or request ) non persistent
 * data like language, sitegroup, authentication type. Persistent data
 * like database name, blobs directory are associated with MidgardConfig object.
 *
 */ 
extern MidgardConnection *midgard_connection_new(void);

/**
 * \ingroup midgard_connection
 *
 * Opens a connection to the named Midgard database. The configuration file
 * for the named database is read from the standard configuration directories
 * and used as the configuration for the created connection.
 *
 * If the named database configuration can not be read or the connection fails,
 * then \c NULL is returned and an error message is written to the optional
 * \c error argument.
 *
 * \param[in] mgd newly initialized MidgardConnection object   
 * \param[in]  name  name of the Midgard database configuration
 * \param[out] error placeholder for an error message, or \c NULL
 * \return Midgard connection, or \c NULL
 * 
 * Additionally this function creates new MidgardConfig object instance, 
 * which is accessible with midgard_connection_config_get function.
 * 
 */
extern gboolean midgard_connection_open(
		MidgardConnection *mgd, const char *name, GError *error);

/* extern gboolean midgard_connection_reopen (MidgardConnection *self); */

/**
 *
 * \ingroup midgard_connection
 * 
 * Opens a Midgard connection with the given configuration. If the connection
 * fails, then \c NULL is returned and an error message is written to the
 * optional \c error argument.
 *
 * \param[in] mgd newly initialized MidgardConnection object
 * \param[in] config Midgard configuration object
 * \param[out] error  placeholder for an error message, or \c NULL
 * \return Midgard connection, or \c NULL
 */
extern gboolean midgard_connection_open_config(
		MidgardConnection *mgd, MidgardConfig *config, GError *error);


/**
 *
 * \ingroup midgard_connection
 *
 * Closes database connection.
 *
 * \param[in] mgd MidgardConnection object
 *
 * This function closes database connection , destroys MidgardConnection object
 * and frees its memory. It may be replaced by g_object_unref, with one exception.
 * 
 * When MidgardConnection object is initialized with midgard_connection_open
 * function then MidgardConfig object is also destroyed and its memory is freed.
 * 
 * Caller is responsible to free MidgardConfig object when is used with 
 * MidgardConnection and connection object was initialized with 
 * midgard_connection_open_config function.
 *
 *  
 */ 
extern void midgard_connection_close(MidgardConnection *mgd);


/**
 * \ingroup midgard_connection
 *
 * Sets sitegroup's value for the given MidgardConnection.
 *
 * \param mgd MidgardConnection.
 * \param[in] guid , name which identifies sitegroup
 * \return TRUE when connection's sitegroup was set, FALSE otherwise. 
 * 
 */ 
extern gboolean midgard_connection_set_sitegroup(MidgardConnection *mgd, const gchar *name);

/**
 *
 * \ingroup midgard_connection
 *
 * Return the sitegroup of the given MidgardConnection.
 * 
 * Returned string is a name of a sitegroup which is used for a connection.
 * 
 * \param[in] mgd MidgardConnection for which sitegroup is returned
 * \return sitegroup's name, or NULL if not sitegroup is set ( default SG0 )
 * 
 * Midgard sitegroups concept:
 * http://www.midgard-project.org/documentation/concepts-sitegroups
 * 
 */ 
extern const gchar *midgard_connection_get_sitegroup(MidgardConnection *mgd);

/**
 * \ingroup midgard_connection
 * 
 * Sets the language of the given Midgard connection. 
 * The connection language affects all MultiLang database accesses 
 * as described in the  Midgard MultiLang documentation.
 * 
 * The given \c language parameter is the two-letter language code
 * stored in the  \c language table in the Midgard database. 
 * This method returns \c FALSE if the given language is not found.
 * 
 * \param connection the Midgard connection whose language is changed
 * \param[in] language the new language code
 * 
 * \return \c TRUE if the connection language was changed, \c FALSE otherwise
 * \see http://www.midgard-project.org/documentation/midgard-and-multilingual-content/
 * 
 */
extern gboolean midgard_connection_set_lang(
		MidgardConnection *mgd, const char *language);

/**
 *
 * \ingroup midgard_connection
 *
 * Returns language of the given MidgardConnection.
 *
 * \param[in] mgd MidgardConnection for which language is returned.
 * 
 * \return language's code string or NULL if no language is set for 
 * MidgardConnection
 *
 * Returned language value is the two-letter language code.
 *
 */
extern const gchar *midgard_connection_get_lang(MidgardConnection *mgd);

extern gboolean midgard_connection_set_default_lang(
		MidgardConnection *self, const gchar *language);

extern const gchar *midgard_connection_get_default_lang(MidgardConnection *self);

/**
 *
 * \ingroup midgard_connection
 *
 * Sets log level of the given MidgardConnection.
 * 
 * \param mgd MidgardConnection object
 * \param[in] level Loglevel string
 *
 * Overwrites internal MidgardConnection's log level defined in configuration file. 
 * By default MidgardConnection holds loglevel which is associated with and duplicated 
 * from MidgardConfig. 
 * MidgardConfig object's log level isn't changed by this function
 */
extern gboolean midgard_connection_set_loglevel(MidgardConnection *mgd, const gchar *level, GLogFunc log_func);

/**
 *
 * \ingroup midgard_connection
 *
 * Get loglevel's value.
 *
 * \param[in] MidgardConnection object
 * \return unsigned integer 
 *
 * Returned value is an unsigned integer flag specified by GLogLevelFlags.
 * 
 */ 
extern guint midgard_connection_get_loglevel(MidgardConnection *mgd);

/**
 *
 * \ingroup midgard_connection
 *
 * Sets  loghandler of the given MidgardConnection.
 *
 * \param[in] MidgardConnection object
 * \param[in] loghandler id
 *
 * Sets internal loghandler id associated with G_LOG_DOMAIN and loglevel.
 * Caller is responsible to remove loghandler using g_log_remove_handler
 * when new loglevel for G_LOG_DOMAIN is set.
 * 
 */ 
extern void midgard_connection_set_loghandler(
		MidgardConnection *mgd, guint loghandler);

/**
 *
 * \ingroup midgard_connection
 *
 * Get loghandler associated with MidgardConnection.
 * 
 * \param[in] mgd MidgardConnection object
 * \return unsigned integer
 * 
 * Returned unsigned integer value is associated with G_LOG_DOMAIN and 
 * MidgardConnection's loglevel currently used by application.
 * 
 */
extern guint midgard_connection_get_loghandler(MidgardConnection *mgd);

/**
 * \ingroup midgard_connection 
 *
 * Get last error id.
 *
 * \param mgd, MidgardConnection instance
 * \return error, error id
 *
 * Error id may be one of set by #midgard_error.
 */ 
extern gint midgard_connection_get_error(MidgardConnection *mgd);

extern void midgard_connection_set_error(MidgardConnection *mgd, gint errcode); 

/**
 * \ingroup midgard_connection
 *
 * Get last error string.
 *
 * \param[in] mgd, MidgardConnection instance
 * \return error, error string
 *
 * Error string may be one set by #midgard_error.
 */
extern const gchar *midgard_connection_get_error_string(MidgardConnection *mgd);

/**
 * \ingroup midgard_connection
 *
 * Get logged in user
 *
 * \param[in] mgd, MidgardConnection instance
 * \return user, MidgardUser instance or NULL 
 *
 * NULL is explicitly returned if there's no midgard_user logged in 
 * for the given MidgardConnection.
 * Use #midgard_user method if you need midgard_person associated with user.
 * 
 */ 
extern MidgardUser *midgard_connection_get_user(MidgardConnection *mgd);

extern void                    midgard_connection_enable_quota                 (MidgardConnection *self, gboolean toggle);
extern void                    midgard_connection_enable_replication           (MidgardConnection *self, gboolean toggle);
extern void                    midgard_connection_enable_dbus                  (MidgardConnection *self, gboolean toggle);
extern gboolean                midgard_connection_is_enabled_quota             (MidgardConnection *self);
extern gboolean                midgard_connection_is_enabled_replication       (MidgardConnection *self);
extern gboolean                midgard_connection_is_enabled_dbus              (MidgardConnection *self);

extern void midgard_connection_unref_implicit_user(MidgardConnection *mgd);

#endif /* MIDGARD_CONNNECTION_H */
