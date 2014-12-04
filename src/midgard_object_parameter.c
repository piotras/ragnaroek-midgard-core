/* 
 * Copyright (C) 2006 Piotr Pokora <piotr.pokora@infoglob.pl>
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

#include "src/midgard_core_object.h"
#include "midgard/types.h"
#include "midgard/midgard_collector.h"
#include "midgard/midgard_error.h"
#include "midgard/guid.h"
#include "midgard/uuid.h"
#include "midgard_core_object_parameter.h"
#include "midgard/midgard_blob.h"

static MidgardCollector *__create_domain_collector(MidgardConnection *mgd, const gchar *domain)
{
	/* Initialize collector for object and its domain */
	GValue *domain_value = g_new0(GValue, 1);
	g_value_init(domain_value, G_TYPE_STRING);
	g_value_set_string(domain_value, domain);
	
	MidgardCollector *mc =
		midgard_collector_new(mgd,
				"midgard_parameter",
				"domain", domain_value);
	
	midgard_collector_set_key_property(mc, "name", NULL);
	
	/* Add value */
	midgard_collector_add_value_property(mc, "value");

	return mc;
}

static MidgardCollector *__get_parameters_collector(MgdObject *object, 
		const gchar *domain)
{	
	if(!object->priv->_params) {
		object->priv->_params = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		g_hash_table_insert(object->priv->_params, (gpointer) g_strdup(domain), NULL);
		return NULL;
	}
	
	return g_hash_table_lookup(object->priv->_params, domain);
}

static void __register_domain_collector(
		MgdObject *object, const gchar *domain, MidgardCollector *mc)
{		
	if(object->priv->_params)
		g_hash_table_insert(object->priv->_params, (gpointer) g_strdup(domain), mc);
}

static gboolean __is_guid_valid(MgdObject *self)
{
	if(self->private->guid == NULL) {

		midgard_set_error(self->mgd->_mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_PROPERTY_VALUE,
				" Guid property is NULL for %s. ", 
				G_OBJECT_TYPE_NAME(self));
		g_warning("%s", self->mgd->_mgd->err->message); 
		g_clear_error(&self->mgd->_mgd->err);
		return FALSE;
	}

	if(!midgard_is_guid(self->private->guid)) {

		midgard_set_error(self->mgd->_mgd,
				MGD_GENERIC_ERROR,
				MGD_ERR_INVALID_PROPERTY_VALUE,
				" Guid property is invalid for %s. ", 
				G_OBJECT_TYPE_NAME(self));
		g_warning("%s", self->mgd->_mgd->err->message);
		g_clear_error(&self->mgd->_mgd->err);
		return FALSE;
	}

	return TRUE;
}

/**
 * midgard_object_get_paramater:
 * @self: #MgdObject instance
 * @domain: parameter's domain string
 * @name: parameter's name string
 *
 * NULL is returned if parameter record with given domain and name is not found.
 * Returned GValue is owned by midgard-core and shouldn't be freed.
 *
 * Returns: GValue which holds value for domain&name pair
 */
const GValue *midgard_object_get_parameter(MgdObject *self,
		const gchar *domain, const gchar *name)
{
	g_assert(self);

	if(!__is_guid_valid(self))
		return NULL;

	GValue *ret_value;
	MidgardCollector *domain_collector = 
		__get_parameters_collector(self, domain);	

	if(domain_collector == NULL){
		
		MidgardCollector *mc = 
			__create_domain_collector(self->mgd->_mgd, domain);

		/* Limit records to these with parent guid */
		GValue guid_value = {0, };
		g_value_init(&guid_value, G_TYPE_STRING);
		g_value_set_string(&guid_value, self->private->guid);
	
		midgard_collector_add_constraint(mc,
				"parentguid", "=", &guid_value);
		g_value_unset(&guid_value);

		midgard_collector_execute(mc);
		
		/* prepend collector to object's parameter list 
		 * we will free it in MgdObject destructor */
		self->priv->parameters = 
			g_slist_prepend(self->priv->parameters, mc);

		ret_value = 
			midgard_collector_get_subkey(mc, name, "value");

		__register_domain_collector(self, domain, mc);

		return (const GValue*)ret_value;
	
	} else {
		
		ret_value = 
			midgard_collector_get_subkey(
					domain_collector, name, "value");
		return (const GValue *)ret_value;
	}

	return NULL;
}

