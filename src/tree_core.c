/* $Id: tree_core.c 19219 2008-12-03 15:36:50Z piotras $
 *
 * midgard-lib: database access for Midgard clients
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

/*! \file lib/src/tree_core.c
 *  \brief Tree core library
 *
 *  The functions of this file are used to manipulate the tree objects
 */

#include <config.h>
//#include "midgard/types.h"
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

#include "midgard/midgard_object.h"
#include "schema.h"

//! Initializer for a Midgard tree node
const tree_node zero_node = {
	0,
	0,
	0,
	0,
	NULL,
	NULL
}; 

//! Allocate and initialize a Midgard tree node
#define ALLOC_TNODE(pool, node) {\
	(node) = (tree_node*)mgd_alloc(pool, sizeof(tree_node));\
	*(node) = zero_node;\
}

/*! \brief Traverse the tree and execute a user defined function
 *         (please see mgd_walk_table_tree())
 *  \param mgd			Midgard handle
 *  \param tree			The tree to traverse
 *  \param order		Specify when the user function will be executed
 *  \param level		The current recursion level
 *  \param maxlevel		The maximum recursion level
 *  \param xparam		The user defined variable passed to the user
 *  					defined function
 *  \param func			The user defined function
 *  
 *  \return				non-0 if ok to continue, 0 to stop
 *
 *  walktree() may not be used outside of the lib.
 *
 */
static int walktree(midgard * mgd, tree_node * tree, int order, int level,
		    int maxlevel, void *xparam, midgard_userfunc func)
{
	int branch_ok = 1;
	tree_node *tmp;
	if(maxlevel && level > maxlevel) return 1;
	if (order && func)
		if(!(branch_ok = func(mgd, tree->id, level, xparam)))
			return 0;
	if(branch_ok > 0) {
		tmp = tree->child;
		while(tmp) {
			if(!walktree(mgd, tmp, order, level + 1, maxlevel, xparam, func))
				return 0;
			tmp = tmp->next;
		}
	}
	if ((!order) && func)
		if(!func(mgd, tree->id, level, xparam))
			return 0;
	return 1;
}

/*! \brief Get all the children of a list of nodes
 *  \param mgd			Midgard handle
 *  \param pool			A memory pool used during the process
 *  \param table		Identify the type of the node
 *  \param upfield		A string containing the name of the up field
 *  \param sort			The sort fields, separated by a comma
 *  \param ids			The ids of the father nodes
 *  \retval nodes		The children nodes
 *
 *  \return				The number of children
 *  
 *  mgd_tree_get_level() may not be used outside of the lib.
 *
 *  How this function work:<br>
 *  It gets the children and link them together according to their upfield,
 *  making them siblings.
 *  
 */
int mgd_tree_get_level(midgard *mgd, midgard_pool * pool, const char *table,
						const char * upfield, const char * sort,
						int ** ids, tree_node ** nodes)
{
	midgard_res *res;
	int i,n;

	if(!ids) return 0;
	if(!upfield || !(*upfield)) upfield = "up";
	if((*ids)[0] && (*ids)[1]) {
		res = mgd_query(mgd,
			"SELECT id,$s FROM $s WHERE $s IN $D AND ((sitegroup in (0, $d) OR $d<>0) AND $s.metadata_deleted=0) ORDER BY $s,$s",
			upfield, table,
			upfield, *ids, mgd_sitegroup(mgd), mgd_isroot(mgd),
			table,
			upfield, sort ? sort : "id");
	} else { /* root query (single id) */
		res = mgd_query(mgd,
			"SELECT id,$s FROM $s WHERE $s=$d AND ((sitegroup in (0, $d) OR $d<>0) AND $s.metadata_deleted=0) ORDER BY $s,$s",
			upfield, table,
			upfield, (*ids)[0], mgd_sitegroup(mgd), mgd_isroot(mgd),
			table,
			upfield, sort ? sort : "id");
	}
	if(!res || !(n = mgd_rows(res))) {
		nodes = NULL;
		ids = NULL;
		return 0;
	}
	*nodes = (tree_node *) mgd_alloc(pool, n * sizeof (tree_node));
	*ids = (int *) mgd_alloc(pool, (n + 1) * sizeof(int *));
	for(i = 0; mgd_fetch(res); i++) {
		(*ids)[i] = mgd_sql2int(res, 0);
		(*nodes)[i].id = (*ids)[i];
		(*nodes)[i].up = mgd_sql2int(res, 1);
		(*nodes)[i].level = 0;
		(*nodes)[i].size = 0;
		(*nodes)[i].child = NULL;
		(*nodes)[i].next = NULL;
		// if the current node and the previous node are from the same parent,
		// they are siblings so we update the next of the previous node
		if(i && (*nodes)[i].up == (*nodes)[i-1].up) {
			(*nodes)[i-1].next = &((*nodes)[i]);
		}
	}
	(*ids)[i] = 0;
	mgd_release(res);
	return n;
}

