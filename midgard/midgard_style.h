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

#ifndef MIDGARD_STYLE_H
#define MIDGARD_STYLE_H

#include "midgard/midgard_object.h"

/* Midgard style structure */

/**
 * \defgroup style Midgard Style 
 * 
 * The opaque Midgard Style type.
 *
 * \li stack Midgard Styles stack
 * \li styles Hash table with which contains all registered styles
 * \li page_elements a pointer to Hash table with elements assigned to page
 * \li style_elements a pointer to  Hash table with elements assigned to currently used style
 */
typedef struct _MgdStyle{
        GList *stack;
        GHashTable *styles;
        GHashTable *page_elements;
        GHashTable *style_elements;
}MgdStyle;

/**
 * \ingroup style
 *
 * Creates a new MgdStyle
 *
 * \param mgd midgard struct connection handler 
 *
 * \return a new MgdStyle
 */
extern MgdStyle *midgard_style_new(midgard *mgd);

/**
 * \ingroup style 
 *
 * Load page and its elements
 *
 * \param style MgdStyle struct
 * \param object MgdObject instance
 *
 * This function expects midgard_page type object as second parameter.
 *
 * \return  TRUE if the page elements was loaded, FALSE otherwise
 */
extern gboolean midgard_style_load_page(MgdStyle *style, MgdObject *object);

/**
 * \ingroup style
 *
 * Register style 
 *
 * \param mgd midgard struct connection handler
 * \param path full path to style
 * \param style MgdStyle assigned to request
 * 
 * \return \c TRUE if the style was registered successfully  (\c AND or \c OR), \c FALSE otherwise
 * 
 * Registers style in Midgard internal styles' stack.
 * path parameter expects full to the style. If prefixed with 'file://' style from file  system is set.
 * In such case path is expected to be relative.
 */ 
extern gboolean midgard_style_register(midgard *mgd, const gchar *name, MgdStyle *style);

/**
 * \ingroup style 
 *
 * Get style or page element
 *
 * \param style MgdStyle struct pointer
 * \param name style or page element to get
 *
 * \return pointer to style or page element value
 *
 * Returned value is only a pointer and caller shouldn't free it.
 */ 
extern gchar *midgard_style_get_element(MgdStyle *style, const gchar *name);

/**
 * \ingroup style 
 *
 * Check if element is loaded
 *
 * \param style MgdStyle struct pointer
 * \param name style or page element to check 
 *
 * \return TRUE if the elementis already  loaded, FALSE otherwise
 */
extern gboolean midgard_style_is_element_loaded(MgdStyle *style, const gchar *name);

/**
 * \ingroup style 
 *
 * Free midgard style
 *
 * \param style MgdStyle struct pointer
 *
 * Frees memory allocated for midgard style.
 */ 
extern void midgard_style_free(MgdStyle *style);	
	
#endif /* MIDGARD_STYLE_H */
