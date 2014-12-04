/* $Id: tree.c 16963 2008-07-23 19:06:40Z piotras $
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

#include <config.h>
#include "midgard/midgard_legacy.h"
#include "defaults.h"
#if HAVE_CRYPT_H
#include <crypt.h>
#else
#define _XOPEN_SOURCE
#ifndef WIN32
#include <unistd.h>
#endif
#endif
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <time.h>

static void mgd_copy_all_parameters(midgard * mgd, int id, const char * table,
                                                    int newoid)
{
	midgard_res *res;
	res =
	   mgd_sitegroup_select(mgd, "*", "record_extension",
					                  "record_extension.tablename=$q "
									  "AND record_extension.oid=$i",
								NULL, table, id);
	if (res) {
		while (mgd_fetch(res)) {
			mgd_copy_object ( mgd, res, "record_extension", "oid", newoid);
		}
		mgd_release(res);
	}
}

#if HAVE_MIDGARD_MULTILANG
static int mgd_copy_all_content(midgard * mgd, int id, const char * table,
                                                    int newsid)
{
	midgard_res *res;
	char * table_i = malloc(256 * sizeof(char));
	int retval = 0;
	assert (strlen(table) < 254);
	strcpy (table_i, table);
	strcat (table_i, "_i");
	
	res =
	   mgd_ungrouped_select(mgd, "*", table_i,
					                 "$s.sid=$d",
								NULL, table_i, id);

	if (res) {
		while (mgd_fetch(res)) {
		  retval &= mgd_copy_object ( mgd, res, table_i, "sid", newsid);
		}
		mgd_release(res);
	}
	free(table_i);
	return retval;
}


#endif /* HAVE_MIDGARD_MULTILANG */
/* DG: this function to copy blobs (to allow moving the blob from one type of object to another)
 * Can be optimized... */
static int mgd_copy_object_blob(midgard * mgd, midgard_res * object, 
		                  int new_pid, const char *new_ptable)
{
#define MGD_BLOB_BUF_SIZE 4096
	FILE * fpo, * fpi;
	midgard_pool *pool;
	char *fields, *values;
	char new_location[MIDGARD_REPLIGARD_TAG_LENGTH + 5];
	static char dir[16] = "0123456789ABCDEF";
	char *new_path, *old_path;
	size_t sz;
	char * buf;
	const char *old_location = NULL;
	int i;

	if (!object)
		return 0;
	pool = mgd_alloc_pool();
	if (!pool)
		return 0;

        /* compute the new_location string */
        gchar *guid = midgard_guid_new(mgd);
        strcpy(new_location + 4, guid);
        g_free(guid);

	/* some basic hashing to aid in dirsize balancing. This probably
		needs refinement since the rc5 checksum we use to generate the names
		returns all readable chars which don't distribute well with the
		simple method below
	*/
	new_location[1] = new_location[3] = '/';
	new_location[0] = dir[ ((unsigned char)new_location[4]) / 16 ];
	new_location[2] = dir[ ((unsigned char)new_location[4]) % 16 ];

	new_path = mgd_format(mgd, pool, "$s/$s", 
					mgd_get_blobdir(mgd), new_location);

	/* 1. Create fields for query */
	fields = NULL;
	values = NULL;
	for (i = 0; i < mgd_cols(object); i++) {
		if ((strcmp("id", mgd_colname(object, i)) != 0)
		    && (strcmp("sitegroup", mgd_colname(object, i)) != 0)
		   ) {
			fields = fields ?
			   mgd_format(mgd, pool, "$s,$s", fields,
				      mgd_colname(object, i)) : mgd_strdup(pool,
									   mgd_colname (object, i));
			/* Replace 'pid' field by new value */
			if (!strcmp("pid", mgd_colname(object, i))) {
				values =
				   values ? mgd_format(mgd, pool, "$s,$i",
						       values, new_pid) : mgd_format(mgd,
									    pool, "$i", new_pid);
			}
			/* Replace 'ptable' field by new value */
			else if (new_ptable && !strcmp("ptable", mgd_colname(object, i))) {
				values =
				   values ? mgd_format(mgd, pool, "$s,$q",
						       values, new_ptable) : mgd_format(mgd,
									    pool, "$q", new_ptable);
			}
			/* Replace 'location' field by new value */
			else if (!strcmp("location", mgd_colname(object, i))) {
				old_location = mgd_sql2str(object, i);
				values =
				   values ? mgd_format(mgd, pool, "$s,$q",
						       values, new_location) : mgd_format(mgd,
									    pool, "$q", new_location);
			}
			else {
				values = values ?
				   mgd_format(mgd, pool, "$s,$q", values,
					      mgd_colvalue(object,
							   i)) :
				   mgd_format(mgd, pool, "$q", mgd_colvalue(object, i));
			}
		}
	}
	/* 2. Create new object and update Repligard */
	i = mgd_create(mgd, "blobs", fields, "$s", values);
	if (i) {
/* TODO: insert here the code to copy(blobdir+old_location, blobdir+new_location) */
		if((buf = mgd_alloc(pool, MGD_BLOB_BUF_SIZE))) {
			assert(old_location);
			old_path = mgd_format(mgd, pool, "$s/$s", 
							mgd_get_blobdir(mgd), old_location);
			if ((fpo = fopen(new_path, "w")) == NULL) {
				mgd_free_pool(pool);
				return 0;
			}
			else if ((fpi = fopen(old_path, "r")) == NULL) {
				fclose(fpo);
				mgd_free_pool(pool);
				return 0;
			}
			while(!feof(fpi) && !ferror(fpi) && !ferror(fpo)) {
				if((sz = fread(buf, 1, MGD_BLOB_BUF_SIZE, fpi))) {
					fwrite(buf, 1, sz, fpo);
				}
			}
			if(ferror(fpi) || ferror(fpo)) {
				fclose(fpi); fclose(fpo);
				mgd_free_pool(pool);
/* TODO: ROLLBACK: too bad we don't have transaction support :( */
				return 0;
			}
			fclose(fpi);
			fclose(fpo);
		}
		CREATE_REPLIGARD(mgd, "blobs", i)
	}
	mgd_free_pool(pool);
	return i;
}

