/* $Id: midgard_legacy.h 26773 2010-12-30 14:15:03Z piotras $
 *
 * midgard-lib: database access for Midgard clients
 *
 * Copyright (C) 1999 Jukka Zitting <jukka.zitting@iki.fi>
 * Copyright (C) 2000 The Midgard Project ry
 * Copyright (C) 2003 David Schmitter, Dataflow Solutions GmbH <schmitt@dataflow.ch>
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

#ifndef MIDGARD_LEGACY_H
#define MIDGARD_LEGACY_H

/** 
 * \defgroup legacy Legacy and Obsolete
 *
 */

#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "midgard/midgard_config_auto.h"
#include "midgard/midgard_defs.h"
#include "midgard/parser.h"
#include "midgard/tablenames.h"
#include "midgard/pool.h"
#include "midgard/midgard_connection.h"
#include "midgard/midgard_schema.h"

typedef enum {
	MGD_AUTHTYPE_NORMAL = 0,
	MGD_AUTHTYPE_PAM
} midgard_auth_type;

struct _mgd_userinfo {
	int id, is_admin, *member_of;
	int is_root, sitegroup;
};

struct _midgard {
	midgard_mysql *msql;
	midgard_res *res;
	midgard_pool *pool, *tmp;
	mgd_parser* parser;
	mgd_userinfo user_at_pagestart, setuid_user;
	mgd_userinfo *current_user;
	int lang;
	int default_lang;
	int p_lang;
	int a_lang;
	char *username;
	char *blobdir;
#if HAVE_MIDGARD_VC
	char *cvs_script;
#endif /* HAVE_MIDGARD_VC */
	char *ah_prefix;
#if HAVE_MIDGARD_QUOTA
	int quota;
	int quota_err;
	GHashTable *qlimits;
#endif /* HAVE_MIDGARD_QUOTA */
	midgard_auth_type auth_type;
	MidgardSchema *schema;
	gchar *schemafile;
	gint errn;
	guint loglevel;
	const gchar *logfile;
	gpointer person;
	GHashTable *style_elements;
	GError *err;
	guint loghandler;
	MidgardConnection *_mgd;
	const gchar *pamfile;
	gboolean is_copy;
	gchar *configname;
};

#if HAVE_MIDGARD_QUOTA
struct _qlimit {
	int number;
	int space;
	char* fields;
};
#endif /* HAVE_MIDGARD_QUOTA */

struct _midgard_res {
	midgard *mgd;
	midgard_res *next, *prev;
	midgard_pool *pool;
	midgard_mysql *msql;
	int rown, rows, cols;
};

/* PATH_MAX is not available on Win32, we use our own define here */
#ifndef MAXPATHLEN
# ifdef PATH_MAX
#  define MAXPATHLEN PATH_MAX
# else
#  define MAXPATHLEN 256    /* Should be safe for any weird systems that do not define it */
# endif
#endif

/*! Identifier of the Midgard library API functions */
#define MGD_API extern

#define MIDGARD_REPLIGARD_TAG_LENGTH 64
#define MIDGARD_LOGIN_SG_SEPARATOR "+*!$;"
#define MIDGARD_LOGIN_RESERVED_CHARS MIDGARD_LOGIN_SG_SEPARATOR "="

#include "midgard/guid.h"

#define MGD_STRUCT_STD_FIELDS \
  midgard_res *res; int restype; \
  int id, creator, revisor, revision; \
  time_t created, revised;

/*[eeh] Custom log levels */
typedef enum {
   G_LOG_LEVEL_DEBUG_PARSE_URI_VERBOSE   =  1 << G_LOG_LEVEL_USER_SHIFT
} MgdGLogLevelFlags;

MGD_API const char *mgd_version();
MGD_API void mgd_init();
MGD_API guint mgd_parse_log_levels(const char *levels);
MGD_API void mgd_init_ex(guint log_levels, char *logfile);
MGD_API void mgd_done();
MGD_API void mgd_log_debug_default(const gchar *domain, GLogLevelFlags level,
          const gchar *msg, gpointer holder);

/* MGD_API const char *mgd_lib_prefix(); */

