/* 
 * Copyright (C) 2005 Jukka Zitting <jz@yukatan.fi>
 * Copyright (C) 2005 Piotr Pokora <piotr.pokora@nemein.net>
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
#include "simple_constraint.h"
#include "midgard/midgard_object.h"
#include "midgard/midgard_metadata.h"
#include "midgard_mysql.h"
#include <string.h>
#include <stdlib.h>

static const char *operator[] = {
        "=", ">", "<", "<>", "<=", ">=", "LIKE", "NOT LIKE", "IN", "NOT IN", "INTREE",
        NULL
};

static void add_sql_value(
                MidgardSimpleConstraint *constraint, GValue *value, GString *sql) {
        g_assert(constraint != NULL);
        g_assert(value != NULL);
        g_assert(sql != NULL);

        GValue *target = g_new0(GValue, 1);
        g_value_init(target, G_PARAM_SPEC_VALUE_TYPE(constraint->spec));

        /* Convert the value to the correct type. */
	if (!g_param_value_convert((GParamSpec *)constraint->spec, value, target, FALSE)) {
		/* The default conversions failed, try custom conversions */
		if (G_VALUE_HOLDS_STRING(value) && G_VALUE_HOLDS_UINT(target)) {
			guint uvalue = strtoul(g_value_get_string(value), NULL, 0);
			g_value_set_uint(target, uvalue);
		} else {
			g_warning(
					"Invalid value conversion from %s to %s",
					G_VALUE_TYPE_NAME(value),
					G_VALUE_TYPE_NAME(target));
		}
	}

        /* Format the value for the SQL statement */
        if (G_VALUE_HOLDS_STRING(target)) {
                const gchar *strval = g_value_get_string(target);
		if(strval == NULL)
			strval = "";
                guint length = strlen(strval);
                gchar *escaped = g_new(gchar, 2 * length + 1);
                /* TODO: Use libgda value escaping */
                mysql_real_escape_string(
                        constraint->mgd->msql->mysql, escaped, strval, length);
                g_string_append_printf(sql, "'%s'", escaped);
                g_free(escaped);
        } else if (G_VALUE_HOLDS_BOOLEAN(target)) {
                if (g_value_get_boolean(target)) {
                        g_string_append(sql, "TRUE");
                } else {
                        g_string_append(sql, "FALSE");
                }
        } else if (G_VALUE_HOLDS_UINT(target)) {
                g_string_append_printf(sql, "%u", g_value_get_uint(target));
	} else if (G_VALUE_HOLDS_INT(target)) {
		g_string_append_printf(sql, "%d", g_value_get_int(target));
	} else if (G_VALUE_HOLDS_FLOAT(target)) {
                g_string_append_printf(sql, "%g", g_value_get_float(target));
        } else {
                g_warning(
                        "Unexpected constraint value type %s",
                        G_VALUE_TYPE_NAME(target));
                g_string_append(sql, "NULL");
        }

        g_value_unset(target);
        g_free(target);
}

