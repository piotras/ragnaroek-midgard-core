/* 
 * Copyright (C) 2005 Piotr Pokora <piotr.pokora@infoglob.pl>
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

#include <stdlib.h>
#include <glib.h>
#include "midgard/midgard_datatypes.h"
#include "midgard/midgard_style.h"
#include "midgard/query_builder.h"

typedef struct{
	gchar *name;
	GHashTable *table;
}_midgard_style;

/* Creates a new MgdStyle */
MgdStyle *midgard_style_new(midgard *mgd)
{
	MgdStyle *style = g_new(MgdStyle, 1);
	style->stack = NULL;
	style->styles = g_hash_table_new(g_str_hash, g_str_equal);
	style->page_elements = midgard_hash_strings_new();
	style->style_elements = midgard_hash_strings_new();
	
	return style;
}

static void _get_parent_page_elements(MgdObject *object, GHashTable *table)
{
	MgdObject *parent = midgard_object_get_parent(object);

	if(parent == NULL)
		return;
	
	guint n_objects, i;
	GParamSpec *prop;
	gchar *name, *value;
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(parent);
	gchar *primary = midgard_object_class_get_primary_property(klass);
	
	MidgardQueryBuilder *builder = 
		midgard_query_builder_new(parent->mgd, "midgard_pageelement");

	if(!builder) {
		MIDGARD_ERRNO_SET(parent->mgd, MGD_ERR_INTERNAL);
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				"Invalid query builder configuration (%s)",
				G_OBJECT_TYPE_NAME(G_OBJECT(parent)));
		return;
	}

	/* Find object's primary property */
	prop = g_object_class_find_property(
			G_OBJECT_GET_CLASS(G_OBJECT(parent)), primary);
	
	GValue pval = {0,};
	/* Initialize primary property value */
	g_value_init(&pval,prop->value_type);
	g_object_get_property(G_OBJECT(object), primary, &pval);
	/* I think we can hardcode parent property */
	midgard_query_builder_add_constraint(builder, "page", "=", &pval);
	g_value_unset(&pval);
	/* Get only those elements which has inheritance set */
	g_value_init(&pval,G_TYPE_STRING);
	g_value_set_string(&pval, "inherit");
	midgard_query_builder_add_constraint(builder, "info", "=", &pval);
	g_value_unset(&pval);
	
	GObject **childs = 
		midgard_query_builder_execute(builder, n_objects);

	for(i = 0; i < n_objects; i++){
		
		g_object_get(G_OBJECT(childs[i]), 
					"name", &name, 
					"value", &value,
					NULL);
		g_hash_table_insert(table,
				g_strdup(name), g_strdup(value));
		g_object_unref(childs[i]);		
	}
	midgard_query_builder_free(builder);

	/* Walk to root page */
	_get_parent_page_elements(parent, table);
	
	g_object_unref(parent);	
}

/* Load page and its elements */
gboolean midgard_style_load_page(MgdStyle *style, MgdObject *object)
{
	g_assert(object != NULL);
	gchar *title, *content, *name, *value;
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	gchar *primary = midgard_object_class_get_primary_property(klass);
	
	/* Get title and content of the page */
 	g_object_get(G_OBJECT(object), "title", &title, NULL);
	g_object_get(G_OBJECT(object), "content", &content, NULL);

	g_hash_table_insert(style->page_elements, "title", g_strdup(title));
	g_hash_table_insert(style->page_elements, "content", g_strdup(content));
	
	/* Get all page elements */
	MgdObject **childs = midgard_object_list_childs(object, "midgard_pageelement", &n_objects);
	
	if(childs != NULL) {
		for(i = 0; i < n_objects; i++){
			
			g_object_get(G_OBJECT(childs[i]), 
					"name", &name,
					"value", &value,
					NULL);
			g_hash_table_insert(style->page_elements,
					g_strdup(name), g_strdup(value));
			g_object_unref(childs[i]);
		}
	}

	/* Walk to root page and collect all page elements, 
	 * replacing existing elements  with parent's page ones if inherited */
	_get_parent_page_elements(object, style->page_elements);
}


GHashTable *_style_get_elements_from_dir(midgard *mgd, const gchar *path)        
{
        gchar  *content, **ffname, *fpfname = NULL;
        const gchar *fname = NULL;
        GDir *dir;
        GHashTable *elements;
        
	/* We need full path for multiple midcoms installed and running 
	 * with the same midgard core version */
        /* apath = g_strconcat(MIDGARD_SHARE_DIR, "/", path, NULL); */
        
        dir = g_dir_open(path, 0, NULL);
        if (dir != NULL) {
                
                /* Initialize "elements" ( files ) hash table */
                elements = midgard_hash_strings_new();
                
                /* Read files form directory */
                while ((fname = g_dir_read_name(dir)) != NULL) {
                        
                        fpfname = g_strconcat( path, "/", fname, NULL);
                        /* Get filename without extension. */ 
                        ffname = g_strsplit(fname, ".", -1);
                   
                        /* Get file content and set as value in hash table ,
                         * * filename without extension is a key */
                        if (g_file_get_contents(fpfname,  &content, NULL, NULL) == TRUE) {
                                
                                g_hash_table_insert(elements, g_strdup(ffname[0]), g_strdup(content));
                                g_free(content);
                        }
                        g_free(fpfname);
                        g_strfreev(ffname);
                        
                }
		g_dir_close(dir);
                return elements;
        }
        return NULL;
}


