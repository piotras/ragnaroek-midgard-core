/* Midgard schema , records and objects definition.
   
  Copyright (C) 2004,2005 Piotr Pokora <pp@infoglob.pl>
  
  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <config.h>
#include "midgard/midgard_schema.h"
#include "midgard/midgard_datatypes.h"
#include "midgard/midgard_type.h"
#include "schema.h"
#include "midgard_core_object.h"
#include "midgard_core_object_class.h"
#include <libxml/parser.h>
#include <libxml/parserInternals.h>

/* Global schema_trash used when MgdSchema is destroyed 
 * Trash must be global as we share pointers between different schemas.
 * Especially Midgard core object's MgdSchemaTypes are shared.
 * */
static GSList *schema_trash = NULL;

/* Global schema file path, it's a pointer to filename being parsed.
 * Only for warning purposes */
static const gchar *parsed_schema = NULL;

/* prototypes */ 
void _copy_schemas(gpointer key, gpointer value, gpointer userdata);

MgdSchemaPropertyAttr *_mgd_schema_property_attr_new(){
	
	MgdSchemaPropertyAttr *prop = g_new(MgdSchemaPropertyAttr, 1);
	prop->gtype = G_TYPE_NONE;
	prop->type = NULL;
	prop->klass = NULL;
	prop->dbtype = NULL;
	prop->field = NULL;
	prop->dbindex = FALSE;
	prop->table = NULL;
	prop->tablefield = NULL;
	prop->upfield = NULL;
	prop->parentfield = NULL;
	prop->primaryfield = NULL;
	prop->is_multilang = FALSE;
	prop->link = NULL;
	prop->link_target = NULL;
	prop->is_primary = FALSE;
	prop->is_reversed = FALSE;
	prop->is_link = FALSE;
	prop->is_linked = FALSE;
	prop->description = g_strdup ("");
	prop->user_fields = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	
	return prop; 
}

void _mgd_schema_property_attr_free(gpointer data)
{
	g_assert(data != NULL);

	MgdSchemaPropertyAttr *prop = (MgdSchemaPropertyAttr *)data;

	g_free((gchar *)prop->type);
	g_free((gchar *)prop->dbtype);
	g_free((gchar *)prop->field);
	g_free((gchar *)prop->table);
	
	if(prop->tablefield != NULL)
		g_free((gchar *) prop->tablefield);

	g_free((gchar *)prop->upfield);
	g_free((gchar *)prop->parentfield);
	g_free((gchar *)prop->primaryfield);
	g_free((gchar *)prop->link);
	g_free((gchar *)prop->link_target);

	g_free((gchar *)prop->description);
	prop->description = NULL;

	g_hash_table_destroy(prop->user_fields);
	prop->user_fields = NULL;

	g_free(prop);
}

void _destroy_property_hash(gpointer key, gpointer value, gpointer userdata)
{
	MgdSchemaPropertyAttr *prop = (MgdSchemaPropertyAttr *) value;
	gchar *name = (gchar *) key;
	
	if(prop)
		_mgd_schema_property_attr_free(prop);

	if(name)
		g_free(name);
}

MgdSchemaTypeQuery *_mgd_schema_type_query_new()
{
	MgdSchemaTypeQuery *query = g_new(MgdSchemaTypeQuery, 1);
	query->select = NULL;
	query->select_full = NULL;		
	query->link = NULL;
	query->tables = g_strdup("");
	query->primary = NULL;
	query->parentfield = NULL;
	query->upfield = NULL;
	query->use_lang = FALSE;

	return query;
}

void _mgd_schema_type_query_free(MgdSchemaTypeQuery *query)
{	
	g_assert(query != NULL);
	
	if(query->select != NULL) {
		g_free((gchar *)query->select);
		query->select = NULL;
	}

	if(query->select_full != NULL)
		g_free((gchar *)query->select_full);

	g_free((gchar *)query->primary);
	if(query->link != NULL)
		g_free((gchar *)query->link);
	if(query->tables != NULL)
		g_free((gchar *)query->tables);
	if(query->parentfield != NULL)
		g_free((gchar *)query->parentfield);
	if(query->upfield != NULL)
		g_free((gchar *)query->upfield);

	if(query)
		g_free(query);
}

void _destroy_query_hash(gpointer key, gpointer value, gpointer userdata)
{
	MgdSchemaTypeQuery *query = (MgdSchemaTypeQuery *) value;
	gchar *name = (gchar *) key;
	
	if(query)
		_mgd_schema_type_query_free(query);

	if(name)
		g_free(name);
}

MgdSchemaTypeAttr *_mgd_schema_type_attr_new()
{
	MgdSchemaTypeAttr *type = g_new(MgdSchemaTypeAttr, 1);
	type->base_index = 0;
	type->num_properties = 0;
	type->params = NULL;
	type->properties = NULL;
	type->table = NULL;
	type->parent = NULL;
	type->primary = NULL;
	type->property_up = NULL;
	type->property_parent = NULL;				
	type->property_count = 0;
	type->use_lang = FALSE;
	type->tables = g_hash_table_new(g_str_hash, g_str_equal);
	type->prophash = g_hash_table_new_full(g_str_hash, g_str_equal, 
			g_free, _mgd_schema_property_attr_free);
	type->query = _mgd_schema_type_query_new();
	type->childs = NULL;
	type->metadata = 
		g_hash_table_new_full(g_str_hash, g_str_equal,
				g_free, (GDestroyNotify) _mgd_schema_property_attr_free);

	type->user_values = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	return type;
}

void _mgd_schema_type_attr_free(MgdSchemaTypeAttr *type)
{
	g_assert(type != NULL);

	g_free((gchar *)type->table);
	g_free((gchar *)type->parent);
	g_free((gchar *)type->primary);
	g_free((gchar *)type->property_up);
	g_free((gchar *)type->property_parent);

	g_hash_table_foreach(type->tables, _destroy_query_hash, NULL);
	g_hash_table_destroy(type->tables);	
	
	//g_hash_table_foreach(type->prophash, _destroy_property_hash, NULL);
	g_hash_table_destroy(type->prophash);

	g_hash_table_destroy(type->metadata);

	_mgd_schema_type_query_free(type->query);

	g_free(type->params);
	g_free(type->properties);

	if (type->user_values)
		g_hash_table_destroy (type->user_values);
	type->user_values = NULL;

	g_free(type);
}

/* return type's struct  or NULL if type is not in schema */
MgdSchemaTypeAttr *
midgard_schema_lookup_type (MidgardSchema *schema, gchar *name)
{
	g_assert(schema != NULL);
	g_assert(name != NULL);

	return (MgdSchemaTypeAttr *) g_hash_table_lookup(schema->types, name);	
}


/**
 * \ingroup mgdschema
 * \internal
 * Lookup for property attributes 
 *
 * @param schema MgdSchema used by application
 * @param type Name of type registered in schema
 * @param prop Name of property registered for type
 *
 * @return hash table which contains property's attributes
 */ 