static void mgd_copy_all_attachments(midgard * mgd, int id, const char * table,
                                                    int newid, const char * newtable)
{
	midgard_res *res;
	int i;
	res =
	   mgd_sitegroup_select(mgd, "*", "blobs", "blobs.ptable=$q AND blobs.pid=$i",
								NULL, table, id);
	if (res) {
		while (mgd_fetch(res)) {
	/* First copy the blob */
			i=mgd_copy_object_blob(mgd, res, newid, newtable);
	/* Then copy any parameters linked to the blob */
			if(i) mgd_copy_all_parameters ( mgd, 
							strtol(mgd_column(res, "id"), NULL, 10),
							"blobs", i);
		}
		mgd_release(res);
	}
	
}

int mgd_copy_object(midgard * mgd, midgard_res * object, const char *table,
		    const char *upfield, int new_up)
{
	midgard_pool *pool;
	char *fields, *values, *stmp;
	int i;

	if (!object)
		return 0;
	pool = mgd_alloc_pool();
	if (!pool)
		return 0;
	/* 1. Create fields for query */
	fields = NULL;
	values = NULL;
	for (i = 0; i < mgd_cols(object); i++) {
		if ((strcmp("id", mgd_colname(object, i)) != 0)
		    && (strcmp("sitegroup", mgd_colname(object, i)) != 0)
		   ) {
			stmp = fields;
			fields = fields ?
					mgd_format(mgd, pool, "$s,$s", fields,
				      mgd_colname(object, i)) : mgd_strdup(pool,
									   mgd_colname (object, i));
			if (stmp)
				mgd_free_from_pool(pool, stmp);
			stmp = values;
			/* Replace 'up' field by new value */
			if (!strcmp(upfield, mgd_colname(object, i))) {
				values =
				   values ? mgd_format(mgd, pool, "$s,$i",
						       values,
						       new_up) : mgd_format(mgd,
									    pool,
									    "$i",
									    new_up);
			}
			else {
				values = values ?
				   mgd_format(mgd, pool, "$s,$q", values,
					      mgd_colvalue(object,
							   i)) :
				   mgd_format(mgd, pool, "$q", mgd_colvalue(object, i));
			}
			if (stmp)
				mgd_free_from_pool(pool, stmp);
		}
	}
	/* 2. Create new object and update Repligard */
	i = mgd_create(mgd, table, fields, "$s", values);
	if (i) {
		CREATE_REPLIGARD(mgd, table, i)
	}
	mgd_free_pool(pool);
	return i;
}


/* DG: this function to copy articles (they have 2 'up' fields)
 * Can be optimized... */
int mgd_copy_object_article(midgard * mgd, midgard_res * object,
		    int new_topic, int new_up)
{
	midgard_pool *pool;
	char *fields, *values, *stmp;
	int i, old_topic, force_name_change;
   time_t now = time(NULL);

	if (!object)
		return 0;
	pool = mgd_alloc_pool();
	if (!pool)
		return 0;
	/* 1. Create fields for query */
	fields = NULL;
	values = NULL;
	force_name_change = 0;
	for (i = 0; i < mgd_cols(object); i++) {
		if ((strcmp("id", mgd_colname(object, i)) != 0)
		    && (strcmp("sitegroup", mgd_colname(object, i)) != 0)
		   ) {
			stmp = fields;
			fields = fields ?
			   mgd_format(mgd, pool, "$s,$s", fields,
				      mgd_colname(object, i)) : mgd_strdup(pool,
									   mgd_colname
									   (object,
									    i));
			if (stmp)
				mgd_free_from_pool(pool, stmp);
			stmp = values;
			/* Replace 'topic' field by new value */
			if ((!strcmp("topic", mgd_colname(object, i))) && (new_topic)) {
				/* Check target topic to be the same */
				old_topic = mgd_sql2id(object, i);
				if (old_topic == new_topic) {
				    /* if true, do not forget to change article name later */
				    force_name_change = 1;
				}
				values =
				   values ? mgd_format(mgd, pool, "$s,$i",
						       values,
						       new_topic) : mgd_format(mgd,
									    pool,
									    "$i",
									    new_topic);
			}
			/* Replace 'up' field by new value */
			else if (!strcmp("up", mgd_colname(object, i))) {
				values =
				   values ? mgd_format(mgd, pool, "$s,$i",
						       values,
						       new_up) : mgd_format(mgd,
									    pool,
									    "$i",
									    new_up);
			}
			else if ((!strcmp("name", mgd_colname(object, i))) && (force_name_change)) {
			    /* Add current timestamp to the article name in order to prevent duplicated names */
				values =
				   values ? mgd_format(mgd, pool, "$s,'$Q ($Q)'",
							values,
							mgd_colvalue(object, i), asctime(gmtime(&now))) : 
					    mgd_format(mgd, pool, "'$Q ($Q)'", 
							mgd_colvalue(object, i), asctime(gmtime(&now)));
				force_name_change = 0;
			    
			}
			else {
				values = values ?
				   mgd_format(mgd, pool, "$s,$q", values,
					      mgd_colvalue(object,
							   i)) :
				   mgd_format(mgd, pool, "$q", mgd_colvalue(object, i));
			}
			if (stmp)
				mgd_free_from_pool(pool, stmp);
		}
	}
	/* 2. Create new object and update Repligard */
	i = mgd_create(mgd, "article", fields, "$s", values);
	if (i) {
		CREATE_REPLIGARD(mgd, "article", i)
	}
	mgd_free_pool(pool);
	return i;
}