/* Database connection */
MGD_API midgard *mgd_setup();
//MGD_API midgard *mgd_connect(const char *database,
//			    const char *username, const char *password);
MGD_API midgard *mgd_connect(const char *hostname, const char *database,
 			    const char *username, const char *password);
MGD_API void mgd_close(midgard * mgd);
/* MGD_API gboolean mgd_reopen (midgard *mgd, const char *hostname, const char *database,
		                const char* username, const char *pass, int n_try, int sleep_seconds); */
MGD_API midgard *mgd_legacy_midgard_working_copy_new(midgard *orig);

#define MGD_CLEAR_BASE        0
#define MGD_CLEAR_SITEGROUP   1
#define MGD_CLEAR_PARSER      2
#define MGD_CLEAR_ALL         (MGD_CLEAR_BASE + MGD_CLEAR_SITEGROUP + MGD_CLEAR_PARSER)
MGD_API void mgd_clear(midgard * mgd, int clearflags);
MGD_API int mgd_errno(midgard * mgd);
MGD_API const char *mgd_error(midgard * mgd);
MGD_API int mgd_select_db(midgard *mgd, const char *database);
MGD_API int mgd_assert_db_connection(midgard *mgd, const char *hostname, const char *database,
      const char* username, const char *pass);

MGD_API int mgd_auth(midgard * mgd,
      const char *username, const char *password,
      int setuid);

MGD_API int mgd_auth_sg0(midgard * mgd,
      const char *username, const char *password,
      int setuid);

/**
 * \ingroup legacy
 *
 * Marks the given user as the current Midgard user without performing
 * any authentication checks. This method function can be used in
 * environments where users are authenticated using an external
 * authentication mechanism not normally supported by Midgard.
 *
 * \param[inout] mgd      the midgard connection
 * \param[in]    username name (possibly sitegrouped) of the authenticated user
 * \return status code
 */
MGD_API int mgd_auth_trusted(midgard *mgd, const char *username);

MGD_API int mgd_auth_is_setuid(midgard * mgd);
MGD_API int mgd_auth_unsetuid(midgard * mgd, long int rsg);

/* Language parser utilites */
MGD_API int mgd_select_parser(midgard * mgd, const char *parser);
MGD_API const char *mgd_get_parser_name(midgard * mgd);
/* Encoding of mail messages */
MGD_API const char *mgd_get_encoding(midgard * mgd);
/* Is mail should be sent as quoted printable text? */
MGD_API int mgd_mail_need_qp(midgard * mgd);

/* User information */
MGD_API int mgd_user(midgard * mgd);
MGD_API int mgd_isauser(midgard * mgd);
MGD_API int mgd_isadmin(midgard * mgd);
MGD_API int mgd_isroot(midgard * mgd);
MGD_API void mgd_force_root(midgard * mgd); /* use with extreme care */
MGD_API int mgd_isuser(midgard * mgd, int user);
MGD_API int mgd_ismember(midgard * mgd, int group);
MGD_API int *mgd_groups(midgard * mgd);
MGD_API void mgd_force_admin(midgard * mgd); /* use with extreme care */

/* Sitegroup information */
MGD_API void mgd_set_sitegroup(midgard * mgd, int sitegroup);
MGD_API int mgd_sitegroup(midgard * mgd);

#if HAVE_MIDGARD_MULTILANG
/* I18N stuff */
MGD_API int mgd_internal_set_parameters_defaultlang(midgard *mgd, int lang);
MGD_API int mgd_parameters_defaultlang(midgard * mgd);
MGD_API int mgd_internal_set_attachments_defaultlang(midgard *mgd, int lang);
MGD_API int mgd_attachments_defaultlang(midgard * mgd);
MGD_API int mgd_internal_set_lang(midgard *mgd, int lang);
MGD_API void mgd_set_default_lang(midgard *mgd, int lang);
MGD_API int mgd_get_default_lang(midgard *mgd);
MGD_API int mgd_lang(midgard * mgd);


