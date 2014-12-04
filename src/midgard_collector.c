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

#include "midgard/midgard_collector.h"
#include "midgard/types.h"
#include "midgard/query_builder.h"
#include "midgard/midgard_object.h"
#include "midgard_core_query_builder.h"
#include "midgard_mysql.h"

struct _MidgardCollectorPrivate{
	const gchar *typename;
	GType type;
	const gchar *domain;
	GValue *domain_value;
	const gchar *keyname;
	GValue *keyname_value;
	MidgardQueryBuilder *builder;
	MidgardObjectClass *klass;
	GList *values;
	GData *datalist;
};

/* FIXME */
/* This function return duplicated string due to lack of guid's nick.
 * It should be resolved on mgdschema level by column name assigned to property
 * and available on core's level API */
static const gchar *_collector_find_class_property(
		MidgardCollector *self, const gchar *propname)
{
	if(g_str_equal(propname, "guid"))
		return g_strdup_printf(
				"%s.guid",
				midgard_object_class_get_table(
					self->private->klass));

	if(g_str_equal(propname, "sitegroup"))
		return g_strdup_printf(
				"%s.sitegroup",
				midgard_object_class_get_table(
					self->private->klass));

	GParamSpec *pspec = g_object_class_find_property(
			G_OBJECT_CLASS(self->private->klass), propname);

	/* Let's try metadata class */
	if(!pspec) {
		
		MidgardMetadataClass *mklass =
			(MidgardMetadataClass*) g_type_class_peek(g_type_from_name("midgard_metadata"));
		pspec = g_object_class_find_property(G_OBJECT_CLASS(mklass), propname);
	}

	if(!pspec){
		g_warning("Invalid property name '%s' for '%s' class",
				propname,
				self->private->typename);
		return NULL;
	}
	return g_strdup(g_param_spec_get_nick(pspec));
}

static void _unset_subkey_value(gpointer value)
{
	if(value != NULL) {
		/* FIXME */
		/* g_value_unset((GValue *)value);
		g_free(value);
		value = NULL; */
	}
}

static void __unset_subkey_valuelist (gpointer value)
{
	if(value != NULL){
		/* FIXME */
		/* GData *datalist = (GData *)value;
		g_datalist_clear(&datalist);
		value = NULL; */
	}
}

/* Creates new Midgard Collector object instance */
MidgardCollector *midgard_collector_new(
		MidgardConnection *mgd, const gchar *typename, 
		const gchar *domain, GValue *value)
{
	g_assert(mgd);
	g_assert(typename);
	g_assert(domain);
	g_assert(value);

	GType type = g_type_from_name (typename);
	if (!type) {
		g_warning ("Class %s is not registered in GType system", typename);
		return NULL;
	}

	GObjectClass *dbklass = G_OBJECT_CLASS (g_type_class_peek (type));
	if (!dbklass) {
		g_warning ("Can not find %s class", typename);
		return NULL;
	}

	if (!MIDGARD_IS_DBOBJECT_CLASS (dbklass)) {
		g_warning ("Given '%s' classname is not MidgardDBObject derived class", typename);
		return NULL;
	}

	MidgardCollector *self = 
		(MidgardCollector *)g_object_new(MIDGARD_TYPE_COLLECTOR, NULL);
	
	self->private->klass =
		MIDGARD_OBJECT_GET_CLASS_BY_NAME(typename);

	/* Initialize private QB instance and set domain as constraint */
	self->private->builder = midgard_query_builder_new(
			mgd->mgd, typename);
	midgard_query_builder_add_constraint(self->private->builder,
			domain,
			"=", value);

	MidgardQueryConstraint *constraint = midgard_query_constraint_new();
	if(!_midgard_core_qb_parse_property(self->private->builder,
				&constraint, domain)) {
		
		g_object_unref(constraint);

		/* FIXME */
		if(value) {
			
			g_value_unset(value);
			g_free(value);
		}

		g_object_unref(self);

		return NULL;
	}
	
	g_object_unref(constraint);

	self->private->typename = (const gchar *) g_strdup(typename);
	self->private->type = g_type_from_name(typename);
	self->private->domain = (const gchar*) g_strdup(domain);
	self->private->domain_value = value;
	self->private->keyname = NULL;
	self->private->keyname_value = NULL;
	
	g_value_unset(value);
	g_free(value);
	
	self->private->values = NULL;
	g_datalist_init(&self->private->datalist);

	return self;
}