static int article_copier(midgard * mgd, int id, int level, void *xprm)
{
	struct mgd_article_tree {
		int topic;
		int *ups;
	} *xparam=xprm;
	midgard_res *res;
	int i;
	int *up = xparam->ups;
	res = mgd_sitegroup_record(mgd, "*", "article", id);
	if (!res)
		return 1;
	if (mgd_fetch(res)) {
		up[level + 1] =
			i = mgd_copy_object_article(mgd, res, xparam->topic,
				   level ? up[level] : 0);
			mgd_copy_all_parameters ( mgd,
							strtol(mgd_column(res, "id"), NULL, 10),
							"article", i);
			mgd_copy_all_attachments ( mgd,
							strtol(mgd_column(res, "id"), NULL, 10),
							"article", i, NULL);
#if HAVE_MIDGARD_MULTILANG
			mgd_copy_all_content ( mgd, strtol(mgd_column(res, "id"), NULL, 10), "article", i);
		
#endif /* HAVE_MIDGARD_MULTILANG */
	}
	mgd_release(res);
	return 1;
}

int mgd_copy_article(midgard * mgd, int root, int newtopic)
{
	struct mgd_article_tree {
		int topic;
		int *ups;
	} xparam;
	int newroot;
	xparam.topic=newtopic;
	/* This is ugly but how we could find number of replies instead? */
	xparam.ups = mgd_tree(mgd, "article", "up", root, 0, NULL);
	mgd_walk_table_tree(mgd, "article", "up", root, 0, 1, (void *) &xparam,
			    						article_copier, NULL);
	newroot = xparam.ups[1];
	free(xparam.ups);
	return newroot;
}

int mgd_copy_page_element(midgard * mgd, int id, int newpage)
{
	midgard_res *res;
	int i = 0;
	assert(mgd);
	res = mgd_sitegroup_record(mgd, "*", "pageelement", id);
	if (!res)
		return 0;
	if (mgd_fetch(res))
		i = mgd_copy_object(mgd, res, "pageelement", "page",
				    newpage ? newpage :
				    strtol(mgd_column(res, "page"), NULL, 10));
	mgd_release(res);
#if HAVE_MIDGARD_MULTILANG
	mgd_copy_all_parameters (mgd, id, "pageelement", i);
	mgd_copy_all_attachments (mgd, id, "pageelement", i, NULL);
	mgd_copy_all_content(mgd, id, "pageelement", i);
#endif /* HAVE_MIDGARD_MULTILANG */
	return i;
}

int mgd_copy_element(midgard * mgd, int id, int newstyle)
{
	midgard_res *res;
	int i = 0;
	assert(mgd);
	res = mgd_sitegroup_record(mgd, "*", "element", id);
	if (!res)
		return 0;
	if (mgd_fetch(res))
		i = mgd_copy_object(mgd, res, "element", "style",
				    newstyle ? newstyle :
				    strtol(mgd_column(res, "style"), NULL, 10));
	mgd_release(res);
#if HAVE_MIDGARD_MULTILANG
	mgd_copy_all_parameters (mgd, id, "element", i);
	mgd_copy_all_attachments (mgd, id, "element", i, NULL);
	mgd_copy_all_content(mgd, id, "element", i);		
#endif /* HAVE_MIDGARD_MULTILANG */
	return i;
}

int mgd_copy_snippet(midgard * mgd, int id, int newsnippetdir)
{
	midgard_res *res;
	int i = 0;
	assert(mgd);
	res = mgd_sitegroup_record(mgd, "*", "snippet", id);
	if (!res)
		return 0;
	if (mgd_fetch(res))
		i = mgd_copy_object(mgd, res, "snippet", "up",
				    newsnippetdir ? newsnippetdir :
				    strtol(mgd_column(res, "up"), NULL, 10));
	mgd_release(res);
#if HAVE_MIDGARD_MULTILANG
	mgd_copy_all_parameters (mgd, id, "snippet", i);
	mgd_copy_all_attachments (mgd, id, "snippet", i, NULL);
	mgd_copy_all_content(mgd, id, "snippet", i);		
#endif /* HAVE_MIDGARD_MULTILANG */
	return i;
}

static int topic_copier(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res, *art_res;
	int *up = xparam;
	int i;
	res = mgd_sitegroup_record(mgd, "*", "topic", id);
	if (!res)
		return 1;
	if (mgd_fetch(res)) {
		up[level + 1] =
		   mgd_copy_object(mgd, res, "topic", "up",
				   level ? up[level] : 0);
		mgd_copy_all_parameters ( mgd, id, "topic", up[level + 1]);
		mgd_copy_all_attachments ( mgd, id, "topic", up[level + 1], NULL);
		art_res =
		   mgd_sitegroup_select(mgd, "id", "article", "article.topic=$i "
						   "AND article.up=0", "article.created", id);
		if (art_res) {
			while (mgd_fetch(art_res)) {
				i = mgd_copy_article(mgd, mgd_sql2int(art_res,0), up[level + 1] );
			}
			mgd_release(art_res);
		}
	}
	mgd_release(res);
	return 1;
}

