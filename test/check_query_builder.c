/* 
 * Copyright (C) 2005 Jukka Zitting <jz@yukatan.fi>
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

#include "check_query_builder.h"
#include "midgard/midgard_legacy.h"
#include "midgard/midgard_schema.h"
#include "midgard/query_builder.h"
#include "midgard/midgard_datatypes.h"

static midgard *mgd = NULL;

static void setup(void) {
        mgd_init();
        mgdlib_init();
        mgd = mgd_connect("localhost", "midgard", "midgard", "midgard");
        GObject *schema  = g_object_new(MIDGARD_TYPE_SCHEMA, NULL);
	midgard_schema_init((MidgardSchema *)schema);
	mgd->schema = (MidgardSchema *)schema;
}

static void teardown(void) {
        mgd_close(mgd);
        mgd_done();
}

START_TEST(test_empty)
{
        MidgardQueryBuilder *builder =
                midgard_query_builder_new(mgd, "midgard_host");
        fail_if(builder == NULL, NULL);

        MidgardTypeHolder *holder = g_new(MidgardTypeHolder, 1);
        GObject **objects = midgard_query_builder_execute(builder, holder);
        fail_if(objects == NULL, NULL);
        if (objects != NULL) {
                guint i;
                for (i = 0; i < holder->elements; i++) {
                        fail_if(objects[i] == NULL, NULL);
                        g_object_unref(objects[i]);
                }
        }
        g_free(objects);
	g_free(holder);

        midgard_query_builder_free(builder);
}
END_TEST

START_TEST(test_simple)
{
        MidgardQueryBuilder *builder =
                midgard_query_builder_new(mgd, "midgard_topic");
        fail_if(builder == NULL, NULL);

        GValue *value = g_new0(GValue, 1);
        g_value_init(value, G_TYPE_UINT);
        g_value_set_uint(value, 1);
        midgard_query_builder_add_constraint(builder, "id", "=", value);
        g_value_unset(value);
        g_free(value);

        MidgardTypeHolder *holder = g_new(MidgardTypeHolder, 1);
        GObject **objects = midgard_query_builder_execute(builder, holder);
        fail_if(objects == NULL, NULL);
        if (objects != NULL) {
                guint i;
                for (i = 0; i < holder->elements; i++) {
                        fail_if(objects[i] == NULL, NULL);
                        g_object_unref(objects[i]);
                }
        }
        g_free(objects);
	g_free(holder);

        midgard_query_builder_free(builder);
}
END_TEST

START_TEST(test_group)
{
        MidgardQueryBuilder *builder =
                midgard_query_builder_new(mgd, "midgard_article");
        fail_if(builder == NULL, NULL);

        fail_unless(midgard_query_builder_begin_group(builder, "OR"), NULL);

        GValue *value = g_new0(GValue, 1);
        g_value_init(value, G_TYPE_UINT);
        g_value_set_uint(value, 1);
        midgard_query_builder_add_constraint(builder, "id", "=", value);
        g_value_set_uint(value, 2);
        midgard_query_builder_add_constraint(builder, "id", "=", value);
        g_value_unset(value);
        g_free(value);

        fail_unless(midgard_query_builder_end_group(builder), NULL);

        fail_unless(midgard_query_builder_begin_group(builder, "AND"), NULL);
        fail_unless(midgard_query_builder_end_group(builder), NULL);

        fail_unless(midgard_query_builder_begin_group(builder, "OR"), NULL);
        fail_unless(midgard_query_builder_begin_group(builder, "AND"), NULL);
        fail_unless(midgard_query_builder_end_group(builder), NULL);
        fail_unless(midgard_query_builder_end_group(builder), NULL);

        MidgardTypeHolder *holder = g_new(MidgardTypeHolder, 1);
        GObject **objects = midgard_query_builder_execute(builder, holder);
        fail_if(objects == NULL, NULL);
        if (objects != NULL) {
                guint i;
                for (i = 0; i < holder->elements; i++) {
                        fail_if(objects[i] == NULL, NULL);
                        g_object_unref(objects[i]);
                }
        }
        g_free(objects);
	g_free(holder);

        midgard_query_builder_free(builder);
}
END_TEST

START_TEST(test_array)
{
        MidgardQueryBuilder *builder =
                midgard_query_builder_new(mgd, "midgard_style");
        fail_if(builder == NULL, NULL);

        GValue *value;
        GValueArray *array = g_value_array_new(2);

        value = g_new0(GValue, 1);
        g_value_init(value, G_TYPE_UINT);
        g_value_set_uint(value, 1);
        g_value_array_append(array, value);
        g_value_unset(value);
        g_free(value);

        value = g_new0(GValue, 1);
        g_value_init(value, G_TYPE_UINT);
        g_value_set_uint(value, 2);
        g_value_array_append(array, value);
        g_value_unset(value);
        g_free(value);

        value = g_new0(GValue, 1);
        g_value_init(value, G_TYPE_VALUE_ARRAY);
        g_value_set_boxed(value, array);
        midgard_query_builder_add_constraint(builder, "id", "IN", value);
        g_value_unset(value);
        g_free(value);

        g_value_array_free(array);

        MidgardTypeHolder *holder = g_new(MidgardTypeHolder, 1);
        GObject **objects = midgard_query_builder_execute(builder, holder);
        fail_if(objects == NULL, NULL);
        if (objects != NULL) {
                guint i;
                for (i = 0; i < holder->elements; i++) {
                        fail_if(objects[i] == NULL, NULL);
                        g_object_unref(objects[i]);
                }
        }
        g_free(objects);
	g_free(holder);

        midgard_query_builder_free(builder);
}
END_TEST

TCase *midgard_query_builder_test_case(void) {
        TCase *test_case = tcase_create("Query builder");
        tcase_add_checked_fixture(test_case, setup, teardown);
        tcase_add_test(test_case, test_empty);
        tcase_add_test(test_case, test_simple);
        tcase_add_test(test_case, test_group);
        tcase_add_test(test_case, test_array);
        return test_case;
}
