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
 * */

#include <config.h>
#include <stdlib.h>
#include "midgard/midgard_defs.h"
#include "midgard/midgard_metadata.h"
#include "midgard/midgard_timestamp.h"
#include "midgard/query.h"
#include "midgard/midgard_object.h"
#include "midgard_core_object.h"
#include "midgard_core_object_class.h"
#include "midgard/midgard_dbobject.h"
#include "schema.h"
#include <libxml/xmlreader.h>
#include <libxml/parserInternals.h>

enum {
    MIDGARD_METADATA_CREATOR = 1,
    MIDGARD_METADATA_CREATED,
    MIDGARD_METADATA_REVISOR,
    MIDGARD_METADATA_REVISED,
    MIDGARD_METADATA_REVISION,
    MIDGARD_METADATA_LOCKER,
    MIDGARD_METADATA_LOCKED,
    MIDGARD_METADATA_APPROVER,
    MIDGARD_METADATA_APPROVED,
    MIDGARD_METADATA_AUTHORS,
    MIDGARD_METADATA_OWNER,
    MIDGARD_METADATA_SCHEDULE_START,
    MIDGARD_METADATA_SCHEDULE_END,
    MIDGARD_METADATA_HIDDEN,
    MIDGARD_METADATA_NAV_NOENTRY,
    MIDGARD_METADATA_SIZE,
    MIDGARD_METADATA_PUBLISHED,
    MIDGARD_METADATA_SCORE,
    MIDGARD_METADATA_IMPORTED,
    MIDGARD_METADATA_EXPORTED,
    MIDGARD_METADATA_DELETED,
    MIDGARD_METADATA_ISLOCKED,
    MIDGARD_METADATA_ISAPPROVED
};

static void __set_from_xml_node(MidgardDBObject *object, xmlNode *node);

typedef struct _MidgardDBSchema MidgardDBSchema;

struct _MidgardDBSchema {
	const gchar *property;
	const gchar *field;
};

MidgardDBSchema metadatadb[] = {
	{"creator", 		"metadata_creator"		},
	{"created", 		"metadata_created"		},
	{"revisor", 		"metadata_revisor"		},
	{"revised", 		"metadata_revised"		},
	{"revision",		"metadata_revision"		},
	{"locker",		"metadata_locker"		},
	{"locked",		"metadata_locked"		},
	{"approver",		"metadata_approver"		},
	{"approved",		"metadata_approved"		},
	{"authors",		"metadata_authors"		},
	{"owner",		"metadata_owner"		},
	{"schedulestart",	"metadata_schedule_start"	},
	{"scheduleend",		"metadata_schedule_end"		},
	{"hidden",		"metadata_hidden"		},
	{"navnoentry",		"metadata_nav_noentry"		},
	{"size",		"metadata_size"			},
	{"published",		"metadata_published"		},
	{"score",		"metadata_score"		},
	{"exported",		"metadata_exported"		},
	{"imported",		"metadata_imported"		},
	{"deleted",		"metadata_deleted"		},
	
	{NULL,	NULL}
};

void midgard_core_metadata_property_attr_new(MgdSchemaTypeAttr *type_attr)
{
	guint i = 0;
	while(metadatadb[i].property != NULL) {
		
		 MgdSchemaPropertyAttr *mprop_attr = _mgd_schema_property_attr_new();
		 mprop_attr->table = g_strdup(type_attr->table);
		 mprop_attr->field = g_strdup(metadatadb[i].field);
		 mprop_attr->tablefield = g_strjoin(".", mprop_attr->table, mprop_attr->field, NULL);
		 
		 g_hash_table_insert(type_attr->metadata,
				 g_strdup(metadatadb[i].property),
				 mprop_attr); 
		 i++;
	}
}

