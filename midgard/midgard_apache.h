/* $Id: midgard_apache.h 25236 2010-03-06 10:14:55Z piotras $
 *
 * midgard/apache.h: apache helper routines and data structures
 *
 * Copyright (C) 1999 Jukka Zitting <jukka.zitting@iki.fi>
 * Copyright (C) 2000 The Midgard Project ry
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
#ifndef MGD_APACHE_H
#define MGD_APACHE_H
#include <midgard/midgard.h>
#include <midgard/types.h>
#include <midgard/midgard_schema.h>
#include <midgard/pageresolve.h>
#include <midgard/authfailure.h>
#include <midgard/midgard_datatypes.h>
#include <midgard/midgard_type.h>
#include <midgard/midgard_config.h>

#define MOD_MIDGARD_RESOURCE_PAGE            0
#define MOD_MIDGARD_RESOURCE_PAGE_REDIRECT   1
#define MOD_MIDGARD_RESOURCE_BLOB            2
#define MOD_MIDGARD_RESOURCE_FILETEMPLATE    3

#define MIDGARD_HANDLER "application/x-httpd-php-midgard"

struct _midgard_database_connection;

typedef struct {
	char *username;
	char *password;
	char *hostname;
	int refcount;
	struct _midgard_database_connection *current;
	midgard *mgd;
} midgard_database_handle;

typedef struct _midgard_database_connection {
	char *name;
	midgard_database_handle *handle;
} midgard_database_connection;

typedef struct _server_config {
	char *default_realm;
	
	struct {
		midgard_database_connection* auth;
		midgard_database_connection* main;
	} database;
	
	struct {
		int on, set;
	} forcedroot;

	char *pagecache;
	char *ntlm_auth;
	gpointer ntlm_map_domains;
	midgard_auth_type authtype;
	int authtrusted;
	gboolean authtrusted_createuser;
	gchar *authtrusted_groupguid;
	gpointer object;
	const gchar *pamfile;
} midgard_server_config;

#define MGD_AUTH_SNOOP_NONE   0
#define MGD_AUTH_SNOOP_OK     1
#define MGD_AUTH_SNOOP_FAILED 2

typedef struct _request_config {
	gpointer    req;
	gpointer    pool;
	midgard     *mgd;
	int host, style;
	int author;

	struct {
		int required;
		int passed;
		int cookie;
	} auth;
	
	int prelen;
	int self_len;
	
	struct {
		int status; /*[eeh] obsolete in mod2 */
		int id; /*[eeh] rename to page for mod2 */
		int type; /*[eeh] obsolete in mod2 */
		int active_page; /*[eeh] obsolete in mod2 */
		const char *content_type;
	} resource;
	
	const char *path; /*[eeh] obsolete in mod2 */
	gpointer argv;
	int nelts;
	char **elts;
	char *uri;
	GHashTable *elements;
	midgard_server_config *scfg;
	
	int preparser_active;
	
	struct {
		midgard_database_connection* current;
		midgard_database_connection* main;
	} database;
} midgard_request_config;

typedef struct _directory_config {
	
	struct {
		int on;
		int set;
	} engine;
	
	struct {
		int on;
		int set;
	} extauth;
	
	char *rootfile;
	char *templatefile;
	char *blobdir;
	char *parser;
	char *extension;
#if HAVE_MIDGARD_VC
	char *cvs_script;
#endif /* HAVE_MIDGARD_VC */

	struct {
		int on;
		int set;
	} quota;
	
	struct {
		int on;
		int set;
	} attachmenthost;
	
	char* ah_prefix;
	guint loglevel;
	const gchar *logfile; 
	MidgardSchema *schema;
	const char *schemafile;
	gpointer object;
} midgard_directory_config;

#endif
