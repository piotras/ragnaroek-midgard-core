/* 
 * Copyright (C) 2005 Piotr Pokora <piotr.pokora@infoglob.pl>
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
#include <glib.h>
#include <glib-object.h>
#include "midgard/types.h"
#include "midgard/midgard_legacy.h"

gchar * midgard_object_serialize(MgdObject *mobj)
{

  GParamSpec **pspec; 
  guint propn, i;
  GString *so;
  GValue pval = {0,};
  gchar *rso;
  const gchar *pcchar;
    

  if ((pspec = g_object_class_list_properties((GObjectClass *) mobj->klass, &propn)) == NULL )
    return NULL;

  so = g_string_new("<type name=");
  g_string_append_printf(so, "\"%s\">\n", mobj->cname);

  for (i = 0; i < propn; i++) {

    g_value_init(&pval,pspec[i]->value_type);

    g_string_append(so, "\t<property name=");
    g_string_append_printf(so, "\"%s\"", pspec[i]->name);
    g_string_append(so, " value=");

    switch (pspec[i]->value_type) {

      case G_TYPE_STRING:
        
        if ((pcchar = g_strdup(g_value_get_string(&pval))) == NULL ) 
          pcchar = "";         
        g_string_append_printf(so, "\"%s\"/>\n", pcchar);
        break;

      case G_TYPE_UINT:

        g_string_append_printf(so, "\"%d\"/>\n", g_value_get_uint(&pval));
        break;

    }

    g_value_unset(&pval);
   
  }
  
  g_string_append(so, "</type>\n");
  
  rso = g_string_free(so, FALSE);

  return rso;
}

