/* Midgard config files routine functions 
 *    
 * Copyright (C) 2005, 2006, 2007 Piotr Pokora <piotrek.pokora@gmail.com>
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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include "midgard/midgard_object.h"
#include "midgard/midgard_config.h"
#include "midgard/midgard_datatypes.h"
#include "src/midgard_core_object.h"
#include "src/midgard_core_query.h"
#include "midgard/midgard_defs.h"
#include "midgard_core_config.h"
#include "midgard/query.h"

/* Properties */
enum {
	MIDGARD_CONFIG_DBTYPE = 1,
	MIDGARD_CONFIG_DBNAME,
	MIDGARD_CONFIG_DBUSER, 
	MIDGARD_CONFIG_DBPASS, 
	MIDGARD_CONFIG_HOST,
	MIDGARD_CONFIG_BLOBDIR,
	MIDGARD_CONFIG_LOGFILENAME,
	MIDGARD_CONFIG_SCHEMA,
	MIDGARD_CONFIG_DEFAULT_LANG,
	MIDGARD_CONFIG_LOGLEVEL,
	MIDGARD_CONFIG_TABLECREATE,
	MIDGARD_CONFIG_TABLEUPDATE,
	MIDGARD_CONFIG_LOGFILE,
	MIDGARD_CONFIG_TESTUNIT,
	MIDGARD_CONFIG_MGDUSERNAME,
	MIDGARD_CONFIG_MGDPASSWORD,
	MIDGARD_CONFIG_AUTHTYPE,
	MIDGARD_CONFIG_PAMFILE
};

static MidgardConfigPrivate *midgard_config_private_new(void)
{
	MidgardConfigPrivate *config_private = 
		g_new(MidgardConfigPrivate, 1);
        config_private->keyfile = NULL;
	config_private->log_channel = NULL;
	config_private->configname = NULL;

	return config_private;
}

MidgardConfig *midgard_config_new(void)
{
	MidgardConfig *self = 
		g_object_new(MIDGARD_TYPE_CONFIG, NULL);
	return self;
}

/* Get configuration directory path.
 * We define this directory according to MIDGARD_LIB_PREFIX 
 * We also cache the result as it is costs a lot to do g_dir_open() each time we calculate paths
 * This means that returned mdirs* should be constant.
 */
 
const mdirs *_midgard_core_config_get_config_dirs()
{
	static mdirs *iconf = NULL;
	GDir *dir;
	
	if (iconf != NULL) return iconf;
	
	iconf = g_new(mdirs,1);
	
	if (g_ascii_strcasecmp(MIDGARD_LIB_PREFIX, "/usr") == 0 ) {
		/* Set typical FSH directory */
		iconf->confdir = g_strconcat("/etc/", MIDGARD_PACKAGE_NAME, "/conf.d", NULL);
		iconf->blobdir = g_strconcat("/var/lib/", MIDGARD_PACKAGE_NAME, "/blobs", NULL);
	} else {
		/* Set any directory which follows midgard prefix */
		iconf->confdir = g_strconcat(MIDGARD_LIB_PREFIX, "/etc/", 
				MIDGARD_PACKAGE_NAME, "/conf.d", NULL);
		iconf->blobdir = g_strconcat(MIDGARD_LIB_PREFIX, "/var/lib/", 
				MIDGARD_PACKAGE_NAME, "/blobs", NULL);
	}

	/* Set blobdir to /var/local when /usr/local was set as prefix */
	if (g_str_equal(MIDGARD_LIB_PREFIX, "/usr/local")) {
		iconf->blobdir = g_strconcat("/var/local/lib/", MIDGARD_PACKAGE_NAME, 
				"/blobs", NULL);
	}
	
	iconf->sharedir = g_strconcat( MIDGARD_LIB_PREFIX, "/share/", 
			MIDGARD_PACKAGE_NAME, NULL);
	
	dir = g_dir_open(iconf->confdir, 0, NULL);
	if (dir == NULL ) {
		/* No idea at this moment how to create this directory and copy example file
		 * * when midgard-core is installed */ 
		g_error("Unable to open %s directory" , iconf->confdir);
		g_free(iconf->confdir);
		g_free(iconf->blobdir);
		g_free(iconf->sharedir);
		iconf = NULL;
		return NULL;
	} else {
		g_dir_close(dir);
		return iconf;
	}
	
	/* should not be reached */
	iconf = NULL;
	return NULL;
}

void _midgard_core_config_dirs_free(mdirs **iconf)
{
	if (!(iconf && *iconf)) return;
	g_free((*iconf)->confdir);
	g_free((*iconf)->blobdir);
	g_free((*iconf)->sharedir);

	g_free(*iconf);
	*iconf = NULL;
}

