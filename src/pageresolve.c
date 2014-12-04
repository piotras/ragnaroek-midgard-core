#include <config.h>
#include "midgard/midgard_legacy.h"
#include "midgard/pageresolve.h"
#include "midgard/midgard_datatypes.h"
#include <string.h>
#ifdef WIN32
#include <windows.h>
#endif

#include "midgard_mysql.h"

int mgd_find_host(midgard *mgd,
   const char *hostname, int port, const char *uri, int null_prefix,
   mgd_host_t *parsed)
{
midgard_res *res;

   parsed->host = 0;
   parsed->style = 0;
   parsed->page = 0;
   parsed->lang = 0;
   parsed->sitegroup = 0;
   parsed->auth_required = 0;
   parsed->prefix_length = 0;
   parsed->found = 0;

  /* valgrind reports leak here , however res is released */
   res = mgd_ungrouped_select(mgd,
      "id,style,root,lang,info&1,Length(prefix)"
      ",sitegroup",
      "host",
      "name=$q AND port in (0,$d) AND"
      " ( ($d <> 0 AND prefix = '')"
      "  OR ($d = 0 AND prefix=Left($q,Length(prefix)) AND"
      "      IF($d>=Length(prefix),MID($q,1+Length(prefix),1) IN ('','/'),0)=1"
      "     )"
      " )"
      " AND online <> 0 AND host.metadata_deleted=0",
      "prefix DESC, port DESC",
      hostname, port, null_prefix, null_prefix, uri, strlen(uri), uri
      );

	if (!res || mgd_rows(res) == 0 || !mgd_fetch(res)) {
      if (res) mgd_release(res);
		return 0;
   }

   parsed->host = atoi(mgd_colvalue(res, 0));
   parsed->style = atoi(mgd_colvalue(res, 1));
   parsed->page = atoi(mgd_colvalue(res, 2));
   parsed->lang = atoi(mgd_colvalue(res, 3));
   parsed->auth_required = atoi(mgd_colvalue(res, 4));
   parsed->prefix_length = atoi(mgd_colvalue(res, 5));
   parsed->sitegroup = atoi(mgd_colvalue(res, 6));


	mgd_release(res);
   parsed->found = 1;
   return 1;
}

void __collect_page_elements_from_res(midgard *mgd, long host, long page, midgard_res *res, GHashTable *element_cache,  mgd_store_style_elt_cb callback, void *userdata, long caching_page) {
    
    const char *name;
    const char *value;
    long id;

    if (!res)
        return;

    while (mgd_fetch(res)) {
        id = atol(mgd_colvalue(res, 0));
        name = mgd_colvalue(res, 1);
        value = mgd_colvalue(res, 2);

        /* skip storing the element if it's allready defined */
        if (g_hash_table_lookup(element_cache, name) != NULL) { continue; }

        g_hash_table_insert(element_cache, g_strdup(name), g_strdup(""));
        //g_hash_table_insert(mgd->elements, g_strdup(name), g_strdup(value));

        mgd_cache_add(mgd, host, caching_page, 0, MGD_CACHE_ELT_PAGE, id);

        callback(name, value, userdata);
    }
    mgd_release(res);
}


void _collect_page_elements(midgard *mgd, long host, long page, int inherited_only,
    GHashTable *element_cache,
    mgd_store_style_elt_cb callback, void *userdata,
    long caching_page)
{
    midgard_res *res;

    if (callback == NULL) return;

    res = mgd_ungrouped_select(mgd, 
            "pageelement.id AS id,name,value",
            "pageelement, pageelement_i",
            "page=$d AND pageelement.id=pageelement_i.sid AND pageelement_i.lang=$d $s"
	    " AND pageelement.sitegroup IN (0, $d) AND pageelement.metadata_deleted=0 ",
            NULL,
            page, mgd->lang, inherited_only ? "AND info&1<>0" : "", mgd_sitegroup(mgd));

    __collect_page_elements_from_res(mgd, host, page, res, 
            element_cache, callback, userdata, caching_page);
  
    if (mgd->lang > 0) { 
        res = mgd_ungrouped_select(mgd, 
                "pageelement.id AS id,name,value", 
                "pageelement, pageelement_i", 
                "page=$d AND pageelement.id=pageelement_i.sid AND pageelement_i.lang=$d $s"
		" AND pageelement.sitegroup IN (0, $d) AND pageelement.metadata_deleted=0 ",
                NULL,
                page, mgd_get_default_lang(mgd),
		inherited_only ? "AND info&1<>0" : "", 
		mgd_sitegroup(mgd));

        __collect_page_elements_from_res(mgd, host, page, res,
                element_cache, callback ,userdata, caching_page);
    }
}