/* Sets key property for the given MidgardCollector object. */
gboolean midgard_collector_set_key_property(
		MidgardCollector *self, const gchar *key, GValue *value)
{
	g_assert(self != NULL);

	MidgardQueryConstraint *constraint = midgard_query_constraint_new();
	if(!_midgard_core_qb_parse_property(self->private->builder, 
				&constraint, key)) {
		
		g_object_unref(constraint);
		return FALSE;
	}
	
	gchar *sql_field = g_strconcat(constraint->priv->current->table, ".",
			constraint->priv->current->field, " AS midgard_collector_key", NULL);

	g_object_unref(constraint);

	self->private->values = g_list_prepend(self->private->values,
			sql_field);

	gboolean ml_fallback_do = TRUE;
	MidgardQueryBuilder *builder = self->private->builder;
	if (builder && builder->priv->unset_lang) 
		ml_fallback_do = FALSE;

	if (midgard_object_class_is_multilang (self->private->klass) && ml_fallback_do) {
		gchar *ml_fallback_field = g_strconcat (midgard_object_class_get_table (self->private->klass), "_i.sid AS sid", NULL);
		self->private->values = g_list_prepend(self->private->values, ml_fallback_field);
	}

	self->private->keyname = (const gchar*) g_strdup(key);

	/* add constraint if the value is set*/
	if(value){
		midgard_query_builder_add_constraint(
				self->private->builder,
				key,
				"=", value);
		self->private->keyname_value = value;
	}

	return TRUE;
}

/* Adds value property for the given MidgardCollector object.*/
gboolean midgard_collector_add_value_property(
		MidgardCollector *self, const gchar *value)
{
	g_assert(self != NULL);
	g_assert(value != NULL);

	if(!self->private->keyname){
		g_warning("Collector's key is not set. Call 'set_key_property' method");
		return FALSE;
	}

	MidgardQueryConstraint *constraint = midgard_query_constraint_new();
	if(!_midgard_core_qb_parse_property(self->private->builder,
				&constraint, value)) {
		
		g_object_unref(constraint);
		return FALSE;
	}
	
	gchar *sql_field = g_strconcat(constraint->priv->current->table, ".",
			constraint->priv->current->field, " AS ", 
			constraint->priv->pspec->name, NULL);
	
	g_object_unref(constraint);
	
	self->private->values = 
		g_list_prepend(self->private->values, sql_field);
	
	return TRUE;
}

/* Sets ( or adds  new key ) and new key's value for the given MidgardCollector. */
gboolean midgard_collector_set(
		MidgardCollector *self, 
		const gchar *key, const gchar *subkey, GValue *value)
{
	g_assert(self != NULL);
	GQuark keyquark = g_quark_from_string(key);
	GData *valueslist;
	GQuark subkeyquark;

	if(subkey == NULL){

		/* This is workaround. Far from being ellegant.
		 * datalist foreach doesn't call user function 
		 * if datalist's data isw NULL.
		 * Even if key quark is set*/
		subkeyquark = g_quark_from_string("0");
		valueslist = (GData *) g_datalist_id_get_data(
				&self->private->datalist,
				subkeyquark);
		
		if(valueslist == NULL)
			g_datalist_init(&valueslist);
		
		GValue *val = g_new0(GValue, 1);
		g_value_init(val, G_TYPE_UINT);
		g_value_set_uint(val, 0);
		g_datalist_id_set_data_full (
				&valueslist,
				subkeyquark,
				val,
				_unset_subkey_value);

		g_datalist_id_set_data_full (
				&self->private->datalist,
				keyquark,
				(gpointer) valueslist,
				__unset_subkey_valuelist);

		return TRUE;
	}

	const gchar *nick = 
		_collector_find_class_property(self, subkey);
	if(!nick)
		return FALSE;
	g_free((gchar *)nick);

	subkeyquark = g_quark_from_string(subkey);
	valueslist = (GData *) g_datalist_id_get_data(
			&self->private->datalist,
			keyquark);
		
	if(valueslist == NULL)
		g_datalist_init(&valueslist);

	g_datalist_id_set_data_full (
			&valueslist,
			subkeyquark, 
			(gpointer) value,
			_unset_subkey_value);

	g_datalist_id_set_data_full (
			&self->private->datalist,
			keyquark,
			(gpointer) valueslist,
			__unset_subkey_valuelist);

	return TRUE;
}

/* Get key's value for the given MidgardCollector object. */
GData *midgard_collector_get(
		MidgardCollector *self, const gchar *key)
{
	g_assert(self);

	if(!self->private->keyname){
		g_warning("Collector's key is not set. Call set_key_property method");
		return FALSE;
	}

	return (GData *) g_datalist_id_get_data (
			&self->private->datalist,
			g_quark_from_string(key));
}

