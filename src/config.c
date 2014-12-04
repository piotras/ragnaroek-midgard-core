/* Midgard config files routine functions 
 *    
 * Copyright (C) 2005,2006 Piotr Pokora <pp@infoglob.pl>
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
#include "midgard/types.h"
#include "midgard/midgard_config.h"
#include "midgard/parser.h"
#include "fmt_russian.h"
#include "midgard/midgard_datatypes.h"
#include "src/midgard_core_object.h"

/* Properties */
enum {
	MIDGARD_CONFIG_DBTYPE = 1,
	MIDGARD_CONFIG_DBNAME, /* remove me */
	MIDGARD_CONFIG_DBUSER, /* remove me */
	MIDGARD_CONFIG_DBPASS, /* remove me */
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
	MIDGARD_CONFIG_LOGHANDLER,
	MIDGARD_CONFIG_MGDUSERNAME,
	MIDGARD_CONFIG_MGDPASSWORD,
	MIDGARD_CONFIG_AUTHTYPE,
	MIDGARD_CONFIG_PAMFILE
};

static void
_midgard_config_set_property (GObject *object, guint property_id,
		        const GValue *value, GParamSpec *pspec){
	
	MidgardConfig *self = (MidgardConfig *) object;

	switch (property_id) {

		case MIDGARD_CONFIG_DBTYPE:
			self->private->dbtype = g_value_get_uint(value);
			break;

		case MIDGARD_CONFIG_DBNAME:
			g_free(self->private->database);
			self->private->database = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_DBUSER: 
			g_free(self->private->dbuser);
			self->private->dbuser = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_DBPASS:
			g_free(self->private->dbpass);
			self->private->dbpass = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_HOST:
			g_free(self->private->host);
			self->private->host = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_BLOBDIR:
			g_free(self->private->blobdir);
			self->private->blobdir = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_LOGFILENAME:
			g_free(self->private->logfilename);
			self->private->logfilename = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_SCHEMA:
			g_free(self->private->schemafile);
			self->private->schemafile = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_DEFAULT_LANG:
			g_free(self->private->default_lang);
			self->private->default_lang = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_LOGLEVEL:
			g_free(self->private->loglevel);
			self->private->loglevel = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_TABLECREATE:
			self->private->tablecreate = g_value_get_boolean(value);
			break;

		case MIDGARD_CONFIG_TABLEUPDATE:
			self->private->tableupdate = g_value_get_boolean(value);
			break;

		case MIDGARD_CONFIG_TESTUNIT:
			self->private->testunit = g_value_get_boolean(value);
			break;

		case MIDGARD_CONFIG_LOGHANDLER:
			self->private->loghandler = g_value_get_boolean(value);
			break;
		
		case MIDGARD_CONFIG_MGDUSERNAME:
			g_free(self->private->mgdusername);
			self->private->mgdusername = g_value_dup_string(value);
			break;
			
		case MIDGARD_CONFIG_MGDPASSWORD:
			g_free(self->private->mgdpassword);
			self->private->mgdpassword = g_value_dup_string(value);
			break;

		case MIDGARD_CONFIG_AUTHTYPE:
			self->private->authtype = g_value_get_uint(value);
			break;

		case MIDGARD_CONFIG_PAMFILE:
			g_free(self->private->pamfile);
			self->private->pamfile = g_value_dup_string(value);
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
			g_value_set_uint(value, self->private->dbtype);
			break;
	
		case MIDGARD_CONFIG_DBNAME: /* remove me */
			g_value_set_string (value, self->private->database);
			break;

		case MIDGARD_CONFIG_DBUSER: /* remove me */
			g_value_set_string (value, self->private->dbuser);
			break;

		case MIDGARD_CONFIG_DBPASS: /* remove me */
			g_value_set_string (value, self->private->dbpass);
			break;

		case MIDGARD_CONFIG_HOST:
			g_value_set_string (value, self->private->host);
			break;

		case MIDGARD_CONFIG_BLOBDIR:
			g_value_set_string (value, self->private->blobdir);
			break;

		case MIDGARD_CONFIG_LOGFILENAME:
			g_value_set_string (value, self->private->logfilename);
			break;

		case MIDGARD_CONFIG_SCHEMA:
			g_value_set_string (value, self->private->schemafile);
			break;

		case MIDGARD_CONFIG_DEFAULT_LANG:
			g_value_set_string (value, self->private->default_lang);
			break;

		case MIDGARD_CONFIG_LOGLEVEL:
			g_value_set_string(value, self->private->loglevel);
			break;

		case MIDGARD_CONFIG_TABLECREATE:
			g_value_set_boolean (value, self->private->tablecreate);
			break;
	
		case MIDGARD_CONFIG_TABLEUPDATE:
			g_value_set_boolean (value, self->private->tableupdate);
			break;

		case MIDGARD_CONFIG_TESTUNIT:
			g_value_set_boolean (value, self->private->testunit);
			break;
		
		case MIDGARD_CONFIG_LOGHANDLER:
			g_value_set_uint(value, self->private->loghandler);
			break;
			
		case MIDGARD_CONFIG_MGDUSERNAME:
			g_value_set_string(value, self->private->mgdusername);
			break;
		
		case MIDGARD_CONFIG_MGDPASSWORD:
			g_value_set_string(value, self->private->mgdpassword);
			break;

		case MIDGARD_CONFIG_AUTHTYPE:
			g_value_set_uint(value, self->private->authtype);
			break;

		case MIDGARD_CONFIG_PAMFILE:
			g_value_set_string(value, self->private->pamfile);
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

	if(self->private->mgdusername)
		g_free(self->private->mgdusername);
	if(self->private->mgdpassword)
		g_free(self->private->mgdpassword);
	if(self->private->host)
		g_free(self->private->host);
	if(self->private->database)
		g_free(self->private->database);
	if(self->private->dbuser)
		g_free(self->private->dbuser);
	if(self->private->dbpass)
		g_free(self->private->dbpass);
	if(self->private->blobdir)
		g_free(self->private->blobdir);
	if(self->private->logfilename)
		g_free(self->private->logfilename);
	if(self->private->schemafile)
		g_free(self->private->schemafile);
	if(self->private->default_lang)
		g_free(self->private->default_lang);
	if(self->private->loglevel)
		g_free(self->private->loglevel);
	if(self->private->pamfile)
		g_free(self->private->pamfile);
	
	g_free(self->private);

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
	pspec = g_param_spec_uint ("dbtype",
			"",
			"Database type",
			0, G_MAXUINT32, 1, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DBTYPE,
			pspec);
	
	pspec = g_param_spec_string ("host",
			"",
			"Host name or IP",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_HOST,
			pspec);
	
	pspec = g_param_spec_string ("database",
			"",
			"Remove me",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DBNAME,
			pspec);

	pspec = g_param_spec_string ("dbuser",
			"",
			"Remove me",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DBUSER,
			pspec);

	pspec = g_param_spec_string ("dbpass",
			"",
			"Remove me",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DBPASS,
			pspec);

	pspec = g_param_spec_string ("blobdir",
			"",
			"Location of the blobs directory",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_BLOBDIR,
			pspec);
	
	pspec = g_param_spec_string ("logfilename",
			"",
			"Location of the log file",
			"",
			G_PARAM_READABLE);    
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_LOGFILENAME,
			pspec);

	pspec = g_param_spec_string ("schema",
			"",  
			"Location of the schema file",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_SCHEMA,
			pspec);

	pspec = g_param_spec_string ("defaultlang",
			"",  
			"Default language",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_DEFAULT_LANG,
			pspec);
	
	pspec = g_param_spec_string ("loglevel",
			"warn",
			"Log level",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_LOGLEVEL,
			pspec);

	pspec = g_param_spec_boolean("tablecreate",
			"",
			"Database creation switch",
			FALSE, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_TABLECREATE,
			pspec);

	pspec = g_param_spec_boolean("tableupdate",
			"",
			"Database update switch",
			FALSE, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_TABLEUPDATE,
			pspec);

	pspec = g_param_spec_boolean("testunit",
			"",
			"Database and objects testing switch",
			FALSE, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_TESTUNIT,
			pspec);

	pspec = g_param_spec_uint ("loghandler",
			"",
			"GLib log handler",
			0, G_MAXUINT32, 0, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_LOGHANDLER,
			pspec);   
	
	pspec = g_param_spec_string ("midgardusername",
			"admin",
			"Midgard sitegroup person's username",
			"",
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_MGDUSERNAME,
			pspec);
	
	pspec = g_param_spec_string ("midgardpassword",
			"password",
			"Midgard sitegroup person's password",
			"",                                                                                                        G_PARAM_READABLE);   
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_MGDPASSWORD,
			pspec);

	pspec = g_param_spec_string ("pamfile",
			"midgard",
			"Name of the file used with PAM authentication type",
			"",                                                                                                        G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_PAMFILE,
			pspec);
	
	pspec = g_param_spec_uint ("authtype",
			"",
			"Authentication type used with connection.",
			0, G_MAXUINT32, 0, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_CONFIG_AUTHTYPE,
			pspec);		
}