#endif /* HAVE_MIDGARD_MULTILANG */
/* Data retrieval */
MGD_API midgard_res *mgd_query(midgard * mgd, const char *query, ...);
MGD_API midgard_res *mgd_vquery(midgard * mgd, const char *query, va_list args);
MGD_API midgard_res *mgd_ungrouped_select(midgard * mgd,
					 const char *fields, const char *from,
					 const char *where, const char *sort,
					 ...);
MGD_API midgard_res *mgd_ungrouped_vselect(midgard * mgd, const char *fields,
					  const char *from, const char *where,
					  const char *sort, va_list args);
MGD_API midgard_res *mgd_sitegroup_select(midgard * mgd,
					 const char *fields, const char *from,
					 const char *where, const char *sort,
					 ...);
MGD_API midgard_res *mgd_sitegroup_vselect(midgard * mgd, const char *fields,
					  const char *from, const char *where,
					  const char *sort, va_list args);
MGD_API midgard_res *mgd_sitegroup_record(midgard * mgd, const char *fields,
					 const char *table, int id);
#if HAVE_MIDGARD_MULTILANG
MGD_API midgard_res *mgd_sitegroup_record_lang(midgard * mgd,
				  const char *fields, const char *table, int id);

#endif /* HAVE_MIDGARD_MULTILANG */
MGD_API midgard_res *mgd_ungrouped_record(midgard * mgd,
					 const char *fields, const char *table,
					 int id);
MGD_API int mgd_idfield(midgard * mgd, const char *field, const char *table,
		       int id);
/* Only use mgd_exists_id on single-table queries that are not the
   sitegroup table */
MGD_API int mgd_exists_id(midgard * mgd, const char *from, const char *where, ...);
/* make sure you do your own sitegroup verifications, mgd_exists_bool
   doesn't */
MGD_API int mgd_exists_bool(midgard * mgd, const char *from, const char *where, ...);
MGD_API int mgd_global_exists(midgard * mgd, const char *where, ...);

MGD_API int mgd_fetch(midgard_res * res);
MGD_API void mgd_release(midgard_res * res);

MGD_API int mgd_rows(midgard_res * res);
MGD_API int mgd_cols(midgard_res * res);

MGD_API const char *mgd_colname(midgard_res * res, int col);
MGD_API const char *mgd_colvalue(midgard_res * res, int col);
MGD_API int mgd_colisnum(midgard_res * res, int col);
MGD_API const char *mgd_column(midgard_res * res, const char *name);

MGD_API int mgd_sql2id(midgard_res * res, int col);
MGD_API int mgd_sql2int(midgard_res * res, int col);
MGD_API const char *mgd_sql2str(midgard_res * res, int col);
MGD_API time_t mgd_sql2date(midgard_res * res, int col);
MGD_API time_t mgd_sql2time(midgard_res * res, int col);
MGD_API time_t mgd_sql2datetime(midgard_res * res, int col);

/* Data modification */
MGD_API int mgd_exec(midgard * mgd, const char *command, ...);
MGD_API int mgd_vexec(midgard * mgd, const char *command, va_list args);
MGD_API int mgd_create(midgard * mgd, const char *table,
		      const char *fields, const char *values, ...);
MGD_API int mgd_vcreate(midgard * mgd, const char *table,
		       const char *fields, const char *values, va_list args);
MGD_API int mgd_update(midgard * mgd, const char *table, int id,
		      const char *fields, ...);
MGD_API int mgd_vupdate(midgard * mgd, const char *table, int id,
		       const char *fields, va_list args);
MGD_API int mgd_delete(midgard * mgd, const char *table, int id);

/* repligard functions */
MGD_API int mgd_create_repligard(midgard * mgd, const char *table,
				const char *fields, const char *values, ...);
MGD_API int mgd_vcreate_repligard(midgard * mgd, const char *table,
				 const char *fields, const char *values,
				 va_list args);
MGD_API int mgd_update_repligard(midgard * mgd, const char *table, int id,
				const char *fields, ...);
MGD_API int mgd_vupdate_repligard(midgard * mgd, const char *table, int id,
				 const char *fields, va_list args);
MGD_API int mgd_delete_repligard(midgard * mgd, const char *table, int id);

