/* 
 * Copyright (C) 2006 Piotr Pokora <piotrek.pokora@gmail.com>
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

#ifndef MIDGARD_OBJECT_PARAMETER_H
#define MIDGARD_OBJECT_PARAMETER_H

G_BEGIN_DECLS

/**
 * \defgroup object_parameter Midgard Object Parameter
 *
 * Object's parameters is a special Midgard feature which allows 
 * to extend object's data by storing object's parameters identified by 
 * domain, name and value via midgard_parameter type.
 *
 * The main purpose of parameters is to assign some values to object 
 * without any need to set these values as typical object's properties.
 */ 

extern const GValue *midgard_object_get_parameter(MgdObject *self,
		const gchar *domain, const gchar *name);
 
extern gboolean midgard_object_set_parameter(MgdObject *self,
		const gchar *domain, const gchar *name,
		GValue *value, gboolean lang);

extern MgdObject **midgard_object_list_parameters(MgdObject *self, const gchar *domain);

extern gboolean midgard_object_delete_parameters(MgdObject *self,
                guint n_params, const GParameter *parameters);

extern gboolean midgard_object_purge_parameters(MgdObject *self,
                guint n_params, const GParameter *parameters);

extern MgdObject **midgard_object_find_parameters(MgdObject *self,
                guint n_params, const GParameter *parameters);

G_END_DECLS

#endif /* MIDGARD_OBJECT_PARAMETER_H */