static void _midgard_config_instance_init(
		GTypeInstance *instance, gpointer g_class) 
{
	MidgardConfig *self = (MidgardConfig *) instance;
	self->private = g_new(MidgardConfigPrivate, 1);	

	self->private->mgdusername = NULL;
	self->private->mgdpassword = NULL;
	self->private->host = NULL;
	self->private->database = NULL;
	self->private->dbuser = NULL;
	self->private->dbpass = NULL;
	self->private->blobdir = NULL;
	self->private->logfilename = NULL;
	self->private->schemafile = NULL;
	self->private->default_lang = NULL;
	self->private->loglevel = NULL;
	self->private->pamfile = NULL;

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

MidgardConfig *midgard_config_new(void)
{
	MidgardConfig *self = 
		g_object_new(MIDGARD_TYPE_CONFIG, NULL);
	return self;
}

/* Read config file  */
typedef struct 
{
  gchar *confdir;
  gchar *blobdir;
  gchar *sharedir;

}mdirs;

/* Get configuration directory path.
 * We define this directory according to MIDGARD_LIB_PREFIX */

static mdirs *_get_config_dir()
{
  mdirs *iconf;
  GDir *dir;

  iconf = g_new(mdirs,1);
  
  if (g_ascii_strcasecmp(MIDGARD_LIB_PREFIX, "/usr") == 0 ) {
    /* Set typical FSH directory */
    iconf->confdir = g_strdup("/etc/midgard/conf.d");
    iconf->blobdir = g_strdup("/var/lib/midgard/blobs");
  } else {
    /* Set any directory which follows midgard prefix */
    iconf->confdir = g_strconcat( MIDGARD_LIB_PREFIX, "/", "etc/midgard/conf.d", NULL);
    iconf->blobdir = g_strconcat( MIDGARD_LIB_PREFIX, "/", "var/lib/midgard/blobs", NULL);
  }
  
  /* Set blobdir to /var/local when /usr/local was set as prefix */
  if (g_ascii_strcasecmp(MIDGARD_LIB_PREFIX, "/usr/local") == 0 ) {
    iconf->blobdir = g_strdup("/var/local/lib/midgard/blobs");
  }

  iconf->sharedir = g_strconcat( MIDGARD_LIB_PREFIX, "/", "share/midgard", NULL);
  
  dir = g_dir_open(iconf->confdir, 0, NULL);
  if (dir == NULL ) {
    /* No idea at this moment how to create this directory and copy example file
     * when midgard-core is installed 
     */ 
    g_log("midgard-lib", G_LOG_LEVEL_CRITICAL, "Unable to open %s directory" , iconf->confdir);
    return NULL;
  } else {
    g_dir_close(dir);
    return iconf;
  }
  return NULL;
}


/* Use this function when glib >= 2.6 will be available in all distros */
/* 
static midgard *_set_config_from_file(const gchar *name)
{
  mdirs *iconf;
  const gchar *fname = NULL;
  GKeyFile *cfile;
  gchar *dbtype, *host, *dbname, *username, *password, *encoding, *blobdir, *schemafile;
  midgard *mgd;

  if ((iconf = _get_config_dir()) == NULL)
    return NULL;
  
  fname = g_strconcat(iconf->confdir, "/", name, NULL);

  cfile = g_key_file_new(); 
  
  if ((g_key_file_load_from_file (cfile, fname, G_KEY_FILE_NONE, NULL)) == FALSE) {
    g_log("midgard-lib", G_LOG_LEVEL_CRITICAL, "Unable to open %s configuration file" , fname);
    return NULL;
  }

  //Get database type 
  if ((dbtype = g_key_file_get_string(cfile, "Database", "Type", NULL)) == NULL )
    dbtype = "MySQL";
 
  // Get database host 
  if ((host = g_key_file_get_string(cfile, "Database", "Host", NULL)) == NULL )
    host = "localhost";
    
  // Get database name 
  if ((dbname = g_key_file_get_string(cfile, "Database", "Name", NULL)) == NULL )
    dbname = "midgard";
    
  // Get database username 
  username = g_key_file_get_string(cfile, "Database", "Username", NULL);

  // Get database user's password 
  password = g_key_file_get_string(cfile, "Database", "Password", NULL);

  // Get encoding 
  if ((encoding = g_key_file_get_string(cfile, "Database", "Encoding", NULL)) != NULL)
    encoding = "utf-8";


  // Get blobdir 
  if ((blobdir = g_key_file_get_string(cfile, "Database", "Blobdir", NULL)) == NULL )
    blobdir = g_strconcat(iconf->blobdir, "/", dbname, NULL);
    

  // Get Schema file 
  if ((schemafile = g_key_file_get_string(cfile, "Database", "Schema", NULL)) == NULL )
    schemafile = g_strconcat(iconf->sharedir, "/", "MgdObjects.xml", NULL);

  mgd_init();

  if ((mgd = mgd_connect(host, dbname, username, password)) != NULL) {

    mgd->blobdir = blobdir;
    mgd->schemafile = schemafile;
   
    //mgd_select_parser (mgd, encoding);
    mgd_select_parser(mgd, "russian");
    return mgd;

  }
  g_free(dbtype);
  g_free(dbname);
  g_free(username);
  g_free(password);
  g_free(encoding);
  g_free(blobdir);
  g_free(schemafile);

  return NULL;
}
*/

/* Workaround for g_key_file_get_string , we set all keys and values in GHashTable */

static GHashTable *__get_hash_from_config_file(const gchar *fname)
{
	gchar *content, **flines = NULL, **kval = NULL;
	gint ic = 0;
	GHashTable *chash;
	GError *err = NULL;
	
	if (g_file_get_contents(fname,  &content, NULL, &err) == FALSE) {
		g_warning("%s",	err->message);
		g_error_free (err);
		return NULL;
	}
	
	/* get file content line by line */ 
	flines = g_strsplit(content, "\n", -1);
	chash = midgard_hash_strings_new();
	
	while (flines[ic]) {
		
		/* This is our key=value entry */
		if (g_strstr_len(flines[ic], strlen(flines[ic]), "=") != NULL) {
			kval = g_strsplit(flines[ic], "=", -1);
			g_hash_table_insert(chash, g_strdup((gchar*)kval[0]),
					g_strdup((gchar*)kval[1]));    
			g_strfreev(kval);
		}
		ic++;
	}
	g_free(content);   
	g_strfreev(flines);
	
	return chash; 
}

gboolean midgard_config_read_file(MidgardConfig *self, const gchar *filename)
{
	mdirs *iconf;
	gchar *fname = NULL;
	gchar  *testunit, *tmpstr, *tablecreate, *tableupdate;
	GHashTable *chash;

	g_assert(self != NULL);
	
	if ((iconf = _get_config_dir()) == NULL)
		return FALSE;

	/* We get user's config file first */
	fname = g_build_path (G_DIR_SEPARATOR_S, 
			g_get_home_dir(), ".midgard/conf.d", filename, NULL);
			
	/* If user file doesn't exist we try to read from /etc */
	if (!g_file_test (fname, G_FILE_TEST_EXISTS)){
		g_free(fname);
		fname = g_strconcat(iconf->confdir, "/", filename, NULL);
	}

	chash = __get_hash_from_config_file(fname);
	g_free((gchar *)fname);

	if (chash == NULL) 
		return FALSE;

	/* Get database type */
	if ((tmpstr = g_hash_table_lookup(chash, "Type")) == NULL)
		tmpstr = "MySQL";
	self->private->dbtype = 1 ; /* FIXME */
	
	/* Get host name or IP */
	if ((tmpstr = g_hash_table_lookup(chash, "Host")) == NULL)
		tmpstr = "localhost";
	self->private->host = g_strdup(tmpstr);
			
	/* Get database name */
	if ((tmpstr = g_hash_table_lookup(chash, "Name")) == NULL)
		tmpstr = "midgard";
	self->private->database = g_strdup(tmpstr);
	
	/* Get database's username */
	if ((tmpstr = g_hash_table_lookup(chash, "Username")) == NULL)
		tmpstr = "midgard";
	self->private->dbuser = g_strdup(tmpstr);
	
	/* Get password for database user */
	if ((tmpstr = g_hash_table_lookup(chash, "Password")) == NULL)
		tmpstr = "midgard";
	self->private->dbpass = g_strdup(tmpstr);
	
	/* Get default language */
	if ((tmpstr = g_hash_table_lookup(chash, "DefaultLanguage")) == NULL) {
		tmpstr = "";
		self->private->default_lang = NULL;
	} else {
		self->private->default_lang = g_strdup(tmpstr);
	}
	
	/* Get blobs' path */
	if ((tmpstr = g_hash_table_lookup(chash, "Blobdir")) == NULL) {
		tmpstr = g_strconcat(iconf->blobdir, "/", self->private->database, NULL);
		self->private->blobdir = g_strdup(tmpstr);     
		g_free(tmpstr);
	} else {
		self->private->blobdir = g_strdup(tmpstr);
	}
	
	/* Get schema file */
	if ((tmpstr = g_hash_table_lookup(chash, "Schema")) == NULL){
		tmpstr = NULL;
	} else {
		if(g_ascii_strcasecmp(tmpstr, "") == 0) {
			g_warning("Schema directive used , but no file defined. Using default Schema." );
			tmpstr = NULL;
		}
	}
	self->private->schemafile = g_strdup(tmpstr);
	
	/* Get log filename */
	if ((tmpstr = g_hash_table_lookup(chash, "Logfile")) == NULL)
		tmpstr = NULL;
	self->private->logfilename = g_strdup(tmpstr);
	
	/* Get log level */
	if ((tmpstr = g_hash_table_lookup(chash, "Loglevel")) == NULL)
		tmpstr = "warn";
	self->private->loglevel = g_strdup(tmpstr);
	
	/* Get database creation mode */
	if ((tablecreate = g_hash_table_lookup(chash, "TableCreate")) == NULL)
		tablecreate = "false";

	/* Get database update mode */
	if ((tableupdate = g_hash_table_lookup(chash, "TableUpdate")) == NULL)
		tableupdate = "false";	

	/* Get SG admin username */
	if ((tmpstr = g_hash_table_lookup(chash, "MidgardUsername")) == NULL)
		tmpstr = NULL;
	self->private->mgdusername = g_strdup(tmpstr);

	/* Get SG admin password */
	if ((tmpstr = g_hash_table_lookup(chash, "MidgardPassword")) == NULL)
		tmpstr = NULL;
	self->private->mgdpassword = g_strdup(tmpstr);
	
	/* Get test mode */
	if ((testunit = g_hash_table_lookup(chash, "TestUnit")) == NULL)
		testunit = "false";

	/* FIXME, add trusted auth type and trigger error if unknown type 
	 * was defined */
	/* Get auth type */
	if ((tmpstr = g_hash_table_lookup(chash, "AuthType")) == NULL) {
		self->private->authtype = MGD_AUTHTYPE_NORMAL; 
	} else {
		if(g_str_equal(tmpstr, "PAM"))
			self->private->authtype = MGD_AUTHTYPE_PAM;
	}

	/* Get Pam file */
	if ((tmpstr = g_hash_table_lookup(chash, "PamFile")) == NULL) 
		tmpstr = "midgard";
	self->private->pamfile = g_strdup(tmpstr);

	/* Set boolean from dbcreate value string */
	self->private->tablecreate = FALSE; 
	tmpstr = g_ascii_strdown(tablecreate, -1);
	if(g_str_equal(tmpstr, "true"))
		self->private->tablecreate = TRUE;
	g_free(tmpstr);

	/* Set boolean from testunit value string */
	self->private->testunit = FALSE;
	tmpstr = g_ascii_strdown(testunit, -1);
	if(g_str_equal(tmpstr, "true"))
		self->private->testunit = TRUE;
	g_free(tmpstr);			 

	/* Set boolean from dbupdate value string */
	self->private->tableupdate = FALSE;
	tmpstr = g_ascii_strdown(tableupdate, -1);
	if(g_str_equal(tmpstr, "true"))
		self->private->tableupdate = TRUE;
	g_free(tmpstr);

	guint logspec = mgd_parse_log_levels(self->private->loglevel);
	mgd_init_ex(logspec, self->private->logfilename);

	g_hash_table_destroy(chash);
	g_free(iconf->confdir);
	g_free(iconf->blobdir);
	g_free(iconf->sharedir);
	g_free(iconf);
	
	return TRUE; 
}

midgard *midgard_config_init(gchar *filename)
{
	/* FIXME , unref this object when we 
	 * switch to midgard_config and midgard objects */
	MidgardConfig *config = g_object_new(MIDGARD_TYPE_CONFIG, NULL);
	
	if(!midgard_config_read_file(config, filename))
		return NULL;

	midgard *mgd = mgd_connect(
			config->private->host,
			config->private->database,
			config->private->dbuser,
			config->private->dbpass);

	if(mgd == NULL) 
		return NULL;

	mgd->logfile = NULL;	
	mgd->loglevel = mgd_parse_log_levels(config->private->loglevel);
	mgd->schemafile = g_strdup(config->private->schemafile);
	mgd->blobdir = g_strdup(config->private->blobdir);

	/* FIXME , holder should be assigned to mgd in this case and freed when 
	 * connection handler is freed */
	MidgardTypeHolder *holder = g_new(MidgardTypeHolder, 1);
	holder->level = mgd->loglevel;
	mgd->loghandler = g_log_set_handler(G_LOG_DOMAIN, mgd->loglevel,
			mgd_log_debug_default, (gpointer)holder);
	
	mgd->auth_type = config->private->authtype;
	mgd->pamfile = config->private->pamfile;

	/* Authenticate user if MidgardUsername is set */
	if(config->private->mgdusername && config->private->mgdpassword)
		mgd_auth(mgd, config->private->mgdusername
				, config->private->mgdpassword, 1);

	return mgd;
}
