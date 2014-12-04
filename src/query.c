/* 
 * Copyright (C) 2005, 2007 Piotr Pokora <piotrek.pokora@gmail.com>
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

#include <config.h>
#include "midgard/midgard_schema.h"
#include <glib/gprintf.h>
#include "midgard/midgard_legacy.h"
#include "midgard/midgard_object.h"
#include "schema.h"
#include "midgard/midgard_object_property.h"
#include "midgard/midgard_reflection_property.h"
#include "midgard_mysql.h"
#include "midgard_core_query.h"
#include "midgard/midgard_metadata.h"
#include "midgard_core_object.h"
#include "midgard/midgard_timestamp.h"

/* Define Default Query String */
#define _DQS 512

/* Column types */
#define COL_TYPE_VARCHAR	"varchar(255)"
#define COL_TYPE_GUID		"varchar(80)"
#define COL_TYPE_INT		"int(11)"
#define COL_TYPE_DATETIME	"datetime"
#define COL_TYPE_BOOL		"tinyint(1)"
#define COL_TYPE_TEXT		"longtext"
#define COL_TYPE_DATE		"date"
#define COL_TYPE_FLOAT		"float"

/* static prototypes */
static gchar *__get_column_type(MidgardConnection *mgd,
		const gchar *table, const gchar *column);
static gboolean __add_table_column(MidgardConnection *mgd, 
		const gchar *table, const gchar *column, 
		const gchar *column_type, GType type);
static gboolean __add_table_index(MidgardConnection *mgd, 
		const gchar *table, const gchar *colname);
static gboolean __property_is_indexed(MidgardReflectionProperty *mrp,
		MidgardObjectClass *klass, const gchar *name);
static void __add_table_metadata(MidgardConnection *mgd, const gchar *table);

static const gchar *__get_column_type_from_midgard_type(GType type)
{
	const gchar *col_type = "varchar(255)";

	if(type == MGD_TYPE_STRING) {

		col_type = COL_TYPE_VARCHAR;
	
	} else if(type == MGD_TYPE_LONGTEXT) {

		col_type = COL_TYPE_TEXT;
	
	} else if(type == MGD_TYPE_GUID){
		
		col_type = COL_TYPE_GUID;
	
	} else if(type == MGD_TYPE_INT
			|| type ==  MGD_TYPE_UINT) {

		col_type = COL_TYPE_INT;
	
	} else if(type == MGD_TYPE_BOOLEAN){

		col_type = COL_TYPE_BOOL;
		
	} else if(type == MGD_TYPE_FLOAT) {

		col_type = COL_TYPE_FLOAT;
	
	} else if(type == MGD_TYPE_TIMESTAMP) {
		
		col_type = COL_TYPE_DATETIME;
	}

	return col_type;
}

gint midgard_query_execute(midgard *mgd, gchar *sql, gpointer user_data)
{
	g_assert(mgd->msql->mysql != NULL);
	
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", sql);
	
	gint sq = mysql_query(mgd->msql->mysql, sql);
	
	if (sq != 0) {

		g_warning("QUERY FAILED: \n %s \n QUERY: \n %s",
				mysql_error(mgd->msql->mysql), sql);
		g_free(sql);
		return -1;
	}
	g_free(sql);
	gint rows = mysql_affected_rows(mgd->msql->mysql);         

	MYSQL_RES *mres = 
		mysql_store_result(mgd->msql->mysql);
	if(mres)
		mysql_free_result(mres);

	return rows;
}

gint midgard_query_execute_with_fail(midgard *mgd, gchar *sql)
{
	g_assert(mgd->msql->mysql != NULL);
	
	g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", sql);
	
	mysql_query(mgd->msql->mysql, sql);
	g_free(sql);

	MYSQL_RES *mres =
		mysql_store_result(mgd->msql->mysql);

	if(!mres)
		return 0;

	gint rows = mysql_num_rows(mres);         

	if(mres)
		mysql_free_result(mres);

	return rows;
}

GList *midgard_query_get_single_row(gchar *sql, MgdObject *object) 
{

    g_assert(sql);
    g_assert(object);
    g_assert(object->mgd);

    midgard *mgd = object->mgd;
    GParamSpec *prop;
        
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", sql);
    
    gint sq = mysql_query(mgd->msql->mysql, sql);
    g_free(sql);        

    if (sq != 0) {
        g_warning("query failed: %s", mysql_error(mgd->msql->mysql));
        return NULL;
    }

    guint retf, i;
    
    MYSQL_RES *results = mysql_store_result(mgd->msql->mysql);
    if (!results)
        return NULL;
    if ((retf = mysql_num_fields(results)) == 0) { 
        mysql_free_result(results);  
        return NULL;                                    
    }            
        
    MYSQL_ROW row = mysql_fetch_row(results);
    MYSQL_FIELD *field;
   
    if(!row){
	    mysql_free_result(results);
	    return NULL;
    }
    
    GList *list = NULL;

    for(i = 0; i < retf; i++){
    
        if (row[i] == NULL)
            row[i] = "";

        field = mysql_fetch_field_direct(results, i);
        prop = g_object_class_find_property(
                (GObjectClass *)object->klass, field->name);

        if(prop != NULL) {

            switch (prop->value_type) {

                /*
                case G_TYPE_UINT:
                    list = g_list_prepend(list, GUINT_TO_POINTER(row[i]));
                    break;
                */    

                default:
                    list = g_list_prepend(list, g_strdup((gchar *)row[i]));
                    break;
            }
            
        } else {
            
            list = g_list_prepend(list, g_strdup((gchar *)row[i]));        
        }
    }

    list = g_list_reverse(list);
    mysql_free_result(results);

    return list;
    
}

