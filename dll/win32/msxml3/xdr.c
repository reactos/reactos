/*
 * XDR (XML-Data Reduced) -> XSD (XML Schema Document) conversion
 *
 * Copyright 2010 Adam Martinson for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <libxml/tree.h>

#include "wine/debug.h"

/* Both XDR and XSD are valid XML
 * We just convert the doc tree, no need for a parser.
 */

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

static const xmlChar DT_prefix[] = "dt";
static const xmlChar DT_href[] = "urn:schemas-microsoft-com:datatypes";
static const xmlChar XDR_href[] = "urn:schemas-microsoft-com:xml-data";
static const xmlChar XSD_prefix[] = "xsd";
static const xmlChar XSD_href[] = "http://www.w3.org/2001/XMLSchema";

static const xmlChar xs_all[] = "all";
static const xmlChar xs_annotation[] = "annotation";
static const xmlChar xs_any[] = "any";
static const xmlChar xs_anyAttribute[] = "anyAttribute";
static const xmlChar xs_attribute[] = "attribute";
static const xmlChar xs_AttributeType[] = "AttributeType";
static const xmlChar xs_base[] = "base";
static const xmlChar xs_choice[] = "choice";
static const xmlChar xs_complexType[] = "complexType";
static const xmlChar xs_content[] = "content";
static const xmlChar xs_datatype[] = "datatype";
static const xmlChar xs_default[] = "default";
static const xmlChar xs_description[] = "description";
static const xmlChar xs_documentation[] = "documentation";
static const xmlChar xs_element[] = "element";
static const xmlChar xs_ElementType[] = "ElementType";
static const xmlChar xs_eltOnly[] = "eltOnly";
static const xmlChar xs_enumeration[] = "enumeration";
static const xmlChar xs_extension[] = "extension";
static const xmlChar xs_group[] = "group";
static const xmlChar xs_lax[] = "lax";
static const xmlChar xs_length[] = "length";
static const xmlChar xs_many[] = "many";
static const xmlChar xs_maxOccurs[] = "maxOccurs";
static const xmlChar xs_minOccurs[] = "minOccurs";
static const xmlChar xs_mixed[] = "mixed";
static const xmlChar xs_model[] = "model";
static const xmlChar xs_name[] = "name";
static const xmlChar xs_namespace[] = "namespace";
static const xmlChar xs_no[] = "no";
static const xmlChar xs_open[] = "open";
static const xmlChar xs_optional[] = "optional";
static const xmlChar xs_order[] = "order";
static const xmlChar xs_processContents[] = "processContents";
static const xmlChar xs_ref[] = "ref";
static const xmlChar xs_required[] = "required";
static const xmlChar xs_restriction[] = "restriction";
static const xmlChar xs_schema[] = "schema";
static const xmlChar xs_seq[] = "seq";
static const xmlChar xs_sequence[] = "sequence";
static const xmlChar xs_simpleContent[] = "simpleContent";
static const xmlChar xs_simpleType[] = "simpleType";
static const xmlChar xs_strict[] = "strict";
static const xmlChar xs_targetNamespace[] = "targetNamespace";
static const xmlChar xs_textOnly[] = "textOnly";
static const xmlChar xs_true[] = "true";
static const xmlChar xs_type[] = "type";
static const xmlChar xs_unbounded[] = "unbounded";
static const xmlChar xs_use[] = "use";
static const xmlChar xs_value[] = "value";
static const xmlChar xs_values[] = "values";
static const xmlChar xs_xsd_string[] = "xsd:string";

typedef enum _CONTENT_TYPE
{
    CONTENT_EMPTY,
    CONTENT_TEXTONLY,
    CONTENT_ELTONLY,
    CONTENT_MIXED
} CONTENT_TYPE;

typedef enum _ORDER_TYPE
{
    ORDER_SEQ,
    ORDER_MANY,
    ORDER_ONE
} ORDER_TYPE;

#define FOREACH_CHILD(node, child) \
    for (child = node->children; child != NULL; child = child->next) \
        if (child->type == XML_ELEMENT_NODE)

#define FOREACH_ATTR(node, attr) \
    for (attr = node->properties; attr != NULL; attr = attr->next)