/* Get subkey's value */
GValue *midgard_collector_get_subkey(
		MidgardCollector *self, const gchar *key, const gchar *subkey)
{
	g_assert(self);

	const gchar *nick;
	nick = _collector_find_class_property(self, subkey);
	if(!nick)
		return NULL;

	g_free((gchar *)nick);

	if(&self->private->datalist == NULL){
		g_warning("Collector's key set with NULL value");
		return NULL;
	}

	GData *valueslist = g_datalist_id_get_data (
			&self->private->datalist,
			g_quark_from_string(key));
		
	if(!valueslist) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, 
				"midgard_collector: No subkeys associated with the given '%s' key",
				key);
		return NULL;
	}

	GValue *value =  g_datalist_id_get_data (
			&valueslist, 
			g_quark_from_string(subkey));

	return value;
}

static void __merge_datalist (	GQuark key_id,
				gpointer data,
				gpointer user_data)
{
	GValue *value = (GValue *) data;
	GData *datalist = (GData *) user_data;
	GValue *new_value = g_new0(GValue, 1);
	g_value_init(new_value, G_VALUE_TYPE(value));
	g_value_copy(value, new_value);

	g_datalist_id_set_data_full (
			&datalist,
			key_id,
			(gpointer) user_data,
			_unset_subkey_value);
}

/* Merges collection's keys and its values. */
gboolean midgard_collector_merge(
		MidgardCollector *self, MidgardCollector *mc, 
		gboolean overwrite)
{
	g_assert(self);
	g_assert(mc);

	if(!MIDGARD_COLLECTOR(mc)){
		g_warning("Second argument is not an instance of midgard_collector class!");
		return FALSE;
	}

	guint i = 0;
	gchar **keys = 
		midgard_collector_list_keys(mc);
	GData *self_datalist, *mc_datalist;

	if(!keys) return FALSE;

	while(keys[i] != NULL) {
		
		self_datalist = 
			midgard_collector_get(self, keys[i]);
		if((self_datalist && overwrite) || 
				(self_datalist == NULL)){
			
			mc_datalist = 
				midgard_collector_get(mc, keys[i]);

			if(!self_datalist) 
				g_datalist_init(&self_datalist);

			g_datalist_foreach(
					&mc_datalist,
					__merge_datalist,
					self_datalist);

			g_datalist_id_set_data_full (
					&self->private->datalist,  
					0,
					NULL, 
					_unset_subkey_value);
			}	
		i++;
	}
	g_free(keys);
	return TRUE;
}

typedef struct _dle _dle;

struct _dle{
	guint elements;	
};

static void __count_datalist_elements (	GQuark key_id,
					gpointer data,
					gpointer user_data)
{
	_dle **le = (_dle**) user_data;
	(*le)->elements++;
}

static void __get_collection_keys (	GQuark key_id,
					gpointer data,
					gpointer user_data)
{
	GList **list = user_data;
	*list = g_list_prepend(*list,
			(gchar *)g_quark_to_string(key_id));
}

/* List all keys in collection */
gchar **midgard_collector_list_keys(
		MidgardCollector *self)
{	
	g_assert(self);

	guint list_size = 0 , i = 0;
	_dle *le = g_new(_dle, 1);
	le->elements = 0;

	g_datalist_foreach (&self->private->datalist,
			__count_datalist_elements,
			&le);

	if(le->elements == 0) {
		g_free(le);
		return NULL;
	}

	list_size = le->elements;
	g_free(le);
	
	gchar **keys = g_new(gchar *, list_size+1);
	GList *list = NULL;	

	g_datalist_foreach (&self->private->datalist,
			__get_collection_keys,
			&list);

	if(list) {
		
		/* List is originally prepended ( which is faster than append ) 
		 * but we reverse list in __get_collection_keys.
		 * DO NOT reverse list here */ 
		for(; list; list = list->next){
			keys[i] = list->data;
			i++;
		}
		keys[i] = NULL;
		g_list_free(list);
		return keys;
	}
	
	g_free(keys);
	return NULL;
}

/* Removes key and associated value for the given MidgardCollector object.*/
gboolean midgard_collector_remove_key(
		MidgardCollector *self, const gchar *key)
{
	g_assert(self != NULL);
	g_assert(key != NULL);

	GQuark keyquark = g_quark_from_string(key);

	GData *valueslist = g_datalist_id_get_data (
			&self->private->datalist,
			keyquark);
	if(!valueslist)
		return FALSE;

	g_datalist_clear(&valueslist);
	g_datalist_id_remove_data(&self->private->datalist, keyquark); 

	return TRUE;
}

