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
#ifndef MIDGARD_UUID_H
#define MIDGARD_UUID_H

/**
 * \defgroup uuid Universally Unique Identifiers
 * This is a simple implementation of the RFC 4122 type 1 and type 3 UUIDs
 * used as Midgard Globally Unique Identifiers (GUIDs).
 *
 * Type 1 UUIDs are generated using a random per-process node identifier.
 * This makes the generated UUIDs a bit less unique, but remove the need for
 * persistent state and inter-process synchronization.
 *
 * Type 3 UUIDs are generated using the special Midgard namespace UUID
 * \c 00dc46a0-0e0c-1085-82bb-0002a5d5fd2e as the namespace.
 *
 * The UUID functions should only be called directly if an explicit UUID
 * is needed. The more general GUID functions should be used for object
 * identifiers and other normal GUIDs. Although the GUID functions normally
 * use the UUID functions internally, they make it possible to specify
 * database-specific GUID generators or constraints.
 */

#include <glib.h>

/**
 * \ingroup uuid
 * Checks whether the given string matches the UUID format.
 *
 * \param[in] uuid the potential UUID string
 * \return \c TRUE if the given string is a UUID, \c FALSE otherwise
 */
extern gboolean midgard_is_uuid(const gchar *uuid);

/**
 * \ingroup uuid
 * Generates a new UUID string. The returned UUID is a time-based time 1
 * UUID. A random per-process node identifier is used to avoid keeping global
 * state and maintaining inter-process synchronization.
 *
 * \return UUID string (newly allocated)
 */
extern gchar *midgard_uuid_new(void);

/**
 * \ingroup uuid
 * Generates a name-based (type 3) UUID string from the given external
 * identifier. The special Midgard namespace UUID
 * \c 00dc46a0-0e0c-1085-82bb-0002a5d5fd2e is used as the namespace of
 * the generated UUID.
 *
 * \param[in] external the external identifier
 * \return UUID string (newly allocated)
 */
extern gchar *midgard_uuid_external(const gchar *external);

#endif
