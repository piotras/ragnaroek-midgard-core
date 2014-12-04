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

#include <midgard/midgard.h>
#include <midgard/midgard_legacy.h>
#include <libxml/parser.h> 
#include <libxml/tree.h>
#include <libxml/xmlreader.h>
#include <libxml/parserInternals.h>
#include <sys/stat.h>
#include "midgard/midgard_datatypes.h"

#include "midgard_core_object.h"
#include "midgard/midgard_blob.h"

static const gchar *MIDGARD_OBJECT_HREF = "http://www.midgard-project.org/midgard_object/1.8";

static void _write_nodes(GObject *object, xmlNodePtr node)
{
	g_assert(object);
	g_assert(node);

	guint prop_n;
	MgdObject *mgdobject = (MgdObject *)object;
	MidgardReflectionProperty *mrp = NULL;
	MidgardObjectClass *klass = NULL;
	GParamSpec **pspec = g_object_class_list_properties(
			G_OBJECT_GET_CLASS(G_OBJECT(object)), &prop_n);
	
	if(MIDGARD_IS_OBJECT(object)) {
		klass = 
			MIDGARD_OBJECT_GET_CLASS(object);
		if(klass)
			mrp = midgard_reflection_property_new(klass);
	}

	GValue pval = {0, }, *lval;
	GString *gstring;
	gchar *strprop = NULL;
	xmlChar *escaped;
	guint i;
	xmlNodePtr op_node = NULL;
	const gchar *linktype;
	MidgardCollector *mc;
	guint _uint;
	gint _int;
	gint object_action = -1;

	if(MIDGARD_IS_OBJECT(mgdobject)) {
	
		const gchar *oguid = mgdobject->private->guid != NULL ? mgdobject->private->guid : "";

		if (midgard_is_guid(oguid)) {
	
			GString *_sql = g_string_new("SELECT object_action");
			g_string_append_printf(_sql, " FROM repligard WHERE guid = '%s'", oguid);
	
			if (!mgd_isroot(mgdobject->mgd)) {
	
				g_string_append(_sql, " AND ");
				g_string_append_printf(
						_sql, "sitegroup = %u",
						mgdobject->mgd->current_user->sitegroup);
			}
		
			gchar *tmpstr = g_string_free(_sql, FALSE);
			midgard_res *res = mgd_query(mgdobject->mgd, tmpstr);
			
			if(res && mgd_fetch(res)){
				object_action =  (guint)mgd_sql2id(res,0);
				if(res) mgd_release(res);
			}
			g_free(tmpstr);
		}

		gchar *_action;
		if(object_action > -1) {
			
			switch(object_action) {
				
				case MGD_OBJECT_ACTION_CREATE:
					_action = "created";		
					break;
				
				case MGD_OBJECT_ACTION_UPDATE:
					_action = "updated";
					break;
				
				case MGD_OBJECT_ACTION_DELETE:
					_action = "deleted";
					break;
				
				case MGD_OBJECT_ACTION_PURGE:
					_action = "purged";
					break;
				
				default:
					_action = "none";
					break;
			}
			
			xmlNewProp(node, BAD_CAST "action",
					BAD_CAST _action);
		}
	}

	for(i = 0; i < prop_n; i++) {
		
		g_value_init(&pval,pspec[i]->value_type);
		g_object_get_property(G_OBJECT(object), 
				pspec[i]->name, &pval);

		if(g_str_equal("guid", pspec[i]->name)) {
			/* Add guid attribute */
			xmlNewProp(node, BAD_CAST "guid", 
					BAD_CAST MGD_OBJECT_GUID(object));
			g_value_unset(&pval);
			continue;
		}
	
		if(klass && midgard_object_class_is_multilang(klass)) {
			
			if (g_str_equal ("lang", pspec[i]->name)) {


				xmlAttr *lang_attr = xmlHasProp (node, (const xmlChar*) "lang");

				/* Add lang property if lang is not 0 and node doesn't have lang property */
				guint object_lang_id = g_value_get_uint (&pval);
				g_value_unset (&pval);

				if (object_lang_id == 0 || lang_attr)
					continue;

				GString *sql = g_string_new ("SELECT code from midgard_language ");
				g_string_append_printf (sql, "WHERE id=%d", object_lang_id);

				midgard_res *res = mgd_query (mgdobject->mgd, sql->str);
				g_string_free (sql, TRUE);

				if (!res || !mgd_fetch (res)) {

					if (res)
						mgd_release (res);

					continue;
				}
				
				gchar *code = g_strdup ((char *)mgd_colvalue (res, 0));
				mgd_release (res);

				xmlNewProp(node, BAD_CAST "lang", BAD_CAST code);
				g_free (code);

				continue;	
			}
		}

		/* If property is a link we need to query guid 
		 * which identifies link object. Only if property 
		 * is not of guid or string type */
		if(mrp){
			if(midgard_reflection_property_is_link(mrp, 
						pspec[i]->name)){

				lval = g_new0(GValue, 1);
				switch(pspec[i]->value_type) {
					
					case G_TYPE_UINT:
						g_value_init(lval, G_TYPE_UINT);
						_uint = g_value_get_uint(&pval);
						if(!_uint){
							g_value_unset(lval);
							g_free(lval);
							goto export_unchecked_property;
						}
						g_value_set_uint(lval, _uint);
						break;

					case G_TYPE_INT:
						g_value_init(lval, G_TYPE_INT);
						_int = g_value_get_int(&pval);
						if(!_int){
							g_value_unset(lval);
							g_free(lval);
							goto export_unchecked_property;
						}
						g_value_set_int(lval, _int);
						break;

					default:
						g_free(lval);
						goto export_unchecked_property;
				}
					
				linktype = 
					midgard_reflection_property_get_link_name(
							mrp, pspec[i]->name);
				
				if(linktype){
					mc = midgard_collector_new(
							mgdobject->mgd->_mgd,
							linktype,
							"id",
							lval);
					
					midgard_collector_set_key_property(
							mc,
							"guid", NULL);
					/* If the property is a link, it always keep the same reference in every language.
					 * Do not depend on language content in this case, and do not worry about default 
					 * language being set */
					midgard_collector_unset_languages(mc);
					if(!midgard_collector_execute(mc)){
						g_object_unref(mc);
						g_value_unset(&pval);
						continue;
					}				
					gchar **linkguid = 
						midgard_collector_list_keys(mc);
					if(linkguid){
						if(linkguid[0])
							strprop = g_strdup(linkguid[0]);
					}
					if(!strprop)
						strprop = g_strdup("");
					
					/* Create node */
					escaped = xmlEncodeEntitiesReentrant(
							NULL, (const xmlChar*)strprop);
					xmlNewTextChild(node, NULL,
							BAD_CAST pspec[i]->name,
							BAD_CAST escaped);
					
					g_free(linkguid);
					g_free(strprop);
					g_free(escaped);
					g_object_unref(mc);	
				}
				g_value_unset(&pval);
				continue;
			}
		}	
		
		export_unchecked_property:
		switch(pspec[i]->value_type) {
			
			case G_TYPE_STRING:				
				strprop = g_value_dup_string(&pval);
				if(!strprop)
					strprop = g_strdup("");
				escaped = xmlEncodeEntitiesReentrant(
						NULL, (const xmlChar*)strprop);
				xmlNewTextChild(node, NULL, 
						BAD_CAST pspec[i]->name,
						BAD_CAST escaped);
			         
				/* This is commented because I have no idea how to use CDATA
				 * block between property tags. And more, CDATA is not 
				 * recomended for such usage ( at least by libxml developers )
				  
				xmlNodePtr cdatanode = 
					xmlNewCDataBlock(NULL, 
							(const xmlChar *) strprop,
							xmlStrlen((const xmlChar *)strprop));
				xmlNodeSetName(cdatanode, (const xmlChar *)pspec[i]->name);
				xmlSetProp(cdatanode, "aprop", "apropval");
				xmlAddChild(node, cdatanode);
				*/
				g_free(strprop);
				g_free(escaped);
				break;

			case G_TYPE_INT:
				gstring = g_string_new("");
				g_string_append_printf(gstring,
						"%d", g_value_get_int(&pval));
				strprop = g_string_free(gstring, FALSE);
				xmlNewChild(node, NULL, 
						BAD_CAST pspec[i]->name,
						BAD_CAST (xmlChar *)strprop);
				g_free(strprop);
				break;

			case G_TYPE_UINT:
				if(!g_str_equal(pspec[i]->name, 
							"sitegroup")) {
					gstring = g_string_new("");
					g_string_append_printf(gstring, 
							"%i", g_value_get_uint(&pval));
					strprop = g_string_free(gstring, FALSE);
					xmlNewChild(node, NULL,
							BAD_CAST pspec[i]->name,
							BAD_CAST (xmlChar *)strprop);
					g_free(strprop);
				}
				break;

			case G_TYPE_FLOAT:
				gstring = g_string_new("");
				g_string_append_printf(gstring,
						"%g", g_value_get_float(&pval));
				strprop = g_string_free(gstring, FALSE);
				xmlNewChild(node, NULL,
						BAD_CAST pspec[i]->name,
						BAD_CAST (xmlChar *)strprop);
				g_free(strprop);
				break;

			case G_TYPE_BOOLEAN:
				if(g_value_get_boolean(&pval))
					strprop = "1";
				else 
					strprop = "0";

				xmlNewChild(node, NULL,
						BAD_CAST pspec[i]->name,
						BAD_CAST (xmlChar *)strprop);
				break;

			case G_TYPE_OBJECT:
				op_node = xmlNewNode(NULL, 
						BAD_CAST pspec[i]->name);
				_write_nodes(G_OBJECT(g_value_get_object(&pval)), 
						op_node);
				xmlAddChild(node, op_node);
				break;
		}
		g_value_unset(&pval);
	}
	g_free(pspec);
	if(mrp)
		g_object_unref(mrp);
}		