GHashTable *
midgard_schema_lookup_type_member(MidgardSchema *schema, const gchar *typename, gchar *property)
{
	g_assert(schema != NULL);
	g_assert(typename != NULL);
	g_assert(property != NULL);
	
	MgdSchemaTypeAttr *sts;
	
	sts = midgard_schema_lookup_type(schema, (gchar *)typename);
	
	if (sts != NULL) 
		return g_hash_table_lookup(sts->prophash, property);

	return NULL;
}

static void __warn_msg(xmlNode * node, const gchar *msg)
{
	gchar *prop = (gchar *)xmlGetProp(node, (const xmlChar *)"name");
	g_warning("%s ( %s:%s, %s on line %ld )", msg, node->name, prop, 
			parsed_schema, xmlGetLineNo(node));
	g_free(prop);
}

/* Define which element names should be addedd to schema. 
 * We change them only here , and in xml file itself.
 */
static const gchar *mgd_complextype[] = { "type", "property", "Schema", "description", "include", NULL };
static const gchar *mgd_attribute[] = { "name", "table", "parent", "parentfield", 
	"type", "link", "upfield", "field", "multilang", "reverse", "primaryfield", "dbtype", "index", NULL };

static const gchar *schema_allowed_types[] = { 
	"integer", 
	"unsigned integer", 
	"float", 
	"double",
	"string", 
	"text", 
	"guid", 
	"longtext", 
	"bool", 
	"boolean", 
	"datetime", 
	NULL 
};

/* RESERVED type names */
static const gchar *rtypes[] = { 
	"midgard_sitegroup", 
	"midgard_account",
	NULL
};

/* RESERVED table names */
static const gchar *rtables[] = {
	"sitegroup",
	"repligard",
	"midgard_account",
	NULL
};

/* RESERVED column names */
static const gchar *rcolumns[] = {
	"metadata_",
	"sitegroup",
	"group", /* This is MySQL reserved word */
	NULL
};

static gboolean strv_contains(const char **strv, const xmlChar *str) {
        g_assert(strv != NULL);
        g_assert(str != NULL);
        while (*strv != NULL) {
                if (g_str_equal(*strv++, str)) {
                        return TRUE;
                }
        }
        return FALSE;
}

static void check_metadata_column(const xmlChar *column){
	
	if(g_str_has_prefix((const gchar *)column, "metadata_"))
		g_critical("Column names prefixed with 'metadata_' are reserved ones");
}

static void check_property_name(const xmlChar *name, const gchar *typename){
	
	g_assert(name != NULL);

	if(g_strrstr_len((const gchar *)name, strlen((const gchar *)name), "_") != NULL)
		g_critical("Type '%s', property '%s'. Underscore not allowed in property names!",
				typename, name);

	/* temporary solution for midgard_quota */
	if(!g_str_equal(typename, "midgard_quota"))
		if(g_str_equal(name, "sitegroup"))
			g_critical("Type '%s'.Property name 'sitegroup' is reserved!",
					typename);

	if(g_str_equal(name, "guid"))
		g_critical("Type '%s'.Property name 'guid' is reserved!",
				typename);
		
}


/* Get all type attributes */
static void
_get_type_attributes(xmlNode * node, MgdSchemaTypeAttr *type_attr)
{
	xmlAttr *attr;
	xmlChar *attrval;

	if (node != NULL){

		attr = node->properties;
		attr = attr->next; /* avoid getting already added */

		while (attr != NULL){

			if(!strv_contains(mgd_attribute, attr->name)){
				g_warning("Wrong attribute '%s' in '%s' on line %ld",
						attr->name, parsed_schema, xmlGetLineNo(node));	
			}	
			attrval = xmlNodeListGetString (node->doc, attr->children, 1);
					
			if(g_str_equal(attr->name, "table")) {
				/* Check if table name is reserved one */
				if (strv_contains(rtables, attrval)) {
					g_critical("'%s' is reserved table name",
							attrval);
				}								
				type_attr->table = g_strdup((const gchar *)attrval);
			}

			if(g_str_equal(attr->name, "parent"))
				type_attr->parent = g_strdup((gchar *)attrval);
	
			xmlFree(attrval);
			attr = attr->next;			
		}
	}
}

static void _midgard_core_schema_get_property_type(xmlNode *node, 
		MgdSchemaTypeAttr *type_attr, MgdSchemaPropertyAttr *prop_attr)
{
	gboolean typeisset = FALSE;
	xmlAttr *attr;
	xmlChar *attrval;

	if (node != NULL) {
		
		attr = node->properties;
		attr = attr->next;
		
		while (attr != NULL) {

			if(g_str_equal(attr->name, "type")) {

				attrval = xmlNodeListGetString (node->doc, attr->children, 1);

				if (strv_contains(schema_allowed_types, attrval)) {
					
					typeisset = TRUE;
					
					prop_attr->type = g_strdup((const gchar*)attrval);
			
					if(g_str_equal(attrval, "string"))
						prop_attr->gtype = MGD_TYPE_STRING;

					if(g_str_equal(attrval, "integer"))
						prop_attr->gtype = MGD_TYPE_INT;

					if(g_str_equal(attrval, "unsigned integer"))
						prop_attr->gtype = MGD_TYPE_UINT;

					if(g_str_equal(attrval, "float"))
						prop_attr->gtype = MGD_TYPE_FLOAT;
					
					/* FIXME, change to MGD_TYPE_DOUBLE once mgdschema supports it */
					if(g_str_equal(attrval, "double"))
						prop_attr->gtype = MGD_TYPE_FLOAT;

					if(g_str_equal(attrval, "boolean"))
						prop_attr->gtype = MGD_TYPE_BOOLEAN;

					if(g_str_equal(attrval, "bool"))
						prop_attr->gtype = MGD_TYPE_BOOLEAN;
				
					if(g_str_equal(attrval, "datetime"))
						prop_attr->gtype = MGD_TYPE_TIMESTAMP;
				
					if(g_str_equal(attrval, "longtext"))
						prop_attr->gtype = MGD_TYPE_LONGTEXT;
				
					if(g_str_equal(attrval, "text"))
						prop_attr->gtype = MGD_TYPE_LONGTEXT;				

					if(g_str_equal(attrval, "guid"))
						prop_attr->gtype = MGD_TYPE_GUID;
				
				} else {

					prop_attr->type = g_strdup("string");
					prop_attr->gtype = MGD_TYPE_STRING;
					g_debug("Setting string type for declared %s type", attrval);
				}

				xmlFree(attrval);
			}

			attr = attr->next;
		}
	}
	
	return;
}

