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

#include <midgard/midgard_error.h>
#include "midgard_core_object.h"
#include "midgard/midgard_datatypes.h"
#include <sys/types.h>
#include <unistd.h>

GQuark midgard_error_generic(void)
{
	static GQuark q = 0;
	if (q == 0)
		q = g_quark_from_static_string ("midgard-generic-error-quark");
	return q;
}

const gchar *midgard_error_string(GQuark domain, gint errcode)
{
	switch(errcode){

		case MGD_ERR_OK:
			return _("MGD_ERR_OK");
			break;

		case MGD_ERR_ACCESS_DENIED:
			return _("Access Denied");
			break;
		
		case MGD_ERR_SITEGROUP_VIOLATION:
			return _("Resource link crosses sitegroup borders");
			break;

		case MGD_ERR_NOT_OBJECT:
			return _("Not Midgard Object");
			break;

		case MGD_ERR_NOT_EXISTS:
			return _("Object does not exist");
			break;
	
		case MGD_ERR_INVALID_NAME:
			return _("Invalid characters in object's name");
			break;

		case MGD_ERR_DUPLICATE:
			return _("Object already exist");
			break;
			
		case MGD_ERR_HAS_DEPENDANTS:
			return _("Object has dependants");
			break;
			
		case MGD_ERR_RANGE:
			return _("Date range error"); /* FIXME , replace with invalid date range or format */
			break;

		case MGD_ERR_NOT_CONNECTED:
			return _("Not connected to the Midgard database");
			break;

		case MGD_ERR_SG_NOTFOUND:
			return _("Sitegroup not found");
			break;

		case MGD_ERR_INVALID_OBJECT:
			return _("Object not registered as Midgard Object");
			break;
			
		case MGD_ERR_QUOTA:
			return _("Quota limit reached");
			break;
			
		case MGD_ERR_INTERNAL:
			return _("Critical internal error");
			break;

		case MGD_ERR_OBJECT_NAME_EXISTS:
			return _("Object with such name exists in tree");
			break;
			
		case MGD_ERR_OBJECT_NO_STORAGE:
			return _("Storage table not defined for object");
			break;

		case MGD_ERR_OBJECT_NO_PARENT:
			return _("Parent object in tree not defined");
			break;

		case MGD_ERR_INVALID_PROPERTY_VALUE:
			return _("Invalid property value.");
			break;

		case MGD_ERR_INVALID_PROPERTY:
			return _("Invalid property.");
			break;

		case MGD_ERR_USER_DATA:
			return "";
			break;

		case MGD_ERR_OBJECT_DELETED:
			return _("Object deleted");
			break;

		case MGD_ERR_OBJECT_PURGED:
			return _("Object purged");
			break;

		case MGD_ERR_OBJECT_EXPORTED:
			return _("Object already exported");
			break;

		case MGD_ERR_OBJECT_IMPORTED:
			return _("Object already imported");
			break;

		case MGD_ERR_MISSED_DEPENDENCE:
			return _("Missed dependence for object.");
			break;

		case MGD_ERR_TREE_IS_CIRCULAR:
			return _("Circular reference found in object's tree");
			break;
	
		case MGD_ERR_OBJECT_IS_LOCKED:
			return _("Object is locked");
			break;

		default:
			return _("Undefined error");
					
	}		
}

static gchar* _midgard_error_format(const gchar *msg, va_list args)
{
	if(!msg)
		return g_strdup("");

	gchar *new_string = 
		g_strdup_vprintf(msg, args);

	return new_string;
}

void midgard_set_error(
		MidgardConnection *mgd, GQuark domain, gint errcode, const gchar *msg, ...)
{
	g_assert(mgd != NULL);

	if(!domain) 
		domain = MGD_GENERIC_ERROR;
	
	if(mgd->errstr)
		g_free(mgd->errstr);

	gchar *new_msg;
	va_list args;

	va_start(args, msg);
	new_msg =  _midgard_error_format(msg, args);
	va_end(args);	

	g_clear_error(&mgd->err);

	/* watch out! midgard 1.7 and midcom needs MGD_ERR_OK string.
	 * Keep string formatters together */
	mgd->err = g_error_new(domain, errcode,
			"%s%s", 
			midgard_error_string(domain, errcode),
			new_msg);	
	g_free(new_msg);

	if(errcode != MGD_ERR_OK)
		g_signal_emit_by_name(mgd, "error");

	mgd->errnum = errcode;
	mgd->errstr = g_strdup(mgd->err->message);	

	return;		
}

