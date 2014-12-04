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

#ifndef MIDGARD_CONFIG_H
#define MIDGARD_CONFIG_H

#include <midgard/midgard_type.h>
#include "midgard/midgard_defs.h"

/**
 * \defgroup midgard_config Midgard Config
 *
 * MidgardConfig class ( registered as midgard_config ) holds Midgard unified 
 * configuration file's data. 
 * References:
 * 	- http://www.midgard-project.org/documentation/unified-configuration/
 */	

/* convention macros */
#define MIDGARD_TYPE_CONFIG (midgard_config_get_type())
#define MIDGARD_CONFIG(object)  \
	(G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_CONFIG, MidgardConfig))
#define MIDGARD_CONFIG_CLASS(klass)  \
	(G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_CONFIG, MidgardConfigClass))
#define MIDGARD_IS_CONFIG(object)   \
	(G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_CONFIG))
#define MIDGARD_IS_CONFIG_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_CONFIG))
#define MIDGARD_CONFIG_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_CONFIG, MidgardConfigClass))

typedef struct MidgardConfig MidgardConfig;
typedef struct MidgardConfigClass MidgardConfigClass;
typedef struct _MidgardConfigPrivate MidgardConfigPrivate;

/**
 * \ingroup midgard_config
 *
 * The opaque Midgard Config type.
 */ 
struct MidgardConfig{
	GObject parent;
	
	gchar *dbtype;
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
	GIOChannel *log_channel;
	gchar *mgdusername;
	gchar *mgdpassword;
	gboolean testunit;
	guint loghandler;
	guint authtype;
	gchar *pamfile;
	gboolean gdathreads;

	MidgardConfigPrivate *private;
};

/**
 * \ingroup midgard_config
 * 
 * The opaque Midgard Config Class type.
 */
struct MidgardConfigClass{
	GObjectClass parent;
	
	/* class members */
	gboolean (*read_config) (MidgardConfig *self, 
				const gchar *filename, gboolean user);
	gboolean (*save_config) (MidgardConfig *self,
				const gchar *name, gboolean user);
	gchar (**list_files) (gboolean user);
	gboolean (*create_midgard_tables)(MidgardConfig *self,
					MidgardConnection *mgd);
	gboolean (*create_class_table) (MidgardConfig *self,
					MidgardObjectClass *klass, 
					MidgardConnection *mgd);
};

/**
 * \ingroup midgard_config
 *
 * Database provider type enum.
 */
typedef enum {
	MIDGARD_DB_TYPE_MYSQL = 1,
	MIDGARD_DB_TYPE_POSTGRES,
	MIDGARD_DB_TYPE_FREETDS,
	MIDGARD_DB_TYPE_SQLITE,
	MIDGARD_DB_TYPE_ODBC,
	MIDGARD_DB_TYPE_ORACLE
} MidgardDBType;

/**
 * \ingroup midgard_config
 *
 * Returns the MidgardConfig value type. 
 * Registers the type as a fundamental GType unless already registered.
 */
extern GType midgard_config_get_type(void);

/**
 * \ingroup midgard_config
 *
 * Creates new Midgard Config object instance.
 *
 * \return pointer to MidgardConfig object or NULL on failure.
 *
 * Initializes new instance of MidgardConfig object type. 
 * NULL is returned when object can not be initialized.
 *
 * Properties of Midgard Config object:
 *
 * - dbtype 
 * - host
 * - dbname
 * - dbuser
 * - dbpass
 * - blobdir
 * - logfilename
 * - schema
 * - loglevel
 * - tablecreate
 * - tableupdate
 * - testunit
 * - loghandler
 */
extern MidgardConfig *midgard_config_new(void);

