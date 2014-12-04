/* 
 * Copyright (C) 2007 Piotr Pokora <piotrek.pokora@gmail.com>
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
 *   */

#include "midgard_core_config.h"

#define MIDGARD_USER_CONF_DIR ".midgard"

gchar *_midgard_core_config_build_path(
		const gchar **dirs, const gchar *filename, gboolean user)
{
	gchar *filepath = NULL;
	gchar *path = NULL;
	guint i = 0, j = 0, k = 0;
	gchar **paths = NULL;
	gchar *fpath = NULL;

	if(user) {
		
		while(dirs && dirs[i]) {	
			i++;
		}
		
		paths = g_new(gchar *, i+3);
		paths[0] = MIDGARD_USER_CONF_DIR;
		paths[1] = "conf.d";

		k = 2;
		while(dirs && dirs[j]) {
			paths[k] = (gchar*)dirs[j];
			j++;
			k++;
		}

		/* Add terminating NULL */
		paths[k] = NULL;

		/* Check if every directory in path exists, if not create */
		i = 0;
		fpath = g_strdup(g_get_home_dir());
		
		while(paths[i]) {
			
			path = g_build_path(G_DIR_SEPARATOR_S,
					fpath, paths[i], NULL);
			
			if(!g_file_test((const gchar *)path,
						G_FILE_TEST_EXISTS)) {
				g_mkdir(path, 0700);
			}
			
			if(!g_file_test((const gchar *)path,
						G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
				
				g_warning("%s is not a directory!", path);
				g_free(path);
				g_free(paths);
				g_free(fpath);
				return NULL;
			}
			
			g_free(fpath);
			fpath = g_strdup(path);
			g_free(path);
			path = NULL;
			i++;
		}
		
		if(filename) {
			
			filepath = g_build_path(G_DIR_SEPARATOR_S,
					fpath, filename, NULL);
		
		} else {

			filepath = fpath;

		}

	} else {
		
		/* TODO */
	}

	g_free(paths);	

	return filepath;
}