int mgd_copy_topic(midgard * mgd, int root)
{
	int *ups, newroot;
	/* This is ugly but how we could find number of sub-topics instead? */
	ups = mgd_tree(mgd, "topic", "up", root, 0, NULL);
	mgd_walk_table_tree(mgd, "topic", "up", root, 0, 1, (void *) ups,
			    						topic_copier, NULL);
	newroot = ups[1];
	free(ups);
	return newroot;
}

static int event_copier(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res, *emb_res;
	int *up = xparam;
	int i;
	res = mgd_sitegroup_record(mgd, "*", "event", id);
	if (!res)
		return 1;
	if (mgd_fetch(res)) {
		up[level + 1] =
		   mgd_copy_object(mgd, res, "event", "up",
				   level ? up[level] : 0);
		mgd_copy_all_parameters ( mgd, id, "event", up[level + 1]);
		mgd_copy_all_attachments ( mgd, id, "event", up[level + 1], NULL);
		emb_res =
		   mgd_sitegroup_select(mgd, "*", "eventmember", "eid=$i",
					NULL, id);
		if (emb_res) {
			while (mgd_fetch(emb_res)) {
				i = mgd_copy_object(mgd, emb_res, "eventmember",
						"eid", up[level + 1]);
				mgd_copy_all_parameters ( mgd,
								strtol(mgd_column(emb_res, "id"), NULL, 10),
								"eventmember", i);
				mgd_copy_all_attachments ( mgd,
								strtol(mgd_column(emb_res, "id"), NULL, 10),
								"eventmember", i, NULL);
			}
			mgd_release(emb_res);
		}
	}
	mgd_release(res);
	return 1;
}

int mgd_copy_event(midgard * mgd, int root)
{
	int *ups, newroot;
	/* This is ugly but how we could find number of sub-topics instead? */
	ups = mgd_tree(mgd, "event", "up", root, 0, NULL);
	mgd_walk_table_tree(mgd, "event", "up", root, 0, 1, (void *) ups,
										event_copier, NULL);
	newroot = ups[1];
	free(ups);
	return newroot;
}

static int page_copier(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res, *elem_res;
	int *up = xparam;
	int i;
	res = mgd_sitegroup_record(mgd, "*", "page", id);
	if (!res)
		return 1;
	if (mgd_fetch(res)) {
		up[level + 1] =
		   mgd_copy_object(mgd, res, "page", "up",
				   level  ? up[level] : 0);
#if HAVE_MIDGARD_MULTILANG
	  mgd_copy_all_content ( mgd, id, "page", up[level + 1] );
#endif /* HAVE_MIDGARD_MULTILANG */
		mgd_copy_all_parameters ( mgd, id, "page", up[level + 1]);
		mgd_copy_all_attachments ( mgd, id, "page", up[level + 1], NULL);
		elem_res =
		   mgd_sitegroup_select(mgd, "*", "pageelement", "page=$i",
					NULL, id);
		if (elem_res) {
			while (mgd_fetch(elem_res)) {
#if ! HAVE_MIDGARD_MULTILANG
				i = mgd_copy_object(mgd, elem_res, "pageelement",
						"page", up[level + 1]);
				mgd_copy_all_parameters ( mgd,
								strtol(mgd_column(elem_res, "id"), NULL, 10),
								"pageelement", i);
				mgd_copy_all_attachments ( mgd,
								strtol(mgd_column(elem_res, "id"), NULL, 10),
								"pageelement", i, NULL);
#else /* HAVE_MIDGARD_MULTILANG */
				i = mgd_copy_page_element(mgd, strtol(mgd_column(elem_res, "id"), NULL, 10), up[level + 1]);
#endif /* HAVE_MIDGARD_MULTILANG */
			}
			mgd_release(elem_res);
		}
	}
	mgd_release(res);
	return 1;
}

int mgd_copy_page(midgard * mgd, int root)
{
	int *ups, newroot;
	/* This is ugly but how we could find number of sub-pages instead? */
	ups = mgd_tree(mgd, "page", "up", root, 0, NULL);
	mgd_walk_table_tree(mgd, "page", "up", root, 0, 1, (void *) ups,
										page_copier, NULL);
	newroot = ups[1];
	free(ups);
	return newroot;
}

static int style_copier(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res, *elem_res;
	int *up = xparam;
	int i;
	res = mgd_sitegroup_record(mgd, "*", "style", id);
	if (!res)
		return 1;
	if (mgd_fetch(res)) {
		up[level + 1] =
		   mgd_copy_object(mgd, res, "style", "up",
				   level ? up[level] : 0);
		mgd_copy_all_parameters ( mgd, id, "style", up[level + 1]);
		mgd_copy_all_attachments ( mgd, id, "style", up[level + 1], NULL);
		elem_res =
		   mgd_sitegroup_select(mgd, "*", "element", "element.style=$i",
					NULL, id);
		if (elem_res) {
			while (mgd_fetch(elem_res)) {
#if ! HAVE_MIDGARD_MULTILANG
				i = mgd_copy_object(mgd, elem_res, "element",
						"style", up[level + 1]);
				mgd_copy_all_parameters ( mgd,
								strtol(mgd_column(elem_res, "id"), NULL, 10),
								"element", i);
				mgd_copy_all_attachments ( mgd,
								strtol(mgd_column(elem_res, "id"), NULL, 10),
								"element", i, NULL);
#else /* HAVE_MIDGARD_MULTILANG */
				i = mgd_copy_element(mgd, strtol(mgd_column(elem_res, "id"), NULL, 10), up[level + 1]);
#endif /* HAVE_MIDGARD_MULTILANG */
			}
			mgd_release(elem_res);
		}
	}
	mgd_release(res);
	return 1;
}

