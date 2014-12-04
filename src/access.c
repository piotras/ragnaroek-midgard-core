/* $Id: access.c 9738 2006-05-30 14:20:09Z piotras $
 *
 * Copyright (C) 1999 Jukka Zitting <jukka.zitting@iki.fi>
 * Copyright (C) 2000 The Midgard Project ry
 * Copyright (C) 2000 Emiliano Heyns, Aurora SA
 * Copyright (C) 2003 David Schmitter, Dataflow Solutions GmbH <schmitt@dataflow.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
#include "midgard/midgard_legacy.h"
#include "midgard/tablenames.h"

int mgd_istopicreader(midgard *mgd, int topic)
{
	int reader;

	if (mgd_isadmin(mgd))
		return 1;

	while (topic) {
		reader = mgd_idfield(mgd, "reader", "topic", topic);
		if (reader)
			return mgd_ismember(mgd, reader);
		topic = mgd_idfield(mgd, "up", "topic", topic);
	}

	return 1;
}

int mgd_isarticlereader(midgard *mgd, int article)
{
	return
	   mgd_istopicreader(mgd, mgd_idfield
			 (mgd, "topic", "article", article));
}

int mgd_istopicowner(midgard *mgd, int topic)
{
	if (mgd_isadmin(mgd))
		return 1;

	for (; topic; topic = mgd_idfield(mgd, "up", "topic", topic))
		if (mgd_ismember(mgd,
				 mgd_idfield(mgd, "owner", "topic",
					     topic))) return 1;

	return 0;
}

int mgd_issnippetdirowner(midgard *mgd, int snippetdir)
{
	if (mgd_isadmin(mgd))
		return 1;

	for (; snippetdir;
	     snippetdir =
	     mgd_idfield(mgd, "up", "snippetdir",
			 snippetdir)) if (mgd_ismember(mgd,
						       mgd_idfield(mgd, "owner",
								   "snippetdir",
								   snippetdir)))
			   return 1;

	return 0;
}

/* Allocate and initialize a Midgard tree node */
#define GRP_ALLOC_TNODE(pool, node) {\
  (node) = (grp_tree_node*)mgd_alloc(pool, sizeof(grp_tree_node));\
  *(node) = grp_zero_node;\
}
const grp_tree_node grp_zero_node = { 0, NULL, };

int mgd_isgroupowner(midgard *mgd, int gid)
{
midgard_pool *pool;
grp_tree_node *node, *root_node, *tmp;
int nodes;
pool = mgd_alloc_pool();
GRP_ALLOC_TNODE(pool, root_node);
nodes=0;

	if (mgd_isadmin(mgd))
		return 1;

   for (; gid; gid = mgd_idfield(mgd, "owner", "grp", gid)) {
      if (mgd_ismember(mgd, gid)) {
         return 1;
      } else {    
         /* look for it in the grp tree node array */
         tmp = root_node;
         while(tmp->child != NULL) {
            if (tmp->id == gid) {
               /* found it already! */
               mgd_free_pool(pool);
               return 0;
            }

            tmp = tmp->child;
         }

         /* ADD A NODE! */
         GRP_ALLOC_TNODE(pool, node);       
         tmp->id = gid;
         tmp->child = node;
      }      
   }

	return 0;
}

int mgd_isgroupreader(midgard *mgd, int gid)
{
	return mgd_isadmin(mgd)
	   || mgd_ismember(mgd,
			   mgd_idfield(mgd, "owner", "grp", gid))
	   || mgd_ismember(mgd,
			   mgd_idfield(mgd, "reader", "grp", gid));
}

