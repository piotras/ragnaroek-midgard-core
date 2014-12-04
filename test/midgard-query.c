/* 
 * Copyright (C) 2005, 2007 Piotr Pokora <piotr.pokora@nemein.com>
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

#include "midgard/midgard.h"
#include "midgard/query.h"

static gchar *config_name;
static gchar *query;
static gchar *config_type = NULL;

static GOptionEntry entries[] = 
{
	{ "config-file", 'c', 0, 
		G_OPTION_ARG_STRING, &config_name, 
		"Name of the configuration", NULL },
	{ "query", 'q', 0, 
		G_OPTION_ARG_STRING, &query, 
		"Query to be executed", "Q" },
	{ "config type", 't', 0, 
		G_OPTION_ARG_STRING, &config_type, 
		"Type of the configuration. (user or system)", NULL },
	{ NULL }
};

int 
main (int argc, char **argv)
{
	GError *error = NULL;
	
	GOptionContext *context = g_option_context_new ("- execute SQL query");
	g_option_context_add_main_entries (context, entries, "midgard2-query");
	g_option_context_parse (context, &argc, &argv, &error);
	
	if(!config_name) {
		
		g_print("Configuration name missed. Try --help \n");
		return(1);
	}
	
	if(!query) {
		
		g_print("Query is missed. Try --help \n");
		return(1);
	}

	g_type_init();

	g_log_set_always_fatal(G_LOG_LEVEL_ERROR);

	MidgardConnection *mgd = midgard_connection_new();
	
	if(!mgd)
		g_error("Can not initialize midgard connection");
	
	MidgardConfig *config = g_object_new(MIDGARD_TYPE_CONFIG, NULL);

	gboolean cts = FALSE;
	if(config_type) {
		
		if(g_str_equal(config_type, "user"))
			cts = TRUE;
	}

	if(!midgard_config_read_file(config, (const gchar *)config_name, cts))
		return 1;

	GError *err = NULL;
	if(!midgard_connection_open_config(mgd, config, err)) {
		return 1;
	}

	//midgard_connection_set_loglevel(mgd, "warning");
	g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK,
			mgd_log_debug_default, (gpointer)mgd);
	
	gchar **queries = g_strsplit(query, ";", 0);
	
	guint i = 0;
	gint rv;
	
	do {
		rv = midgard_query_execute(mgd->mgd, (gchar *)queries[i], NULL);
		
		i++;
	
	} while (queries[i]);

	g_strfreev(queries);
	g_option_context_free(context);
	
	return(0);
}