/* utility functions */
MGD_API midgard_pool *mgd_pool(midgard * mgd);
MGD_API midgard_pool *mgd_res_pool(midgard_res * res);
MGD_API void mgd_set_blobdir(midgard *mgd, char *blobdir);
MGD_API const char* mgd_get_blobdir(midgard *mgd);
#if HAVE_MIDGARD_VC
MGD_API void mgd_set_cvs_script(midgard *mgd, char *cvs_script);
MGD_API const char* mgd_get_cvs_script(midgard *mgd);
#endif /* HAVE_MIDGARD_VC */
#if HAVE_MIDGARD_QUOTA
MGD_API void mgd_set_quota(midgard *mgd, int quota);
MGD_API int mgd_get_quota(midgard *mgd);
MGD_API int mgd_get_quota_count(midgard *mgd, const char* table, int sitegroup);
MGD_API int mgd_get_quota_space(midgard *mgd, const char* table, const char* fields, int sitegroup);
MGD_API int mgd_touch_recorded_quota(midgard *mgd, const char* table, int sitegroup);
MGD_API void mgd_touch_quotacache(midgard *mgd);
MGD_API void mgd_set_quota_error(midgard *mgd, int nr);
MGD_API int mgd_get_quota_error(midgard *mgd);
#endif /* HAVE_MIDGARD_QUOTA */
MGD_API void mgd_set_ah_prefix(midgard *mgd, char* prefix);
MGD_API char *mgd_get_ah_prefix(midgard *mgd);
MGD_API char *mgd_format(midgard * mgd, midgard_pool * pool, const char *fmt,
			...);
MGD_API char *mgd_vformat(midgard * mgd, midgard_pool * pool, const char *fmt,
			 va_list args);

#if HAVE_MIDGARD_VC
MGD_API void mgd_cvs_dump_nowait(midgard * mgd, const char* table, int id);
MGD_API void mgd_cvs_dump_nowait_guid(midgard * mgd, const char* guid);
MGD_API int mgd_vc_create_repligard(midgard *mgd, const char *table, int id);
#endif /* HAVE_MIDGARD_VC */

/* Repligard utilities */
MGD_API char* mgd_repligard_guid(midgard* mgd, midgard_pool* pool, const char* table, int id);
MGD_API int mgd_repligard_id(midgard* mgd, const char* guid);
MGD_API int mgd_delete_repligard_guid(midgard* mgd, const char* guid);
MGD_API int mgd_delete_repligard(midgard* mgd, const char* table, int id);
MGD_API char* mgd_repligard_changed(midgard* mgd, midgard_pool* pool, const char* table, int id);
MGD_API char* mgd_repligard_changed_guid(midgard* mgd, midgard_pool* pool, const char* guid);
MGD_API char* mgd_repligard_updated(midgard* mgd, midgard_pool* pool, const char* table, int id);
MGD_API char* mgd_repligard_updated_guid(midgard* mgd, midgard_pool* pool, const char* guid);
MGD_API char* mgd_repligard_table(midgard* mgd, midgard_pool* pool, const char* guid);
MGD_API char* mgd_repligard_action(midgard* mgd, midgard_pool* pool, const char* guid);

/* Updates Repligard's records for given table: creates if they aren't exist */
MGD_API void mgd_repligard_touch(midgard * mgd, const char *table);

/* Creates a record in the Repligard table. Generates guid based on object ID, 
   class (table name) and current time. GUID is a md5 sum of  'magic string'
*/
# define CREATE_REPLIGARD(mgd, table,id)

/* Updates a record in the Repligard table. Sets changed time to the current time */
#define UPDATE_REPLIGARD(mgd, table,id)\
        mgd_update_repligard(mgd, table, id, "changed=NULL,action='update'");
/* Updates a record in the Repligard table. Sets changed time to the specified one */
#define UPDATE_REPLIGARD_TIME(mgd, table,id, changed)\
        mgd_update_repligard(mgd, table, id, "changed=$q,action='update'", changed);

/* Marks a record in the Repligard table for deletion */
#define DELETE_REPLIGARD(mgd, table,id)\
        mgd_update_repligard(mgd, table, id, "updated=0,action='delete'");