static void
_metadata_set_property (GObject *object, guint property_id,
        const GValue *value, GParamSpec *pspec)
{
	MidgardMetadata *self = (MidgardMetadata *) object;
	MgdObject *mobject = self->private->object;

	if (mobject->private->read_only) {
		g_warning("Can not write property. Object in read only mode");
		return;
	}

	switch (property_id) {
		
		case MIDGARD_METADATA_CREATOR:
			g_free (self->private->creator);
			self->private->creator = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_CREATED:
			g_free (self->private->created);
			self->private->created = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_REVISOR:	    
			g_free (self->private->revisor);
			self->private->revisor = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_REVISED:
			g_free (self->private->revised);
			self->private->revised = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_REVISION:
			self->private->revision = g_value_get_uint (value);
			break;
			
		case MIDGARD_METADATA_LOCKER:
			g_free (self->private->locker);                
			self->private->locker = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_LOCKED:
			g_free (self->private->locked);                
			self->private->locked = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_APPROVER:
			g_free (self->private->approver);                
			self->private->approver = g_value_dup_string (value);
			break;		                                  
			
		case MIDGARD_METADATA_APPROVED:    
			g_free (self->private->approved);
			self->private->approved = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_AUTHORS:
			g_free (self->private->authors);
			self->private->authors = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_OWNER:
			g_free (self->private->owner);
			self->private->owner = g_value_dup_string (value);
			break;

		case MIDGARD_METADATA_SCHEDULE_START:
			g_free (self->private->schedule_start);
			self->private->schedule_start = g_value_dup_string (value);
			break;
		
		case MIDGARD_METADATA_SCHEDULE_END:
			g_free (self->private->schedule_end);
			self->private->schedule_end = g_value_dup_string (value);
			break;
		
		case MIDGARD_METADATA_HIDDEN:
			self->private->hidden = g_value_get_boolean (value);
			break;
			
		case MIDGARD_METADATA_NAV_NOENTRY:
			self->private->nav_noentry = g_value_get_boolean (value);
			break;
			
		case MIDGARD_METADATA_SIZE:
			self->private->size = g_value_get_uint (value);
			break;
			
		case MIDGARD_METADATA_PUBLISHED:	    
			g_free (self->private->published);
			self->private->published = g_value_dup_string (value);
			break;

		case MIDGARD_METADATA_SCORE:
			self->private->score = g_value_get_int (value);
			break;

		case MIDGARD_METADATA_EXPORTED:
			g_free (self->private->exported);
			self->private->exported = g_value_dup_string (value);
			break;
		
		case MIDGARD_METADATA_IMPORTED:
			g_free (self->private->imported);
			self->private->imported = g_value_dup_string (value);
			break;
			
		case MIDGARD_METADATA_DELETED:
			self->private->deleted = g_value_get_boolean (value);
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
			break;                  
	}
}
    
static gboolean __set_property_from_mysql_row(MidgardMetadata *self, GValue *value, GParamSpec *pspec)
{
	MgdObject *object = self->private->object;

	if (object == NULL 
			|| ((object != NULL && !G_IS_OBJECT(object))))
		return FALSE;

	if (!object->private->read_only)
		return FALSE;

	MgdSchemaPropertyAttr *attr = _midgard_core_class_get_property_attr (G_OBJECT_GET_CLASS(self), pspec->name);
	if (!attr)
		return FALSE;

	/* 0 idx is invalid for metadata object's property */
	gint col_idx = attr->property_col_idx;
	if (col_idx == 0)
		return FALSE;

	switch (G_TYPE_FUNDAMENTAL (pspec->value_type)) {

		case G_TYPE_STRING:
			g_value_set_string(value, (gchar *)object->private->_mysql_row[col_idx]);
			break;

		default:
			if (object->private->_mysql_row[col_idx] == NULL)
				return TRUE;

			/* FIXME, replace this with metadata private constants */
			if (col_idx == 23)
				self->private->lock_is_set = TRUE;
			if (col_idx == 24)
				self->private->approve_is_set = TRUE;

			if (pspec->value_type == G_TYPE_UINT)
				g_value_set_uint(value, atoi(object->private->_mysql_row[col_idx]));
			else if (pspec->value_type == G_TYPE_INT)
				g_value_set_int(value, atoi(object->private->_mysql_row[col_idx]));
			else if (pspec->value_type == G_TYPE_BOOLEAN)
				g_value_set_boolean(value, atoi(object->private->_mysql_row[col_idx]));
		
			break;
	}

	return TRUE;
}

