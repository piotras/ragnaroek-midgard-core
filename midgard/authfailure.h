#ifndef MGD_AUTHFAILURE_H
#define MGD_AUTHFAILURE_H

#include <glib.h>

typedef enum {
        MGD_AUTH_ANONYMOUS     = -0, /*< nick=Anonymous login >*/
        MGD_AUTH_NOT_CONNECTED = -1, /*< nick=Not connected to midgard database >*/
        MGD_AUTH_INVALID_NAME  = -2, /*< nick=Invalid characters in username >*/
        MGD_AUTH_REAUTH        = -3, /*< nick=Stacked re-authentication not allowed >*/
        MGD_AUTH_SG_NOTFOUND   = -4, /*< nick=Sitegroup does not exist >*/
        MGD_AUTH_DUPLICATE     = -5, /*< nick=Username found more than once >*/
        MGD_AUTH_NOTFOUND      = -6, /*< nick=Username not found >*/
        MGD_AUTH_INVALID_PWD   = -7  /*< nick=Invalid password >*/
} MidgardAuthFailure;

#define VALID_MGD_AUTH_CODE(x) (((x) <= 0) && ((x) >= -7))

extern const gchar *mgd_authfailure_msg[];

#endif
