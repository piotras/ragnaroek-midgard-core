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

#ifndef MIDGARD_COLLECTOR_H
#define MIDGARD_COLLECTOR_H

/**
 * \defgroup mc Midgard Collector
 *
 * MidgardCollector is special limited resource data handler and 
 * is optimized for performance and data accessibility. 
 * The most important approach is to reuse the same functionality 
 * for Midgard features like style engine, objects' parameters and object's attachments.
 * 
 * http://www.midgard-project.org/development/mrfc/0029.html
 *
 * Object Hierarchy
 * 	- GObject
 * 		- MidgardQueryBuilder
 * 			- MidgardCollector
 *
 */ 

#include "midgard/midgard_connection.h"
#include <midgard/query_builder.h>

/* convention macros */
#define MIDGARD_TYPE_COLLECTOR (midgard_collector_get_type())
#define MIDGARD_COLLECTOR(object)  \
	(G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_COLLECTOR, MidgardCollector))
#define MIDGARD_COLLECTOR_CLASS(klass)  \
	(G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_COLLECTOR, MidgardCollectorClass))
#define MIDGARD_IS_COLLECTOR(object)   \
	(G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_COLLECTOR))
#define MIDGARD_IS_COLLECTOR_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_COLLECTOR))
#define MIDGARD_COLLECTOR_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_COLLECTOR, MidgardCollectorClass))

typedef struct _MidgardCollectorPrivate MidgardCollectorPrivate;

/**
 * \ingroup mc
 * The opaque Midgard Collector type.
 */
typedef struct MidgardCollector MidgardCollector;

struct MidgardCollector{
	MidgardQueryBuilder parent;
	
	/* < private > */
	MidgardCollectorPrivate *private;
};

/**
 * \ingroup mc
 * The opaque Midgard Collector Class type.
 */
typedef struct MidgardCollectorClass MidgardCollectorClass;

struct MidgardCollectorClass{
	MidgardQueryBuilderClass parent;
	
	/* class members */
	gboolean (*set_key_property) (
		MidgardCollector *collector, const gchar *key, GValue *value);

	gboolean (*add_value_property) (
		MidgardCollector *collector, const gchar *value);

	gboolean (*set) (
		MidgardCollector *collector, const gchar *key, 
		const gchar *subkey, GValue *value);

	GData *(*get) (
		MidgardCollector *collector, const gchar *key);

	GValue *(*get_subkey) (
		MidgardCollector *collector, const gchar *key, const gchar *subkey);

	gboolean (*merge) (
		MidgardCollector *collector, MidgardCollector *mc, gboolean overwrite);

	gchar **(*list_keys)(
		MidgardCollector *collector);
	
	gboolean (*remove_key) (
		MidgardCollector *collector, const gchar *key);

	void (*destroy) (
		MidgardCollector *collector);

	gboolean (*add_constraint) (
		MidgardCollector *self, const gchar *name,
		const gchar *op, const GValue *value);

	gboolean (*begin_group) (
		MidgardCollector *self, const gchar *type);
	
	gboolean (*end_group) (
		MidgardCollector *self);

	gboolean (*add_order) (
		MidgardCollector *self, 
		const gchar *name, const gchar *dir);

	void (*set_offset) (
		MidgardCollector *self, guint offset);

	void (*set_limit) (
		MidgardCollector *self, guint limit);
	
	void (*set_lang) (
		MidgardCollector *self, gint lang);

	void (*unset_languages) (
		MidgardCollector *self);

	void (*count) (
		MidgardCollector *self);

	gboolean (*execute) (
		MidgardCollector *self);
};

/** 
 * \ingroup mc
 *
 * Returns MidgardCollector type.
 * Registers the type as  a fundamental GType unless already registered.
 */ 
extern GType midgard_collector_get_type(void);

/**
 * \ingroup mc
 *
 * Creates new Midgard Collector object instance
 *
 * \param[in] mgd MidgardConnection pointer
 * \param[in] domain property name for which domain should be set
 * \param[in] value value of the domain property  
 *
 * \return MidgardCollector object or NULL
 *
 * Property name used as domain must be registered property for te given classname.
 * Given value must be the same type as type registered for domain property.
 * GValue value passed as third argument is owned by Midgard Collector. If value
 * should be reused, its copy should be passed to constructor.
 *
 * Cases to return NULL:
 *
 * - connection handler is invalid
 * - classname is not registered in schema or doesn't extend schema class
 * - domain property is not registered for classname 
 * - value has invalid type  
 *
 */
extern MidgardCollector *midgard_collector_new(
	MidgardConnection *mgd, const gchar *classname, const gchar *domain, GValue *value);