static void
_metadata_get_property (GObject *object, guint property_id,
        GValue *value, GParamSpec *pspec)
{    
	MidgardMetadata *self =	(MidgardMetadata *) object;

	if (__set_property_from_mysql_row(self, value, pspec))
		return;

	switch (property_id) {
		
		case MIDGARD_METADATA_CREATOR: 
			g_value_set_string (value, self->private->creator);
			break;
			
		case MIDGARD_METADATA_CREATED:
			g_value_set_string (value, self->private->created);
			break;
			
		case MIDGARD_METADATA_REVISOR:
			g_value_set_string (value, self->private->revisor);
			break;
			
		case MIDGARD_METADATA_REVISED:
			g_value_set_string (value, self->private->revised);
			break;
			
		case MIDGARD_METADATA_LOCKER:
			g_value_set_string (value, self->private->locker);
			break;
			
		case MIDGARD_METADATA_LOCKED:
			g_value_set_string (value, self->private->locked);
			break;
			
		case MIDGARD_METADATA_APPROVER:
			g_value_set_string (value, self->private->approver);
			break;
			
		case MIDGARD_METADATA_APPROVED:    
			g_value_set_string (value, self->private->approved);
			break;
			
		case MIDGARD_METADATA_REVISION:             
			g_value_set_uint (value, self->private->revision);            
			break;
			
		case MIDGARD_METADATA_AUTHORS:
			g_value_set_string (value, self->private->authors);
			break;
			
		case MIDGARD_METADATA_OWNER:
			g_value_set_string (value, self->private->owner);
			break;
			
		case MIDGARD_METADATA_SCHEDULE_START:
			g_value_set_string (value, self->private->schedule_start);
			break;
			
		case MIDGARD_METADATA_SCHEDULE_END:
			g_value_set_string (value, self->private->schedule_end);
			break;
			
		case MIDGARD_METADATA_HIDDEN:
			g_value_set_boolean (value, self->private->hidden);
			break;
			
		case MIDGARD_METADATA_NAV_NOENTRY:
			g_value_set_boolean (value, self->private->nav_noentry);
			break;
			
		case MIDGARD_METADATA_SIZE:
			g_value_set_uint(value,  self->private->size);
			break;
			
		case MIDGARD_METADATA_PUBLISHED:
			g_value_set_string (value, self->private->published);
			break;

		case MIDGARD_METADATA_SCORE:
			g_value_set_int (value, self->private->score);
			break;

		case MIDGARD_METADATA_EXPORTED:
			g_value_set_string (value, self->private->exported);
			break;

		case MIDGARD_METADATA_IMPORTED:
			g_value_set_string (value, self->private->imported);
			break;

		case MIDGARD_METADATA_DELETED:
			g_value_set_boolean (value, self->private->deleted);
			break;

		case MIDGARD_METADATA_ISLOCKED:
			g_value_set_boolean (value, self->private->is_locked);
			break;

		case MIDGARD_METADATA_ISAPPROVED:
			g_value_set_boolean (value, self->private->is_approved);
			break;
			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
			break;
	}
}

static void 
_metadata_instance_init(GTypeInstance *instance, gpointer g_class) 
{	
	MidgardMetadata *self = (MidgardMetadata *)instance;
	
	/* allocate private data */
	self->private = g_new(MidgardMetadataPrivate, 1);
	self->private->creator = NULL;
	self->private->created = NULL;
	self->private->revised = NULL;
	self->private->revisor = NULL;
	self->private->locker = NULL;
	self->private->locked = NULL;
	self->private->approved = NULL;
	self->private->approver = NULL;
	self->private->authors = NULL;
	self->private->owner = NULL;
	self->private->revision = 0;
	self->private->schedule_start = NULL;
	self->private->schedule_end = NULL;
	self->private->hidden = FALSE;
	self->private->nav_noentry = FALSE;
	self->private->size = 0;
	self->private->published = NULL;
	self->private->score = 0;
	self->private->exported = NULL;
	self->private->imported = NULL;
	self->private->deleted = FALSE;
	self->private->is_locked = FALSE;
	self->private->lock_is_set = FALSE;
	self->private->is_approved = FALSE;
	self->private->approve_is_set = FALSE;

	self->private->object = NULL;
}