int mgd_isuserowner(midgard *mgd, int uid)
{
	if (mgd_isadmin(mgd)) return 1;
   
   if (mgd_isuser(mgd, uid)) return 1;

   /* is user owner if owner of one of the groups 'uid' is member from
    */
	if (mgd_exists_bool(mgd, "member,grp",
			 "uid=$d AND gid=grp.id AND grp.owner IN $D"
			 " AND grp.sitegroup IN (0,$d)"
			 , uid, mgd_groups(mgd)
			,mgd->current_user->sitegroup
		)) return 1;

   /* is user owner if creator of 'uid' and 'uid' has no username
    */
   if (mgd_exists_bool(mgd, "person",
            "id=$d AND creator=$d AND username=''"
            " AND sitegroup IN (0,$d)"
            , uid, mgd->current_user->id
            ,mgd->current_user->sitegroup
      )) return 1;

   return 0;
}

#if HAVE_MIDGARD_PAGELINKS
/* pagelink */
int mgd_ispagelinkowner(midgard *mgd, int pagelink)
{
	if (mgd_isadmin(mgd))
		return 1;
	if (mgd_exists_id(mgd, "pagelink", "id=$d AND owner IN $D",
		       pagelink, mgd_groups(mgd)))
		return 1;

	return
	   mgd_ispageowner(mgd, mgd_idfield(mgd, "up", "pagelink", pagelink));
}
#endif

int mgd_isarticleowner(midgard *mgd, int article)
{
	int topic, locker, author;

	if (mgd_isadmin(mgd))
		return 1;

	locker = mgd_idfield(mgd, "locker", "article", article);
	if (locker)
		return mgd_isuser(mgd, locker);

	author = mgd_idfield(mgd, "author", "article", article);
	if (mgd_isuser(mgd, author))
		return 1;

	topic = mgd_idfield(mgd, "topic", "article", article);
	while (topic) {
		if (mgd_ismember(mgd,
				 mgd_idfield(mgd, "owner", "topic",
					     topic))) return 1;
		topic = mgd_idfield(mgd, "up", "topic", topic);
	}

	return 0;
}

int mgd_iseventowner(midgard *mgd, int event)
{
	int parent, locker, creator;

	if (mgd_isadmin(mgd))
		return 1;

	locker = mgd_idfield(mgd, "locker", "event", event);
	if (locker)
		return mgd_isuser(mgd, locker);

	creator = mgd_idfield(mgd, "creator", "event", event);
	if (mgd_isuser(mgd, creator))
		return 1;

	parent = event;
	while (parent) {
		if (mgd_ismember(mgd, mgd_idfield(mgd, "owner", "event", parent)))
			return 1;
		parent = mgd_idfield(mgd, "up", "event", parent);
	}

	return 0;
}

int mgd_ishostowner(midgard *mgd, int host)
{
	return mgd_isadmin(mgd)
	   || mgd_ismember(mgd,
			   mgd_idfield(mgd, "owner", "host", host));
}

int mgd_ispageowner(midgard *mgd, int page)
{
	if (mgd_isadmin(mgd))
		return 1;

   if (mgd_exists_id(mgd, "page", "id=$d AND author=$d", page, mgd_user(mgd)))
      return 1;

	while (page) {
#if HAVE_MIDGARD_PAGE_OWNER
	  if (mgd_exists_id(mgd, "page", "id=$d AND owner != 0", page)) {
	    if (mgd_exists_id(mgd, "page", "id=$d AND owner IN $D", page, mgd_groups(mgd)))
	      return 1;
	    else 
	      return 0;
	  }
#endif /* HAVE_MIDGARD_PAGE_OWNER */
		if (mgd_exists_id(mgd, "host", "root=$d AND owner IN $D",
			       page, mgd_groups(mgd)))
			return 1;
		page = mgd_idfield(mgd, "up", "page", page);
	}
	return 0;
}