/* Tree management */
/*! one tree node */
struct _tree_node {
	int id;						//!< id of the node
	int up;						//!< id of its parent
	int level;					//!< level in the tree from the root (root=0)
	int size;					//!< size of subtree including this node
	struct _tree_node * child;	//!< first child (none when =NULL)
	struct _tree_node * next;	//!< next sibling (last when =NULL)
};

typedef struct _tree_node tree_node;

struct _grp_tree_node {
   int id;                          /* !< id of the node */
   struct _grp_tree_node * child;   /* !< first child (none when = NULL) */
};

typedef struct _grp_tree_node grp_tree_node;


/* Returns raw tree information about given table.
   Maximum possible level of branches is returned in maxlevel. 
   Sort specifies sorting and defaults to 'id' if omitted.
   Actual value of sort should be name of one of the fields in record.
*/
tree_node * mgd_tree_build(midgard * mgd, midgard_pool * pool,
			const char * table, const char * upfield, int root,
			int maxlevel, const char * sort);
/* Returns array of ids of branches under selected tree with given id 
   but no deeper than maxlevel. If maxlevel == 0 then whole tree will 
   be returned. If id == 0 then all distinct branches will be returned.
*/
MGD_API int *mgd_tree(midgard * mgd, const char * table, const char * upfield,
			int id, int maxlevel, const char *sort);
MGD_API int mgd_is_in_tree(midgard * mgd, const char *table, const char *upfield,
				int root, int id);
/*! the prototype of a user defined function (see mgd_walk_table_tree()) */
typedef int (*midgard_userfunc) (midgard * mgd, int id, int level,
				  void *xparam);

/* Traverses table tree starting from root and calls user-defined function */
MGD_API int mgd_walk_table_tree(midgard * mgd, const char *table,
				const char * upfield, int root,
				int maxlevel, int order, void *xparam,
				midgard_userfunc func, const char * sort);

MGD_API int mgd_copy_object(midgard * mgd, midgard_res * object,
			   const char *table, const char *upfield, int new_up);
MGD_API int mgd_move_object(midgard * mgd, const char *table,
			   const char *upfield, int id, int newup);

/* Copies topic sub-tree with included articles and return ID of copy 
   of root topic
*/
MGD_API int mgd_copy_topic(midgard * mgd, int root);

/* Deletes topic sub-tree with included articles */
MGD_API int mgd_delete_topic(midgard * mgd, int root);

/* Copies page sub-tree with included pageelements and return ID of copy 
   of page
*/
MGD_API int mgd_copy_page(midgard * mgd, int root);

/* Copy article/pageelement/element to specified topic/page/style
   If newtopic/newpage/newstyle is equal to 0, a copy will be created
   in old container.
*/
MGD_API int mgd_copy_article(midgard * mgd, int id, int newtopic);
MGD_API int mgd_copy_page_element(midgard * mgd, int id, int newpage);
MGD_API int mgd_copy_element(midgard * mgd, int id, int newstyle);
MGD_API int mgd_copy_snippet(midgard * mgd, int id, int newsnippetdir);

/* Deletes page sub-tree with included page elements */
MGD_API int mgd_delete_page(midgard * mgd, int root);

/* Copies style sub-tree with included elements and return ID of copy 
   of style
*/
MGD_API int mgd_copy_style(midgard * mgd, int root);

/* Deletes style sub-tree with included elements */
MGD_API int mgd_delete_style(midgard * mgd, int root);

/* Copies snippetdir sub-tree with included snippets and return ID of copy 
   of snippetdir
*/
MGD_API int mgd_copy_snippetdir(midgard * mgd, int root);

/* Deletes snippetdir sub-tree with included snippets */
MGD_API int mgd_delete_snippetdir(midgard * mgd, int root);

#define MIDGARD_PATH_ELEMENT_NOTEXISTS		0
#define MIDGARD_PATH_ELEMENT_EXISTS		1
#define MIDGARD_PATH_OBJECT_NOTEXISTS		2
#define MIDGARD_PATH_OBJECT_EXISTS		3
typedef int (*midgard_pathfunc) (midgard * mgd, const char *table,
				 const char *name, int up, int id, int flag);