static void add_sql(MidgardQueryConstraint *constraint, GString *sql) {
        g_assert(constraint);
        g_assert(sql);

        MidgardSimpleConstraint *simple = MIDGARD_SIMPLE_CONSTRAINT(constraint);
	guint i;

	const gchar *table = midgard_object_class_get_table(
			(MidgardObjectClass *)simple->initial_klass);

	/* Dereference a parametrized value pointer */
	GValue *value = simple->value;
	if (G_VALUE_HOLDS_POINTER(value)
			&& G_IS_VALUE(g_value_get_pointer(value))) {
		value = (GValue *) g_value_get_pointer(value);
	}	
	
	/* FIXME, MGD_TYPE_SITEGROUP and MGD_TYPE_GUID should be implemented */
	/* Get table.sitegroup */
	if(g_ascii_strcasecmp(simple->spec->name, "sitegroup") == 0){
		gchar *sgfield = g_strconcat(
				midgard_object_class_get_table((MidgardObjectClass *)simple->klass), 
				".sitegroup",
				NULL);
		g_string_append(sql, sgfield);
		g_free(sgfield);
	/* Get table.guid */	
	} else if (g_ascii_strcasecmp(simple->spec->name, "guid") == 0){
		gchar *sgfield = g_strconcat(
				midgard_object_class_get_table((MidgardObjectClass *)simple->klass),
				".guid",
				NULL);
		g_string_append(sql, sgfield);
		g_free(sgfield);  
	} else if (simple->ext_type == MIDGARD_TYPE_METADATA) { 
		MidgardObjectClass *pklass = 
			(MidgardObjectClass*) g_type_class_peek(simple->parent_type);
		if(pklass)
			table = midgard_object_class_get_table(pklass);
		g_string_append_printf(sql, 
				"%s.%s",
				table,
				g_param_spec_get_nick((GParamSpec *) simple->spec));				
	} else {
		g_string_append(sql, g_param_spec_get_nick((GParamSpec *) simple->spec));
	}

	g_string_append_c(sql, ' ');
	if(g_str_equal(simple->op, "INTREE")){
		
		gint is_integer = 2;
		if(!G_VALUE_HOLDS(value, G_TYPE_UINT)){
			is_integer--;
		}
		if(!G_VALUE_HOLDS(value, G_TYPE_INT)){
			is_integer--;
		}

		if(is_integer == 0){
			g_warning(" Only integers values supported by INTREE operator");
			return;
		}
		/* This comparison is done to avoid ugly endless loops for 
		 * properties which are not parent or up ones 
		 * This should be changed when midgard_tree is implemented */
		const gchar *parent_field, *up_field = NULL;
		guint j = 0;
		parent_field = 
			midgard_object_class_get_parent_property(
					(MidgardObjectClass *)simple->klass);
		if(parent_field == NULL)
			parent_field = "";
		up_field =
				midgard_object_class_get_up_property(
						(MidgardObjectClass *)simple->klass);	
		if(up_field == NULL)
			up_field = "";
		/*if(!g_str_equal(parent_field, simple->spec->name))
			j++;
		if(!g_str_equal(up_field, simple->spec->name)) 
			j++;
		if(j > 1){
			g_warning("Constraint's property is neither parent nor up property");
			return;
			
		}*/		
		g_string_append(sql, "IN"); 
	} else {
		g_string_append(sql, simple->op);
	}
	g_string_append_c(sql, ' ');

        if (g_str_equal(simple->op, "IN") || g_str_equal(simple->op, "NOT IN")) {
                g_string_append_c(sql, '(');
                if (G_VALUE_HOLDS(value, G_TYPE_VALUE_ARRAY)) {
                        GValueArray *array = (GValueArray *) g_value_get_boxed(value);
                        for (i = 0; i < array->n_values; i++) {
                                if (i > 0) {
                                        g_string_append_c(sql, ',');
                                }
                                add_sql_value(simple, g_value_array_get_nth(array, i), sql);
                        }
                } else {
                        add_sql_value(simple, value, sql);
                }
                g_string_append_c(sql, ')');
		
		/* FIXME , rewrite it when midgard_tree class or something similiar 
		 * is implemented */
	} else if(g_str_equal(simple->op, "INTREE")) {
		table = midgard_object_class_get_table(
				(MidgardObjectClass *)simple->klass);
		
		guint tid = 0;
		if(G_VALUE_HOLDS(value, G_TYPE_UINT))
			tid = g_value_get_uint(value);

		if(G_VALUE_HOLDS(value, G_TYPE_INT))
			tid = (guint)g_value_get_int(value);

		gint *prnts =
			mgd_tree((midgard *)simple->mgd, table, 
					simple->spec->name, 
					tid, 
					0, NULL);
		if(prnts) {
			g_string_append_c(sql, '(');
			i = 0;
			do {
				if(i > 0)
					g_string_append(sql,",");
							
				g_string_append_printf(sql,
						"%d", prnts[i]);
				i++;
				
			} while (prnts[i]);
			g_string_append_c(sql, ')');
		}
					
        } else {
                add_sql_value(simple, value, sql);
        }
}

G_DEFINE_TYPE(MidgardSimpleConstraint, midgard_simple_constraint, MIDGARD_TYPE_QUERY_CONSTRAINT)

static void midgard_simple_constraint_init(MidgardSimpleConstraint *self) {
        g_assert(self);
}

static void midgard_simple_constraint_dispose(MidgardSimpleConstraint *simple) {
        g_assert(simple);

        g_value_unset(simple->value);
        g_free(simple->value);

        GObjectClass *parent =
                G_OBJECT_CLASS(midgard_simple_constraint_parent_class);
        parent->dispose(G_OBJECT(simple));
}

static void midgard_simple_constraint_class_init(
                MidgardSimpleConstraintClass *klass) {
        g_assert(klass);
        MIDGARD_QUERY_CONSTRAINT_CLASS(klass)->add_sql = add_sql;
        G_OBJECT_CLASS(klass)->dispose =
                (void (*)(GObject *)) midgard_simple_constraint_dispose;
}

/**
 * Creates a simple constraint for the Midgard Query Builder.
 *
 * @param object object of the correct type
 * @param name   property name
 * @param op     comparison operator
 * @param value  scalar value for comparison
 * @return query constraint, or NULL if the constraint is not valid
 */
MidgardSimpleConstraint *midgard_simple_constraint_new(
                midgard *mgd, GObjectClass *klass,
                const gchar *name, const gchar *op, const GValue *value) {
        /* Find the matching object property */
        const GParamSpec *spec = g_object_class_find_property(klass, name);
              
        if (spec == NULL) {
                g_warning("Invalid constraint property %s\n", name);
                return NULL;
        }

        /* Find the matching operator */
        guint i;
        for (i = 0; operator[i] != NULL; i++) {
                if (g_ascii_strcasecmp(op, operator[i]) == 0) {
                        op = operator[i];
                        break;
                }
        }
        if (operator[i] == NULL) {
                g_warning("Invalid comparison operator %s\n", op);
                return NULL;     
        }

        /* Copy the value */
        GValue *new_value = g_new0(GValue, 1);
        g_value_init(new_value, G_VALUE_TYPE(value));
        g_value_copy(value, new_value);
        
        MidgardSimpleConstraint *simple = MIDGARD_SIMPLE_CONSTRAINT(
                g_object_new(MIDGARD_TYPE_SIMPLE_CONSTRAINT, NULL));
        simple->mgd = mgd;
        simple->spec = spec;
        simple->op = op;
        simple->value = new_value;
	simple->klass = klass;
        return simple;
}