/* Get property attributes */
static void 
_get_property_attributes(xmlNode * node, 
		MgdSchemaTypeAttr *type_attr, MgdSchemaPropertyAttr *prop_attr)
{
	xmlAttr *attr;
	xmlChar *attrval;

	if (node != NULL){

		attr = node->properties;
		attr = attr->next;

		while (attr != NULL){

			attrval = xmlNodeListGetString (node->doc, attr->children, 1);
		
			if(!strv_contains(mgd_attribute, attr->name)){
				g_warning("Wrong attribute '%s' in '%s' on line %ld",
						attr->name, parsed_schema, xmlGetLineNo(node));
			}
			
			if(g_str_equal(attr->name, "dbtype"))
				prop_attr->dbtype = g_strdup((gchar *)attrval);
			
			if(g_str_equal(attr->name, "field")){
				/* Check if column name is reserved one */
				check_metadata_column(attrval);
				if (strv_contains(rcolumns, attrval)) {

					if(prop_attr->field != NULL)
						g_free((gchar *) prop_attr->field);

					g_critical("'%s' is reserved column name",
							attrval);
				}						

				if(prop_attr->field == NULL)
					prop_attr->field = g_strdup((gchar *)attrval);
			}

			if(g_str_equal(attr->name, "index")) {
				
				if(g_str_equal(attrval, "yes"))
					prop_attr->dbindex = TRUE;
			}
		
			//prop_attr->table = g_strdup(type_attr->table);
			if(g_str_equal(attr->name, "table")){
				/* Check if table name is reserved one */
				if (strv_contains(rtables, attrval)) {
					
					g_critical("'%s' is reserved table name",
							attrval);
					
					if(prop_attr->table != NULL)
						g_free((gchar *)prop_attr->table);
					
					prop_attr->table = NULL;
				}
				
				if(prop_attr->table == NULL) {
					
					if(prop_attr->table)
						g_free((gchar *)prop_attr->table);
					
					prop_attr->table = g_strdup((gchar *)attrval);
				}
			}

			if(g_str_equal(attr->name, "upfield")){
				check_metadata_column(attrval);
				/* Check if column name is reserved one */
				if (strv_contains(rcolumns, attrval)) {
					g_critical("'%s' is reserved column name",
							attrval);
				}				
				if(!type_attr->property_up){
					xmlChar *tmpattr = 
						xmlGetProp (node, (const xmlChar *)"name");
					type_attr->property_up = g_strdup((gchar *)tmpattr);
					type_attr->query->upfield = g_strdup((gchar *)attrval);
					if(!_midgard_core_object_prop_up_is_valid(prop_attr->gtype)) {
						__warn_msg(node, "Invalid type for up property.");
						/* FIXME, Enable it about 8.09.6 */
						/* g_error("Wrong schema attribute"); */
					}
					xmlFree(tmpattr);
				} else {
					__warn_msg(node, "upfield redefined");
				}
				
				prop_attr->upfield = g_strdup((gchar *)attrval);
				if(prop_attr->field != NULL)
					g_free((gchar *)prop_attr->field);
				prop_attr->field = g_strdup((gchar *)attrval);
			}

			if(g_str_equal(attr->name, "parentfield")){
				check_metadata_column(attrval);
				/* Check if column name is reserved one */
				if (strv_contains(rcolumns, attrval)) {
					g_critical("'%s' is reserved column name",
							attrval);
				}
				if(!type_attr->property_parent){
					xmlChar *tmpattr = xmlGetProp (node, (const xmlChar *)"name");
					type_attr->property_parent = g_strdup((gchar *)tmpattr);
					type_attr->query->parentfield = g_strdup((gchar *)attrval);
					if(!_midgard_core_object_prop_parent_is_valid(prop_attr->gtype)) {
						__warn_msg(node, "Invalid type for parent property.");
						/* FIXME, Enable it about 8.09.6 */
						/* g_error("Wrong schema attribute"); */
					}
					xmlFree(tmpattr);
				} else {                       
					__warn_msg(node, "parentfield redefined");
				}
				prop_attr->parentfield = g_strdup((gchar *)attrval);
				if(prop_attr->field != NULL)
					g_free((gchar *)prop_attr->field);
				prop_attr->field = g_strdup((gchar *)attrval);
			}			

			if(g_str_equal(attr->name, "multilang")) {
				if(g_str_equal(attrval, "yes")) {
					prop_attr->is_multilang = TRUE;	
					type_attr->use_lang = TRUE;
				}
			}

			if(g_str_equal(attr->name, "primaryfield")) {
				check_metadata_column(attrval);
				/* Check if column name is reserved one */
				if (strv_contains(rcolumns, attrval)) {
					g_critical("'%s' is reserved column name",
							attrval);
				}
				prop_attr->primaryfield = g_strdup((gchar *)attrval);
				if(prop_attr->field != NULL)
					g_free((gchar *)prop_attr->field);
				prop_attr->field = g_strdup((gchar *)attrval);
				prop_attr->is_primary = TRUE;
				xmlChar *tmpattr = xmlGetProp (node, (const xmlChar *)"name");
				type_attr->primary = g_strdup((gchar *)tmpattr);
				type_attr->query->primary = g_strdup((gchar *)attrval);
				xmlFree(tmpattr);
			}

			if(g_str_equal(attr->name, "reverse")) {
				if(g_str_equal(attrval, "yes"))
					prop_attr->is_reversed = TRUE;	
			}
			
			if(g_str_equal(attr->name, "link")) {
				
				if(!_midgard_core_object_prop_link_is_valid(prop_attr->gtype)) {
					__warn_msg(node, "Invalid type for link property.");
					/* FIXME, Enable it about 8.09.6 */
					/* g_error("Wrong schema attribute"); */
				}

				gchar **link = g_strsplit((gchar *)attrval, ":", -1);
				prop_attr->link = g_strdup((gchar *)link[0]);
				prop_attr->is_link = TRUE;
				if(link[1])
					prop_attr->link_target = 
						g_strdup((gchar *)link[1]);
				else 
					prop_attr->link_target =
						g_strdup("guid");

				g_strfreev(link);
			}

			xmlFree(attrval);
			attr = attr->next;								
		}
	}
}

