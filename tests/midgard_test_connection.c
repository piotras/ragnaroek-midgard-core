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

#include "midgard_test.h"

MidgardConnection *midgard_test_connection_open_user_config(const gchar *name, MidgardConfig **config)
{
	g_assert(*config == NULL);
	MidgardConnection *mgd = midgard_connection_new();

	gboolean opened = FALSE;
	GError *err = NULL;

	*config = midgard_test_config_new_user_config(name);
	g_assert(*config != NULL);

	opened  = midgard_connection_open_config(mgd, *config, err);
	
	/* Fail, we didn't make connection */
	g_assert(opened == TRUE);

	/* Fail if we connected and error is set */
	g_assert(err == NULL);

	/* Fail if midgard error is not OK */
	MIDGARD_TEST_ERROR_OK(mgd);
	
	return mgd;
}

void midgard_test_connection_run(void)
{
	MidgardConfig *config = NULL;
	MidgardConnection *mgd = midgard_test_connection_open_user_config(CONFIG_CONFIG_NAME, &config);
	g_object_unref(mgd);
	g_object_unref(config);
}