/**
 * midgard_object_set_parameter:
 * @self: #MgdObject instance
 * @domain: parameter's domain string
 * @name: parameter's name string
 * @value: a GValue value which should be set for domain&name pair
 * @lang: boolean which determines if default language should be forced
 *
 * Creates object's parameter object if it doesn't exists, updates otherwise.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */ 
gboolean midgard_object_set_parameter(MgdObject *self,
		const gchar *domain, const gchar *name,
		GValue *value, gboolean lang) 
{

	g_assert(self != NULL);

	if(!__is_guid_valid(self))
		return FALSE;
		
	MgdObject *param;
	const gchar *value_string =
		g_value_get_string(value);
	gboolean delete_parameter = FALSE;
	gboolean do_return_true = FALSE;

	if (value_string == NULL 
			|| *value_string == '\0')
		delete_parameter = TRUE;

	const GValue *get_value = 
		midgard_object_get_parameter(self,
				domain, name);

	MidgardCollector *domain_collector =
		__get_parameters_collector(self, domain);

	if(!domain_collector) {

		domain_collector = __create_domain_collector(self->mgd->_mgd, domain);
		__register_domain_collector(self, domain, domain_collector);
	}

	/* This is the case when set_parameter is invoked
	 * before any get_parameter */
	if(get_value == NULL && delete_parameter) {
		MIDGARD_ERRNO_SET(self->mgd, MGD_ERR_NOT_EXISTS);
		return FALSE;
	}

	/* Parameter doesn't exist. We have to create it */
	if(get_value == NULL && !delete_parameter){
		
		param = midgard_object_new(self->mgd, "midgard_parameter", NULL);
		g_object_set(param, 
				"domain", domain, 
				"name", name,
				"parentguid", self->private->guid,
				NULL);
		
		/* Set parameter's sitegroup using "parent's" sitegroup */
		param->private->sg = self->private->sg;

		g_object_set_property(G_OBJECT(param), "value", value);

		if(midgard_object_create(param)) {
		
			if(domain_collector) {
				if(!midgard_collector_set(domain_collector,
							name, "value", value)){
					g_warning("Failed to update parameter's cache"); 
				}
			}
			
			g_object_unref(param);
			return TRUE;

		} else {
			/* Error should be already set by create */
			g_object_unref(param);
			return FALSE;
		}

	
		/* Parameter exists. if value is '' we delete it.
		 * In any other case we update it */
	} else {
		
		MidgardQueryBuilder *builder = 
			midgard_query_builder_new(self->mgd,
					"midgard_parameter");
		GValue gval = {0,};
		g_value_init(&gval, G_TYPE_STRING);
		g_value_set_string(&gval, self->private->guid);
		midgard_query_builder_add_constraint(builder,
				"parentguid", "=", &gval);
		g_value_unset(&gval);

		g_value_init(&gval, G_TYPE_STRING);
		g_value_set_string(&gval, domain);
		midgard_query_builder_add_constraint(builder,
				"domain", "=", &gval);
		g_value_unset(&gval);

		g_value_init(&gval, G_TYPE_STRING);
		g_value_set_string(&gval, name);
		midgard_query_builder_add_constraint(builder,
				"name", "=", &gval);
		g_value_unset(&gval);

		GObject **ret_object =
			midgard_query_builder_execute(
					builder, NULL);
	
		g_object_unref(builder);
		if(!ret_object){		
			return FALSE;
		}

		if(delete_parameter){

			if(midgard_object_delete(
						MIDGARD_OBJECT(ret_object[0]))) {
				midgard_collector_remove_key(
						domain_collector,
						name);
				do_return_true = TRUE;				
			}
			
		} else {
			
			g_object_set(ret_object[0], "value", value_string, NULL);
			if(midgard_object_update(
						MIDGARD_OBJECT(ret_object[0]))) {
				
				if(domain_collector) {
					midgard_collector_set(domain_collector,
							name, "value", value);
				}

				do_return_true = TRUE;
			}
		}
		
		g_object_unref(ret_object[0]);
		g_free(ret_object);
		if(do_return_true)
			return TRUE;

	}

	return FALSE;
}

/**
 * midgard_object_list_parameters: 
 * @self: a #MgdObject self instance
 * @domain: optional paramaters' domain
 * 
 * Returned objects are midgard_parameter class. Parameter objects are 
 * fetched from database unconditionally if domain i sexplicitly set to NULL. 
 * That is, only those which parent guid property matches object's guid. 
 *
 * Returned array should be freed when no longer needed.
 * 
 * Returns: Newly allocated and NULL terminated array of midgard_parameter objects. 
 */