/**
 * \ingroup mc
 *
 * Sets key property for the given MidgardCollector object.
 *
 * \param collector MidgardCollector instance
 * \param[in] key property name for which key should be set
 * \param[in] value value of the key property
 *
 * \return TRUE if key has been added to MidgardCollector , FALSE otherwise
 *
 * Cases to return FALSE:
 *
 * - MidgardCollector object is invalid
 * - key property is not registered for the classname for which MidgardCollector
 *   has been initialized 
 * - value has invalid type
 * - key property is already set for MidgardCollector  
 *
 * If value is explicitly set to NULL , then all key property name's records
 * are selected from database and set in internal collector's resultset. 
 * If not, key property name and its value is used as constraint to limit selected
 * records from database. In latter case add_value_property method should be invoked.
 * GValue value passed as third argument is owned by Midgard Collector. If value
 * should be reused, its copy should be passed to constructor.
 *
 */
extern gboolean midgard_collector_set_key_property(
	MidgardCollector *collector, const gchar *key, GValue *value); 

/**
 * \ingroup mc
 *
 * Adds value property for the given MidgardCollector object.
 *
 * \param collector MidgardCollector instance
 * \param[in] value property name for which value should be set
 *
 * \return TRUE if value has been added to MidgardCollector, FALSE otherwise
 *
 * Cases to return FALSE:
 *
 * - MidgardCollector object is invalid
 * - value property is not registered for the MidgardCollector's classname   
 * 
 * Number of value properties added to Midgard Collector is limited by
 * the number of properties registered for type which has been initialized
 * for the given Midgard Collector instance.
 * 
 * 1.8.1 - referenced properties can be passed as values
 *
 */
extern gboolean midgard_collector_add_value_property(
	MidgardCollector *collector, const gchar *value);

/**
 * \ingroup mc
 *
 * Sets ( or adds  new key ) and new key's subkey for the given MidgardCollector.
 *
 * \param collector MidgardCollector instance
 * \param[in] key name for which value should be set 
 * \param[in] subkey property name which explicitly should be defined as subkey
 * \param[in] value GValue for the given subkey property
 *
 * \return TRUE if key's value has been set, FALSE otherwise
 *
 * Cases to return FALSE:
 *
 * - MidgardCollector object is invalid
 * - subkey property name is not registered for collestor's class
 * - value has invalid type
 *
 * If the key is already added to MidgardCollector then its value 
 * (as subkey&value pair) is destroyed and new one is set. 
 * In other case new key and its subkey&value pair is added  to collector.
 * 
 * Key used in this function is a value returned ( or set ) for collector's key.
 * Keys are collection of values returned from property fields.  
 * Subkey is an explicit  property name.
 *
 * GValue \c value argument is owned by MidgardCollector.
 * If value should be reused, its copy should be passed as method argument.
 * 
 */ 
extern gboolean midgard_collector_set(
	MidgardCollector *collector, const gchar *key, const gchar *subkey, GValue *value);

/**
 * \ingroup mc
 *
 * Get key's value for the given MidgardCollector object.
 *
 * \param collector MidgardCollector instance
 * \param[in] key name of the key to look for
 *
 * \return GData for the given key or NULL if key is not found in 
 * key's collection. 
 * GData keys ( collector's subkeys ) are inserted to GData as
 * Quarks , so you must call g_quark_to_string if you need get strings 
 * ( e.g. implementing hash table for language bindings ).
 *
 */
extern GData *midgard_collector_get(
	MidgardCollector *collector, const gchar *key);

/**
 * \ingroup mc
 *
 * Gets value associated with the key's subkey.
 *
 * \param collector MidgardCollector instance
 * \param[in] key name of the name to look for
 * \param[in] subkey name of the subkey for which value should be found
 *
 * \return GValue of a subkey
 */ 
extern GValue *midgard_collector_get_subkey(
	MidgardCollector *collector, const gchar *key, const gchar *subkey);

/**
 * \ingroup mc
 *
 * Returns all keys in collection.
 *
 * \param self MidgardCollector instance
 *
 * \return NULL terminated array of strings or NULL
 *
 * Returned array of string is newly created array with pointers to each string 
 * in array. It should be freed without any need to free each string.
 * g_free should be used to free list keys instead of g_strfreev.
 *
 */ 
extern gchar **midgard_collector_list_keys(
	MidgardCollector *self);


/**
 * \ingroup mc
 *
 * Merges collection's keys and its values.
 *
 * \param self MidgardCollector instance
 * \param mc MidgardCollector to get keys from
 * \param overwrite boolean which defines whether keys should be overwtitten
 *
 * \return TRUE if collectors has been merged, FALSE otherwise
 *
 * Cases to return FALSE:
 *
 * 	- Second argument is not MIDGARD_COLLECTOR object type
 * 	- Second argument object has no keys collection
 *
 * If third overwrite parameter is set as TRUE then all keys which exists 
 * in self and mc collector's instance will be oberwritten in self colection
 * instance. If set as FALSE , only those keys will be added which do not exist
 * in self collection and exist in mc collection.
 */