int mgd_copy_style(midgard * mgd, int root)
{
	int *ups, newroot;
	/* This is ugly but how we could find number of sub-topics instead? */
	ups = mgd_tree(mgd, "style", "up", root, 0, NULL);
	mgd_walk_table_tree(mgd, "style", "up", root, 0, 1, (void *) ups,
			    						style_copier, NULL);
	newroot = ups[1];
	free(ups);
	return newroot;
}

static int snippetdir_copier(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res, *snip_res;
	int *up = xparam;
	int i;
	res = mgd_sitegroup_record(mgd, "*", "snippetdir", id);
	if (!res)
		return 1;
	if (mgd_fetch(res)) {
		up[level + 1] =
		   mgd_copy_object(mgd, res, "snippetdir", "up",
				   level ? up[level] : 0);
		mgd_copy_all_parameters ( mgd, id, "snippetdir", up[level + 1]);
		mgd_copy_all_attachments ( mgd, id, "snippetdir", up[level + 1], NULL);
		snip_res =
		   mgd_sitegroup_select(mgd, "*", "snippet", "up=$i", NULL, id);
		if (snip_res) {
			while (mgd_fetch(snip_res)) {
				i = mgd_copy_object(mgd, snip_res, "snippet", "up",
						up[level + 1]);
				mgd_copy_all_parameters ( mgd,
								strtol(mgd_column(snip_res, "id"), NULL, 10),
								"snippet", i);
				mgd_copy_all_attachments ( mgd,
								strtol(mgd_column(snip_res, "id"), NULL, 10),
								"snippet", i, NULL);
#if HAVE_MIDGARD_MULTILANG
				mgd_copy_all_content ( mgd,
								strtol(mgd_column(snip_res, "id"), NULL, 10),
								"snippet", i);
#endif /* HAVE_MIDGARD_MULTILANG */
			}
			mgd_release(snip_res);
		}
	}
	return 1;
	mgd_release(res);
}

int mgd_copy_snippetdir(midgard * mgd, int root)
{
	int *ups, newroot;
	/* This is ugly but how we could find number of sub-topics instead? */
	ups = mgd_tree(mgd, "snippetdir", "up", root, 0, NULL);
	mgd_walk_table_tree(mgd, "snippetdir", "up", root, 0, 1, (void *) ups,
											snippetdir_copier, NULL);
	newroot = ups[1];
	free(ups);
	return newroot;
}

#if HAVE_MIDGARD_MULTILANG
static int mgd_delete_all_content(midgard * mgd, int id, const char * table) { 
  midgard_res * res;
  
  
  char table_i[256];
  int retcode = 0;
  assert (strlen(table) < 254);
  strcpy (table_i, table);
  strcat (table_i, "_i");
	

  res =
    mgd_ungrouped_select(mgd, "id", table_i, "$s.sid=$i",
			 NULL, table_i, id);
  if (res) {
    while (mgd_fetch(res)) {
      retcode &= mgd_delete(mgd, table_i, mgd_sql2int(res, 0));
      DELETE_REPLIGARD(mgd, table_i, mgd_sql2int(res, 0));
    }
  }		
  return retcode;
}  

#endif /* HAVE_MIDGARD_MULTILANG */
static int mgd_delete_all_parameters(midgard * mgd, int id, const char * table)
{
	midgard_res *res;
	res = mgd_sitegroup_select(mgd, "id", "record_extension",
						"record_extension.tablename=$q AND record_extension.oid=$i",
						NULL, table, id);
	if (res) {
		while (mgd_fetch(res)) {
			if(!mgd_delete(mgd, "record_extension", mgd_sql2int(res, 0))) return 0;
			DELETE_REPLIGARD(mgd, "record_extension",
					     mgd_sql2int(res, 0));
		}
		mgd_release(res);
	}
	return 1;
}

int mgd_delete_all_attachments(midgard * mgd, int id, const char * table)
{
	midgard_res *res;
	midgard_pool * pool;
	char * path;
	res =
	   mgd_sitegroup_select(mgd, "id,location,sitegroup", "blobs",
			             "blobs.ptable=$q AND blobs.pid=$i",
				     	 NULL, table, id);
	if (res) {
		pool = mgd_alloc_pool();
		while (mgd_fetch(res)) {
	/* Now we delete the blob file */
			path = mgd_format(mgd, pool, "$s/$s", mgd_get_blobdir(mgd),
							mgd_colvalue(res,1));
			if (unlink(path) == 0 || errno == ENOENT) {
	/* First delete any parameters linked to the blob */
#if HAVE_MIDGARD_QUOTA
			        mgd_touch_recorded_quota(mgd, "blobsdata", mgd_sql2int(res,2));
				mgd_touch_recorded_quota(mgd, "wholesg", mgd_sql2int(res,2));
#endif /* HAVE_MIDGARD_QUOTA */
				if(!mgd_delete_all_parameters ( mgd, mgd_sql2int(res, 0), "blobs")) 
					return 0;
				if(!mgd_delete(mgd, "blobs", mgd_sql2int(res, 0))) return 0;
				DELETE_REPLIGARD(mgd, "blobs", mgd_sql2int(res, 0));
			}
		}
		mgd_free_pool(pool);
		mgd_release(res);
	}
	return 1;
}

