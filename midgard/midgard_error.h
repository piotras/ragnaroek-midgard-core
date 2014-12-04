/* 
 * Copyright (C) 2006 Piotr Pokora <piotr.pokora@infoglob.pl>
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

#ifndef MIDGARD_ERROR_H
#define MIDGARD_ERROR_H

#include "midgard/midgard_connection.h"

G_BEGIN_DECLS

/**
 * \defgroup midgard_error Midgard Error
 *
 */ 

#define MGD_GENERIC_ERROR midgard_error_generic()

/**
 * \ingroup midgard_error
 *
 * MidgardErrorGeneric enum type.
 * This enum provides new generic error constants , and also keeps 
 * backward compatible ones.
 */ 

typedef enum
{
	MGD_ERR_OK = -0, /**< No error  <br /> */ 
	MGD_ERR_ERROR = -1, /**<  <br />*/ 
	MGD_ERR_ACCESS_DENIED = -2, /**< Access denied *  <br /> */
	MGD_ERR_SITEGROUP_VIOLATION = -3, /**< Resource link crosses sitegroup borders   <br /> */
	MGD_ERR_NOT_OBJECT = -4, /**< Not Midgard Object   <br /> */
	MGD_ERR_NOT_EXISTS = -5, /**< Object does not exist   <br /> */
	MGD_ERR_INVALID_PROPERTY = -6 , /**<  Invalid property <br /> */
	MGD_ERR_INVALID_NAME = -7 , /**<   <br /> */
	MGD_ERR_DUPLICATE = -8 , /**< Object already exist   <br /> */
	MGD_ERR_HAS_DEPENDANTS = -9 , /**< Object has dependants   <br /> */
	MGD_ERR_RANGE = -10, /**<   <br /> */
	MGD_ERR_NOT_CONNECTED = -11, /**< Not connected to the Midgard database   <br /> */
	MGD_ERR_SG_NOTFOUND = -12, /**< Sitegroup not found   <br /> */
	MGD_ERR_INVALID_OBJECT = -13, /**< Object not registered as Midgard Object   <br /> */ 
	MGD_ERR_QUOTA = -14, /**< Quota limit reached   <br /> */
	MGD_ERR_INTERNAL = -15, /**< Critical internal error   <br /> */
	MGD_ERR_OBJECT_NAME_EXISTS = -16, /**< Object with such name exists in tree   <br /> */
	MGD_ERR_OBJECT_NO_STORAGE = -17, /**< Storage table not defined for object   <br /> */
	MGD_ERR_OBJECT_NO_PARENT= -18, /**< Parent object in tree not defined   <br /> */
	MGD_ERR_INVALID_PROPERTY_VALUE = -19, /**< Invalid property value   <br /> */
	MGD_ERR_USER_DATA = -20, /**< Empty error message reserved for application's developers   <br /> */
	MGD_ERR_OBJECT_DELETED = -21, /**< Object deleted   <br /> */
	MGD_ERR_OBJECT_PURGED = -22, /**< Object purged   <br /> */
	MGD_ERR_OBJECT_EXPORTED = -23, /**< Object already exported   <br /> */
	MGD_ERR_OBJECT_IMPORTED = -24, /**< Object already imported   <br /> */
	MGD_ERR_MISSED_DEPENDENCE = -25, /**< Missed dependence for object   <br /> */
	MGD_ERR_TREE_IS_CIRCULAR = -26, /**< Circular reference found in object's tree <br />*/
	MGD_ERR_OBJECT_IS_LOCKED = -27 /**< Object is locked */
}MgdErrorGeneric;

/**
 * \ingroup midgard_error
 *
 * GQuark for Midgard Error. It's used by Midgard Error implementation, and 
 * probably is not needed to use by any application.
 *
 * \return MGD_GENERIC_ERROR GQuark
 */
extern GQuark midgard_error_generic(void);

/**
 * \ingroup midgard_error
 *
 * Get error message for the given error code.
 *
 * \param domain , GQuark which represents MidgardError domain.
 * \param errcode, MidgardErrorGeneric enum value.
 *
 * \return error messages which is owned by midgard-core and should not be freed.
 */ 
extern const gchar *midgard_error_string(GQuark domain, gint errcode);

/**
 * \ingroup midgard_error
 *
 * Sets internal error constant and error message.
 *
 * \param mgd, MidgardConnection instance 
 * \param domain, GQuark which represents MidgardError domain
 * \param errcode, MidgardErrorGeneric enum value
 * \param msg, a message which should be appended to string represented by errcode
 * \param ..., message argument list ( if required )
 *
 * This function sets internal error constant, and creates new error message.
 * User defined message is appended to internal one.
 * Any message created by application ( and its corresponding constant ) are destroyed 
 * and reset to MGD_ERR_OK when any API function is invoked.
 * Second \c domain parameter is optional , and can be safely defined as NULL for 
 * MGD_GENERIC_ERROR domain.
 *
 * <strong> Example: </strong>
 * @code
 *	
 *	void set_wrong_property(MidgardConnection *mgd, gchar *prop)
 *	{
 *		midgard_set_error(mgd, NULL, 
 *				MGD_ERR_INVALID_PROPERTY_VALUE,
 *				"My application doesn't accept %s property",
 *				prop);
 *	}
 * @endcode
 */ 
extern void midgard_set_error(
		MidgardConnection *mgd, GQuark domain, 
		gint errcode, const gchar *msg, ...);

/**
 * \ingroup midgard_error
 *
 * Default function for log messages.
 *
 * \param domain, domain for the given log message
 * \param level, GLogLevelFlags
 * \param msg, log message
 * \param ptr, pointer to structure which holds loglevel 
 *
 * \c ptr pointer may be a pointer to MidgardConnection or MidgardTypeHolder
 * structure. This function checks pointer type using MIDGARD_IS_CONNECTION
 * convention macro. Next midgard_connection_get_loglevel is called to get loglevel.
 * If MidgardConnection check fails , a typecast to MidgardTypeHolder is made.
 * In this case, level member is used to get loglevel.
 *
 * You are responsible to correctly set MidgardConnection or MidgardTypeHolder
 * before passing ptr argument. The main approach is to follow configuration's
 * loglevel even if MidgardConnection pointer is not yet available.
 *
 * Use midgard_connection_set_loglevel to set log level.	
 *
 */
extern void midgard_error_default_log(const gchar *domain, GLogLevelFlags level,
		const gchar *msg, gpointer ptr);

/**
 * \ingroup midgard_error
 *
 * Get GLogLevelFlags value from loglevel string.
 *
 * \param levelstring, string which should be parsed
 *
 * \return GLogLevelFlags or -1 on failure
 *
 */
extern gint midgard_error_parse_loglevel(const gchar *levelstring);

/**
 * \ingroup midgard_error 
 *
 * #define MIDGARD_ERRNO_SET(mgd, errcode)
 *
 * A simplified macro to set default error
 */ 

#define MIDGARD_ERRNO_SET(str, errcode)  \
	str->errn = errcode;  \
	midgard_set_error(str->_mgd, \
		MGD_GENERIC_ERROR, \
		errcode, \
		NULL); \
	/* g_signal_emit_by_name(str, "error"); */ \
	g_clear_error(&str->_mgd->err); 

G_END_DECLS

#endif /* MIDGARD_ERROR_H */
