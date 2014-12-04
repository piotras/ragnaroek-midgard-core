/*
 * Copyright (c) 2005 Jukka Zitting <jz@yukatan.fi>
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
#ifndef MIDGARD_TIMESTAMP_H
#define MIDGARD_TIMESTAMP_H

#include <time.h>
#include <glib.h>
#include <glib-object.h>

/**
 * The fundamental value type for Midgard timestamps.
 */
#define MIDGARD_TYPE_TIMESTAMP (midgard_timestamp_get_type())

/**
 * Returns the Midgard timestamp value type. Registers the type as
 * a fundamental GType unless already registered.
 */
extern GType midgard_timestamp_get_type(void);

/**
 * Returns the ISO 8601 string representation of this timestamp value.
 * The returned string is newly allocated and needs to be freed by
 * the caller.
 *
 * @param value timestamp value
 * @return ISO 8601 string
 */
extern gchar *midgard_timestamp_dup_string(const GValue *value);

/**
 * Parses the given string for a ISO 8601 datetime and sets the
 * given timestamp to the parsed time.
 *
 * @param value timestamp value to set
 * @param time ISO 8601 string
 */
extern void midgard_timestamp_set_string(GValue *value, const gchar *time);

/**
 * Returns the unix timestamp representation of this timestamp value.
 *
 * @param value timestamp value
 * @return unix timestamp
 */
extern time_t midgard_timestamp_get_time(const GValue *value);

/**
 * Sets the given timestamp value to the given unix timestamp.
 *
 * @param value timestamp value to set
 * @param time unxi timestamp
 */
extern void midgard_timestamp_set_time(GValue *value, time_t time);

gchar *midgard_timestamp_current_as_char();

GValue midgard_timestamp_current_as_value(GType type);

#endif
