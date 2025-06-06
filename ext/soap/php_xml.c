/*
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | https://www.php.net/license/3_01.txt                                 |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Brad Lafountain <rodif_bl@yahoo.com>                        |
  |          Shane Caraveo <shane@caraveo.com>                           |
  |          Dmitry Stogov <dmitry@php.net>                              |
  +----------------------------------------------------------------------+
*/

#include "php_soap.h"
#include "ext/libxml/php_libxml.h"
#include "libxml/parser.h"
#include "libxml/parserInternals.h"

/* Channel libxml file io layer through the PHP streams subsystem.
 * This allows use of ftps:// and https:// urls */

static bool is_blank(const xmlChar* str)
{
	while (*str != '\0') {
		if (*str != ' '  && *str != 0x9 && *str != 0xa && *str != 0xd) {
			return false;
		}
		str++;
	}
	return true;
}

/* removes all empty text, comments and other insignificant nodes */
static void cleanup_xml_node(xmlNodePtr node)
{
	xmlNodePtr trav;
	xmlNodePtr del = NULL;

	trav = node->children;
	while (trav != NULL) {
		if (del != NULL) {
			xmlUnlinkNode(del);
			xmlFreeNode(del);
			del = NULL;
		}
		if (trav->type == XML_TEXT_NODE) {
			if (is_blank(trav->content)) {
				del = trav;
			}
		} else if ((trav->type != XML_ELEMENT_NODE) &&
		           (trav->type != XML_CDATA_SECTION_NODE)) {
			del = trav;
		} else if (trav->children != NULL) {
			cleanup_xml_node(trav);
		}
		trav = trav->next;
	}
	if (del != NULL) {
		xmlUnlinkNode(del);
		xmlFreeNode(del);
	}
}

static void soap_ignorableWhitespace(void *ctx, const xmlChar *ch, int len)
{
}

static void soap_Comment(void *ctx, const xmlChar *value)
{
}

xmlDocPtr soap_xmlParseFile(const char *filename)
{
	xmlParserCtxtPtr ctxt = NULL;
	xmlDocPtr ret;
	bool old_allow_url_fopen;

	old_allow_url_fopen = PG(allow_url_fopen);
	PG(allow_url_fopen) = 1;
	ctxt = xmlCreateFileParserCtxt(filename);
	PG(allow_url_fopen) = old_allow_url_fopen;
	if (ctxt) {
		bool old;

		php_libxml_sanitize_parse_ctxt_options(ctxt);
		/* TODO: In libxml2 2.14.0 change this to the new options API so we don't rely on deprecated APIs. */
		ZEND_DIAGNOSTIC_IGNORED_START("-Wdeprecated-declarations")
		ctxt->keepBlanks = 0;
		ctxt->options |= XML_PARSE_HUGE;
		ZEND_DIAGNOSTIC_IGNORED_END
		ctxt->sax->ignorableWhitespace = soap_ignorableWhitespace;
		ctxt->sax->comment = soap_Comment;
		ctxt->sax->warning = NULL;
		ctxt->sax->error = NULL;
		/*ctxt->sax->fatalError = NULL;*/
		old = php_libxml_disable_entity_loader(1);
		xmlParseDocument(ctxt);
		php_libxml_disable_entity_loader(old);
		if (ctxt->wellFormed) {
			ret = ctxt->myDoc;
			if (ret->URL == NULL && ctxt->directory != NULL) {
				ret->URL = xmlCharStrdup(ctxt->directory);
			}
		} else {
			ret = NULL;
			xmlFreeDoc(ctxt->myDoc);
			ctxt->myDoc = NULL;
		}
		xmlFreeParserCtxt(ctxt);
	} else {
		ret = NULL;
	}

	if (ret) {
		cleanup_xml_node((xmlNodePtr)ret);
	}
	return ret;
}

xmlDocPtr soap_xmlParseMemory(const void *buf, size_t buf_size)
{
	xmlParserCtxtPtr ctxt = NULL;
	xmlDocPtr ret;

	ctxt = xmlCreateMemoryParserCtxt(buf, buf_size);
	if (ctxt) {
		bool old;

		php_libxml_sanitize_parse_ctxt_options(ctxt);
		ctxt->sax->ignorableWhitespace = soap_ignorableWhitespace;
		ctxt->sax->comment = soap_Comment;
		ctxt->sax->warning = NULL;
		ctxt->sax->error = NULL;
		/*ctxt->sax->fatalError = NULL;*/
		/* TODO: In libxml2 2.14.0 change this to the new options API so we don't rely on deprecated APIs. */
		ZEND_DIAGNOSTIC_IGNORED_START("-Wdeprecated-declarations")
		ctxt->options |= XML_PARSE_HUGE;
		ZEND_DIAGNOSTIC_IGNORED_END
		old = php_libxml_disable_entity_loader(1);
		xmlParseDocument(ctxt);
		php_libxml_disable_entity_loader(old);
		if (ctxt->wellFormed) {
			ret = ctxt->myDoc;
			if (ret->URL == NULL && ctxt->directory != NULL) {
				ret->URL = xmlCharStrdup(ctxt->directory);
			}
		} else {
			ret = NULL;
			xmlFreeDoc(ctxt->myDoc);
			ctxt->myDoc = NULL;
		}
		xmlFreeParserCtxt(ctxt);
	} else {
		ret = NULL;
	}

/*
	if (ret) {
		cleanup_xml_node((xmlNodePtr)ret);
	}
*/
	return ret;
}