#define FOREACH_NS(node, ns) \
    for (ns = node->nsDef; ns != NULL; ns = ns->next)

static inline xmlNodePtr get_schema(xmlNodePtr node)
{
    return xmlDocGetRootElement(node->doc);
}

static inline xmlNodePtr get_child(xmlNodePtr node, xmlChar const* name)
{
    xmlNodePtr child = NULL;
    if (node)
    {
        FOREACH_CHILD(node, child)
        {
            if (xmlStrEqual(child->name, name))
                break;
        }
    }

    return child;
}

static inline xmlNodePtr get_child_with_attr(xmlNodePtr node, xmlChar const* name,
                                             xmlChar const* attr_ns, xmlChar const* attr_name,
                                             xmlChar const* attr_val)
{
    xmlChar* str;
    if (node)
    {
        FOREACH_CHILD(node, node)
        {
            if (xmlStrEqual(node->name, name))
            {
                str = (attr_ns != NULL)? xmlGetNsProp(node, attr_name, attr_ns) :
                                                  xmlGetProp(node, attr_name);
                if (str)
                {
                    if (xmlStrEqual(str, attr_val))
                    {
                        xmlFree(str);
                        return node;
                    }
                    xmlFree(str);
                }
            }
        }
    }

    return NULL;
}

static inline xmlNsPtr get_dt_ns(xmlNodePtr node)
{
    xmlNsPtr ns;

    node = get_schema(node);
    assert(node != NULL);

    FOREACH_NS(node, ns)
    {
        if (xmlStrEqual(ns->href, DT_href))
            break;
    }

    return ns;
}

static inline xmlChar* get_dt_type(xmlNodePtr xdr)
{
    xmlChar* str = xmlGetNsProp(xdr, xs_type, DT_href);
    if (!str)
    {
        xmlNodePtr datatype = get_child(xdr, xs_datatype);
        if (datatype)
            str = xmlGetNsProp(datatype, xs_type, DT_href);
    }
    return str;
}

static inline xmlChar* get_attr_val(xmlAttrPtr attr)
{
    return xmlNodeGetContent((xmlNodePtr)attr);
}

static inline xmlNodePtr add_any_child(xmlNodePtr parent, BOOL set_occurs)
{
    xmlNodePtr child = xmlNewChild(parent, NULL, xs_any, NULL);
    if (set_occurs)
    {
        xmlSetProp(child, xs_minOccurs, BAD_CAST "0");
        xmlSetProp(child, xs_maxOccurs, xs_unbounded);
    }
    xmlSetProp(child, xs_processContents, xs_strict);
    return child;
}

static inline xmlNodePtr add_anyAttribute_child(xmlNodePtr parent)
{
    xmlNodePtr child = xmlNewChild(parent, NULL, xs_anyAttribute, NULL);
    xmlSetProp(child, xs_processContents, xs_lax);
    return child;
}

static inline xmlAttrPtr copy_prop_ignore_ns(xmlAttrPtr xdr_attr, xmlNodePtr node)
{
    xmlChar* str = get_attr_val(xdr_attr);
    xmlAttrPtr attr = xmlSetProp(node, xdr_attr->name, str);
    xmlFree(str);
    return attr;
}
static inline xmlAttrPtr XDR_A_default(xmlAttrPtr xdr_attr, xmlNodePtr node)
{
    TRACE("(%p, %p)\n", xdr_attr, node);

    return copy_prop_ignore_ns(xdr_attr, node);
}

static inline xmlAttrPtr XDR_A_dt_type(xmlAttrPtr xdr_attr, xmlNodePtr node)
{
    xmlChar* str = get_attr_val(xdr_attr);
    xmlAttrPtr attr;

    TRACE("(%p, %p)\n", xdr_attr, node);

    if (xmlStrEqual(str, xs_enumeration))
        attr = NULL;
    else
        attr = xmlSetNsProp(node, get_dt_ns(node), DT_prefix, str);
    xmlFree(str);
    return attr;
}

static xmlAttrPtr XDR_A_maxOccurs(xmlAttrPtr xdr_attr, xmlNodePtr node)
{
    xmlChar* str = get_attr_val(xdr_attr);
    xmlAttrPtr attr;

    TRACE("(%p, %p)\n", xdr_attr, node);

    if (xmlStrEqual(str, BAD_CAST "*"))
        attr = xmlSetProp(node, xs_maxOccurs, xs_unbounded);
    else
        attr = copy_prop_ignore_ns(xdr_attr, node);

    xmlFree(str);
    return attr;
}