gchar *_midgard_core_object_to_xml(GObject *gobject)
{
	g_assert(gobject != NULL);
	
	xmlDocPtr doc = NULL; 
	xmlNodePtr root_node = NULL;
	gboolean multilang = FALSE;
	gint lang;
	gint init_lang, init_dlang;
	//MidgardDBObject *object = MIDGARD_DBOBJECT(gobject);
	GObject *object = G_OBJECT(gobject);

	LIBXML_TEST_VERSION;
		
	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL, 
			BAD_CAST "midgard_object");
	xmlNewNs(root_node,
			BAD_CAST MIDGARD_OBJECT_HREF,
			NULL);
	xmlNodePtr object_node = 
		xmlNewNode(NULL, BAD_CAST G_OBJECT_TYPE_NAME(G_OBJECT(object)));
	/* Add purged info */
	/* We could add this attribute in _write_nodes function 
	 * but this could corrupt xsd compatibility for midgard_metadata nodes.
	 * So it's added here and for every multilingual content */
	xmlNewProp(object_node, BAD_CAST "purge", BAD_CAST "no");
	xmlAddChild(root_node, object_node);
	xmlDocSetRootElement(doc, root_node);
	_write_nodes(G_OBJECT(object), object_node);

	/* Get current object's language */
	guint current_object_lang = 0;

	/* Add lang attribute if object's class is multilang */
	if(MIDGARD_IS_DBOBJECT(object)) {
		MidgardDBObjectClass *klass =
			MIDGARD_DBOBJECT_GET_CLASS(object);
		if(klass) {
			if(MIDGARD_IS_OBJECT(object)) {
				MidgardObjectClass *oclass = MIDGARD_OBJECT_GET_CLASS(object);
				if(midgard_object_class_is_multilang(oclass)) {
					multilang = TRUE;
				}
			}
		}
	}

	if(multilang) {

		MidgardConnection *_mgd = MGD_OBJECT_CNC(object);
		if(!_mgd) {
			g_warning("MidgardConnection pointer not associated with '%s' instance", 
					G_OBJECT_TYPE_NAME(object));
			return NULL;
		}
		
		midgard *mgd = _mgd->mgd;

		g_object_get(G_OBJECT(object), "lang", &current_object_lang, NULL);
		
		guint i;
		init_lang = mgd_lang(mgd);
		init_dlang = mgd_get_default_lang(mgd);
	
		MidgardTypeHolder *holder = 
			g_new(MidgardTypeHolder, 1);
		GObject **lang_objects = NULL;
	        lang_objects = midgard_object_get_languages(MIDGARD_OBJECT(object), holder);
		gchar *code;

		if(lang_objects) {
		
			for(i = 0; i < holder->elements ; i++){
				
				g_object_get(lang_objects[i], "id", &lang, NULL);
	
				/* Do not create duplicates in xml */
				if(lang == current_object_lang)
					continue;

				mgd_internal_set_lang(mgd, lang);
				mgd_set_default_lang(mgd, lang);

				GValue gval = {0, };
				g_value_init(&gval , G_TYPE_STRING);
				g_value_set_string(&gval, MIDGARD_OBJECT(object)->private->guid);
				MgdObject *lobject = 
					midgard_object_new(mgd, 
							G_OBJECT_TYPE_NAME(G_OBJECT(object)),
							&gval);
				g_value_unset(&gval);

				object_node =
					xmlNewNode(NULL, BAD_CAST G_OBJECT_TYPE_NAME(G_OBJECT(object)));
			
				g_object_get(lang_objects[i], "code", &code, NULL);
				xmlNewProp(object_node, BAD_CAST "lang", BAD_CAST code);
				g_free(code);

				/* Add purged info */
				xmlNewProp(object_node, BAD_CAST "purge", BAD_CAST "no");
				xmlAddChild(root_node, object_node);
				_write_nodes(G_OBJECT(lobject), object_node);

				g_object_unref(lobject);
				g_object_unref(lang_objects[i]);

				mgd_internal_set_lang(mgd, init_lang);
				mgd_set_default_lang(mgd, init_dlang);
			}
		}

		g_free(lang_objects);
		g_free(holder);
	}

	xmlChar *buf;
	gint size;
	xmlDocDumpFormatMemoryEnc(doc, &buf, &size, "UTF-8", 1);
	
	xmlFreeDoc(doc);
	xmlCleanupParser();

	return (gchar*) buf;
}