static void
_get_user_fields (xmlNode *node, MgdSchemaPropertyAttr *prop_attr)
{
	xmlNode *snode = NULL;
	
	for (snode = node->children; snode != NULL; snode = snode->next) {

		if (snode->type == XML_ELEMENT_NODE){

			xmlParserCtxtPtr parser = xmlNewParserCtxt();
			gchar *value = (gchar *)xmlNodeGetContent(snode);

			xmlChar *decoded = 
				xmlStringDecodeEntities(parser, (const xmlChar *) value, XML_SUBSTITUTE_REF, 0, 0, 0);

			if (g_str_equal (snode->name, "description")) {

				g_free (prop_attr->description); /* Explicitly free, it's initialized with new empty string by default */
				prop_attr->description = g_strdup((gchar *)decoded);

			} else {

				g_hash_table_insert(prop_attr->user_fields, g_strdup((gchar *)snode->name), g_strdup((gchar *)decoded));
	 		}

			g_free(decoded);
			xmlFreeParserCtxt(parser);
			g_free(value);
		}
	}
}
static void
__get_properties (xmlNode * curn , MgdSchemaTypeAttr *type_attr, MidgardSchema *schema)
{
	xmlNode *obj = NULL;
	xmlChar *nv = NULL, *dparent;	
	MgdSchemaPropertyAttr *prop_attr;
	gchar *tmpstr;

	for (obj = curn; obj; obj = obj->next){
		
		if (obj->type == XML_ELEMENT_NODE){
			/* Get nodes with "name" name. Check if obj->name was defined */
			nv = xmlGetProp (obj, (const xmlChar *)"name"); 
			
			if (g_str_equal(obj->name, "property")){
				
				dparent = xmlGetProp(obj->parent,
						(const xmlChar *)"name");
				check_property_name(nv, (const gchar *)dparent);
				/* g_debug("\t Property: %s ", nv); */ 
				prop_attr = g_hash_table_lookup(
						type_attr->prophash, (gchar *)nv);
				
				if (prop_attr != NULL) {
					g_warning("Property '%s' already added to %s",
							nv ,dparent);
				} else {
					/* FIXME
					 * one prop_attr seems to be assigned nowhere 
					 * probably fixed ( duplicated sitegroup property
					 * in midgard_quota schema ) */
					prop_attr = _mgd_schema_property_attr_new();
					_midgard_core_schema_get_property_type(obj, type_attr, prop_attr);
					_get_user_fields (obj, prop_attr);
					_get_property_attributes(obj, type_attr, prop_attr);
				
					if(prop_attr->field == NULL)
						prop_attr->field = g_strdup((gchar *)nv);
					
					if(prop_attr->is_primary 
							&& prop_attr->gtype != MGD_TYPE_UINT){
						tmpstr = (gchar *)xmlGetProp(obj->parent,
								(const xmlChar *)"name");
						g_message(" %s - type for primaryfield not defined"
								" as 'unsigned integer'."
								" Forcing uint type"
								, tmpstr);
						g_free(tmpstr);
					}

					g_hash_table_insert(type_attr->prophash, 
							g_strdup((gchar *)nv), prop_attr);
				}					
				xmlFree(dparent);				
			}
				
			xmlFree(nv);
		}
	}
}

static void
__get_type_user_values (xmlNode *node, MgdSchemaTypeAttr *type_attr)
{
	xmlNode *snode = NULL;
	xmlNode *user_value = NULL;

	for (snode = node; snode != NULL; snode = snode->next) {

		if (snode->type != XML_ELEMENT_NODE)
			continue;

		if (g_str_equal (snode->name, "user_values")) {

			for (user_value = snode->children; user_value != NULL; user_value = user_value->next) {
	      
				if (user_value->type != XML_ELEMENT_NODE)
 					continue;

				xmlParserCtxtPtr parser = xmlNewParserCtxt();
				gchar *value = (gchar *)xmlNodeGetContent(user_value);
 
				xmlChar *decoded =
    					xmlStringDecodeEntities (parser, (const xmlChar *) value,
			       				XML_SUBSTITUTE_REF, 0, 0, 0);
  
				g_hash_table_insert (type_attr->user_values,
						g_strdup ((gchar *)user_value->name), g_strdup ((gchar *)decoded));
			
				g_free (decoded);
	  			xmlFreeParserCtxt (parser);
    				g_free (value);
			}
		}
	}
}


static void
_get_element_names (xmlNode * curn , MgdSchemaTypeAttr *type_attr, MidgardSchema *schema)
{
	xmlNode *obj = NULL;
	xmlChar *nv = NULL;
	MgdSchemaTypeAttr *duplicate;

	for (obj = curn; obj; obj = obj->next){
		
		if (obj->type != XML_ELEMENT_NODE)
			continue;
			
		if(!strv_contains(mgd_complextype, obj->name)){

			g_warning("Wrong node name '%s' in '%s' on line %ld",
					obj->name, parsed_schema, xmlGetLineNo(obj));
		}

		if (g_str_equal(obj->name, "type")) {
			
			nv = xmlGetProp (obj, (const xmlChar *)"name"); 

			/* Check if type name is reserved one */
			if (strv_contains(rtypes, nv)) 
				g_critical("'%s' is reserved type name", nv);

			duplicate = midgard_schema_lookup_type(schema, (gchar *)nv);

			if (duplicate != NULL) {
			
				g_error("%s:%s already added to schema!", obj->name, nv);

			} else {

				type_attr = _mgd_schema_type_attr_new();
				type_attr->cols_idx = MGD_RES_OBJECT_IDX;
				g_hash_table_insert(schema->types, g_strdup((gchar *)nv), type_attr);
				_get_type_attributes (obj, type_attr);
			
				if (obj->children != NULL) {

					__get_properties(obj->children, type_attr, schema);
					__get_type_user_values (obj->children, type_attr);
				}
			}

			xmlFree(nv);

			continue;	
		}

		if (obj->children != NULL)
				_get_element_names (obj->children, type_attr, schema);
	}
}

/* Start processing type's properties data. 
 * This function is called from __get_tdata_foreach 
 */