MgdObject **midgard_object_list_parameters(MgdObject *self, const gchar *domain)
{
	g_return_val_if_fail(self != NULL, NULL);
	g_return_val_if_fail(self->private->guid != NULL, NULL);
	g_return_val_if_fail(self->mgd != NULL, NULL);

	MgdObject **objects = NULL;

	if(domain == NULL) {

		objects = midgard_core_object_parameters_list(
				self->mgd->_mgd, "midgard_parameter", self->private->guid);
		
		return objects;
	}

	GParameter *parameters = g_new0(GParameter, 1);
	GValue dval = {0, };
	g_value_init(&dval, G_TYPE_STRING);
	g_value_set_string(&dval, domain);

	parameters[0].name = "domain";
	parameters[0].value = dval;

	objects = midgard_core_object_parameters_find(
			self->mgd->_mgd, "midgard_parameter", 
			self->private->guid, 1, (const GParameter*) parameters);

	g_value_unset(&dval);
	g_free(parameters);

	return objects;
}

/**
 * midgard_object_parameter_create:
 * @self: #MgdObject instance
 * @n_params: number of properties
 * @parameters: properties list
 *
 * Creates object's parameter using given properties.
 *
 * Returns: newly created #MgdObject of midgard_parameter class or %NULL on failure
 */
MgdObject *midgard_object_parameter_create(MgdObject *self, 
		guint n_params, const GParameter *parameters)
{
	return NULL;

	g_return_val_if_fail(self != NULL, NULL);
	
	if(n_params < 3 || !parameters) {
		
		g_warning("3 parameters expected at least ( domain, name, value )");
		return NULL;
	}
	
	guint i;
	const gchar *prop_name;
	GValue pval;

	for( i = 0; i < n_params; i++) {
		
		prop_name = parameters[i].name;
		pval = parameters[i].value;

		if(g_str_equal(prop_name, "domain")) {

		}

	}
}

/**
 * midgard_object_delete_parameters:
 * @self: #MgdObject instance
 * @n_params: number of properties
 * @parameters: properties list
 *
 * Delete object's parameter(s) which match given properties' values.
 * Properties list in @parameters is optional. All object's parameters are 
 * deleted ( if exist ) if @parameters is explicitly set to %NULL.
 *
 * Returns: %TRUE on success, %FALSE if at least one of the parameters could not be deleted
 */
gboolean midgard_object_delete_parameters(MgdObject *self, 
		guint n_params, const GParameter *parameters)
{
	g_assert(self != NULL);

	if(!self->private->guid) {
		
		g_warning("Object is not fetched from database. Empty guid");
	}

	return midgard_core_object_parameters_delete(
			self->mgd->_mgd, "midgard_parameter", 
			self->private->guid, n_params, parameters);
}

/**
 * midgard_object_purge_parameters:
 * @self: #MgdObject instance
 * @n_params: number of properties
 * @parameters: properties list
 *
 * Purge object's parameter(s) which match given properties' values.
 * Properties list in @parameters is optional. All object's parameters are 
 * purged ( if exist ) if @parameters is explicitly set to %NULL.
 *
 * Returns: %TRUE on success, %FALSE if at least one of the parameters could not be purged
 */
gboolean midgard_object_purge_parameters(MgdObject *self, 
		guint n_params, const GParameter *parameters)
{
	g_assert(self != NULL);

	if(!self->private->guid) {
		
		g_warning("Object is not fetched from database. Empty guid");
	}

	return midgard_core_object_parameters_purge(
			self->mgd->_mgd, "midgard_parameter", 
			self->private->guid, n_params, parameters);
}

/**
 * midgard_object_find_parameters:
 * @self: #MgdObject instance
 * @n_params: number of properties
 * @parameters: properties list
 *
 * Find object's parameter(s) with matching given properties.
 * @parameters argument is optional. All object's parameters are 
 * returned ( if exist ) if @parameters is explicitly set to %NULL.
*
 * Returns: newly created, NULL terminated array of #MgdObject ( midgard_parameter class ) or %NULL on failure
 */