xmlDoc *_midgard_core_object_create_xml_doc(void)
{
	xmlDocPtr doc = NULL;
	xmlNodePtr root_node = NULL;

	doc = xmlNewDoc(BAD_CAST "1.0");
	root_node = xmlNewNode(NULL,
			BAD_CAST "midgard_object");
	xmlNewNs(root_node,
			BAD_CAST MIDGARD_OBJECT_HREF,
			NULL);

	xmlDocSetRootElement(doc, root_node);

	return doc;
}

xmlNode *_get_type_node(xmlNode *node)
{
	xmlNode *cur = NULL;	
	for (cur = node; cur; cur = cur->next) {
		if (cur->type == XML_ELEMENT_NODE)
			return cur;
	}
	return NULL;
}

gboolean _nodes2object(GObject *object, xmlNode *node, gboolean force)
{
	g_assert(object);
	g_assert(node);

	xmlNode *cur = NULL;
	GObject *prop_object;
	gchar *nodeprop = NULL;
	xmlChar *decoded;
	xmlParserCtxtPtr parser;
	MgdObject *mobject = NULL;
	MgdObject *lobject = NULL;
	MidgardReflectionProperty *mrp = NULL;
	const gchar *linktype = NULL;

	if(MIDGARD_IS_OBJECT(object)) {
		mobject = MIDGARD_OBJECT(object);
		MidgardObjectClass *klass =
			MIDGARD_OBJECT_GET_CLASS(mobject);
		if(klass)
			mrp = midgard_reflection_property_new(klass);
	}

	if(MIDGARD_IS_OBJECT(object) || MIDGARD_IS_DBOBJECT(object)) {
		gpointer set_from_xml_func = MIDGARD_DBOBJECT_GET_CLASS(object)->dbpriv->set_from_xml_node;
		if(set_from_xml_func != NULL) {
			MIDGARD_DBOBJECT_GET_CLASS(object)->dbpriv->set_from_xml_node(MIDGARD_DBOBJECT(object), node);
			return TRUE;
		}
	}

	for (cur = node; cur; cur = cur->next) {
		if (cur->type == XML_ELEMENT_NODE) {
		
			linktype = NULL;

			GParamSpec *pspec = g_object_class_find_property(
					G_OBJECT_GET_CLASS(G_OBJECT(object)), 
					(const gchar *)cur->name);
			if(pspec) {	

				GValue pval = {0, };
				g_value_init(&pval, pspec->value_type);
				if(nodeprop)
					g_free(nodeprop);
				nodeprop = (gchar *)xmlNodeGetContent(cur);

				if(mrp) {
					if(midgard_reflection_property_is_link(
								mrp, pspec->name)){
						linktype =
							midgard_reflection_property_get_link_name(
									mrp, pspec->name);
					}
				}

				/* moved out from mrp condition check to avoid nested indents */

				if(linktype && midgard_is_guid(
							(const gchar *) nodeprop)){

					/* Just set property quickly, if property holds a guid */
					GType mtype = midgard_reflection_property_get_midgard_type(mrp, pspec->name);
					if (mtype == MGD_TYPE_GUID) {
						
						g_value_unset(&pval);
						g_object_set(mobject, (const gchar *)cur->name, nodeprop, NULL);
						continue;
					}

					/* we can use nodeprop directly */
					lobject = midgard_object_class_get_object_by_guid(
							mobject->mgd->_mgd,
							(const gchar *) nodeprop);

					if(!lobject && !force){
						g_object_unref(mrp);
						g_value_unset(&pval);
						midgard_set_error(mobject->mgd->_mgd, 
								MGD_GENERIC_ERROR, 
								MGD_ERR_MISSED_DEPENDENCE, 
								" Can not import %s. "
								"No '%s' object identified by '%s'",
								G_OBJECT_TYPE_NAME(object),
								linktype, nodeprop); 
						g_clear_error(&mobject->mgd->_mgd->err);	
						return FALSE;
					}
					
					/* When force parameter is set we do not translate guids to ids */
					if(force && !lobject && midgard_is_guid(
								(const gchar *) nodeprop)) {
						
						switch(pspec->value_type) {
							
							case G_TYPE_UINT:
								g_value_set_uint(&pval, 0);
								break;
							
							case G_TYPE_INT:
								g_value_set_int(&pval, 0);
								break;
							
							default:
								goto set_property_unchecked;
								break;
						}
						
						g_object_set_property(
								G_OBJECT(object),
								(const gchar *) cur->name,
								&pval);
						g_value_unset(&pval);
						continue;
					}

					GValue tval = {0, };
					g_value_init(&tval, pspec->value_type);

					g_object_get_property(G_OBJECT(lobject),
							"id", &tval);

					if(G_VALUE_TYPE(&pval) == G_TYPE_INT)
						g_value_transform((const GValue *) &tval,
								&pval);
					else 
						g_value_copy((const GValue*) &tval,
								&pval);	

					g_object_set_property(
							G_OBJECT(object),
							(const gchar *) cur->name,
							&pval);
					g_value_unset(&pval);
					g_object_unref(lobject);
					g_value_unset(&tval);
					continue;					
				}

				set_property_unchecked:
				switch (pspec->value_type) {
				
					case G_TYPE_STRING:
						parser = xmlNewParserCtxt();
						decoded = 
							xmlStringDecodeEntities(parser,
									(const xmlChar *) nodeprop, 
									XML_SUBSTITUTE_REF, 
									0, 0, 0);
						g_value_set_string(&pval, 
								(gchar *)decoded);
						g_free(decoded);
						xmlFreeParserCtxt(parser);
						break;

					case G_TYPE_INT:
						if(nodeprop) 
							g_value_set_int(&pval, 
									(gint)atoi((gchar *)nodeprop));
						break;

					case G_TYPE_UINT:
						if(nodeprop)
							g_value_set_uint(&pval,
									(guint)atoi((gchar *)nodeprop));
						break;

					case G_TYPE_FLOAT:
						g_value_set_float(&pval,
								(gfloat)atof((gchar *)nodeprop));
						break;

					case G_TYPE_BOOLEAN:
						g_value_set_boolean(&pval,
								(gboolean)atoi((gchar*)nodeprop));
						break;

					case G_TYPE_OBJECT:
						g_object_get(G_OBJECT(object),
								(const gchar *) cur->name,
								&prop_object, NULL);
						_nodes2object(prop_object, cur->children, force);
						g_value_set_object(&pval, prop_object);
						break;

					default:
						/* do nothing */
						break;						
				}
				g_object_set_property(
						G_OBJECT(object), 
						(const gchar *) cur->name, 
						&pval);
				g_value_unset(&pval);
			} else {
				g_warning("Undefined property '%s' for '%s'",
						cur->name, G_OBJECT_TYPE_NAME(object));
			}
		}		
	}

	if(nodeprop)
		g_free(nodeprop);

	GParamSpec *idspec = 
		g_object_class_find_property(G_OBJECT_GET_CLASS(object), "id");

	if(idspec)
		g_object_set(G_OBJECT(object), "id", 0, NULL);

	if(mrp)
		g_object_unref(mrp);

	return TRUE;
}