xmlNsPtr node_find_ns(xmlNodePtr node)
{
	if (node->ns) {
		return node->ns;
	} else {
		return xmlSearchNs(node->doc, node, NULL);
	}
}

int attr_is_equal_ex(xmlAttrPtr node, const char *name, const char *ns)
{
	if (node->name && strcmp((const char *) node->name, name) == 0) {
		xmlNsPtr nsPtr = node->ns;
		if (ns) {
			if (nsPtr) {
				return (strcmp((const char *) nsPtr->href, ns) == 0);
			} else {
				return FALSE;
			}
		} else if (nsPtr) {
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

int node_is_equal_ex(xmlNodePtr node, const char *name, const char *ns)
{
	if (name == NULL || ((node->name) && strcmp((char*)node->name, name) == 0)) {
		if (ns) {
			xmlNsPtr nsPtr = node_find_ns(node);
			if (nsPtr) {
				return strcmp((const char *) nsPtr->href, ns) == 0;
			} else {
				return FALSE;
			}
		}
		return TRUE;
	}
	return FALSE;
}

int node_is_equal_ex_one_of(xmlNodePtr node, const char *name, const char *const *namespaces)
{
	if ((node->name) && strcmp((char*)node->name, name) == 0) {
		xmlNsPtr nsPtr = node_find_ns(node);
		if (nsPtr) {
			do {
				if (strcmp((const char *) nsPtr->href, *namespaces) == 0) {
					return TRUE;
				}
				namespaces++;
			} while (*namespaces != NULL);
		}
		return FALSE;
	}
	return FALSE;
}

xmlAttrPtr get_attribute_any_ns(xmlAttrPtr node, const char *name)
{
	while (node!=NULL) {
		if (node->name && strcmp((const char *) node->name, name) == 0) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

/* Finds an attribute by name and namespace.
 * If ns is NULL, the attribute must not be in any namespace.
 * If ns is not NULL, the attribute must be in the specified namespace.
 */
xmlAttrPtr get_attribute_ex(xmlAttrPtr node, const char *name, const char *ns)
{
	while (node != NULL) {
		if (attr_is_equal_ex(node, name, ns)) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

xmlNodePtr get_node_ex(xmlNodePtr node, const char *name, const char *ns)
{
	while (node!=NULL) {
		if (node_is_equal_ex(node, name, ns)) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

xmlNodePtr get_node_with_attribute_ex(xmlNodePtr node, const char *name, const char *name_ns, const char *attribute, const char *value, const char *attr_ns)
{
	xmlAttrPtr attr;

	while (node != NULL) {
		if (name != NULL) {
			node = get_node_ex(node, name, name_ns);
			if (node==NULL) {
				return NULL;
			}
		}

		attr = get_attribute_ex(node->properties, attribute, attr_ns);
		if (attr != NULL && strcmp((char*)attr->children->content, value) == 0) {
			return node;
		}
		node = node->next;
	}
	return NULL;
}

xmlNodePtr get_node_with_attribute_recursive_ex(xmlNodePtr node, const char *name, const char *name_ns, const char *attribute, const char *value, const char *attr_ns)
{
	while (node != NULL) {
		if (node_is_equal_ex(node, name, name_ns)) {
			xmlAttrPtr attr = get_attribute_ex(node->properties, attribute, attr_ns);
			if (attr != NULL && strcmp((char*)attr->children->content, value) == 0) {
				return node;
			}
		}
		if (node->children != NULL) {
			xmlNodePtr tmp = get_node_with_attribute_recursive_ex(node->children, name, name_ns, attribute, value, attr_ns);
			if (tmp) {
				return tmp;
			}
		}
		node = node->next;
	}
	return NULL;
}

/* namespace is either a copy or NULL, value is never NULL and never a copy. */
void parse_namespace(const xmlChar *inval, const char **value, char **namespace)
{
	const char *found = strrchr((const char *) inval, ':');

	if (found != NULL && found != (const char *) inval) {
		(*namespace) = estrndup((const char *) inval, found - (const char *) inval);
		(*value) = ++found;
	} else {
		(*value) = (const char *) inval;
		(*namespace) = NULL;
	}
}