static int article_deleter(midgard * mgd, int id, int level, void *xparam)
{
	int *retcode=xparam;
	
	/* First we delete all the attachments linked to the article */
	*retcode &= mgd_delete_all_attachments(mgd, id, "article");
	
	/* Now we delete the parameters linked to the article */
	*retcode &= mgd_delete_all_parameters(mgd, id, "article");
	
#if HAVE_MIDGARD_MULTILANG
	/* Now we delete the content linked to the article */
	
	*retcode &= mgd_delete_all_content(mgd, id, "article");
	
#endif /* HAVE_MIDGARD_MULTILANG */
	/* And finally, delete the article */
	*retcode &= mgd_delete(mgd, "article", id);
	DELETE_REPLIGARD(mgd, "article", id);
	return 1;
}

static int topic_deleter(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res;
	int *retcode=xparam;
	
	/* First we delete all the attachments linked to the topic */
	*retcode &= mgd_delete_all_attachments(mgd, id, "topic");
	
	/* Now we delete the parameters linked to the topic */
	*retcode &= mgd_delete_all_parameters(mgd, id, "topic");
	
	res = mgd_sitegroup_select(mgd, "id", "article", "article.topic=$i",
				   "article.created", id);
	if (res) {
		while (mgd_fetch(res)) {
	/* First we delete all the attachments linked to the article */
			*retcode &= mgd_delete_all_attachments(mgd, mgd_sql2int(res, 0), "article");
	/* Now we delete the parameters linked to the article */
			*retcode &= mgd_delete_all_parameters(mgd, mgd_sql2int(res, 0), "article");
#if HAVE_MIDGARD_MULTILANG
			*retcode &= mgd_delete_all_content(mgd, mgd_sql2int(res, 0), "article");
#endif /* HAVE_MIDGARD_MULTILANG */
	/* Now we can safely delete the article */
			*retcode &= mgd_delete(mgd, "article", mgd_sql2int(res, 0));
			DELETE_REPLIGARD(mgd, "article", mgd_sql2int(res, 0));
		}
		mgd_release(res);
	}
	/* And finally, delete the topic */
	*retcode &= mgd_delete(mgd, "topic", id);
	DELETE_REPLIGARD(mgd, "topic", id);
	return 1;
}

int mgd_delete_element(midgard * mgd, int id) {
  int retcode=1;
  /* First we delete all the attachments linked to the element */
  retcode &= mgd_delete_all_attachments(mgd, id, "element");
  
  /* Now we delete the parameters linked to the element */
  retcode &= mgd_delete_all_parameters(mgd, id, "element");
#if HAVE_MIDGARD_MULTILANG
  retcode &= mgd_delete_all_content(mgd, id, "element");
#endif
  retcode &= mgd_delete(mgd, "element", id);
  DELETE_REPLIGARD(mgd, "element", id);

  return retcode;
}


int mgd_delete_page_element(midgard * mgd, int id) {
  int retcode=1;
  /* First we delete all the attachments linked to the pageelement */
  retcode &= mgd_delete_all_attachments(mgd, id, "pageelement");
  
  /* Now we delete the parameters linked to the pageelement */
  retcode &= mgd_delete_all_parameters(mgd, id, "pageelement");
#if HAVE_MIDGARD_MULTILANG
  retcode &= mgd_delete_all_content(mgd, id, "pageelement");
#endif
  retcode &= mgd_delete(mgd, "pageelement", id);
  DELETE_REPLIGARD(mgd, "pageelement", id);

  return retcode;
}

int mgd_delete_article(midgard * mgd, int root)
{
	int retcode=1;
	mgd_walk_table_tree(mgd, "article", "up", root, 0, 1, &retcode,
										article_deleter, NULL);
	return retcode;
}

int mgd_delete_topic(midgard * mgd, int root)
{
	int retcode=1;
	mgd_walk_table_tree(mgd, "topic", "up", root, 0, 1, &retcode,
										topic_deleter, NULL);
	return retcode;
}

static int page_deleter(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res;
	int *retcode=xparam;

#if HAVE_MIDGARD_PAGELINKS
	// DG: does repligard handle pagelinks ??
	// DELETE_REPLIGARD(mgd, "pagelink", mgd_exists_id(mgd, "up=$d", id));
	mgd_query(mgd, "DELETE FROM pagelink WHERE up=$d", id);
#endif
	/* First we delete all the attachments linked to the page */
	*retcode &= mgd_delete_all_attachments(mgd, id, "page");
	
	/* Now we delete the parameters linked to the page */
	*retcode &= mgd_delete_all_parameters(mgd, id, "page");
	/* Deleting pageelements now */
	res =
	   mgd_sitegroup_select(mgd, "id", "pageelement", "pageelement.page=$i",
				NULL, id);
	if (res) {
		while (mgd_fetch(res)) {
	/* First we delete all the attachments linked to the pageelement */
			*retcode &= mgd_delete_all_attachments(mgd, mgd_sql2int(res, 0), "pageelement");
	/* Now we delete the parameters linked to the pageelement */
			*retcode &= mgd_delete_all_parameters(mgd, mgd_sql2int(res, 0), "pageelement");
	/* Now we can safely delete the pageelement */
			*retcode &= mgd_delete(mgd, "pageelement", mgd_sql2int(res, 0));
			DELETE_REPLIGARD(mgd, "pageelement", mgd_sql2int(res, 0));
#if HAVE_MIDGARD_MULTILANG
			*retcode &= mgd_delete_all_content(mgd, mgd_sql2int(res, 0), "pageelement");
#endif /* HAVE_MIDGARD_MULTILANG */
		}
		mgd_release(res);
	}

#if HAVE_MIDGARD_MULTILANG
	/* delete content */
	*retcode &= mgd_delete_all_content(mgd, id, "page");
		  
#endif /* HAVE_MIDGARD_MULTILANG */
	/* And finally, delete the page */
	*retcode &= mgd_delete(mgd, "page", id);
	DELETE_REPLIGARD(mgd, "page", id);
	return 1;
}

