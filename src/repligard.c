/* $Id: repligard.c 15873 2008-03-27 15:11:38Z piotras $
 *
 * midgard-lib: database access for Midgard clients
 *
 * Copyright (C) 2000 The Midgard Project ry
 * Copyright (C) 2000 Alexander Bokovoy and Aurora S.A.
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

#include <config.h>
#include "midgard/midgard_legacy.h"
#include "defaults.h"
#include "midgard/parser.h"
#include "fmt_russian.h"
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif
#ifdef WIN32
#include <process.h>
#include "win32/win95nt.h"
#endif

#include "midgard_mysql.h"

#if HAVE_MIDGARD_VC
int mgd_vc_create_repligard(midgard *mgd, const char *table, int id) {
	int ret;
	gchar *guid = midgard_guid_new(mgd);
	
	/* Do not update guid in multilang tables */
	if(!g_str_has_suffix(table, "_i")) {
		GString *guid_sql = g_string_new("UPDATE "); 
		g_string_append_printf(guid_sql,
				" %s SET guid='%s' WHERE id=%d AND sitegroup=%d", 
				table, guid, id,  mgd_sitegroup(mgd));
		gchar *tmpstr = g_string_free(guid_sql, FALSE); 
		mgd_exec(mgd, tmpstr, NULL); 
		g_free(tmpstr);
	}		
	ret = mgd_create_repligard(mgd, table, "guid,id,changed,action,sitegroup",
			"$q,$d,NULL,'create',$d",guid, id, mgd_sitegroup(mgd));
	mgd_cvs_dump_nowait_guid(mgd, guid);
	g_free(guid);
	return ret;
}
#endif /* HAVE_MIDGARD_VC */

int mgd_create_repligard(midgard * mgd, const char *table,
			 const char *fields, const char *values, ...)
{
	int id;
	va_list args;
	va_start(args, values);
	id = mgd_vcreate_repligard(mgd, table, fields, values, args);
	va_end(args);
	return id;
}

int mgd_vcreate_repligard(midgard * mgd, const char *table,
			  const char *fields, const char *values, va_list args)
{
	const char *command;
	int rv;
	gchar *typename = NULL;

	/* Legacy objects ugly workaround */
	if(g_str_equal(table, "article"))
		typename = "midgard_article";	
	if(g_str_equal(table, "blobs"))
		typename = "midgard_attachment";
	if(g_str_equal(table, "element"))
		typename = "midgard_element";
	if(g_str_equal(table, "event"))
		typename = "midgard_event";
	if(g_str_equal(table, "eventmember"))
		typename = "midgard_eventmember";
	if(g_str_equal(table, "grp"))
		typename = "midgard_group";
	if(g_str_equal(table, "host"))
		typename = "midgard_host";
	if(g_str_equal(table, "member"))
		typename = "midgard_member";
	if(g_str_equal(table, "page"))
		typename = "midgard_page";
	if(g_str_equal(table, "pageelement"))
		typename = "midgard_pageelement";
	if(g_str_equal(table, "person"))
		typename = "midgard_person";
	if(g_str_equal(table, "record_extension"))
		typename = "midgard_parameter";
	if(g_str_equal(table, "snippet"))
		typename = "midgard_snippet";
	if(g_str_equal(table, "snippetdir"))
		typename = "midgard_snippetdir";
	if(g_str_equal(table, "style"))
		typename = "midgard_style";
	if(g_str_equal(table, "topic"))
		typename = "midgard_topic";
	
	if(typename == NULL)
		typename = "";
	/* End of legacy workaround */

	/* format command */
#if HAVE_MIDGARD_VC

	if (mgd->cvs_script) {
	command = mgd_format(mgd, mgd->tmp,
			       "INSERT INTO repligard (realm,locked,author,typename,$s) VALUES ($q,1,$d,$q,$s)",
			       fields, table, mgd->current_user->id, typename,values);
	} else {
#endif /* HAVE_MIDGARD_VC */
	command = mgd_format(mgd, mgd->tmp,
			     "INSERT INTO repligard (realm,typename,$s) VALUES ($q,$q,$s)",
			     fields, table, typename,values);
#if HAVE_MIDGARD_VC
	}
	
#endif /* HAVE_MIDGARD_VC */
	if (!command)
		return 0;
	/* execute command */
	rv = mgd_vexec(mgd, command, args);	
	
	mgd_clear_pool(mgd->tmp);
	return rv ? mysql_insert_id(mgd->msql->mysql) : 0;
}