void __style_load_from_res(midgard *mgd, long host, long cache_for_page, long style, midgard_res *res, GHashTable *element_cache, mgd_store_style_elt_cb callback, void *userdata) 
{

  const char *name;
  const char *value;
  long id;

  if (!res)
    return;

         while (mgd_fetch(res)) {
            id = atol(mgd_colvalue(res, 0));
            name = mgd_colvalue(res, 1);
            value = mgd_colvalue(res, 2);

            /* skip storing the element if it's allready defined */
            if (g_hash_table_lookup(element_cache, name) != NULL) { continue; }

            g_hash_table_insert(element_cache, g_strdup(name), g_strdup(""));
            //g_hash_table_insert(mgd->elements, g_strdup(name), g_strdup(value));
            mgd_cache_add(mgd, host, cache_for_page, style, MGD_CACHE_ELT_STYLE, id);
            callback(name, value, userdata);
         }
         mgd_release(res);
      }


void mgd_style_load(midgard *mgd, long host, long cache_for_page, long style,
   GHashTable *element_cache,
   mgd_store_style_elt_cb callback, void *userdata)
{

    midgard_res *res;
    if (callback == NULL) return;

    while (style) {
        res = mgd_ungrouped_select(mgd, 
                "element.id AS id,name,value",
                "element, element_i", 
                "style=$d AND element.id=element_i.sid AND element_i.lang=$d"
		" AND element.sitegroup IN (0, $d)  AND element.metadata_deleted=0 ", 
                NULL, style, mgd->lang, mgd_sitegroup(mgd));
        
        __style_load_from_res(mgd, host, cache_for_page, 
                style, res, element_cache, callback, userdata);
    
        if (mgd->lang > 0) {
            res = mgd_ungrouped_select(mgd, 
                    "element.id AS id,name,value", 
                    "element, element_i", 
                    "style=$d AND element.id=element_i.sid AND element_i.lang=$d"
		    " AND element.sitegroup IN (0, $d) AND element.metadata_deleted=0 ", 
                    NULL, style, 
		    mgd_get_default_lang(mgd), 
		    mgd_sitegroup(mgd));
            
            __style_load_from_res(mgd, host, cache_for_page, 
                    style, res, element_cache, callback, userdata);
        }
        
        style = mgd_idfield(mgd, "up", "style", style);
    }
}

int mgd_find_blob_global(midgard *mgd, mgd_page_t *page, mgd_host_t *host, char* uri) 
{    
    return 0;
}


int mgd_find_blob(midgard *mgd, mgd_page_t *page, char *blobname)
{
	return 0;
}