static void 
_metadata_object_finalize(GObject *object) 
{
	g_assert(object);
	
	MidgardMetadata *self = (MidgardMetadata *)object;
	
	/* Free private data */
	g_free(self->private->creator);
	g_free(self->private->created);
	g_free(self->private->revised);
	g_free(self->private->revisor);
	g_free(self->private->locker);
	g_free(self->private->locked);
	g_free(self->private->approved);
	g_free(self->private->approver);
	g_free(self->private->authors);
	g_free(self->private->owner);
	g_free(self->private->schedule_start);
	g_free(self->private->schedule_end);
	g_free(self->private->published);
	g_free(self->private->exported);
	g_free(self->private->imported);
	
	g_free(self->private);
}

/* Set properties when object is created */
void midgard_metadata_set_create(MgdObject *object)
{
	MgdObject *person = (MgdObject *)object->mgd->person;
	gchar *person_guid;
	g_object_get(G_OBJECT(person), "guid", &person_guid, NULL);

	GValue tval = {0, };
	g_value_init(&tval, midgard_timestamp_get_type());
	midgard_timestamp_set_time(&tval, time(NULL));
	
	MidgardMetadata *metadata = (MidgardMetadata *) object->metadata;
	metadata->private->creator = person_guid;    
	metadata->private->created = midgard_timestamp_dup_string(&tval);
}

/* Set properties when object is updated */
void midgard_metadata_set_update(MgdObject *object){

    MgdObject *person = (MgdObject *)object->mgd->person;
    gchar *person_guid;
    g_object_get(G_OBJECT(person), "guid", &person_guid, NULL);

    GValue tval = {0, };
    g_value_init(&tval, midgard_timestamp_get_type());
    midgard_timestamp_set_time(&tval, time(NULL));

    MidgardMetadata *metadata = (MidgardMetadata *) object->metadata;
    metadata->private->revisor = person_guid;
    metadata->private->revised = midgard_timestamp_dup_string(&tval);
    metadata->private->revision += 1; 
}

