#ifndef MGD_PAGERESOLVE_H
#define MGD_PAGERESOLVE_H

#ifdef WIN32
#include <windows.h>
#endif
#include <glib.h>
#include <stdio.h>
#include "midgard/midgard_defs.h"

typedef struct _midgard_style {
        GList *stack;
        GHashTable *styles;
        GHashTable *page_elements;
        GHashTable *style_elements;
}midgard_style;

typedef enum {
   MGD_FOUND_NONE,
   MGD_FOUND_PAGE,
   MGD_FOUND_BLOB
} mgd_resolve_result_t;

typedef struct {
   int active;
   long page;
   long style;
   int addslash;
   int auth_required;
   int self_len;
   int author;
   int owner;
   mgd_resolve_result_t found;
   int blob;
} mgd_page_t;

typedef struct {
   long host;
   long style;
   long page;
   long sitegroup;
   int lang;
   int auth_required;
   int prefix_length;
   int found;
} mgd_host_t;

typedef void (*mgd_store_style_elt_cb)(const char *name, const char *value, GHashTable * table);
typedef const char* (*mgd_get_style_elt_cb)(const char *name, GHashTable * table);
typedef void (*mgd_pathinfo_cb)(const char *name, void *userdata);

int mgd_find_host(midgard *mgd,
   const char *hostname, int port, const char *uri, int null_prefix,
   mgd_host_t *parsed);

/* EEH: if this returns false, or returns true but active is set, you might still want to test
   for pageblobs. The page id will be valid.
*/
int mgd_parse_uri(midgard *mgd, mgd_host_t *host, char *uri,
   mgd_page_t *result,
   mgd_pathinfo_cb pathinfo_cb,
   GArray *path,
   void *userdata);

void mgd_load_styles(midgard *mgd, GArray* path, long host, long style,
                     mgd_store_style_elt_cb style_elt_cb, void *userdata,
                     long cached_page);

extern void midgard_collect_style_elements(midgard *mgd, long host, long style, 
  mgd_store_style_elt_cb style_elt_cb,
  void *userdata);

typedef void (*mgd_parser_output_cb)(char *buffer, int len, void *userdata);

typedef struct {
   struct {
      mgd_parser_output_cb func;
      void                 *userdata;
   } output;

   struct {
      mgd_get_style_elt_cb func;
      void                 *userdata;
   } get_element;
} mgd_parser_itf_t;

void mgd_preparse_buffer(const char *buffer, mgd_parser_itf_t *itf);
void mgd_preparse_file(FILE *f, mgd_parser_itf_t *itf);

void mgd_cache_touch(midgard *mgd, long page);
int mgd_cache_is_current(midgard *mgd, long host, long page, long style);

#define MGD_CACHE_ELT_PAGE    0
#define MGD_CACHE_ELT_STYLE   1

extern midgard_style *midgard_style_new(void);

extern gboolean midgard_style_register(midgard *mgd, const gchar *name,
                midgard_style *mgdstyle);

extern gboolean midgard_style_register_from_dir(midgard *mgd, const gchar *stackname,
                                const gchar *dirname, GHashTable *style_stack);

extern void midgard_style_get_elements(midgard * mgd, glong style, mgd_store_style_elt_cb style_elt_cb, GHashTable * table);

void mgd_cache_add(midgard *mgd, long host, long page, long style, int type, long id);

const gchar * midgard_pc_get_element(const gchar * name, GHashTable * table);

void  midgard_pc_set_element(const gchar * name, const gchar * value, GHashTable * table);

void midgard_pc_output(char *buffer, int len, void * userdata);

extern gboolean midgard_style_get_elements_from_dir(midgard *mgd, const gchar *name,
               mgd_store_style_elt_cb style_elt_cb, GHashTable *table);

#endif