int mgd_delete_page(midgard * mgd, int root)
{
	int retcode=1;
	mgd_walk_table_tree(mgd, "page", "up", root, 0, 1, &retcode,
										page_deleter, NULL);
	return retcode;
}

static int snippetdir_deleter(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res;
	int *retcode=xparam;

	/* First we delete all the attachments linked to the snippetdir */
	*retcode &= mgd_delete_all_attachments(mgd, id, "snippetdir");
	
	/* Now we delete the parameters linked to the snippetdir */
	*retcode &= mgd_delete_all_parameters(mgd, id, "snippetdir");
	/* Deleting snippets now */
	res = mgd_sitegroup_select(mgd, "id", "snippet", "snippet.up=$i",
				   "snippet.name", id);
	if (res) {
		while (mgd_fetch(res)) {
	/* First we delete all the attachments linked to the snippet */
			*retcode &= mgd_delete_all_attachments(mgd, mgd_sql2int(res, 0), "snippet");
	/* Now we delete the parameters linked to the snippet */
			*retcode &= mgd_delete_all_parameters(mgd, mgd_sql2int(res, 0), "snippet");
	/* Now we can safely delete the snippet */
			*retcode &= mgd_delete(mgd, "snippet", mgd_sql2int(res, 0));
			DELETE_REPLIGARD(mgd, "snippet",
					     mgd_sql2int(res, 0));
		}
		mgd_release(res);
	}
	/* And finally, delete the snippetdir */
	*retcode &= mgd_delete(mgd, "snippetdir", id);
	DELETE_REPLIGARD(mgd, "snippetdir", id);
	return 1;
}

int mgd_delete_snippetdir(midgard * mgd, int root)
{
	int retcode=1;
	mgd_walk_table_tree(mgd, "snippetdir", "up", root, 0, 1, &retcode,
			    							snippetdir_deleter, NULL);
	return retcode;
}

static int event_deleter(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res;
	int *retcode=xparam;

	/* First we delete all the attachments linked to the event */
	*retcode &= mgd_delete_all_attachments(mgd, id, "event");
	
	/* Now we delete the parameters linked to the event */
	*retcode &= mgd_delete_all_parameters(mgd, id, "event");
	/* Deleting eventmembers now */
	res = mgd_sitegroup_select(mgd, "id", "eventmember", "eventmember.eid=$i",
				   NULL, id);
	if (res) {
		while (mgd_fetch(res)) {
	/* First we delete all the attachments linked to the eventmember */
			*retcode &= mgd_delete_all_attachments(mgd, mgd_sql2int(res, 0), "eventmember");
	/* Now we delete the parameters linked to the eventmember */
			*retcode &= mgd_delete_all_parameters(mgd, mgd_sql2int(res, 0), "eventmember");
	/* Now we can safely delete the eventmember */
			*retcode &= mgd_delete(mgd, "eventmember", mgd_sql2int(res, 0));
			DELETE_REPLIGARD(mgd, "eventmember",
					     mgd_sql2int(res, 0));
		}
		mgd_release(res);
	}
	/* And finally, delete the event */
	*retcode &= mgd_delete(mgd, "event", id);
	DELETE_REPLIGARD(mgd, "event", id);
	return 1;
}

int mgd_delete_event(midgard * mgd, int root)
{
	int retcode=1;
	mgd_walk_table_tree(mgd, "event", "up", root, 0, 1, &retcode,
			    							event_deleter, NULL);
	return retcode;
}

static int style_deleter(midgard * mgd, int id, int level, void *xparam)
{
	midgard_res *res;
	int *retcode=xparam;

	/* First we delete all the attachments linked to the style */
	*retcode &= mgd_delete_all_attachments(mgd, id, "style");
	
	/* Now we delete the parameters linked to the style */
	*retcode &= mgd_delete_all_parameters(mgd, id, "style");
	/* Deleting elements now */
	res = mgd_sitegroup_select(mgd, "id", "element", "element.style=$i",
				   "element.id", id);
	if (res) {
		while (mgd_fetch(res)) {
	/* First we delete all the attachments linked to the element */
			*retcode &= mgd_delete_all_attachments(mgd, mgd_sql2int(res, 0), "element");
	/* Now we delete the parameters linked to the element */
			*retcode &= mgd_delete_all_parameters(mgd, mgd_sql2int(res, 0), "element");
	/* Now we can safely delete the element */
			*retcode &= mgd_delete(mgd, "element", mgd_sql2int(res, 0));
			DELETE_REPLIGARD(mgd, "element", mgd_sql2int(res, 0));
		}
		mgd_release(res);
	}
	/* And finally, delete the style */
	*retcode &= mgd_delete(mgd, "style", id);
	DELETE_REPLIGARD(mgd, "style", id);
	return 1;
}

int mgd_delete_style(midgard * mgd, int root)
{
	int retcode=1;
	mgd_walk_table_tree(mgd, "style", "up", root, 0, 1, &retcode,
										style_deleter, NULL);
	return retcode;
}