gboolean midgard_object_set_from_query(gchar *sql, MgdObject *object) {

	g_assert(sql);
	g_assert(object);
	g_assert(object->mgd);
    
	midgard *mgd = object->mgd;
	GParamSpec *prop;
	GValue pval = {0,};	

	GString *nsql = g_string_new("");
	g_string_append(nsql, sql);
	g_string_append_printf(nsql, 
			"AND %s.sitegroup IN (0, %d)",
			midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(object)),
			midgard_sitegroup_get_id(object->mgd)
			);

    /* SG0 admin , select all */
    if (mgd_isroot(mgd)) 
        g_string_append(nsql, " AND 1=1");
    
    gchar *query = g_string_free(nsql, FALSE);
            
    g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "query=%s", query);
    
    gint sq = mysql_query(mgd->msql->mysql, query);
    g_free(query);
    g_free(sql);

    if (sq != 0) {
        g_warning("query failed: %s", mysql_error(mgd->msql->mysql));
        return FALSE;
    }

    guint ret_fields, i;
    
    MYSQL_RES *results = mysql_store_result(mgd->msql->mysql);
    
    if (!results)
        return FALSE;
    
    if ((ret_fields = mysql_num_fields(results)) == 0) {
        mysql_free_result(results);
        return FALSE;
    }

    if (mysql_num_rows(results) == 0) {
        
        mysql_free_result(results);
        return FALSE;
    }
    

    MYSQL_ROW row = mysql_fetch_row(results);
    MYSQL_FIELD *field;
    
    for(i = 0; i < ret_fields; i++){
        
        if (row[i] == NULL)
            row[i] = "";
        
        field = mysql_fetch_field_direct(results, i);
    
        if ((prop = g_object_class_find_property((GObjectClass *)object->klass, 
					field->name)) != NULL ) {
		
		g_value_init(&pval,prop->value_type);
		
		switch (prop->value_type) {
			
			case G_TYPE_STRING:
				g_value_set_string(&pval, (gchar *)row[i]);
				break;
				
			case G_TYPE_UINT:
				g_value_set_uint(&pval, atoi(row[i]));
				break;

			case G_TYPE_INT:
				g_value_set_int(&pval, atoi(row[i]));
				break;
				
			case G_TYPE_CHAR:
				g_value_set_string(&pval, row[i]);
				break;
				
			case G_TYPE_LONG:
				g_value_set_long(&pval, atol(row[i]));
				break;
				
			case G_TYPE_FLOAT:
				g_value_set_float(&pval, g_ascii_strtod(row[i], NULL));
				break;
				
			case G_TYPE_BOOLEAN:
				g_value_set_boolean(&pval, atoi(row[i]));
				break;
				
			default:
				break;
			}
		g_object_set_property(G_OBJECT(object), field->name, &pval);
		g_value_unset(&pval);
	}     
    }
   
    mysql_free_result(results);
    return TRUE;      
}

gboolean midgard_object_update_storage(MgdObject *object)
{
	g_assert(object != NULL);
	
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);
	
	return _midgard_core_query_update_class_storage(object->mgd->_mgd, klass);
}

void __remove_text_index(MidgardConnection *mgd, const gchar *table, const gchar *colname, const gchar *col_type)
{
	/* Simple rule: if column type is varchar (but not varchar(80)) or longtext, we look for existing index.
	 * If index exists, we drop it as it's wrong. */
	if(g_str_equal(col_type, COL_TYPE_VARCHAR)
			|| g_str_equal(col_type, COL_TYPE_TEXT)
			|| g_str_equal(col_type, "varchar(100)")) {

		GString *sql = g_string_new("SHOW KEYS FROM");
		g_string_append_printf(sql, " %s WHERE Column_name='%s'", 
				table, colname );

		midgard_res *res = mgd_vquery(mgd->mgd, sql->str, NULL);
		g_string_free(sql, TRUE);

		if(!res)
			return;

		if(res && !mgd_fetch(res)) {
			
			mgd_release(res);
			return;
		}

		sql = g_string_new("ALTER TABLE");
		g_string_append_printf(sql, " %s DROP INDEX %s", 
				table, mgd_colvalue(res, 2));

		midgard_query_execute(mgd->mgd,
				g_string_free(sql, FALSE), NULL);

		mgd_release(res);

		return;
	}
}

