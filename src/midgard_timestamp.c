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

#include "midgard/midgard_timestamp.h"
#include <glib.h>

struct caltime {
  long year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  unsigned long nano;
  long offset;
};

#define leapsecs_add(a,b) (a)

static const gchar *caltime_fmt(struct caltime *ct) {
        GString *string = g_string_sized_new(100);
        g_string_printf(
                string, "%ld-%02d-%02d %02d:%02d:%02d",
                ct->year, ct->month, ct->day, ct->hour, ct->minute, ct->second);
        if (ct->nano > 0) {
                unsigned long n = ct->nano;
                while ((n % 10) == 0) { n = n / 10; }
                g_string_append_printf(string, ".%lo", n);
        }
        g_string_append_printf(string, "%+05ld", ct->offset);
        return g_string_free(string, FALSE);
}

static unsigned int caltime_scan(const char *s, struct caltime *ct) {
  int sign = 1;
  const char *t = s;
  unsigned long z;
  unsigned long c;

  if (*t == '-') { ++t; sign = -1; }
  z = 0; while ((c = (unsigned char) (*t - '0')) <= 9) { z = z * 10 + c; ++t; }
  ct->year = z * sign;

  if (*t++ != '-') return 0;
  z = 0; while ((c = (unsigned char) (*t - '0')) <= 9) { z = z * 10 + c; ++t; }
  ct->month = z;

  if (*t++ != '-') return 0;
  z = 0; while ((c = (unsigned char) (*t - '0')) <= 9) { z = z * 10 + c; ++t; }
  ct->day = z;

  while ((*t == ' ') || (*t == '\t') || (*t == 'T')) ++t;
  z = 0; while ((c = (unsigned char) (*t - '0')) <= 9) { z = z * 10 + c; ++t; }
  ct->hour = z;

  if (*t++ != ':') return 0;
  z = 0; while ((c = (unsigned char) (*t - '0')) <= 9) { z = z * 10 + c; ++t; }
  ct->minute = z;

  if (*t != ':')
    ct->second = 0;
  else {
    ++t;
    z = 0; while ((c = (unsigned char) (*t - '0')) <= 9) { z = z * 10 + c; ++t; }
    ct->second = z;
  }

  if (*t != '.')
    ct->nano = 0;
  else {
    ++t;
    ct->nano = 1000000000;
    z = 0; while ((c = (unsigned char) (*t - '0')) <= 9) { if (ct->nano > 1) { z = z * 10 + c; ++t; ct->nano /= 10; } }
    ct->nano *= z;
  }

  while ((*t == ' ') || (*t == '\t')) ++t;
  if (*t == '+') sign = 1; else if (*t == '-') sign = -1; else return 0;
  ++t;
  c = (unsigned char) (*t++ - '0'); if (c > 9) return 0; z = c;
  c = (unsigned char) (*t++ - '0'); if (c > 9) return 0; z = z * 10 + c;
  c = (unsigned char) (*t++ - '0'); if (c > 9) return 0; z = z * 6 + c;
  c = (unsigned char) (*t++ - '0'); if (c > 9) return 0; z = z * 10 + c;
  ct->offset = z * sign;

  return t - s;
}

static void midgard_timestamp_get_caltime(
                const GValue *value, struct caltime *ct) {
  guint64 u = value->data[0].v_uint64 + 58486;
  long s = u % 86400ULL;
  long day = u / 86400ULL - 53375995543064ULL;
  long year = day / 146097L;
  long month;
  /* int yday; */

  ct->second = s % 60; s /= 60;
  ct->minute = s % 60; s /= 60;
  ct->hour = s;

  day = (day % 146097L) + 678881L;
  while (day >= 146097L) { day -= 146097L; ++year; }

  /* year * 146097 + day - 678881 is MJD; 0 <= day < 146097 */
  /* 2000-03-01, MJD 51604, is year 5, day 0 */

  year *= 4;
  if (day == 146096L) { year += 3; day = 36524L; }
  else { year += day / 36524L; day %= 36524L; }
  year = year * 25 + day / 1461;
  day %= 1461;
  year *= 4;

  if (day == 1460) { year += 3; day = 365; }
  else { year += day / 365; day %= 365; }

  day *= 10;
  month = (day + 5) / 306;
  day = (day + 5) % 306;
  day /= 10;
  if (month >= 10) { ++year; month -= 10; }
  else { month += 2; }

  ct->year = year;
  ct->month = month + 1;
  ct->day = day + 1;

  ct->offset = 0;
  ct->nano = value->data[1].v_ulong;
}