/* Default function for log messages. */
void midgard_error_default_log(const gchar *domain, GLogLevelFlags level,
		const gchar *msg, gpointer ptr)
{
	gchar *level_ad = NULL;
	guint mlevel;
	GIOChannel *channel = NULL;
	MidgardConnection *mgd;
	MidgardTypeHolder *holder;

	if(ptr == NULL) {

		mlevel = G_LOG_LEVEL_WARNING;
	
	} else {
		
		if(MIDGARD_IS_CONNECTION((MidgardConnection *) ptr)) {
			
			mgd = MIDGARD_CONNECTION(ptr);
			mlevel = midgard_connection_get_loglevel(mgd);
			if(mgd->priv->config != NULL && mgd->priv->config->private != NULL)
				channel = mgd->priv->config->private->log_channel;
		
		} else {
			
			holder = (MidgardTypeHolder *) ptr;
			mlevel = holder->level;
		}
	}

	switch (level) {
		case G_LOG_FLAG_RECURSION:
			level_ad =  "RECURSION";
			break;
			
		case G_LOG_FLAG_FATAL:
			level_ad = "FATAL! ";
			break;
			
		case G_LOG_LEVEL_ERROR:
			level_ad =  "ERROR";
			break;
			
		case G_LOG_LEVEL_CRITICAL:
			level_ad = "CRITICAL ";
			break;
			
		case G_LOG_LEVEL_WARNING:
			level_ad =  "WARNING";
			break;
			
		case G_LOG_LEVEL_MESSAGE:
			level_ad = "m";
			break;
		
		case G_LOG_LEVEL_INFO:
			level_ad = "info";
			break;
			
		case G_LOG_LEVEL_DEBUG:
			level_ad = "debug";
			break;

		default:
			level_ad = "midgard-unknown-log-level";
			break;
			
	}

	if (mlevel >= level) {
	
		GString *logstr = g_string_new("");
		g_string_append_printf(logstr,
				"%s (pid:%ld):(%s):%s\n",
				domain,
				(unsigned long)getpid(),
				level_ad,
				msg);
	
		gchar *tmpstr = g_string_free(logstr, FALSE);

		if(channel) {

			g_io_channel_write_chars(channel,
					(const gchar *)tmpstr,
					-1, NULL, NULL);
			g_io_channel_flush(channel, NULL);
			
		} else {

			fprintf(stderr, "%s", tmpstr);
			fflush(stderr);
		}

		g_free(tmpstr);
	}
}

/* Get GLogLevelFlags value from loglevel string */
gint midgard_error_parse_loglevel(const gchar *levelstring)
{
	g_assert(levelstring != NULL);

	guint level = 0, mask = 0;
	guint len = strlen(levelstring);
	gchar *newlevel = g_ascii_strdown(levelstring, len);

	if(g_str_equal(newlevel, "error")) 
		level = G_LOG_LEVEL_ERROR;
	if(g_str_equal(newlevel, "critical"))
		level = G_LOG_LEVEL_CRITICAL;
	if(g_str_equal(newlevel, "warn"))
		level = G_LOG_LEVEL_WARNING;
	if(g_str_equal(newlevel, "warning"))
		level = G_LOG_LEVEL_WARNING;
	if(g_str_equal(newlevel, "message"))	
		level = G_LOG_LEVEL_MESSAGE;
	if(g_str_equal(newlevel, "info"))
		level = G_LOG_LEVEL_INFO;
	if(g_str_equal(newlevel, "debug"))
		level = G_LOG_LEVEL_DEBUG;

	g_free(newlevel);
	
	switch(level){

		case G_LOG_LEVEL_ERROR:
		case G_LOG_LEVEL_CRITICAL:
		case G_LOG_LEVEL_WARNING:
		case G_LOG_LEVEL_MESSAGE:
		case G_LOG_LEVEL_INFO:
		case G_LOG_LEVEL_DEBUG:
			for (; level && level > G_LOG_FLAG_FATAL; level >>= 1) {
				mask |= level;
			}
			return  mask;
			break;

		default:
			return -1;
	}
	
	return -1;	
}