int mgd_parse_uri(midgard *mgd, mgd_host_t *host, char *uri,
   mgd_page_t *result,
   mgd_pathinfo_cb pathinfo_cb,
   GArray *path,
   void *userdata)
{
midgard_res *res;
char *tmp_uri;
char *basename, *ext;
int uri_len;
char *dir;
long tmp;
char *blob = NULL;

   result->style = host->style;
   result->active = 0;
   result->page = host->page;
   result->auth_required = host->auth_required;
   result->author = 0;
   result->found = MGD_FOUND_NONE;
   result->blob = 0;

   g_array_set_size(path, 0);

   g_array_append_val(path, host->page);

   basename = strrchr(uri + host->prefix_length, '/');
   ext = (basename != NULL) ? strrchr(basename + 1, '.') : NULL;
   result->addslash = (basename == NULL)
                     || (basename[1] != '\0' && ext == NULL);

   if (ext && strcmp(ext, ".html") == 0) {
      uri_len = ext - uri - host->prefix_length;
      tmp_uri = mgd_strndup(mgd->pool, uri + host->prefix_length, uri_len);
   } else {
      tmp_uri = mgd_strdup(mgd->pool, uri + host->prefix_length);
      uri_len = strlen(tmp_uri);
   }

   if (!result->auth_required)
      result->auth_required = mgd_idfield(mgd, "info&1", "page", host->page);
   result->active = mgd_idfield(mgd, "info&2", "page", host->page);
   tmp = mgd_idfield(mgd, "style", "page", host->page);
   if (tmp != 0) result->style = tmp;

   /*[eeh]  We always have a root page */
   result->found = MGD_FOUND_PAGE;
   for (dir = strtok(tmp_uri, "/");
         dir != NULL;
         dir = strtok(NULL, "/")) {
      if (dir[0] == '\0') { continue; }

     res = mgd_ungrouped_select(mgd, "id,style,info&1,info&2,author,owner", "page",
         "up=$d AND name=$q  AND page.metadata_deleted=0 ", NULL, result->page, dir);

      if (!res || !mgd_fetch(res)) {
         if (res) mgd_release(res);
         result->found = MGD_FOUND_NONE;
         break;
      }

      result->page = atoi(mgd_colvalue(res, 0));

      if ((tmp = atoi(mgd_colvalue(res, 1))) != 0) {
         result->style = tmp;
         /*[eeh]  don't reset path of pages so lower inherited page
          *       elements 'show through'
          */
      }

      if (!result->auth_required) result->auth_required = atoi(mgd_colvalue(res, 2));

      result->active = atoi(mgd_colvalue(res, 3));
      result->author = atol(mgd_colvalue(res, 4));
      result->owner = atol(mgd_colvalue(res, 4));
      g_array_append_val(path, result->page);

   }

   result->self_len = host->prefix_length + (dir != NULL ? (dir - tmp_uri) : uri_len);

   /*[eeh]  page not found or active page, try blob */
   if (result->found == MGD_FOUND_NONE || result->active) {
      blob = uri + result->self_len;
      if (mgd_find_blob(mgd, result, blob)) return 1;
   }

   /* page not found, try attachment host */
   if (mgd->ah_prefix && result->found == MGD_FOUND_NONE && host->page == result->page) {
     if (mgd_find_blob_global(mgd, result, host, uri)) return 1;
   }
   
   if (result->found == MGD_FOUND_NONE && !result->active) {
      return 0;
   }

   if (result->active) {
      if (dir != NULL) result->addslash = 0;
      result->found = MGD_FOUND_PAGE;
      if (pathinfo_cb != NULL) {
         while (dir != NULL) {
            if (dir[0] != '\0') pathinfo_cb(dir, userdata);
            dir = strtok(NULL, "/");
         }
      }
   }

   return 1;
}

gboolean element_cache_free(gpointer key, gpointer value, gpointer userdata)
{
  g_free(key);
  return TRUE;
}