/*! \brief Extract all ids from a tree to build a flat tree ids list.
 *  \param tree			A subtree to process
 *  \param ids			The ids list
 *  \param index		An index on the tree list \a ids
 *  
 *  mgd_tree_get_ids() may not be used outside of the lib.
 *  \warning \a ids *must* be pre-allocated, for this, first call
 *  mgd_tree_init() on the tree, mgd_alloc(pool, tree->size + 1) and then you
 *  can call mgd_tree_get_ids().
 *  You have to set \a *index to 0 before calling this function.
 *  
 */
void mgd_tree_get_ids(tree_node * tree, int *ids, int *index) {
	tree_node * tmp;

	if(!tree) return;
	ids[*index] = tree->id;
	(*index)++;
	tmp = tree->child;
	while(tmp) {
		mgd_tree_get_ids(tmp, ids, index);
		tmp = tmp->next;
	}
}

/*! \brief Initialize internal parameters of the tree.
 *  \param tree			A subtree to process
 *  \param level		The recursion level
 *  
 *  mgd_tree_init() may not be used outside of the lib.
 *  
 */
int mgd_tree_init(tree_node * tree, int level) {
	tree_node * tmp;

	if(!tree) return 0;
	tree->size = 1;
	tree->level = level;
	tmp = tree->child;
	while(tmp) {
		tree->size += mgd_tree_init(tmp, level + 1);
		tmp = tmp->next;
	}
	
	return tree->size; /* store the size of the subtree + the current node */
}

/*! \brief Build a tree in memory
 *  \param mgd			Midgard handle
 *  \param pool			A memory pool used during the process
 *  \param table		Identify the type of the node
 *  \param upfield		A string containing the name of the up field
 *  \param root			The node id root of the subtree
 *  \param maxlevel		The maximum recursion depth
 *  \param sort			The sort fields, separated by a comma
 *  
 *  mgd_tree_build() may not be used outside of the lib.
 *  
 */
tree_node * mgd_tree_build(midgard * mgd, midgard_pool * pool,
				const char * table, const char * upfield, int root,
				int maxlevel, const char * sort)
{
	int root_ids[2] = {root, 0};
	int *child_ids = root_ids;
	tree_node *root_node, *parent_nodes, *child_nodes;
	int parent_num, child_num, i, j, k;

	if(sort && (!strncmp(sort, "id", 2) || !strncmp(sort, "up", 2)
						|| !strncmp(sort, "sitegroup", 9))) // who knows ?? ;)
		sort = NULL;
	ALLOC_TNODE(pool, root_node);
	root_node->id = root;
	parent_num = 1;
	parent_nodes = root_node;
	for(i = 0; !maxlevel || (i < maxlevel); i++) {
		child_num = mgd_tree_get_level(mgd, pool, table, upfield, sort,
									&child_ids, &child_nodes);
		if(!child_num) break;
		// if more parents than children, loop thru parents once
		if(parent_num > child_num) {
			for(j = 0; j < parent_num; j++) {
				for(k = 0; k < child_num; k++) {
					// zap child until we find the next one with parent
					if(parent_nodes[j].id != child_nodes[k].up) continue;
					parent_nodes[j].child = child_nodes + k;
					break;
				}
			}
		} else { // more children than parents, loop thru children once
			for(k = 0; k < child_num; k++) {
				for(j = 0; j < parent_num; j++) {
					// zap parent until we find the next one with children
					if(parent_nodes[j].id != child_nodes[k].up) continue;
					parent_nodes[j].child = child_nodes + k;
					// found it, zap siblings
					while(child_nodes[k].next) k++;
				}
			}
		}
		parent_nodes = child_nodes;
		parent_num = child_num;
	}
	mgd_tree_init(root_node, 0);
	return root_node;
}

/*! \brief Execute a function for each node of the tree.
 *  \param mgd			Midgard handle
 *  \param table		Identify the type of the node
 *  \param upfield		A string containing the name of the up field
 *  \param root			The node id root of the subtree
 *  \param maxlevel		The maximum recursion depth
 *  \param order		Specify when the function is executed for each node
 *  					during the walk: 0-before, 1-after
 *  \param xparam		A user defined variable passed to the user function
 *  \param func			A user defined function called for each node
 *  \param sort			The sort fields, separated by a comma
 *  
 *  \return				non-0 if the tree walk has been completed, 0 otherwise
 *  mgd_walk_table_tree() is used to execute a function for
 *  each node of the arborescence.
 *  
 */