extern gboolean midgard_collector_merge(
	MidgardCollector *self, MidgardCollector *mc, gboolean overwrite);

/**
 * \ingroup mc
 *
 * Removes key and associated value for the given MidgardCollector object.
 *
 * \param collector MidgardCollector instance
 * \param[in] key name of the key in key's collection
 *
 * \return TRUE if key and its value have been removed , FALSE if key
 * is not found in collection.
 *
 */
extern gboolean midgard_collector_remove_key(
	MidgardCollector *collector, const gchar *key);

/**
 * \ingroup mc
 *
 * Destroys given MidgardCollector object.
 *
 * \param collector MidgardCollector instance
 *
 * Destroys all key, value pairs in collection. Frees memory allocated
 * for keys and values and for collector object itself.
 *
 */ 
extern void midgard_collector_destroy(
	MidgardCollector *collector);
/**
 * \ingroup mc
 *
 * Method inherited from MidgardQueryBuilder class.
 *
 * http://www.midgard-project.org/api-docs/midgard/core/html/group__qb.html#g523f6a9410a76d0f38192abcf4bc0af1
 */ 
extern gboolean midgard_collector_add_constraint(
	MidgardCollector *self, const gchar *name,
	const gchar *op, const GValue *value);

/**
 * \ingroup mc
 *
 * Method inherited from MidgardQueryBuilder class.
 *
 * http://www.midgard-project.org/api-docs/midgard/core/html/group__qb.html#gf916e78a4f4eb90aa84278148982b734
 */ 
extern gboolean midgard_collector_begin_group(
	MidgardCollector *self, const gchar *type);

/**
 * \ingroup mc
 *
 * Method inherited from MidgardQueryBuilder class.
 *
 * http://www.midgard-project.org/api-docs/midgard/core/html/group__qb.html#g95473e600e569c9b4982501a5097b9ea
 */ 
extern gboolean midgard_collector_end_group(
	MidgardCollector *self);

/**
 * \ingroup mc
 *
 * Method inherited from MidgardQueryBuilder class.
 *
 * http://www.midgard-project.org/api-docs/midgard/core/html/group__qb.html#g2f9fd0992e935498c85c0ffac15d5811
 */ 
extern gboolean midgard_collector_add_order(
	MidgardCollector *self,
	const gchar *name, const gchar *dir);

/**
 * \ingroup mc
 *
 * Method inherited from MidgardQueryBuilder class.
 *
 * http://www.midgard-project.org/api-docs/midgard/core/html/group__qb.html#g0243d37dfe9017bb3146209fbeaa23ee
 */ 
extern void midgard_collector_set_offset(
	MidgardCollector *self, guint offset);

/**
 * \ingroup mc
 *
 * Method inherited from MidgardQueryBuilder class.
 *
 * http://www.midgard-project.org/api-docs/midgard/core/html/group__qb.html#g5142ab9d65759a02e706d8a00902784b
 */ 
extern void midgard_collector_set_limit(
	MidgardCollector *self, guint limit);

/**
 * /ingroup mc
 * 
 * Method inherited from MidgardQueryBuilder class.
 * 
 */
extern void midgard_collector_set_lang(
        MidgardCollector *self, gint lang);

/**
 * \ingroup mc
 *
 * Method inherited from MidgardQueryBuilder class.
 *
 */ 
extern void midgard_collector_unset_languages(
	MidgardCollector *self);

/**
 * \ingroup mc
 * 
 * Not yet re-implemented
 */ 
extern void midgard_collector_count(
	MidgardCollector *self);

/**
 * \ingroup mc
 *
 * Executes SQL query and set internal keys&values collection.
 *
 * \param self MidgardCollector instance
 *
 * \return TRUE on success , FALSE otherwise
 *
 * Re-implementation of MidgardQueryBuilder execute method.
 * Unlike QB's execute method this one returns boolean value.
 * Resultset is stored inernally by MidgardCollector instance.
 *
 * Cases to return FALSE:
 *
 * 	- Method set_key_property was not invoked
 * 	- Method add_valye property was not invoked
 * 	- Database engine returned SQL query syntax error
 * 	- No record was selected from database
 *
 * In any other case this method returns TRUE.
 */ 	
extern gboolean midgard_collector_execute(
	MidgardCollector *self);

#endif /* MIDGARD_COLLECTOR_H */