void mgd_load_styles(midgard *mgd, GArray* path, 
        long host, long style,
        mgd_store_style_elt_cb style_elt_cb, void *userdata, long cached_page)
{
    int i;
    midgard_res *res;
    long page;
    const char *title;
    const char *content;
    GHashTable *element_cache = NULL;
    long parent;

    if (style_elt_cb == NULL || (cached_page == 0 && (path == NULL || path->len == 0))) return;

    if (cached_page != 0) { 
        page = cached_page; 
    } else { 
        page = g_array_index(path, long, path->len - 1); 
    }

    res = mgd_ungrouped_select(mgd, 
            "title,content", 
            "page_i", 
            "sid=$d AND lang=$d", 
            NULL, page, mgd->lang);
   
    if (!res || !mgd_fetch(res)) { 
        if (res)
            mgd_release(res);
        
        res = mgd_ungrouped_select(mgd, 
                "title,content", 
                "page_i", 
                "sid=$d AND lang=$d", 
                NULL, page, mgd_get_default_lang(mgd));
        
        if (!res || !mgd_fetch(res)) { 
       /* EEH: This should never happen */

            if (res) mgd_release(res);
            return;
        }
    }

    title = mgd_colvalue(res, 0);
    content = mgd_colvalue(res, 1);
    style_elt_cb("title", title, userdata);
    style_elt_cb("content", content, userdata);

    mgd_release(res);
    
    if (cached_page != 0) {
        
        /* Get style elements with default lang(mgd->default_lang) */
        res = mgd_ungrouped_select(mgd,
                "element.name,element_i.value", 
                "element,element_i,cache",
                "element.id=element_i.sid AND "
                "element_i.lang=$d AND cache.page=$d "
                " AND cache.type=$d AND cache.id=element.id "
                " AND cache.style=element.style"
		" AND element.sitegroup IN (0, $d)  AND element.metadata_deleted=0 ",
                NULL,
		mgd_get_default_lang(mgd),
                cached_page, MGD_CACHE_ELT_STYLE, mgd_sitegroup(mgd));

        if (res) {
            while (mgd_fetch(res)) style_elt_cb(mgd_colvalue(res, 0), mgd_colvalue(res, 1), userdata);
            mgd_release(res);
        }
      
        /* Get style elements with lang already being set (mgd->lang != 0) */
        if (mgd->lang != 0) {
            
            res = mgd_ungrouped_select(mgd,
                    "element.name,element_i.value",
                    "element,element_i,cache",
                    "element.id=element_i.sid AND "
                    "element_i.lang=$d AND "
                    "cache.page=$d AND cache.type=$d AND "
                    "cache.id=element.id AND "
                    "cache.style = element.style"
		    " AND element.sitegroup IN (0, $d) AND element.metadata_deleted=0 ",
                    NULL, mgd->lang, cached_page, MGD_CACHE_ELT_STYLE, mgd_sitegroup(mgd));

            if (res) {
                while (mgd_fetch(res)) style_elt_cb(mgd_colvalue(res, 0), mgd_colvalue(res, 1), userdata);
                mgd_release(res);
            }
        }

        /* Get page elements with default lang (mgd->lang = 0) */
        res = mgd_ungrouped_select(mgd,
                "pageelement.name,pageelement_i.value",
                "pageelement,pageelement_i,cache",
                "pageelement.id=pageelement_i.sid AND "
                "pageelement_i.lang=$d AND cache.page=$d AND "
                "cache.type=$d AND cache.id=pageelement.id"
		" AND pageelement.sitegroup IN (0, $d)  AND pageelement.metadata_deleted=0 ",
                NULL, 
		mgd_get_default_lang(mgd),
		cached_page, MGD_CACHE_ELT_PAGE, mgd_sitegroup(mgd));

        if (res) {
            while (mgd_fetch(res)) style_elt_cb(mgd_colvalue(res, 0), mgd_colvalue(res, 1), userdata);
            mgd_release(res);
        }

        /* Get page elements with lang already being set (mgd->lang != 0) */
        if (mgd->lang != 0) {
            
            res = mgd_ungrouped_select(mgd,
                    "pageelement.name,pageelement_i.value",
                    "pageelement,pageelement_i,cache",
                    "pageelement.id=pageelement_i.sid AND "
                    "pageelement_i.lang=$d AND cache.page=$d "
                    "AND cache.type=$d AND cache.id=pageelement.id "
		    "AND pageelement.sitegroup IN (0, $d)  AND pageelement.metadata_deleted=0 ",
                    NULL, mgd->lang, cached_page, MGD_CACHE_ELT_PAGE, mgd_sitegroup(mgd));
        
            if (res) {
                while (mgd_fetch(res)) style_elt_cb(mgd_colvalue(res, 0), mgd_colvalue(res, 1), userdata);
                mgd_release(res);
            }
        }
        
        return;
    }

    element_cache = midgard_hash_strings_new();
    g_hash_table_insert(element_cache, g_strdup("title"), g_strdup(""));
    g_hash_table_insert(element_cache, g_strdup("content"), g_strdup(""));
	  
    
    _collect_page_elements(mgd, host, page, 0, element_cache, style_elt_cb, userdata, page);

    while ((parent = mgd_idfield(mgd, "up", "page", g_array_index(path, long, 0)))) {
        g_array_prepend_val(path, parent);     
    }

    for (i = path->len - 2; i >= 0; i--) {
        _collect_page_elements(mgd, host, 
                g_array_index(path, long, i), 1, element_cache, style_elt_cb, userdata, page);
    }
    
    mgd_style_load(mgd, host, page, style, element_cache, style_elt_cb, userdata);
    
    g_hash_table_destroy(element_cache);
}

void mgd_cache_touch(midgard *mgd, long page)
{
	GString *command;
	long sitegroup = (long)mgd_sitegroup(mgd);
	
	if (sitegroup == 0) {
		mysql_query(mgd->msql->mysql, "DELETE FROM cache");
	} else {
		command = g_string_new("");
		g_string_append_printf(command, "DELETE FROM cache WHERE sitegroup=%ld", sitegroup);
		gchar *tmpstr = g_string_free(command, FALSE);	
		mysql_query(mgd->msql->mysql, tmpstr);
		g_free(tmpstr);
	}
}


