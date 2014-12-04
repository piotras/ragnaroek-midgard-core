/* gbase64.h - Base64 coding functions
 *
 *  Copyright (C) 2005  Alexander Larsson <alexl@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#ifndef __G_BASE64_H__
#define __G_BASE64_H__
#define __MIDGARD_HAVE_G_BASE64

#if    __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define G_GNUC_MALLOC                           \
	__attribute__((__malloc__))
#else
#define G_GNUC_MALLOC
#endif

#include <glib/gtypes.h>

G_BEGIN_DECLS

gsize   g_base64_encode_step  (const guchar *in,
			       gsize         len,
			       gboolean      break_lines,
			       gchar        *out,
			       gint         *state,
			       gint         *save);
gsize   g_base64_encode_close (gboolean      break_lines,
			       gchar        *out,
			       gint         *state,
			       gint         *save);
gchar*  g_base64_encode       (const guchar *data,
			       gsize         len) G_GNUC_MALLOC;
gsize   g_base64_decode_step  (const gchar  *in,
			       gsize         len,
			       guchar       *out,
			       gint         *state,
			       guint        *save);
guchar *g_base64_decode       (const gchar  *text,
			       gsize        *out_len) G_GNUC_MALLOC;

G_END_DECLS

#endif /* __G_BASE64_H__ */
