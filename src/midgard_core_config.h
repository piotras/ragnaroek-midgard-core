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

#ifndef MIDGARD_CORE_CONFIG_H
#define MIDGARD_CORE_CONFIG_H

#include <glib.h>
#include <glib/gstdio.h>

/* Read config file  */
typedef struct
{
	gchar *confdir;
	gchar *blobdir;
	gchar *sharedir;
}mdirs;

/* a cached version of paths */
extern const mdirs *_midgard_core_config_get_config_dirs(void);

extern void _midgard_core_config_dirs_free(mdirs **dirs);

extern gchar *_midgard_core_config_build_path(
		const gchar **dirs, const gchar *filename, gboolean user);

#endif /* MIDGARD_CORE_CONFIG_H */