int mgd_move_object(midgard * mgd, const char *table, const char *upfield,
		    int id, int newup)
{
	return mgd_update(mgd, table, id, "$s=$i", upfield, newup);
}

/* mgd_parse_path
    Scans records in a tables 'table' and 'uptable' and returns IDs of
    an object queried in path and its uplink.
    upfield 	-	name of field in 'table' points to uplink
    namefiled	-	name of field in 'table' contains 'name' of an object
			('article' contains two names - 'name' and 'title',
			and we can search by both)
    If id == NULL then will assume that only 'uptable' table should be scanned
    and path contains only info about structure ('directories'). 
    E.g. if path = '/topic1/topic2/topic3/article'
    returns id = ID of 'article' under 'topic3' and
	    up = ID of 'topic3'
    if path = '/topic1/topic2/topic3' and id = NULL
    return up = ID of 'topic3'
    
    0 if query was successful, 1 in other case
    
    Could be used for quering any tables that have 'id', 'up', and 'name' fields.
*/
char *mgd_strtok(char *str, char c)
{
	static char *ptr=NULL;
	char *p;
	if(str) ptr=str;
	if(*ptr==c) ptr++; 
	p=ptr;
	if(!(ptr=strchr(p,c))) return NULL;
	*ptr='\0'; ptr++;
	return p;
}

int mgd_parse_path(midgard * mgd, const char *path, const char *uptable,
		   const char *table, const char *upfield,
		   const char *namefield, int *id, int *up)
{
	char *tok, *filename, *part;
	midgard_res *res;
	midgard_pool *pool;
	int upval;
	part = NULL;
	pool = mgd_alloc_pool();
	if (id) {
		part = strrchr(path, '/');
		if (part) {
			part++;
			filename = mgd_strndup(pool, path, part - path);
		}
		else {
			/* '/' not found - path is not absolute */
			if (pool)
				mgd_free_pool(pool);
			return 1;
		}
	}
	else {
		filename = mgd_strdup(pool, path);
	}
	upval = 0;

	/* Traverse the path structure */
	for (tok = mgd_strtok(filename, '/'); tok; tok = mgd_strtok(NULL, '/')) {
		/* Find the correct uplink */
		res = mgd_sitegroup_select
		   (mgd, "id", uptable, "up=$d AND name=$q", NULL, upval, tok);
		if (!res || !mgd_fetch(res)) {
			if (res)
				mgd_release(res);
			upval = -1;
			break;
		}
		upval = atoi(mgd_colvalue(res, 0));
		mgd_release(res);
	}

	if (pool)
		mgd_free_pool(pool);

	if (upval == -1) {
		/* some path component not found */	
		return 1;
	}

	(*up) = upval;

	if (id) {

		(*id) =
		   mgd_exists_id(mgd, table, "$s=$d AND $s=$q", upfield, upval,
			      namefield, part);
	}

	return 0;
}

int mgd_parse_path_with_hook(midgard * mgd, const char *path,
			     const char *uptable, const char *table,
			     const char *upfield, const char *namefield,
			     int *id, int *up, midgard_pathfunc hook)
{
	char *tok, *filename, *part;
	midgard_res *res;
	midgard_pool *pool;
	int upval, idval;
	part = NULL;
	pool = mgd_alloc_pool();
	if (id) {
		part = strrchr(path, '/');
		if (part) {
			part++;
			filename = mgd_strndup(pool, path, part - path);
		}
		else {
			/* '/' not found - path is not absolute */
			if (pool)
				mgd_free_pool(pool);
			return 1;
		}
	}
	else {
		filename = mgd_strdup(pool, path);
	}

	upval = 0;

	/* Traverse the path structure */
	for (tok = mgd_strtok(filename, '/'); tok; tok = mgd_strtok(NULL, '/')) {
		/* Find the correct uplink */
		res = mgd_sitegroup_select
		   (mgd, "id", uptable, "up=$d AND name=$q", NULL, upval, tok);
		if (!res || !mgd_fetch(res)) {
			if (res)
				mgd_release(res);
			idval =
			   hook(mgd, uptable, tok, upval, 0,
				MIDGARD_PATH_ELEMENT_NOTEXISTS);
			if (idval)
				goto query;
			upval = -1;
			break;
		}
		idval = atoi(mgd_colvalue(res, 0));
		mgd_release(res);
		hook(mgd, uptable, tok, upval, idval,
		     MIDGARD_PATH_ELEMENT_EXISTS);
	      query:
		upval = idval;
	}

	if (pool)
		mgd_free_pool(pool);

	if (upval == -1) {
		/* some path component not found */
		return 1;
	}

	(*up) = upval;

	if (id) {

		(*id) =
		   mgd_exists_id(mgd, table, "$s=$d AND $s=$q", upfield, upval,
			      namefield, part);
		if ((*id) == 0) {
			(*id) = hook(mgd, table, part, upval, 0, MIDGARD_PATH_OBJECT_NOTEXISTS);
		}
		else {
			hook(mgd, table, part, upval, 0,
			     MIDGARD_PATH_OBJECT_EXISTS);
		}
	}

	return 0;
}

int mgd_has_dependants(midgard * mgd, int id, char * table)
{
    if(mgd_exists_id(mgd,"record_extension","oid=$d AND tablename=$q", id, table))
	    return 1;
    if(mgd_exists_id(mgd,"blobs","pid=$d AND ptable=$q", id, table))
	    return 1;
    return 0;
}

