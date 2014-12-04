/* 
 * Copyright (C) 2007 Piotr Pokora <piotrek.pokora@gmail.com>
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

#ifndef MIDGARD_USER_H
#define MIDGARD_USER_H

#include <glib-object.h>
#include "midgard/midgard_dbobject.h"
#include "midgard/midgard_object.h"

G_BEGIN_DECLS

/* convention macros */
#define MIDGARD_TYPE_USER (midgard_user_get_type())
#define MIDGARD_USER(object)  \
	        (G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_USER, MidgardUser))
#define MIDGARD_USER_CLASS(klass)  \
	        (G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_USER, MidgardUserClass))
#define MIDGARD_IS_USER(object)   \
	        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_USER))
#define MIDGARD_IS_USER_CLASS(klass) \
	        (G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_USER))
#define MIDGARD_USER_GET_CLASS(obj) \
	        (G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_USER, MidgardUserClass))

typedef struct MidgardUserClass MidgardUserClass;
typedef struct _MidgardUserPrivate MidgardUserPrivate; 

/**
 * 
 * \defgroup midgard_user Midgard User
 *
 * MidgardUser class is a glue between midgard_person class 
 * and internal authentication mechanism.
 */ 
 
/**  
 * \ingroup midgard_user
 *
 * The MidgardUserClass class structure . 
 */
struct MidgardUserClass{
	GObjectClass parent;

	MidgardDBObjectPrivate *dbpriv;
};

/**
 * \ingroup midgard_user
 *
 * The MidgardUser object structure.
 */ 
struct _MidgardUser{
	GObject parent;
	MidgardDBObjectPrivate *dbpriv;
	/* < private > */
	MidgardUserPrivate *priv;
	/* MidgardMetadata *metadata; */
};

/** 
 * \ingroup midgard_user
 *
 * Hash type for midgard_user authentication.
 */
extern enum {
	MIDGARD_USER_HASH_LEGACY = 0,
	MIDGARD_USER_HASH_LEGACY_PLAIN,
	MIDGARD_USER_HASH_CRYPT,
	MIDGARD_USER_HASH_MD5,
	MIDGARD_USER_HASH_PLAIN, 
	MIDGARD_USER_HASH_SHA1,
	MIDGARD_USER_HASH_PAM
} MidgardUserHashType;

/**
 * \ingroup midgard_user
 *
 * Returns the MidgardUser value type. 
 * Registers the type as  a fundamental GType unless already registered.
 *
 */
extern GType midgard_user_get_type(void);

/**
 * \ingroup midgard_user 
 * 
 * Creates new MidgardUser object for the given Midgard Object.
 *
 * \param person, midgard_person object instance
 * \return pointer to MidgardUser object or NULL on failure.
 *
 * Given \c MgdObject must be midgard_person class object.
 * Person should be initialized with any method like :
 * midgard_object_get_by_id or midgard_object_get_by_guid.
 *
 * NULL is returned when: 
 *  - MgdObject is not instance of midgard_person class
 *  - MgdObject is not fetched from database
 */ 
extern MidgardUser *midgard_user_new(MgdObject *person);

/**
 * \ingroup midgard_user
 *
 * Authenticate Midgard User.
 *
 * \param mgd, MidgardConnection 
 * \param name, user's login
 * \param password, user's password
 * \param sitegroup, sitegroup name 
 * \param trusted, whether auth should be done trusted
 *
 * \return MidgardUser instance if user is identified, FALSE otherwise.
 *
 * If \c sitegroup is set to NULL, authentication is done against SG0.
 * If \c trusted is set to TRUE, then password is not verified.
 * In such case caller is responsible for "trusted" authentication type.
 *
 * Sitegroup can not be NULL when \c trusted is TRUE.
 * 
 * This is static method. 
 * A pointer to midgard_person is available through MidgardConnection API.
 * 
 */
extern MidgardUser *midgard_user_auth(MidgardConnection *mgd, 
				const gchar *name, const gchar *password, 
				const gchar *sitegroup, gboolean trusted);

/**
 * \ingroup midgard_user
 *
 * Check if user is a user
 *
 * \param self, MidgardUser instance
 *
 * \return TRUE if user is a user, FALSE otherwise
 *
 * If Midgard User is a user,(s)he's not an admin or root.
 */
extern gboolean midgard_user_is_user(MidgardUser *self);

/**
 * \ingroup midgard_user
 *
 * Check if user is an admin 
 *
 * \param self, MidgardUser instance
 *
 * \return TRUE if user is an admin, FALSE otherwise
 *
 * If Midgard User is an admin, (s)he's sitegroup administrator.
 */
extern gboolean midgard_user_is_admin(MidgardUser *self);

/**
 * \ingroup midgard_user
 *
 * Check if user is root
 *
 * \param self, MidgardUser instance
 *
 * \return TRUE if user is root, FALSE otherwise
 *
 * If Midgard User is root, (s)he's SG0 administrator.
 */
extern gboolean midgard_user_is_root(MidgardUser *self);

/**
 * \ingroup midgard_user
 *
 * Sets midgard_user active flag
 *
 * \param self, MidgardUser object
 * \param flag, active or inactive flag
 *
 * \return TRUE if flag has been successfully changed, FALSE otherwise
 */ 
extern gboolean midgard_user_set_active(MidgardUser *user, gboolean flag);

/**
 * \ingroup midgard_user
 *
 * Sets user's password.
 *
 * \param self, MidgardUser instance
 * \param name, login to be set
 * \param password, password to be set
 * \param hashtype, the type to generate password
 *
 * \return TRUE if password has been set , FALSE otherwise
 *
 * Available hash types:
 * 
 *  - MIDGARD_USER_HASH_LEGACY
 *  - MIDGARD_USER_HASH_LEGACY_PLAIN
 *  - MIDGARD_USER_HASH_MD5
 *  - MIDGARD_USER_HASH_PLAIN
 *  - MIDGARD_USER_HASH_SHA1
 *  - MIDGARD_USER_HASH_PAM
 */
extern gboolean midgard_user_password(MidgardUser *self, const gchar *login,
				const gchar *password, guint hashtype);

/** 
 * \ingroup midgard_user
 *
 * Get user's person 
 *
 * \param self, MidgardUser instance
 *
 * \return MgdObject or NULL
 *
 * Returned MgdObject is 'midgard-person' type.
 * NULL is returned explicitly if there's no person object 
 * associated with given MidgardUser.
 *
 */
extern MgdObject *midgard_user_get_person(MidgardUser *self);

G_END_DECLS

#endif /* MIDGARD_USER_H */
