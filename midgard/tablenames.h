#ifndef MIDGARD_TABLENAMES_H
#define MIDGARD_TABLENAMES_H

#include <glib.h>

#define MIDGARD_OBJECT_COUNT         26
#define mgd_table_id_valid(id) (((id) >= 1) && ((id) <= 26))
typedef enum {
   MIDGARD_OBJECT_ARTICLE          = 1,  /*< nick=BOTH(article) >*/
   MIDGARD_OBJECT_BLOBS            = 2,  /*< nick=INT(blobs)EXT(attachment) >*/
   MIDGARD_OBJECT_ELEMENT          = 3,  /*< nick=BOTH(element) >*/
   MIDGARD_OBJECT_EVENT            = 4,  /*< nick=BOTH(event) >*/
   MIDGARD_OBJECT_EVENTMEMBER      = 5,  /*< nick=BOTH(eventmember) >*/
   MIDGARD_OBJECT_FILE             = 6,  /*< nick=BOTH(file) >*/
   MIDGARD_OBJECT_GRP              = 7,  /*< nick=INT(grp)EXT(group) >*/
   MIDGARD_OBJECT_HISTORY          = 8,  /*< nick=BOTH(history) >*/
   MIDGARD_OBJECT_HOST             = 9,  /*< nick=BOTH(host) >*/
   MIDGARD_OBJECT_IMAGE            = 10, /*< nick=BOTH(image) >*/
   MIDGARD_OBJECT_LANGUAGE         = 11, /*< nick=BOTH(language) >*/
   MIDGARD_OBJECT_MEMBER           = 12, /*< nick=BOTH(member) >*/
   MIDGARD_OBJECT_PAGE             = 13, /*< nick=BOTH(page) >*/
   MIDGARD_OBJECT_PAGEELEMENT      = 14, /*< nick=BOTH(pageelement) >*/
   MIDGARD_OBJECT_PAGELINK         = 15, /*< nick=BOTH(pagelink) >*/
   MIDGARD_OBJECT_PERSON           = 16, /*< nick=BOTH(person) >*/
   MIDGARD_OBJECT_PREFERENCE       = 17, /*< nick=BOTH(preference) >*/
   MIDGARD_OBJECT_QUOTA            = 18, /*< nick=BOTH(quota) >*/
   MIDGARD_OBJECT_RECORD_EXTENSION = 19, /*< nick=INT(record_extension)EXT(parameter) >*/
   MIDGARD_OBJECT_REPLIGARD        = 20, /*< nick=BOTH(repligard) >*/
   MIDGARD_OBJECT_SITEGROUP        = 21, /*< nick=BOTH(sitegroup) >*/
   MIDGARD_OBJECT_SNIPPET          = 22, /*< nick=BOTH(snippet) >*/
   MIDGARD_OBJECT_SNIPPETDIR       = 23, /*< nick=BOTH(snippetdir) >*/
   MIDGARD_OBJECT_STYLE            = 24, /*< nick=BOTH(style) >*/
   MIDGARD_OBJECT_TOPIC            = 25, /*< nick=BOTH(topic) >*/
   MIDGARD_OBJECT_UNSET            = 26  /*< nick=BOTH(unset) >*/
} mgd_tableid_t;

extern const gchar *mgd_table_name[];
extern const gchar *mgd_table_extname[];
#endif
