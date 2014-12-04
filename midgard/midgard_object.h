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
 *   */

#ifndef MIDGARD_OBJECT_H
#define MIDGARD_OBJECT_H

/**
 * \defgroup moc Midgard Object Class 
 *
 * This part of API focuses on reflection possibilities for Midgard Object Class.
 * Every function or method which reffers to database data will be moved from here.
 *
 */

#include <glib.h>
#include <midgard/types.h>

/*
 * Returns table name used for object.
 *
 * @param klass MidgardObjectClass pointer
 *
 * @return table name
 *
 * Returned string  is a pointer to table name used for object's class.
 */ 
extern const gchar *midgard_object_class_get_table(MidgardObjectClass *klass);

/*
 * Returns table names used for object.
 * 
 * @param klass MidgardObjectClass pointer
 * 
 * @return table name 
 * 
 * Returned string is a pointer to table names used for object's class.
 * One table name is returned if  object's class uses one table, or
 * table names separated by comas otherwise
 */
extern const gchar *midgard_object_class_get_tables(MidgardObjectClass *klass);

/*
 * Returns primary field name used for object's class.
 *  
 * @param klass MidgardObjectClass pointer
 * 
 * @return field name 
 * 
 * Returned string name is a pointer to primary field name used for object's class.
 * Primary field is a field with unique value used by table ( auto_incremnented id 
 * or guid or uuid for example)
 */
extern const gchar *midgard_object_class_get_primary_field(MidgardObjectClass *klass);

/*
 * Returns table name used for property
 *
 * @param klass MidgardObjectClass
 * @param property_name name of property 
 *
 * @return table name
 *
 * Returned string is a pointer to table name used for property_name property's value.
 */
extern const gchar *midgard_object_class_get_property_table(
        MidgardObjectClass *klass, const gchar *property_name);

/*
 * Returns field name used for property name.
 * 
 * @param klass MidgardObjectClass pointer
 * @parame property_name object's property name  
 *  
 * @return field name
 * 
 * Returned string is a pointer to field name used for object's property name.
 * 
 */
extern const gchar *midgard_object_class_get_property_field(
        MidgardObjectClass *klass, const gchar *property_name);

/*
 * \ingroup moc 
 *
 * Returns MidgardObjectClass for linked object.
 * 
 * @param klass MidgardObjectClass pointer
 * @parame property_name object's property name  
 *  
 * @return linked object's class pointer
 *  
 * Returned class is a pointer to object's class which is linked to property_name
 * property and defined in schema. It's owned by midgard core so you shouldn't 
 * free it.
 */
extern  MidgardObjectClass *midgard_object_class_get_link_target(
        MidgardObjectClass *klass, const gchar *property_name);

/*
 * \ingroup moc
 *
 * Returns the name of object's primary property.
 * 
 * @param klass MidgardObjectClass pointer
 *  
 * @return primary property name
 * 
 * Returned string is a pointer to object's primary property.
 * Primary property is a property which holds value of object's primary field
 * (midgard_object_class_get_primary_field)
 */           
extern const gchar *midgard_object_class_get_primary_property(MidgardObjectClass *klass); 

/*
 * \ingroup moc
 *
 * Returns the name of object's parent property.
 *  
 * @param klass MidgardObjectClass pointer
 *
 * @return parent property name
 * 
 * Returned string is a pointer to object's parent property.
 * Parent property is a property which holds value of field which points to 
 * parent object's identifier. Parent property always points to object's 
 * identifier which is not the same type as MidgardObjectClass klass.
 */
extern const gchar *midgard_object_class_get_parent_property(MidgardObjectClass *klass);

extern const gchar *midgard_object_class_get_property_parent(MidgardObjectClass *klass);

/*
 * \ingroup moc
 *
 * Returns the name of object's up property.
 * 
 * @param klass MidgardObjectClass pointer
 * 
 * @return up property name
 * 
 * Returned string is a pointer to object's up property.
 * Up property is a property which holds value of field which points to 
 * "upper" object's identifier. Up property always points to object's
 * dentifier which is the same type as MidgardObjectClass klass.
 */
extern const gchar *midgard_object_class_get_up_property(MidgardObjectClass *klass);

extern const gchar *midgard_object_class_get_property_up(MidgardObjectClass *klass);

/**
 * \ingruop moc
 *
 * Returns array of childreen tree classes for the given MidgardObjectClass.
 *
 * \param klass , MidgardObjectClass pointer
 *
 * Returns newly allocated children tree classes' pointers. 
 * NULL is added as tha last array element. Returned array should be freed
 * after use without freeing array's elements. 
 *
 */
MidgardObjectClass **midgard_object_class_list_children(
	MidgardObjectClass *klass);

/**
 * \ingroup moc
 * 
 * Checks whether given MidgardObjectClass is multilingual.
 * 
 * \param[in] object MidgardObjectClass
 * 
 * \return TRUE if MidgardObjectClass is mutlilingual, FALSE otherwise.
 *
 **/
extern gboolean midgard_object_class_is_multilang(MidgardObjectClass *klass);

/**
 * \ingruop moc
 *
 * Returns Midgard Object identified by guid
 *
 * \param guid, guid string which identifies object
 * 
 * \return MgdObject or NULL
 */
extern MgdObject *midgard_object_class_get_object_by_guid(MidgardConnection *mgd, const gchar *guid);

/** 
 * \ingroup moc
 *
 * Undelete object identified by given guid.
 *
 * \param mgd, MidgardConnection object
 * \param guid, a guid which identifies object
 *
 *\return TRUE on success, FALSE otherwise
 *
 */
extern gboolean midgard_object_class_undelete(MidgardConnection *mgd, const gchar *guid);

extern const gchar *midgard_object_class_get_schema_value (MidgardObjectClass *klass, const gchar *name);

#endif /* MIDGARD_OBJECT_H */