void __create_log_dir(const gchar *path)
{
	if(path == NULL)
		return;

	if(g_str_equal(path, ""))
		return;

	if(g_file_test(path, G_FILE_TEST_EXISTS)) 
		return;

	gchar *dir_path = g_path_get_dirname(path);

	gint rv = g_mkdir_with_parents((const gchar *)dir_path, 0711);

	if(rv == -1) {
		g_warning("Failed to create '%s' directory", dir_path);
		g_free(dir_path);
		return ;
	}	
}

gboolean midgard_config_read_file(MidgardConfig *self, const gchar *filename, gboolean user)
{
	const mdirs *iconf;
	gchar *fname = NULL;
	gchar  *tmpstr;
	GKeyFile *keyfile;	
	gboolean tmpbool;
	GError *error = NULL;

	g_assert(self != NULL);
	
	if ((iconf = _midgard_core_config_get_config_dirs()) == NULL)
		return FALSE;

	if(user) {
		gchar *_umcd = g_strconcat(".", MIDGARD_PACKAGE_NAME, NULL);
		fname = g_build_path (G_DIR_SEPARATOR_S, 
				g_get_home_dir(), _umcd, "conf.d", filename, NULL);
		g_free(_umcd);
	} else {
		fname = g_build_path (G_DIR_SEPARATOR_S,
				iconf->confdir, "/", filename, NULL);
	}

	keyfile = g_key_file_new();
	if (!g_key_file_load_from_file (
				keyfile, fname, G_KEY_FILE_NONE, &error)) {
		g_critical("Can not open %s. %s" , fname, error->message);
		g_clear_error(&error);
		g_free(fname);
		return FALSE;
	}

	g_free(fname);	

	/* Get database type */
	self->private->dbtype = MIDGARD_DB_TYPE_MYSQL;
	tmpstr = g_key_file_get_string(keyfile, "Database", "Type", NULL);

	if(!tmpstr || g_str_equal(tmpstr, "")) {
		g_free(tmpstr);
		tmpstr = g_strdup("MySQL");
	}
	
	if(g_str_equal(tmpstr, "MySQL")) {
		self->private->dbtype = MIDGARD_DB_TYPE_MYSQL;
	} else if(g_str_equal(tmpstr, "PostgreSQL")) {
		self->private->dbtype = MIDGARD_DB_TYPE_POSTGRES;
	} else if(g_str_equal(tmpstr, "FreeTDS")) {
		self->private->dbtype = MIDGARD_DB_TYPE_FREETDS;
	} else if(g_str_equal(tmpstr, "SQLite")) {
		self->private->dbtype = MIDGARD_DB_TYPE_SQLITE;
	} else if(g_str_equal(tmpstr, "ODBC")) {
		self->private->dbtype = MIDGARD_DB_TYPE_ODBC;
	} else if(g_str_equal(tmpstr, "Oracle")) {
		self->private->dbtype = MIDGARD_DB_TYPE_ORACLE;
	} else if(tmpstr == NULL) {
		self->private->dbtype = MIDGARD_DB_TYPE_MYSQL;
	} else {
		g_warning("'%s' database type is invalid. Setting default MySQL one", tmpstr);
		self->private->dbtype = MIDGARD_DB_TYPE_MYSQL;
		g_free(tmpstr);
		tmpstr = g_strdup("MySQL");
	}

	self->dbtype = g_strdup(tmpstr);

	g_free(tmpstr);
		
	/* Get host name or IP */
	tmpstr = g_key_file_get_string(keyfile, "Database", "Host", NULL);
	if(tmpstr  && *tmpstr != '\0') {
		g_free(self->host);
		self->host = g_strdup(tmpstr);
	}
	g_free(tmpstr);
			
	/* Get database name */
	tmpstr = g_key_file_get_string(keyfile, "Database", "Name", NULL);
	if(tmpstr && *tmpstr != '\0') {
		g_free(self->database);
		self->database = g_strdup(tmpstr);
	}
	g_free(tmpstr);
	
	/* Get database's username */
	tmpstr = g_key_file_get_string(keyfile, "Database", "Username", NULL);
	if(tmpstr && *tmpstr != '\0') {
		g_free(self->dbuser);
		self->dbuser = g_strdup(tmpstr);
	}
	g_free(tmpstr);
		
	/* Get password for database user */
	tmpstr = g_key_file_get_string(keyfile, "Database", "Password", NULL);
	if(tmpstr && *tmpstr != '\0') {
		g_free(self->dbpass);
		self->dbpass = g_strdup(tmpstr);
	}
	g_free(tmpstr);

	/* Get default language */
	tmpstr = g_key_file_get_string(keyfile, "Database", "DefaultLanguage", NULL); 
	if(tmpstr == NULL)
		self->default_lang = NULL;
	else 
		self->default_lang = g_strdup(tmpstr);

	g_free(tmpstr);

	/* Get blobs' path */
	tmpstr = g_key_file_get_string(keyfile, "Database", "Blobdir", NULL);
	if(tmpstr == NULL) {
		self->blobdir = 
			g_strconcat(iconf->blobdir, "/", self->database, NULL);
	} else {
		self->blobdir = g_strdup(tmpstr);
		g_free(tmpstr);
	}
		
	/* Get schema file */
	tmpstr = g_key_file_get_string(keyfile, "Database", "Schema", NULL);
	if(tmpstr) {
		self->schemafile = g_strdup(tmpstr);
		g_free(tmpstr);
		tmpstr = NULL;
	}
	
	/* Get log filename */
	tmpstr = g_key_file_get_string(keyfile, "Database", "Logfile", NULL);
	GIOChannel *channel = NULL;
	if(tmpstr != NULL && !g_str_equal(tmpstr, "")) {

		__create_log_dir((const gchar *) tmpstr);

		g_free(self->logfilename);
		self->logfilename = g_strdup(tmpstr);
		GError *err = NULL;
		channel =
			g_io_channel_new_file(
					tmpstr,
					"a",
					&err);
		g_free(tmpstr);
		tmpstr = NULL;

		if(!channel){
			g_warning("Can not open '%s' logfile. %s", 
					self->logfilename,
					err->message);
		} else {
			self->private->log_channel = channel;
		}
		g_clear_error(&err);
	}

	if(tmpstr)
		g_free(tmpstr);
	
	/* Get log level */
	tmpstr = g_key_file_get_string(keyfile, "Database", "Loglevel", NULL);
	if(tmpstr && *tmpstr != '\0') {
		g_free(self->loglevel);
		self->loglevel = g_strdup(tmpstr);
	}
	g_free(tmpstr);
	tmpstr = NULL;

	/* Get database creation mode */
	tmpbool = g_key_file_get_boolean(keyfile, "Database", "TableCreate", NULL);
	self->tablecreate = tmpbool;
	

	/* Get database update mode */
	tmpbool = g_key_file_get_boolean(keyfile, "Database", "TableUpdate", NULL);
	self->tableupdate = tmpbool;

	/* Get SG admin username */
	tmpstr = g_key_file_get_string(keyfile, "Database", "MidgardUsername", NULL);
	if(tmpstr != NULL) {
		g_free(self->mgdusername);
		self->mgdusername = g_strdup(tmpstr);		
		g_free(tmpstr);
	}
	
	/* Get SG admin password */
	tmpstr = g_key_file_get_string(keyfile, "Database", "MidgardPassword", NULL);
	if(tmpstr != NULL) {
		g_free(self->mgdpassword);
		self->mgdpassword = g_strdup(tmpstr);
		g_free(tmpstr);
	}
	
	/* Get test mode */
	tmpbool = g_key_file_get_boolean(keyfile, "Database", "TestUnit", NULL);
	self->testunit = tmpbool;
	
	/* Get auth type */
	tmpstr = g_key_file_get_string(keyfile, "Database", "AuthType", NULL);
	if(tmpstr == NULL) {
		
		self->authtype = MIDGARD_AUTHTYPE_NORMAL;
	
	} else {
		
		if(g_str_equal(tmpstr, "Normal")) {
			self->authtype = MIDGARD_AUTHTYPE_NORMAL;
		} else if(g_str_equal(tmpstr, "PAM")) {
			self->authtype = MIDGARD_AUTHTYPE_PAM;
		} else if(g_str_equal(tmpstr, "Trusted")) {
			self->authtype = MIDGARD_AUTHTYPE_TRUST;
		} else {
			g_error("Unknown '%s' auth type", tmpstr);
		}
		g_free(tmpstr);
	}
	
	/* Get Pam file */
	tmpstr = g_key_file_get_string(keyfile, "Database", "PamFile", NULL);
	if(tmpstr == NULL)
		tmpstr = g_strdup("midgard");
	self->pamfile = g_strdup(tmpstr);
	g_free(tmpstr);

	/* Disable threads */
	tmpbool = g_key_file_get_boolean(keyfile, "Database", "GdaThreads", NULL);
	self->gdathreads = tmpbool;

	/* We will free it when config is unref */
	self->private->keyfile = keyfile;

	self->private->configname = g_strdup(filename);

	return TRUE; 
}