void __get_pdata_foreach(gpointer key, gpointer value, gpointer user_data)
{
	MgdSchemaTypeAttr *type_attr  = (MgdSchemaTypeAttr *) user_data;

	if(value == NULL) {
		g_warning("Missed property attribute for defined class with table %s", type_attr->table);
		return;
	}

	MgdSchemaPropertyAttr *prop_attr = (MgdSchemaPropertyAttr *) value;
	const gchar *table, *tables, *primary, *parentname, *ext_table = NULL;
	gchar *tmp_sql = NULL , *nick = "";
	MgdSchemaTypeQuery *tquery;
	/* type_attr->num_properties is used ( and is mandatory! ) 
	 * when we register new object types and set class properties */
	guint n = ++type_attr->num_properties;;
	const gchar *pname = (gchar *) key;
	const gchar *fname = NULL, *upfield = NULL, *parentfield = NULL; 
	gchar *tmpstr = NULL;
	GParamSpec **params = type_attr->params;
	GString *tmp_gstring_sql, *table_sql;
	
	/* Set default string type if not set */
	if(!prop_attr->type) {
		prop_attr->type = g_strdup("string");
		prop_attr->gtype = MGD_TYPE_STRING;
	}		
	
	table = prop_attr->table;
	tables = type_attr->query->tables;
	parentname = table;

	/* Search for upfield and parentfield definition.
	 * We will use them ( if found ) for nick ( table.field ) definition 
	 * without need to define field attribute.  */
	
	upfield = prop_attr->upfield;
	parentfield = prop_attr->parentfield;
	primary = prop_attr->primaryfield;
	
	ext_table = table;
	
	/* "external" tables for type */
	
	/* Create GHashTable for all tables described in type's schema definition */
	if (ext_table != NULL){

		/* Check if table key already exists , and create one if not */
		if ((tquery = g_hash_table_lookup(type_attr->tables, ext_table)) == NULL){
			tquery = _mgd_schema_type_query_new();
			g_hash_table_insert(type_attr->tables, g_strdup(ext_table), tquery);      
		}
		
		if(g_strstr_len (tables, strlen(tables), ext_table) == NULL){
			tmpstr = g_strconcat(tables, ext_table, ",", NULL);
			g_free((gchar *)tables);
			type_attr->query->tables = g_strdup(tmpstr);
			g_free(tmpstr);
		}
		
		table_sql = g_string_new("");
		if(tquery->select)
			g_string_append(table_sql, tquery->select);
		
		if (prop_attr->is_multilang)
			tquery->use_lang = TRUE;
			
		fname = prop_attr->field;
			
		if (fname != NULL) {
			tmpstr = g_strjoin(".", ext_table, fname,NULL);	
		} else {
			tmpstr = g_strjoin(".", ext_table, pname,NULL);
			if(prop_attr->field != NULL)
				g_free((gchar *)prop_attr->field);
			prop_attr->field = g_strdup(pname);
		}
		nick = g_strdup(tmpstr);
		
		tmp_gstring_sql = g_string_new(" ");		
		if(prop_attr->gtype == MGD_TYPE_TIMESTAMP) {			
			g_string_append_printf(tmp_gstring_sql,
					"IF(%s = '0000-00-00 00:00:00', NULL, %s) AS %s",
					tmpstr, tmpstr, pname);
		} else {
			g_string_append_printf(tmp_gstring_sql,
					"%s AS %s",
					tmpstr, pname);
		}
		g_free(tmpstr);
		tmpstr = g_string_free(tmp_gstring_sql, FALSE);
		/* Avoid duplicated coma */
		if(tquery->select)
			g_string_append(table_sql, ", ");
		g_string_append(table_sql, tmpstr);
		g_free(tmpstr);
		g_free(tquery->select);
		tquery->select = g_string_free(table_sql, FALSE);
	
		g_hash_table_insert(type_attr->tables, (gchar *)ext_table, tquery);
		/* "external" tables end */
	}

	table = type_attr->table;
	
	if (ext_table == NULL) {
		tmpstr = NULL;	
		if(prop_attr->table != NULL) g_free((gchar *)prop_attr->table);
		prop_attr->table = g_strdup(table);
		fname = prop_attr->field;
		if ( (fname == NULL ) && ((upfield == NULL) && (parentfield == NULL)) 
				&& (primary == NULL)){
			tmpstr = g_strjoin(".", table, pname,NULL);
		} else if((fname != NULL) && ((parentfield == NULL) || (upfield == NULL))) {
			tmpstr = g_strjoin(".", table, fname,NULL);
		}
		if(tmpstr){
			table_sql = g_string_new("");
			if(type_attr->query->select)
				g_string_append(table_sql, type_attr->query->select);			
			nick = g_strdup(tmpstr);
			tmp_gstring_sql = g_string_new(" ");
			if(prop_attr->gtype == MGD_TYPE_TIMESTAMP) {
				g_string_append_printf(tmp_gstring_sql,
						"IF(%s = '0000-00-00 00:00:00', NULL, %s) AS %s",
						tmpstr, tmpstr, pname);
			} else {
				g_string_append_printf(tmp_gstring_sql,
						"%s AS %s",
						tmpstr, pname);
			}
			g_free(tmpstr);
			tmpstr = g_string_free(tmp_gstring_sql, FALSE);
			/* Do not prepend coma if type select is empty */
			if(type_attr->query->select)
				g_string_append(table_sql, ",");
			g_string_append(table_sql, tmpstr);	
			g_free(tmpstr);
			g_free(type_attr->query->select);
			type_attr->query->select = g_string_free(table_sql, FALSE);
		}	
	}
	/* FIXME , this needs to be refactored for real property2field mapping */
	if(!type_attr->query->primary)
		type_attr->query->primary =  g_strdup("guid");

	if(!type_attr->primary){
		type_attr->primary = g_strdup("guid");
	}
	

	if(primary != NULL && nick == NULL) {
		if(nick != NULL)
			g_free(nick);
		nick = g_strjoin(".", table, primary, NULL);
		tmpstr = g_strconcat(nick, " AS ", pname, NULL);
		tmp_sql = g_strjoin(",", tmpstr, type_attr->query->select,NULL);
		if(type_attr->query->select)
			g_free(type_attr->query->select);
		type_attr->query->select = g_strdup(tmp_sql);
		g_free(tmpstr);
		g_free(tmp_sql);                        
	}

	/* Set field which is used as up.
	 * At the same time we define property_up which is keeps such data  */ 
	if(upfield != NULL && nick == NULL) {
		if(nick != NULL)
			g_free(nick);
		nick = g_strjoin(".", table, upfield, NULL);
		tmpstr = g_strconcat(nick, " AS ", pname, NULL);
		tmp_sql = g_strjoin(",", tmpstr, type_attr->query->select,NULL);
		if(type_attr->query->select)
			g_free(type_attr->query->select);
		type_attr->query->select = g_strdup(tmp_sql);
		g_free(tmpstr);
		g_free(tmp_sql);
	}
	
	/* Set parentfield and property_parent */
	if(parentfield != NULL && nick == NULL) {    
		if(nick != NULL)
			g_free(nick);
		nick = g_strjoin(".", table, parentfield, NULL);
		tmpstr = g_strconcat(nick, " AS ", pname, NULL);
		tmp_sql = g_strjoin(",", tmpstr, type_attr->query->select,NULL);
		if(type_attr->query->select)
			g_free(type_attr->query->select);
		type_attr->query->select = g_strdup(tmp_sql);
		g_free(tmpstr);
		g_free(tmp_sql);
	}
	
	/* Force G_TYPE_UINT type for primaryfield */
	if((prop_attr->is_primary) && (prop_attr->gtype != G_TYPE_UINT))
		prop_attr->gtype = G_TYPE_UINT;
	
	/* Create param_specs for object's properties */
	switch (prop_attr->gtype) {
		
		case MGD_TYPE_STRING:
			params[n-1] = g_param_spec_string(
					pname, nick, prop_attr->description,
					"",  G_PARAM_READWRITE);
			break;

		case MGD_TYPE_UINT:
			params[n-1] = g_param_spec_uint(
					pname, nick, prop_attr->description,
					0, G_MAXUINT32, 0, G_PARAM_READWRITE);
			break;

		case MGD_TYPE_INT:
			params[n-1] = g_param_spec_int(
					pname, nick, prop_attr->description,
					G_MININT32, G_MAXINT32, 0, G_PARAM_READWRITE);
			break;

		case MGD_TYPE_FLOAT:
			params[n-1] = g_param_spec_float(
					pname, nick, prop_attr->description,
					-G_MAXFLOAT, G_MAXFLOAT, 0, G_PARAM_READWRITE);
			break;

		case MGD_TYPE_BOOLEAN:
			params[n-1] = g_param_spec_boolean(
					pname, nick, prop_attr->description,
					FALSE, G_PARAM_READWRITE);
			break;

		default:
			params[n-1] = g_param_spec_string(
					pname, nick, prop_attr->description,
					"", G_PARAM_READWRITE);			
					
	}

	prop_attr->tablefield = g_strdup(nick);
	g_free(nick);    
}