int mgd_cache_is_current (midgard *mgd, long host, long page, long style)
{
	midgard_res *res;
	int is_current;
	/*[eeh] page, type, id, sitegroup */

	/* It's too early at this moment to parse the whole style tree.
	 * Style id is ignored. In most cases cache is folled by inherited style being set. */
	res = mgd_ungrouped_select (mgd, "page", "cache", 
			"host=$d and page=$d AND lang=$d LIMIT 1", NULL , host, page, mgd_lang(mgd));
	/* res = mgd_ungrouped_select (mgd, "page", "cache", 
			"host=$d and page=$d AND lang=$d LIMIT 1", NULL , host, page, style, mgd_lang(mgd)); */

	if (!res && mgd_lang (mgd) > 0) {	
		/* Do language 0 fallback, if previous query returned nothing */
		res = mgd_ungrouped_select (mgd, "page", "cache", 
				"host=$d and page=$d AND lang=$d LIMIT 1", NULL , host, page, 0);
	}

     	is_current = (res != NULL && mgd_fetch(res));
     	if (res != NULL) mgd_release(res);
     	return is_current;
}


void mgd_cache_add(midgard *mgd, long host, long page, long style, int type, long id)
{
	g_assert(mgd != NULL);
	
	GString *command = g_string_new("");
	g_string_append_printf(command,
			"INSERT INTO cache (host,page,style,type,id,lang,sitegroup) VALUES (%ld, %ld, %ld, %d, %ld, %d, %ld)",
			host, page, style, type, id, mgd_lang(mgd),
			(long)mgd_sitegroup(mgd));
	gchar *tmpstr = g_string_free(command, FALSE);
	mysql_query(mgd->msql->mysql, tmpstr);
	g_free(tmpstr);
}


void midgard_collect_style_elements(midgard *mgd, long host, long style, mgd_store_style_elt_cb style_elt_cb, void *userdata)
{
  midgard_res *res;  
  GHashTable *elements;
  
  if (style_elt_cb == NULL) 
    return;
  elements = g_hash_table_new(g_str_hash, g_str_equal);
  
  res = mgd_ungrouped_select(mgd,
    "element.name,element_i.value", "element,element_i",
    "element.id=element_i.sid AND element.style=$d"
    " AND element.sitegroup IN (0, $d)  AND element.metadata_deleted=0 ",
    NULL, 
    style, mgd_sitegroup(mgd));

  if (res) {
    while (mgd_fetch(res)) {
      style_elt_cb(mgd_colvalue(res, 0), mgd_colvalue(res, 1), userdata);
    }
    mgd_release(res);
  }

  g_hash_table_foreach_remove(elements, element_cache_free, NULL);
  g_hash_table_destroy(elements);


}

/* Get element from table hash */
const gchar * midgard_pc_get_element(const gchar * name, GHashTable * table)
{
	gchar *value;
	
	return (const gchar*) g_hash_table_lookup(table, name);
}


/* Add element to table hash */
void  midgard_pc_set_element(const gchar * name, const gchar * value, GHashTable * table)
{
    g_hash_table_insert(table, g_strdup(name), g_strdup(value));
}


void midgard_pc_output(char *buffer, int len, void *userdata)
{
  FILE *f = (FILE *) userdata;

  fwrite (buffer, sizeof (char), len, f);
}


void midgard_style_get_elements(midgard * mgd, glong style, 
        mgd_store_style_elt_cb style_elt_cb, GHashTable *table)
{
    g_assert(mgd);
    
    midgard_res *res;
                
    if (style_elt_cb == NULL || !style)
        return;

    /* Get style elements with default (mgd->lang = 0) */
    res = mgd_ungrouped_select(mgd,
            "element.name,element_i.value", "element,element_i",
            "element.id=element_i.sid AND element_i.lang=$d AND element.style=$d"
	    " AND element.sitegroup IN (0, $d)  AND element.metadata_deleted=0 ",
            NULL, 
	    mgd_get_default_lang(mgd), 
	    style, mgd_sitegroup(mgd), NULL);

    if (res) {
        while (mgd_fetch(res)) style_elt_cb(mgd_colvalue(res, 0), mgd_colvalue(res, 1), table);
        mgd_release(res);
    }
    
    /* Get style elements with lang already being set (mgd->lang != 0) */
    if (mgd->lang != 0) {
        res = mgd_ungrouped_select(mgd,
                "element.name,element_i.value", "element,element_i,cache",
                "element.id=element_i.sid AND element_i.lang=$d AND element.style=$d"
		" AND element.sitegroup IN (0, $d) AND element.metadata_deleted=0 ",
                NULL, mgd->lang, style, mgd_sitegroup(mgd), NULL);
        
        if (res) {
            while (mgd_fetch(res)) style_elt_cb(mgd_colvalue(res, 0), mgd_colvalue(res, 1), table);
            mgd_release(res);
        }
    } /* Get all elements from parent style(s) */

    do {
    
        style = mgd_idfield(mgd, "up", "style", style);
        midgard_style_get_elements(mgd, style, style_elt_cb, table);
        
    } while (style != 0);

    return;
}

