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

#include "midgard_test_object_fetch.h"

/* DO NOT fail inside this function.
 * Main purpose is to get (or not) object intentionaly.
 * If it fails, return FALSE and fail with TRUE OR FALSE assertion 
 * depending on actual need */
gboolean midgard_test_object_fetch_by_id(MgdObject *object, guint id)
{
	g_assert(object != NULL);

	MidgardConnection *mgd = object->mgd->_mgd;
	gboolean fetched = midgard_object_get_by_id(object, id);

	if(fetched == FALSE) {

		MIDGARD_TEST_ERROR_ASSERT(mgd, MGD_ERR_NOT_EXISTS);
		g_assert(fetched == FALSE);

		return FALSE;
	}

	MIDGARD_TEST_ERROR_OK(mgd);
	g_assert(fetched == TRUE);

	return TRUE;
}

void midgard_test_object_get_by_id_created(MgdObjectTest *mot, gconstpointer data)
{
	g_assert(mot != NULL);

	MgdObject *_object = mot->object;
	MidgardConnection *mgd = mot->mgd;

	guint oid = 0;
	gchar *oguid = NULL;
	gchar *created = NULL;
	gchar *creator = NULL;
	g_object_get(_object,
			"id", &oid,
			"guid", &oguid,
			NULL);
	
	g_object_get(_object->metadata,
			"created", &created,
			"creator", &creator, NULL);
	
	g_assert_cmpuint(oid, >, 0);

	/* midgard_object_get_by_id */
	MgdObject *object = midgard_test_object_basic_new(mgd, G_OBJECT_TYPE_NAME(_object), NULL);
	g_assert(object != NULL);
	gboolean fetched_by_id =
		midgard_test_object_fetch_by_id(object, oid);
	g_assert(fetched_by_id == TRUE);
	MIDGARD_TEST_ERROR_OK(mgd);
	midgard_test_metadata_check_create(object);
	midgard_test_metadata_check_datetime_properties(object, created, "created", "revised", NULL);
	midgard_test_metadata_check_person_references(object, creator, "creator", "revisor", NULL);
	g_object_unref(object);
	
	g_free(created);
	g_free(creator);
	g_free(oguid);
}

void midgard_test_object_get_by_id_updated(MgdObjectTest *mot, gconstpointer data)
{
	g_assert(mot != NULL);

	MgdObject *_object = mot->object;
	MidgardConnection *mgd = mot->mgd;

	guint oid = 0;
	gchar *oguid = NULL;
	gchar *revised = NULL;
	gchar *revisor = NULL;
	gchar *creator = NULL;
	gchar *created = NULL;
	g_object_get(_object,
			"id", &oid,
			"guid", &oguid,
			NULL);
	
	g_object_get(_object->metadata,
			"revised", &revised,
			"revisor", &revisor, 
			"created", &created, 
			"creator", &creator, 
			NULL);
	
	g_assert_cmpuint(oid, >, 0);

	/* midgard_object_get_by_id */
	MgdObject *object = midgard_test_object_basic_new(mgd, G_OBJECT_TYPE_NAME(_object), NULL);
	g_assert(object != NULL);
	gboolean fetched_by_id =
		midgard_test_object_fetch_by_id(object, oid);
	g_assert(fetched_by_id == TRUE);
	MIDGARD_TEST_ERROR_OK(mgd);
	midgard_test_metadata_check_update(object);
	midgard_test_metadata_check_datetime_properties(object, revised, "revised", NULL);
	midgard_test_metadata_check_datetime_properties(object, created, "created", NULL);
	midgard_test_metadata_check_person_references(object, revisor, "revisor", NULL);
	midgard_test_metadata_check_person_references(object, creator, "creator", NULL);
	g_object_unref(object);
	
	g_free(revised);
	g_free(revisor);
	g_free(created);
	g_free(creator);
	g_free(oguid);

}

void midgard_test_object_get_by_id_deleted(MgdObjectTest *mot, gconstpointer data)
{
	g_assert(mot != NULL);

	MgdObject *_object = mot->object;
	MidgardConnection *mgd = mot->mgd;

	guint oid = 0;
	g_object_get(_object,
			"id", &oid,	
			NULL);
	
	g_assert_cmpuint(oid, >, 0);

	/* midgard_object_get_by_id */
	MgdObject *object = midgard_test_object_basic_new(mgd, G_OBJECT_TYPE_NAME(_object), NULL);
	g_assert(object != NULL);
	gboolean fetched_by_id =
		midgard_test_object_fetch_by_id(object, oid);
	
	g_assert(fetched_by_id == FALSE);
	MIDGARD_TEST_ERROR_ASSERT(mgd, MGD_ERR_NOT_EXISTS);
}