/* Destroys given MidgardCollector object. */
void midgard_collector_destroy(
		MidgardCollector *self)
{
	g_assert(self);
	g_object_unref(self);
}


/* Parent methods re-implementation */

gboolean midgard_collector_add_constraint(
		MidgardCollector *self, const gchar *name, 
		const gchar *op, const GValue *value)
{
	g_assert(self);
	return MIDGARD_QUERY_BUILDER_GET_CLASS(
			self->private->builder)->add_constraint(
				self->private->builder, name,
				op, value);
}

gboolean midgard_collector_begin_group(
		MidgardCollector *self, const gchar *type)
{
	g_assert(self);
	return MIDGARD_QUERY_BUILDER_GET_CLASS(
			self->private->builder)->begin_group(
				self->private->builder, type);
}

gboolean midgard_collector_end_group(
		MidgardCollector *self)
{
	g_assert(self);
	return MIDGARD_QUERY_BUILDER_GET_CLASS(
			self->private->builder)->end_group(
				self->private->builder);
}

gboolean midgard_collector_add_order(
		MidgardCollector *self,
		const gchar *name, const gchar *dir)
{
	g_assert(self);
	return MIDGARD_QUERY_BUILDER_GET_CLASS(
			self->private->builder)->add_order(
				self->private->builder, name, dir);
}

void midgard_collector_set_offset(
		MidgardCollector *self, guint offset)
{
	g_assert(self);
	MIDGARD_QUERY_BUILDER_GET_CLASS(
			self->private->builder)->set_offset(
				self->private->builder, offset);
}

void midgard_collector_set_limit(
		MidgardCollector *self, guint limit)
{
	g_assert(self);
	MIDGARD_QUERY_BUILDER_GET_CLASS(
			self->private->builder)->set_limit(
				self->private->builder, limit);
}

void midgard_collector_set_lang(
		MidgardCollector *self, gint lang)
{
	g_assert(self);
	MIDGARD_QUERY_BUILDER_GET_CLASS(
			self->private->builder)->set_lang(
				self->private->builder, lang);
}


void midgard_collector_unset_languages(
		MidgardCollector *self)
{
	g_assert(self);
	MIDGARD_QUERY_BUILDER_GET_CLASS(
			self->private->builder)->unset_languages(
				self->private->builder);
}

void midgard_collector_count(
		MidgardCollector *self)
{
	g_assert(self);
	return;
}

static void __set_value(GValue *val, gchar *field_value)
{
	switch(G_VALUE_TYPE(val)){
		
		case G_TYPE_STRING:
			g_value_set_string(val, (gchar *)field_value);
			break;
		
		case G_TYPE_UINT:
			g_value_set_uint(val, atoi(field_value));
			break;
		
		case G_TYPE_INT:
			g_value_set_int(val, atoi(field_value));
			break;
		
		case G_TYPE_FLOAT:
			g_value_set_float(val, g_ascii_strtod(field_value, NULL));
			break;
		
		case G_TYPE_BOOLEAN:
			g_value_set_boolean(val, atoi(field_value));
			break;
	}
}