MgdObject **midgard_object_find_parameters(MgdObject *self, 
		guint n_params, const GParameter *parameters)
{
	g_assert(self != NULL);

	if(!self->private->guid) {
		
		g_warning("Object is not fetched from database. Empty guid");
	}

	return midgard_core_object_parameters_find(
			self->mgd->_mgd, "midgard_parameter", 
			self->private->guid, n_params, parameters);
}

/* Internal routines */

MgdObject **midgard_core_object_parameters_list(
		MidgardConnection *mgd, const gchar *class_name, const gchar *guid)
{
	g_assert(class_name != NULL);
	g_assert(guid != NULL);

	MidgardQueryBuilder *builder =
		midgard_query_builder_new(mgd->mgd, class_name);

	if(!builder)
		return NULL;

	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, guid);
	midgard_query_builder_add_constraint(builder,
			"parentguid", "=", &gval);
	g_value_unset(&gval);
	
	GObject **objects =
		midgard_query_builder_execute(builder, NULL);

	g_object_unref(builder);

	return (MgdObject **)objects;
}

MgdObject *midgard_core_object_parameters_create(
		MidgardConnection *mgd, const gchar *class_name,
		const gchar *guid, guint n_params, const GParameter *parameters)
{
	g_assert(mgd != NULL);
	g_assert(class_name != NULL);
	g_assert(guid != NULL);
	g_assert(n_params > 0);

	guint i;
	GParamSpec *pspec = NULL;
	GValue pval;
	const gchar *prop_name;

	MidgardObjectClass *klass = 
		MIDGARD_OBJECT_GET_CLASS_BY_NAME(class_name);

	MgdObject *object = midgard_object_new(mgd->mgd, class_name, NULL);

	if(g_object_class_find_property(G_OBJECT_CLASS(klass), "location")) {
		
		/* Create (and overwrite) location */
		gchar *up_a, *up_b;
		gchar *fname = midgard_uuid_new();
		up_a = g_strdup(" ");
		up_a[0] = g_ascii_toupper(fname[0]);
		up_b = g_strdup(" ");
		up_b[0] = g_ascii_toupper(fname[1]);
		/* g_strdup is used to avoid filling every byte, which should be done with g_new(gchar, 2) */
		gchar *location = g_build_path(G_DIR_SEPARATOR_S,
				up_a, up_b, fname, NULL);
		
		g_object_set(G_OBJECT(object), "location", location , NULL);
		
		g_free(fname);
		g_free(up_a);
		g_free(up_b);
		g_free(location);
	}

	/* Check if properties in parameters are registered for given class */
	for ( i = 0; i < n_params; i++) {
	
		prop_name = parameters[i].name;
		pval = parameters[i].value;

		pspec = g_object_class_find_property(G_OBJECT_CLASS(klass), prop_name);

		if(!pspec) {
			
			MIDGARD_ERRNO_SET(mgd->mgd, MGD_ERR_INVALID_PROPERTY);
			g_warning("Property '%s' not registered for '%s' class", 
					parameters[i].name, class_name);
			g_object_unref(object);
			return NULL;
		}

		g_object_set_property(G_OBJECT(object), prop_name, &pval);
	}
	
	/* Set parentguid so we definitely define parameter or attachment*/
	g_object_set(object, "parentguid", guid, NULL);

	if(!midgard_object_create(object)) {
		
		g_object_unref(object);
		/* error code is set by create method */
		return NULL;
	}

	return object;
}

static GObject **__fetch_parameter_objects(
		MidgardConnection *mgd, const gchar *class_name,
		const gchar *guid, guint n_params, const GParameter *parameters)
{
	MidgardQueryBuilder *builder =
		midgard_query_builder_new(mgd->mgd, class_name);
	
	/* Error is set in builder constructor */
	if(!builder)
		return FALSE;
	
	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, guid);
	
	if(!midgard_query_builder_add_constraint(builder, "parentguid", "=", &gval)) {
		
		g_value_unset(&gval);
		g_object_unref(builder);
		return FALSE;
	}
	
	g_value_unset(&gval);

	guint i;
	const gchar *prop_name;
	GValue pval;
	
	for ( i = 0; i < n_params; i++) {
		
		prop_name = parameters[i].name;
		pval = parameters[i].value;
		
		if(!midgard_query_builder_add_constraint(builder,
					prop_name, "=", &pval)) {
			
			/* error is set by add_constraint */
			g_object_unref(builder);
			return FALSE;
		}
	}
	
	GObject **objects = midgard_query_builder_execute(builder, NULL);
	g_object_unref(builder);
	
	return objects;
}

