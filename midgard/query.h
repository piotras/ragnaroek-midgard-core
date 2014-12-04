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

#ifndef MIDGARD_QUERY
#define MIDGARD_QUERY

/* Executes SQL query without data returned.
 * 
 * @param mgd Midgard structure
 * @param sql SQL query which should be executed
 * @param user_data user data to pass to this function
 *
 * @return number of affected rows by the executed command, or -1 on failure
 */ 

gint midgard_query_execute(midgard *mgd, gchar *sql, gpointer user_data);

gint midgard_query_execute_with_fail(midgard *mgd, gchar *sql);

/* Executes SQL query and return data 
 *
 * @param sql SQL query which should be executed
 * @param object MgdObject 
 *
 * This function type cast selected values to object property type if found.
 * If object property is not found , value will be type casted to gpointer.
 * 
 * @return GList with rows value set as GList data or NULL on failure
 */ 

GList *midgard_query_get_single_row(gchar *sql, MgdObject *object);


/* Creates a table defined for object in schema. It also creates all fields
 * which should be used by object.
 *
 * @param object MgdObject object
 *
 * @return TRUE if successful, FALSE otherwise.
 */

gboolean midgard_object_create_storage(MgdObject *object);

/* 
 * Updates table defined for object.
 *
 * \param object MgdObject object
 *
 * \return TRUE if successfull , FALSE otherwise
 *
 */
gboolean midgard_object_update_storage(MgdObject *object);

/* Executes sql query and set object's properties.
 * 
 * @param sql SQL query which will be freed 
 * @param object MgdObject 
 *
 * @return TRUE if object's properties was set, FALSE otherwise.
 */ 
gboolean midgard_object_set_from_query(gchar *sql, MgdObject *object);

#endif