void __remove_tablecol_index(MidgardConnection *mgd, const gchar *table, const gchar *colname)
{
	GString *ois = g_string_new("");
	g_string_append_printf(ois, "%s_%s_idx", table, colname);

	GString *sql = g_string_new("SHOW KEYS FROM");
	g_string_append_printf(sql, " %s WHERE Key_name='%s'", table, ois->str);

	g_string_free(ois, TRUE);

	midgard_res *res = mgd_vquery(mgd->mgd, sql->str, NULL);
	g_string_free(sql, TRUE);

	if(!res)
		return;
	
	if(res && !mgd_fetch(res)) {
		
		mgd_release(res);
		return;
	}

	sql = g_string_new("ALTER TABLE");
	g_string_append_printf(sql, " %s DROP INDEX %s", 
			table, mgd_colvalue(res, 2));
	
	midgard_query_execute(mgd->mgd,	g_string_free(sql, FALSE), NULL);

	mgd_release(res);
	
	return;
}

gboolean _midgard_core_query_update_class_storage(
		MidgardConnection *mgd, MidgardObjectClass *klass)
{
	g_assert(mgd != NULL);
	g_assert(klass != NULL);
	
	const gchar *table = midgard_object_class_get_table(klass);
	const gchar *primary_property = 
		midgard_object_class_get_primary_property(klass);
	if(table == NULL)
		return FALSE;

	/* Check table collation. See ticket #1292 */
	const gchar *database = (const gchar *) mgd->priv->config->database;
	GString *query = g_string_new ("SELECT TABLE_COLLATION FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = "); 
	g_string_append_printf (query, " '%s' AND TABLE_NAME='%s'", database, table);

	midgard_res *res = mgd_vquery (mgd->mgd, query->str, NULL);
        g_string_free (query, TRUE);

	if (!res || !mgd_fetch (res)) {
		
		if (res) mgd_release (res);
		return FALSE;
	}

	gchar *collation = NULL;

	if (res) {

		collation = g_strdup ((gchar *)mgd_colvalue (res, 0));
		mgd_release(res);
	}

	if (collation) {	

		if (!g_str_has_prefix (collation, "utf8"))
			g_warning ("Table '%s' has invalid (%s) character set.", table, collation); 

		g_free (collation);
	}


	guint propn;
	gint i;
	GType prop_type;
	gchar **real_field = NULL;
	MgdSchemaPropertyAttr *prop_attr;
	GParamSpec **props = 
		g_object_class_list_properties(
				G_OBJECT_CLASS(klass), &propn);
	MidgardReflectionProperty *mrp = 
		midgard_reflection_property_new(klass);

	const gchar *col_type;

	for(i = 0 ; i < propn; i++){
		prop_type = 
			midgard_reflection_property_get_midgard_type(
					mrp, props[i]->name);
		prop_attr = g_hash_table_lookup(
				klass->storage_data->prophash, props[i]->name);
		/* Get table.column value from nick */
		const gchar *nick =  g_param_spec_get_nick (props[i]);
		if ((strlen(nick)) < 1) 
			continue;
		
		real_field = g_strsplit(nick, ".", 2);
				
		GString *alter_cmd = g_string_new("ALTER TABLE ");
		g_string_append_printf(
				alter_cmd,
				" %s MODIFY ",
				real_field[0]);

		/* dbtype attribute found , table altered with its value */
		if((prop_attr != NULL) && (prop_attr->dbtype != NULL)) {
			
			g_string_append_printf(
					alter_cmd, "%s %s ", 
					real_field[1],
					prop_attr->dbtype
					);
			col_type = (gchar *)prop_attr->dbtype;

		} else {
			
			if(prop_type == MGD_TYPE_STRING){
				g_string_append_printf(
						alter_cmd,
						"%s varchar(255) NOT NULL "
						"default ''",
						real_field[1]);
				col_type = COL_TYPE_VARCHAR;

			} else if(prop_type == MGD_TYPE_LONGTEXT){
				g_string_append_printf(
						alter_cmd,
						"%s longtext NOT NULL "
						"default ''",
						real_field[1]);
				col_type = COL_TYPE_TEXT;

			} else if(prop_type == MGD_TYPE_INT
					|| prop_type ==  MGD_TYPE_UINT){
				g_string_append_printf(
						alter_cmd,
						"%s int(11) NOT NULL ",
						real_field[1]);
				col_type = COL_TYPE_INT;
				if(g_str_equal(props[i]->name, primary_property))
					g_string_append(alter_cmd,
							"auto_increment");
				else 
					g_string_append(alter_cmd,
							"default 0");

			} else if(prop_type == MGD_TYPE_FLOAT) {
				g_string_append_printf(
						alter_cmd,
						"%s FLOAT NOT NULL "
						"default '0.0000'",
						real_field[1]);
				col_type = COL_TYPE_FLOAT;

			} else if(prop_type == MGD_TYPE_BOOLEAN) {
				g_string_append_printf(
						alter_cmd,
						"%s BOOLEAN NOT NULL "
						"default 0",
						real_field[1]);
				col_type = COL_TYPE_BOOL;

			} else if(prop_type == MGD_TYPE_TIMESTAMP) {
				g_string_append_printf(
						alter_cmd,
						"%s datetime NOT NULL "
						"default '0000-00-00 00:00:00'",
						real_field[1]);
				col_type = COL_TYPE_DATETIME;

			} else if(prop_type == MGD_TYPE_GUID){ 
				g_string_append_printf(
						alter_cmd,
						"%s varchar(80) NOT NULL "
						"default ''",
						real_field[1]);
				col_type = COL_TYPE_GUID;
			} else {
				g_string_append_printf(
						alter_cmd,
						"%s varchar(255) NOT NULL "
						"default ''",
						real_field[1]);
				col_type = COL_TYPE_VARCHAR;
			}
		}
		
		gchar *db_col_type = 
			__get_column_type(mgd, real_field[0], real_field[1]);

		gint rv = 0;
		gboolean _created;
		
		/* column doesn't exist, we need to create it */
		if(db_col_type == NULL) {

			_created =  __add_table_column(mgd,
					real_field[0], real_field[1],
					col_type, prop_type);
			
			if(!_created)
				rv = -1;
		
		} else if(!g_str_equal(db_col_type, col_type)) {
		
			__remove_text_index(mgd, real_field[0], real_field[1], db_col_type);
			__remove_tablecol_index(mgd, real_field[0], real_field[1]);
			rv = midgard_query_execute(mgd->mgd, 
					g_string_free(alter_cmd, FALSE), NULL);
		
		} else {
			
			rv = 0;
		}

		/* CREATE INDEX */
		if(__property_is_indexed(mrp, klass, props[i]->name) 
				|| prop_type == MGD_TYPE_GUID) 
			__add_table_index(mgd, real_field[0], real_field[1]);
		
		g_strfreev(real_field);
		g_free(db_col_type);

		if(rv < 0){

			g_free(props);
			g_object_unref(mrp);
			return FALSE;
		}
	}

	g_object_unref(mrp);
	g_free(props);

	__add_table_metadata(mgd, table);

	return TRUE;
}