/* Initialize class */
static void
_metadata_class_init (gpointer g_class,
        gpointer g_class_data){
    
    GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
    MidgardMetadataClass *klass = MIDGARD_METADATA_CLASS (g_class); 
    GParamSpec *pspec;

    gobject_class->set_property = _metadata_set_property;
    gobject_class->get_property = _metadata_get_property;
    gobject_class->finalize = _metadata_object_finalize;
    
    /* Register signals */
    /* Create signal which should be emitted before MgdObject is created */
    klass->set_created = midgard_metadata_set_create;
    klass->signal_set_created = g_signal_new("set_created",
            G_TYPE_FROM_CLASS(g_class),
            G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
            G_STRUCT_OFFSET (MidgardMetadataClass, set_created),
            NULL, /* accumulator */
            NULL, /* accu_data */
            NULL, /* marshaller */
            G_TYPE_NONE,
            0);
    
    /* Create signal which should be emitted before MgdObject is updates */
    klass->set_updated = midgard_metadata_set_update;
    klass->signal_set_updated = g_signal_new("set_updated",
            G_TYPE_FROM_CLASS(g_class),
            G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
            G_STRUCT_OFFSET (MidgardMetadataClass, set_updated),
            NULL, /* accumulator */
            NULL, /* accu_data */
            NULL, /* marshaller */
            G_TYPE_NONE,
            0);
       
	MgdSchemaPropertyAttr *prop_attr;
	MgdSchemaTypeAttr *type_attr = _mgd_schema_type_attr_new();
	
	/* Register properties */        

	/* creator */
	pspec = g_param_spec_string ("creator",
			"metadata_creator",
			"The person who created object's record",
			NULL /* We can set midgard admin */,
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_CREATOR,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 2;
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("metadata_creator");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"creator"), prop_attr);

	/* created */
	pspec = g_param_spec_string ("created",
			"metadata_created",
			"The date when object's record was created",
			"0000-00-00 00:00",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_CREATED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 3;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_created");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"created"), prop_attr);

	/* revisor */
	pspec = g_param_spec_string ("revisor",
			"metadata_revisor",
			"The person who revised object's record",
			NULL, 
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_REVISOR,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 4;
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("metadata_revisor");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"revisor"), prop_attr);

	/* revised */
	/*pspec = g_param_spec_gtype ("revised",
			"rev",
			"",
			MGD_TYPE_TIMESTAMP,
			G_PARAM_READWRITE); */
	pspec = g_param_spec_string ("revised",
			"metadata_revised",
			"The date when object's record was revised",
			"0000-00-00 00:00",
			G_PARAM_READABLE); 
	/*pspec = g_param_spec_internal (MGD_TYPE_TIMESTAMP,
			"revised",
			"",
			"",
			G_PARAM_READWRITE);*/
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_REVISED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 5;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_revised");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"revised"), prop_attr);

	/* revision */
	pspec = g_param_spec_uint ("revision",
			"metadata_revision",
			"Object's record revision number",
			0, G_MAXUINT32, 0, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_REVISION,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 6;
	prop_attr->gtype = MGD_TYPE_UINT;
	prop_attr->field = g_strdup("metadata_revision");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"revision"), prop_attr);

	/* locker */
	pspec = g_param_spec_string ("locker",
			"metadata_locker",
			"The person who (un)locked object's record",
			NULL,
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_LOCKER,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 7;
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("metadata_locker");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"locker"), prop_attr);

	/* locked */
	pspec = g_param_spec_string ("locked",
			"metadata_locked",
			"The date when object's record  was (un)locked",
			"" /* We can set midgard admin */,
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_LOCKED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 8;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_locked");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"locked"), prop_attr);

	/* approver */
	pspec = g_param_spec_string ("approver",
			"metadata_approver",
			"The person who (un)approved object's record",
			NULL,
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_APPROVER,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 9;
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("metadata_approver");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"approver"), prop_attr);

	/* approved */
	pspec = g_param_spec_string ("approved",
			"metadata_approved",
			"The date when object's record was (un)approved",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_APPROVED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 10;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_approved");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"approved"), prop_attr);

	/* authors */
	pspec = g_param_spec_string ("authors",
			"metadata_authors",
			"The person who is an author",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_AUTHORS,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 11;
	prop_attr->gtype = MGD_TYPE_STRING;
	prop_attr->field = g_strdup("metadata_authors");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"authors"), prop_attr);

	/* owner */
	pspec = g_param_spec_string ("owner",
			"metadata_owner",
			"Group's guid which is an owner of the object",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_OWNER,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 12;
	prop_attr->gtype = MGD_TYPE_GUID;
	prop_attr->field = g_strdup("metadata_owner");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"owner"), prop_attr);

	/* schedulestart */
	pspec = g_param_spec_string ("schedulestart",
			"metadata_schedule_start",
			"",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_SCHEDULE_START,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 13;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_schedule_start");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"schedulestart"), prop_attr);

	/* scheduleend */
	pspec = g_param_spec_string ("scheduleend",
			"metadata_schedule_end",
			"",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_SCHEDULE_END,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 14;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_schedule_end");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"scheduleend"), prop_attr);

	/* hidden */
	pspec = g_param_spec_boolean ("hidden",
			"metadata_hidden",
			"",
			FALSE,
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_HIDDEN,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 15;
	prop_attr->gtype = MGD_TYPE_BOOLEAN;
	prop_attr->field = g_strdup("metadata_hidden");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"hidden"), prop_attr);

	/* nav no entry */
	pspec = g_param_spec_boolean ("navnoentry",
			"metadata_nav_noentry",
			"",
			FALSE,
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_NAV_NOENTRY,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 16;
	prop_attr->gtype = MGD_TYPE_BOOLEAN;
	prop_attr->field = g_strdup("metadata_nav_noentry");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"navnoentry"), prop_attr);

	/* size */
	pspec = g_param_spec_uint ("size",
			"metadata_size",
			"Object's size in bytes",
			0, G_MAXUINT32, 0, G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_SIZE,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 17;
	prop_attr->gtype = MGD_TYPE_UINT;
	prop_attr->field = g_strdup("metadata_size");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"size"), prop_attr);

	/* published */
	pspec = g_param_spec_string ("published",
			"metadata_published",
			"",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_PUBLISHED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 18;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_published");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"published"), prop_attr);

	/* score */
	pspec = g_param_spec_int ("score",
			"metadata_score",
			"Object's record score",
			G_MININT32, G_MAXINT32, 0, G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,      
			MIDGARD_METADATA_SCORE,        
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 22;
	prop_attr->gtype = MGD_TYPE_INT;
	prop_attr->field = g_strdup("metadata_score");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"score"), prop_attr);

	/* exported */
	pspec = g_param_spec_string ("exported",
			"metadata_exported",
			"",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_EXPORTED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 19;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_exported");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"exported"), prop_attr);

	/* imported */
	pspec = g_param_spec_string ("imported",
			"metadata_imported",
			"",
			"",
			G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_IMPORTED,
			pspec);
    	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 20;
	prop_attr->gtype = MGD_TYPE_TIMESTAMP;
	prop_attr->field = g_strdup("metadata_imported");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"imported"), prop_attr);

	pspec = g_param_spec_boolean ("deleted",
			"metadata_deleted",
			"",
			FALSE,
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_DELETED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 21;
	prop_attr->gtype = MGD_TYPE_BOOLEAN;
	prop_attr->field = g_strdup("metadata_deleted");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"deleted"), prop_attr);

	pspec = g_param_spec_boolean ("isapproved",
			"metadata_isapproved",
			"",
			FALSE,
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_ISAPPROVED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 24;
	prop_attr->gtype = MGD_TYPE_BOOLEAN;
	prop_attr->field = g_strdup("metadata_isapproved");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"isapproved"), prop_attr);

	pspec = g_param_spec_boolean ("islocked",
			"metadata_islocked",
			"",
			FALSE,
			G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
			MIDGARD_METADATA_ISLOCKED,
			pspec);
	prop_attr = _mgd_schema_property_attr_new();
	prop_attr->property_col_idx = 23;
	prop_attr->gtype = MGD_TYPE_BOOLEAN;
	prop_attr->field = g_strdup("metadata_islocked");
	prop_attr->table = NULL;
	prop_attr->tablefield = NULL;
	g_hash_table_insert(type_attr->prophash,
			g_strdup((gchar *)"islocked"), prop_attr);

	/* Initialize private member */
	klass->dbpriv = g_new(MidgardDBObjectPrivate, 1);
	klass->dbpriv->storage_data = type_attr;

	klass->dbpriv->set_from_xml_node = __set_from_xml_node;
}