/* Parses a path and returns IDs of requested object and its uplink in 'id' and 'up'. Returns 0 on success */
MGD_API int mgd_parse_path(midgard * mgd, const char *path, const char *uptable,
			  const char *table, const char *upfield,
			  const char *namefield, int *id, int *up);
/* The same but hook is called whenever path element is queried. 'flag' will be set to one of predefined state */
/* (id is undefined in such case) but parsing will continue if hook returns !0 */
MGD_API int mgd_parse_path_with_hook(midgard * mgd, const char *path,
				    const char *uptable, const char *table,
				    const char *upfield, const char *namefield,
				    int *id, int *up, midgard_pathfunc hook);
/* Several predefined macroses to parse common cases: */
/* 1. Parse standard 'id'-'up'-'name' tables */
#define MGD_PARSE_COMMON_PATH(mgd, path, uptable, table, id, up)\
    mgd_parse_path(mgd, path, uptable, table, "up", "name", id, up)
/* 2. Parse articles by 'name' field */
#define MGD_PARSE_ARTICLE_PATH(mgd, path, id, up)\
    mgd_parse_path(mgd, path, "topic", "article", "topic", "name", id, up)
/* 3. Parse articles by 'title' field */
#define MGD_PARSE_ARTICLE_PATH_BY_TITLE(mgd, path, id, up)\
    mgd_parse_path(mgd, path, "topic", "article", "topic", "title", id, up)
/* 4. Parse topics only */
#define MGD_PARSE_TOPIC_PATH(mgd, path, up)\
    mgd_parse_path(mgd, path, "topic", NULL, NULL, NULL, NULL, up)

/* functions exported from tree.c */

int mgd_delete_all_attachments(midgard * mgd, int id, const char * table);
int mgd_delete_article(midgard * mgd, int root);
int mgd_delete_topic(midgard * mgd, int root);
int mgd_delete_page(midgard * mgd, int root);
int mgd_delete_snippetdir(midgard * mgd, int root);
int mgd_delete_event(midgard * mgd, int root);
int mgd_delete_page_element(midgard * mgd, int id);
int mgd_delete_element(midgard * mgd, int id);
int mgd_delete_style(midgard * mgd, int root);
int mgd_has_dependants(midgard * mgd, int id, char * table);
int mgd_copy_article(midgard * mgd, int root, int newtopic);
int mgd_copy_page_element(midgard * mgd, int id, int newpage);
int mgd_copy_element(midgard * mgd, int id, int newstyle);
int mgd_copy_snippet(midgard * mgd, int id, int newsnippetdir);
int mgd_copy_topic(midgard * mgd, int root);
int mgd_copy_event(midgard * mgd, int root);
int mgd_copy_page(midgard * mgd, int root);
int mgd_copy_style(midgard * mgd, int root);
int mgd_copy_snippetdir(midgard * mgd, int root);
int mgd_copy_object(midgard * mgd, midgard_res * object, const char *table,
		    const char *upfield, int new_up);
int mgd_copy_object_article(midgard * mgd, midgard_res * object,
		    int new_topic, int new_up);

/* access.c */
MGD_API int mgd_istopicreader(midgard *mgd, int topic);
MGD_API int mgd_isarticlereader(midgard *mgd, int article);
MGD_API int mgd_istopicowner(midgard *mgd, int topic);
MGD_API int mgd_issnippetdirowner(midgard *mgd, int snippetdir);
MGD_API int mgd_isgroupowner(midgard *mgd, int gid);
MGD_API int mgd_isgroupreader(midgard *mgd, int gid);
MGD_API int mgd_isuserowner(midgard *mgd, int uid);
#if HAVE_MIDGARD_PAGELINKS
MGD_API int mgd_ispagelinkowner(midgard *mgd, int pagelink);
#endif
MGD_API int mgd_isarticleowner(midgard *mgd, int article);
MGD_API int mgd_isarticlelocker(midgard *mgd, int article);
MGD_API int mgd_iseventowner(midgard *mgd, int event);
MGD_API int mgd_ishostowner(midgard *mgd, int host);
MGD_API int mgd_ispageowner(midgard *mgd, int page);
#if HAVE_MIDGARD_MULTILANG
/* i18n */
MGD_API int mgd_ispagecontentowner(midgard *mgd, int page, int lang);
#endif /* HAVE_MIDGARD_MULTILANG */
MGD_API int mgd_isstyleowner(midgard *mgd, int style);