static inline xmlAttrPtr XDR_A_minOccurs(xmlAttrPtr xdr_attr, xmlNodePtr node)
{
    TRACE("(%p, %p)\n", xdr_attr, node);

    return copy_prop_ignore_ns(xdr_attr, node);
}

static inline xmlAttrPtr XDR_A_name(xmlAttrPtr xdr_attr, xmlNodePtr node)
{
    TRACE("(%p, %p)\n", xdr_attr, node);

    return copy_prop_ignore_ns(xdr_attr, node);
}

static xmlAttrPtr XDR_A_type(xmlAttrPtr xdr_attr, xmlNodePtr node)
{
    xmlChar* str = get_attr_val(xdr_attr);
    xmlAttrPtr attr = xmlSetProp(node, xs_ref, str);

    TRACE("(%p, %p)\n", xdr_attr, node);

    xmlFree(str);
    return attr;
}

static xmlAttrPtr XDR_A_required(xmlAttrPtr xdr_attr, xmlNodePtr node)
{
    xmlChar* str = get_attr_val(xdr_attr);
    xmlAttrPtr attr;

    TRACE("(%p, %p)\n", xdr_attr, node);

    if (xmlStrEqual(str, xs_no))
        attr = xmlSetProp(node, xs_use, xs_optional);
    else /* yes */
        attr = xmlSetProp(node, xs_use, xs_required);
    xmlFree(str);
    return attr;
}

static xmlNodePtr XDR_E_description(xmlNodePtr xdr, xmlNodePtr parent)
{
    xmlNodePtr xsd_node = xmlNewChild(parent, NULL, xs_annotation, NULL);
    xmlAttrPtr xdr_attr;

    TRACE("(%p, %p)\n", xdr, parent);

    xmlNewChild(xsd_node, NULL, xs_documentation, xdr->content);

    FOREACH_ATTR(xdr, xdr_attr)
    {
        xmlCopyProp(xsd_node, xdr_attr);
    }
    return xsd_node;
}

static xmlNodePtr XDR_E_AttributeType(xmlNodePtr xdr, xmlNodePtr parent)
{
    xmlChar *str, *type = get_dt_type(xdr);
    xmlNodePtr xsd_node, xsd_child, xdr_child;
    xmlAttrPtr xdr_attr;

    TRACE("(%p, %p)\n", xdr, parent);

    xsd_node = xmlNewChild(parent, NULL, xs_attribute, NULL);

    if (type && xmlStrEqual(type, xs_enumeration))
    {
        xmlChar *tmp, *tokBegin, *tokEnd = NULL;
        xmlNodePtr xsd_enum;
        xsd_child = xmlNewChild(xsd_node, NULL, xs_simpleType, NULL);
        xsd_child = xmlNewChild(xsd_child, NULL, xs_restriction, NULL);
        xmlSetProp(xsd_child, xs_base, xs_xsd_string);

        tokBegin = str = xmlGetNsProp(xdr, xs_values, DT_href);
        while (tokBegin && *tokBegin)
        {
            while (*tokBegin && isspace(*tokBegin))
                ++tokBegin;
            tokEnd = tokBegin;
            while (*tokEnd && !isspace(*tokEnd))
                ++tokEnd;
            if (tokEnd == tokBegin)
                break;
            xsd_enum = xmlNewChild(xsd_child, NULL, xs_enumeration, NULL);
            tmp = xmlStrndup(tokBegin, tokEnd-tokBegin);
            xmlSetProp(xsd_enum, xs_value, tmp);
            xmlFree(tmp);
            tokBegin = tokEnd;
        }
        xmlFree(str);

    }
    else if (type)
    {
        str = xmlStrdup(DT_prefix);
        str = xmlStrcat(str, BAD_CAST ":");
        str = xmlStrcat(str, type);
        xmlSetProp(xsd_node, xs_type, str);
        xmlFree(str);
    }
    xmlFree(type);

    FOREACH_ATTR(xdr, xdr_attr)
    {
        if (xmlStrEqual(xdr_attr->name, xs_default))
            XDR_A_default(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_name))
            XDR_A_name(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_type) && xdr_attr->ns == get_dt_ns(xdr))
            XDR_A_dt_type(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_values) && xdr_attr->ns == get_dt_ns(xdr))
            ; /* already handled */
        else if (xmlStrEqual(xdr_attr->name, xs_required))
            XDR_A_required(xdr_attr, xsd_node);
        else
            xmlCopyProp(xsd_node, xdr_attr);
    }

    FOREACH_CHILD(xdr, xdr_child)
    {
        if (xmlStrEqual(xdr_child->name, xs_datatype))
            ; /* already handled */
        else if (xmlStrEqual(xdr_child->name, xs_description))
            XDR_E_description(xdr_child, xsd_node);
        else
            FIXME("unexpected child <%s>\n", xdr_child->name);
    }

    return xsd_node;
}