gboolean midgard_object_create_storage(MgdObject *object)
{
	g_assert(object != NULL);

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);

	return _midgard_core_query_create_class_storage(object->mgd->_mgd, klass);
}

gboolean __table_exists(MidgardConnection *mgd, const gchar *table)
{
	GString *cmd = g_string_new("SHOW CREATE TABLE ");
	g_string_append(cmd, table);

	gint rows = 
		midgard_query_execute_with_fail(mgd->mgd, g_string_free(cmd, FALSE));

	if(rows < 1)
		return FALSE;

	return TRUE;
}

static gchar *__get_column_type(MidgardConnection *mgd, 
		const gchar *table, const gchar *column)
{
	GString *cmd = g_string_new("SHOW COLUMNS FROM ");
	g_string_append_printf(cmd,
			"%s WHERE Field='%s'",
			table, column);

	midgard_res *res = mgd_vquery(mgd->mgd, cmd->str, NULL);

	g_string_free(cmd, TRUE);

	if (!res || !mgd_fetch(res)) {
		
		if(res) mgd_release(res);
		return NULL;
	}
	
	gchar *column_type = NULL;

	if(res) {
		
		column_type = g_strdup((gchar *)mgd_colvalue(res, 1));
		mgd_release(res);
	}

	return column_type;
}

static gboolean __property_is_indexed(MidgardReflectionProperty *mrp, 
		MidgardObjectClass *klass, const gchar *name)
{
	if(g_str_equal("sitegroup", name))
		return TRUE;

	if(midgard_reflection_property_is_link(mrp, name))
		return TRUE;

	if(midgard_reflection_property_is_linked(mrp, name))
		return TRUE;

	const gchar *parent = 
		midgard_object_class_get_parent_property(klass);
	
	if(parent && g_str_equal(parent, name))
		return TRUE;

	const gchar *up = 
		midgard_object_class_get_up_property(klass);
	if(up && g_str_equal(up, name))
		return TRUE;

	if(midgard_object_class_is_multilang(klass)) {
		
		if(g_str_equal("sid", name))
			return TRUE;
	}

	MgdSchemaPropertyAttr *prop_attr;
	prop_attr = g_hash_table_lookup(klass->dbpriv->storage_data->prophash, name);
	if(prop_attr != NULL && prop_attr->dbindex == TRUE)
		return TRUE;

	return FALSE;
}

static gchar *__index_name(const gchar *table, const gchar *colname)
{
	return g_strconcat(colname, "_idx", NULL);	
}

gboolean __table_index_exists(MidgardConnection *mgd, const gchar *table, const gchar *name)
{
	g_assert(mgd != NULL);
	gboolean index_exists = FALSE;
	
	GString *cmd = g_string_new("SHOW KEYS FROM ");
	g_string_append_printf(cmd, 
			"%s WHERE Key_name='%s'",
			table, name);

	midgard_res *res = mgd_vquery(mgd->mgd, cmd->str, NULL);
	g_string_free(cmd, TRUE);

	if (!res) 
		return FALSE;


	mgd_release(res);

	return TRUE;

	while(mgd_fetch(res)) {
	
		
		if(g_str_equal(name, (gchar *)mgd_colvalue(res, 2))) {
		
			index_exists = TRUE;
			break;
		}
	}

	if(res)
		mgd_release(res);

	return index_exists;
}