gchar **midgard_config_list_files(gboolean user)
{
	gchar *config_dir;
	const mdirs *iconf;

	if(user) {
		
		gchar *_umcd = g_strconcat(".", MIDGARD_PACKAGE_NAME, NULL);
		config_dir = g_build_path (G_DIR_SEPARATOR_S,
				g_get_home_dir(), _umcd, "conf.d", NULL);
		g_free(_umcd);
		if (!g_file_test (config_dir, G_FILE_TEST_IS_DIR)){
			g_error("Config directory '%s' doesn't exist",
					config_dir);
			g_free(config_dir);
			return NULL;
		} 

	} else {

		iconf = _midgard_core_config_get_config_dirs();
		if(!iconf)
			return NULL;
		
		config_dir = g_strdup(iconf->confdir);
	}

	GError *error = NULL;
	GSList *list = NULL;
	gchar **filenames = NULL;
	guint i = 0, j = 0;
	GDir *dir = g_dir_open(config_dir, 0, &error);
	if (dir != NULL) {
		const gchar *file = g_dir_read_name(dir);
	
		while (file != NULL) {
			
			if(!g_str_has_prefix(file, ".")
					&& (!g_str_has_prefix(file, "#"))
					&& (!g_str_has_suffix(file, "~"))
					&& (!g_str_has_suffix(file, "example"))) {
					
				list = g_slist_append(list, (gpointer)g_strdup(file));
				i++;
			}
			file = g_dir_read_name(dir);
		}
		
		g_dir_close(dir);
		
		filenames = g_new(gchar *, i+1);
		for( ; list; list = list->next) {
			filenames[j] = (gchar *)list->data;
			j++;
		}
		filenames[i] = NULL;		
	}
	
	g_free(config_dir);

	return filenames;
}