static xmlNodePtr XDR_E_attribute(xmlNodePtr xdr, xmlNodePtr parent)
{
    xmlChar* str = xmlGetProp(xdr, xs_type);
    xmlNodePtr xsd_node, xdr_child, xdr_attrType;
    xmlAttrPtr xdr_attr;

    TRACE("(%p, %p)\n", xdr, parent);

    xdr_attrType = get_child_with_attr(xdr->parent, xs_AttributeType, NULL, xs_name, str);
    xmlFree(str);

    if (xdr_attrType)
        xsd_node = XDR_E_AttributeType(xdr_attrType, parent);
    else
        xsd_node = xmlNewChild(parent, NULL, xs_attribute, NULL);

    FOREACH_ATTR(xdr, xdr_attr)
    {
        if (xmlStrEqual(xdr_attr->name, xs_default))
            XDR_A_default(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_type) && !xdr_attrType)
            XDR_A_type(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_required))
            XDR_A_required(xdr_attr, xsd_node);
        else
            xmlCopyProp(xsd_node, xdr_attr);
    }

    FOREACH_CHILD(xdr, xdr_child)
    {
        FIXME("unexpected child <%s>\n", xdr_child->name);
    }

    return xsd_node;
}

static xmlNodePtr XDR_E_element(xmlNodePtr xdr, xmlNodePtr parent)
{
    xmlNodePtr xdr_child, xsd_node = xmlNewChild(parent, NULL, xs_element, NULL);
    xmlAttrPtr xdr_attr;

    FOREACH_ATTR(xdr, xdr_attr)
    {
        if (xmlStrEqual(xdr_attr->name, xs_type))
            XDR_A_type(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_maxOccurs))
            XDR_A_maxOccurs(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_minOccurs))
            XDR_A_minOccurs(xdr_attr, xsd_node);
        else
            xmlCopyProp(xsd_node, xdr_attr);
    }

    FOREACH_CHILD(xdr, xdr_child)
    {
        FIXME("unexpected child <%s>\n", xdr_child->name);
    }

    return xsd_node;
}

static xmlNodePtr XDR_E_group(xmlNodePtr xdr, xmlNodePtr parent)
{
    xmlNodePtr xdr_child, xsd_node;
    xmlChar* str = xmlGetProp(xdr, xs_order);
    xmlAttrPtr xdr_attr;

    TRACE("(%p, %p)\n", xdr, parent);

    if (!str || xmlStrEqual(str, xs_seq))
        xsd_node = xmlNewChild(parent, NULL, xs_sequence, NULL);
    else if (xmlStrEqual(str, xs_many))
        xsd_node = xmlNewChild(parent, NULL, xs_choice, NULL);
    else /* one */
        xsd_node = xmlNewChild(parent, NULL, xs_all, NULL);
    xmlFree(str);

    FOREACH_ATTR(xdr, xdr_attr)
    {
        if (xmlStrEqual(xdr_attr->name, xs_order))
            ; /* already handled */
        else if (xmlStrEqual(xdr_attr->name, xs_model))
            ; /* ignored */
        else if (xmlStrEqual(xdr_attr->name, xs_maxOccurs))
            XDR_A_maxOccurs(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_minOccurs))
            XDR_A_minOccurs(xdr_attr, xsd_node);
        else
            xmlCopyProp(xsd_node, xdr_attr);
    }

    FOREACH_CHILD(xdr, xdr_child)
    {
        if (xmlStrEqual(xdr_child->name, xs_description))
            XDR_E_description(xdr_child, xsd_node);
        else if (xmlStrEqual(xdr_child->name, xs_element))
            XDR_E_element(xdr_child, xsd_node);
    }

    return xsd_node;
}