GType midgard_metadata_get_type (void)
{    
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (MidgardMetadataClass),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			_metadata_class_init,   /* class_init */			
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			sizeof (MidgardMetadata),
			0,      /* n_preallocs */
			_metadata_instance_init    /* instance_init */
		};
		type = g_type_register_static (MIDGARD_TYPE_DBOBJECT,
				"midgard_metadata",
				&info, 0);
	}
	return type;
}

MidgardMetadata *midgard_metadata_new(MgdObject *object) 
{
	g_assert(object->mgd);

	MidgardMetadata *self = 
		(MidgardMetadata *) g_object_new(MIDGARD_TYPE_METADATA, NULL);
	self->private->object = object;

	return self;
}

MidgardMetadata *midgard_object_metadata_get(MgdObject *object){

    MidgardMetadata *mm = midgard_metadata_new(object);
    if(mm)
        return mm;

    return NULL;
}

gchar *midgard_metadata_get_sql(MidgardMetadata *object){

    g_assert(object);
    
    guint i = 0;    
    const gchar *nick, *prop_str;
    GValue pval = {0, };
    GParamSpec *prop;
   
    static gchar *props[] = { "locker", "locked", "approver", "approved", 
        "authors", "owner", "schedulestart", "scheduleend", "hidden",
        "navnoentry", "published", "score", NULL};

    GString *sql = g_string_new("");
    
    while (props[i] != NULL) {
        
        prop = g_object_class_find_property(
                G_OBJECT_GET_CLASS(G_OBJECT(object)), 
                props[i]);
        
        nick = g_param_spec_get_nick (prop);
        if((g_ascii_strcasecmp(nick, "") != 0)){

            g_value_init(&pval,prop->value_type);
            g_object_get_property(G_OBJECT(object), props[i], &pval);

            switch (prop->value_type) {
                
                case G_TYPE_STRING:
		    prop_str = g_value_get_string(&pval);
		    if(prop_str == NULL) prop_str = "";
                    g_string_append_printf(sql,
                            ",%s='%s' ",
                            nick, prop_str);
                    break;

                case G_TYPE_BOOLEAN:
                    g_string_append_printf(sql,
                            ",%s=%d ",
                            nick, g_value_get_boolean(&pval));
                    break;

                case G_TYPE_UINT:
                    g_string_append_printf(sql,
                            ",%s=%d ",
                            nick, g_value_get_uint(&pval));
                    break;

		case G_TYPE_INT:
		    g_string_append_printf(sql,
				    ",%s=%d ",
				    nick, g_value_get_int(&pval));
		    break;
            }
            g_value_unset(&pval);
        }
        i++;
    }

    return g_string_free(sql, FALSE);
}