/* This is almost the same function like _get_parent_page_elements.
 * I keep them both to avoid plenty of conditions in one function in a future */
static void _get_parent_style_elements(MgdObject *object, GHashTable *table)
{
	MgdObject *parent = midgard_object_get_parent(object);

	if(parent == NULL)
		return;
	
	guint n_objects, i;
	GParamSpec *prop;
	gchar *name, *value;
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(parent);
	gchar *primary = midgard_object_class_get_primary_property(klass);
	
	MidgardQueryBuilder *builder = 
		midgard_query_builder_new(parent->mgd, "midgard_element");

	if(!builder) {
		MIDGARD_ERRNO_SET(parent->mgd, MGD_ERR_INTERNAL);
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				"Invalid query builder configuration (%s)",
				G_OBJECT_TYPE_NAME(G_OBJECT(parent)));
		return;
	}

	/* Find object's primary property */
	prop = g_object_class_find_property(
			G_OBJECT_GET_CLASS(G_OBJECT(parent)), primary);
	
	GValue pval = {0,};
	/* Initialize primary property value */
	g_value_init(&pval,prop->value_type);
	g_object_get_property(G_OBJECT(object), primary, &pval);
	/* I think we can hardcode parent property */
	midgard_query_builder_add_constraint(builder, "style", "=", &pval);
	g_value_unset(&pval);

	GObject **childs = 
		midgard_query_builder_execute(builder, n_objects);

	for(i = 0; i < n_objects; i++){
		
		g_object_get(G_OBJECT(childs[i]), 
					"name", &name, 
					"value", &value,
					NULL);
		g_hash_table_insert(table,
				g_strdup(name), g_strdup(value));
		g_object_unref(childs[i]);		
	}
	midgard_query_builder_free(builder);

	/* Walk to root page */
	_get_parent_page_elements(parent, table);
	
	g_object_unref(parent);	
}


GHashTable *_style_get_elements_from_db(midgard *mgd, const gchar *path)
{
	g_assert(mgd != NULL);

	gchar *spltd = g_strsplit(path, "/", -1);
	guint n_objects, i = 0;
	GHashTable *table = midgard_hash_strings_new();

	MgdObject *style = midgard_object_get_by_path(mgd, "midgard_style", path);

	/* Get all style elements */
	MgdObject **childs = midgard_object_list_childs(object, "midgard_element", &n_objects);
	
	if(childs != NULL) {
		for(i = 0; i < n_objects; i++){
			
			g_object_get(G_OBJECT(childs[i]),
					"name", &name,
					"value", &value,
					NULL);
			g_hash_table_insert(table, g_strdup(name), g_strdup(value));
			g_object_unref(childs[i]);
		}
	}

	_get_parent_style_elements(style, table);
	g_object_unref(style);
	return table;
}


/* Registers style in Midgard internal styles' stack. */
gboolean midgard_style_register(midgard *mgd, const gchar *path, MgdStyle *style)
{
	g_assert(mgd != NULL);
	g_assert(style != NULL);

	gchar **spltd;
	GHashTable *table;

	/* Database style , prefix mgd:// */
	if(g_str_has_prefix(path, "mgd://")){

		spltd = g_strsplit(path, "mgd://", 2);
		_style_get_elements_from_db(mgd, (const gchar *)spltd[1]);
		g_strfreev(spltd);

	/* File system style , get files */	
	} else if(g_str_has_prefix(path, "file://")) {

		spltd = g_strsplit(path, "file://", 2);
		_style_get_elements_from_dir(mgd, (const gchar *)spltd[1]);
		g_strfreev(spltd);

	/* This is default for database style */	
	} else if(g_str_has_prefix(path, "/")) {

		_style_get_elements_from_db(mgd, (const gchar *)spltd[1]);
	}
}

/* TODO
 * midgard_style_list_styles(MgdStyle *style);
 */ 

gchar *midgard_style_get_element(MgdStyle *style, const gchar *name)
{
	g_assert(style != NULL);
	gchar *element;

	/* First we gete element from page elements */
	element = g_hash_table_lookup(style->page_elements, name);
	
	/* Element not found , we look for element in every style's hash */
	if(element == NULL) {

		/* TODO , queue, stack , whatever lookup */

		if(element == NULL)
			element = "";
	}
	return element;	
}

gboolean midgard_style_is_element_loaded(MgdStyle *style, const gchar *name)
{
	g_assert(style != NULL);

	/* Get element, pointer is returned so no need to free memory */
	if(midgard_style_get_element(style, name) != NULL)
		return TRUE;

	/* Element not found , return FALSE */
	return FALSE;
}

void midgard_style_free(MgdStyle *style)
{
	g_assert(style != NULL);
	
	/* TODO , free style members when defined */
}