static xmlNodePtr XDR_E_ElementType(xmlNodePtr xdr, xmlNodePtr parent)
{
    xmlChar *str, *type = get_dt_type(xdr);
    BOOL is_open = TRUE;
    int n_attributes = 0, n_elements = 0, n_groups = 0;
    CONTENT_TYPE content;
    ORDER_TYPE order;
    xmlNodePtr xsd_node, xsd_type, xsd_child, xdr_child;
    xmlAttrPtr xdr_attr;
    xmlNsPtr dt_ns = get_dt_ns(parent);

    TRACE("(%p, %p)\n", xdr, parent);

    str = xmlGetProp(xdr, xs_model);
    if (str && !xmlStrEqual(str, xs_open))
        is_open = FALSE;
    xmlFree(str);

    if (type)
    {
        content = CONTENT_TEXTONLY;
    }
    else
    {
        str = xmlGetProp(xdr, xs_content);
        if (!str || xmlStrEqual(str, xs_mixed))
            content = CONTENT_MIXED;
        else if (xmlStrEqual(str, xs_eltOnly))
            content = CONTENT_ELTONLY;
        else if (xmlStrEqual(str, xs_textOnly))
            content = CONTENT_TEXTONLY;
        else /* empty */
            content = CONTENT_EMPTY;
        xmlFree(str);
    }

    str = xmlGetProp(xdr, xs_order);
    if (!str || xmlStrEqual(str, xs_seq))
    {
        order = ORDER_SEQ;
    }
    else if (xmlStrEqual(str, xs_many))
    {
        order = ORDER_MANY;
    }
    else /* one */
    {
        order = ORDER_ONE;
        is_open = FALSE;
    }
    xmlFree(str);

    FOREACH_CHILD(xdr, xdr_child)
    {
        if (xmlStrEqual(xdr_child->name, xs_element))
            ++n_elements;
        else if (xmlStrEqual(xdr_child->name, xs_group))
            ++n_groups;
        else if (xmlStrEqual(xdr_child->name, xs_attribute))
            ++n_attributes;
    }

    xsd_node = xmlNewChild(parent, NULL, xs_element, NULL);
    assert(xsd_node != NULL);
    switch (content)
    {
        case CONTENT_MIXED:
        case CONTENT_ELTONLY:
            {
                xmlNodePtr xsd_base;
                xsd_type = xmlNewChild(xsd_node, NULL, xs_complexType, NULL);

                if (content == CONTENT_MIXED)
                    xmlSetProp(xsd_type, xs_mixed, xs_true);

                if (is_open)
                    xsd_base = xmlNewChild(xsd_type, NULL, xs_sequence, NULL);
                else
                    xsd_base = xsd_type;

                if (is_open && n_elements < 2 && !n_groups)
                {/* no specific sequence of elements we need,
                    just has to start with the right one, if any */
                    if ((xdr_child = get_child(xdr, xs_element)))
                    {
                        xsd_child = XDR_E_element(xdr_child, xsd_base);
                        xmlUnsetProp(xsd_child, xs_maxOccurs);
                    }
                }
                else
                {
                    switch (order)
                    {
                        case ORDER_SEQ:
                            xsd_child = xmlNewChild(xsd_base, NULL, xs_sequence, NULL);
                            break;
                        case ORDER_MANY:
                            xsd_child = xmlNewChild(xsd_base, NULL, xs_choice, NULL);
                            xmlSetProp(xsd_child, xs_maxOccurs, xs_unbounded);
                            break;
                        case ORDER_ONE:
                            xsd_child = xmlNewChild(xsd_base, NULL, xs_all, NULL);
                            break;
                    }

                    FOREACH_CHILD(xdr, xdr_child)
                    {
                        if (xmlStrEqual(xdr_child->name, xs_element))
                            XDR_E_element(xdr_child, xsd_child);
                        else if (xmlStrEqual(xdr_child->name, xs_group))
                            XDR_E_group(xdr_child, xsd_child);
                    }
                }

                if (n_attributes)
                {
                    FOREACH_CHILD(xdr, xdr_child)
                    {
                        if (xmlStrEqual(xdr_child->name, xs_attribute))
                            XDR_E_attribute(xdr_child, xsd_type);
                    }
                }

                if (is_open)
                {
                    add_any_child(xsd_base, TRUE);
                    add_anyAttribute_child(xsd_type);
                }
            }
            break;
        case CONTENT_TEXTONLY:
            {
                if (is_open)
                {
                    xsd_type = xmlNewChild(xsd_node, NULL, xs_complexType, NULL);
                    if (type)
                    {
                        xsd_child = xmlNewChild(xsd_type, NULL, xs_simpleContent, NULL);
                        xsd_child = xmlNewChild(xsd_child, NULL, xs_extension, NULL);
                        str = xmlStrdup(DT_prefix);
                        str = xmlStrcat(str, BAD_CAST ":");
                        str = xmlStrcat(str, type);
                        xmlSetProp(xsd_child, xs_base, str);
                        xmlFree(str);
                        assert(dt_ns != NULL);
                        xmlSetNsProp(xsd_node, dt_ns, DT_prefix, type);
                    }
                    else
                    {
                        xmlSetProp(xsd_type, xs_mixed, xs_true);
                        xsd_child = xmlNewChild(xsd_type, NULL, xs_choice, NULL);
                        xmlSetProp(xsd_child, xs_minOccurs, BAD_CAST "0");
                        xmlSetProp(xsd_child, xs_maxOccurs, xs_unbounded);
                        xsd_child = add_any_child(xsd_child, FALSE);
                        xmlSetProp(xsd_child, xs_namespace, BAD_CAST "##other");
                        xsd_child = xsd_type;
                    }

                    if (n_attributes)
                        FOREACH_CHILD(xdr, xdr_child)
                        {
                            if (xmlStrEqual(xdr_child->name, xs_attribute))
                                XDR_E_attribute(xdr_child, xsd_child);
                        }

                    xmlNewChild(xsd_child, NULL, xs_anyAttribute, NULL);
                }
                else if (!n_attributes)
                {
                    if (type)
                    {
                        str = xmlStrdup(DT_prefix);
                        str = xmlStrcat(str, BAD_CAST ":");
                        str = xmlStrcat(str, type);
                        xmlSetProp(xsd_node, xs_type, str);
                        xmlFree(str);
                        str = NULL;
                        xmlSetNsProp(xsd_node, dt_ns, DT_prefix, type);
                    }
                    else
                    {
                        xmlSetProp(xsd_node, xs_type, xs_xsd_string);
                    }
                }
                else
                {
                    xsd_type = xmlNewChild(xsd_node, NULL, xs_complexType, NULL);
                    xsd_child = xmlNewChild(xsd_type, NULL, xs_simpleContent, NULL);
                    xsd_child = xmlNewChild(xsd_child, NULL, xs_extension, NULL);
                    xmlSetProp(xsd_child, xs_base, xs_xsd_string);

                    FOREACH_CHILD(xdr, xdr_child)
                    {
                        if (xmlStrEqual(xdr_child->name, xs_attribute))
                            XDR_E_attribute(xdr_child, xsd_child);
                    }
                }
            }
            break;
        case CONTENT_EMPTY: /* not allowed with model="open" */
            {
                if (n_attributes)
                {
                    xsd_type = xmlNewChild(xsd_node, NULL, xs_complexType, NULL);

                    FOREACH_CHILD(xdr, xdr_child)
                    {
                        if (xmlStrEqual(xdr_child->name, xs_attribute))
                            XDR_E_attribute(xdr_child, xsd_type);
                    }
                }
                else
                {
                    xsd_type = xmlNewChild(xsd_node, NULL, xs_simpleType, NULL);
                    xsd_child = xmlNewChild(xsd_type, NULL, xs_restriction, NULL);
                    xmlSetProp(xsd_child, xs_base, xs_xsd_string);
                    xsd_child = xmlNewChild(xsd_child, NULL, xs_length, NULL);
                    xmlSetProp(xsd_child, xs_value, BAD_CAST "0");
                }
            }
            break;
    }
    xmlFree(type);

    FOREACH_ATTR(xdr, xdr_attr)
    {
        if (xmlStrEqual(xdr_attr->name, xs_content))
            ; /* already handled */
        else if (xmlStrEqual(xdr_attr->name, xs_name))
            XDR_A_name(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_type) && xdr_attr->ns == get_dt_ns(xdr))
            XDR_A_dt_type(xdr_attr, xsd_node);
        else if (xmlStrEqual(xdr_attr->name, xs_model))
            ; /* already handled */
        else if (xmlStrEqual(xdr_attr->name, xs_order))
            ; /* already handled */
        else
            xmlCopyProp(xsd_node, xdr_attr);

    }

    FOREACH_CHILD(xdr, xdr_child)
    {
        if (xmlStrEqual(xdr_child->name, xs_attribute))
            ; /* already handled */
        else if (xmlStrEqual(xdr_child->name, xs_AttributeType))
            ; /* handled through XDR_E_attribute when parent is not <Schema> */
        else if (xmlStrEqual(xdr_child->name, xs_datatype))
            ; /* already handled */
        else if (xmlStrEqual(xdr_child->name, xs_description))
            XDR_E_description(xdr_child, xsd_node);
        else if (xmlStrEqual(xdr_child->name, xs_element))
            ; /* already handled */
        else if (xmlStrEqual(xdr_child->name, xs_group))
            ; /* already handled */
        else
            FIXME("unexpected child <%s>\n", xdr_child->name);
    }

    return xsd_node;
}

