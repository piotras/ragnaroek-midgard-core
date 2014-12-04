
/* Define structures, keep it to make legacy and new code 
 * existance in better shape */

#ifndef MIDGARD_DEFS_H
#define MIDGARD_DEFS_H

#include <glib.h>

typedef struct _mgd_userinfo mgd_userinfo;
typedef struct _midgard_mysql midgard_mysql;
typedef struct _midgard_object_mysql midgard_object_mysql;
typedef struct _midgard midgard;
typedef struct _midgard_res midgard_res;
typedef struct _qlimit qlimit;
typedef struct _MgdSchemaTypeAttr MgdSchemaTypeAttr;
typedef struct MidgardTypeHolder MidgardTypeHolder;
typedef struct MidgardConnection MidgardConnection;
typedef struct _MidgardObjectClass MidgardObjectClass;

typedef struct _MidgardUser MidgardUser;

#define MIDGARD_PACKAGE_NAME "midgard"

typedef enum {
	MIDGARD_AUTHTYPE_NORMAL = 0,
	MIDGARD_AUTHTYPE_PAM,
	MIDGARD_AUTHTYPE_TRUST
} MidgardTypeAuth;

#define MIDGARD_PACKAGE_NAME "midgard"

#endif /* MIDGARD_DEFS_H */
