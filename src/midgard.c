/* $Id: midgard.c 26773 2010-12-30 14:15:03Z piotras $
 *
 * midgard-lib: database access for Midgard clients
 *
 * Copyright (C) 1999 Jukka Zitting <jukka.zitting@iki.fi>
 * Copyright (C) 2000 The Midgard Project ry
 * Copyright (C) 2003 David Schmitter, Dataflow Solutions GmbH <schmitt@dataflow.ch>
 * Copyright (C) 2003-2004 Alexander Bokovoy <ab@altlinux.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include "midgard/midgard_connection.h"
#include "midgard/authfailure.h"
#include "defaults.h"
#include "fmt_russian.h"
#include "midgard/midgard_quota.h"
#if HAVE_CRYPT_H
# include <crypt.h>
#else
# define _XOPEN_SOURCE
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef WIN32
# include <windows.h>
#endif
#include <glib.h>
#include <ctype.h>
#ifndef WIN32
# include <unistd.h>
#endif
#include <signal.h>

#ifdef WIN32
# include <process.h>
# include "win32/flock.h"
# include "win32/win95nt.h"
#else
# include <sys/utsname.h>
# include <sys/file.h>
# include <sys/wait.h>
# include <netdb.h>
#endif

#include <sys/stat.h>

#if HAVE_MIDGARD_VC
# include <sys/time.h>
#endif

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN 255
#endif

#ifndef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN="midgard-core"
#endif

#include "midgard_mysql.h"
#include "midgard/midgard_legacy.h"
#include "midgard/midgard_datatypes.h"
#include "midgard/midgard_timestamp.h"
#include "midgard_core_object.h"
#include "midgard_core_query.h"

static FILE *_log_file = NULL;

typedef struct {
	guint name;
	guint id;
}_CacheIDS;

static _CacheIDS **id_list_new(guint n)
{
	if(n == 0)
		return NULL;
	
	_CacheIDS **ids = g_new(_CacheIDS *, n+1);
	
	guint i = 0;
	for (i = 0; i < n; i++) {
		ids[i] = g_new0(_CacheIDS, 1);
	}
	ids[n] = NULL;
	return ids;
}

static void id_list_insert(_CacheIDS **ids, guint index, 
		const gchar *name, guint id)
{
	g_assert(ids != NULL);
	
	GQuark qid = g_quark_from_string(name);	

	ids[index]->name = qid;
	ids[index]->id = id;
}

guint id_list_lookup(gpointer _ids, const gchar *name)
{
	g_assert(_ids != NULL);
	g_assert(name != NULL);

	_CacheIDS **ids = (_CacheIDS**)_ids;

	GQuark qid = g_quark_from_string(name);

	guint i = 0;

	while(ids[i] != NULL) {

		if(ids[i]->name == qid) 	
			return ids[i]->id;

		i++;
	}

	return 0;
}

static const gchar *id_list_lookup_by_id(_CacheIDS **ids, guint id)
{
	g_assert(ids != NULL);

	guint i = 0;

	while(ids[i] != NULL) {
		
		if(ids[i]->id == id)
			return g_quark_to_string(ids[i]->name);
		i++;
	}

	return NULL;
}

gboolean id_list_replace(gpointer idsptr, const gchar *old_name,
		const gchar *new_name, guint id)
{
	g_assert(idsptr != NULL);

	_CacheIDS **ids = (_CacheIDS **) idsptr;

	GQuark old_qid = g_quark_from_string(old_name);
	GQuark new_qid = g_quark_from_string(new_name);
	
	guint i = 0;

	while(ids[i] != NULL) {
		
		if(ids[i]->name == old_qid) {
			
			ids[i]->name = new_qid;
			ids[i]->id = id;

			return TRUE;
		}

		i++;
	}

	return FALSE;
}

void id_list_free(gpointer idsptr)
{
	g_assert(idsptr != NULL);

	_CacheIDS **ids = (_CacheIDS **) idsptr;

	guint i = 0;

	while(ids[i] != NULL) {
	
		if (ids[i] != NULL) {
			
			g_free(ids[i]);
			ids[i] = NULL;
		}

		i++;
	}
	
	g_free(ids);
	idsptr = NULL;
}

void mgd_legacy_load_sitegroup_cache(midgard *mgd)
{
	g_assert(mgd != NULL);

	/* This is probably the initial connection.
	 * Table sitegroup doesn't exist yet, so silently return.
	 * We'll load sitegroup cache later if needed. */
	if (!_midgard_core_query_table_exists(mgd->_mgd, "sitegroup"))
		return;

	gpointer sg_ids = mgd->_mgd->priv->sg_ids;

	if(sg_ids != NULL && !mgd->_mgd->priv->is_copy) 
		id_list_free(sg_ids);


	mgd->_mgd->priv->sg_ids = NULL;

	gchar *query = "SELECT id, name FROM sitegroup";
	midgard_res *res = mgd_query(mgd, query);
	guint i = 0, j;

	if (!res)
		return;

	j = mgd_rows(res);
	
	if(j == 0) {
		
		mgd_release(res);
		return;
	}

	sg_ids = (gpointer) id_list_new(j);
	
	while(mgd_fetch(res)){
		
		id_list_insert(sg_ids, i, 
				g_strdup((const gchar *)mgd_colvalue(res, 1)),
				(guint) mgd_sql2id(res, 0));
		i++;
	}

	mgd->_mgd->priv->sg_ids = (gpointer) sg_ids;	

	mgd_release(res);

	return;
}

void mgd_log_debug_none(const gchar *domain, GLogLevelFlags level,
      const gchar *msg, gpointer userdata)
{
}

void mgd_log_debug_default(const gchar *domain, GLogLevelFlags level,
      const gchar *msg, gpointer holder)
{
   int fno;
   gchar *level_ad = NULL;
   MidgardTypeHolder *th  = (MidgardTypeHolder *) holder;
   guint userlevel = th->level; 

   switch (level) {
      case G_LOG_FLAG_RECURSION:
         level_ad =  "RECURSION"; 
         break;

      case G_LOG_FLAG_FATAL:
         level_ad = "FATAL! ";
         break;

      case G_LOG_LEVEL_ERROR:
         level_ad =  "ERROR";
         break;

      case G_LOG_LEVEL_CRITICAL:
         level_ad = "CRITICAL ";
         break;

      case G_LOG_LEVEL_WARNING:
         level_ad =  "WARNING";
         break;

      case G_LOG_LEVEL_MESSAGE:
         level_ad = "m";
         break;

      case G_LOG_LEVEL_INFO:
         level_ad = "info";
         break;

      case G_LOG_LEVEL_DEBUG:
         level_ad = "debug";
         break;

      default:
         /* Should never happen , default log level should be set 
          * in midgard_config_init when unknown defined */
         level_ad =  "Unknown level";
         break;
   }

   if (userlevel >= level) {
       
       if (_log_file == NULL) _log_file = stderr;
       fno = fileno(_log_file);

       if (fno != 2) flock(fno, LOCK_EX);

       fprintf(_log_file, "%s ", domain != NULL ? domain : "midgard-core");
       /* I am not sure if we need pid */
       fprintf(_log_file, "(pid:%ld):", (unsigned long)getpid()); 
   
       fprintf(_log_file, "(%s):", level_ad);
       fprintf(_log_file, "  %s\n", msg);
       
       fflush(_log_file);
       if (fno != 2) flock(fno, LOCK_UN);
   }
}

void mgd_init()
{
   mgd_init_ex(0, NULL);
}

guint mgd_parse_log_levels(const char *levels)
{
   const char *c;
   guint mask = 0;
   int level = 0;

   for (c = levels; *c != '\0'; ) {
      /* skip over non-text */
      if (!isalpha(*c)) { c++; continue; }

      /* first char determines log level */
      switch (tolower(*c)) {
         /*[eeh]  I don't think we're supposed to offer these as log
          * level flags
            case 'r': level = G_LOG_FLAG_RECURSION; break;
            case 'f': level = G_LOG_FLAG_FATAL; break;
         */
         case 'e': level = G_LOG_LEVEL_ERROR; break;
         case 'c': level = G_LOG_LEVEL_CRITICAL; break;
         case 'w': level = G_LOG_LEVEL_WARNING; break;
         case 'm': level = G_LOG_LEVEL_MESSAGE; break;
         case 'i': level = G_LOG_LEVEL_INFO; break;
         case 'd': level = G_LOG_LEVEL_DEBUG; break;
         default: level = 4; break;
      }

      /* skip over rest of 'word' */
      for (c++; *c != '\0' && isalpha(*c); c++) {}

      /* last word was garbage, ignore, maybe we should log this */
      if (level == 0) continue;

      /* select single log level */
      if (*c == '+') {
         mask |= level;
         continue;
      }

      /* select loglevel and all higher loglevels */
      for (; level && level > G_LOG_FLAG_FATAL; level >>= 1) {
         mask |= level;
      }
      
   }

   return mask;
}

void mgd_init_ex(guint log_levels, char *logname)
{
	if (mgd_parser_list())
		return;
	/* MySQL expects utf8 instead of utf-8. 
	 * latin1 instead of iso-8859-1.
	 */ 	
        (void) mgd_parser_init_raw("utf-8", "UTF-8");

	if (logname == NULL) {
		_log_file = stderr;
	} else {
		_log_file = fopen(logname, "a");
		if (_log_file == NULL) {
			_log_file = stderr;
			g_log("midgard-core", G_LOG_LEVEL_WARNING,
					"Could not open logfile '%s', using stderr", logname);
		}
	}
}

void mgd_done()
{
	mgd_parser_free_all();
}

const char *mgd_version()
{
	return MIDGARD_LIB_VERSION;
}

midgard *__mgd_legacy_midgard_new()
{
	midgard *mgd;
	/* create midgard handle */
	mgd = g_new(midgard, 1);
	if (!mgd)
		return NULL;

	mgd->res = NULL;

	mgd->pool = mgd_alloc_pool();
	if (!mgd->pool) {
		free(mgd);
		return NULL;
	}

	mgd->tmp = mgd_alloc_pool();
	if (!mgd->tmp) {
		mgd_free_pool(mgd->pool);
		free(mgd);
		return NULL;
	}

	mgd->current_user = &mgd->user_at_pagestart;
	
	mgd->user_at_pagestart.id = 0;
	mgd->setuid_user.id = 0;
	mgd->user_at_pagestart.is_admin = 0;
	mgd->setuid_user.is_admin = 0;
	mgd->user_at_pagestart.is_root = 0;
	mgd->setuid_user.is_root = 0;
	mgd->user_at_pagestart.sitegroup = 0;
	mgd->setuid_user.sitegroup = 0;
	mgd->user_at_pagestart.member_of = NULL;
	mgd->setuid_user.member_of = NULL;

	mgd->username = NULL;

	mgd->parser = NULL;

	mgd->msql = NULL;	
	mgd->blobdir = NULL;

	mgd->ah_prefix = NULL;
	mgd->auth_type = MGD_AUTHTYPE_NORMAL;

	mgd->lang = 0;
	mgd->default_lang = 0;
	mgd->a_lang = 0;
	mgd->p_lang = 0;

	mgd->quota = 0;
	mgd->quota_err = 0;
	mgd->qlimits = NULL;

	mgd->loglevel = mgd_parse_log_levels("warn");
	mgd->logfile = NULL;
	mgd->loghandler = 0;
	mgd->person = NULL;
	mgd->schema = NULL;
	mgd->err = NULL;

	mgd->_mgd = midgard_connection_new();
	mgd->_mgd->mgd = mgd;
	mgd->pamfile = "midgard";
	
	mgd->is_copy = FALSE;

	return mgd;
}