static gboolean __add_table_index(MidgardConnection *mgd, const gchar *table, const gchar *colname)
{

	if(g_str_equal("id", colname)) {
		
		g_debug("Ignore index on id");
		return TRUE;
	}

	gboolean added = FALSE;
	gboolean idx_exists = FALSE;

	gchar *index_name = __index_name(table, colname);

	idx_exists = __table_index_exists(mgd, table, index_name);

	if(idx_exists) {
		g_debug("INDEX '%s' exists", index_name);
		g_free(index_name);
		return TRUE;
	}

	GString *cmd = g_string_new("CREATE INDEX ");
	g_string_append_printf(cmd, "%s ON %s (%s)",
			index_name, table, colname);

	gint rv = midgard_query_execute(
			mgd->mgd, cmd->str, NULL);
	g_string_free(cmd, FALSE);
	g_free(index_name);
	
	if(rv == -1)
		added = FALSE;

	return added;
}

static void __add_metadata_indexes(MidgardConnection *mgd, const gchar *table)
{
	__add_table_index(mgd, table, "metadata_creator");
	__add_table_index(mgd, table, "metadata_created");
	__add_table_index(mgd, table, "metadata_revisor");
	__add_table_index(mgd, table, "metadata_revised");
	__add_table_index(mgd, table, "metadata_approved");
	__add_table_index(mgd, table, "metadata_owner");
	__add_table_index(mgd, table, "metadata_schedule_start");
	__add_table_index(mgd, table, "metadata_schedule_end");
	__add_table_index(mgd, table, "metadata_published");
	__add_table_index(mgd, table, "metadata_exported");
	__add_table_index(mgd, table, "metadata_score");
}

gboolean __add_table_column(MidgardConnection *mgd, const gchar *table,
		const gchar *column, const gchar *column_type, GType type)
{
	GString *cmd = g_string_new("ALTER TABLE ");
	g_string_append_printf(cmd, "%s ADD ", table);        
	
	
	if(type == 0) {
			g_string_append_printf(cmd, 
					"%s %s ",
					column, column_type);
	} else {
		
		g_string_append_printf(cmd, 
				"%s %s NOT NULL ",
				column, column_type);
	}
	
	if(type == MGD_TYPE_STRING 
			|| type == MGD_TYPE_LONGTEXT
			|| type == MGD_TYPE_GUID){

		g_string_append(cmd, "default ''");
		
		
	} else if(type == MGD_TYPE_INT
			|| type ==  MGD_TYPE_UINT
			|| type == MGD_TYPE_BOOLEAN){
		
		g_string_append(cmd, "default 0");
				
	} else if(type == MGD_TYPE_FLOAT) {
		
		g_string_append(cmd, "default '0.0000'");
		
	} else if(type == MGD_TYPE_TIMESTAMP) {
		
		g_string_append(cmd, "default '0000-00-00 00:00:00'");
				
	}
	
	gint rv = midgard_query_execute(
			mgd->mgd, g_string_free(cmd, FALSE), NULL);

	if(rv == -1)
		return FALSE;

	return TRUE;
}

static gboolean __table_column_exists(MidgardConnection *mgd, const gchar *table, const gchar *colname)
{
	gchar *db_col_type =
		__get_column_type(mgd, table, colname);

	if(db_col_type == NULL)
		return FALSE;

	g_free(db_col_type);
	return TRUE;
}

static void __add_table_metadata(MidgardConnection *mgd, const gchar *table)
{
	/* Add metadata columns */
	/* FIXME , change this when MIDGARD_TYPE_TIMESTAMP is fully supported */
	
	GParamSpec **props;
	GString *cmd;
	guint i = 0, metapropn = 0;
	MidgardMetadataClass *mklass = 
		g_type_class_peek_static(MIDGARD_TYPE_METADATA);
	props = g_object_class_list_properties(
			G_OBJECT_CLASS(mklass), &metapropn);
	const gchar *meta_field;	
	for(i = 0 ; i < metapropn; i++){
		
		meta_field =  g_param_spec_get_nick (props[i]);
		
		if(g_str_equal(props[i]->name, "creator") ||
				g_str_equal(props[i]->name, "revisor") ||
				g_str_equal(props[i]->name, "approver") ||
				g_str_equal(props[i]->name, "locker") ||
				g_str_equal(props[i]->name, "owner")) {
			cmd = g_string_new("ALTER table ");
			g_string_append_printf(cmd, 
					"%s ADD (%s "
					"varchar(80) NOT NULL default '')",
					table, meta_field);
			if(!__table_column_exists(mgd, table, meta_field))
				midgard_query_execute(mgd->mgd, 
						g_string_free(cmd, FALSE), NULL);
		}
		
		if(g_str_equal(props[i]->name, "authors")) {
			cmd = g_string_new("ALTER table ");
			g_string_append_printf(cmd, 
					"%s ADD (%s "
					"longtext NOT NULL default '')",
					table, meta_field);
			if(!__table_column_exists(mgd, table, meta_field))
				midgard_query_execute(mgd->mgd, 
						g_string_free(cmd, FALSE), NULL);
		}
		
		if(g_str_equal(props[i]->name, "created") ||
				g_str_equal(props[i]->name, "revised") ||
				g_str_equal(props[i]->name, "approved") ||
				g_str_equal(props[i]->name, "locked") ||
				g_str_equal(props[i]->name, "schedulestart") ||
				g_str_equal(props[i]->name, "scheduleend") ||
				g_str_equal(props[i]->name, "published") ||
				g_str_equal(props[i]->name, "exported") ||
				g_str_equal(props[i]->name, "imported")) { 
			cmd = g_string_new("ALTER table ");
			g_string_append_printf(cmd,
					"%s ADD (%s "
					"datetime NOT NULL default '0000-00-00 00:00:00')",
					table, meta_field);
			if(!__table_column_exists(mgd, table, meta_field))
				midgard_query_execute(mgd->mgd, 
						g_string_free(cmd, FALSE), NULL);
		}
		
		if(g_str_equal(props[i]->name, "hidden") ||
				g_str_equal(props[i]->name, "navnoentry") ||
				g_str_equal(props[i]->name, "deleted") ||
				g_str_equal(props[i]->name, "islocked") ||
				g_str_equal(props[i]->name, "isapproved")) {			
			cmd = g_string_new("ALTER table ");
			g_string_append_printf(cmd,
					"%s ADD (%s "
					"BOOL default 0)",
					table, meta_field);
			if(!__table_column_exists(mgd, table, meta_field))
				midgard_query_execute(mgd->mgd,
						g_string_free(cmd, FALSE), NULL);
		}
		
		if(g_str_equal(props[i]->name, "size") ||
				g_str_equal(props[i]->name, "revision") ||
				g_str_equal(props[i]->name, "score")) {
			cmd = g_string_new("ALTER table ");
			g_string_append_printf(cmd,
					"%s ADD (%s "
					"int(11) NOT NULL default '0')",
					table, meta_field);
			if(!__table_column_exists(mgd, table, meta_field))
				midgard_query_execute(mgd->mgd,
						g_string_free(cmd, FALSE), NULL);
		}
	}   
	g_free(props);
	__add_metadata_indexes(mgd, table);
}