gboolean midgard_collector_execute(
		MidgardCollector *self)
{
	g_assert(self);

	if(!self->private->keyname){
		g_warning("Collector's key is not set. Call set_key_property method");
		return FALSE;
	}

	if(!self->private->values)
		return FALSE;

        GList *list =
		g_list_reverse(self->private->values);
	GString *sgs = g_string_new("");
	guint i = 0;
	for( ; list; list = list->next){
		if(i > 0)
			g_string_append(sgs, ", ");
		g_string_append(sgs, list->data);
		i++;
	} 

	GValue *pval = NULL;

	gchar *select = g_string_free(sgs, FALSE);
	gchar *sql = _midgard_core_qb_get_sql( 
			self->private->builder, MQB_SELECT_FIELD, 
			select, TRUE);
	
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", sql);
	if(!sql)
		return FALSE;
	
	gint sq = mysql_query(self->private->builder->priv->mgd->msql->mysql, sql);

	if (sq != 0) {
		g_warning("\nQUERY FAILED: \n %s \nQUERY: \n %s",
				mysql_error(self->private->builder->priv->mgd->msql->mysql),
				sql);
		g_free(sql);
		return FALSE;
	}
	g_free(sql);

	i = 0;
	guint ret_rows, ret_fields, j;
	MYSQL_ROW row;
	MYSQL_FIELD *field;
	MYSQL_RES *results = mysql_store_result(self->private->builder->priv->mgd->msql->mysql);
	GParamSpec *pspec;
	//GValue pval = {0, };

	if (!results)
		return FALSE;
	
	if ((ret_rows = mysql_num_rows(results)) == 0) {
		mysql_free_result(results);
		return FALSE;
	}
	
	for(i = 0; i < ret_rows; i++){
		row = mysql_fetch_row(results);
		ret_fields =  mysql_num_fields(results);
	
		for (j = 0; j < ret_fields; j++){
			field = mysql_fetch_field_direct(results, j);
			
			if ((pspec = g_object_class_find_property(
							(GObjectClass *)self->private->klass, 
							field->name)) != NULL) {
			
				pval = g_new0(GValue, 1);
				g_value_init(pval, pspec->value_type);
				
				__set_value(pval, (gchar *)row[j]);

				midgard_collector_set(self,
					(gchar *) row[0],
					field->name,
					pval);

			} else if (ret_fields == 1) {
				
				midgard_collector_set(self, (gchar *) row[0], NULL, NULL);

			} else {

				MidgardMetadataClass *mklass = 
					(MidgardMetadataClass*) g_type_class_peek(g_type_from_name("midgard_metadata"));
				pspec = g_object_class_find_property(G_OBJECT_CLASS(mklass), field->name);
				
				if(pspec) {
					
					pval = g_new0(GValue, 1);
					g_value_init(pval, pspec->value_type);
					
					__set_value(pval, (gchar *)row[j]);
					
					midgard_collector_set(self,
							(gchar *) row[0], field->name, pval);
				}
			}
		}
	}

	mysql_free_result(results);
	return TRUE;
}

/* GOBJECT ROUTINES */

static void _midgard_collector_instance_init(
		GTypeInstance *instance, gpointer g_class)
{
	MidgardCollector *self = (MidgardCollector *) instance;
	self->private = g_new(MidgardCollectorPrivate, 1);

	self->private->domain = NULL;
	self->private->domain_value = NULL;
	self->private->typename = NULL;
	self->private->type = 0;
	self->private->klass = NULL;
	self->private->keyname = NULL;
	self->private->datalist = NULL;
	self->private->keyname_value = NULL;
	self->private->builder = NULL;
	self->private->values = NULL;
}

static void _midgard_collector_finalize(GObject *object)
{
	g_assert(object != NULL);

	MidgardCollector *self = (MidgardCollector *) object;

	if(self->private->datalist)
		g_datalist_clear(&self->private->datalist);
	
	g_free((gchar *)self->private->typename);
	g_free((gchar *)self->private->domain);
	g_free((gchar *)self->private->keyname);
	
	if(self->private->keyname_value) {
		g_value_unset(self->private->keyname_value);
		g_free(self->private->keyname_value);
	}
	
	g_object_unref(self->private->builder);

	GList *list = self->private->values;
	for( ; list; list = list->next){
		g_free(list->data);
	}
	g_list_free(list);

	g_free(self->private);
}

static void _midgard_collector_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardCollectorClass *klass = MIDGARD_COLLECTOR_CLASS (g_class);
	
	gobject_class->finalize = _midgard_collector_finalize;
	klass->set_key_property = midgard_collector_set_key_property;
	klass->add_value_property = midgard_collector_add_value_property;
	klass->set = midgard_collector_set;
	klass->get = midgard_collector_get;
	klass->get_subkey = midgard_collector_get_subkey;
	klass->merge = midgard_collector_merge;
	klass->list_keys = midgard_collector_list_keys;
	klass->remove_key = midgard_collector_remove_key;
	klass->destroy = midgard_collector_destroy;
	klass->add_constraint = midgard_collector_add_constraint;
	klass->begin_group = midgard_collector_begin_group;
	klass->end_group = midgard_collector_end_group;
	klass->add_order = midgard_collector_add_order;
	klass->set_offset = midgard_collector_set_offset;
	klass->set_limit = midgard_collector_set_limit;
	klass->set_lang = midgard_collector_set_lang;
	klass->unset_languages = midgard_collector_unset_languages;
	klass->count = midgard_collector_count;
	klass->execute = midgard_collector_execute;
}

GType midgard_collector_get_type(void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardCollectorClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_collector_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardCollector),
			0,              /* n_preallocs */
			(GInstanceInitFunc) _midgard_collector_instance_init /* instance_init */
		};	
		type = g_type_register_static (MIDGARD_TYPE_QUERY_BUILDER,
				"midgard_collector",
				&info, 0);
	}
	return type;
}
