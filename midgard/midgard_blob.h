/* 
 * Copyright (C) 2006, 2008 Piotr Pokora <piotr.pokora@infoglob.pl>
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

#ifndef _MIDGARD_BLOB_H
#define _MIDGARD_BLOB_H

/**
 * \defgroup midgard_blob Midgard Blob
 * 
 * Simple class which is intented to manage file descriptors via GIOChannel interface.
 * MidgardBlob encapsulate relations among all typical MidgardSchema objects, midgard_attachment
 * objects and MidgardConnection handler. It's designed as a helper to build language bindings
 * written for Midgard system. 
 */

#include "midgard/midgard_connection.h"
#include "midgard/midgard_object.h"

G_BEGIN_DECLS

/* convention macros */
#define MIDGARD_TYPE_BLOB (midgard_blob_get_type())
#define MIDGARD_BLOB(object)  \
	(G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_BLOB, MidgardBlob))
#define MIDGARD_BLOB_CLASS(klass)  \
	(G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_BLOB, MidgardBlobClass))
#define MIDGARD_IS_BLOB(object)   \
	(G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_BLOB))
#define MIDGARD_IS_BLOB_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_BLOB))
#define MIDGARD_BLOB_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_BLOB, MidgardBlobClass))

typedef struct MidgardBlob MidgardBlob;
typedef struct MidgardBlobClass MidgardBlobClass;
typedef struct _MidgardBlobPrivate MidgardBlobPrivate;

/**
 * \ingroup midgard_blob
 *
 * MidgardObjectClass structure.
 */ 
struct MidgardBlobClass {
	GObjectClass parent;

	gchar *(*read_content) (MidgardBlob *self, gsize *bytes_read);
	gboolean (*write_content) (MidgardBlob *self, const gchar *content);
	gboolean (*remove_file) (MidgardBlob *self);
	gboolean (*exists) (MidgardBlob *self);
	const gchar *(*get_path) (MidgardBlob *self);
	GIOChannel *(*get_handler) (MidgardBlob *self, const gchar *mode);
};

/**
 * \ingroup midgard_blob
 *
 * MidgardObject structure
 */ 
struct MidgardBlob {
	GObject parent;

	MidgardBlobPrivate *priv;
};

/**
 * \ingroup midgard_blob
 *
 * Returns Midgard Blob Gtype value.
 * Registers this type unless already registered.
 */ 
extern GType 
midgard_blob_get_type(void);

/**
 * \ingroup midgard_blob
 * 
 * Instatiate new Midgard Blob.
 *
 * \param attachment , MidgardObject of MIDGARD_TYPE_ATTACHMENT type.
 * 
 * \return MidgardBlob, newly instatiated MidgardBlob object.
 * 
 * Instatiate new Midgard Blob object for the given midgard_attachment object. 
 * This is almost the same constructor as g_object_new, but unlike that one,
 * midgard_object_new requires  MgdObject object's pointer.
 */
extern MidgardBlob *
midgard_blob_new(MgdObject *attachment, const gchar *encoding);

/**
 * \ingroup midgard_blob
 *
 * Returns content of the file.
 *
 * \param self, MidgardBlob self instance
 * \param bytes_read, number of bytes read
 * 
 * \return file's content or NULL.
 *
 * Returned content should be freed when no longer needed.
 * \c bytes_read returned content's size. It should be used if content
 * is passed to function which requires its size. In such case, there's
 * no need to count content size once again.
 *
 * This function should be used to get content of small files.
 * For large and huge ones midgard_blob_get_handler should be used
 * to get file handle.
 */ 
extern gchar *
midgard_blob_read_content(MidgardBlob *self, gsize *bytes_read);

/**
 * \ingroup midgard_blob
 *
 * Write given content to a file.
 *
 * \param self, MidgardBlob self instance.
 * \param content, content which should be written to file.
 *
 * \return TRUE if content has been written to file, FALSE otherwise.
 */ 
extern gboolean
midgard_blob_write_content(MidgardBlob *self,
				const gchar *content);

/**
 * \ingroup midgard_blob
 *
 * Get GIOChannel
 *
 * \param self, MidgardBlob instance
 *
 * \return GIOChannel
 *
 * The main idea is to get file handler. On C level it returns
 * GIOChannel, but language bindings could return typical file handler 
 * or anything else which is needed for particular language.
 *
 * Returned channel is owned by midgard_blob and should not be freed
 * or unreferenced.
 */
extern GIOChannel *midgard_blob_get_handler(MidgardBlob *self, const gchar *mode);

/** 
 * \ingroup midgard_blob
 *
 * Get absolute path of a file
 *
 * \param self, MidgardBlob instance
 * 
 * \return absolute path or NULL in case of failure
 *
 * Returned path is owned by midgard_blob and should not be freed.
 * It basically contains blobdir and relative file's location.
 */ 
extern const gchar *midgard_blob_get_path(MidgardBlob *self);

/**
 * \ingroup midgard_blob
 *
 * Check if file exists
 *
 * \param self, MidgardBlob instance
 * \return TRUE if file exists, FALSE otherwise
 *
 * Check if file associated with midgard_blob exists.
 * This function will also return FALSE, if file is not yet 
 * associated.
 */ 
extern gboolean midgard_blob_exists(MidgardBlob *self);

/**
 * \ingroup midgard_blob
 * 
 * Delete file associated wtih MidgardBlob instance.
 *
 * \param self, MidgardBlob self instance.
 *
 * \return TRUE on success, FALSE otherwise
 *
 * Deletes a file which is associated with blob and located
 * in attachment's location which is initialized for blob.
 * midgard_blob_exists should be invoked if file may be already 
 * deleted, for example when one file is shared among many attachments.
 */ 
extern gboolean midgard_blob_remove_file(MidgardBlob *self);

G_END_DECLS

#endif /* _MIDGARD_BLOB_H */
