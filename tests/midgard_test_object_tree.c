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

#include "midgard_test_object_tree.h"
#include "midgard_test_object_basic.h"

void midgard_test_object_tree_basic(MgdObjectTest *mot, gconstpointer data)
{
	g_assert(mot != NULL);
	MgdObject *object = MIDGARD_OBJECT(mot->object);
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(object);

	/* Get parent (in tree) class and check if parent has the same child declared */
	const gchar *pname = midgard_object_parent(object);
	
	if(!pname)
		return;

	MidgardObjectClass *pklass = MIDGARD_OBJECT_GET_CLASS_BY_NAME(pname);
	g_assert(pklass != NULL);

	/* There is tree parent declared so parent property can not be NULL */
	const gchar *parent_property = midgard_object_class_get_parent_property(klass);
	g_assert(parent_property != NULL);

	MidgardObjectClass **children = midgard_object_class_list_children(pklass);
	g_assert(children != NULL);

	guint i = 0;
	gboolean has_child_class = FALSE;
	
	while(children[i] != NULL) {

		if(G_OBJECT_CLASS_TYPE(children[i]) == G_OBJECT_CLASS_TYPE(klass))
			has_child_class = TRUE;
		i++;
	}

	g_free(children);

	g_assert(has_child_class != FALSE);
}

void midgard_test_object_tree_create(MgdObjectTest *mot, gconstpointer data)
{
	g_assert(mot != NULL);
	MgdObject *_object = MIDGARD_OBJECT(mot->object);
	MidgardConnection *mgd = MIDGARD_CONNECTION(mot->mgd);
	MidgardObjectClass *klass = MIDGARD_OBJECT_GET_CLASS(_object);

	/* Get parent (in tree) class and check if parent has the same child declared */
	const gchar *pname = midgard_object_parent(_object);
	
	if(!pname)
		return;

	MidgardObjectClass *pklass = MIDGARD_OBJECT_GET_CLASS_BY_NAME(pname);
	g_assert(pklass != NULL);

	/* Check if class has 'name' property registered */
	GParamSpec *pspec = g_object_class_find_property(G_OBJECT_CLASS(klass), "name");
	if(!pspec)
		return;

	/* Empty name */
	MgdObject *object = midgard_object_new(mgd->mgd, G_OBJECT_TYPE_NAME(_object), NULL);
	g_object_set(object, "name", "", NULL);

	/* Workaround */
	const gchar *parent_property = midgard_object_class_get_property_parent(klass);
	
	if (parent_property)
		g_object_set(object, parent_property, 1, NULL);

	gboolean created = midgard_object_create(object);
	MIDGARD_TEST_ERROR_OK(mgd);
	g_assert(created != FALSE);

	gboolean purged = midgard_object_purge(object);
	g_assert(purged != FALSE);
	g_object_unref(object);

	object = midgard_object_new(mgd->mgd, G_OBJECT_TYPE_NAME(_object), NULL);
	g_object_set(object, "name", "", NULL);

	/* Workaround */
	parent_property = midgard_object_class_get_property_parent(klass);
	
	if (parent_property)
		g_object_set(object, parent_property, 1, NULL);

	created = midgard_object_create(object);
	MIDGARD_TEST_ERROR_OK(mgd);
	g_assert(created != FALSE);

	purged = midgard_object_purge(object);
	g_assert(purged != FALSE);
	g_object_unref(object);

	/* Unique name */
	object = midgard_object_new(mgd->mgd, G_OBJECT_TYPE_NAME(_object), NULL);
	g_object_set(object, "name", "Unique", NULL);

	/* Workaround */
	parent_property = midgard_object_class_get_property_parent(klass);
	
	if (parent_property)
		g_object_set(object, parent_property, 1, NULL);

	created = midgard_object_create(object);
	MIDGARD_TEST_ERROR_OK(mgd);
	g_assert(created != FALSE);

	MgdObject *dupobject = midgard_object_new(mgd->mgd, G_OBJECT_TYPE_NAME(_object), NULL);
	g_object_set(dupobject, "name", "Unique", NULL);
	
	/* Workaround */
	parent_property = midgard_object_class_get_property_parent(klass);
	
	if (parent_property)
		g_object_set(dupobject, parent_property, 1, NULL);

	gboolean dupcreated = midgard_object_create(dupobject);	
	MIDGARD_TEST_ERROR_ASSERT(mgd, MGD_ERR_DUPLICATE);
	g_assert(dupcreated != TRUE);	
	g_object_unref(dupobject);

	purged = midgard_object_purge(object);
	g_assert(purged != FALSE);
	g_object_unref(object);
}