int mgd_update_repligard(midgard * mgd, const char *table, int id,
			 const char *fields, ...)
{
	int rv;
	va_list args;
	va_start(args, fields);
	rv = mgd_vupdate_repligard(mgd, table, id, fields, args);
	va_end(args);
	return id;
}


int mgd_vupdate_repligard(midgard * mgd, const char *table, int id,
			  const char *fields, va_list args)
{
	const char *command;
	int rv;

	if(g_str_has_suffix(table, "_i")) 
		return 0;

	midgard_res *res = mgd_sitegroup_select(
			mgd, "guid" , table,
			"id=$d", NULL, id);
	if (!res || !mgd_fetch(res)) {
		if (res) mgd_release(res);
		return 0;
	}
	gchar *guid = (gchar *)mgd_colvalue(res, 0);
		
	/* format command */
#if HAVE_MIDGARD_VC
	
	if (mgd->cvs_script) {
	command = mgd_format(mgd, mgd->tmp,
			       "UPDATE $s SET locked=1,author=$d,$s WHERE guid=$q AND realm=$q",
			       "repligard", mgd->current_user->id, fields, guid, table);
	  
	} else {
#endif /* HAVE_MIDGARD_VC */
	command = mgd_format(mgd, mgd->tmp,
			     "UPDATE $s SET $s WHERE guid=$q AND realm=$q",
			     "repligard", fields, guid, table);
#if HAVE_MIDGARD_VC
	}
#endif /* HAVE_MIDGARD_VC*/
	mgd_release(res);
	if (!command)
		return 0;

	/* execute command */
	rv = mgd_vexec(mgd, command, args);	

#if HAVE_MIDGARD_VC
	mgd_cvs_dump_nowait(mgd,table,id);
#endif /* HAVE_MIDGARD_VC */

	mgd_clear_pool(mgd->tmp);
	return rv;
}

int mgd_delete_repligard(midgard * mgd, const char *table, int id)
{
	const char *command;
	int rv;

	/* format command */
	command =
	   mgd_format(mgd, mgd->tmp,
		      "DELETE FROM repligard WHERE realm=$q AND id=$d", table,
		      id);
	if (!command)
		return 0;

	/* execute command */
	rv = mgd_exec(mgd, command);

	mgd_clear_pool(mgd->tmp);
	return rv;
}

int mgd_delete_repligard_guid(midgard * mgd, const char *guid)
{
	const char *command;
	int rv;

	/* format command */
	command =
	   mgd_format(mgd, mgd->tmp,
		      "DELETE FROM repligard WHERE guid=$q", guid);
	if (!command)
		return 0;

	/* execute command */
	rv = mgd_exec(mgd, command);

	mgd_clear_pool(mgd->tmp);
	return rv;
}


char *mgd_repligard_guid(midgard * mgd, midgard_pool * pool, const char *table,
			int id)
{
	midgard_res *res;
	char *guid;
	res =
	   mgd_ungrouped_select(mgd, "guid", "repligard", "realm=$q AND id=$d",
				NULL, table, id);
	if (res && mgd_fetch(res)) {
		guid = mgd_strdup(pool, mgd_colvalue(res, 0));
		mgd_release(res);
		return guid;
	}
	return NULL;
}