MGD_API int mgd_walk_table_tree(midgard * mgd, const char *table, const char *upfield,
			int root, int maxlevel, int order, void *xparam,
			 midgard_userfunc func, const char * sort)
{
	midgard_pool *pool;
	tree_node *tree;
	int ret;

	pool = mgd_alloc_pool();

	tree = mgd_tree_build(mgd, pool, table, upfield, root, maxlevel, sort);

	if (!tree) {
		return 0;
	}
	/* Be aware of top level trees */
	ret = walktree(mgd, tree, order, 0, maxlevel, xparam, func);
	mgd_free_pool(pool);
	return ret;
}

/*! \brief Get a flat tree ids list.
 *  \param mgd			Midgard handle
 *  \param table		Identify the type of the node
 *  \param upfield		A string containing the name of the up field
 *  \param id			The node id root of the subtree
 *  \param maxlevel		The maximum recursion depth
 *  \param sort			The sort fields, separated by a comma
 *  
 *  mgd_tree() flattens the tree and returns a list of ids corresponding to
 *  each node of the arborescence.
 *  
 */
MGD_API int *mgd_tree(midgard * mgd, const char *table, const char *upfield,
				int id, int maxlevel, const char *sort)
{
	int *ids, i = 0;
	tree_node * tree;
	midgard_pool * pool;

	pool = mgd_alloc_pool();
	tree = mgd_tree_build(mgd, pool, table, upfield, id, maxlevel, sort);
	if(!tree) return NULL;

	ids = (int *)malloc((tree->size + 1) * sizeof(int));
	
	mgd_tree_get_ids(tree, ids, &i);
	ids[i] = 0;

	mgd_free_pool(pool);
	return ids;
}

/*! \brief Check if an node is descendant of another node.
 *  \param mgd		Midgard handle
 *  \param table	Identify the type of object
 *  \param upfield	A string containing the name of the up field
 *  \param root		The root node id
 *  \param id		The node id we want to check
 *  
 *  This function may be used to check the ascendance of a node in a tree.
 *  
 */
MGD_API int mgd_is_in_tree(midgard * mgd, const char * table,
				const char * upfield, int root, int id)
{
	midgard_res * res;
	midgard_pool * pool;
	int level=0;

	pool = mgd_alloc_pool();

	if(!table) return 0;
	if(!upfield || !(*upfield)) upfield = "up";
	while(root != id) {
		res = mgd_query(mgd,
			"SELECT $s FROM $s WHERE id=$d AND (sitegroup in (0, $d) OR $d<>0)",
			upfield, table, id, mgd_sitegroup(mgd), mgd_isroot(mgd));
		if(!res || !mgd_rows(res)) {
			mgd_free_pool(pool);
			if(res)
				mgd_release(res);
			return 0;
		}
		mgd_fetch(res);
		id = mgd_sql2int(res, 0);
		level++;
		mgd_release(res);
	}
	mgd_free_pool(pool);
	if(root == id) return level+1;
	return 0;
}


MGD_API gchar *midgard_object_build_path(MgdObject *mobj)
{
  g_assert(mobj != NULL);	
  midgard_res * res;
  midgard_pool * pool;
  gchar  *path = NULL, *pename;
  gint  id;
  const gchar *parent_table, *upfield;
    
  pool = mgd_alloc_pool();
  if (!pool)
    return NULL;

  MgdObject *parent = midgard_object_new(mobj->mgd, 
		  midgard_object_parent(mobj), NULL);

  MidgardObjectClass *parent_class = MIDGARD_OBJECT_GET_CLASS(parent);
  MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(mobj);
    
  parent_table = midgard_object_class_get_table(parent_class);
  upfield = parent_class->storage_data->query->upfield;

  g_object_unref(parent);

  if ((parent_table == NULL) || (upfield == NULL))
    return NULL;

  g_log("midgard-lib", G_LOG_LEVEL_DEBUG, "Walk the tree using table '%s' and  upfield: '%s'", parent_table, upfield);  
  
  g_object_get((GObject *) mobj, klass->storage_data->query->upfield, &id, NULL);
  
  do {
    res = mgd_query(mobj->mgd,
        "SELECT $s,name FROM $s WHERE id=$d AND (sitegroup in (0, $d) OR $d<>0)",
        upfield, parent_table, id, mgd_sitegroup(mobj->mgd), mgd_isroot(mobj->mgd));
    
    if(!res || !mgd_rows(res)) {
      mgd_free_pool(pool);
      return NULL;
    } 

    mgd_fetch(res);
    id = mgd_sql2int(res, 0);
    pename = (gchar *)mgd_colvalue(res, 1);
    pename = g_strjoin("/", "", pename, NULL);
    path = g_strjoin("", pename, path, NULL);
    mgd_release(res);
  } while (id != 0);

  mgd_free_pool(pool);
  g_log("midgard-lib", G_LOG_LEVEL_DEBUG, "PATH %s", path);
  return path;
}