/* Append all tables' select to type's full_select */
void _set_object_full_select(gpointer key, gpointer value, gpointer userdata)
{
	GString *fullselect = (GString *) userdata;
	MgdSchemaTypeQuery *query = (MgdSchemaTypeQuery *) value;
	
	g_string_append_printf(fullselect, ",%s", query->select);
}
static void _set_property_col_idx(gpointer key, gpointer value, gpointer userdata)
{	
	MgdSchemaPropertyAttr *prop_attr = (MgdSchemaPropertyAttr *) value;
	MgdSchemaTypeAttr *type_attr = (MgdSchemaTypeAttr *) userdata;
	if (prop_attr->is_multilang)
		return;
	type_attr->cols_idx++;
	prop_attr->property_col_idx = type_attr->cols_idx;
}

static void _set_multilang_property_col_idx(gpointer key, gpointer value, gpointer userdata)
{
	MgdSchemaPropertyAttr *prop_attr = (MgdSchemaPropertyAttr *) value;
	MgdSchemaTypeAttr *type_attr = (MgdSchemaTypeAttr *) userdata;
	if (!prop_attr->is_multilang)
		return;
	type_attr->cols_idx++;
	prop_attr->property_col_idx = type_attr->cols_idx;
}

/* Get types' data. Type's name and its values.
 * We can not start creating GParamSpec while we parse xml file.
 * This is internally used with _register_types function. At this 
 * moment we can count all properties of type and call sizeof
 * with correct values
 */ 

void __get_tdata_foreach(gpointer key, gpointer value, gpointer user_data)
{
	GType new_type;
	MgdSchemaTypeAttr *type_attr = (MgdSchemaTypeAttr *) value;
	guint np;	
	gchar *tmpstr;
	
	if(g_type_from_name(key))
		return;
	
	np = g_hash_table_size(type_attr->prophash);

	type_attr->params = g_malloc(sizeof(GParamSpec*)*(np+1)); 

	if (np > 0) {
		g_hash_table_foreach(type_attr->prophash, __get_pdata_foreach, type_attr);
		
		/* add NULL as last value, 
		 * we will use ud.param as GParamSpec for object's properties. */ 
		
		type_attr->params[np] = NULL;
		
		/* Define tree management fields and tables */

		if (type_attr->table != NULL ) {
			tmpstr = g_strconcat(type_attr->query->tables, type_attr->table, NULL);	
			g_free((gchar *)type_attr->query->tables);
			type_attr->query->tables = g_strdup(tmpstr);
			g_free(tmpstr);
		}

		//type_attr->parent = typename;
		
		/* Set NULL to child_cname, we will fill this value when all types
		 * are already registered ( _schema_postconfig ).  */ 
		type_attr->childs = NULL;
		
		GString *fullselect = g_string_new("");
		g_string_append(fullselect , type_attr->query->select );
		g_hash_table_foreach(type_attr->tables,  _set_object_full_select, fullselect);
		g_hash_table_foreach(type_attr->prophash, _set_property_col_idx, type_attr);
		if (type_attr->use_lang)
			g_hash_table_foreach(type_attr->prophash, _set_multilang_property_col_idx, type_attr);
		type_attr->query->select_full = g_string_free(fullselect, FALSE);

		/* Initialize metadata DB data structures */
		midgard_core_metadata_property_attr_new(type_attr);

		/* Register type , and initialize class. We can not add properties while 
		 * class is registered and we can not initialize class with properties later.
		 * Or rather "we should not" do this later. */ 
		if (type_attr->params != NULL) {

			new_type = midgard_type_register(key, type_attr);
			if (new_type){
				
				GObject *foo = g_object_new(new_type, NULL);
				g_object_unref(foo); 
			}
		}
	
	} else {
		g_warning("Type %s has less than 1 property!", (gchar *)key);
	}
}

/* Copy source schema types ( class names ) and its MgdSchemaType structures
 * to destination schema */
void _copy_schemas(gpointer key, gpointer value, gpointer userdata)
{
	gchar *typename = (gchar *)key;
	MgdSchemaTypeAttr *type = (MgdSchemaTypeAttr *)value;
	MgdSchema *ts = (MgdSchema *) userdata;

	if(!g_hash_table_lookup(ts->types, typename)) {
		
		if(type != NULL)
			g_hash_table_insert(ts->types, g_strdup(typename), type); 
	}
}

/* We CAN NOT define some data during __get_tdata_foreach.
 * parent and childs relation ( for example ) must be done AFTER
 * all types are registered and all types internal structures are
 * already initialized and defined
 */ 
void __postconfig_schema_foreach(gpointer key, gpointer value, gpointer user_data)
{
	MgdSchemaTypeAttr *type_attr = (MgdSchemaTypeAttr *) value, *parenttype;
	MidgardSchema *schema = (MidgardSchema *) user_data;
	gchar *typename = (gchar *) key;
	const gchar *parentname;

	parentname = type_attr->parent;

	/* FIXME, remove this if some validation without parents shoudl be done */
	if(parentname == NULL)
		return;
	
	if(parentname != NULL) {
		/* Set child type name for parent's one */
		/* PP: I use g_type_from_name(typename)) as gslist->data because
		 * typename ( even typecasted to char ) doesn't work.
		 * If you think this is wrong and have better solution , 
		 * feel free to change this code. */
		
		/* WARNING!! , all types which has parenttype defined will be appended to 
		 * this list. It doesn't matter if they are "registered" in such schema.
		 * Every GObjectClass has only one corresponding C structure , so it is impossible
		 * to initialize new MgdSchemaTypeAttr per every classname 
		 * in every schema and keep them  separated for every GObjectClass , 
		 * as we define only one GType.
		 * It may be resolved by expensive hash lookups for every new object instance.
		 */

		if ((parenttype = midgard_schema_lookup_type(schema, (gchar *)parentname)) != NULL){
			/* g_debug("type %s, parent %s", typename, parentname); */
			if (!g_slist_find(parenttype->childs, 
						(gpointer)g_type_from_name(typename))) {
				parenttype->childs = 
					g_slist_append(parenttype->childs, 
							(gpointer)g_type_from_name(typename));
			}			
		}
	}
}

/**
 * \internal
 * The namespace of the Midgard schema files.
 */
static const char *NAMESPACE = "http://www.midgard-project.org/repligard/1.4";

/* Forward declarations used by read_schema_path() */
static void read_schema_directory(GHashTable *files, const char *directory);
static void read_schema_file(GHashTable *files, const char *file);

/**
 * \internal
 * Reads the schema file or directory from the given path to the given schema
 * hash table. A warning is logged if the path does not exist.
 */
static void read_schema_path(GHashTable *files, const char *path) {
        g_assert(files != NULL);
        g_assert(path != NULL);
        if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
                read_schema_directory(files, path);
        } else if (g_file_test(path, G_FILE_TEST_EXISTS)) {
                read_schema_file(files, path);
        } else {
                g_warning("Schema path %s not found", path);
        }
}