midgard *mgd_legacy_midgard_working_copy_new(midgard *orig)
{
	midgard *mgd;
	/* create midgard handle */
	mgd = g_new(midgard, 1);
	if (!mgd)
		return NULL;

	mgd->res = NULL;

	mgd->pool = mgd_alloc_pool();
	if (!mgd->pool) {
		free(mgd);
		return NULL;
	}

	mgd->tmp = mgd_alloc_pool();
	if (!mgd->tmp) {
		mgd_free_pool(mgd->pool);
		free(mgd);
		return NULL;
	}

	mgd->current_user =  orig->current_user;
	
	mgd->user_at_pagestart.id = orig->user_at_pagestart.id;
	mgd->setuid_user.id = orig->setuid_user.id;
	mgd->user_at_pagestart.is_admin = orig->user_at_pagestart.is_admin;
	mgd->setuid_user.is_admin = orig->setuid_user.is_admin;
	mgd->user_at_pagestart.is_root = orig->user_at_pagestart.is_root;
	mgd->setuid_user.is_root = orig->setuid_user.is_root;
	mgd->user_at_pagestart.sitegroup = orig->user_at_pagestart.sitegroup;
	mgd->setuid_user.sitegroup = orig->setuid_user.sitegroup;
	mgd->user_at_pagestart.member_of = orig->user_at_pagestart.member_of;
	mgd->setuid_user.member_of = orig->setuid_user.member_of;

	mgd->username = orig->username;

	mgd->parser = orig->parser;

	mgd->msql = orig->msql;
	mgd->msql->mysql = orig->msql->mysql;
	mgd->blobdir = orig->blobdir;

	mgd->ah_prefix = orig->ah_prefix;
	mgd->auth_type = orig->auth_type;

	mgd->lang = orig->lang;
	mgd->default_lang = orig->default_lang;
	mgd->a_lang = orig->a_lang;
	mgd->p_lang = orig->p_lang;

	mgd->quota = orig->quota;
	mgd->quota_err = orig->quota_err;
	mgd->qlimits = orig->qlimits;

	mgd->loglevel = orig->loglevel;
	mgd->logfile = orig->logfile;
	mgd->loghandler = orig->loghandler;
	mgd->person = orig->person;
	mgd->schema = orig->schema;
	mgd->err = orig->err;

	mgd->_mgd = midgard_connection_new();
	mgd->_mgd->priv->configname = orig->configname;
	mgd->_mgd->mgd = mgd;

	/* Set language */
	if (orig->_mgd->priv->default_lang != NULL)
		mgd->_mgd->priv->default_lang = g_strdup(orig->_mgd->priv->default_lang);
	mgd->_mgd->priv->default_lang_id = orig->_mgd->priv->default_lang_id;

	if (orig->_mgd->priv->lang != NULL)
		mgd->_mgd->priv->lang = g_strdup(orig->_mgd->priv->lang);
	mgd->_mgd->priv->lang_id = orig->_mgd->priv->lang_id;

	/* Set sitegroup ids cache */
	mgd_legacy_load_sitegroup_cache(mgd);

	mgd->pamfile = orig->pamfile;

	mgd->is_copy = TRUE;
	mgd->_mgd->priv->is_copy = TRUE;
	
	return mgd;
}

void mgd_clear(midgard * mgd, int clearflags)
{
	assert(mgd != NULL);

	/* clear user information */
	mgd->current_user->id = 0;
	mgd->current_user->is_admin = 0;
	mgd->current_user->is_root = 0;

   	if (clearflags & MGD_CLEAR_SITEGROUP) 
		mgd->current_user->sitegroup = 0; 

	mgd->username = NULL;

	if (mgd->current_user->member_of != NULL)
		g_free(mgd->current_user->member_of);

	mgd->current_user->member_of = NULL;

	/* we automaticaly select first parser to avoid confusion */
	if (clearflags & MGD_CLEAR_PARSER) 
		mgd->parser = mgd_parser_list(); 

	/* clear resources reserved */
	while (mgd->res) mgd_release(mgd->res);

	mgd->person = NULL;	
}

void mgd_close(midgard * mgd)
{
	assert(mgd != NULL);
	
	mgd_clear(mgd, MGD_CLEAR_ALL);

	if (mgd->user_at_pagestart.member_of != NULL)
		g_free(mgd->user_at_pagestart.member_of);
	if (mgd->setuid_user.member_of != NULL)
		g_free(mgd->setuid_user.member_of);

	if(mgd->tmp != NULL)
		mgd_free_pool(mgd->tmp);
	mgd->tmp = NULL;

	if(mgd->pool != NULL)
		mgd_free_pool(mgd->pool);
	mgd->pool = NULL;

	/* Free low level members */
	if(!mgd->is_copy) {
		
		if(mgd->msql->mysql) {
			
			mysql_close(mgd->msql->mysql);
			mgd->msql->mysql = NULL;
			
			g_free(mgd->msql);
			mgd->msql = NULL;

			g_free(mgd->configname);
		}
	}

	if(mgd->_mgd != NULL && G_IS_OBJECT(mgd->_mgd))
		g_object_unref(mgd->_mgd);
	mgd->_mgd = NULL;

	if (mgd->blobdir)
		g_free(mgd->blobdir);
	mgd->blobdir = NULL;

	g_free(mgd);
}

midgard *mgd_setup()
{
	midgard *mgd = __mgd_legacy_midgard_new();
	
	mgd->parser = mgd_parser_list();
	mgd->msql = g_new(midgard_mysql, 1);

	mgd->current_user = &mgd->user_at_pagestart;

	return mgd;
}

void mgd_set_authtype(midgard *mgd, midgard_auth_type authtype)
{
	mgd->auth_type = authtype;
}

void mgd_easy_connect(midgard *mgd, const char *host, const char *database,
      const char *user, const char *password)
{
	int connected;

	/* create mysql connection handle */
	mgd->msql->mysql = mysql_init(NULL);

        /* Set the correct character encoding, UTF-8 by default. Note */
        /* that Midgard uses "utf-8" where MySQL uses "utf8".         */
        if (mgd->msql->mysql != NULL) 
	        mysql_options(mgd->msql->mysql, MYSQL_SET_CHARSET_NAME, "utf8");
        

	/* connect to the database */
   	connected = mgd->msql->mysql &&
      	mysql_real_connect(mgd->msql->mysql, host,
			user, password, database, 0, NULL, 0);

	if (!mgd->msql->mysql || !connected) {
		
		/* EEH/TODO: I know this is ugly but we need this info.
		 * There will be proper logging at some point  */
		
		g_warning(" %s@%s://%s connection failed: %s",
				user, host, database,
				mgd->msql->mysql ? mysql_error(mgd->msql->mysql) : "<mysql server not found?>");
		
		if (mgd->msql->mysql) mysql_close(mgd->msql->mysql);
		mgd->msql->mysql = NULL;

		return;
	}

	mgd_legacy_load_sitegroup_cache(mgd);
}

midgard *mgd_connect(const char *hostname, const char *database,
	     const char *username, const char *password)
{
	midgard *mgd = mgd_setup();

	if (mgd == NULL)
		return NULL;

/* use default values if none specified */
	if (!hostname)
		hostname = MGD_MYSQL_HOST;
	if (!username)
		username = MGD_MYSQL_USERNAME;
	if (!password)
		password = MGD_MYSQL_PASSWORD;
	if (!database)
		database = MGD_MYSQL_DATABASE;

/* create mysql connection handle */
	mgd_easy_connect(mgd, hostname, database, username, password);

	if (!mgd->msql->mysql) {
		mgd_free_pool(mgd->tmp);
		mgd_free_pool(mgd->pool);
		free(mgd);
		return NULL;
	}

	/* Workaround, we need to set it as dbus services namespace */
	mgd->_mgd->priv->configname = g_strdup(database);
	mgd->configname = g_strdup(database);

	return mgd;
}

int mgd_select_db(midgard * mgd, const char *database)
{	
	assert(mgd != NULL);
	assert(database != NULL);

	gint errcode;
	guint i = 0;
	
	errcode = mysql_select_db(mgd->msql->mysql, database);
       
	if (errcode == 0) 
		return 1;

	do {

		errcode = mgd_select_db(mgd, database);
		if(errcode == 1)
			return errcode;

		sleep(1);
		i++;
	
	} while(i < 3);
		
	g_critical("Midgard: mgd_select_db(%s) failed: %s (%d)\n",
			database, mysql_error(mgd->msql->mysql), errcode);
	return 0;
}

static int __reconnect(midgard *mgd, const char *hostname, const char *database,
		const char* username, const char *pass)
{
 	/* sleep(1); */
	mgd_easy_connect(mgd, hostname, database, username, pass);

	if(!mgd->msql)
		return 0;

	if(!mgd->msql->mysql)
		return 0;

	return 1;
}


gboolean
mgd_reopen (midgard *mgd, const char *hostname, const char *database,
		const char* username, const char *pass, int n_try, int sleep_seconds)
{
	if (mgd == NULL)
		return FALSE;

	if (!mgd->msql)
		return FALSE;

	if (!mgd->msql->mysql)
		return FALSE;

	/* Dummy query */
	int sq = mysql_query (mgd->msql->mysql, "SELECT 1");
	
	if (sq == 0) 
		return TRUE;

	/* Let's assume installed MySQL is new enough to provide server gone away constant */
	if (mysql_errno (mgd->msql->mysql) != 2006) /* CR_SERVER_GONE_ERROR */
		return TRUE;

	int i = 0;

	do {
		sleep (sleep_seconds);
		mysql_close(mgd->msql->mysql);
		mgd_easy_connect (mgd, hostname, database, username, pass);

		if (!mgd->msql)
			return FALSE;

		if (!mgd->msql->mysql)
			return FALSE;
		
		return TRUE;
	} while (i < n_try);

	return FALSE;
}

int mgd_assert_db_connection(midgard *mgd, const char *hostname, const char *database,
const char* username, const char *pass)
{
	void (*handler) (int);

	handler=signal(SIGPIPE, SIG_IGN);

	/* we can not do any assertion here! */
	if(!mgd->msql)
		return __reconnect(mgd, hostname, database, username, pass);

	if(!mgd->msql->mysql)
		return __reconnect(mgd, hostname, database, username, pass);

/* EEH: ensure that the link did not die, from PHPs mysql connector */
#if defined(CR_SERVER_GONE_ERROR)
	mysql_stat(mgd->msql->mysql);
	if (mysql_errno(mgd->msql->mysql) == CR_SERVER_GONE_ERROR) {
#else
	if (!strcasecmp(mysql_stat(mgd->msql->mysql), "mysql server has gone away")) {
#endif
		mysql_close(mgd->msql->mysql);
		sleep(1);
		mgd_easy_connect(mgd, hostname, database, username, pass);
	}

	signal(SIGPIPE, handler);
	return (mgd->msql->mysql != NULL);
}

#ifdef HAVE_LIBPAM

#ifdef HAVE_SECURITY_PAM_APPL_H
#include <security/pam_appl.h>
#endif
/* Mac OS X puts PAM headers into /usr/include/pam, not /usr/include/security */
#ifdef HAVE_PAM_PAM_APPL_H
#include <pam/pam_appl.h>
#endif

typedef struct {
char *username;
char *password;
} _midgard_pam_appdata;

static int _midgard_pam_conv(int num_msg, const struct pam_message **msg, 
		struct pam_response **resp, void *appdata_ptr)
{
	_midgard_pam_appdata *appdata = (_midgard_pam_appdata*) appdata_ptr;
	int i = 0, j = 0;

	if (num_msg && msg && *msg && resp && appdata_ptr) {
		*resp = malloc(sizeof(struct pam_response)*num_msg);
		if (*resp) {
			/* Process conversation and fill in results */
			for(; i < num_msg; i++) {
				(*resp)[i].resp_retcode = 0;
				(*resp)[i].resp = NULL;
				switch((*msg)[i].msg_style) {
					
					case PAM_PROMPT_ECHO_ON: /* username */
						(*resp)[i].resp = strdup(appdata->username);
						break;
					
					case PAM_PROMPT_ECHO_OFF: /* password */
						(*resp)[i].resp = strdup(appdata->password);
						break;
				}

				/* If there was an error during strdup(), 
				 * clean up already allocated  structures and return PAM_CONV_ERR */
				if (!(*resp)[i].resp) {
					for(j = i; j >= 0 ; j--) {
						if ((*resp)[j].resp)
							free((*resp)[j].resp);
					}
					free(*resp);
					*resp = NULL;
					g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, 
							"Return PAM_CONV_ERROR due to strdup() failure");
					return PAM_CONV_ERR;
				}
			}
			g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Return PAM_SUCCESS");
			return PAM_SUCCESS;
		}
	}
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, "Return PAM_CONV_ERROR");
	return PAM_CONV_ERR;
}
#endif

#define MGD_USERNAME_MAXLEN 80