xmlNode *__dbobject_xml_lookup_node(xmlNode *node, const gchar *name)
{
	xmlNode *cur = NULL;

	for (cur = node; cur; cur = cur->next) {
		
		if (cur->type == XML_ELEMENT_NODE) {
			
			if(g_str_equal(cur->name, name))
					return cur;
		}
	}

	return NULL;
}

gchar *__get_node_content_string(xmlNode *node)
{
	if(!node)
		return NULL;

	gchar *content = NULL;
	content = (gchar *)xmlNodeGetContent(node);

	if(content == NULL)
		return NULL;

	xmlParserCtxtPtr parser = xmlNewParserCtxt();
	xmlChar *decoded =
		 xmlStringDecodeEntities(parser,
				 (const xmlChar *) content,
				 XML_SUBSTITUTE_REF,
				 0, 0, 0);

	xmlFreeParserCtxt(parser);
	g_free(content);
	
	return (gchar*)decoded;
}

gint __get_node_content_int(xmlNode *node)
{
	if(!node)
		return 0;

	gchar *content = NULL;
	content = (gchar *)xmlNodeGetContent(node);

	if(content == NULL)
		return 0;

	gint rv = (gint)atoi((gchar *)content);

	g_free(content);

	return rv;
}

guint __get_node_content_uint(xmlNode *node)
{
	if(!node)
		return 0;

	gchar *content = NULL;
	content = (gchar *)xmlNodeGetContent(node);

	if(content == NULL)
		return 0;

	guint rv = (guint)atoi((gchar *)content);

	g_free(content);

	return rv;
}

gboolean __get_node_content_bool(xmlNode *node)
{
	if(!node)
		return FALSE;

	gchar *content = NULL;
	content = (gchar *)xmlNodeGetContent(node);

	if(content == NULL)
		return 0;

	gboolean rv = (gboolean)atoi((gchar *)content);

	g_free(content);

	return rv;
}

static void __set_from_xml_node(MidgardDBObject *object, xmlNode *node)
{
	g_assert(object != NULL);

	MidgardMetadata *self = MIDGARD_METADATA(object);
	xmlNode *lnode = NULL;	

	lnode = __dbobject_xml_lookup_node(node, "creator");
	self->private->creator = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "created");
	self->private->created = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "revisor");
	self->private->revisor = __get_node_content_string(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "revised");
	self->private->revised = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "revision");
	self->private->revision = __get_node_content_uint(lnode);

	lnode = __dbobject_xml_lookup_node(node, "locker");
	self->private->locker = __get_node_content_string(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "locked");
	self->private->locked = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "approver");
	self->private->approver = __get_node_content_string(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "approved");
	self->private->approved = __get_node_content_string(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "authors");
	self->private->authors = __get_node_content_string(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "owner");
	self->private->owner = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "schedulestart");
	self->private->schedule_start = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "scheduleend");
	self->private->schedule_end = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "hidden");
	self->private->hidden = __get_node_content_bool(lnode);

	lnode = __dbobject_xml_lookup_node(node, "navnoentry");
	self->private->nav_noentry = __get_node_content_bool(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "size");
	self->private->size = __get_node_content_uint(lnode);

	lnode = __dbobject_xml_lookup_node(node, "published");
	self->private->published = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "score");
	self->private->score = __get_node_content_int(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "imported");
	self->private->imported = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "exported");
	self->private->exported = __get_node_content_string(lnode);

	lnode = __dbobject_xml_lookup_node(node, "deleted");
	self->private->deleted = __get_node_content_bool(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "islocked");
	self->private->is_locked = __get_node_content_bool(lnode);
	
	lnode = __dbobject_xml_lookup_node(node, "isapproved");
	self->private->is_approved = __get_node_content_bool(lnode);
}