gboolean _midgard_core_query_table_exists(
		MidgardConnection *mgd, const gchar *table)
{
	return __table_exists(mgd, table);
}

gboolean _midgard_core_query_create_class_storage(
		                MidgardConnection *mgd, MidgardObjectClass *klass)
{
	GString *cmd, *cmd_modify;
	const gchar *pfield = midgard_object_class_get_primary_field(klass);
	const gchar *primary_property = midgard_object_class_get_primary_property(klass);
	const gchar *table = midgard_object_class_get_table(klass);
	
	if(table == NULL)
		return FALSE;
	
	GParamSpec *prop, **props;
	gint rv;
	guint propn, i = 0;
	gchar **real_field = NULL;
	GType prop_type;
	gboolean create_table;
	
	/* CREATE TABLES and PRIMARY KEY */
	gchar **tables = g_strsplit(
			midgard_object_class_get_tables(klass), ",", 0);
	
	do {	
		create_table = TRUE;

		if(__table_exists(mgd, (const gchar *)tables[i])) {
			
			g_debug("Table %s exists.", tables[i]);
			create_table = FALSE;
		}

		cmd = g_string_new("CREATE TABLE IF NOT EXISTS ");
		g_string_append(cmd, tables[i]);
		
		/* Get primary field used by object to define PRIMARY KEY in 
		 * newly created table. If no field is defined we use guid */
		
		g_string_append_printf(cmd , "(%s ", pfield);
		
		if ((prop = g_object_class_find_property(
						G_OBJECT_CLASS(klass), 
						primary_property)) != NULL ) {

			switch (prop->value_type){				
				case G_TYPE_STRING:
					g_string_append_printf(cmd , "varchar(80) NOT NULL");
					break;
				
				case G_TYPE_UINT:
					g_string_append_printf(cmd,
							"int(%u) NOT NULL auto_increment", 11);
			}
		}
		
		g_string_append_printf(cmd , ", PRIMARY KEY(%s)) ", pfield);
		g_string_append(cmd, " CHARACTER SET utf8 ");
		
		if(create_table)
			rv = midgard_query_execute(mgd->mgd, g_string_free(cmd, FALSE), NULL);
		else 
			rv = 0;

		if (rv == -1){

			g_warning("Table %s not created", table);
			i++;
			return FALSE;

		} else {
			g_message("Table '%s' OK", tables[i]);

			/* Add sitegroup column */
			cmd_modify = g_string_new("ALTER TABLE ");
			g_string_append_printf(cmd_modify,
					"%s ADD sitegroup int(11) NOT NULL",  tables[i]);
			
			if(create_table)
				midgard_query_execute(mgd->mgd, 
						g_string_free(cmd_modify, FALSE), NULL);
		
			/* Add key for sitegroup column */
			cmd_modify = g_string_new("ALTER TABLE ");
			g_string_append_printf(cmd_modify, 
					"%s ADD KEY(sitegroup)", tables[i]);
			
			if(create_table)
				midgard_query_execute(mgd->mgd, 
						g_string_free(cmd_modify, FALSE), NULL);
			
			i++;
		}
		
	} while (tables[i]);

	g_strfreev(tables);

	if(!create_table)
		goto CREATED_AND_RETURN;
    
	/* CREATE columns for properties */
	props = g_object_class_list_properties(
			G_OBJECT_CLASS(klass), &propn);
	MidgardReflectionProperty *mrp =
		midgard_reflection_property_new(klass);
	
	for(i = 0 ; i < propn; i++){
		MgdSchemaPropertyAttr *prop_attr =
			g_hash_table_lookup(klass->storage_data->prophash, props[i]->name);
		if(!prop_attr)
			continue;
		
		/* Do not modify table if property is primary one */
		if (g_ascii_strcasecmp(props[i]->name, prop->name) == 0)
			continue;
		
		prop_type =
			midgard_reflection_property_get_midgard_type(
					mrp, props[i]->name);
		
		const gchar *nick =  g_param_spec_get_nick (props[i]);
		/* Property inherited from parent base midgard class, 
		 * do not create fields */
		if ((strlen(nick)) < 1) 
			continue;

		/* Get table and field */
		real_field = g_strsplit(nick, ".", 2);   

		const gchar *col_type;
		
		if(prop_attr->dbtype != NULL) {

			col_type = prop_attr->dbtype;
		
		} else {
			
			col_type = __get_column_type_from_midgard_type(prop_type);
		}

		gboolean _added = 
			__add_table_column(mgd, (const gchar *)real_field[0],
					(const gchar *) real_field[1], 
					col_type, prop_type);

		if(_added)
			g_message("Column '%s' OK", real_field[1]);	
		else 
			g_warning("Failed to create '%s.%s' column",
					real_field[0], real_field[1]);

		g_strfreev(real_field);
	}
	g_free(props);
	
	/* Add guid column */
	cmd_modify = g_string_new("ALTER TABLE ");
	g_string_append_printf(cmd_modify,
			"%s ADD guid varchar(80) NOT NULL",  table);
	rv = midgard_query_execute(mgd->mgd, g_string_free(cmd_modify, FALSE), NULL);        
	
	/* Add key for guid column */
	__add_table_index(mgd, table, "guid");
	/*cmd_modify = g_string_new("ALTER TABLE ");
	g_string_append_printf(cmd_modify, "%s ADD KEY(guid)", table);
	rv = midgard_query_execute(mgd->mgd, g_string_free(cmd_modify, FALSE), NULL);
	*/
	
	/* Add key for parent and up columns */
	/*
	if (midgard_object_class_get_parent_property(klass)) {
		cmd_modify = g_string_new("ALTER TABLE ");
		g_string_append_printf(cmd_modify, "%s ADD KEY(%s)", 
				table, midgard_object_class_get_parent_property(klass));
		rv = midgard_query_execute(mgd->mgd, g_string_free(cmd_modify, FALSE), NULL); 
	}

	if (midgard_object_class_get_up_property(klass)) {
		cmd_modify = g_string_new("ALTER TABLE ");
		g_string_append_printf(cmd_modify, "%s ADD KEY(%s)",
				table, midgard_object_class_get_up_property(klass));
		rv = midgard_query_execute(mgd->mgd, g_string_free(cmd_modify, FALSE), NULL);
	}
	*/

	__add_table_metadata(mgd, table);	
	
CREATED_AND_RETURN:
	g_message("Table and columns for '%s' OK", G_OBJECT_CLASS_NAME(klass));
	return TRUE;   
}