static int mgd_auth_su(midgard * mgd, const char *username,
		       const char *password, int su, int setuid, int trusted, int sg0)
{
	midgard_res *res;
	int i;
	const char *cipher, *passwd;
	int host_sitegroup = 0;
	int req_sitegroup, admingroup;
	char *sitegroup;
	int rootuser = 0;
	char delim = '+';
	int grant;
	char *reauth = NULL;	

	assert(mgd);

	if (mgd->msql->mysql == NULL)
		return MGD_AUTH_NOT_CONNECTED;

	if (!sg0)
		host_sitegroup = mgd->current_user->sitegroup;
	mgd_clear(mgd, MGD_CLEAR_BASE);

	if (setuid) {
		mgd->current_user = &mgd->setuid_user;
		mgd_clear(mgd, MGD_CLEAR_BASE);
		mgd->current_user->sitegroup = host_sitegroup;
	}

	/* anonymous */
	if (!username || !*username) {
		return MGD_AUTH_ANONYMOUS;
	}

	guint username_length = strlen(username);
	
	if(username_length > MGD_USERNAME_MAXLEN
			|| username_length < 1)
		return MGD_AUTH_INVALID_NAME;

#define MIDGARD_LOGIN_SG_ROOT_SEP "*!$"

#define MIDGARD_LOGIN_SG_ROOT_ROOT	'*'
#define MIDGARD_LOGIN_SG_ROOT_ADMIN	'!'
#define MIDGARD_LOGIN_SG_ROOT_USER	'$'
#define MIDGARD_LOGIN_SG_ADMIN_USER	';'
#define MIDGARD_LOGIN_SG_USER_USER	'+'
#define MIDGARD_LOGIN_SG_REAUTH		'='

	gchar *uname = g_strdup(username);
	
	sitegroup = strpbrk(uname, MIDGARD_LOGIN_SG_SEPARATOR);

	if (sitegroup) {
		delim = *sitegroup;
		*sitegroup = '\0';
		sitegroup++;
		rootuser = (strchr(MIDGARD_LOGIN_SG_ROOT_SEP, delim) != NULL);

		if (rootuser && !*sitegroup) return MGD_AUTH_SG_NOTFOUND;

		goto SG_MAGIC_DONE;

	} else if (host_sitegroup != 0) {
		rootuser = 0;
		goto SG_MAGIC_DONE;
	} else if (host_sitegroup == 0) {
		delim = MIDGARD_LOGIN_SG_ROOT_ROOT;
		rootuser = 1;
		goto SG_MAGIC_DONE;
	} else if (!sitegroup && host_sitegroup == 0) {
		delim = MIDGARD_LOGIN_SG_ROOT_ROOT;
		rootuser = 1;
		goto SG_MAGIC_DONE;
	} else if (!sitegroup) {
		delim = MIDGARD_LOGIN_SG_ROOT_ROOT;
		rootuser = 1;
		goto SG_MAGIC_DONE;
	} else if (sitegroup && *sitegroup == '\0') {
		delim = MIDGARD_LOGIN_SG_ROOT_ROOT;
		rootuser = 1;
		goto SG_MAGIC_DONE;
	} else {
		delim = MIDGARD_LOGIN_SG_USER_USER;
		rootuser = 0;
		goto SG_MAGIC_DONE;
	}

SG_MAGIC_DONE:

	reauth = strchr(uname, MIDGARD_LOGIN_SG_REAUTH);
	if (reauth) {
		*reauth = '\0';
		reauth++;
	}

	if ((su == 1) && (sitegroup || reauth)) {
		g_free(uname);
		return MGD_AUTH_REAUTH;
	}

	if (sitegroup && *sitegroup) {
		res =
		   mgd_ungrouped_select(mgd, "id,admingroup", "sitegroup",
					"name=$q", NULL, sitegroup);

		/* sitegroup must be found */
		if (!res || !mgd_fetch(res)) {
			if (res)
				mgd_release(res);
			g_free(uname);
			return MGD_AUTH_SG_NOTFOUND;
		}

		req_sitegroup = mgd_sql2id(res, 0);
		admingroup = mgd_sql2id(res, 1);
		mgd_release(res);
	}
	else {
		req_sitegroup = host_sitegroup;
		admingroup = 0;

		res =
		   mgd_ungrouped_select(mgd, "admingroup", "sitegroup",
					"id=$d", NULL, host_sitegroup);

		if (res) {
			if (mgd_fetch(res))
				admingroup = mgd_sql2id(res, 0);
			mgd_release(res);
		}
	}

	if (rootuser) {
		res = mgd_ungrouped_select(mgd, "person.id,person.password",
					   "person,member",
					   "person.username=$q AND person.sitegroup=0 "
					   "AND member.uid = person.id AND member.gid=0 "
					   "AND person.metadata_deleted=0 ",
					   NULL, uname);
	}
	else {
		res = mgd_ungrouped_select(mgd, "id,password", "person",
					   "username=$q AND sitegroup=$d "
					   "AND person.metadata_deleted=0 ",
					   NULL, uname, req_sitegroup);
	}

	/* username not found */
	if (!res) {
		g_free(uname);
		return MGD_AUTH_NOTFOUND;
	}

	/* only one user may match */
	if (mgd_rows(res) != 1) {
      mgd_release(res);
		return MGD_AUTH_DUPLICATE;
	}

	/* username not found */
	if (!res) {
		g_free(uname);
		return MGD_AUTH_NOTFOUND;
	}

	while (mgd->current_user->id == 0 && mgd_fetch(res)) {
		
		switch(mgd->auth_type) {
		case MGD_AUTHTYPE_NORMAL:
		
			/* Ugly workaround for compiler warning message.
			 * 'fallback' label is available in function scope , so 
			 * no warning should be thrown. */
			if( 0 > 1)
				goto fallback;

fallback:
			if(!trusted) {

				g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, 
						"Authentication type: NORMAL");

				passwd = mgd_colvalue(res, 1);

				/* Empty password found. */
				if (passwd == NULL || *passwd == '\0') {

					g_debug("Found empty password. Can not authenticate.");
					g_free(uname);

					if (res)
						mgd_release(res);

					return MGD_AUTH_INVALID_PWD;
				}
				
				if (passwd[0] == '*' && passwd[1] == '*') {
					
					passwd += 2;
					cipher = password;
				
				} else {
					
					cipher = (const char *)crypt(password, passwd);
				}

				if (su || !strcmp(cipher, passwd))
					mgd->current_user->id = mgd_sql2id(res, 0);

			} else {
				
				mgd->current_user->id = mgd_sql2id(res, 0);
			}

			break;
#ifdef HAVE_LIBPAM
		case MGD_AUTHTYPE_PAM:
			/* Authenticate against PAM source. We use predefined PAM service 'midgard' as auth source */
			{
				_midgard_pam_appdata appdata = {
					(char*) uname, 
					(char*) password
				};
				
				struct pam_conv pconv = {
					_midgard_pam_conv,
					&appdata
				};

				pam_handle_t *phandle = NULL;
				
				int result = pam_start(mgd->pamfile, uname, &pconv, &phandle);
				g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, 
						"PAM start result: %d", result);
				
				if (result == PAM_SUCCESS) 
					result = pam_authenticate(phandle, 0);
				g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, 
						"PAM authenticate result: %d", result);
				if (su || (result == PAM_SUCCESS)) {
					mgd->current_user->id = mgd_sql2id(res, 0);
					g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
							"PAM auth was successful, new user id: %d", mgd->current_user->id);
				}
				result = pam_end(phandle, su ? PAM_SUCCESS : result);
				g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO,
						"PAM end result: %d", result);
				if (mgd->current_user->id == 0) {
					/* Fallback to normal authentication */
					g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, 
							"PAM auth was unsuccessful, going to normal auth");
					goto fallback;
	    		  }
			}
		break;
#endif
		default:
			g_error("Unsupported midgard authentication type %d", mgd->auth_type);
		}
	}

	mgd_release(res);

	if (mgd->current_user->id == 0) {
		g_free(uname);
		return MGD_AUTH_INVALID_PWD;
	}

	/* Get group information */
	res = mgd_ungrouped_select(mgd, "gid", "member",
				   "uid=$i AND sitegroup in (0, $i) AND metadata_deleted=0",
				   "gid DESC", mgd->current_user->id, req_sitegroup);

	if (res) { 
		mgd->current_user->member_of = 
			(int *) realloc(mgd->current_user->member_of,
					sizeof (int) * (mgd_rows(res) + 1));
		i = 0;
		while (mgd_fetch(res)) {
			if ((mgd->current_user->member_of[i] = mgd_sql2id(res, 0)) != 0) {
				if (mgd->current_user->member_of[i] == admingroup)
					mgd->current_user->is_admin = 1;
				i++;
			} else {
				mgd->current_user->is_admin = 1;
				mgd->current_user->is_root = 1;
			}
		}

		mgd->current_user->member_of[i] = 0;
		mgd_release(res);
	}

	grant = (rootuser && mgd->current_user->is_root);
	if (!grant && !rootuser && !mgd->current_user->is_root) {
		if (host_sitegroup != 0 && req_sitegroup == host_sitegroup)
			grant = 1;
		if (host_sitegroup == 0 && req_sitegroup != 0)
			grant = 1;
	}

	if (!grant) {
		mgd_clear(mgd, MGD_CLEAR_BASE);
	}
	else {
		/* Change current sitegroup */
		mgd->current_user->sitegroup = req_sitegroup;
	}

	switch (delim) {
	case MIDGARD_LOGIN_SG_ROOT_USER:
	case MIDGARD_LOGIN_SG_ADMIN_USER:
		mgd->current_user->is_admin = 0;
	case MIDGARD_LOGIN_SG_ROOT_ADMIN:
         	mgd->current_user->is_root = 0;
	}

	if (su == 1) {
		if (!grant || mgd->current_user->sitegroup == 0
            || !mgd->current_user->is_admin || !reauth) {
			mgd_clear(mgd, MGD_CLEAR_BASE);
		}
		else {
			/* this depends on mgd->sitegroup already being set */
			mgd_auth_su(mgd, reauth, "", 1, 0, FALSE, FALSE);
		}
	}

	/* This is not handled by legacy API at all. Return 0 and do not set any user or person.
	 * current_user->id = 0 comes from mgd_clear which is invoked for some cases.
	 * Do not check group id which comes from memebr table, do not check sitegroup as we still
	 * can be in not sitegrouped mode. */ 
	if (mgd->current_user->id == 0) {

		g_free (uname);
		return 0;
	}

	GValue idval = {0, };
	g_value_init(&idval, G_TYPE_UINT);
	g_value_set_uint(&idval, (guint)mgd->current_user->id);
	MgdObject *pobject = 
		midgard_object_new(mgd,	"midgard_person", &idval);		
	g_value_unset(&idval);
	
	if(mgd->person && G_IS_OBJECT(mgd->person))
		g_object_unref(mgd->person);

	mgd->person = pobject;
	
	if(!mgd->person) {
		
		g_warning("Couldn't fetch person object for given '%s' name.", uname);
		g_free(uname);
		return MGD_AUTH_INVALID_NAME;
	}

	/* paranoid */
	if(!G_IS_OBJECT(mgd->person)) {

		g_warning("Expected Midgard Object person for given '%s' name.", uname);
		g_free(uname);
		return MGD_AUTH_INVALID_NAME;
	}

	midgard_connection_unref_implicit_user(mgd->_mgd);
	//if(mgd->_mgd->priv->user && G_IS_OBJECT(mgd->_mgd->priv->user))
	//	g_object_unref(mgd->_mgd->priv->user);

	mgd->_mgd->priv->user = G_OBJECT(midgard_user_new(mgd->person));

	/* TODO, check if it might affect midgard-apache basic auth and midgard-php 
	 * not yet being initialized data */
	if (sitegroup && *sitegroup)
		midgard_connection_set_sitegroup(mgd->_mgd, sitegroup);

	g_free(uname);

	return mgd->current_user->id;
}

int mgd_auth(midgard * mgd,
      const char *username, const char *password, int setuid)
{
	return mgd_auth_su(mgd, username, password, 0, setuid, FALSE, FALSE);
}

int mgd_auth_sg0(midgard * mgd,
      const char *username, const char *password, int setuid)
{
	return mgd_auth_su(mgd, username, password, 0, setuid, FALSE, TRUE);
}

