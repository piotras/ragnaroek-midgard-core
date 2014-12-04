/* 
 * Copyright (C) 2005 Piotr Pokora <piotr.pokora@infoglob.pl>
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

#ifndef MIDGARD_QUOTA_H
#define MIDGARD_QUOTA_H

#include "midgard/types.h"
#include "midgard/midgard_object.h"

/**
 * 
 * \defgroup quota Midgard Quota 
 * 
 * Returns object's disk usage size 
 *
 * \param object MgdObject instance
 * 
 * \return object's size 
 *
 * This function returns object's size stored in object's table record. 
 * 
 */ 
guint midgard_quota_get_object_size(MgdObject *object);


/**
 *
 * \ingroup quota 
 *
 * Returns object's size
 *
 * \param object MgdObject instance
 *
 * \return object's size
 *
 * This function returns current size of object, including properties' size
 * and attachment's size if object is midgard_attachment type
 */
guint midgard_object_get_size(MgdObject *object);

/**
 * 
 * \ingroup quota
 *
 * \param object MgdObject object instance
 *
 * Creates object's quota entry. This function updates object's
 * quota size and increase number of object's type records
 *
 * \return FALSE when quota limit is reached , TRUE otherwise 
 * 
 */
gboolean midgard_quota_create(MgdObject *object);

/**
 *
 * \ingroup quota 
 *
 * \param object MgdObject object instance
 *
 * Updates object's storage setting object's size
 *
 * \return FALSE when quota limit is reached , TRUE otherwise
 * 
 */
gboolean midgard_quota_update(MgdObject *object);

/**
 * 
 * \ingroup quota
 * 
 * \param size object's size 
 * \param object MgdObject object instance
 *
 * Removes object's quota entry. This function decrease object's 
 * quota size and decrease number of object's type records.
 */
void midgard_quota_remove(MgdObject *object, guint size);

/**
 * 
 * \ingroup quota
 *
 * Sets type's quota limits.
 * 
 * \param mgd midgard struct connection handler
 * \param sitegroup identification id for a sitegroup
 * \param typename name of midgard's object type
 * \param size quota size in bytes
 * \param records number of records allowed per type
 * 
 */
void midgard_quota_set_type_size(midgard *mgd, guint sitegroup, const gchar *typename,
		guint size, guint records);

/**
 *
 * \ingroup quota
 *
 * Gets typename quota size.
 *
 * \param mgd midgard struct connection handler
 * \param typename name of midgard's object type
 *
 * Returns quota size used by all typename objects
 */
guint midgard_quota_get_type_size(midgard *mgd, const gchar *typename);


/**
 *
 * \ingroup quota
 *
 * Sets sitegroup's quota limits
 *
 * \param mgd midgard struct connection handler
 * \param sitegroup identification id for a sitegroup
 * \param size quota size in bytes
 * \param records number of records allowed per type
 *
 * Sets quota limit for disk and records usage.
 * 
 */ 
void midgard_quota_set_sitegroup_size(midgard *mgd, guint sitegroup,
		guint size, guint records);

/**
 *
 * \ingroup quota
 *
 * Gets quota size 
 *
 * \param mgd midgard struct connection handler
 * \param sitegroup identification id for a sitegroup
 *
 * Returns quota size used by sitegroup.
 */
guint midgard_quota_get_sitegroup_size(midgard *mgd, guint sitegroup);

/**
 *
 * \ingroup quota
 *
 * Checks if quota is reached
 *
 * \param object MgdObject instance
 * \size amount of bytes which should be added to sitegroup's and object's size
 *
 * Returns TRUE if quota limit size is reached, FALSE otherwise.
 */ 
gboolean midgard_quota_size_is_reached(MgdObject *object, gint size);

/**
 *
 * \ingroup quota
 *
 * Returns sitegroup's size
 *
 * \param mgd midgard struct sonnection handler
 * \param sg identification id for a sitegroup
 *
 * \return Sitegroup's size in bytes, including all attachments' size
 *
 */ 
guint32 midgard_quota_get_sitegroup_size(midgard *mgd, guint sg);

#endif /* MIDGARD_QUOTA_H */