gboolean _midgard_core_query_create_basic_db(MidgardConnection *mgd)
{
	g_assert(mgd != NULL);

	/* Internal tables hardcoded */
	
	/* TABLE repligard */
	gchar *rep_table = g_strdup("CREATE TABLE IF NOT EXISTS repligard ( 		\
		id int(11) NOT NULL default '0',		\
		guid varchar(80) NOT NULL default '',		\
		sitegroup int(11) NOT NULL default '0',		\
		typename varchar(80) NOT NULL default '',	\
		lang int(11) NOT NULL default '0',		\
		object_action int(11) NOT NULL default '0',	\
		object_action_date datetime NOT NULL default '0000-00-00 00:00:00', \
		realm varchar(255) NOT NULL default '',		\
		changed timestamp NOT NULL,			\
		updated timestamp NOT NULL,			\
		action enum('create','update','delete') NOT NULL default 'create',	\
		PRIMARY KEY (guid, sitegroup) \
		);");

	/* TABLE sitegroup */
	gchar *sg_table = g_strdup("CREATE TABLE IF NOT EXISTS sitegroup (		\
		id int(11) NOT NULL auto_increment,		\
		name varchar(255) NOT NULL default '',		\
		realm varchar(255) NOT NULL default '',		\
		admingroup int(11) NOT NULL default '0',	\
		sitegroup int(11) NOT NULL default '0',		\
		guid varchar(80) NOT NULL default '',           \
		metadata_created datetime NOT NULL default '0000-00-00 00:00:00', \
		metadata_revised datetime NOT NULL default '0000-00-00 00:00:00', \
		metadata_deleted BOOL NOT NULL default 0,	\
		PRIMARY KEY (id),				\
		KEY sitegroup_sitegroup_idx(sitegroup)		\
		);");

	
	/* TODO: Add person table */

	gint rv = -1;
	rv = midgard_query_execute(mgd->mgd, rep_table, NULL);

	if(rv == -1) {
		g_warning("Failed to create repligard table");
		return FALSE;
	}

	rv = midgard_query_execute(mgd->mgd, sg_table, NULL);

	if(rv == -1) {
		g_warning("Failed to create sitegroup table");
		return FALSE;
	}

	return TRUE;
}

#define __ADD_COMA(__i, __str) \
	if(__i > 0) g_string_append(__str, ", ");