int mgd_auth_trusted(midgard *mgd, const char *username) {
        /* TODO: The mgd_auth_su() method needs to be refactored.    */
        /* Until that is done, we just add *another* special         */
        /* argument to overload the default behaviour. Not good. :-( */
        return mgd_auth_su(mgd, username, NULL, 0, FALSE, TRUE, FALSE);
}

int mgd_auth_is_setuid(midgard * mgd)
{
   return (mgd->current_user == &mgd->setuid_user);
}

MGD_API int mgd_auth_unsetuid(midgard * mgd, long int rsg)
{
  if (mgd->current_user == &mgd->user_at_pagestart) return 0;
  mgd->current_user = &mgd->user_at_pagestart;
  mgd->current_user->sitegroup = rsg;
  g_log("midgard-lib", G_LOG_LEVEL_DEBUG, "Unset for SG: %d", (int) rsg);
  return 1;
}

int mgd_errno(midgard * mgd)
{
	assert(mgd);
	return mysql_errno(mgd->msql->mysql);
}

const char *mgd_error(midgard * mgd)
{
	assert(mgd);
	return mysql_error(mgd->msql->mysql);
}

int mgd_select_parser(midgard * mgd, const char *name)
{
	return mgd_parser_activate(mgd, name);
}

const char *mgd_get_parser_name(midgard * mgd)
{
	if (mgd && mgd->parser)
		return mgd->parser->name;
	return NULL;
}

const char *mgd_get_encoding(midgard * mgd)
{
	if (mgd && mgd->parser)
		return mgd->parser->encoding;
	return NULL;
}

int mgd_mail_need_qp(midgard * mgd)
{
	if (mgd && mgd->parser)
		return mgd->parser->need_qp;
	return 0;
}

void mgd_set_sitegroup(midgard * mgd, int sitegroup)
{
	g_assert(mgd != NULL);
	mgd->current_user->sitegroup = sitegroup;

	_CacheIDS **ids = (_CacheIDS**) mgd->_mgd->priv->sg_ids;

	if (ids == NULL) {

		g_warning("Sitegroup cache not initialized");
		return;
	}

	const gchar *name = id_list_lookup_by_id(ids, sitegroup);

	midgard_connection_set_sitegroup(mgd->_mgd, name);
}

int mgd_sitegroup(midgard * mgd)
{
	assert(mgd);
	return mgd->current_user->sitegroup;
}

int mgd_user(midgard * mgd)
{
	assert(mgd);
	return mgd->current_user->id;
}

int mgd_isauser(midgard * mgd)
{
	assert(mgd);
	return (mgd->current_user->id != 0);
}

int mgd_isadmin(midgard * mgd)
{
	assert(mgd);
	return mgd->current_user->is_admin;
}

int mgd_isroot(midgard * mgd)
{
	assert(mgd);
	return mgd->current_user->is_root;
}

void mgd_force_root(midgard * mgd)
{
	assert(mgd);
	mgd->current_user->is_root = 1;
	mgd->current_user->is_admin = 1;
	mgd->current_user->id = 0;
}

void mgd_force_admin(midgard * mgd)
{
	assert(mgd);
	mgd->current_user->is_admin = 1;
	mgd->current_user->id = 0;
}


#if HAVE_MIDGARD_MULTILANG
int mgd_lang(midgard * mgd)
{
	assert(mgd != NULL);
	return mgd->lang;
}

void mgd_set_default_lang(midgard *mgd, int lang)
{
	g_assert(mgd);
	mgd->default_lang = lang;

	if(mgd->_mgd && G_IS_OBJECT (mgd->_mgd))
		mgd->_mgd->priv->default_lang_id = lang;
}

int mgd_get_default_lang(midgard *mgd)
{
	g_assert(mgd != NULL);	
	return mgd->default_lang;
}

int mgd_parameters_defaultlang(midgard * mgd)
{
	assert(mgd);
	return mgd->p_lang;
}
int mgd_attachments_defaultlang(midgard * mgd)
{
	assert(mgd);
	return mgd->a_lang;
}

#endif /* HAVE_MIDGARD_MULTILANG */
int mgd_isuser(midgard * mgd, int user)
{
	assert(mgd);
	return (mgd->current_user->id == user);
}

int mgd_ismember(midgard * mgd, int group)
{
	int i;
   int *member_of;

	assert(mgd);

   member_of = mgd->current_user->member_of;

	if (member_of == NULL) return 0;

	for (i = 0; member_of[i]; i++) if (member_of[i] == group) return 1;

	return 0;
}

int *mgd_groups(midgard * mgd)
{
	return mgd->current_user->member_of;
}

midgard_res *mgd_query(midgard * mgd, const char *query, ...)
{
	midgard_res *res;
	va_list args;
	va_start(args, query);
	res = mgd_vquery(mgd, query, args);
	va_end(args);
	return res;
}

midgard_res *mgd_vquery(midgard * mgd, const char *query, va_list args)
{
	midgard_res *res;
	midgard_pool *pool = NULL;
	MYSQL_RES *mres;
	int rv;
	char *fquery = (gchar *)query;
	assert(mgd);
	assert(query);
	
	if (mgd->msql->mysql == NULL)
		return NULL;

	/* format and send query, only if args is not NULL */
	if(args != NULL){
		
		pool = mgd_alloc_pool();
		if (!pool)
			return NULL;
		fquery = mgd_vformat(mgd, pool, query, args);
		if (!fquery) {
			mgd_free_pool(pool);
			return NULL;
		}
	}

   	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", fquery); 

	rv = mysql_query(mgd->msql->mysql, fquery);
	
	if (rv != 0) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_WARNING,
				"QUERY FAILED: \n"
				"%s \n"
				"QUERY: \n"
				"%s", 
				mysql_error(mgd->msql->mysql), fquery);
		return NULL;
	}

	if(pool)
		mgd_free_pool(pool);	
	
	/* get results */
	mres = mysql_store_result(mgd->msql->mysql);
	if (!mres)
		return NULL;
	if (mysql_num_rows(mres) == 0) {
		mysql_free_result(mres);
		return NULL;
	}
	/* create result handle */
	res = g_new(midgard_res, 1);
	if (!res) {
		mysql_free_result(mres);
		return NULL;
	}
	res->pool = mgd_alloc_pool();
	if (!res->pool) {
		free(res);
		mysql_free_result(mres);
		return NULL;
	}
	res->mgd = mgd;
	res->msql = g_new(midgard_mysql, 1);
	res->msql->res = mres;
	res->msql->row = NULL;
	res->rown = 0;
	res->rows = mysql_num_rows(mres);	
	res->cols = mysql_num_fields(mres);
   	res->msql->fields = mysql_fetch_fields(mres);

	/* insert into result chain */
	if (mgd->res)
		mgd->res->prev = res;
	res->prev = NULL;
	res->next = mgd->res;
	mgd->res = res;

	return res;
}

midgard_res *mgd_ungrouped_select(midgard * mgd,
				  const char *fields, const char *from,
				  const char *where, const char *sort, ...)
{
	midgard_res *res;
	va_list args;
	va_start(args, sort);
	res = mgd_ungrouped_vselect(mgd, fields, from, where, sort, args);
	va_end(args);
	return res;
}

midgard_res *mgd_sitegroup_select(midgard * mgd,
				  const char *fields, const char *from,
				  const char *where, const char *sort, ...)
{
	midgard_res *res;
	va_list args;
	va_start(args, sort);
	res = mgd_sitegroup_vselect(mgd, fields, from, where, sort, args);
	va_end(args);
	return res;
}

midgard_res *mgd_ungrouped_vselect(midgard * mgd,
				   const char *fields, const char *from,
				   const char *where, const char *sort,
				   va_list args)
{
	midgard_res *res;
	assert(fields && from);
	GString *query = g_string_new("SELECT ");
	va_list aq;
	
	/* format query */
	if (where)
		if (sort)
			g_string_append_printf(query,
					"%s FROM %s WHERE %s ORDER BY %s",
					fields, from, where, sort);
		else
			g_string_append_printf(query,
					"%s FROM %s WHERE %s",
					fields, from, where);
		
	else if (sort)
		g_string_append_printf(query,
				"%s FROM %s ORDER BY %s",
				fields, from, sort);
	else
		g_string_append_printf(query,
				"%s FROM %s ",
				fields, from);
		

	gchar *tmpstr = g_string_free(query, FALSE);
	/* execute query */
	va_copy(aq, args);
	res = mgd_vquery(mgd, tmpstr, aq);
	va_end(aq);
	g_free(tmpstr);

	return res;
}

/* EmHe: add support for named tables at some point */
char *mgd_sitegroup_clause(midgard * mgd, midgard_pool * pool,
			   const char *tables)
{
	char *table , *tmptable;
	const char *sgclause = NULL;

	if (mgd->current_user->is_root) {
		return g_strdup("");
	}
	
	tmptable = g_strdup(tables);
	table = strtok((gchar *)tmptable, " ,");	

	while (table && !*table)
		table = strtok(NULL, " ,");

	if (!table) {	
		return g_strdup("AND 0=1");
	}			/* EmHe: this is an error */

	/* workaround for mgd_get_object_by_guid */
	if(g_str_equal(table, "repligard")) {
		sgclause = mgd_format(mgd, pool, 
				" AND ($s.sitegroup in (0,$d) ",
				table, mgd->current_user->sitegroup);
		g_free(table);
		return mgd_format(mgd, pool, "$s)", sgclause);
	}

	if (!strstr(table, "_i")) {
		sgclause = mgd_format(mgd, pool, 
				"AND $s.metadata_deleted = FALSE AND ($s.sitegroup in (0,$d) ",
				table, table, mgd->current_user->sitegroup);
		g_free(table);
		gchar *_clause = mgd_format(mgd, pool, "$s)", sgclause);	
		return _clause;
	} else {
		sgclause = mgd_strdup(pool, "1=1");
	}

	if (!sgclause) {
		return g_strdup("AND 0=2");
	}			/* EmHe: this is an error */

	for (table = strtok(NULL, " ,"); table; table = strtok(NULL, " ,")) {
		if (!*table)
			continue;
		
		if (!strstr(table, "_i")) {
			sgclause =
				mgd_format(mgd, pool, 
						"$s AND $s.metadata_deleted = FALSE"
						" AND $s.sitegroup in (0, $d)",
						sgclause, table, table, 
						mgd->current_user->sitegroup);
		}
		
		if (!sgclause) {
			return g_strdup("AND 0=3");
		}		/* EmHe: this is an error */
	}

	g_free(table);
	return mgd_format(mgd, pool, "$s)", sgclause);
}

midgard_res *mgd_sitegroup_vselect(midgard * mgd,
				   const char *fields, const char *from,
				   const char *where, const char *sort,
				   va_list args)
{
	midgard_res *res;
	const char *query;
	midgard_pool *pool;
	const char *sgclause;
	assert(fields && from);
	pool = mgd_alloc_pool();
	if (!pool)
		return NULL;

	sgclause = mgd_sitegroup_clause(mgd, pool, from);

	if (sort)
		query = mgd_format(mgd, pool,
				   "SELECT $s FROM $s WHERE ($s) $s"
				   " ORDER BY $s",
				   fields, from, where ? where : "1=1",
				   sgclause, sort);
	else
		query = mgd_format(mgd, pool,
				   "SELECT $s FROM $s WHERE ($s) $s",
				   fields, from, where ? where : "1=1",
				   sgclause);	
	
	if (!query) {
		mgd_free_pool(pool);
		return NULL;
	}

	/* execute query */
	res = mgd_vquery(mgd, query, args);	

	mgd_free_pool(pool);
	return res;
}

midgard_res *mgd_ungrouped_record(midgard * mgd,
				  const char *fields, const char *table, int id)
{
	midgard_res *res;
	const char *query;
	midgard_pool *pool;
	pool = mgd_alloc_pool();
	if (!pool)
		return NULL;

	/* format query */
#if ! HAVE_MIDGARD_MULTILANG
	query = mgd_format(mgd, pool, "SELECT $s FROM $s WHERE id=$i",
			   fields, table, id);
#else /* HAVE_MIDGARD_MULTILANG */
	query = mgd_format(mgd, pool, "SELECT $s FROM $s WHERE $s.id=$i",
			   fields, table, table, id);
#endif /* HAVE_MIDGARD_MULTILANG */
	if (!query) {
		mgd_free_pool(pool);
		return NULL;
	}

	/* execute query */
	res = mgd_query(mgd, query);	

	mgd_free_pool(pool);
	return res;
}

