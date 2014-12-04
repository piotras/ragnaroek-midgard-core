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

#ifndef MIDGARD_SCHEMA_H
#define MIDGARD_SCHEMA_H

#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>

/* Undefine XMLCALL defined in expat library needed by repligard
   and already included in midgard.h file. */
#ifdef XMLCALL
#undef XMLCALL
#endif

extern GType midgard_schema_get_type(void);

/* convention macros */
#define MIDGARD_TYPE_SCHEMA (midgard_schema_get_type())
#define MIDGARD_SCHEMA(object)  \
	(G_TYPE_CHECK_INSTANCE_CAST ((object),MIDGARD_TYPE_SCHEMA, midgard))
#define MIDGARD_SCHEMA_CLASS(klass)  \
	(G_TYPE_CHECK_CLASS_CAST ((klass), MIDGARD_TYPE_SCHEMA, MidgardSchemaClass))
#define MIDGARD_IS_SCHEMA(object)   \
	(G_TYPE_CHECK_INSTANCE_TYPE ((object), MIDGARD_TYPE_SCHEMA))
#define MIDGARD_IS_SCHEMA_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE ((klass), MIDGARD_TYPE_SCHEMA))
#define MIDGARD_SCHEMA_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS ((obj), MIDGARD_TYPE_SCHEMA, MidgardSchemaClass))

typedef struct _MidgardSchema MidgardSchema;
typedef struct _MidgardSchemaClass MidgardSchemaClass;


typedef struct _MgdSchema MgdSchema; 

/* FIXME , move MgdSchema structure to source file */
struct _MgdSchema {
	GHashTable *types;
};


/**
 *  \defgroup mgdschema  Midgard Schema
 */  

/**
 * \ingroup mgdschema
 *
 * The MidgardSchema object structure
 *
 */ 
struct _MidgardSchema {
	GObject parent;

	/* < private > */
	GHashTable *types;
};

/**
 * \ingroup mgdschema
 *
 * The MgdSchemaClass structure . 
 *
 */
struct _MidgardSchemaClass{
	GObjectClass parent;

	/* < public > */
	void 	  (*init) (MidgardSchema *self);
	gboolean  (*read_dir) (MidgardSchema *self, const gchar *dirname);
	void	  (*read_file) (MidgardSchema *self, const gchar *filename);	
	gboolean  (*type_exists) (MidgardSchema *self, const gchar *classname);
};

/**
 * \ingroup mgdschema
 * 
 * Initializes basic Midgard classes  
 * 
 * \param self MidgardSchema struct
 * 
 * Initializes global schema from global MgdObjects.xml schema file.
 * This function registers only basic classes.
 * 
 * Caller is responsible to free allocated memory using midgard_schema_destroy.
 * The memory is not freed for MidgardSchema object itslef. It is freed for all 
 * internal schema structures allocated for this object instance.
 */
extern void midgard_schema_init(MidgardSchema *self);

/**
 * \ingroup mgdschema 
 *
 * Initializes classes from schema files.
 * 
 * \param self MidgardSchema struct
 * \param dirname full directory path 
 * 
 * Initializes classes which are defined in all schema files 
 * located under dirname directory. All registered classes
 * are "appended" to self MidgardSchema structure.
 * 
 * \return TRUE when dirname directory was read and xml schema files
 * was parsed, FALSE otherwise.
 * 
 */
extern gboolean midgard_schema_read_dir(MidgardSchema *self, const gchar *dirname);

/**
 * \ingroup mgdschema
 *
 * Checks if classname exists in schema.
 *
 * \param self MidgardSchema struct
 * \classname Name of the class
 *
 * \return TRUE if classname is registered for schema, FALSE otherwise.
 */
extern gboolean midgard_schema_type_exists(MidgardSchema *self, const gchar *classname);

/**
 * \ingroup mgdschema
 *
 * Initializes classes from file
 *
 * \param self MidgardSchema struct
 * \param filename name of the file to parse
 *
 * Filename is not limited to any directory name,
 * and should be absolute path.
 * 
 */
extern void midgard_schema_read_file(MidgardSchema *self, const gchar *filename);

#endif /* MIDGARD_SCHEMA_H */
