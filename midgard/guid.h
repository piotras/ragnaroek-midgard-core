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
#ifndef MIDGARD_GUID_H
#define MIDGARD_GUID_H

/**
 * \defgroup guid Midgard Globally Unique Identifiers
 * This module abstracts the generation and handling of Midgard Globally
 * Unique Identifiers (GUIDs).
 *
 * Each Midgard object is assigned a GUID when it is created. GUIDs can be
 * used to identify a Midgard object even across database and system
 * boundaries.
 *
 * GUIDs are normally assigned automatically by Midgard core when an object
 * is created. An application can optionally set the object GUID before the
 * object has been saved in a database. This feature is normally only used
 * when replicating or otherwise mirroring content from external sources.
 *
 * GUIDs can either be created from scratch with midgard_guid_new() or from
 * an external unique identifier with midgard_guid_external().
 *
 * The midgard_is_guid() function can be used to check whether a given string
 * looks like a Midgard GUID.
 */

#include "midgard/midgard_defs.h"
#include <glib.h>

/**
 * \ingroup guid
 * Checks whether the given string matches the GUID format.
 *
 * \param[in] guid the potential GUID string
 * \return \c TRUE if the given string is a GUID, \c FALSE otherwise
 */
extern gboolean midgard_is_guid(const gchar *guid);

/**
 * \ingroup guid
 * 
 * Generates a new GUID string. The returned UUID is either an old-style
 * Midgard GUID (a hex-encoded semi-random MD5 hash), or a time-based time 1
 * \UUID.
 *
 * \param[in] mgd the Midgard handle containing GUID configuration
 * \return GUID string (newly allocated)
 */
extern gchar *midgard_guid_new(midgard *mgd);

/**
 * \ingroup guid
 * Generates a GUID string from the given external identifier.
 *
 * \param[in] mgd the Midgard handle containing GUID configuration
 * \param[in] external the external identifier
 * \return UUID string (newly allocated)
 */
extern gchar *midgard_guid_external(midgard *mgd, const gchar *external);

/**
 * \ingroup guid
 * Generates a GUID string from the given table name and record identifier.
 *
 * \param[in] mgd the Midgard handle containing GUID configuration
 * \param[in] table table name (unused)
 * \param[in] id record identifier (unused)
 * \return UUID string (newly allocated)
 * \deprecated Use the midgard_guid_new() function instead.
 */
extern gchar *mgd_create_guid(midgard *mgd, const gchar *table, guint id);

#endif