midgard_res *mgd_sitegroup_record(midgard * mgd,
				  const char *fields, const char *table, int id)
{
	midgard_res *res;
	const char *query;
	midgard_pool *pool;

	pool = mgd_alloc_pool();
	if (!pool)
		return NULL;

	/* format query */
	query = mgd_format(mgd, pool,
			   "SELECT $s,sitegroup FROM $s WHERE id=$i AND (sitegroup in (0, $d) OR $d<>0)",
			   fields, table, id,
            mgd->current_user->sitegroup,
            mgd->current_user->is_root);
	if (!query) {
		mgd_free_pool(pool);
		return NULL;
	}

	/* execute query */
	res = mgd_query(mgd, query);	

	mgd_free_pool(pool);
	return res;
}

midgard_res *mgd_sitegroup_record_lang(midgard * mgd,
				  const char *fields, const char *table, int id)
{
	midgard_res *res;
	GString *query;
	gchar *tmpstr;
	
	query = g_string_new("SELECT ");
	
	g_string_append_printf(query, 
			"%s FROM %s, %s_i WHERE %s.id=%d AND "
			"%s.id = %s_i.sid AND (%s.sitegroup in (0, %d) "
			"OR %d<>0) AND lang=%d",
			fields, table, table, table, id,
			table, table, table, mgd_sitegroup(mgd),
			mgd_isroot(mgd), mgd_lang(mgd));

	tmpstr = g_string_free(query, FALSE);
	res = mgd_query(mgd, tmpstr);
	g_free(tmpstr);

	/* Get record with lang 0 if other lang is set and lang 
	 * record was not found */
	if(!res && mgd_lang(mgd) > 0){
		
		query = g_string_new("SELECT ");
		g_string_append_printf(query,
				"%s FROM %s, %s_i WHERE %s.id=%d AND "
				"%s.id = %s_i.sid AND (%s.sitegroup in (0, %d) "
				"OR %d<>0) AND lang=%d",
				fields, table, table, table, id,
				table, table, table, mgd_sitegroup(mgd),
				mgd_isroot(mgd), mgd_get_default_lang(mgd));
		
		tmpstr = g_string_free(query, FALSE);
		res = mgd_query(mgd, tmpstr);
		g_free(tmpstr);
	}
	
	if(!res)
		return NULL;
	
	return res;
}

int mgd_idfield(midgard * mgd, const char *field, const char *table, int id)
{
	midgard_res *res;
	int rv;

	/* get record field */
	res = mgd_ungrouped_record(mgd, field, table, id);
	if (!res)
		return 0;

	/* get id value */
	if (mgd_fetch(res))
		rv = mgd_sql2id(res, 0);
	else
		rv = 0;
	mgd_release(res);

	return rv;
}

static char *midgard_all_tables[] = {
	"article",
	"element",
	"event",
	"eventmember",	
	"grp",
	"host",
	"member",
	"page",
	"pageelement",
	"pagelink",
	"person",
	"preference",
	"snippet",
	"snippetdir",
	"style",
	"topic",
	NULL
	   /* "sitegroup" omitted because it's more of a meta-info kind of record */
	   /* "repligard" omitted because it's more of a meta-info kind of record */
};

int mgd_global_exists(midgard * mgd, const char *where, ...)
{
	midgard_pool *pool;
	char **from = &midgard_all_tables[0];
	char *query;
	va_list args;

	pool = mgd_alloc_pool();
	if (!pool)
		return 0;

	va_start(args, where);
	query = mgd_vformat(mgd, pool, where, args);
	va_end(args);

	while (*from) {
		if (mgd_exists_id(mgd, *from, query)) {
			mgd_free_pool(pool);
			return 1;
		}

		from++;
	}

	mgd_free_pool(pool);
	return 0;
}

int mgd_exists_id(midgard * mgd, const char *from, const char *where, ...)
{
	midgard_res *res;
	const char *query;
	va_list args;
	int rv = 0;
	midgard_pool *pool;

	pool = mgd_alloc_pool();
	if (!pool)
		return 0;

	/* format query */
	if (strchr(from, ',')) return 0;

#if HAVE_MIDGARD_MULTILANG

	/* language table hack */
	if (strstr(from, "_i")) {
   query =
	    mgd_format(mgd, pool, "SELECT id FROM $s WHERE ($s)", from, where
		       );
	} else {
#endif /* HAVE_MIDGARD_MULTILANG */
   query =
	   mgd_format(mgd, pool, "SELECT id FROM $s WHERE ($s)"
      " AND (sitegroup IN (0,$d) OR $d<>0)"
      , from, where
      , mgd->current_user->sitegroup, mgd->current_user->is_root
      );
#if HAVE_MIDGARD_MULTILANG
	}
#endif /* HAVE_MIDGARD_MULTILANG */

	if (!query) {
		mgd_free_pool(pool);
		return 0;
	}

	/* execute query */
	va_start(args, where);
	res = mgd_vquery(mgd, query, args);
	va_end(args);	

	mgd_free_pool(pool);
	if (!res)
		return 0;
	if (mgd_fetch(res))
		rv = atoi(mgd_colvalue(res, 0));

	mgd_release(res);
	return rv;
}

int mgd_exists_bool(midgard * mgd, const char *from, const char *where, ...)
{
	midgard_res *res;
	const char *query;
	va_list args;
	int rv = 0;
	midgard_pool *pool;
	pool = mgd_alloc_pool();
	if (!pool)
		return 0;

	query =
	   mgd_format(mgd, pool, "SELECT 1 FROM $s WHERE $s", from,
		      where);

	if (!query) {
		mgd_free_pool(pool);
		return 0;
	}

	/* execute query */
	va_start(args, where);
	res = mgd_vquery(mgd, query, args);
	va_end(args);	
	
	mgd_free_pool(pool);
	if (!res)
		return 0;
	if (mgd_fetch(res))
		rv = atoi(mgd_colvalue(res, 0));

	mgd_release(res);
	return rv;
}

int mgd_fetch(midgard_res * res)
{
	assert(res && res->msql->res);
	if (res->msql->row)
		res->rown++;
	res->msql->row = mysql_fetch_row(res->msql->res);
	if (!res->msql->row)
		return 0;
	return 1;
}

void mgd_release(midgard_res * res)
{
	assert(res);
	
	if (res->msql->res == NULL) {
		g_warning("Someone is holding a stale reference to a midgard handle."
				"More than one midgard library loaded?");
		return;
	}
	
	mysql_free_result(res->msql->res);
	res->msql->res = NULL;
	mgd_free_pool(res->pool);
	if (res->next)
		res->next->prev = res->prev;
	if (res->prev)
		res->prev->next = res->next;
	else
		res->mgd->res = res->next;

	g_free(res->msql);
	g_free(res);
}

int mgd_rows(midgard_res * res)
{
	assert(res);
	return res->rows;
}

int mgd_cols(midgard_res * res)
{
	assert(res);
	return res->cols;
}

const char *mgd_colname(midgard_res * res, int i)
{
	assert(res && res->msql->res);
	if (0 <= i && i < res->cols)
		return mysql_fetch_field_direct(res->msql->res, i)->name;
	else
		return NULL;
}

int mgd_colisnum(midgard_res * res, int i)
{
        assert(res && res->msql->res);
	if (0 < i && i >= res->cols) return 0;
	
	switch (res->msql->fields[i].type) {
#if MYSQL_VERSION_ID > 41000
                case MYSQL_TYPE_TINY:
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_LONG:
                case MYSQL_TYPE_INT24:
                case MYSQL_TYPE_LONGLONG:
#else 
                case FIELD_TYPE_TINY:      
                case FIELD_TYPE_SHORT:      
                case FIELD_TYPE_LONG:      
                case FIELD_TYPE_INT24:      
                case FIELD_TYPE_LONGLONG:
#endif
                return 1;

                default:
                return 0;
        }
}

const char *mgd_colvalue(midgard_res * res, int i)
{
	assert(res && res->msql->row);
	if (0 <= i && i < res->cols)
		return res->msql->row[i];
	else
		return NULL;
}

const char *mgd_column(midgard_res * res, const char *name)
{
	int i;
	for (i = 0; i < mgd_cols(res); i++)
		if (strcmp(mgd_colname(res, i), name) == 0)
			return mgd_colvalue(res, i);
	return NULL;
}

int mgd_exec(midgard * mgd, const char *command, ...)
{
	int rv;
	va_list args;
	va_start(args, command);
	rv = mgd_vexec(mgd, command, args);
	va_end(args);
	return rv;
}

int mgd_vexec(midgard * mgd, const char *command, va_list args)
{
	int rv;
	char *fcommand;
	midgard_pool *pool;
	assert(mgd);
	assert(command);

	if (mgd->msql->mysql == NULL)
		return 0;

	pool = mgd_alloc_pool();
	if (!pool)
		return 0;
	/* format and send command */
	fcommand = mgd_vformat(mgd, pool, command, args);
	if (!fcommand) {
		mgd_free_pool(pool);
		return 0;
	}
	
    	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", fcommand);
	rv = mysql_query(mgd->msql->mysql, fcommand);

	if (rv != 0) {
		g_warning("\n\nQUERY FAILED: \n %s \n QUERY: \n %s\n",
				mysql_error(mgd->msql->mysql), fcommand);
	}	
	
	mgd_free_pool(pool);
	return rv ? 0 : 1;
}

#if HAVE_MIDGARD_VC
char * mgd_get_microtime() {
  struct timeval tv;
  struct timezone tz;
  gettimeofday(&tv, &tz);
  return g_strdup_printf("%d%06d", (int)(tv.tv_sec), (int)(tv.tv_usec));
}

void mgd_cvs_dump_if_locked(midgard * mgd, const char* table, int id) 
{
  return;	
  int locked;
  const char * guid;
  midgard_res * res;
  gchar * microtime;
  int status;
  pid_t pid;
  midgard_pool *pool;
  pool = mgd_alloc_pool();
  if (!pool)
    return;

  /* see if there is a lock (repligard export / cvs commit running) */
  if (mgd->cvs_script) {
    res =
      mgd_ungrouped_select(mgd, "locked, guid", "repligard", "realm=$q AND id=$d",
			   NULL, table, id);
    if (res && mgd_fetch(res)) {
      locked = atoi(mgd_colvalue(res, 0));
      guid = mgd_strdup(pool, mgd_colvalue(res,1));
      mgd_release(res);
      if (locked) {
	microtime = mgd_get_microtime();
	g_log("midgard-lib", G_LOG_LEVEL_DEBUG, "calling %s %s %s %s and waiting", mgd->cvs_script, mgd->msql->mysql->db, guid, microtime);
	pid = fork ();
	if (pid == 0)
	  {
	    close (0);
	    close (1);
	    close (2);
	    execl (mgd->cvs_script, mgd->cvs_script, mgd->msql->mysql->db, guid, microtime, NULL);
	    _exit (EXIT_FAILURE);
	  }
	else {
	  /* This is the parent process.  Wait for the child to complete.  */
	  g_free(microtime);
	  waitpid (pid, &status, 0);
	}
      }
    }
  }
  mgd_free_pool(pool);
}

void mgd_cvs_dump_nowait(midgard * mgd, const char* table, int id) {
  const char * guid;
  midgard_res * res;
  pid_t pid;
  gchar * microtime;
  midgard_pool *pool;
  pool = mgd_alloc_pool();
  if (!pool)
    return;

  /* see if there is a lock (repligard export / cvs commit running) */
  if (mgd->cvs_script) {
    res =
      mgd_ungrouped_select(mgd, "guid", "repligard", "realm=$q AND id=$d",
			   NULL, table, id);

    if (res && mgd_fetch(res)) {
      guid = mgd_strdup(pool, mgd_colvalue(res,0));
      microtime = mgd_get_microtime();
      g_log("midgard-lib", G_LOG_LEVEL_DEBUG, "calling %s %s %s %s (no wait)", mgd->cvs_script, mgd->msql->mysql->db, guid, microtime);
      pid = fork ();
      if (pid == 0)
	{
	  close(0);
	  close(1);
	  close(2);
	  execl (mgd->cvs_script, mgd->cvs_script, mgd->msql->mysql->db, guid, microtime, NULL);
	  _exit (EXIT_FAILURE);
	}
      else {
	g_free(microtime);
	waitpid (pid, NULL, 0);
	//waitpid (pid, NULL, WNOHANG | WUNTRACED);
	/* Don't wait. */
      }

    }
  }
  mgd_free_pool(pool);
}