#if HAVE_MIDGARD_MULTILANG
int mgd_ispagecontentowner(midgard *mgd, int page, int lang) {
  int my_page = page;
  if (mgd_isadmin(mgd))
    return 1;
  if (mgd_exists_id(mgd, "page_i", "sid=$d AND lang=$d AND author=$d", page, lang, mgd_user(mgd)))
    return 1;
  
  
  while (page) {
    if (mgd_exists_id(mgd, "page_i", "sid=$d AND lang=$d AND owner != 0", page, lang)) {
      if (mgd_exists_id(mgd, "page_i", "sid=$d AND lang=$d AND owner IN $D", page, lang, mgd_groups(mgd)))
	return 1;
      else 
	return 0;
    }
    page = mgd_idfield(mgd, "up", "page", page);
  }
  
  page = my_page;

  while (page) {
    if (mgd_exists_id(mgd, "page", "id=$d AND owner != 0", page)) {
      if (mgd_exists_id(mgd, "page", "id=$d AND owner IN $D", page, mgd_groups(mgd)))
	return 1;
      else 
	return 0;
    }

    if (mgd_exists_id(mgd, "host", "root=$d AND owner IN $D",
		      page, mgd_groups(mgd)))
      return 1;
    page = mgd_idfield(mgd, "up", "page", page);
  }

  return 0;
  
}


#endif /* HAVE_MIDGARD_MULTILANG */
int mgd_isstyleowner(midgard *mgd, int style)
{
   if (mgd_isadmin(mgd)) return 1;

   if (mgd_exists_id(mgd, "style", "id=$d AND owner IN $D", style, mgd_groups(mgd))) return 1;

   style = mgd_idfield(mgd, "up", "style", style);
   while (style != 0) {
      if (mgd_exists_id(mgd, "style", "id=$d AND owner IN $D", style, mgd_groups(mgd))) return 1;
   }

   return 0;
}

int mgd_lookup_table_id(const char *table)
{
	int s = 1;
	int e = MIDGARD_OBJECT_COUNT;
	int c;
	int i;

	while (s <= e) {
		i = s + ((e - s) / 2);
		c = strcmp(table, mgd_table_name[i]);

		if (c == 0)
			return i;
		if (c < 0)
			e = i - 1;
		else
			s = i + 1;
	}

	return 0;
}

#if ! HAVE_MIDGARD_MULTILANG
int mgd_global_is_owner_ex(midgard *mgd, int table, int id, int dbg)
#else /* HAVE_MIDGARD_MULTILANG */
int mgd_global_is_owner(midgard *mgd, int table, int id) {
  return mgd_global_is_owner_ex(mgd, table, id, 0);
}


int mgd_global_is_owner_ex(midgard *mgd, int table, int id, int dbg) { 
  return mgd_global_is_owner_ex_lang(mgd, table, id, dbg, 0, 0);
}

int mgd_global_is_owner_lang(midgard *mgd, int table, int id, int content, int lang) { 
  return mgd_global_is_owner_ex_lang(mgd, table, id, 0, content, lang);
}