gboolean midgard_core_object_parameters_delete(
		MidgardConnection *mgd, const gchar *class_name,
		const gchar *guid, guint n_params, const GParameter *parameters)
{
	g_assert(mgd != NULL);
	g_assert(class_name != NULL);
	g_assert(guid != NULL);
	
	GObject **objects = __fetch_parameter_objects(
			mgd, class_name, guid, n_params, parameters);

	/* nothing to delete */
	if(!objects)
		return FALSE;
	
	gboolean rv = FALSE;
	guint i = 0;
	
	while (objects[i] != NULL ) {
	
		if(midgard_object_delete(MIDGARD_OBJECT(objects[i])))
			rv = TRUE;

		g_object_unref(objects[i]);
		i++;
	}

	g_free(objects);
	return rv;
}

gboolean midgard_core_object_parameters_purge(
		MidgardConnection *mgd, const gchar *class_name,
		const gchar *guid, guint n_params, const GParameter *parameters)
{
	g_assert(mgd != NULL);
	g_assert(class_name != NULL);
	g_assert(guid != NULL);
	
	GObject **objects = __fetch_parameter_objects(
			mgd, class_name, guid, n_params, parameters);

	/* nothing to purge */
	if(!objects)
		return FALSE;
	
	gboolean rv = FALSE;
	guint i = 0;
	
	while (objects[i] != NULL ) {
	
		if(midgard_object_purge(MIDGARD_OBJECT(objects[i])))
			rv = TRUE;

		g_object_unref(objects[i]);
		i++;
	}

	g_free(objects);
	return rv;
}

gboolean midgard_core_object_parameters_purge_with_blob(
		MidgardConnection *mgd, const gchar *class_name,
		const gchar *guid, guint n_params, const GParameter *parameters)
{
	g_assert(mgd != NULL);
	g_assert(class_name != NULL);
	g_assert(guid != NULL);
	
	GObject **objects = __fetch_parameter_objects(
			mgd, class_name, guid, n_params, parameters);

	/* nothing to purge */
	if(!objects)
		return FALSE;
	
	gboolean rv = FALSE;
	guint i = 0;
	MidgardBlob *blob = NULL;
	
	while (objects[i] != NULL ) {

		/* ! IMPORTANT !
		 * Blob on filesystem must be deleted first.
		 * In any other case, if something goes wrong, we won't be able 
		 * to check which file is orphaned.
		 * If file is removed and record's deletion failed, on is able 
		 * to check file with blob's file_exists method */
		blob = midgard_blob_new(MIDGARD_OBJECT(objects[i]), NULL);
		
		if(!blob) {
			g_warning("Can not handle blob for given attachment");
			continue;
		}

		if(midgard_blob_exists(blob))
			midgard_blob_remove_file(blob);
		
		midgard_object_purge(MIDGARD_OBJECT(objects[i]));

		g_object_unref(blob);
		g_object_unref(objects[i]);
		i++;
	}

	g_free(objects);
	return rv;
}

MgdObject **midgard_core_object_parameters_find(
		MidgardConnection *mgd, const gchar *class_name,
		const gchar *guid, guint n_params, const GParameter *parameters)
{
	g_assert(mgd != NULL);
	g_assert(class_name != NULL);
	g_assert(guid != NULL);
	
	MidgardQueryBuilder *builder = 
		midgard_query_builder_new(mgd->mgd, class_name);

	/* error is set in builder constructor */
	if(!builder)
		return FALSE;

	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, guid);
	
	if(!midgard_query_builder_add_constraint(builder, "parentguid", "=", &gval)) {
		
		g_value_unset(&gval);
		g_object_unref(builder);
		return FALSE;
	}

	g_value_unset(&gval);

	guint i;
	const gchar *prop_name;
	GValue pval;

	for ( i = 0; i < n_params; i++) {

		prop_name = parameters[i].name;
		pval = parameters[i].value;

		if(!midgard_query_builder_add_constraint(builder, 
					prop_name, "=", &pval)) {

			/* error is set by add_constraint */
			g_object_unref(builder);
			return FALSE;
		}
	}

	GObject **objects = midgard_query_builder_execute(builder, NULL);
	g_object_unref(builder);

	return (MgdObject **)objects;
}	