void mgd_cvs_dump_nowait_guid(midgard * mgd, const char* guid) {
  pid_t pid;
  gchar * microtime;
  midgard_pool *pool = mgd_alloc_pool();
  if (!pool)
    return;

  if (mgd->cvs_script) {
      microtime = mgd_get_microtime();
      g_log("midgard-lib", G_LOG_LEVEL_DEBUG, "calling %s %s %s %s (no wait)", mgd->cvs_script, mgd->msql->mysql->db, guid, microtime);
      pid = fork ();
      if (pid == 0)
	{
	  close(0);
	  close(1);
	  close(2);
	  execl (mgd->cvs_script, mgd->cvs_script, mgd->msql->mysql->db, guid, microtime, NULL);
	  _exit (EXIT_FAILURE);
	}
      else {
	g_free(microtime);
	//waitpid (pid, NULL, WNOHANG | WUNTRACED);
	waitpid (pid, NULL, 0);
	/* Don't wait. */
      }
  }
  mgd_free_pool(pool);
}


#endif /* HAVE_MIDGARD_VC */

#if HAVE_MIDGARD_QUOTA

void mgd_set_quota_error(midgard *mgd, int nr) {
  mgd->quota_err = nr;
}

int mgd_get_quota_error(midgard *mgd) {
  return mgd->quota_err;
}

gboolean ghr_empty(gpointer key, gpointer value, gpointer user_data) {
  g_free(key);
  g_free(value);
  return 1;
}

void mgd_touch_quotacache(midgard *mgd) {
  g_hash_table_freeze(mgd->qlimits);
  g_hash_table_foreach_remove(mgd->qlimits, ghr_empty, NULL);
  g_hash_table_thaw(mgd->qlimits);
}

int mgd_touch_recorded_quota(midgard *mgd, const char* table, int sitegroup) {
  if (mgd_query(mgd, "UPDATE quota SET space_is_current=0, count_is_current=0 WHERE tablename=$q AND sg=$d", table, sitegroup)) {
    return 1;
  } else {
    return 0;
  }
}

int mgd_set_recorded_quota_count(midgard *mgd, const char* table, int sitegroup, int count) {
  if (mgd_query(mgd, "UPDATE quota SET eff_number=$d,count_is_current=1 WHERE tablename=$q AND sg=$d", count, table, sitegroup)) {
    return 1;
  } else {
    return 0;
  }
}

int mgd_get_recorded_quota_count(midgard *mgd, const char *table, int sitegroup) {
  int ret = -1;
  midgard_res *res;
  res = mgd_query(mgd, "SELECT count_is_current, eff_number FROM quota WHERE tablename=$q AND sg=$d", table, sitegroup);
  if (res) {
    if (mgd_fetch(res)) {
      if (mgd_sql2id(res, 0)) {
	ret = mgd_sql2id(res, 1);
      }
    }
    mgd_release(res);
  }
  return ret;
}

int mgd_get_quota_count_new_record(midgard *mgd, const char* table, int sitegroup, int newcount) {
  midgard_res *res;
  midgard_pool *pool;
  char *tablecpy, *underscore;
  int ret = 0, count = 0, sg = (mgd->current_user->sitegroup)?mgd->current_user->sitegroup:sitegroup;
  if (!sg) return 0;
  pool = mgd_alloc_pool();
  tablecpy = mgd_strdup(pool, table);
  if ((underscore = strstr(tablecpy, "_i"))) {
    *underscore = '\0';
  }
  if (mgd_lookup_table_id(tablecpy)) {
    count = mgd_get_recorded_quota_count(mgd, table, sg);
    if (count == -1) {
      res = mgd_query(mgd, "SELECT COUNT(*) FROM $s WHERE sitegroup = $d", table, sg);
      if (res) {
	if (mgd_fetch(res)) {
	  ret = mgd_sql2id(res, 0);
	}
	mgd_release(res);
      }
      mgd_set_recorded_quota_count(mgd, table, sitegroup, ret);
    } else {
      ret = count + newcount;
    }
  }
  mgd_free_pool(pool);
  return ret;
}

int mgd_get_quota_count(midgard *mgd, const char* table, int sitegroup) {
  return mgd_get_quota_count_new_record(mgd, table, sitegroup, 0);
}

int mgd_set_recorded_quota_space(midgard *mgd, const char* table, int sitegroup, int space) {
  if (mgd_query(mgd, "UPDATE quota SET eff_space=$d,space_is_current=1 WHERE tablename=$q AND sg=$d", space, table, sitegroup)) {
    return 1;
  } else {
    return 0;
  }
}

int mgd_get_recorded_quota_space(midgard *mgd, const char *table, int sitegroup) {
  int ret = -1;
  midgard_res *res;
  res = mgd_query(mgd, "SELECT space_is_current, eff_space FROM quota WHERE tablename=$q AND sg=$d", table, sitegroup);
  if (res) {
    if (mgd_fetch(res)) {
      if (mgd_sql2id(res, 0)) {
	ret = mgd_sql2id(res, 1);
      }
    }
    mgd_release(res);
  }
  return ret;
}

int mgd_compute_quota_space_sg(midgard *mgd, int sitegroup);

int mgd_get_quota_space_kb_new_record(midgard *mgd, const char* table, const char* fields, int sitegroup, int KB, int newlength) {
  midgard_res *res;
  char *tablecpy, *underscore, *path;
  struct stat buf;
  int ret = 0, space = 0, sg = (mgd->current_user->sitegroup)?mgd->current_user->sitegroup:sitegroup;

  if (!sg) return 0;
  tablecpy = g_strdup(table);
  if ((underscore = strstr(tablecpy, "_i"))) {
    *underscore = '\0';
  }
  if (mgd_lookup_table_id(tablecpy) || !strcmp(table, "blobsdata") || !strcmp(table, "wholesg")) {
    space = mgd_get_recorded_quota_space(mgd, table, sg);
    if (space == -1) {
      if (!strcmp(table, "blobsdata")) {
	res = mgd_query(mgd, "SELECT location FROM blobs WHERE sitegroup = $d", sg);
	if (res) {
	  while (mgd_fetch(res)) {
		  path = g_strconcat(mgd->blobdir,
				  "/",
				  (gchar *)mgd_colvalue(res, 0),
				  NULL);
		  if(stat(path, &buf) == 0){
			  ret = ret + buf.st_size;
		  }
		  g_free(path);
	  }
	  mgd_release(res);
	}
      } else if (!strcmp(table, "wholesg")) {
	ret = mgd_compute_quota_space_sg(mgd, sg);
      } else {
	if (fields && strlen(fields) > 0) {
	  res = mgd_query(mgd, "SELECT SUM(LENGTH(CONCAT($s))) FROM $s WHERE sitegroup = $d", fields, table, sg);
	  if (res) {
	    if (mgd_fetch(res)) {
	      ret = mgd_sql2id(res, 0);
	    }
	    mgd_release(res);
	  }
	}
      }
      mgd_set_recorded_quota_space(mgd, table, sg, ret);
    } else {
      ret = space + newlength;
    }
  }
  g_free(tablecpy);
  return KB?(ret/1024)+((ret%1024>0)?1:0):ret;
}

int mgd_get_quota_space(midgard *mgd, const char *table, const char *fields, int sitegroup) {
  return mgd_get_quota_space_kb_new_record(mgd, table, fields, sitegroup, 1, 0);
}

int mgd_get_quota_space_bytes(midgard *mgd, const char *table, const char *fields, int sitegroup) {
  return mgd_get_quota_space_kb_new_record(mgd, table, fields, sitegroup, 0, 0);
}

int mgd_get_quota_space_new_record(midgard *mgd, const char *table, const char *fields, int sitegroup, int newlength) {
  return mgd_get_quota_space_kb_new_record(mgd, table, fields, sitegroup, 0, newlength);
}

int mgd_compute_quota_space_sg(midgard *mgd, int sitegroup) {
  midgard_pool *pool;
  midgard_res *res;
  gchar *table, *fields;
  int space = 0;
  pool = mgd_alloc_pool();
  res = mgd_query(mgd, "SELECT tablename, spacefields FROM quota WHERE sg = $d AND tablename != 'wholesg'", sitegroup);
  if (res) {
    while (mgd_fetch(res)) {
      table = mgd_strdup(pool, mgd_colvalue(res,0));
      fields = mgd_strdup(pool, mgd_colvalue(res,1));
      space += mgd_get_quota_space_bytes(mgd, table, fields, sitegroup);
    }
    mgd_release(res);
  }
  mgd_set_recorded_quota_space(mgd, "wholesg", sitegroup, space);
  mgd_free_pool(pool);
  return space;
}



int mgd_get_record_space(midgard *mgd, const char *table, const char *spacefields, int id) {
  midgard_res *res;
  int ret = 0;
  res = mgd_query(mgd, "SELECT LENGTH(CONCAT($s)) FROM $s WHERE id=$d", spacefields, table, id);
  if (res) {
    if (mgd_fetch(res)) {
      ret = mgd_sql2id(res, 0);
    }
    mgd_release(res);
  }
  return ret;
}

void mgd_recompute_quota (midgard *mgd, int sitegroup) {
  mgd_query(mgd, "UPDATE quota SET space_is_current=0, count_is_current WHERE sg=$d", sitegroup);
  mgd_compute_quota_space_sg(mgd, mgd->current_user->sitegroup);
}

qlimit * mgd_lookup_qlimit(midgard *mgd, const char *table, int sitegroup) {
  midgard_res *res;
  qlimit *limit;
  gchar *key = g_strdup_printf("%d%s", mgd->current_user->sitegroup, table);
  limit = (qlimit *) g_hash_table_lookup(mgd->qlimits, key);
  if (limit == NULL) {
    limit = (qlimit*)malloc(sizeof(qlimit));
    limit->number = 0;
    limit->space = 0;
    limit->fields = NULL;
    res = mgd_query(mgd, "SELECT number, space, spacefields FROM quota WHERE sg = $d and tablename = $q", mgd->current_user->sitegroup, table);
    if (res) {
      if (mgd_fetch(res)) {
	limit->number = mgd_sql2id(res, 0);
	limit->space = mgd_sql2id(res, 1);
	if (strlen(mgd_colvalue(res, 2)) > 0)
	  limit->fields = g_strdup(mgd_colvalue(res,2));
      }
      mgd_release(res);
    }
    g_log("midgard-lib", G_LOG_LEVEL_DEBUG,
	  "INSERTING BY KEY: %s %s", key, limit->fields);
    g_hash_table_insert(mgd->qlimits, key, limit);
  } else {
    g_free(key);
  }
  return limit;
}

gboolean mgd_check_quota(midgard * mgd, const char *table) {
  /* it is assumed that mgd->current_user->sitegroup > 0 */
  qlimit *limit;
  qlimit *sglimit;
  gboolean ret = TRUE;
  limit = mgd_lookup_qlimit(mgd, table, mgd->current_user->sitegroup);
  sglimit = mgd_lookup_qlimit(mgd, "wholesg", mgd->current_user->sitegroup);
  if (limit->number) {
    if (mgd_get_quota_count(mgd, table, mgd->current_user->sitegroup) >= limit->number) {
      g_log("midgard-lib", G_LOG_LEVEL_DEBUG,
	    "Quota exceeded");
      ret = FALSE;
    }
  }
  if (ret && limit->space && limit->fields) {
    if (mgd_get_quota_space(mgd, table, limit->fields, 0) >= limit->space) {
      g_log("midgard-lib", G_LOG_LEVEL_DEBUG,
	    "Quota exceeded");
      ret = FALSE;
    }
  }
  if (sglimit->space) {
	  guint32 sgsize = midgard_quota_get_sitegroup_size(mgd, mgd_sitegroup(mgd));
	  sgsize = (sgsize/1024)+(sgsize%1024);
	  if(sgsize >= sglimit->space) {
		  g_log("midgard-lib", G_LOG_LEVEL_DEBUG,
				  "Quota exceeded");
		  ret = FALSE;
	  }
  }
  return ret;
}


