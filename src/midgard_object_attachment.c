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

#include "midgard_core_object.h"
#include "midgard/types.h"
#include "midgard_core_object_parameter.h"
#include "midgard/midgard_error.h"
#include "midgard/uuid.h"

/**
 * midgard_object_list_attachments: 
 * @self: a #MgdObject self instance 
 * 
 * Returned objects are midgard_attachment class. Attachments objects are 
 * fetched from database unconditionally. 
 * That is, only those which parent guid property matches object's guid. 
 *
 * Returned array should be freed when no longer needed.
 * 
 * Returns: Newly allocated and NULL terminated array of midgard_attachment objects. 
 */
MgdObject **midgard_object_list_attachments(MgdObject *self)
{
	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(self->private->guid != NULL, NULL);
	g_return_val_if_fail(self->mgd->_mgd != NULL, NULL);

	MgdObject **objects = NULL;

	objects = midgard_core_object_parameters_list(
			self->mgd->_mgd, "midgard_attachment", self->private->guid);
		
	return objects;
}

/**
 * midgard_object_create_attachment:
 * @self: #MgdObject instance
 * @name: name for attachment
 * @title: its title
 * @mimetype: and mimetype
 *
 * Creates object's attachment using given properties.
 * Any property may be explicitly set to NULL.
 *
 * Returns: newly created #MgdObject of midgard_attachment class or %NULL on failure
 */
MgdObject *midgard_object_create_attachment(MgdObject *self, 
		const gchar *name, const gchar *title, const gchar *mimetype)
{
	g_return_val_if_fail(self != NULL, NULL);

	if(!self->private->guid) {
		g_warning("Object is not fetched from database. Empty guid");
		return NULL;
	}

	guint n_params, i = 0;

	/* Check if there's duplicate name */
	if(name != NULL && *name != '\0') {
		
		MidgardQueryBuilder *builder = 
			midgard_query_builder_new(self->mgd, "midgard_attachment");
		
		if(!builder)
			return NULL;

		GValue pval = {0, };
		g_value_init(&pval, G_TYPE_STRING);
		g_value_set_string(&pval, self->private->guid);
		midgard_query_builder_add_constraint(builder, "parentguid", "=", &pval);
		g_value_unset(&pval);
		g_value_init(&pval, G_TYPE_STRING);
		g_value_set_string(&pval, name);
		midgard_query_builder_add_constraint(builder, "name", "=", &pval);
		g_value_unset(&pval);

		i = midgard_query_builder_count(builder);
		g_object_unref(builder);

		if(i > 0) {
			
			MIDGARD_ERRNO_SET(self->mgd, MGD_ERR_OBJECT_NAME_EXISTS);
			return NULL;
		}
	}

	/* create parameters */
	i = 0;
	if(name) i++;
	if(title) i++;
	if(mimetype) i++;

	n_params = i;

	GValue nval = {0, };
	GValue tval = {0, };
	GValue mval = {0, };

	GParameter *parameters = NULL;

	/* TODO, implement  parameters from va_list function if needed */

	if(i > 0) {
		
		parameters = g_new0(GParameter, i);
		
		if(name) {
			i--;
			parameters[i].name = "name";
			g_value_init(&nval, G_TYPE_STRING);
			g_value_set_string(&nval, name);
			parameters[i].value = nval;
		}

		if(title) {
			i--;
			parameters[i].name = "title";
			g_value_init(&tval, G_TYPE_STRING);
			g_value_set_string(&tval, title);
			parameters[i].value = tval;
		}

		if(mimetype) {
			i--;
			parameters[i].name = "mimetype";
			g_value_init(&mval, G_TYPE_STRING);
			g_value_set_string(&mval, mimetype);
			parameters[i].value = mval;
		}
	}

	MgdObject *att = 
		midgard_core_object_parameters_create(self->mgd->_mgd, 
				"midgard_attachment", self->private->guid, 
				n_params, parameters);
	
	for(i = 0; i < n_params; i++) {	
		g_value_unset(&parameters[i].value);
	}

	g_free(parameters);

	return att;
}

/**
 * midgard_object_delete_attachments:
 * @self: #MgdObject instance
 * @n_params: number of properties
 * @parameters: properties list
 *
 * Delete object's attachments(s) which match given properties' values.
 * Properties list in @parameters is optional. All object's attachments are 
 * deleted ( if exist ) if @parameters is explicitly set to %NULL.
 *
 * Returns: %TRUE on success, %FALSE if at least one of the attachment could not be deleted
 */
gboolean midgard_object_delete_attachments(MgdObject *self, 
		guint n_params, const GParameter *parameters)
{
	g_assert(self != NULL);

	if(!self->private->guid) {
		
		g_warning("Object is not fetched from database. Empty guid");
	}

	return midgard_core_object_parameters_delete(
			self->mgd->_mgd, "midgard_attachment", 
			self->private->guid, n_params, parameters);
}


/**
 * midgard_object_purge_attachments:
 * @self: #MgdObject instance
 * @delete_blob: whether blob should be deleted as well
 * @n_params: number of properties
 * @parameters: properties list
 *
 * Purge object's attachments(s) which match given properties' values.
 * Properties list in @parameters is optional. All object's attachments are 
 * purged ( if exist ) if @parameters is explicitly set to %NULL.
 *
 * @delete_blob should be set to %TRUE if midgard_attachment holds a reference 
 * to blob located on filesystem ( it should be set to %TRUE by default ).
 * However, if midgard_attachment is created for blobs sharing and file should not 
 * be deleted, @delete_blob should be set to %FALSE.
 *
 * There's no way to determine if midgard_attachment is sharing blob, so aplication 
 * itelf is responsible to create such own logic.
 *
 * Returns: %TRUE on success, %FALSE if at least one of the attachment could not be purged
 */
gboolean midgard_object_purge_attachments(MgdObject *self, gboolean delete_blob,
		guint n_params, const GParameter *parameters)
{
	g_assert(self != NULL);

	if(!self->private->guid) {
		
		g_warning("Object is not fetched from database. Empty guid");
	}

	gboolean rv = FALSE;

	if(delete_blob) {
	
		rv = midgard_core_object_parameters_purge_with_blob(
				self->mgd->_mgd, "midgard_attachment", 
				self->private->guid, n_params, parameters);
	
	} else {
		
		rv = midgard_core_object_parameters_purge(
				self->mgd->_mgd, "midgard_attachment",
				self->private->guid, n_params, parameters);
	}

	return rv;
}

/**
 * midgard_object_find_attachments:
 * @self: #MgdObject instance
 * @n_params: number of properties
 * @parameters: properties list
 *
 * Find object's attachment(s) with matching given properties.
 * @parameters argument is optional. All object's attachments are 
 * returned ( if exist ) if @parameters is explicitly set to %NULL.
*
 * Returns: newly created, NULL terminated array of #MgdObject ( midgard_attachment class ) or %NULL on failure
 */
MgdObject **midgard_object_find_attachments(MgdObject *self, 
		guint n_params, const GParameter *parameters)
{
	g_assert(self != NULL);

	if(!self->private->guid) {
		
		g_warning("Object is not fetched from database. Empty guid");
	}

	return midgard_core_object_parameters_find(
			self->mgd->_mgd, "midgard_attachment", 
			self->private->guid, n_params, parameters);
}