char *mgd_repligard_changed(midgard * mgd, midgard_pool * pool, const char *table, int id)
{
	midgard_res *res;
	char* changed;
	res = mgd_ungrouped_select(mgd, "changed",
				   "repligard", "realm=$q AND id=$d",
				   NULL, table, id);
	if (res && mgd_fetch(res)) {
		changed = mgd_strdup(pool, mgd_colvalue(res, 0));
		mgd_release(res);
		return changed;
	}
	return NULL;
}

char *mgd_repligard_changed_guid(midgard * mgd, midgard_pool * pool, const char *guid)
{
	midgard_res *res;
	char* changed;
	res = mgd_ungrouped_select(mgd, "changed",
				   "repligard", "guid=$q",
				   NULL, guid);
	if (res && mgd_fetch(res)) {
		changed = mgd_strdup(pool, mgd_colvalue(res, 0));
		mgd_release(res);
		return changed;
	}
	return NULL;
}

char *mgd_repligard_updated(midgard * mgd, midgard_pool * pool, const char *table, int id)
{
	midgard_res *res;
	char* updated;
	res = mgd_ungrouped_select(mgd, "updated",
				   "repligard", "realm=$q AND id=$d",
				   NULL, table, id);
	if (res && mgd_fetch(res)) {
		updated = mgd_strdup(pool, mgd_colvalue(res, 0));
		mgd_release(res);
		return updated;
	}
	return NULL;
}

char *mgd_repligard_updated_guid(midgard * mgd, midgard_pool * pool, const char *guid)
{
	midgard_res *res;
	char* updated;
	res = mgd_ungrouped_select(mgd, "updated",
				   "repligard", "guid=$q",
				   NULL, guid);
	if (res && mgd_fetch(res)) {
		updated = mgd_strdup(pool, mgd_colvalue(res, 0));
		mgd_release(res);
		return updated;
	}
	return NULL;
}

char* mgd_repligard_table(midgard * mgd, midgard_pool* pool, const char *guid)
{
	midgard_res *res;
	char* table;
	res =
	   mgd_ungrouped_select(mgd, "realm", "repligard", "guid=$q",
				NULL, guid);
	if (res && mgd_fetch(res)) {
		table = mgd_strdup(pool, mgd_colvalue(res, 0));
		mgd_release(res);
		return table;
	}
	return NULL;
}

char* mgd_repligard_action(midgard * mgd, midgard_pool* pool, const char *guid)
{
	midgard_res *res;
	char* action;
	res =
	   mgd_ungrouped_select(mgd, "action", "repligard", "guid=$q",
				NULL, guid);
	if (res && mgd_fetch(res)) {
		action = mgd_strdup(pool, mgd_colvalue(res, 0));
		mgd_release(res);
		return action;
	}
	return NULL;
}

int mgd_repligard_id(midgard * mgd, const char *guid)
{
	midgard_res *res;
	int id;
	res =
	   mgd_ungrouped_select(mgd, "id", "repligard", "guid=$q",
				NULL, guid);
	if (res && mgd_fetch(res)) {
		id = mgd_sql2id(res, 0);
		mgd_release(res);
		return id;
	}
	return 0;
}

/*  Touches all records from table and set their timestamps in Repligard
    If record doesn't exist in Repligard, it will be created.
*/
void mgd_repligard_touch(midgard * mgd, const char *table)
{
	midgard_res *res;

	res = mgd_ungrouped_select(mgd, "id", table, NULL, NULL);
	if (res) {
		while (mgd_fetch(res)) {
			if (mgd_exists_bool(mgd, "repligard", "id=$d AND realm=$q",
				       mgd_sql2id(res, 0), table)) {
				UPDATE_REPLIGARD(mgd, table, mgd_sql2id(res, 0))
			}
			else {
				CREATE_REPLIGARD(mgd, table, mgd_sql2id(res, 0))
			}
		}
		mgd_release(res);
	}
}