/**
 * \ingroup midgard_config 
 *
 * Reads configuration file for the given Midgard Config object.
 *
 * \param object MidgardConfig object instance
 * \param filename 
 * \param user, boolean switch for system or user's config files
 *
 * \return TRUE when file has been read , FALSE otherwise.
 * 
 * This method reads configuration file passed as method parameter
 * and sets Midgard Config object's properties.
 * Such initialized Midgard Config instance may be reused among midgard-core, 
 * midgard-apache module and midgard-php extension for example, without any 
 * need to re-read configuration file and without any need to re-initalize
 * Midgard Config object instance. 
 *
 * Set TRUE as user boolean value to read files from user's home directory.
 *
 */ 
extern gboolean midgard_config_read_file(MidgardConfig *object, 
				const gchar *filename, gboolean user);

/**
 * \ingroup midgard_config
 *
 * List all available configuration files.
 *
 * \param user, boolean switch for system or user's config files
 *
 * If \c user value is set to TRUE, all available files from 
 * ~/.midgard/conf.d will be listed. 
 * Only system files ( usually from /etc/midgard/conf.d ) will be listed
 * if user value is set to FALSE.
 *
 * \return newly allocated and NULL terminated array of file names.
 *
 * Returned array should be freed when no longer needed.
 *
 */
extern gchar **midgard_config_list_files(gboolean user);

/**
 * \ingroup midgard_config 
 *
 * Saves configuration file for the given MidgardConfig.
 *
 * \param self, MidgardConfig instance
 * \param name, configuration name 
 * \param user, whether configuration file should be saved in $HOME directory 
 *
 * This method saves configuration file with the given name.
 * If third \c user parameter is set to TRUE, then configuration file will 
 * be saved in ~/.midgard/conf.d directory.
 *
 * User's conf.d directory will be created if doesn't exist.
 * FALSE is returned ( with propper warning message ) if system wide 
 * directory doesn't exist or file can not be saved.
 *
 */
extern gboolean midgard_config_save_file(MidgardConfig *self,
				const gchar *name, gboolean user);
/**
 * \ingroup midgard_config
 *
 * Creates basic Midgard database tables.
 *
 * \param self, MidgardConfig instance
 * \param mgd, MidgardConnection handler
 * \return TRUE if tables has been created, FALSE otherwise
 *
 * Tables created by this method:
 * - repligard
 * - sitegroup
 * - person
 * - member
 *
 */ 
extern gboolean midgard_config_create_midgard_tables(MidgardConfig *self,
				MidgardConnection *mgd);
/**
 * \ingroup midgard_config
 *
 * Creates table for the given MidgardObjectClass
 *
 * \param self, MidgardConfig instance
 * \param klass, a pointer to MidgardObjectClass
 * \param mgd, MidgardConnection pointer
 * \return TRUE is table has been created, FALSE otherwise
 *
 * This method creates table defined in MgdSchema xml file for the given 
 * class. Also all metadata columns are created.
 * 
 * Indexes are created if:
 * - property is a link type
 * - property is linked to another property
 * 
 * Auto increment field is created if property is defined as primaryproperty,
 * and it's integer ( or unsigned one ) type. In such case , field is also 
 * a primary field.
 *
 * \c self parameter may be NULL if the purpose is to call static method
 *
 */
extern gboolean midgard_config_create_class_table(MidgardConfig *self,
				MidgardObjectClass *klass, MidgardConnection *mgd);

extern gboolean midgard_config_update_class_table(MidgardConfig *self,
                                MidgardObjectClass *klass, MidgardConnection *mgd);

extern gboolean midgard_config_class_table_exists(MidgardConfig *self,
                                MidgardObjectClass *klass, MidgardConnection *mgd);

/*
 * This is a part of public API , howevere it shouldn't be used in any 
 * application unless you really know what you are doing.
 */ 
extern MidgardConfig *midgard_config_struct_new(void);
extern MidgardConfig *midgard_config_struct2gobject(MidgardConfig *config);
extern void midgard_config_struct_free(MidgardConfig *config);

#endif /* MIDGARD_CONFIG_H */