#endif /* HAVE_MIDGARD_QUOTA */

int mgd_create(midgard * mgd, const char *table,
	       const char *fields, const char *values, ...)
{
	int id;
	va_list args;
	va_start(args, values);
	id = mgd_vcreate(mgd, table, fields, values, args);
	va_end(args);
	return id;
}


int mgd_vcreate(midgard * mgd, const char *table,
		const char *fields, const char *values, va_list args)
{
#if HAVE_MIDGARD_QUOTA
  char *spacefields = NULL;
  int recordspace;
  qlimit *sglimit, *limit;
#endif
	const char *command = NULL;
	int rv;
	int id = 0;
	midgard_pool *pool;

	pool = mgd_alloc_pool();
	if (!pool)
		return 0;
	/* format command */

	if ((strcmp(table, "sitegroup") == 0)
         || (mgd->current_user->is_root && mgd->current_user->sitegroup == 0)) {
		/* records on sitegroup table don't get assigned to a sitegroup */
		if (mgd->current_user->is_root)
			command = mgd_format(mgd, pool,
					     "INSERT INTO $s ($s) VALUES ($s)",
					     table, fields, values);

		rv = mgd_vexec(mgd, command, args);
		id = mysql_insert_id(mgd->msql->mysql);
		mgd_free_pool(pool);
		return rv ? id : 0;

	}
	else if (strstr(fields, "sitegroup") != NULL)
	{
		/* sitegroup information provided in fields */
		if (mgd->current_user->is_root && mgd->current_user->sitegroup == 0)
		{
			command = mgd_format(mgd, pool, "INSERT INTO $s ($s) VALUES ($s)",
				table, fields, values);
		}
		else
		{
			/* no rights to specify the sitegroup field if
			 * not SG0 admin logged into SG0
			 */
			return 0;
		}
	}
	else
	{
#if HAVE_MIDGARD_QUOTA
	  if (mgd->quota && mgd->current_user->sitegroup > 0) {
	    if (!mgd_check_quota(mgd, table)) {
		mgd_free_pool(pool);
		mgd_set_quota_error(mgd, 1);
		return -64;
	    }
	  }
#endif /* HAVE_MIDGARD_QUOTA */
		/* no sitegroup information provided */
		command = mgd_format(mgd, pool,
				     "INSERT INTO $s ($s, sitegroup) VALUES ($s, $d)",
				     table, fields, values, mgd->current_user->sitegroup);
	}

	if (!command) {
		mgd_free_pool(pool);
		return 0;
	}

	/* execute command */
	rv = mgd_vexec(mgd, command, args);
	
	id = mysql_insert_id(mgd->msql->mysql);

	/* create guid, update created record , create repligard entry */
	if(id > 0){
		
		/* insert created and revised datetimes */
		GValue tval = {0, };
		time_t utctime = time(NULL);
		g_value_init(&tval, MIDGARD_TYPE_TIMESTAMP);
		midgard_timestamp_set_time(&tval, utctime);
		gchar *timecreated = midgard_timestamp_dup_string(&tval);
		g_value_unset(&tval);
		
		gchar *_guid = midgard_guid_new(mgd);
		if(!g_str_has_suffix(table, "_i")) {
			GString *_guid_sql = g_string_new("UPDATE ");
			g_string_append_printf(_guid_sql,
					" %s SET guid='%s', "
					"metadata_created = '%s', "
					"metadata_revised = '%s' "
					"WHERE id=%d AND sitegroup=%d",
					table, _guid, 
					timecreated, timecreated,
					id,  mgd_sitegroup(mgd));
			gchar *tmpstr = g_string_free(_guid_sql, FALSE);
			mgd_exec(mgd, tmpstr, NULL);
		}
		mgd_create_repligard(mgd, table, "guid,id,changed,action,sitegroup",
				"$q,$d,NULL,'create',$d",_guid, id, mgd_sitegroup(mgd));
		g_free(_guid);
		g_free(timecreated);
	}

#if HAVE_MIDGARD_QUOTA
	if (mgd->quota && mgd->current_user->sitegroup > 0) {
	  sglimit = mgd_lookup_qlimit(mgd, "wholesg", mgd->current_user->sitegroup);
	  limit = mgd_lookup_qlimit(mgd, table, mgd->current_user->sitegroup);
	  if (limit && limit->number > 0) {
	    mgd_set_recorded_quota_count(mgd, table, mgd->current_user->sitegroup, mgd_get_quota_count_new_record(mgd, table, mgd->current_user->sitegroup, 1));
	  }
	  /* FIXME , this value is by default set to NULL and later on , set *nowhere* */
	  if (spacefields) {
	    recordspace = mgd_get_record_space(mgd, table, spacefields, id);
	    mgd_set_recorded_quota_space(mgd, table, mgd->current_user->sitegroup, mgd_get_quota_space_new_record(mgd, table, spacefields, mgd->current_user->sitegroup, recordspace));
	    if (sglimit && sglimit->space > 0) {
	      mgd_set_recorded_quota_space(mgd, "wholesg", mgd->current_user->sitegroup, mgd_get_quota_space_new_record(mgd, "wholesg", NULL, mgd->current_user->sitegroup, recordspace));
	    }
	  }
	} else if (!(mgd->quota)) {
	  mgd_touch_recorded_quota(mgd, table, mgd->current_user->sitegroup);
	  mgd_touch_recorded_quota(mgd, "wholesg", mgd->current_user->sitegroup);
	}
#endif
	mgd_free_pool(pool);
	return rv ? id : 0;
}

int mgd_update(midgard * mgd, const char *table, int id,
	       const char *fields, ...)
{
	int rv;
	va_list args;
	va_start(args, fields);
	rv = mgd_vupdate(mgd, table, id, fields, args);
	va_end(args);
	return rv;
}

int mgd_vupdate(midgard * mgd, const char *table, int id,
		const char *fields, va_list args)
{
#if HAVE_MIDGARD_QUOTA
  char *spacefields = NULL;
  int recordspace_before = 0, recordspace_after;
  qlimit* sglimit;
#endif /* HAVE_MIDGARD_QUOTA */
	const char *command = NULL;
	int rv;
	midgard_pool *pool;
	pool = mgd_alloc_pool();
	if (!pool)
		return 0;
	GString *_usql = NULL;
#if HAVE_MIDGARD_VC
	mgd_cvs_dump_if_locked(mgd,table,id); 
#endif /* HAVE_MIDGARD_VC */

	/* format command */
	/* strcmp tolerable since updates are less common than gets */
	if (strcmp(table, "sitegroup") == 0) {
		if (mgd->current_user->is_root)
			command = mgd_format(mgd, pool,
					     "UPDATE $s SET $s WHERE id=$d",
					     table, fields, id);
	}
	else {
		command = mgd_format(mgd, pool,
				     "UPDATE $s SET $s WHERE id=$d"
				     "  AND (sitegroup = $d OR $d<>0)",
				     table, fields, id, mgd->current_user->sitegroup,
				     mgd->current_user->is_root);

		if(!g_str_has_suffix(table, "_i")) {
			
			GValue tval = {0, };
			time_t utctime = time(NULL);
			g_value_init(&tval, MIDGARD_TYPE_TIMESTAMP);
			midgard_timestamp_set_time(&tval, utctime);
			gchar *timeupdated = midgard_timestamp_dup_string(&tval);
			g_value_unset(&tval);
			
			_usql = g_string_new("UPDATE ");
			g_string_append_printf(_usql, 
					"%s SET metadata_revised = '%s' "
					"WHERE id=%d AND sitegroup = %d ",
					table, timeupdated, id, 
					mgd->current_user->sitegroup);
			
			g_free(timeupdated);
			mgd_exec(mgd, _usql->str, NULL);
			g_string_free(_usql, FALSE);
		}
	}

#if HAVE_MIDGARD_QUOTA
	  if (mgd->quota  && mgd->current_user->sitegroup > 0) {
	    if (!mgd_check_quota(mgd, table)) {
	      mgd_free_pool(pool);
	      mgd_set_quota_error(mgd, 1);
	      return -64;
	    } else {
	      /* FIXME ! */	    
	      if (spacefields) {
		recordspace_before = mgd_get_record_space(mgd, table, spacefields, id);
	      }
	    }
	  }
#endif /* HAVE_MIDGARD_QUOTA */

	if (!command) {
		mgd_free_pool(pool);
		return 0;
	}

	/* execute command */
	rv = mgd_vexec(mgd, command, args);

#if HAVE_MIDGARD_QUOTA
	if (mgd->quota && mgd->current_user->sitegroup > 0) {
	  sglimit = mgd_lookup_qlimit(mgd, "wholesg", mgd->current_user->sitegroup);
	  /* FIXME */
	  if (spacefields) {
	    recordspace_after = mgd_get_record_space(mgd, table, spacefields, id);
	    mgd_set_recorded_quota_space(mgd, table, mgd->current_user->sitegroup, mgd_get_quota_space_new_record(mgd, table, spacefields, mgd->current_user->sitegroup, recordspace_after - recordspace_before));
	    if (sglimit && sglimit->space > 0) {
	      mgd_set_recorded_quota_space(mgd, "wholesg", mgd->current_user->sitegroup, mgd_get_quota_space_new_record(mgd, "wholesg", NULL, mgd->current_user->sitegroup, recordspace_after - recordspace_before));
	    }
	  }
	} else if (!(mgd->quota)) {
	  mgd_touch_recorded_quota(mgd, table, mgd->current_user->sitegroup);
	  mgd_touch_recorded_quota(mgd, "wholesg", mgd->current_user->sitegroup);
	}
#endif /* HAVE_MIDGARD_QUOTA */

#if HAVE_MIDGARD_VC
	mgd_cvs_dump_nowait(mgd,table,id);
#endif /* HAVE_MIDGARD_VC */

	mgd_free_pool(pool);
	return rv;
}

int mgd_delete(midgard * mgd, const char *table, int id)
{
#if HAVE_MIDGARD_QUOTA
  qlimit *limit = NULL, *sglimit = NULL;
  int recordspace = 0;
#endif /* HAVE_MIDGARD_QUOTA */
	const char *command = NULL;
	int rv;
	midgard_pool *pool;

	pool = mgd_alloc_pool();
	if (!pool)
		return 0;


#if HAVE_MIDGARD_VC
	mgd_cvs_dump_if_locked(mgd,table,id);
#endif /* HAVE_MIDGARD_VC */

	/* format command */

	/* strcmp tolerable since updates are less common than gets */
	if (strcmp(table, "sitegroup") == 0) {
		if (mgd->current_user->is_root)
			command = mgd_format(mgd, pool,
					     "DELETE FROM $s WHERE id=$d",
					     table, id);
	}
	else
		command = mgd_format(mgd, pool, "DELETE FROM $s WHERE id=$d"
				     " AND (sitegroup = $d OR $d<>0)",
				     table, id,
                 mgd->current_user->sitegroup,
                 mgd->current_user->is_root);

	if (!command) {
		mgd_free_pool(pool);
		return 0;
	}

#if HAVE_MIDGARD_QUOTA
	  if (mgd->quota  && mgd->current_user->sitegroup > 0) {
	    limit = mgd_lookup_qlimit(mgd, table, mgd->current_user->sitegroup);
	    sglimit = mgd_lookup_qlimit(mgd, "wholesg", mgd->current_user->sitegroup);
	    if (limit && limit->number > 0) {
	      mgd_set_recorded_quota_count(mgd, table, mgd->current_user->sitegroup, mgd_get_quota_count_new_record(mgd, table, mgd->current_user->sitegroup, -1));
	    }
	    if (limit && limit->fields) {
	      recordspace = mgd_get_record_space(mgd, table, limit->fields, id);
	    }
	  } else if (!(mgd->quota)) {
	    mgd_touch_recorded_quota(mgd, table, mgd->current_user->sitegroup);
	    mgd_touch_recorded_quota(mgd, "wholesg", mgd->current_user->sitegroup);
	  }
#endif /* HAVE_MIDGARD_QUOTA */


	/* execute command */
	rv = mgd_exec(mgd, command);
#if HAVE_MIDGARD_QUOTA
	if (recordspace) {
	  mgd_set_recorded_quota_space(mgd, table, mgd->current_user->sitegroup, mgd_get_quota_space_new_record(mgd, table, limit->fields, mgd->current_user->sitegroup, - recordspace));
	  if (sglimit && sglimit->space > 0) {
	    mgd_set_recorded_quota_space(mgd, "wholesg", mgd->current_user->sitegroup, mgd_get_quota_space_new_record(mgd, "wholesg", NULL, mgd->current_user->sitegroup, - recordspace));
	  }
	}
#endif /* HAVE_MIDGARD_QUOTA */
	mgd_free_pool(pool);
	return rv;
}

