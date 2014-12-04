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

MidgardUser *midgard_test_user_auth(MidgardConnection *mgd,
		const gchar *username, const gchar *password, const gchar *sitegroup)
{
	g_assert(mgd != NULL);

	MidgardUser *user = midgard_user_auth(mgd, username, password, sitegroup, FALSE);
	
	return user;
}

void midgard_test_user_run(void)
{
	/* TODO */
}