/* Saves configuration file for the given MidgardConfig. */
gboolean midgard_config_save_file(MidgardConfig *self,
		const gchar *name, gboolean user)
{
	g_return_val_if_fail(self != NULL, FALSE);
	g_return_val_if_fail(name != NULL, FALSE);

	gchar *cnfpath = NULL;
	guint namel = strlen(name);
	gchar *_umcd = NULL;

	if(namel < 1) {
		g_warning("Can not save configuration file without a name");
		return FALSE;
	}

	if(self->private->keyfile == NULL)
		self->private->keyfile = g_key_file_new();

	/* Check configuration directory.
	 * If user's conf.d dir doesn't exist, create it */
	if(user) {

		gchar *_cnf_path = _midgard_core_config_build_path(NULL, NULL, TRUE);
		g_free(_cnf_path);

		_umcd = g_strconcat(".", MIDGARD_PACKAGE_NAME, NULL);
		gchar *path = g_build_path(G_DIR_SEPARATOR_S,
				g_get_home_dir(), _umcd, NULL);
		g_free(_umcd);

		/* Check if .midgard directory exists */
		if(!g_file_test((const gchar *)path, 
					G_FILE_TEST_EXISTS))
			g_mkdir(path, 0);

		if(!g_file_test((const gchar *)path,
					G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {

			g_warning("%s is not a directory!", path);
			g_free(path);
			return FALSE;
		}
		g_free(path);

		_umcd = g_strconcat(".", MIDGARD_PACKAGE_NAME, NULL);
		path = g_build_path(G_DIR_SEPARATOR_S, 
				g_get_home_dir(), _umcd, "conf.d", NULL);
		g_free(_umcd);
		
		/* Check if .midgard/conf.d directory exists */
		if(!g_file_test((const gchar *)path, 
					G_FILE_TEST_EXISTS))
			g_mkdir(path, 0);

		if(!g_file_test((const gchar *)path,
					G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {

			g_warning("%s is not a directory!", path);
			g_free(path);
			return FALSE;
		}
		g_free(path);

		_umcd = g_strconcat(".", MIDGARD_PACKAGE_NAME, NULL);
		cnfpath = g_build_path (G_DIR_SEPARATOR_S,
				g_get_home_dir(), _umcd, "conf.d", name, NULL);
		g_free(_umcd);

	} else {
		
		const mdirs *iconf = _midgard_core_config_get_config_dirs();
		if(!iconf)
			return FALSE;

		if(!g_file_test((const gchar *)iconf->confdir,
					G_FILE_TEST_EXISTS)
				|| (!g_file_test((const gchar *)iconf->confdir,
						G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))) {
			g_warning("%s doesn't exist or is not a directory!", 
					iconf->confdir);
			return FALSE;
		}

		cnfpath = g_build_path (G_DIR_SEPARATOR_S,
				iconf->confdir, name, NULL);

	}

	guint n_props, i;
	GValue pval = {0,};
	const gchar *nick = NULL;
	gchar *tmpstr;
	GParamSpec **props = g_object_class_list_properties(
			G_OBJECT_GET_CLASS(G_OBJECT(self)), &n_props);

	/* It should not happen */
	if(!props)
		g_error("Midgard Config class has no members registered");

	for(i = 0; i < n_props; i++) {
		
		nick = g_param_spec_get_nick (props[i]);

		g_value_init(&pval,props[i]->value_type); 
		g_object_get_property(G_OBJECT(self), (gchar*)props[i]->name, &pval);

		switch(props[i]->value_type){
	
			case G_TYPE_STRING:
				tmpstr = (gchar *)g_value_get_string(&pval);
				if(!tmpstr)
					tmpstr = "";
				g_key_file_set_string(self->private->keyfile,
						"Database",
						nick, tmpstr);
				break;

			case G_TYPE_BOOLEAN:
				g_key_file_set_boolean(self->private->keyfile,
						"Database", 
						nick, 
						g_value_get_boolean(&pval));
				break;
		}

		g_key_file_set_comment(self->private->keyfile, "Database", nick,
				g_param_spec_get_blurb(props[i]), NULL);
		g_value_unset(&pval);
	}

	g_free(props);

	gsize length;
	GError *error = NULL;
	
	gchar *content = g_key_file_to_data(self->private->keyfile,
			&length, &error);

	if(length < 1) {
		g_warning("%s", error->message);
		g_free(cnfpath);
		g_clear_error(&error);
		return FALSE;
	}

	g_clear_error(&error);

	gboolean saved = g_file_set_contents(cnfpath, 
			content, length, &error);

	g_free(content);

	if(!saved) {
		g_warning("%s", error->message);
		g_free(cnfpath);
		g_clear_error(&error);
		return FALSE;
	}

#ifdef G_OS_WIN32
	/* TODO , implement WIN32 */
#else 
	g_chmod(cnfpath, 0600);
#endif /* MIDGARD_IS_WIN32 */
	
	g_free(cnfpath);

	return TRUE;
}

gboolean midgard_config_create_midgard_tables(MidgardConfig *self,
		MidgardConnection *mgd)
{
	if(mgd == NULL) {
		g_warning("Connection should be established for midgard_config_create_midgard_tables");
		return FALSE;
	}

	gboolean ret =  _midgard_core_query_create_basic_db(mgd);

	if(!ret)
		return FALSE;
	
	/* Create person table */
	MidgardObjectClass *person_klass =
		MIDGARD_OBJECT_GET_CLASS_BY_NAME("midgard_person");
	midgard_config_create_class_table(self, person_klass, mgd);

	/* Create member table */
	MidgardObjectClass *member_klass =
		MIDGARD_OBJECT_GET_CLASS_BY_NAME("midgard_member");
	midgard_config_create_class_table(self, member_klass, mgd);

	const gchar *sql = "SELECT lastname, firstname FROM person \
			    WHERE sitegroup=0 AND guid='f6b665f1984503790ed91f39b11b5392'";

	gint rows = midgard_query_execute_with_fail(mgd->mgd, g_strdup((gchar *) sql));
	
	if(rows < 1) {
		
		/* This is initial db so we have to "import" SG0 default Admin */
		/* midgard root record */
		sql = "INSERT INTO person (guid, sitegroup, birthdate, homepage, email, extra, "
			"metadata_creator, lastname, firstname, metadata_created, "
			"metadata_revision, username, password ) "
			"VALUES ('f6b665f1984503790ed91f39b11b5392', 0, '1999-05-18 14:40:01', "
			"'http://www.midgard-project.org', 'dev@lists.midgard-project.org', "
			"'Default administrator account for Midgard', 'f6b665f1984503790ed91f39b11b5392', "
			"'Administrator', 'Midgard', '1999-05-18 14:40:01', 1, "
			"'root', '**password')";
		
		midgard_query_execute(mgd->mgd, g_strdup((gchar *)sql), NULL);

		/* Create root's repligard entry */
		sql = "INSERT INTO repligard (guid, typename, sitegroup, object_action) "
			"VALUES ('f6b665f1984503790ed91f39b11b5392', 'midgard_person', 0, 0)";
		midgard_query_execute(mgd->mgd, g_strdup((gchar *)sql), NULL);

		/* midgard root's member record */
		sql = "INSERT INTO member (guid, uid, gid) VALUES ('df351bde7c09bce7ad2cf14f2eaf7fb7', 1, 0)";
		midgard_query_execute(mgd->mgd, g_strdup((gchar *)sql), NULL);

		/* Create root's membership repligard entry */
		sql = "INSERT INTO repligard (guid, typename, sitegroup, object_action) "
			"VALUES ('df351bde7c09bce7ad2cf14f2eaf7fb7', 'midgard_member', 0, 0)";
		midgard_query_execute(mgd->mgd, g_strdup((gchar *)sql), NULL);

	}

	return TRUE;
}

gboolean midgard_config_create_class_table(MidgardConfig *self,
		MidgardObjectClass *klass, MidgardConnection *mgd)
{
	if(mgd == NULL) {
		g_warning("Connection should be established for midgard_config_create_class_table");
		return FALSE;
	}
	
	g_return_val_if_fail(klass != NULL, FALSE);

	return _midgard_core_query_create_class_storage(mgd, klass);
}

gboolean midgard_config_update_class_table(MidgardConfig *self,
		MidgardObjectClass *klass, MidgardConnection *mgd)
{
	if(mgd == NULL) {
		
		g_warning("Connection should be established for midgard_config_update_class_table");
		return FALSE;
	}

	g_return_val_if_fail(klass != NULL, FALSE); 
	return _midgard_core_query_update_class_storage(mgd, klass); 
}

gboolean midgard_config_class_table_exists(MidgardConfig *self,
		MidgardObjectClass *klass, MidgardConnection *mgd)
{
	const gchar *table = midgard_object_class_get_table(klass);
	return _midgard_core_query_table_exists(mgd, table);
}

/*
MidgardConnection *midgard_config_init(const gchar *filename)
{
	GError *err = NULL;
	MidgardConnection *mgd = midgard_connection_new();

	if(!mgd)
		g_error("Can not initialize midgard_connection object");

	MidgardConfig *config = g_object_new(MIDGARD_TYPE_CONFIG, NULL);
	
	if(!midgard_config_read_file(config, (const gchar *)filename, TRUE)){
		g_object_unref(mgd);
		g_object_unref(config);
		return NULL;
	}

	GHashTable *hash = NULL;
	if(!midgard_connection_open_config(mgd, config, &hash, err)){
		g_object_unref(mgd);
		g_object_unref(config);
		if(hash) g_hash_table_destroy(hash);
		return NULL;
	}
	if(hash) g_hash_table_destroy(hash);

	
	g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK,
			mgd_log_debug_default,
			(gpointer)midgard_connection_get_loglevel(mgd));
	g_log_set_handler("midgard-core", G_LOG_LEVEL_MASK,
			mgd_log_debug_default,
			(gpointer)midgard_connection_get_loglevel(mgd));
	g_log_set_handler("midgard-core-type", G_LOG_LEVEL_MASK,
			mgd_log_debug_default,
			(gpointer)midgard_connection_get_loglevel(mgd));
			

	return mgd;
}
*/

/* !! Undocumented workarounds !! */

static void __config_struct_new(MidgardConfig *self)
{
	self->mgdusername = g_strdup("root");
	self->mgdpassword = g_strdup("password");
	self->host = g_strdup("localhost");
	self->database = g_strdup("midgard");	
	self->dbuser = g_strdup("midgard");
	self->dbpass = g_strdup("midgard");
	self->blobdir = NULL;
	self->logfilename = NULL;
	self->schemafile = NULL;
	self->default_lang = NULL;
	self->loglevel = g_strdup("warn");
	self->pamfile = NULL;
}

MidgardConfig *midgard_config_struct_new(void)
{
	MidgardConfig *self = 
		g_new(MidgardConfig, 1);
	
	__config_struct_new(self);

	self->private = midgard_config_private_new(); 
	
	return self;
}

MidgardConfig *midgard_config_struct2gobject(MidgardConfig *config)
{
	MidgardConfig *new_config = 
		midgard_config_new();

	g_free(new_config->private);
	new_config->private = config->private;

	return new_config;
}

void __midgard_config_struct_free(MidgardConfig *self, gboolean object)
{
	if(self->mgdusername)
		g_free(self->mgdusername);
	if(self->mgdpassword)
		g_free(self->mgdpassword);
	if(self->host)
		g_free(self->host);
	if(self->database)
		g_free(self->database);
	if(self->dbuser)
		g_free(self->dbuser);
	if(self->dbpass)
		g_free(self->dbpass);
	if(self->blobdir)
		g_free(self->blobdir);
	if(self->logfilename)
		g_free(self->logfilename);
	if(self->schemafile)
		g_free(self->schemafile);
	if(self->default_lang)
		g_free(self->default_lang);
	if(self->loglevel)
		g_free(self->loglevel);
	if(self->pamfile)
		g_free(self->pamfile);

	if(self->private->keyfile)
		g_key_file_free(self->private->keyfile);
	self->private->keyfile = NULL;

	if(self->private->configname)
		g_free(self->private->configname);
	self->private->configname = NULL;

	g_free(self->private);
	self->private = NULL;

	if(!object) {
		g_free(self);
		self = NULL;
	}
}

void midgard_config_struct_free(MidgardConfig *config)
{
	g_assert(config != NULL);	
	__midgard_config_struct_free(config, FALSE);
}

/* GOBJECT ROUTINES */
static void
_midgard_config_set_property (GObject *object, guint property_id,
		        const GValue *value, GParamSpec *pspec){
	
	MidgardConfig *self = (MidgardConfig *) object;

	switch (property_id) {

		case MIDGARD_CONFIG_DBTYPE:
			self->dbtype = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_DBNAME:
			g_free(self->database);
			self->database = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_DBUSER: 
			g_free(self->dbuser);
			self->dbuser = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_DBPASS:
			g_free(self->dbpass);
			self->dbpass = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_HOST:
			g_free(self->host);
			self->host = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_BLOBDIR:
			g_free(self->blobdir);
			self->blobdir = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_LOGFILENAME:
			g_free(self->logfilename);
			self->logfilename = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_SCHEMA:
			g_free(self->schemafile);
			self->schemafile = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_DEFAULT_LANG:
			g_free(self->default_lang);
			self->default_lang = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_LOGLEVEL:
			g_free(self->loglevel);
			self->loglevel = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_TABLECREATE:
			self->tablecreate = g_value_get_boolean(value);
			break;

		case MIDGARD_CONFIG_TABLEUPDATE:
			self->tableupdate = g_value_get_boolean(value);
			break;

		case MIDGARD_CONFIG_TESTUNIT:
			self->testunit = g_value_get_boolean(value);
			break;
		
		case MIDGARD_CONFIG_MGDUSERNAME:
			g_free(self->mgdusername);
			self->mgdusername = g_value_dup_string(value);
			break;
			
		case MIDGARD_CONFIG_MGDPASSWORD:
			g_free(self->mgdpassword);
			self->mgdpassword = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_AUTHTYPE:
			self->authtype = g_value_get_uint(value);
			break;

		case MIDGARD_CONFIG_PAMFILE:
			g_free(self->pamfile);
			self->pamfile = g_value_dup_string(value);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
			break;
	}	
}

static void
_midgard_config_get_property (GObject *object, guint property_id,
		GValue *value, GParamSpec *pspec){

	MidgardConfig *self = (MidgardConfig *) object;
	
	switch (property_id) {
		
		case MIDGARD_CONFIG_DBTYPE:
			g_value_set_string(value, self->dbtype);
			break;
	
		case MIDGARD_CONFIG_DBNAME: 
			g_value_set_string (value, self->database);
			break;

		case MIDGARD_CONFIG_DBUSER: 
			g_value_set_string (value, self->dbuser);
			break;

		case MIDGARD_CONFIG_DBPASS: 
			g_value_set_string (value, self->dbpass);
			break;

		case MIDGARD_CONFIG_HOST:
			g_value_set_string (value, self->host);
			break;

		case MIDGARD_CONFIG_BLOBDIR:
			g_value_set_string (value, self->blobdir);
			break;

		case MIDGARD_CONFIG_LOGFILENAME:
			g_value_set_string (value, self->logfilename);
			break;

		case MIDGARD_CONFIG_SCHEMA:
			g_value_set_string (value, self->schemafile);
			break;

		case MIDGARD_CONFIG_DEFAULT_LANG:
			g_value_set_string (value, self->default_lang);
			break;

		case MIDGARD_CONFIG_LOGLEVEL:
			g_value_set_string(value, self->loglevel);
			break;

		case MIDGARD_CONFIG_TABLECREATE:
			g_value_set_boolean (value, self->tablecreate);
			break;
	
		case MIDGARD_CONFIG_TABLEUPDATE:
			g_value_set_boolean (value, self->tableupdate);
			break;

		case MIDGARD_CONFIG_TESTUNIT:
			g_value_set_boolean (value, self->testunit);
			break;
			
		case MIDGARD_CONFIG_MGDUSERNAME:
			g_value_set_string(value, self->mgdusername);
			break;
		
		case MIDGARD_CONFIG_MGDPASSWORD:
			g_value_set_string(value, self->mgdpassword);
			break;

		case MIDGARD_CONFIG_AUTHTYPE:
			g_value_set_uint(value, self->authtype);
			break;

		case MIDGARD_CONFIG_PAMFILE:
			g_value_set_string(value, self->pamfile);
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
			break;
	}
}

static void 
_midgard_config_finalize(GObject *object)
{
	g_assert(object != NULL);
	
	MidgardConfig *self = (MidgardConfig *) object;

	__midgard_config_struct_free(self, TRUE);
}

static void _midgard_config_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardConfigClass *klass = MIDGARD_CONFIG_CLASS (g_class);
	GParamSpec *pspec;
	
	gobject_class->set_property = _midgard_config_set_property;
	gobject_class->get_property = _midgard_config_get_property;
	gobject_class->finalize = _midgard_config_finalize;
		
	klass->read_config = midgard_config_read_file;
	/* klass->connect = mgd_connect; */

	/* Register properties */
	pspec = g_param_spec_string ("dbtype",
			"Type",
			"Database type ( by default MySQL )",
			"MySQL",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DBTYPE,
			pspec);
	
	pspec = g_param_spec_string ("host",
			"Host",
			"Database host ( 'localhost' by default )",
			"localhost",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_HOST,
			pspec);
	
	pspec = g_param_spec_string ("database",
			"Name",
			"Name of the database",
			"midgard",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DBNAME,
			pspec);

	pspec = g_param_spec_string ("dbuser",
			"Username",
			"Username for user who is able to connect to database",
			"midgard",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DBUSER,
			pspec);

	pspec = g_param_spec_string ("dbpass",
			"Password",
			"Password for user who is able to connect to database",
			"midgard",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DBPASS,
			pspec);

	pspec = g_param_spec_string ("blobdir",
			"Blobdir",
			"Location of the blobs directory",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_BLOBDIR,
			pspec);
	
	pspec = g_param_spec_string ("logfilename",
			"Logfile",
			"Location of the log file",
			"",
			G_PARAM_READWRITE);    
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_LOGFILENAME,
			pspec);

	pspec = g_param_spec_string ("schema",
			"Schema",  
			"Location of the schema file",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_SCHEMA,
			pspec);

	pspec = g_param_spec_string ("defaultlang",
			"DefaultLanguage",  
			"Default language",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DEFAULT_LANG,
			pspec);
	
	pspec = g_param_spec_string ("loglevel",
			"Loglevel",
			"Log level",
			"warn",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_LOGLEVEL,
			pspec);

	pspec = g_param_spec_boolean("tablecreate",
			"TableCreate",
			"Database creation switch",
			FALSE, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_TABLECREATE,
			pspec);

	pspec = g_param_spec_boolean("tableupdate",
			"TableUpdate",
			"Database update switch",
			FALSE, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_TABLEUPDATE,
			pspec);

	pspec = g_param_spec_boolean("testunit",
			"TestUnit",
			"Database and objects testing switch",
			FALSE, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_TESTUNIT,
			pspec);
	
	pspec = g_param_spec_string ("midgardusername",
			"MidgardUsername",
			"Midgard sitegroup person's username",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_MGDUSERNAME,
			pspec);
	
	pspec = g_param_spec_string ("midgardpassword",
			"MidgardPassword",
			"Midgard sitegroup person's password",
			"",
			G_PARAM_READWRITE);   
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_MGDPASSWORD,
			pspec);

	pspec = g_param_spec_string ("pamfile",
			"PamFile",
			"Name of the file used with PAM authentication type",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_PAMFILE,
			pspec);
	
	pspec = g_param_spec_uint ("authtype",
			"AuthType",
			"Authentication type used with connection.",
			0, G_MAXUINT32, 0, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_AUTHTYPE,
			pspec);		
}

static void _midgard_config_instance_init(
		GTypeInstance *instance, gpointer g_class) 
{
	MidgardConfig *self = (MidgardConfig *) instance;

	__config_struct_new(self);

	self->private = midgard_config_private_new();
}

/* Register MidgardConfig type */
GType 
midgard_config_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardConfigClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_config_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardConfig),
			0,              /* n_preallocs */
			(GInstanceInitFunc) _midgard_config_instance_init/* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
				"midgard_config",
				&info, 0);
	}	
	return type;
}