gboolean midgard_style_get_elements_from_dir(midgard *mgd, const gchar *name,
        mgd_store_style_elt_cb style_elt_cb, GHashTable *table)
{
    gchar *content, **ffname, *fpfname = NULL;
    const gchar *fname = NULL;
    GDir *dir;
    
    dir = g_dir_open(name, 0, NULL);
    if (dir != NULL) {

        /* Read files form directory */
        while ((fname = g_dir_read_name(dir)) != NULL) {
   
            fpfname = g_strconcat( name, "/", fname, NULL);
            
            /* Get filename without extension. */ 
            ffname = g_strsplit(fname, ".", -1);

            /* Get file content and set as value in hash table ,
             * filename without extension is a key */
            if (g_file_get_contents(fpfname,  &content, NULL, NULL) == TRUE) {
    
                if(!g_hash_table_lookup(table, ffname[0])) {
                    g_hash_table_insert(table, g_strdup(ffname[0]), g_strdup(content));

                }

                g_free(content);
                g_strfreev(ffname);
            }

        }
        
        g_dir_close(dir);
        return TRUE;
    }
    
    return FALSE;
}

gboolean midgard_style_register(midgard *mgd, const gchar *name, midgard_style *mgdstyle)
{
        g_assert(mgd);
        g_assert(name != NULL);
        g_assert(mgdstyle);
       
        /* guint id; */
	return FALSE;
        
        /*
        GHashTable *rstyles = g_hash_table_lookup(mgdstyle->styles, stackname);
        
        if(rstyles == NULL){
                g_warning("User style %s already registered", stackname);
                return FALSE;
        }

        mgdstyle->style_elements = midgard_hash_strings_new();

        MgdObject *object = midgard_object_new_by_id(mgd, "midgard_style", (gpointer) guid);
        if (object == NULL)
                return FALSE;
        
        g_object_get(G_OBJECT(object), "id", &id, NULL);      
                
        midgard_style_get_elements(mgd, id, midgard_pc_set_element, mgdstyle->style_elements);

        g_hash_table_insert(mgdstyle->styles, g_strdup((gchar *)stackname), 
                        mgdstyle->style_elements);
             
        mgdstyle->stack = g_list_prepend(mgdstyle->stack, (gpointer) mgdstyle->style_elements);
        
        g_object_unref(object);
        
        return TRUE;    
        */
}

void midgard_style_move_prev(midgard_style *style)
{
        g_assert(style != NULL);

        style->stack = style->stack->prev;       
}

void midgard_style_move_next(midgard_style *style)
{
        g_assert(style != NULL);

        style->stack = style->stack->next;
}

gchar *midgard_style_get_element(midgard_style *style, const gchar *name)
{
        g_assert(style != NULL);
        
        gchar *value;
        
        /* Get element from page elements which do not change during request */
        if((value = g_hash_table_lookup(style->page_elements, name)) != NULL)
                return value;

        /* Get style elements from top of the stack */
        if(style->stack == NULL)
                return "";

        if((value = g_hash_table_lookup((GHashTable *) g_list_nth_data(style->stack, 0),
                                        name )) != NULL)
                return value;

        return "";                                        
                
}

gboolean midgard_style_register_from_dir(midgard *mgd, const gchar *stackname,
                const gchar *dirname, GHashTable *style_stack)
{
        g_assert(mgd);
        g_assert(dirname);
        g_assert(style_stack);
        
        if(g_hash_table_lookup(style_stack, stackname)){
                g_warning("User style %s already registered", stackname);
                return FALSE; 
        }
       
        GHashTable *style_hash = midgard_hash_strings_new();
       
        midgard_style_get_elements_from_dir(mgd, stackname,
                        midgard_pc_set_element, style_hash);

        g_hash_table_insert(style_stack, g_strdup((gchar *)stackname), style_hash);
        
        return TRUE;                        
}