int mgd_global_is_owner_ex_lang(midgard *mgd, int table, int id, int dbg, int content, int lang)
#endif /* HAVE_MIDGARD_MULTILANG */
{
	midgard_res *res;
	int owner;
   int switched = 1;

	if (mgd_isadmin(mgd))
		return 1;

	switch (table) {
		case MIDGARD_OBJECT_PAGEELEMENT:
			table = MIDGARD_OBJECT_PAGE;
			id = mgd_idfield(mgd, "page", "pageelement", id);
			break;

		case MIDGARD_OBJECT_ELEMENT:
			table = MIDGARD_OBJECT_STYLE;
			id = mgd_idfield(mgd, "style", "element", id);
			break;

		case MIDGARD_OBJECT_MEMBER:
			/* EEH: is this right or should we use info == 'owner'? */
			table = MIDGARD_OBJECT_GRP;
			id = mgd_idfield(mgd, "gid", "member", id);
			break;

		case MIDGARD_OBJECT_EVENTMEMBER:
			table = MIDGARD_OBJECT_EVENT;
			id =
			   mgd_idfield(mgd, "eid", "eventmember", id);
			break;

		case MIDGARD_OBJECT_PREFERENCE:
			table = MIDGARD_OBJECT_PERSON;
			id =
			   mgd_idfield(mgd,  "uid","preference", id);
			break;

		case MIDGARD_OBJECT_FILE:
			table = MIDGARD_OBJECT_ARTICLE;
			id = mgd_idfield(mgd, "article", "file", id);
			break;

		case MIDGARD_OBJECT_SNIPPET:
			table = MIDGARD_OBJECT_SNIPPETDIR;
			id = mgd_idfield(mgd, "up", "snippet", id);
			break;

      default:
         switched = 0;
         break;
	}

   if (dbg && switched) {
      fprintf(stderr, "Dependant resource, checking %s %d instead\n",
         mgd_table_name[table], id);
   }

	switch (table) {
		case MIDGARD_OBJECT_SITEGROUP:
			return mgd_isroot(mgd);

		case MIDGARD_OBJECT_IMAGE:
			return 1;	/* EEH: no ownership seems to exist */
		case MIDGARD_OBJECT_HOST:
			return mgd_ishostowner(mgd, id);

		case MIDGARD_OBJECT_GRP:
			return mgd_isgroupowner(mgd, id);

		case MIDGARD_OBJECT_PERSON:
			return mgd_isuserowner(mgd, id);

		case MIDGARD_OBJECT_TOPIC:
			return mgd_istopicowner(mgd, id);

		case MIDGARD_OBJECT_ARTICLE:
			return mgd_isarticleowner(mgd, id);

		case MIDGARD_OBJECT_EVENT:
			return mgd_iseventowner(mgd, id);

		case MIDGARD_OBJECT_PAGE:
#if HAVE_MIDGARD_MULTILANG
		  if (content == 1) { 
		    return mgd_ispageowner(mgd, id) && mgd_ispagecontentowner(mgd, id, lang);
		  } else if (content == 2) {
		    return mgd_ispagecontentowner(mgd, id, lang);
		  } else {
#endif /* HAVE_MIDGARD_MULTILANG */
			return mgd_ispageowner(mgd, id);
#if HAVE_MIDGARD_MULTILANG
		  }
#endif /* HAVE_MIDGARD_MULTILANG */

#if HAVE_MIDGARD_PAGELINKS
		case MIDGARD_OBJECT_PAGELINK:
			return mgd_ispagelinkowner(mgd, id);
#endif

		case MIDGARD_OBJECT_STYLE:
			return mgd_isstyleowner(mgd, id);

		case MIDGARD_OBJECT_SNIPPETDIR:
			return mgd_issnippetdirowner(mgd, id);

		case MIDGARD_OBJECT_RECORD_EXTENSION:
		case MIDGARD_OBJECT_BLOBS:
		  if (table == MIDGARD_OBJECT_BLOBS) {
#if HAVE_MIDGARD_MULTILANG
		    res =
		      mgd_sitegroup_record(mgd,
					   "ptable,pid,lang", "blobs",
							id);
#else
				res =
				   mgd_sitegroup_record(mgd,
							"ptable,pid", "blobs",
							id);
#endif
		  } else {
#if HAVE_MIDGARD_MULTILANG
		    res =
		      mgd_sitegroup_record(mgd,
					   "tablename,oid,lang",
					   "record_extension", id);
#else
				res =
				   mgd_sitegroup_record(mgd,
							"tablename,oid",
							"record_extension", id);
#endif
		  }
			if (!res || !mgd_fetch(res)) {
				if (res)
					mgd_release(res);
				return 0;
			}

			table = mgd_lookup_table_id(mgd_colvalue(res, 0));
			id = mgd_sql2id(res, 1);
#if HAVE_MIDGARD_MULTILANG
			lang = mgd_sql2id(res, 2);
			owner = mgd_global_is_owner_lang(mgd, table, id, 2, lang);
#else 
			owner = mgd_global_is_owner(mgd, table, id);
#endif
			mgd_release(res);
			return owner;
	}

	return 0;
}