/**
 * \internal
 * Reads the schema file from the given path to the given schema hash table.
 * The file is parsed as an XML file and the resulting xmlDocPtr is inserted
 * to the schema hash table with the file path as the key. Both the XML
 * document and the file path are newly allocated and must be freed when
 * the hash table is destroyed. Logs a warning and returns if the file
 * has already been included in the hash table or if it cannot be parsed.
 * Any includes within the parsed file are processed recursively.
 */
static void read_schema_file(GHashTable *files, const char *file) {
        g_assert(files != NULL);
        g_assert(file != NULL);

        /* Guard against duplicate loading of schema files */
        if (g_hash_table_lookup(files, file) != NULL) {
                g_warning("Skipping already seen schema file %s", file);
                return;
        }

        /* Parse this schema file */
        /* g_debug("Reading schema file %s", file); */
        xmlDocPtr doc = xmlParseFile(file);
        if (doc == NULL) {
                g_error("Skipping malformed schema file %s", file);
                return;
        }
        xmlNodePtr root = xmlDocGetRootElement(doc);
        if (root == NULL
            || root->ns == NULL
            || !g_str_equal(root->ns->href, NAMESPACE)
            || !g_str_equal(root->name, "Schema")) {
                g_error("Skipping invalid schema file %s", file);
                xmlFreeDoc(doc);
                return;
        }

        /* Add the schema file to the hash table */
        g_hash_table_insert(files, g_strdup(file), doc);

        /* Read all included schema files */
        xmlNodePtr node = root->children;
        while (node != NULL) {
                xmlChar *attr = xmlGetNoNsProp(node, (xmlChar *) "name");
                if (node->type == XML_ELEMENT_NODE
                    && node->ns != NULL
                    && g_str_equal(node->ns->href, NAMESPACE)
                    && g_str_equal(node->name, "include")
                    && attr != NULL) {
                        GError *error = NULL;
                        gchar *name = g_filename_from_utf8(
                                (const gchar *) attr, -1, NULL, NULL, &error);
                        if (name == NULL) {
                                g_warning("g_filename_from_utf8: %s", error->message);
                                g_error_free(error);
                        } else if  (g_path_is_absolute(name)) {
                                read_schema_path(files, name);
                                g_free(name);
                        } else {
                                gchar *dir = g_path_get_dirname(file);
                                gchar *path = g_build_filename(dir, name, NULL);
                                read_schema_path(files, path);
                                g_free(path);
                                g_free(dir);
                                g_free(name);
                        }
                }
                if (attr != NULL) {
                        xmlFree(attr);
                }
                node = node->next;
        }
	/* xmlFreeDoc(doc); */ /* wtf? */	
}

/**
 * \internal
 * Reads the schema directory from the given path to the given schema
 * hash table. Ignores all hidden files and subdirectories, and recurses
 * into normal subdirectories. All files with the suffix ".xml" are read
 * and inserted into the schema hash table.
 */
static void read_schema_directory(GHashTable *files, const char *directory) {
        g_assert(files != NULL);
        g_assert(directory != NULL);

        GError *error = NULL;
        GDir *dir = g_dir_open(directory, 0, &error);
        if (dir != NULL) {
                const gchar *file = g_dir_read_name(dir);
                while (file != NULL) {
                        gchar *path = g_build_filename(directory, file, NULL);
                        if (g_str_has_prefix(file, ".")) {
                                /* skip hidden files and directories */
                        } else if (g_str_has_prefix(file, "#")) {
                                /* skip backup files and directories */
                        } else if (!g_file_test(path, G_FILE_TEST_IS_DIR)) {
                                /* recurse into subdirectories */
                                read_schema_directory(files, path);
                        } else if (g_str_has_suffix(file, ".xml")) {
                                /* read xml file, guaranteed to exist */
                                read_schema_file(files, path);
                        }
                        g_free(path);
                        file = g_dir_read_name(dir);
                }
                g_dir_close(dir);
        } else {
                g_warning("g_dir_open: %s", error->message);
                g_error_free(error);
        }
}

/**
 * \internal
 * Reads all the schema files identified by the given path and any includes
 * within the schema files. Returns the fiels as a newly allocated hash
 * table with the schema file paths as keys and the parsed XML documents
 * (xmlDocPtr) as values. The returned hash table contains content destroy
 * functions so the caller can just use g_hash_table_destroy() to release
 * all memory associated with the returned hash table.
 */
static GHashTable *read_schema_files(const char *path) {
        g_assert(path != NULL);
        GHashTable *files = g_hash_table_new_full(
                g_str_hash, g_str_equal, g_free, (GDestroyNotify) xmlFreeDoc);
        read_schema_path(files, path);
        return files;
}

static void parse_schema(gpointer key, gpointer value, gpointer user_data) {
        g_assert(key != NULL);
        g_assert(value != NULL);
        g_assert(user_data != NULL);
	parsed_schema = (const gchar *)key;

        MidgardSchema *schema = (MidgardSchema *) user_data;
        xmlDocPtr doc = (xmlDocPtr) value;
        xmlNodePtr root = xmlDocGetRootElement(doc);
        _get_element_names(root, NULL, schema);
}

/* API functions */

/* Initializes basic Midgard classes */
void midgard_schema_init(MidgardSchema *self)
{	
	g_assert(self != NULL);
	midgard_schema_read_file(self, MIDGARD_GLOBAL_SCHEMA);
}

/* Checks if classname is registsred for schema. */
gboolean midgard_schema_type_exists(MidgardSchema *self, const gchar *classname)
{
	g_assert(self != NULL);
	g_assert(classname != NULL);
	
	if(g_hash_table_lookup(self->types, classname))
		return TRUE;

	return FALSE;
}

void midgard_schema_read_file(
		MidgardSchema *schema, const gchar *filename) 
{
	g_assert(schema != NULL);
        g_assert(filename != NULL);

        GHashTable *files = read_schema_files(filename);
        g_hash_table_foreach(files, parse_schema, schema);
        g_hash_table_destroy(files);

        /* register types */
        g_hash_table_foreach(schema->types, __get_tdata_foreach, schema);
    
        /* Do postconfig for every initialized schema */
        g_hash_table_foreach(schema->types, __postconfig_schema_foreach, schema);
}

static void __is_linked_valid(MidgardObjectClass *klass, const gchar *prop)
{
	MidgardReflectionProperty *smrp = NULL;
	MidgardReflectionProperty *dmrp = NULL;
	smrp = midgard_reflection_property_new(klass);
	const gchar *typename = G_OBJECT_CLASS_NAME(klass);

	if(!midgard_reflection_property_is_link(smrp, prop)) 
		return;

	MidgardObjectClass *pklass = midgard_reflection_property_get_link_class(smrp, prop);
	
	if(!pklass) {		
		g_warning("Can not get link class for %s.%s", typename, prop);
		return;
	}
				
	const gchar *prop_link = midgard_reflection_property_get_link_target(smrp, prop);
	
	if(!prop_link) {
		g_warning("Can not get link for %s.%s", typename, prop);
		return;
	}
	
	/* We need to set is_linked for linked property. It can not be done when schema is parsed because
	 * linked class might be not registered in that phase */
	MgdSchemaPropertyAttr *prop_attr = 
		g_hash_table_lookup(pklass->dbpriv->storage_data->prophash, prop_link);
	
	if (prop_attr == NULL) {
	
		g_warning("Couldn't find property attribute for %s.%s", 
				G_OBJECT_CLASS_NAME(pklass), prop_link);
	
	} else {

		prop_attr->is_linked = TRUE;
	}

	GType src_type = midgard_reflection_property_get_midgard_type(smrp, prop);
	dmrp = midgard_reflection_property_new(pklass);
	GType dst_type = midgard_reflection_property_get_midgard_type(dmrp, prop_link);
	
	if(src_type != dst_type)
		g_warning("Mismatched references' types: %s.%s type != %s.%s type",
				typename, prop, G_OBJECT_CLASS_NAME(pklass), prop_link);

	return;
}

