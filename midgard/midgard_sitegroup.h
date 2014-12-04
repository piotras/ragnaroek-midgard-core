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

#ifndef MIDGARD_SITEGROUP_H
#define MIDGARD_SITEGROUP_H

/**
 * \defgroup sitegroup Midgard Sitegroup
 *
 * Midgard Sitegroup is a special Midgard Framework class designed 
 * to create virtual like databases in one physical database.
 *
 * More documentation about sitegroups:
 * http://www.midgard-project.org/documentation/concepts-sitegroups/
 * 
 * Sitegroups may be created only by Midgard Administrator. May be listed
 * and queried by any logged in person ( or even in anonymous mode ) as long 
 * as sitegroup is not created as private one. 
 */ 

#include <glib.h>
#include <glib-object.h>
#include "midgard_defs.h"
#include "midgard_connection.h"
#include "midgard_dbobject.h"

/* convention macros */
#define MIDGARD_TYPE_SITEGROUP (midgard_sitegroup_get_type())
#define MIDGARD_SITEGROUP(object)  \
	        (G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_SITEGROUP, MidgardSitegroup))
#define MIDGARD_SITEGROUP_CLASS(klass)  \
	        (G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_SITEGROUP, MidgardSitegroupClass))
#define MIDGARD_IS_SITEGROUP(object)   \
	        (G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_SITEGROUP))
#define MIDGARD_IS_SITEGROUP_CLASS(klass) \
	        (G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_SITEGROUP))
#define MIDGARD_SITEGROUP_GET_CLASS(obj) \
	        (G_TYPE_INSTANCE_GET_CLASS ((object), MIDGARD_TYPE_SITEGROUP, MidgardSitegroupClass))

typedef struct _MidgardSitegroup MidgardSitegroup;
typedef struct _MidgardSitegroup midgard_sitegroup;
typedef struct _MidgardSitegroupClass MidgardSitegroupClass;
typedef struct _MidgardSitegroupPrivate MidgardSitegroupPrivate;

/**
 * \ingroup sitegroup
 *
 * Returns the MidgardSitegroup value type.
 * Registers the type as a fundamental GType unless already registered.
 */ 
extern GType midgard_sitegroup_get_type(void);

/**
 * \ingroup sitegroup
 *
 * The opaque structure for Midgard Sitegroup object type.
 * Structure's members which hold values of object properties are
 * private and accessible only with GObject API.
 */ 
struct _MidgardSitegroup{
	GObject parent;
	MidgardDBObjectPrivate *dbpriv;
	MidgardSitegroupPrivate *priv;
	const gchar *name;
	const gchar *realm;
	const gchar *group;
	gboolean public;
	guint id;
	guint adminid;
	GObject *metadata;
};

/**
 * \ingroup sitegroup
 *
 * The opaque structure for Midgard Sitegroup class type.
 */
struct _MidgardSitegroupClass {
	GObjectClass parent;
	MidgardDBObjectPrivate *dbpriv;
};


extern MidgardSitegroup *midgard_sitegroup_new(MidgardConnection *mgd, const GValue *value);
extern gchar **midgard_sitegroup_list(MidgardConnection *mgd);
extern gboolean midgard_sitegroup_create(MidgardSitegroup *sitegroup);
extern gboolean midgard_sitegroup_update(MidgardSitegroup *sitegroup);
extern gboolean midgard_sitegroup_delete(MidgardSitegroup *sitegroup);  

#endif /* MIDGARD_SITEGROUP_H */