void mgd_set_blobdir(midgard *mgd, char *blobdir)
{
   if (blobdir == NULL) mgd->blobdir = NULL;
   else mgd->blobdir = blobdir;
}

const char* mgd_get_blobdir(midgard *mgd)
{
   return (const char*)mgd->blobdir;
}

#if HAVE_MIDGARD_VC
void mgd_set_cvs_script(midgard *mgd, char *cvs_script)
{
   if (cvs_script == NULL) mgd->cvs_script = NULL;
   else mgd->cvs_script = cvs_script;
}

const char* mgd_get_cvs_script(midgard *mgd)
{
   return (const char*)mgd->cvs_script;
}

#endif /* HAVE_MIDGARD_VC */

#if HAVE_MIDGARD_QUOTA
void mgd_set_quota(midgard *mgd, int quota)
{
   mgd->quota = quota;
}

int mgd_get_quota(midgard *mgd)
{
   return (int)mgd->quota;
}

#endif /* HAVE_MIDGARD_QUOTA */

void mgd_set_ah_prefix(midgard *mgd, char* prefix)
{
   mgd->ah_prefix = prefix;
}

char* mgd_get_ah_prefix(midgard *mgd)
{
   return mgd->ah_prefix;
}


int midgard_lib_compiled_with_sitegroups()
{
	return 1;
}

#if MIDGARD_PHP_REQUEST_CONFIG_BUG_WORKAROUND
static void *hack_rcfg;
static void *hack_dcfg;

void mgd_php_bug_workaround_set_rcfg_dcfg(void *rcfg, void *dcfg)
{
  hack_rcfg = rcfg;
  hack_dcfg = dcfg;
}

void* mgd_php_bug_workaround_get_rcfg(midgard * mgd)
{
	return hack_rcfg;
}

void* mgd_php_bug_workaround_get_dcfg(midgard *mgd )
{
	return hack_dcfg;
}
#endif

#if HAVE_MIDGARD_MULTILANG
int mgd_internal_set_lang(midgard *mgd, int lang) 
{
	g_assert(mgd != NULL);

	if(lang < 0)
		return -1;

	mgd->lang = lang;
	
	/* Language is reset at the end of the request and MidgardConnection might be already destroyed */
	if (mgd->_mgd && MIDGARD_IS_CONNECTION (mgd->_mgd))
		mgd->_mgd->priv->lang_id = lang;

  	return 0;
}
int mgd_internal_set_parameters_defaultlang(midgard *mgd, int lang) {
  mgd->p_lang = lang;
  return 0;
}
int mgd_internal_set_attachments_defaultlang(midgard *mgd, int lang) {
  mgd->a_lang = lang;
  return 0;
}
#endif /* HAVE_MIDGARD_MULTILANG */

static FILE *midgard_ntlm_pipe_in, *midgard_ntlm_pipe_out;

void midgard_ntlm_set_pipes(FILE *pipe_in, FILE *pipe_out) {
	midgard_ntlm_pipe_in = pipe_in;
	midgard_ntlm_pipe_out = pipe_out;
}

/*
 * Process NTLM authentication phases
 * [AB]: I added macros for multi-line statements to keep actual code clean and understandable
 * because NTLM handshake doesn't clean itself.
 */
midgard_ntlm_auth_state midgard_ntlm_process_auth(midgard *mgd, midgard_ntlm_auth_state auth_state,
						  /* Hook for helper to get client reponses */
						  const char * (*get_info)(void *data),
						  /* Hook for helper to set our responses */
						  void (*set_info)(const char *info, void *data),
						  /* Hook for identifying policy and returning logon string for mgd_auth */
						  /* Returned values: authline -- logon string, (result) - [0 - forbid], [1 - normal], [2 - setuid] */
						  /* Will be called just before finishing authentication process */
						  int (*define_policy)(midgard *mgd, void *data, const char *username, const char *domain, const char **authline),
						  void *data)
{
        const char *response_string = "NTLM";
        const char *auth_line = NULL;
	char auth_response[NTLM_MAX_BUF_SIZE+1];
	char *lc, *tc;
        int result;
	midgard_pool *pool;

        switch(auth_state) {
                case MGD_NTLM_INITIATE:
			set_info(response_string, data);
			return MGD_NTLM_WAIT_NEGOTIATE;
                case MGD_NTLM_WAIT_NEGOTIATE:
			auth_line = get_info(data);
                        if ((auth_line != NULL) && (strstr(auth_line, "NTLM") == auth_line)) {
				/* Note that we are adding a space after KK as a part of auth_line+4 */
				fprintf(midgard_ntlm_pipe_in, "%s%s\n", "KK", auth_line+4);
			request_check:
				/* Get ntlm_auth's response and pass back to client */
				if (fgets(auth_response, sizeof(auth_response)-1, midgard_ntlm_pipe_out) != NULL) {
					/* Crop line on '\n' */
					lc = memchr(auth_response, '\n', sizeof(auth_response)-1);
					if (lc) *lc = '\0';
					if (strstr(auth_response, "TT") == auth_response) {
						/* ntlm_auth asked for more information, forward the question
						to the client using Authenticate: NTLM mechanism */
						pool = mgd_alloc_pool();
						response_string = mgd_format(mgd, pool, "NTLM$s", auth_response+2);
						set_info(response_string, data);
						mgd_free_pool(pool);
						return MGD_NTLM_WAIT_NEGOTIATE;
					} else if (strstr(auth_response, "BH") == auth_response) {
						/* Oops, broken helper, return helper to initial state */
						fprintf(midgard_ntlm_pipe_in, "YR\n");
						goto request_check;
					} else if (strstr(auth_response, "AF") == auth_response) {
						/* We passed authorization. Returned string is DOMAIN\USER 
            [PP:] We use '+' instead '\\'. By default most samba configuration 
            have winbind separator set to '+'. Make sure you do not use default domain 
            for winbind as this forces winbind to put ONLY username  in pipe file! */
            tc = memchr(auth_response, '+', sizeof(auth_response)-1);
						if (tc) {
							*tc = '\0';
							/* Call external hook and let it to define policy
							   for Midgard authorization */
	   						result = define_policy(mgd, data, /* username */ tc+1,
										/* domain */ auth_response+3,
										&auth_line);
							*tc = '\\';
							/* 'Define policy' hook must return:
							 * 0 - if user didn't pass policy check
							 * 1 - if user should be authorized as regular one
							 * 2 - if user needs to setuid
							 */
							if (result > 0) {
								result = mgd_auth_su(mgd, auth_line, "", 2, result - 1, FALSE, FALSE);
								return (result > 0 ) ?
									MGD_NTLM_AUTHORIZED : MGD_NTLM_UNAUTHORIZED;
							}
						}
						set_info(response_string, data);
						return MGD_NTLM_INITIATE;
					} else if(strstr(auth_response, "NA") == auth_response) {
						/* There is an error. Returned string is NT_STATUS_* error */
						/* Return back to initial state */
						if(strstr(auth_response, "NT_STATUS_INVALID_PARAMETER") == auth_response+3) {
							/* Oops, broken helper, return helper to initial state */
							fprintf(midgard_ntlm_pipe_in, "YR\n");
							goto request_check;
						}
						set_info(response_string, data);
						return MGD_NTLM_START;
					}
					if(lc) *lc = '\n';
				}
				return MGD_NTLM_WAIT_NEGOTIATE;
			}
			/* We got no Authorization response */
			set_info(response_string, data);
			return MGD_NTLM_START;
		case MGD_NTLM_AUTHORIZED:
		case MGD_NTLM_UNAUTHORIZED:
		case MGD_NTLM_START:
			break;
		case MGD_NTLM_REFRESH_AUTH:
			/* Call external hook and let it to define policy
			   for Midgard authorization. We pass NULL username and domain
			   to indicate that hook must return cached auth_line instead
			   of making choices */
			result = define_policy(mgd, data, /* username */ NULL,
						/* domain */ NULL,
						&auth_line);
			/* 'Define policy' hook must return:
			 * 0 - if user didn't pass policy check
			 * 1 - if user should be authorized as regular one
			 * 2 - if user needs to setuid
			 */
			if (result > 0) {
				result = mgd_auth_su(mgd, auth_line, "", 2, result - 1, FALSE, FALSE);
				return (result > 0 ) ?
					MGD_NTLM_AUTHORIZED : MGD_NTLM_UNAUTHORIZED;
			}
			set_info(response_string, data);
			return MGD_NTLM_INITIATE;
			break;

	}
	return MGD_NTLM_INITIATE;
}

/* Start MQL */

/* This is almost the same function as mgd_sitegroup_record.
 * We must define this one cause:
 * - where part must contain table.id and table.sitegroup
 * - it will be a good wrapper for future libgda usage
 */

midgard_res *mgdlib_mql_select_type(midgard * mgd, const gchar *table,
        const char *select, const char *from, const char *where)
{
  midgard_res *res;
  const char *query;
  midgard_pool *pool;
  
  pool = mgd_alloc_pool();
  if (!pool)
    return NULL;
    
  query = mgd_format(mgd, pool,
      "SELECT $s FROM $s WHERE $s AND ($s.sitegroup in (0, $d) OR $d<>0)",
      select, from, where, table,
      mgd->current_user->sitegroup,
      mgd->current_user->is_root);
  
  if (!query) {
    mgd_free_pool(pool);
    return NULL;
  }
  
  res = mgd_query(mgd, query);
  mgd_free_pool(pool);
  return res;
}

/* Get field's  value as gpointer. Type cast is needed according to gobject's
 * property value_type. Remove midgard_res when we switch to libgda.
 * I think we could use object pointer and property name as function 
 * parameters 
 */ 
gpointer midgard_query_field_get(midgard *mgd, const gchar *field,
        const gchar *from, const gchar *where)
{
    midgard_res *res;    
    GString *sql;
    gpointer value;

    sql = g_string_new("SELECT ");
    
    g_string_append(sql, field);
    g_string_append(sql, " FROM ");
    g_string_append(sql, from);
    g_string_append(sql, " WHERE ");
    g_string_append(sql, where);
    g_string_append_printf(sql, " AND sitegroup IN (%u, 0) AND metadata_deleted=0",
            mgd->current_user->sitegroup);

    gchar *tmpstr = g_string_free(sql, FALSE);
    res = mgd_query(mgd, tmpstr);
    g_free(tmpstr);
    
    if (!res)
        return NULL;

    if(!mgd_fetch(res))
        return NULL;
    
    value = (gpointer)mgd_colvalue(res, 0);
    
    return value;
}

guint midgard_sitegroup_get_id(midgard * mgd)
{
    g_assert(mgd);
    return mgd->current_user->sitegroup;
}


void midgard_shutdown(midgard *mgd){

    g_assert(mgd != NULL);
   
    /* FIXME */
    /* move code from mgd_close here and  keep mgd->config destroy */
   
    g_free((gchar *)mgd->logfile);

    /* members not included in mgd_close */
    g_free(mgd->schemafile);    
    g_free(mgd->blobdir);    
    g_hash_table_destroy(mgd->qlimits);
  
    if(mgd->person)
	    if(G_IS_OBJECT(G_OBJECT(mgd->person)))
		    g_object_unref(G_OBJECT(mgd->person));

    if(mgd->schema) {
	    if(G_IS_OBJECT(G_OBJECT(mgd->schema))) {
		    g_object_unref(G_OBJECT(mgd->schema));
	    }
    }
 
    /* Free parsers */
    mgd_done();
  
    /* Free legacy data */
    mgd_close(mgd);
}