static void __midgard_schema_validate()
{
	guint n_types, i;
	guint n_prop = 0;
	guint j = 0;
	const gchar *typename, *parentname;
	GType *all_types = g_type_children(MIDGARD_TYPE_OBJECT, &n_types);
	MidgardObjectClass *klass = NULL;
	MidgardObjectClass *pklass = NULL;
	
	for (i = 0; i < n_types; i++) {

		typename = g_type_name(all_types[i]);
		klass = MIDGARD_OBJECT_GET_CLASS_BY_NAME(typename);

		GParamSpec **pspecs = g_object_class_list_properties(G_OBJECT_CLASS(klass), &n_prop);

		if(n_prop == 0)
			continue;

		for(j = 0; j < n_prop; j++) {

			__is_linked_valid(klass, pspecs[j]->name);
		}

		g_free(pspecs);

		parentname = klass->storage_data->parent;

		if(parentname != NULL) {
			
			/* validate tree parent class */
			pklass = MIDGARD_OBJECT_GET_CLASS_BY_NAME(parentname);
		
			if(pklass == NULL) {
				
				g_warning("Parent %s for %s class is not registered in GType system",
						parentname, typename);
			}
		
			/* validate parent property */
			const gchar *parent_property = midgard_object_class_get_parent_property(klass);
			
			if(parent_property == NULL) {
				
				g_warning("Parent property missed for %s class. %s declared as tree parent class",
						typename, parentname);
			}

			/* validate parent property if exists */
			const gchar *prop = midgard_object_class_get_property_parent(klass);	
			MidgardReflectionProperty *smrp = NULL;
	
			if(prop) {
		
				smrp = midgard_reflection_property_new(klass);
				if(!midgard_reflection_property_is_link(smrp, prop)) 
					g_warning("Parent property %s of class %s is not defined as link", prop, typename);
			
				__is_linked_valid(klass, prop);

				g_object_unref(smrp);	
			}
			
			/* validate up property link */
			prop = midgard_object_class_get_property_up(klass);
			
			if(prop) {
				
				smrp = midgard_reflection_property_new(klass);
				if(!midgard_reflection_property_is_link(smrp, prop)) 
					g_warning("Up property %s of class %s is not defined as link", prop, typename);
			
				__is_linked_valid(klass, prop);
				g_object_unref(smrp);
			}
		}
	}

	g_free(all_types);
}

gboolean midgard_schema_read_dir(
		MidgardSchema *gschema, const gchar *dirname)
{
	const gchar *fname = NULL;
	gchar *fpfname = NULL ;
	GDir *dir;
	gint visible = 0;

	dir = g_dir_open(dirname, 0, NULL);
	
	if (dir != NULL) {
		
		while ((fname = g_dir_read_name(dir)) != NULL) {

			visible = 1;
			fpfname = g_strconcat(dirname, "/", fname, NULL);
			
			/* Get files only with xml extension, 
			 * swap files with '~' suffix will be ignored */
			if (!g_file_test (fpfname, G_FILE_TEST_IS_DIR) 
					&& g_str_has_suffix(fname, ".xml")){
				
				/* Hide hidden files */
				if(g_str_has_prefix(fname, "."))
					visible = 0;
				
				/* Hide recovery files if such exist */
				if(g_str_has_prefix(fname, "#"))
					visible = 0;
				
				if ( visible == 0)
					g_warning("File %s ignored!", fpfname);
				
				if(visible == 1){  					
					/* FIXME, use fpath here */
					midgard_schema_read_file(gschema, fpfname);
				}
			}
			g_free(fpfname);
			
			/* Do not free ( or change ) fname. 
			 * Glib itself is responsible for data returned from g_dir_read_name */
		}
		g_dir_close(dir);

		/* validate */
		__midgard_schema_validate();

		return TRUE;
	}
	return FALSE;
}

/* SCHEMA DESTRUCTORS */

/* Free type names, collect all MgdSchemaTypeAttr pointers */
static void _get_schema_trash(gpointer key, gpointer val, gpointer userdata)
{
	MgdSchemaTypeAttr *stype = (MgdSchemaTypeAttr *) val;
	
	/* Collect types data pointers*/
	if(!g_slist_find(schema_trash, stype)) {
		/* g_debug("Adding %s type to trash", (gchar *)key); */ 
		schema_trash = g_slist_append(schema_trash, stype);
	}
	
	/* FIXME, key seems to be leaking, but freeing it triggers corruption */
	/* g_free(key); */
}

/* GOBJECT AND CLASS */

/* Create new object instance */
static void 
_schema_instance_init(GTypeInstance *instance, gpointer g_class)
{
	MidgardSchema *self = (MidgardSchema *) instance;
	self->types = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
}

/* Finalize  */
static void _midgard_schema_finalize(GObject *object)
{
	g_assert(object != NULL); /* just in case */
	MidgardSchema *self = (MidgardSchema *) object;	

	if(self->types != NULL) {
		g_hash_table_foreach(self->types, _get_schema_trash, NULL);
		g_hash_table_destroy(self->types);
	}
	
	for( ; schema_trash ; schema_trash = schema_trash->next){
		_mgd_schema_type_attr_free((MgdSchemaTypeAttr *) schema_trash->data);
	}
	g_slist_free(schema_trash);					
}

/* Initialize class */
static void _midgard_schema_class_init(
		gpointer g_class, gpointer g_class_data)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
	MidgardSchemaClass *klass = MIDGARD_SCHEMA_CLASS (g_class);
	
	gobject_class->set_property = NULL;
	gobject_class->get_property = NULL;
	gobject_class->finalize = _midgard_schema_finalize;
	
	klass->init = midgard_schema_init;
	klass->read_dir = midgard_schema_read_dir;
	klass->read_file = midgard_schema_read_file;
	klass->type_exists = midgard_schema_type_exists;
}

/* Register MidgardSchema type */
GType
midgard_schema_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardSchemaClass),
			NULL,           /* base_init */
			NULL,           /* base_finalize */
			(GClassInitFunc) _midgard_schema_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (MidgardSchema),
			0,              /* n_preallocs */
			_schema_instance_init    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_OBJECT,
				"midgard_schema",
				&info, 0);
	}
	return type;
}
