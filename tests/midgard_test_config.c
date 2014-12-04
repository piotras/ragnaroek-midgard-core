/* 
 * Copyright (C) 2008 Piotr Pokora <piotrek.pokora@gmail.com>
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

#include "midgard_test_config.h"

MidgardConfig *midgard_test_config_new_user_config(const gchar *name)
{
	MidgardConfig *config = midgard_config_new();
	gboolean saved = FALSE;
	gboolean read = FALSE;

	g_object_set(config, "database", CONFIG_DB_NAME, NULL);
	saved = midgard_config_save_file(config, name, TRUE);
	g_assert(saved == TRUE);
	g_object_unref(config);

	config = midgard_config_new();
	read = midgard_config_read_file(config, name, TRUE);
	g_assert(read == TRUE);

	return config;
}

void midgard_test_config_init(void)
{
	MidgardConfig *config = midgard_test_config_new_user_config(CONFIG_CONFIG_NAME);
	g_object_unref(config);
}
