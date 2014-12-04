/* $Id: select_db.h 7813 2005-04-07 09:15:55Z piotras $
 *
 * midgard/select_db.h: database switching helper routines
 *
 * Copyright (C) 1999 Jukka Zitting <jukka.zitting@iki.fi>
 * Copyright (C) 2000 The Midgard Project ry
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef MGD_SELECT_DB_H
#define MGD_SELECT_DB_H

/* EEH placed here to avoid superlong log lines in apaches error log */
#line 27 "select_db.h"
#include "midgard/midgard.h"

static int mgd_select_database(request_rec *r, midgard_database_connection *dbc,
      midgard_request_config * rcfg, int assert_conn)
{
   if (dbc && dbc == rcfg->database.current && dbc->handle && dbc->handle->mgd) {
		g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, 
         "Midgard: select_database kept current database %s",
         dbc->name);
      rcfg->mgd = dbc->handle->mgd;

      return assert_conn
         ? mgd_assert_db_connection(rcfg->mgd,
            dbc->handle->hostname, dbc->name, dbc->handle->username, dbc->handle->password)
         : 1;
   }

   if (dbc->handle->current != dbc || dbc->handle->mgd == NULL) {
      if (dbc->handle->mgd == NULL) {
//         dbc->handle->mgd = mgd_connect(
         dbc->handle->mgd = mgd_connect(dbc->handle->hostname,
            dbc->name, dbc->handle->username, dbc->handle->password);

         if (dbc->handle->mgd != NULL)  {
           g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
               "Midgard: select_database opened database %s",
               dbc->name);
         }
         else {
           g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
               "Midgard: failed to open database '%s' for user '%s'",
                  dbc->name, dbc->handle->username);
            return 0;
         }
      }
      else {
         if (mgd_select_db(dbc->handle->mgd, dbc->name)
               || (mgd_assert_db_connection(dbc->handle->mgd,
                     dbc->handle->hostname, dbc->name, dbc->handle->username, dbc->handle->password)
                  && mgd_select_db(dbc->handle->mgd, dbc->name))) {
           g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
                "Midgard: switching to %s", dbc->name);
         }
         else {
           g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
               "Midgard: switching to %s failed", dbc->name);
            /* DONT GIVE UP try disconnect and reconnect!*/  
             mgd_close( dbc->handle->mgd);
             return assert_conn
                  ? mgd_assert_db_connection(rcfg->mgd,
                    dbc->handle->hostname, dbc->name, dbc->handle->username, dbc->handle->password)
                   : 1;
                        
              
             
         }
      }
      dbc->handle->current = dbc;
   }
   else {
     g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
         "Midgard: select_database reused open (%s)",
			      dbc->name);
   }

   rcfg->database.current = dbc;
   rcfg->mgd = dbc->handle->mgd;
   return assert_conn
      ? mgd_assert_db_connection(rcfg->mgd,
         dbc->handle->hostname, dbc->name, dbc->handle->username, dbc->handle->password)
      : 1;
}

#endif