void _midgard_core_object_get_xml_doc(	MidgardConnection *mgd, 
					const gchar *xml, 
					xmlDoc **doc, 
					xmlNode **root_node)
{
	g_assert(*doc == NULL);
	g_assert(*root_node == NULL);

	if(xml == NULL)
		g_warning("Creating Midgard Object from xml requires xml."
				"NULL passed");
	
	LIBXML_TEST_VERSION

	/* midgard_object.xml is a dummy base URL in this case */
	*doc = xmlReadMemory(xml, strlen(xml), "midgard_object.xml", NULL, XML_PARSE_HUGE);
	if(!*doc) {
		g_warning("Can not parse given xml");
		return;
	}

	*root_node = xmlDocGetRootElement(*doc);
	if(!*root_node) {
		g_warning("Can not find root element for given xml");
		xmlFreeDoc(*doc);
		doc = NULL;
		return;
	}

	/* TODO : Get version from href and fail if invalid version exists */
	if(!g_str_equal((*root_node)->ns->href, MIDGARD_OBJECT_HREF)
			|| !g_str_equal((*root_node)->name, "midgard_object")) {
		g_warning("Skipping invalid midgard_object xml");
		xmlFreeDoc(*doc);
		*doc = NULL;
		*root_node = NULL;
		return;
	}
}