static void midgard_timestamp_set_caltime(
		GValue *value, const struct caltime *ct) {
  static const unsigned long times365[4] = { 0, 365, 730, 1095 };
  static const unsigned long times36524[4] = { 0, 36524UL, 73048UL, 109572UL };
  static const unsigned long montab[12] =
    { 0, 31, 61, 92, 122, 153, 184, 214, 245, 275, 306, 337 };
    /* month length after february is (306 * m + 5) / 10 */

  long y = ct->year;
  long m = ct->month - 1;
  long d = ct->day - 678882L;
  long s = ((ct->hour * 60 + ct->minute) - ct->offset) * 60 + ct->second;

  d += 146097L * (y / 400);
  y %= 400;

  if (m >= 2) m -= 2; else { m += 10; --y; }

  y += (m / 12);
  m %= 12;
  if (m < 0) { m += 12; --y; }

  d += montab[m];

  d += 146097L * (y / 400);
  y %= 400;
  if (y < 0) { y += 400; d -= 146097L; }

  d += times365[y & 3];
  y >>= 2;

  d += 1461L * (y % 25);
  y /= 25;

  d += times36524[y & 3];

  value->data[0].v_uint64 =
    d * 86400ULL + 4611686014920671114ULL + (long long) s;
  value->data[1].v_ulong = ct->nano;
  /* TODO: leapsecs */
}


/* Internal initialization function for the midgard_timestamp value type. */
static void value_init_timestamp(GValue *value) {
        g_assert(G_VALUE_HOLDS(value, MIDGARD_TYPE_TIMESTAMP));

        value->data[0].v_uint64 = 4611686018427387914ULL;
        value->data[1].v_ulong = 0;
}

/* Internal copy function for the midgard_timestamp value type. */
static void value_copy_timestamp(const GValue *src, GValue *dst) {
        g_assert(G_VALUE_HOLDS(src, MIDGARD_TYPE_TIMESTAMP));
        g_assert(G_VALUE_HOLDS(dst, MIDGARD_TYPE_TIMESTAMP));

        dst->data[0].v_uint64 = src->data[0].v_uint64;
        dst->data[1].v_ulong = src->data[1].v_ulong;
}

/* Internal collect function for the midgard_timestamp value type. */
static gchar *value_collect_timestamp(
                GValue *value, guint n_collect_values,
                GTypeCValue *collect_values, guint collect_flags) {
        g_assert(G_VALUE_HOLDS(value, MIDGARD_TYPE_TIMESTAMP));
        g_assert(n_collect_values == 1);
        g_assert(collect_values != NULL);

        const gchar *time = (const gchar *) collect_values;
        if (time == NULL) {
                return g_strdup("Midgard timestamp value string passed as NULL");
        }

        return NULL;
}

/* Internal lcopy function for the midgard_timestamp value type. */
static gchar *value_lcopy_timestamp(
                const GValue *value, guint n_collect_values,
                GTypeCValue *collect_values, guint collect_flags) {
        g_assert(G_VALUE_HOLDS(value, MIDGARD_TYPE_TIMESTAMP));
        g_assert(n_collect_values == 1);
        g_assert(collect_values != NULL);

        const gchar **time = (const gchar **) collect_values;
        if (time == NULL) {
                return g_strdup("Midgard timestamp value string passed as NULL");
        }

        return NULL;
}

/** Converts a Midgard timestamp value to a string value. */
/* Not used , commented */
/* 
static void midgard_timestamp_to_string(const GValue *src, GValue *dst) {
        g_assert(G_VALUE_HOLDS(src, MIDGARD_TYPE_TIMESTAMP));
        g_assert(G_VALUE_HOLDS(dst, G_TYPE_STRING));

        g_value_take_string(dst, midgard_timestamp_dup_string(src));
}
*/