static xmlNodePtr XDR_E_Schema(xmlNodePtr xdr, xmlNodePtr parent, xmlChar const* nsURI)
{
    xmlNodePtr xsd_node, xdr_child;
    xmlNsPtr ns, xdr_ns;
    xmlAttrPtr xdr_attr;

    TRACE("(%p, %p)\n", xdr, parent);

    xsd_node = xmlNewDocNode((xmlDocPtr)parent, NULL, xs_schema, NULL);
    xmlDocSetRootElement((xmlDocPtr)parent, xsd_node);
    assert(xsd_node != NULL);

    if (nsURI && *nsURI) xmlNewNs(xsd_node, nsURI, NULL);
    ns = xmlNewNs(xsd_node, XSD_href, XSD_prefix);
    assert(ns != NULL);

    xmlSetNs(xsd_node, ns);

    if (nsURI && *nsURI) xmlSetProp(xsd_node, xs_targetNamespace, nsURI);

    FOREACH_NS(xdr, xdr_ns)
    {
        /* TODO: special handling for dt namespace? */
        assert(xdr_ns->href != NULL);
        if (xmlStrEqual(xdr_ns->href, XDR_href))
            ; /* ignored */
        else if (xdr_ns->prefix != NULL)
            xmlNewNs(xsd_node, xdr_ns->href, xdr_ns->prefix);
        else
            FIXME("unexpected default xmlns: %s\n", xdr_ns->href);
    }

    FOREACH_ATTR(xdr, xdr_attr)
    {
        xmlCopyProp(xsd_node, xdr_attr);
    }

    FOREACH_CHILD(xdr, xdr_child)
    {
        if (xmlStrEqual(xdr_child->name, xs_AttributeType))
            XDR_E_AttributeType(xdr_child, xsd_node);
        else if (xmlStrEqual(xdr_child->name, xs_description))
            XDR_E_description(xdr_child, xsd_node);
        else if (xmlStrEqual(xdr_child->name, xs_ElementType))
            XDR_E_ElementType(xdr_child, xsd_node);
        else
            FIXME("unexpected child <%s>\n", xdr_child->name);
    }

    return xsd_node;
}

xmlDocPtr XDR_to_XSD_doc(xmlDocPtr xdr_doc, xmlChar const* nsURI)
{
    xmlDocPtr xsd_doc = xmlNewDoc(NULL);

    TRACE("(%p)\n", xdr_doc);

    XDR_E_Schema(get_schema((xmlNodePtr)xdr_doc), (xmlNodePtr)xsd_doc, nsURI);

    return xsd_doc;
}