GObject **_midgard_core_object_from_xml(MidgardConnection *mgd, 
		const gchar *xml, gboolean force)
{
	xmlDoc *doc = NULL;
	xmlNode *root = NULL;
	_midgard_core_object_get_xml_doc(mgd, xml, &doc, &root);

	if(doc == NULL && root == NULL)
		return NULL;

	/* we can try to get type name , so we can create schema object instance */
	/* Piotras: Either I am blind or there is no explicit libxml2 API call
	 * to get child node explicitly. That's why _get_type_node is used */
	xmlNodePtr child = _get_type_node(root->children);
	if(!child) {
		g_warning("Can not get midgard type name from the given xml");
		xmlFreeDoc(doc);
		return NULL;
	}

	guint _n_nodes = 0;
	GObject **_objects = NULL;
	GSList *olist = NULL;
	GObject *object;
	
	for(; child; child = _get_type_node(child->next)) {
		
		if(!g_type_from_name((const gchar *)child->name)) {
			g_error("Type %s is not initialized in type system", 
					child->name);
			xmlFreeDoc(doc);
			return NULL;
		}
		
		if(g_str_equal(child->name, "midgard_blob")) {
			
			object = g_object_new(MIDGARD_TYPE_BLOB, NULL);			

		} else {
			
			object = (GObject *)midgard_object_new(
				mgd->mgd, (const gchar *)child->name, NULL);		
		}

		if(!object) {
			g_error("Can not create %s instance", child->name);
			xmlFreeDoc(doc);
			return NULL;
		}
		
		gboolean object_is_multilang = FALSE;
		if (MIDGARD_IS_OBJECT (object))
			object_is_multilang = midgard_object_class_is_multilang (MIDGARD_OBJECT_GET_CLASS (object));

		if(child->children && !_nodes2object(G_OBJECT(object), child->children, force)) {
			g_object_unref(object);
			object = NULL;	

		} else {

			xmlChar *attr = xmlGetProp(child, BAD_CAST "purge");
			if(attr && g_str_equal(attr, "yes")){
				g_object_set(object, "action", "purged", NULL);
				xmlFree(attr);
				attr = NULL;
				
				attr = xmlGetProp(child, BAD_CAST "guid");
				MIDGARD_OBJECT(object)->private->guid = 
					(const gchar *) g_strdup((gchar *)attr);
				xmlFree(attr);
				
				MIDGARD_OBJECT(object)->metadata->private->revised = 
					(gchar *) xmlGetProp(child, BAD_CAST "purged");
				
				olist = g_slist_prepend(olist, object);
				_n_nodes++;
				
				continue;
			}

			if(attr)
				xmlFree(attr);

			xmlChar *action_attr = xmlGetProp(child, BAD_CAST "action");
			if(action_attr) {
				g_object_set(G_OBJECT(object), "action", 
						(gchar *) action_attr, NULL);
				xmlFree(action_attr);
			}

			olist = g_slist_prepend(olist, object);
			_n_nodes++;	

			/* Set guid, do not free attr, it will be freed when object
			 * is destroyed */
			attr = xmlGetProp(child, BAD_CAST "guid");
			if (attr) {
			
				if(g_str_equal(child->name, "midgard_blob")) {
					MIDGARD_BLOB(object)->priv->parentguid = 
						g_strdup((gchar *)attr);					
				} else {
			
					gchar *guid =  (gchar *)MGD_OBJECT_GUID(object);
					g_free((gchar*) guid);
					
					if(MIDGARD_IS_OBJECT(object)) {
						MIDGARD_OBJECT(object)->private->guid = g_strdup((gchar *) attr);	
					} else if(MIDGARD_IS_DBOBJECT(object)) {
						MIDGARD_DBOBJECT(object)->dbpriv->guid = g_strdup((gchar *) attr);
					}
				}

				xmlFree(attr);

			}

			/* Set lang */
			guint lang = 0;
			attr = xmlGetProp(child, BAD_CAST "lang");
			if(attr && object_is_multilang) {
				if(!g_str_equal(attr, "")){
					midgard_res *res = 
						mgd_query(mgd->mgd,
								"SELECT id FROM "
								"midgard_language "
								"WHERE code=$q",
								attr);
					if(res){
						if(mgd_fetch(res))
							lang = atoi(mgd_colvalue(res, 0));
							
						mgd_release(res);
					}
				}
				g_free(attr);
			}

			const GParamSpec *pspec = g_object_class_find_property ( G_OBJECT_GET_CLASS (object), "lang");
			if (pspec && object_is_multilang) 
				g_object_set (object, "lang", lang, NULL);
		}
	}
	
	if(_n_nodes > 0) {
		
		_objects = g_new(GObject *, _n_nodes+1);
		_n_nodes = 0;
		
		for(; olist ; olist = olist->next){
			_objects[_n_nodes] = olist->data;
			_n_nodes++;
		}

		g_slist_free(olist);
		_objects[_n_nodes] = NULL;
	}

	xmlFreeDoc(doc);
	xmlCleanupParser();

	return _objects;
}

gboolean _midgard_object_violates_sitegroup(MgdObject *object)
{
	if (object->private->sg == 0
			&& !mgd_isroot(object->mgd)) {

		MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_ACCESS_DENIED);
		return TRUE;
	}

	if (!mgd_isroot(object->mgd)) {
		
			if (object->private->sg != object->mgd->_mgd->priv->sitegroup_id) {
			
				MIDGARD_ERRNO_SET(object->mgd, MGD_ERR_SITEGROUP_VIOLATION);
				return TRUE;
			}
	}

	return FALSE;
}