/** Converts a string value to a Midgard timestamp value. */
/* Not used, commented */
/*
static void midgard_timestamp_from_string(const GValue *src, GValue *dst) {
        g_assert(G_VALUE_HOLDS(src, G_TYPE_STRING));
        g_assert(G_VALUE_HOLDS(dst, MIDGARD_TYPE_TIMESTAMP));

        midgard_timestamp_set_string(dst, g_value_get_string(src));
}
*/

GType midgard_timestamp_get_type(void) {
        static GType type = 0;
        if (type == 0) {
                static const GTypeValueTable value_table = {
                        &value_init_timestamp,     /* value_init */
                        NULL,                      /* value_free */
                        &value_copy_timestamp,     /* value_copy */
                        NULL,                      /* value_peek_pointer */
                        "p",                       /* collect_format */
                        &value_collect_timestamp,  /* collect_value */
                        "p",                       /* lcopy_format */
                        &value_lcopy_timestamp     /* lcopy_value */
                };
                static const GTypeInfo info = {
                        0,                         /* class_size */
                        NULL,                      /* base_init */
                        NULL,                      /* base_destroy */
                        NULL,                      /* class_init */
                        NULL,                      /* class_destroy */
                        NULL,                      /* class_data */
                        0,                         /* instance_size */
                        0,                         /* n_preallocs */
                        NULL,                      /* instance_init */
                        &value_table               /* value_table */
                };
                static const GTypeFundamentalInfo finfo = {
                        G_TYPE_FLAG_DERIVABLE
                };
                type = g_type_register_fundamental(
                        g_type_fundamental_next(), "midgard_timestamp",
                        &info, &finfo, 0);
        }
        return type;
}

gchar *midgard_timestamp_dup_string(const GValue *value) {
        g_assert(G_VALUE_HOLDS(value, MIDGARD_TYPE_TIMESTAMP));
        struct caltime ct;
        midgard_timestamp_get_caltime(value, &ct);
        return (gchar *)caltime_fmt(&ct); 
}

void midgard_timestamp_set_string(GValue *value, const gchar *time) {
        g_assert(G_VALUE_HOLDS(value, MIDGARD_TYPE_TIMESTAMP));
        g_assert(time != NULL);

        struct caltime ct;
        if (caltime_scan(time, &ct) > 0) {
                midgard_timestamp_set_caltime(value, &ct);
        } else {
                value->data[0].v_uint64 = 4611686018427387914ULL;
                value->data[1].v_ulong = 0;
        }
}

time_t midgard_timestamp_get_time(const GValue *value) {
        g_assert(G_VALUE_HOLDS(value, MIDGARD_TYPE_TIMESTAMP));

        if (value->data[1].v_ulong >= 1000000000 / 2) {
            return value->data[0].v_uint64 - 4611686018427387914ULL + 1;
        } else {
            return value->data[0].v_uint64 - 4611686018427387914ULL;
        }
}

void midgard_timestamp_set_time(GValue *value, time_t time) {
        g_assert(G_VALUE_HOLDS(value, MIDGARD_TYPE_TIMESTAMP));

        value->data[0].v_uint64 = time + 4611686018427387914ULL;
        value->data[1].v_ulong = 0;
}

GValue __get_current_timestamp_value()
{
	GValue tval = {0, };
	time_t utctime = time(NULL);
	g_value_init(&tval, MIDGARD_TYPE_TIMESTAMP);
	midgard_timestamp_set_time(&tval, utctime);
	
	return tval;
}

gchar *midgard_timestamp_current_as_char()
{
	GValue tval = __get_current_timestamp_value();
	gchar *dtstr = midgard_timestamp_dup_string(&tval);
	g_value_unset(&tval);
	
	return dtstr;
}

GValue midgard_timestamp_current_as_value(GType type)
{
	GValue tval = __get_current_timestamp_value();
	
	if(type == G_TYPE_STRING) {
		
		GValue sval = {0, };
		g_value_init(&sval, G_TYPE_STRING);
		g_value_take_string(&sval,  midgard_timestamp_dup_string(&tval));
		g_value_unset(&tval);
		
		return sval;
	}

	return tval;
}