gboolean _midgard_core_query_update_object_fields(MgdObject *object, const gchar *field, ...)
{
	g_assert(object != NULL);
	
	const gchar *name = field;
	va_list var_args;
	va_start(var_args, field);
	GValue *value_arg;
	
	GString *query = g_string_new("UPDATE ");

	const gchar *table = 
		midgard_object_class_get_table(MIDGARD_OBJECT_GET_CLASS(object));

	g_string_append_printf(query, "%s SET ", table);

	guint i = 0;

	while(name != NULL){

		value_arg = va_arg(var_args, GValue*);

		switch (G_TYPE_FUNDAMENTAL(G_VALUE_TYPE(value_arg))) {
			
			case G_TYPE_STRING:
				__ADD_COMA(i, query);
				g_string_append_printf(query, 
						"%s = '%s' ",
						name, g_value_get_string(value_arg));
				break;

			case G_TYPE_UINT:
				__ADD_COMA(i, query);
				g_string_append_printf(query,
						"%s = %d ",
						name, g_value_get_uint(value_arg));
				break;

			case G_TYPE_INT:
				__ADD_COMA(i, query);
				g_string_append_printf(query,
						"%s = %d ",
						name, g_value_get_int(value_arg));
				break;

			case G_TYPE_BOOLEAN:
				__ADD_COMA(i, query);
				g_string_append_printf(query,
						"%s = %d ",
						name, g_value_get_boolean(value_arg));

				break;
		}

		name = va_arg(var_args, gchar*);

		i++;
	}
	
	va_end(var_args);
	
	MidgardConnection *mgd = MGD_OBJECT_CNC(object);
	
	g_string_append_printf(query, 
			" WHERE guid = '%s' AND sitegroup = %d",
			MGD_OBJECT_GUID(object), MGD_OBJECT_SG(object));

	guint rv = mgd_exec(mgd->mgd, query->str, NULL);

	g_string_free(query, TRUE);
	
	if(rv == 0)
		return FALSE;
	
	return TRUE;
}

gboolean 
_midgard_core_query_create_repligard (MidgardConnection *mgd, const gchar *guid, guint id,
		const gchar *typename, const gchar *action_date, guint action, guint lang)
{
	g_return_val_if_fail (mgd != NULL, FALSE);
	g_return_val_if_fail (guid != NULL, FALSE);

	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS_BY_NAME (typename);
	const gchar *table = midgard_object_class_get_table (klass);

	GValue tval;

	if (action_date == NULL)  {
		tval = midgard_timestamp_current_as_value (G_TYPE_STRING);
	} else {
		g_value_init (&tval, G_TYPE_STRING);
		g_value_set_string (&tval, action_date);
	}

	GString *query = g_string_new ("INSERT INTO repligard "
			"('guid', 'id', 'typename', 'object_action_date', 'object_action', 'lang' "
		        "'action', 'realm' ) ");

	g_string_append_printf (query,
			"VALUES ('%s', %d, '%s', '%s', %d, %d, 'create', '%s')",
			guid, id, typename, g_value_get_string (&tval), action, lang,
			table);

	g_value_unset (&tval);

	g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Repligard record create=%s", query->str);
	guint qr = mysql_query (mgd->mgd->msql->mysql, query->str);
	g_string_free (query, TRUE);

	if (qr != 0) {
		g_warning ("Repligard query insert failed: %s", mysql_error (mgd->mgd->msql->mysql));
		return FALSE;
	}

	return TRUE;
}

gboolean 
_midgard_core_query_update_repligard (MidgardConnection *mgd, const gchar *guid, guint id,
		const gchar *typename, const gchar *action_date, guint action, guint lang)
{
	g_return_val_if_fail (mgd != NULL, FALSE);
	g_return_val_if_fail (guid != NULL, FALSE);

	GValue tval;

	if (action_date == NULL)  {
		tval = midgard_timestamp_current_as_value (G_TYPE_STRING);
	} else {
		g_value_init (&tval, G_TYPE_STRING);
		g_value_set_string (&tval, action_date);
	}

	const gchar *legacy_action = "";
	switch (action) {
		case MGD_OBJECT_ACTION_NONE:
			legacy_action = "";
			break;
		case MGD_OBJECT_ACTION_DELETE:
		case MGD_OBJECT_ACTION_PURGE:
			legacy_action = "delete";
			break;
		case MGD_OBJECT_ACTION_CREATE:
			legacy_action = "create";
			break;
		case MGD_OBJECT_ACTION_UPDATE:
			legacy_action = "update";
			break;
	}

	GString *query = g_string_new ("UPDATE repligard SET ");
	g_string_append_printf (query,
			"object_action = %d, object_action_date = '%s', "
			"action = '%s'",
			action, g_value_get_string (&tval), legacy_action);
	g_string_append_printf (query, 
			" WHERE guid = '%s' AND lang = %d ",
			guid, lang);

	g_value_unset (&tval);

	g_log (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, "Repligard record update=%s", query->str);
	guint qr = mysql_query (mgd->mgd->msql->mysql, query->str);
	g_string_free (query, TRUE);

	if (qr != 0) {
		g_warning ("Repligard query update failed: %s", mysql_error (mgd->mgd->msql->mysql));
		return FALSE;
	}

	return TRUE;
}
