/* 
 * Copyright (C) 2004,2005 Piotr Pokora <piotr.pokora@infoglob.pl>
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

#include "midgard/midgard_defs.h"
#include "midgard/types.h"
#include "midgard/query.h"
#include "midgard/midgard_object.h"
#include "midgard/midgard_schema.h"
#include <glib.h>
#include "midgard/midgard_connection.h"
#include "midgard/midgard_object_parameter.h"
#include "midgard/midgard_datatypes.h"

#ifdef G_LOG_DOMAIN
#undef G_LOG_DOMAIN
#define G_LOG_DOMAIN "Schema"
#endif

static void _test_me(MgdObject *object)
{
	/* This is anonymous mode now. 
	 * It should be supported by SG account in config file later.
	 * If you want to add anything here, please add *every* object method 
	 * or function. This is real the best place to use with valgrind */	

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	const gchar *primary_property = midgard_object_class_get_primary_property(klass);
	GString *tmpname;
	gchar *tmpstr;
	const gchar *typename = G_OBJECT_TYPE_NAME(object);

	/* Return when object is midgard_attachment type.
	 * This is an exception. Quota feature calls g_error when location of blob is NULL.
	 * We need g_error in runtime environment, but not now and here */
	if(g_str_equal(typename, "midgard_attachment"))
			return;
	
	GParamSpec *prop = g_object_class_find_property(
			G_OBJECT_GET_CLASS(G_OBJECT(object)), "name");
	if(prop) {
		
		/* Test create */
		tmpname = g_string_new("_schema_test_create");
		g_string_append_printf(tmpname, 
				"%d",
				g_random_int());
		tmpstr = g_string_free(tmpname, FALSE);
		g_object_set(G_OBJECT(object), "name", tmpstr , NULL);	
		g_free(tmpstr);

		if(midgard_object_create(object)){
		
			g_message("Object '%s' create : OK", typename);
		
			/* Test parameters */
			GValue *pval = g_new0(GValue, 1);
			g_value_init(pval, G_TYPE_STRING);
			g_value_set_string(pval, "__midgard_test_value");
			gboolean param_test = FALSE;
			param_test = midgard_object_set_parameter(object, 
					"__midgard_test_domain", 
					"__midgard_test_name", 
					pval, FALSE);
			if(!param_test)
				g_warning("Failed to create test parameter for %s",
						typename);
			
			const GValue *rval = 
				midgard_object_get_parameter(object,
						(const gchar *)"__midgard_test_domain",
						(const gchar *)"__midgard_test_name");

			if(rval == NULL)
				g_warning("Failed to get test parameter for %s", 
						typename);
			
			GValue *dval = g_new0(GValue, 1);
			g_value_init(dval, G_TYPE_STRING);
			g_value_set_string(pval, "");
			param_test = midgard_object_set_parameter(object,
					"__midgard_test_domain",
					"__midgard_test_name",
					pval, FALSE);
			
			if(!param_test)
				g_warning("Failed to delete test parameter for %s",
						typename);

			/* Test update */
			tmpname = g_string_new("_schema_test_update");
			g_string_append_printf(tmpname,
					"%d",
					g_random_int());
			tmpstr = g_string_free(tmpname, FALSE);
			g_object_set(G_OBJECT(object), "name", tmpstr, NULL);
			g_free(tmpstr);
			if(!midgard_object_update(object)) {
				g_warning("Failed to update object %s",	typename);
			} else {
				g_message("Object '%s' update : OK", typename);
			}
			
			/* Test getting object and basic QB functionality 
			 * Get by guid is a wrapper to QB with 
			 * particular constraint */
			MgdObject *new;
			gchar *_guid;
			g_object_get(G_OBJECT(object), "guid" , &_guid, NULL);
			GValue gval = {0, };
			g_value_init(&gval, G_TYPE_STRING);
			g_value_set_string(&gval, _guid);
			new = midgard_object_new(object->mgd, 
					typename,
					&gval);
			g_value_unset(&gval);
			
			if(new == NULL) {
				g_warning("Failed to get %s by %s",
						typename,
						primary_property);
			} else {
				g_object_unref(new);
				g_message("Object '%s' get : OK", typename);
			}
		
			if(midgard_object_delete(object))
				g_message("Object '%s' delete : OK", typename);
		} else {
			g_warning("Failed to create object %s", typename);
		}
	}
}

int 
main (int argc, char **argv)
{
	if (argc != 2){ 
		g_error("You must pass configuration filename as argument!");
		return(1);
	}
	
	gchar *config_file = argv[1];
		
	mgdlib_init();

	MidgardTypeHolder *holder = g_new(MidgardTypeHolder, 1);
	holder->level = G_LOG_LEVEL_WARNING;

	g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK,
			mgd_log_debug_default,
			(gpointer)holder);
	g_log_set_handler("midgard-core", G_LOG_LEVEL_MASK,
			mgd_log_debug_default,
			(gpointer)holder);
	g_log_set_handler("midgard-core-type", G_LOG_LEVEL_MASK,
			mgd_log_debug_default,
			(gpointer)holder);

	g_message("Using '%s' configuration file", config_file);

	GError *err = NULL;
	MidgardConnection *midgard = g_object_new(MIDGARD_TYPE_CONNECTION, NULL);
	MidgardConfig *config = g_object_new(MIDGARD_TYPE_CONFIG, NULL);

	if(!midgard_config_read_file(config, (const gchar *)config_file, FALSE))
		return 1;
	
	/* open config using class member method or function  
	if(!MIDGARD_CONNECTION_GET_CLASS(midgard)->open_config(
				midgard, config, err))
	*/	
	if(!midgard_connection_open_config(midgard, config, err))
		return 1;	
	
	holder->level = midgard_connection_get_loglevel(midgard);

	g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK,
			mgd_log_debug_default, 
			(gpointer)holder);
	g_log_set_handler("midgard-core", G_LOG_LEVEL_MASK,
			mgd_log_debug_default, 
			(gpointer)holder);
	g_log_set_handler("midgard-core-type", G_LOG_LEVEL_MASK,
			mgd_log_debug_default, 
			(gpointer)holder);
				
			
	guint n_types, i;
	GType *all_types = g_type_children(MIDGARD_TYPE_OBJECT,	&n_types);

	const gchar *typename;
	gchar *mgdusername = NULL, *mgdpassword = NULL;
	gboolean tablecreate, tableupdate, testunit;
	g_object_get(G_OBJECT(config), 
			"tablecreate", &tablecreate,
			"tableupdate", &tableupdate,
			"testunit", &testunit,
			"midgardusername", &mgdusername, 
			"midgardpassword", &mgdpassword,
			NULL);
	
	if((mgdusername != NULL) && (mgdpassword != NULL))
		mgd_auth(midgard->mgd, mgdusername, mgdpassword, 1);
	
	for (i = 0; i < n_types; i++) {

		typename = g_type_name(all_types[i]);

		g_message("Type %s", typename);
		MgdObject *object = midgard_object_new(midgard->mgd, typename, NULL);

		/* Create tables */
		if(tablecreate) {
			if(midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(object))){
				midgard_object_create_storage(object);
			}
		}

		/* Update table */
		if(tableupdate) {
			if(midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(object))){
				midgard_object_update_storage(object);
			}
		}
	
		if(g_str_equal(typename, "midgard_parameter"))
			continue;

		/* Test objects */
		if(testunit)
			_test_me(object);
		
		g_object_unref(object);		
	}

	g_free(all_types);
	g_object_unref(config);
	g_free(mgdusername);
	g_free(mgdpassword);
	midgard_connection_close(midgard);
	return 0;
}
