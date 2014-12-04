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

#include <config.h>
#include "midgard/pool.h"
#include <glib.h>

struct midgard_pool {
        GSList *entries;
};

midgard_pool *mgd_alloc_pool(void) {
        midgard_pool *pool = g_new(midgard_pool, 1);
        pool->entries = NULL;
        return pool;
}

void mgd_clear_pool(midgard_pool *pool) 
{
        g_assert(pool != NULL);
	
	if(!pool->entries)
		return;
	GSList *entry; 
	
	for( entry = pool->entries; entry != NULL; entry = entry->next)	{

		if(entry->data)
			g_free(entry->data);

		entry->data = NULL;
	}

	if(pool->entries)
		g_slist_free(pool->entries);

	pool->entries = NULL;
}

void mgd_free_from_pool(midgard_pool *pool, void *ptr) {
        g_assert(pool != NULL);
        g_assert(ptr != NULL);
        pool->entries = g_slist_remove(pool->entries, ptr);
        g_free(ptr);
}

void mgd_free_pool(midgard_pool *pool) {
        g_assert(pool != NULL);
        mgd_clear_pool(pool);
        g_free(pool);
}

void *mgd_alloc(midgard_pool *pool, int len) {
        g_assert(pool != NULL);
        g_assert(len >= 0);
        void *data = g_malloc(len);
        pool->entries = g_slist_prepend(pool->entries, data);
        return data;
}

char *mgd_stralloc(midgard_pool *pool, int len) {
        g_assert(pool != NULL);
        g_assert(len >= 0);
        char *str = g_malloc(len + 1);
        pool->entries = g_slist_prepend(pool->entries, str);
        return str;
}


char *mgd_strdup(midgard_pool * pool, const char *str) {
        g_assert(pool != NULL);
        g_assert(str != NULL);
        char *strcopy = g_strdup(str);
        pool->entries = g_slist_prepend(pool->entries, strcopy);
        return strcopy;
}

char *mgd_strndup(midgard_pool * pool, const char *str, int len) {
        g_assert(pool != NULL);
        g_assert(str != NULL);
        g_assert(len >= 0);
        char *strcopy = g_strndup(str, len);
        pool->entries = g_slist_prepend(pool->entries, strcopy);
        return strcopy;
}