/* Walk parent's type tree to check if our type is in that tree */

gboolean midgard_object_is_in_parent_tree(MgdObject *mobj, guint rootid, guint id)
{
  g_assert(mobj != NULL);	
  
  gint ri;
  const gchar *parent_table, *parent_up;
  
  MgdObject *parent = midgard_object_new(mobj->mgd,
		  midgard_object_parent(mobj), NULL);
  
  MidgardObjectClass *parent_class = MIDGARD_OBJECT_GET_CLASS(parent);
  
  parent_table = midgard_object_class_get_table(parent_class);
  parent_up = parent_class->storage_data->query->upfield;
  
  g_object_unref(parent);
	
  if (!parent_table)
    return FALSE;

  if (!parent_up)
    return FALSE;

  /* TODO , replace with repligard's table check */
  if(!mgd_exists_id(mobj->mgd,  parent_table, "id=$d", id))
    return FALSE;
    
  if((ri = mgd_is_in_tree(mobj->mgd, parent_table,
          parent_up,
          rootid, id)))
    return TRUE;

  return FALSE;
}


/* Walk tree of the same type to check if type is in this tree */
  
gboolean midgard_object_is_in_tree(MgdObject *mobj, guint rootid, guint id)
{
  g_assert(mobj != NULL);
  
  gint ri;
  const gchar *table, *upfield;  
  MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(mobj);
  
  table = midgard_object_class_get_table(klass);
  upfield = klass->storage_data->query->upfield;
  
  if (!table)
    return FALSE;

  if (!upfield)
    return FALSE;

  /* TODO , replace with repligard's table check */
  if(!mgd_exists_id(mobj->mgd,  table, "id=$d", id))
    return FALSE;
    
  if((ri = mgd_is_in_tree(mobj->mgd, table,
          upfield,
          rootid, id)))
    return TRUE;

  return FALSE;
}


/* Walk object's tree ( down ) and collect primary fields of all objects of the same type*/
gchar *midgard_object_get_tree(MgdObject *object, GSList *tnodes)
{
    GValue pval = {0,};
    guint i;
    midgard_res *res = NULL;
    gchar *snodes = "";
    GParamSpec *prop;

    MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
    const gchar *table = midgard_object_class_get_table(klass);
    prop = g_object_class_find_property((GObjectClass *) object->klass, 
            (gchar *)object->data->query->primary);
    
    g_value_init(&pval,prop->value_type);

    g_object_get_property(G_OBJECT(object), g_strdup((gchar *)klass->storage_data->primary), &pval);
    
    switch(prop->value_type){

        case G_TYPE_STRING:
            
            res = mgd_query(object->mgd,
                    "SELECT $s FROM $s WHERE $s='$s' AND (sitegroup in (0, $d) OR $d<>0)",
                    klass->storage_data->primary, table, 
                    klass->storage_data->query->upfield, g_value_get_string(&pval),  
                    mgd_sitegroup(object->mgd), mgd_isroot(object->mgd));
            break;

        case G_TYPE_UINT:

            res = mgd_query(object->mgd,
                    "SELECT $s FROM $s WHERE $s=$d AND (sitegroup in (0, $d) OR $d<>0)",
                    klass->storage_data->primary, table,
                    klass->storage_data->query->upfield, g_value_get_uint(&pval),
                    mgd_sitegroup(object->mgd), mgd_isroot(object->mgd));
            break;                             
    }
    
    g_value_unset(&pval);

    if(!res || !mgd_rows(res)) {
        if(res) mgd_release(res);
        return NULL;
    }

    mgd_fetch(res);

    for (i = 0; i < mgd_cols(res); i++) {

        switch(prop->value_type) {

            case G_TYPE_STRING:
                
                tnodes = g_slist_append(tnodes, g_strdup(mgd_colvalue(res, i)));
                g_object_set(G_OBJECT(object), 
                        g_strdup((gchar *)klass->storage_data->primary), (gchar *)mgd_colvalue(res, i), NULL);
                break;
                
            case G_TYPE_UINT:

                tnodes = g_slist_append(tnodes, GINT_TO_POINTER (mgd_colvalue(res, i)));
                g_object_set(G_OBJECT(object), 
                        g_strdup((gchar *)klass->storage_data->primary), atoi(mgd_colvalue(res, i)), NULL);
                break;
        }
        
        midgard_object_get_tree(object, tnodes);
    }

    return snodes;
}