void midgard_test_object_constructor_id_created(MgdObjectTest *mot, gconstpointer data)
{
	g_assert(mot != NULL);

	MgdObject *_object = mot->object;
	MidgardConnection *mgd = mot->mgd;
	guint oid = 0;
	gchar *oguid = NULL;
	gchar *created = NULL;
	gchar *creator = NULL;
	g_object_get(_object,
			"id", &oid,
			"guid", &oguid,
			NULL);
	
	g_object_get(_object->metadata,
			"created", &created,
			"creator", &creator, NULL);
	
	g_assert_cmpuint(oid, >, 0);

	/* midgard_object_new, construct with id */
	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_UINT);
	g_value_set_uint(&gval, oid);
	MgdObject *object = midgard_test_object_basic_new(mgd, G_OBJECT_TYPE_NAME(_object), &gval);
	g_assert(object != NULL);
	MIDGARD_TEST_ERROR_OK(mgd);
	midgard_test_metadata_check_create(object);
	midgard_test_metadata_check_datetime_properties(object, created, "created", "revised", NULL);
	midgard_test_metadata_check_person_references(object, creator, "creator", "revisor", NULL);
	g_object_unref(object);
	g_value_unset(&gval);

	g_free(created);
	g_free(creator);
	g_free(oguid);
}

gboolean midgard_test_object_fetch_by_guid(MgdObject *object, const gchar *guid)
{
	g_assert(object != NULL);

	MidgardConnection *mgd = object->mgd->_mgd;
	gboolean fetched = midgard_object_get_by_guid(object, guid);

	if(fetched == FALSE) {

		MIDGARD_TEST_ERROR_ASSERT(mgd, MGD_ERR_NOT_EXISTS);
		g_assert(fetched == FALSE);

		return FALSE;
	}

	MIDGARD_TEST_ERROR_OK(mgd);
	g_assert(fetched == TRUE);

	return TRUE;
}

void midgard_test_object_get_by_guid_created(MgdObjectTest *mot, gconstpointer data)
{
	g_assert(mot != NULL);

	MgdObject *_object = mot->object;
	MidgardConnection *mgd = mot->mgd;

	guint oid = 0;
	gchar *oguid = NULL;
	gchar *created = NULL;
	gchar *creator = NULL;
	g_object_get(_object,
			"id", &oid,
			"guid", &oguid,
			NULL);
	
	g_object_get(_object->metadata,
			"created", &created,
			"creator", &creator, NULL);

	MgdObject *object = midgard_test_object_basic_new(mgd, G_OBJECT_TYPE_NAME(_object), NULL);
	g_assert(object != NULL);
	gboolean fetched_by_guid =
		midgard_test_object_fetch_by_guid(object, oguid);
	g_assert(fetched_by_guid == TRUE);
	MIDGARD_TEST_ERROR_OK(mgd);
	midgard_test_metadata_check_create(object);
	g_object_unref(object);

	g_free(created);
	g_free(creator);
	g_free(oguid);
}

void midgard_test_object_constructor_guid_created(MgdObjectTest *mot, gconstpointer data)
{
	g_assert(mot != NULL);

	MgdObject *_object = mot->object;
	MidgardConnection *mgd = mot->mgd;
	guint oid = 0;
	gchar *oguid = NULL;
	gchar *created = NULL;
	gchar *creator = NULL;
	g_object_get(_object,
			"id", &oid,
			"guid", &oguid,
			NULL);
	
	g_object_get(_object->metadata,
			"created", &created,
			"creator", &creator, NULL);
	
	g_assert_cmpuint(oid, >, 0);

	GValue gval = {0, };
	g_value_init(&gval, G_TYPE_STRING);
	g_value_set_string(&gval, oguid);
	MgdObject *object = midgard_test_object_basic_new(mgd, G_OBJECT_TYPE_NAME(_object), &gval);
	g_assert(object != NULL);
	midgard_test_metadata_check_create(object);
	midgard_test_metadata_check_datetime_properties(object, created, "created", "revised", NULL);
	midgard_test_metadata_check_person_references(object, creator, "creator", "revisor", NULL);
	g_object_unref(object);
	g_value_unset(&gval);

	g_free(created);
	g_free(creator);
	g_free(oguid);
}

void midgard_test_object_fetch_run(void)
{
	MidgardConfig *config = NULL;
	MidgardConnection *mgd = 
		midgard_test_connection_open_user_config(CONFIG_CONFIG_NAME, &config);
	g_assert(mgd != NULL);

	MgdObject *object = 
		midgard_test_object_basic_new(mgd, "midgard_article", NULL);
	
	gboolean fetched_by_id = 
		midgard_test_object_fetch_by_id(object, 0);
	g_assert(fetched_by_id == FALSE);

	gboolean fetched_by_guid = 
		midgard_test_object_fetch_by_guid(object, "1dd6d5e36ae2dfc6d5e110notexists2a793640c240c2");
	g_assert(fetched_by_guid == FALSE);
}