#if HAVE_MIDGARD_MULTILANG

MGD_API int mgd_global_is_owner_ex_lang(midgard *mgd, int table, int id, int dbg, int content, int lang);
MGD_API int mgd_global_is_owner_lang(midgard *mgd, int table, int id, int content, int lang);
#endif /* HAVE_MIDGARD_MULTILANG */
MGD_API int mgd_global_is_owner_ex(midgard *mgd, int table, int id, int dbg);
#if ! HAVE_MIDGARD_MULTILANG
#define mgd_global_is_owner(mgd, table, id) mgd_global_is_owner_ex(mgd, table, id, 0) 
#else /* HAVE_MIDGARD_MULTILANG */
MGD_API int mgd_global_is_owner(midgard *mgd, int table, int id);
#endif /* HAVE_MIDGARD_MULTILANG */

MGD_API int mgd_lookup_table_id(const char *table);

MGD_API int midgard_lib_compiled_with_sitegroups();

#if MIDGARD_PHP_REQUEST_CONFIG_BUG_WORKAROUND
void mgd_php_bug_workaround_set_rcfg_dcfg(void *rcfg, void *dcfg);
void* mgd_php_bug_workaround_get_rcfg();
void* mgd_php_bug_workaround_get_dcfg();
#endif

/* NTLMSSP support. It actually works with hooks provided by upper layers in calls to
 * midgard_ntlm_process_auth() and contains no code that requires external dependencies
 * other than libc.
 */
#define NTLM_MAX_BUF_SIZE 2048

typedef enum {
        MGD_NTLM_INITIATE = 0,
        MGD_NTLM_WAIT_NEGOTIATE,
        MGD_NTLM_AUTHORIZED,
        MGD_NTLM_UNAUTHORIZED,
	MGD_NTLM_START,
	MGD_NTLM_REFRESH_AUTH
} midgard_ntlm_auth_state;

void midgard_ntlm_set_pipes(FILE *pipe_in, FILE *pipe_out);

midgard_ntlm_auth_state midgard_ntlm_process_auth(midgard *mgd, midgard_ntlm_auth_state auth_state, 
		  /* Hook for helper to get client reponses */
		  const char * (*get_info)(void *data),
		  /* Hook for helper to set our responses */
		  void(*set_info)(const char *info, void *data),
		  /* Hook for identifying policy and returning logon string for mgd_auth */
		  /* Returned values: authline -- logon string, (result) - [0 - forbid], [1 - normal], [2 - setuid] */
		  /* Will be called just before finishing authentication process */
		  int(*define_policy)(midgard *mgd, void *data, const char *username, const char *domain, const char **authline),
		  void *data);

void mgd_set_authtype(midgard *mgd, midgard_auth_type authtype);

MGD_API void mgdlib_init();
MGD_API midgard *midgard_config_init(gchar *fname);

/* Start MQL functions */

/* Select data from data storage which allow create single type */
MGD_API midgard_res *mgdlib_mql_select_type(midgard * mgd, const gchar *table,
        const char *select, const char *from, const char *where);

MGD_API gpointer midgard_query_field_get(midgard *mgd, const gchar *field,
        const gchar *from, const gchar *where);

/* End MQL */

/* Temporary, delete after 1.8 */
gboolean mgd_check_quota(midgard * mgd, const char *table);

#define midgard_lang_get(m) \
    m->lang

#define midgard_lang_set(m,i) \
    m->lang = i

/* Get sitegroup's id 
 *
 * @param mgd Midgard structure
 *
 * @return integer which identifies sitegroup
 */ 
MGD_API guint midgard_sitegroup_get_id(midgard * mgd);

/* Free midgard structure 
 *
 * This function should be called only when application is closed.
 * All midgard structure members are freed.
 *
 * @param mgd midgard structure
 */
MGD_API void midgard_shutdown(midgard *mgd);

void mgd_legacy_load_sitegroup_cache(midgard *mgd);

#endif /* MIDGARD_LEGACY_H */

