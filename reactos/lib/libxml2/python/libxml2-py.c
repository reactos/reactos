/* Generated */

#include <Python.h>
#include <libxml/xmlversion.h>
#include <libxml/tree.h>
#include <libxml/xmlschemastypes.h>
#include "libxml_wrap.h"
#include "libxml2-py.h"

PyObject *
libxml_xmlGetDocEntity(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlEntityPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlGetDocEntity", &pyobj_doc, &name))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlGetDocEntity(doc, name);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBopomofo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsBopomofo", &code))
        return(NULL);

    c_retval = xmlUCSIsBopomofo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNsLookup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * prefix;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlXPathNsLookup", &pyobj_ctxt, &prefix))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = xmlXPathNsLookup(ctxt, prefix);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlStrstr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlChar * str;
    xmlChar * val;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlStrstr", &str, &val))
        return(NULL);

    c_retval = xmlStrstr(str, val);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderForFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlTextReaderPtr c_retval;
    char * filename;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"zzi:xmlReaderForFile", &filename, &encoding, &options))
        return(NULL);

    c_retval = xmlReaderForFile(filename, encoding, options);
    py_retval = libxml_xmlTextReaderPtrWrap((xmlTextReaderPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderExpand(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderExpand", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderExpand(reader);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlFreeParserInputBuffer(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserInputBufferPtr in;
    PyObject *pyobj_in;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeParserInputBuffer", &pyobj_in))
        return(NULL);
    in = (xmlParserInputBufferPtr) PyinputBuffer_Get(pyobj_in);

    xmlFreeParserInputBuffer(in);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMathematicalAlphanumericSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMathematicalAlphanumericSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsMathematicalAlphanumericSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpNodeList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlNodePtr node;
    PyObject *pyobj_node;
    int depth;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlDebugDumpNodeList", &pyobj_output, &pyobj_node, &depth))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    xmlDebugDumpNodeList(output, node, depth);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHangulJamo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHangulJamo", &code))
        return(NULL);

    c_retval = xmlUCSIsHangulJamo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaWhiteSpaceReplace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlSchemaWhiteSpaceReplace", &value))
        return(NULL);

    c_retval = xmlSchemaWhiteSpaceReplace(value);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_FTP_ENABLED)
PyObject *
libxml_xmlNanoFTPCleanup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlNanoFTPCleanup();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_FTP_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateOneElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlValidateOneElement", &pyobj_ctxt, &pyobj_doc, &pyobj_elem))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlValidateOneElement(ctxt, doc, elem);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlGetID(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * ID;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlGetID", &pyobj_doc, &ID))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlGetID(doc, ID);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMalayalam(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMalayalam", &code))
        return(NULL);

    c_retval = xmlUCSIsMalayalam(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlXPathInit(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlXPathInit();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGFreeParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlRelaxNGParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlRelaxNGFreeParserCtxt", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlRelaxNGParserCtxtPtr) PyrelaxNgParserCtxt_Get(pyobj_ctxt);

    xmlRelaxNGFreeParserCtxt(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlCheckLanguageID(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * lang;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCheckLanguageID", &lang))
        return(NULL);

    c_retval = xmlCheckLanguageID(lang);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaSetValidOptions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlSchemaValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlSchemaSetValidOptions", &pyobj_ctxt, &options))
        return(NULL);
    ctxt = (xmlSchemaValidCtxtPtr) PySchemaValidCtxt_Get(pyobj_ctxt);

    c_retval = xmlSchemaSetValidOptions(ctxt, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidCtxtNormalizeAttributeValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"OOOzz:xmlValidCtxtNormalizeAttributeValue", &pyobj_ctxt, &pyobj_doc, &pyobj_elem, &name, &value))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlValidCtxtNormalizeAttributeValue(ctxt, doc, elem, name, value);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlFreeNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNsPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeNs", &pyobj_cur))
        return(NULL);
    cur = (xmlNsPtr) PyxmlNode_Get(pyobj_cur);

    xmlFreeNs(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNormalizeFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathNormalizeFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathNormalizeFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlGetNoNsProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlGetNoNsProp", &pyobj_node, &name))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlGetNoNsProp(node, name);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlURIGetPath(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetPath", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->path;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNodeAddContentLen(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlChar * content;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlNodeAddContentLen", &pyobj_cur, &content, &len))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeAddContentLen(cur, content, len);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlRegisterDefaultOutputCallbacks(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlRegisterDefaultOutputCallbacks();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlNodeDumpFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * out;
    PyObject *pyobj_out;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OOO:htmlNodeDumpFile", &pyobj_out, &pyobj_doc, &pyobj_cur))
        return(NULL);
    out = (FILE *) PyFile_Get(pyobj_out);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    htmlNodeDumpFile(out, doc, cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathModValues(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathModValues", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathModValues(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPTR_ENABLED)
PyObject *
libxml_xmlXPtrRangeToFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPtrRangeToFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPtrRangeToFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPTR_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogIsEmpty(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlCatalogIsEmpty", &pyobj_catal))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    c_retval = xmlCatalogIsEmpty(catal);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderClose(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderClose", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderClose(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlLoadSGMLSuperCatalog(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlCatalogPtr c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlLoadSGMLSuperCatalog", &filename))
        return(NULL);

    c_retval = xmlLoadSGMLSuperCatalog(filename);
    py_retval = libxml_xmlCatalogPtrWrap((xmlCatalogPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlCopyChar(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int len;
    xmlChar * out;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"izi:xmlCopyChar", &len, &out, &val))
        return(NULL);

    c_retval = xmlCopyChar(len, out, val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaNewMemParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlSchemaParserCtxtPtr c_retval;
    char * buffer;
    int size;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlSchemaNewMemParserCtxt", &buffer, &size))
        return(NULL);

    c_retval = xmlSchemaNewMemParserCtxt(buffer, size);
    py_retval = libxml_xmlSchemaParserCtxtPtrWrap((xmlSchemaParserCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlGetDtdQElementDesc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlElementPtr c_retval;
    xmlDtdPtr dtd;
    PyObject *pyobj_dtd;
    xmlChar * name;
    xmlChar * prefix;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlGetDtdQElementDesc", &pyobj_dtd, &name, &prefix))
        return(NULL);
    dtd = (xmlDtdPtr) PyxmlNode_Get(pyobj_dtd);

    c_retval = xmlGetDtdQElementDesc(dtd, name, prefix);
    py_retval = libxml_xmlElementPtrWrap((xmlElementPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_VALID_ENABLED) && defined(LIBXML_REGEXP_ENABLED)
PyObject *
libxml_xmlValidatePopElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;
    xmlChar * qname;

    if (!PyArg_ParseTuple(args, (char *)"OOOz:xmlValidatePopElement", &pyobj_ctxt, &pyobj_doc, &pyobj_elem, &qname))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlValidatePopElement(ctxt, doc, elem, qname);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) && defined(LIBXML_REGEXP_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathLocalNameFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathLocalNameFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathLocalNameFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlParserHandleReference(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParserHandleReference", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParserHandleReference(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_HTTP_ENABLED)
PyObject *
libxml_xmlNanoHTTPInit(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlNanoHTTPInit();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTTP_ENABLED) */
PyObject *
libxml_xmlCopyNamespaceList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xmlNsPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlCopyNamespaceList", &pyobj_cur))
        return(NULL);
    cur = (xmlNsPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlCopyNamespaceList(cur);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSpecials(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSpecials", &code))
        return(NULL);

    c_retval = xmlUCSIsSpecials(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseCDSect(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseCDSect", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseCDSect(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLatinExtendedB(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLatinExtendedB", &code))
        return(NULL);

    c_retval = xmlUCSIsLatinExtendedB(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLatinExtendedA(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLatinExtendedA", &code))
        return(NULL);

    c_retval = xmlUCSIsLatinExtendedA(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlCopyDtd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDtdPtr c_retval;
    xmlDtdPtr dtd;
    PyObject *pyobj_dtd;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlCopyDtd", &pyobj_dtd))
        return(NULL);
    dtd = (xmlDtdPtr) PyxmlNode_Get(pyobj_dtd);

    c_retval = xmlCopyDtd(dtd);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlNodeListGetRawString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr list;
    PyObject *pyobj_list;
    int inLine;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlNodeListGetRawString", &pyobj_doc, &pyobj_list, &inLine))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    list = (xmlNodePtr) PyxmlNode_Get(pyobj_list);

    c_retval = xmlNodeListGetRawString(doc, list, inLine);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
PyObject *
libxml_xmlErrorGetLine(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlErrorPtr Error;
    PyObject *pyobj_Error;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlErrorGetLine", &pyobj_Error))
        return(NULL);
    Error = (xmlErrorPtr) PyError_Get(pyobj_Error);

    c_retval = Error->line;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewDocNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlNewDocNode", &pyobj_doc, &pyobj_ns, &name, &content))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlNewDocNode(doc, ns, name, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlParseDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    xmlChar * cur;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"zz:htmlParseDoc", &cur, &encoding))
        return(NULL);

    c_retval = htmlParseDoc(cur, encoding);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGInitTypes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    int c_retval;

    c_retval = xmlRelaxNGInitTypes();
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMiscellaneousMathematicalSymbolsA(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMiscellaneousMathematicalSymbolsA", &code))
        return(NULL);

    c_retval = xmlUCSIsMiscellaneousMathematicalSymbolsA(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathFreeParserContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathFreeParserContext", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathFreeParserContext(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlURIGetAuthority(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetAuthority", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->authority;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCreateURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    xmlURIPtr c_retval;

    c_retval = xmlCreateURI();
    py_retval = libxml_xmlURIPtrWrap((xmlURIPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlStrcat(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * cur;
    xmlChar * add;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlStrcat", &cur, &add))
        return(NULL);

    c_retval = xmlStrcat(cur, add);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaFreeParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlSchemaParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSchemaFreeParserCtxt", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlSchemaParserCtxtPtr) PySchemaParserCtxt_Get(pyobj_ctxt);

    xmlSchemaFreeParserCtxt(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderGetParserLineNumber(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderGetParserLineNumber", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderGetParserLineNumber(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsKhmerSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsKhmerSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsKhmerSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseMarkupDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseMarkupDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseMarkupDecl(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlHasNsProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;
    xmlChar * nameSpace;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlHasNsProp", &pyobj_node, &name, &nameSpace))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlHasNsProp(node, name, nameSpace);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlAddPrevSibling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlAddPrevSibling", &pyobj_cur, &pyobj_elem))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlAddPrevSibling(cur, elem);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlGetDtdAttrDesc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttributePtr c_retval;
    xmlDtdPtr dtd;
    PyObject *pyobj_dtd;
    xmlChar * elem;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlGetDtdAttrDesc", &pyobj_dtd, &elem, &name))
        return(NULL);
    dtd = (xmlDtdPtr) PyxmlNode_Get(pyobj_dtd);

    c_retval = xmlGetDtdAttrDesc(dtd, elem, name);
    py_retval = libxml_xmlAttributePtrWrap((xmlAttributePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlGetMetaEncoding(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    htmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:htmlGetMetaEncoding", &pyobj_doc))
        return(NULL);
    doc = (htmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = htmlGetMetaEncoding(doc);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsEnclosedCJKLettersandMonths(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsEnclosedCJKLettersandMonths", &code))
        return(NULL);

    c_retval = xmlUCSIsEnclosedCJKLettersandMonths(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlGetIntSubset(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDtdPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlGetIntSubset", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlGetIntSubset(doc);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCleanupInputCallbacks(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlCleanupInputCallbacks();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateRoot(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlValidateRoot", &pyobj_ctxt, &pyobj_doc))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlValidateRoot(ctxt, doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlNormalizeURIPath(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * path;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlNormalizeURIPath", &path))
        return(NULL);

    c_retval = xmlNormalizeURIPath(path);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstXmlVersion(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstXmlVersion", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstXmlVersion(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCombiningDiacriticalMarksforSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCombiningDiacriticalMarksforSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsCombiningDiacriticalMarksforSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParserInputBufferRead(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserInputBufferPtr in;
    PyObject *pyobj_in;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlParserInputBufferRead", &pyobj_in, &len))
        return(NULL);
    in = (xmlParserInputBufferPtr) PyinputBuffer_Get(pyobj_in);

    c_retval = xmlParserInputBufferRead(in, len);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTTP_ENABLED)
PyObject *
libxml_xmlIOHTTPMatch(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlIOHTTPMatch", &filename))
        return(NULL);

    c_retval = xmlIOHTTPMatch(filename);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTTP_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNewFloat(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    double val;

    if (!PyArg_ParseTuple(args, (char *)"d:xmlXPathNewFloat", &val))
        return(NULL);

    c_retval = xmlXPathNewFloat(val);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatCc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatCc", &code))
        return(NULL);

    c_retval = xmlUCSIsCatCc(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlURISetServer(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    char * server;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlURISetServer", &pyobj_URI, &server))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    if (URI->server != NULL) xmlFree(URI->server);
    URI->server = (char *)xmlStrdup((const xmlChar *)server);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSpacingModifierLetters(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSpacingModifierLetters", &code))
        return(NULL);

    c_retval = xmlUCSIsSpacingModifierLetters(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
#endif
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHighPrivateUseSurrogates(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHighPrivateUseSurrogates", &code))
        return(NULL);

    c_retval = xmlUCSIsHighPrivateUseSurrogates(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlDefaultSAXHandlerInit(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlDefaultSAXHandlerInit();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBraillePatterns(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsBraillePatterns", &code))
        return(NULL);

    c_retval = xmlUCSIsBraillePatterns(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseAttValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseAttValue", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseAttValue(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlStringGetNodeList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlStringGetNodeList", &pyobj_doc, &value))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlStringGetNodeList(doc, value);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlHandleOmittedElem(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"i:htmlHandleOmittedElem", &val))
        return(NULL);

    c_retval = htmlHandleOmittedElem(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathTrueFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathTrueFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathTrueFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogAdd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * type;
    xmlChar * orig;
    xmlChar * replace;

    if (!PyArg_ParseTuple(args, (char *)"zzz:xmlCatalogAdd", &type, &orig, &replace))
        return(NULL);

    c_retval = xmlCatalogAdd(type, orig, replace);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCombiningDiacriticalMarks(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCombiningDiacriticalMarks", &code))
        return(NULL);

    c_retval = xmlUCSIsCombiningDiacriticalMarks(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathEqualValues(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathEqualValues", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = xmlXPathEqualValues(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlCtxtUseOptions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlCtxtUseOptions", &pyobj_ctxt, &options))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlCtxtUseOptions(ctxt, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsShavian(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsShavian", &code))
        return(NULL);

    c_retval = xmlUCSIsShavian(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHebrew(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHebrew", &code))
        return(NULL);

    c_retval = xmlUCSIsHebrew(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathLangFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathLangFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathLangFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaValidateDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlSchemaValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlSchemaValidateDoc", &pyobj_ctxt, &pyobj_doc))
        return(NULL);
    ctxt = (xmlSchemaValidCtxtPtr) PySchemaValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlSchemaValidateDoc(ctxt, doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlCopyError(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlErrorPtr from;
    PyObject *pyobj_from;
    xmlErrorPtr to;
    PyObject *pyobj_to;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlCopyError", &pyobj_from, &pyobj_to))
        return(NULL);
    from = (xmlErrorPtr) PyError_Get(pyobj_from);
    to = (xmlErrorPtr) PyError_Get(pyobj_to);

    c_retval = xmlCopyError(from, to);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGValidateDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlRelaxNGValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlRelaxNGValidateDoc", &pyobj_ctxt, &pyobj_doc))
        return(NULL);
    ctxt = (xmlRelaxNGValidCtxtPtr) PyrelaxNgValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlRelaxNGValidateDoc(ctxt, doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlNodeSetSpacePreserve(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlNodeSetSpacePreserve", &pyobj_cur, &val))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeSetSpacePreserve(cur, val);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsArmenian(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsArmenian", &code))
        return(NULL);

    c_retval = xmlUCSIsArmenian(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCastNodeToNumber(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    double c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathCastNodeToNumber", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlXPathCastNodeToNumber(node);
    py_retval = libxml_doubleWrap((double) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlUTF8Size(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * utf;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlUTF8Size", &utf))
        return(NULL);

    c_retval = xmlUTF8Size(utf);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewText(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlNewText", &content))
        return(NULL);

    c_retval = xmlNewText(content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlDocCopyNodeList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlDocCopyNodeList", &pyobj_doc, &pyobj_node))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlDocCopyNodeList(doc, node);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewDocText(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNewDocText", &pyobj_doc, &content))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewDocText(doc, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewReference(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNewReference", &pyobj_doc, &name))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewReference(doc, name);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNewValueTree(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlNodePtr val;
    PyObject *pyobj_val;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathNewValueTree", &pyobj_val))
        return(NULL);
    val = (xmlNodePtr) PyxmlNode_Get(pyobj_val);

    c_retval = xmlXPathNewValueTree(val);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSupplementalMathematicalOperators(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSupplementalMathematicalOperators", &code))
        return(NULL);

    c_retval = xmlUCSIsSupplementalMathematicalOperators(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlOutputBufferWriteString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlOutputBufferPtr out;
    PyObject *pyobj_out;
    char * str;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlOutputBufferWriteString", &pyobj_out, &str))
        return(NULL);
    out = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_out);

    c_retval = xmlOutputBufferWriteString(out, str);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateDtd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlDtdPtr dtd;
    PyObject *pyobj_dtd;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlValidateDtd", &pyobj_ctxt, &pyobj_doc, &pyobj_dtd))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    dtd = (xmlDtdPtr) PyxmlNode_Get(pyobj_dtd);

    c_retval = xmlValidateDtd(ctxt, doc, dtd);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlIsBlank(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned int ch;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsBlank", &ch))
        return(NULL);

    c_retval = xmlIsBlank(ch);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPTR_ENABLED)
PyObject *
libxml_xmlXPtrNewLocationSetNodes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlNodePtr start;
    PyObject *pyobj_start;
    xmlNodePtr end;
    PyObject *pyobj_end;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPtrNewLocationSetNodes", &pyobj_start, &pyobj_end))
        return(NULL);
    start = (xmlNodePtr) PyxmlNode_Get(pyobj_start);
    end = (xmlNodePtr) PyxmlNode_Get(pyobj_end);

    c_retval = xmlXPtrNewLocationSetNodes(start, end);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPTR_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCombiningMarksforSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCombiningMarksforSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsCombiningMarksforSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlValidateElement", &pyobj_ctxt, &pyobj_doc, &pyobj_elem))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlValidateElement(ctxt, doc, elem);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlPopInputCallbacks(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    int c_retval;

    c_retval = xmlPopInputCallbacks();
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLao(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLao", &code))
        return(NULL);

    c_retval = xmlUCSIsLao(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlNewDocFragment(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlNewDocFragment", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewDocFragment(doc);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlReadMemory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    char * buffer;
    int py_buffsize0;
    int size;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"t#izzi:htmlReadMemory", &buffer, &py_buffsize0, &size, &URL, &encoding, &options))
        return(NULL);

    c_retval = htmlReadMemory(buffer, size, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNodeSetFreeNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNsPtr ns;
    PyObject *pyobj_ns;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathNodeSetFreeNs", &pyobj_ns))
        return(NULL);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    xmlXPathNodeSetFreeNs(ns);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderHasAttributes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderHasAttributes", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderHasAttributes(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGothic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGothic", &code))
        return(NULL);

    c_retval = xmlUCSIsGothic(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlNodeDumpOutput(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlOutputBufferPtr buf;
    PyObject *pyobj_buf;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    int level;
    int format;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"OOOiiz:xmlNodeDumpOutput", &pyobj_buf, &pyobj_doc, &pyobj_cur, &level, &format, &encoding))
        return(NULL);
    buf = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_buf);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeDumpOutput(buf, doc, cur, level, format, encoding);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathRegisteredFuncsCleanup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathRegisteredFuncsCleanup", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    xmlXPathRegisteredFuncsCleanup(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBlock(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;
    char * block;

    if (!PyArg_ParseTuple(args, (char *)"iz:xmlUCSIsBlock", &code, &block))
        return(NULL);

    c_retval = xmlUCSIsBlock(code, block);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderMoveToNextAttribute(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderMoveToNextAttribute", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderMoveToNextAttribute(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatNd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatNd", &code))
        return(NULL);

    c_retval = xmlUCSIsCatNd(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseSDDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseSDDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseSDDecl(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderNewWalker(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlReaderNewWalker", &pyobj_reader, &pyobj_doc))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlReaderNewWalker(reader, doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatNl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatNl", &code))
        return(NULL);

    c_retval = xmlUCSIsCatNl(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatNo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatNo", &code))
        return(NULL);

    c_retval = xmlUCSIsCatNo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlSkipBlankChars(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSkipBlankChars", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlSkipBlankChars(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateNmtokenValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlValidateNmtokenValue", &value))
        return(NULL);

    c_retval = xmlValidateNmtokenValue(value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlGetNodePath(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlGetNodePath", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlGetNodePath(node);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlDocContentDumpOutput(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlOutputBufferPtr buf;
    PyObject *pyobj_buf;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"OOz:htmlDocContentDumpOutput", &pyobj_buf, &pyobj_cur, &encoding))
        return(NULL);
    buf = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_buf);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    htmlDocContentDumpOutput(buf, cur, encoding);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathPopBoolean(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathPopBoolean", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = xmlXPathPopBoolean(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlIsIdeographic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned int ch;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsIdeographic", &ch))
        return(NULL);

    c_retval = xmlIsIdeographic(ch);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLatinExtendedAdditional(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLatinExtendedAdditional", &code))
        return(NULL);

    c_retval = xmlUCSIsLatinExtendedAdditional(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlURISetAuthority(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    char * authority;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlURISetAuthority", &pyobj_URI, &authority))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    if (URI->authority != NULL) xmlFree(URI->authority);
    URI->authority = (char *)xmlStrdup((const xmlChar *)authority);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGValidatePushCData(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlRelaxNGValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * data;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlRelaxNGValidatePushCData", &pyobj_ctxt, &data, &len))
        return(NULL);
    ctxt = (xmlRelaxNGValidCtxtPtr) PyrelaxNgValidCtxt_Get(pyobj_ctxt);

    c_retval = xmlRelaxNGValidatePushCData(ctxt, data, len);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlGetLastError(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    xmlErrorPtr c_retval;

    c_retval = xmlGetLastError();
    py_retval = libxml_xmlErrorPtrWrap((xmlErrorPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlEncodeEntitiesReentrant(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * input;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlEncodeEntitiesReentrant", &pyobj_doc, &input))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlEncodeEntitiesReentrant(doc, input);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlRemoveProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlAttrPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlRemoveProp", &pyobj_cur))
        return(NULL);
    cur = (xmlAttrPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlRemoveProp(cur);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlACatalogDump(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;
    FILE * out;
    PyObject *pyobj_out;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlACatalogDump", &pyobj_catal, &pyobj_out))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);
    out = (FILE *) PyFile_Get(pyobj_out);

    xmlACatalogDump(catal, out);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
PyObject *
libxml_xmlReadFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    char * filename;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"zzi:xmlReadFile", &filename, &encoding, &options))
        return(NULL);

    c_retval = xmlReadFile(filename, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsNumberForms(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsNumberForms", &code))
        return(NULL);

    c_retval = xmlUCSIsNumberForms(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlStrncmp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * str1;
    xmlChar * str2;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zzi:xmlStrncmp", &str1, &str2, &len))
        return(NULL);

    c_retval = xmlStrncmp(str1, str2, len);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogGetPublic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlChar * pubID;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCatalogGetPublic", &pubID))
        return(NULL);

    c_retval = xmlCatalogGetPublic(pubID);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlSaveFormatFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    int format;

    if (!PyArg_ParseTuple(args, (char *)"zOi:xmlSaveFormatFile", &filename, &pyobj_cur, &format))
        return(NULL);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlSaveFormatFile(filename, cur, format);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
PyObject *
libxml_xmlParseXMLDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseXMLDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseXMLDecl(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlNewComment(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlNewComment", &content))
        return(NULL);

    c_retval = xmlNewComment(content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGNewValidCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlRelaxNGValidCtxtPtr c_retval;
    xmlRelaxNGPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlRelaxNGNewValidCtxt", &pyobj_schema))
        return(NULL);
    schema = (xmlRelaxNGPtr) PyrelaxNgSchema_Get(pyobj_schema);

    c_retval = xmlRelaxNGNewValidCtxt(schema);
    py_retval = libxml_xmlRelaxNGValidCtxtPtrWrap((xmlRelaxNGValidCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsKatakana(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsKatakana", &code))
        return(NULL);

    c_retval = xmlUCSIsKatakana(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHalfwidthandFullwidthForms(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHalfwidthandFullwidthForms", &code))
        return(NULL);

    c_retval = xmlUCSIsHalfwidthandFullwidthForms(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateNamesValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlValidateNamesValue", &value))
        return(NULL);

    c_retval = xmlValidateNamesValue(value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlParseURIReference(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlURIPtr uri;
    PyObject *pyobj_uri;
    char * str;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlParseURIReference", &pyobj_uri, &str))
        return(NULL);
    uri = (xmlURIPtr) PyURI_Get(pyobj_uri);

    c_retval = xmlParseURIReference(uri, str);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathOrderDocElems(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    long c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathOrderDocElems", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlXPathOrderDocElems(doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGurmukhi(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGurmukhi", &code))
        return(NULL);

    c_retval = xmlUCSIsGurmukhi(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_namePush(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"Oz:namePush", &pyobj_ctxt, &value))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = namePush(ctxt, value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED)
PyObject *
libxml_xmlNodeSetBase(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlChar * uri;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNodeSetBase", &pyobj_cur, &uri))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeSetBase(cur, uri);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) */
#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlTextReaderRelaxNGSetSchema(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlRelaxNGPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlTextReaderRelaxNGSetSchema", &pyobj_reader, &pyobj_schema))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);
    schema = (xmlRelaxNGPtr) PyrelaxNgSchema_Get(pyobj_schema);

    c_retval = xmlTextReaderRelaxNGSetSchema(reader, schema);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) && defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpAttr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlAttrPtr attr;
    PyObject *pyobj_attr;
    int depth;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlDebugDumpAttr", &pyobj_output, &pyobj_attr, &depth))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    attr = (xmlAttrPtr) PyxmlNode_Get(pyobj_attr);

    xmlDebugDumpAttr(output, attr, depth);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlCleanupOutputCallbacks(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlCleanupOutputCallbacks();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathSetContextNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathSetContextNode", &pyobj_ctxt, &pyobj_node))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    ctxt->node = node;
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlSaveFileEnc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"zOz:xmlSaveFileEnc", &filename, &pyobj_cur, &encoding))
        return(NULL);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlSaveFileEnc(filename, cur, encoding);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathGetFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathGetFunction", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = ctxt->function;
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpOneNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlNodePtr node;
    PyObject *pyobj_node;
    int depth;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlDebugDumpOneNode", &pyobj_output, &pyobj_node, &depth))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    xmlDebugDumpOneNode(output, node, depth);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
PyObject *
libxml_xmlNewNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * href;
    xmlChar * prefix;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlNewNs", &pyobj_node, &href, &prefix))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlNewNs(node, href, prefix);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlStrcasestr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlChar * str;
    xmlChar * val;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlStrcasestr", &str, &val))
        return(NULL);

    c_retval = xmlStrcasestr(str, val);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderReadState(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderReadState", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderReadState(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHangulSyllables(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHangulSyllables", &code))
        return(NULL);

    c_retval = xmlUCSIsHangulSyllables(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlValidateQName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;
    int space;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlValidateQName", &value, &space))
        return(NULL);

    c_retval = xmlValidateQName(value, space);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCompareValues(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int inf;
    int strict;

    if (!PyArg_ParseTuple(args, (char *)"Oii:xmlXPathCompareValues", &pyobj_ctxt, &inf, &strict))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = xmlXPathCompareValues(ctxt, inf, strict);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSyriac(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSyriac", &code))
        return(NULL);

    c_retval = xmlUCSIsSyriac(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlStrQEqual(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * pref;
    xmlChar * name;
    xmlChar * str;

    if (!PyArg_ParseTuple(args, (char *)"zzz:xmlStrQEqual", &pref, &name, &str))
        return(NULL);

    c_retval = xmlStrQEqual(pref, name, str);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlBuildURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * URI;
    xmlChar * base;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlBuildURI", &URI, &base))
        return(NULL);

    c_retval = xmlBuildURI(URI, base);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderGetParserColumnNumber(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderGetParserColumnNumber", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderGetParserColumnNumber(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_valuePop(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:valuePop", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = valuePop(ctxt);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathContainsFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathContainsFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathContainsFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlCtxtUseOptions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Oi:htmlCtxtUseOptions", &pyobj_ctxt, &options))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = htmlCtxtUseOptions(ctxt, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogConvert(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    int c_retval;

    c_retval = xmlCatalogConvert();
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_VALID_ENABLED) && defined(LIBXML_REGEXP_ENABLED)
PyObject *
libxml_xmlValidatePushElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;
    xmlChar * qname;

    if (!PyArg_ParseTuple(args, (char *)"OOOz:xmlValidatePushElement", &pyobj_ctxt, &pyobj_doc, &pyobj_elem, &qname))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlValidatePushElement(ctxt, doc, elem, qname);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) && defined(LIBXML_REGEXP_ENABLED) */
PyObject *
libxml_xmlResetError(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlErrorPtr err;
    PyObject *pyobj_err;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlResetError", &pyobj_err))
        return(NULL);
    err = (xmlErrorPtr) PyError_Get(pyobj_err);

    xmlResetError(err);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsArrows(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsArrows", &code))
        return(NULL);

    c_retval = xmlUCSIsArrows(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathGetContextSize(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathGetContextSize", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = ctxt->contextSize;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLimbu(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLimbu", &code))
        return(NULL);

    c_retval = xmlUCSIsLimbu(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlRemoveID(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlAttrPtr attr;
    PyObject *pyobj_attr;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlRemoveID", &pyobj_doc, &pyobj_attr))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    attr = (xmlAttrPtr) PyxmlNode_Get(pyobj_attr);

    c_retval = xmlRemoveID(doc, attr);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewDocPI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlNewDocPI", &pyobj_doc, &name, &content))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewDocPI(doc, name, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathTranslateFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathTranslateFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathTranslateFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlNodeGetSpacePreserve(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlNodeGetSpacePreserve", &pyobj_cur))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlNodeGetSpacePreserve(cur);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlResetLastError(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlResetLastError();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlXPathIsNaN(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    double val;

    if (!PyArg_ParseTuple(args, (char *)"d:xmlXPathIsNaN", &val))
        return(NULL);

    c_retval = xmlXPathIsNaN(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateDtdFinal(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlValidateDtdFinal", &pyobj_ctxt, &pyobj_doc))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlValidateDtdFinal(ctxt, doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlParseEncName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseEncName", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseEncName(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextAttribute(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextAttribute", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextAttribute(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPTR_ENABLED)
PyObject *
libxml_xmlXPtrEvalRangePredicate(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPtrEvalRangePredicate", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPtrEvalRangePredicate(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPTR_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlAutoCloseTag(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    htmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;
    htmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OzO:htmlAutoCloseTag", &pyobj_doc, &name, &pyobj_elem))
        return(NULL);
    doc = (htmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (htmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = htmlAutoCloseTag(doc, name, elem);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlThrDefLoadExtDtdDefaultValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefLoadExtDtdDefaultValue", &v))
        return(NULL);

    c_retval = xmlThrDefLoadExtDtdDefaultValue(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlThrDefTreeIndentString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    char * v;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlThrDefTreeIndentString", &v))
        return(NULL);

    c_retval = xmlThrDefTreeIndentString(v);
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlGetDocCompressMode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlGetDocCompressMode", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlGetDocCompressMode(doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpDocumentHead(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlDebugDumpDocumentHead", &pyobj_output, &pyobj_doc))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    xmlDebugDumpDocumentHead(output, doc);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlNodeDumpOutput(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlOutputBufferPtr buf;
    PyObject *pyobj_buf;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"OOOz:htmlNodeDumpOutput", &pyobj_buf, &pyobj_doc, &pyobj_cur, &encoding))
        return(NULL);
    buf = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_buf);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    htmlNodeDumpOutput(buf, doc, cur, encoding);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlParseElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:htmlParseElement", &pyobj_ctxt))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    htmlParseElement(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlSubstituteEntitiesDefault(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlSubstituteEntitiesDefault", &val))
        return(NULL);

    c_retval = xmlSubstituteEntitiesDefault(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGreek(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGreek", &code))
        return(NULL);

    c_retval = xmlUCSIsGreek(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlDecodeEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int len;
    int what;
    xmlChar end;
    xmlChar end2;
    xmlChar end3;

    if (!PyArg_ParseTuple(args, (char *)"Oiiccc:xmlDecodeEntities", &pyobj_ctxt, &len, &what, &end, &end2, &end3))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlDecodeEntities(ctxt, len, what, end, end2, end3);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlNamespaceParseNSDef(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlNamespaceParseNSDef", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlNamespaceParseNSDef(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCastNumberToString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    double val;

    if (!PyArg_ParseTuple(args, (char *)"d:xmlXPathCastNumberToString", &val))
        return(NULL);

    c_retval = xmlXPathCastNumberToString(val);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogRemove(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCatalogRemove", &value))
        return(NULL);

    c_retval = xmlCatalogRemove(value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlOutputBufferWrite(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlOutputBufferPtr out;
    PyObject *pyobj_out;
    int len;
    char * buf;

    if (!PyArg_ParseTuple(args, (char *)"Oiz:xmlOutputBufferWrite", &pyobj_out, &len, &buf))
        return(NULL);
    out = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_out);

    c_retval = xmlOutputBufferWrite(out, len, buf);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_FTP_ENABLED)
PyObject *
libxml_xmlIOFTPMatch(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlIOFTPMatch", &filename))
        return(NULL);

    c_retval = xmlIOFTPMatch(filename);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_FTP_ENABLED) */
PyObject *
libxml_xmlParseReference(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseReference", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseReference(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_HTTP_ENABLED)
PyObject *
libxml_xmlNanoHTTPScanProxy(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    char * URL;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlNanoHTTPScanProxy", &URL))
        return(NULL);

    xmlNanoHTTPScanProxy(URL);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTTP_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatMc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatMc", &code))
        return(NULL);

    c_retval = xmlUCSIsCatMc(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlStringLenGetNodeList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * value;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlStringLenGetNodeList", &pyobj_doc, &value, &len))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlStringLenGetNodeList(doc, value, len);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderLocatorBaseURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlTextReaderLocatorPtr locator;
    PyObject *pyobj_locator;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderLocatorBaseURI", &pyobj_locator))
        return(NULL);
    locator = (xmlTextReaderLocatorPtr) PyxmlTextReaderLocator_Get(pyobj_locator);

    c_retval = xmlTextReaderLocatorBaseURI(locator);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_xmlSetNsProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlSetNsProp", &pyobj_node, &pyobj_ns, &name, &value))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlSetNsProp(node, ns, name, value);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlSAXDefaultVersion(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int version;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlSAXDefaultVersion", &version))
        return(NULL);

    c_retval = xmlSAXDefaultVersion(version);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
#if defined(LIBXML_VALID_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlValidateNotationUse(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * notationName;

    if (!PyArg_ParseTuple(args, (char *)"OOz:xmlValidateNotationUse", &pyobj_ctxt, &pyobj_doc, &notationName))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlValidateNotationUse(ctxt, doc, notationName);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlGetCompressMode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    int c_retval;

    c_retval = xmlGetCompressMode();
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlNewDocNoDtD(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    xmlChar * URI;
    xmlChar * ExternalID;

    if (!PyArg_ParseTuple(args, (char *)"zz:htmlNewDocNoDtD", &URI, &ExternalID))
        return(NULL);

    c_retval = htmlNewDocNoDtD(URI, ExternalID);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlURIEscape(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * str;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlURIEscape", &str))
        return(NULL);

    c_retval = xmlURIEscape(str);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlDocContentDumpFormatOutput(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlOutputBufferPtr buf;
    PyObject *pyobj_buf;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    char * encoding;
    int format;

    if (!PyArg_ParseTuple(args, (char *)"OOzi:htmlDocContentDumpFormatOutput", &pyobj_buf, &pyobj_cur, &encoding, &format))
        return(NULL);
    buf = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_buf);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    htmlDocContentDumpFormatOutput(buf, cur, encoding, format);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
PyObject *
libxml_xmlURISetQuery(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    char * query;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlURISetQuery", &pyobj_URI, &query))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    if (URI->query != NULL) xmlFree(URI->query);
    URI->query = (char *)xmlStrdup((const xmlChar *)query);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlTextReaderSchemaValidate(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    char * xsd;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlTextReaderSchemaValidate", &pyobj_reader, &xsd))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderSchemaValidate(reader, xsd);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) && defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGreekandCoptic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGreekandCoptic", &code))
        return(NULL);

    c_retval = xmlUCSIsGreekandCoptic(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlUTF8Strlen(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * utf;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlUTF8Strlen", &utf))
        return(NULL);

    c_retval = xmlUTF8Strlen(utf);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathAddValues(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathAddValues", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathAddValues(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlStrchr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlChar * str;
    xmlChar val;

    if (!PyArg_ParseTuple(args, (char *)"zc:xmlStrchr", &str, &val))
        return(NULL);

    c_retval = xmlStrchr(str, val);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewTextLen(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlChar * content;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlNewTextLen", &content, &len))
        return(NULL);

    c_retval = xmlNewTextLen(content, len);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlXPathIsInf(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    double val;

    if (!PyArg_ParseTuple(args, (char *)"d:xmlXPathIsInf", &val))
        return(NULL);

    c_retval = xmlXPathIsInf(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKUnifiedIdeographs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKUnifiedIdeographs", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKUnifiedIdeographs(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlValidateName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;
    int space;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlValidateName", &value, &space))
        return(NULL);

    c_retval = xmlValidateName(value, space);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlChar * str;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlTextReaderConstString", &pyobj_reader, &str))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstString(reader, str);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaValidateFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlSchemaValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    char * filename;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlSchemaValidateFile", &pyobj_ctxt, &filename, &options))
        return(NULL);
    ctxt = (xmlSchemaValidCtxtPtr) PySchemaValidCtxt_Get(pyobj_ctxt);

    c_retval = xmlSchemaValidateFile(ctxt, filename, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlAddNextSibling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlAddNextSibling", &pyobj_cur, &pyobj_elem))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlAddNextSibling(cur, elem);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSupplementalArrowsA(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSupplementalArrowsA", &code))
        return(NULL);

    c_retval = xmlUCSIsSupplementalArrowsA(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSupplementalArrowsB(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSupplementalArrowsB", &code))
        return(NULL);

    c_retval = xmlUCSIsSupplementalArrowsB(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlErrorGetMessage(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlErrorPtr Error;
    PyObject *pyobj_Error;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlErrorGetMessage", &pyobj_Error))
        return(NULL);
    Error = (xmlErrorPtr) PyError_Get(pyobj_Error);

    c_retval = Error->message;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
#endif
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathFalseFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathFalseFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathFalseFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderHasValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderHasValue", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderHasValue(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlRelaxNGDumpTree(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlRelaxNGPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlRelaxNGDumpTree", &pyobj_output, &pyobj_schema))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    schema = (xmlRelaxNGPtr) PyrelaxNgSchema_Get(pyobj_schema);

    xmlRelaxNGDumpTree(output, schema);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_REGEXP_ENABLED)
PyObject *
libxml_xmlRegexpPrint(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlRegexpPtr regexp;
    PyObject *pyobj_regexp;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlRegexpPrint", &pyobj_output, &pyobj_regexp))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    regexp = (xmlRegexpPtr) PyxmlReg_Get(pyobj_regexp);

    xmlRegexpPrint(output, regexp);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_REGEXP_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlNewValidCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    xmlValidCtxtPtr c_retval;

    c_retval = xmlNewValidCtxt();
    py_retval = libxml_xmlValidCtxtPtrWrap((xmlValidCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlURIEscapeStr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * str;
    xmlChar * list;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlURIEscapeStr", &str, &list))
        return(NULL);

    c_retval = xmlURIEscapeStr(str, list);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCountFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathCountFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathCountFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderNext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderNext", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderNext(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlParserSetPedantic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int pedantic;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlParserSetPedantic", &pyobj_ctxt, &pedantic))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    ctxt->pedantic = pedantic;
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatLu(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatLu", &code))
        return(NULL);

    c_retval = xmlUCSIsCatLu(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatLt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatLt", &code))
        return(NULL);

    c_retval = xmlUCSIsCatLt(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatLo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatLo", &code))
        return(NULL);

    c_retval = xmlUCSIsCatLo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlIsPubidChar(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned int ch;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsPubidChar", &ch))
        return(NULL);

    c_retval = xmlIsPubidChar(ch);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatLm(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatLm", &code))
        return(NULL);

    c_retval = xmlUCSIsCatLm(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatLl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatLl", &code))
        return(NULL);

    c_retval = xmlUCSIsCatLl(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlNewDocProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlNewDocProp", &pyobj_doc, &name, &value))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewDocProp(doc, name, value);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlLoadACatalog(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlCatalogPtr c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlLoadACatalog", &filename))
        return(NULL);

    c_retval = xmlLoadACatalog(filename);
    py_retval = libxml_xmlCatalogPtrWrap((xmlCatalogPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_REGEXP_ENABLED)
PyObject *
libxml_xmlRegexpExec(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlRegexpPtr comp;
    PyObject *pyobj_comp;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlRegexpExec", &pyobj_comp, &content))
        return(NULL);
    comp = (xmlRegexpPtr) PyxmlReg_Get(pyobj_comp);

    c_retval = xmlRegexpExec(comp, content);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_REGEXP_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatPe(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatPe", &code))
        return(NULL);

    c_retval = xmlUCSIsCatPe(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlByteConsumed(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    long c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlByteConsumed", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlByteConsumed(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlHasProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlHasProp", &pyobj_node, &name))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlHasProp(node, name);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlURISetScheme(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    char * scheme;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlURISetScheme", &pyobj_URI, &scheme))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    if (URI->scheme != NULL) xmlFree(URI->scheme);
    URI->scheme = (char *)xmlStrdup((const xmlChar *)scheme);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMiscellaneousSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMiscellaneousSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsMiscellaneousSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlGetDtdQAttrDesc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttributePtr c_retval;
    xmlDtdPtr dtd;
    PyObject *pyobj_dtd;
    xmlChar * elem;
    xmlChar * name;
    xmlChar * prefix;

    if (!PyArg_ParseTuple(args, (char *)"Ozzz:xmlGetDtdQAttrDesc", &pyobj_dtd, &elem, &name, &prefix))
        return(NULL);
    dtd = (xmlDtdPtr) PyxmlNode_Get(pyobj_dtd);

    c_retval = xmlGetDtdQAttrDesc(dtd, elem, name, prefix);
    py_retval = libxml_xmlAttributePtrWrap((xmlAttributePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlSetNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNsPtr ns;
    PyObject *pyobj_ns;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlSetNs", &pyobj_node, &pyobj_ns))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    xmlSetNs(node, ns);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlGetDtdEntity(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlEntityPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlGetDtdEntity", &pyobj_doc, &name))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlGetDtdEntity(doc, name);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlIsXHTML(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * systemID;
    xmlChar * publicID;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlIsXHTML", &systemID, &publicID))
        return(NULL);

    c_retval = xmlIsXHTML(systemID, publicID);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlURIUnescapeString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    char * c_retval;
    char * str;
    int len;
    char * target;

    if (!PyArg_ParseTuple(args, (char *)"ziz:xmlURIUnescapeString", &str, &len, &target))
        return(NULL);

    c_retval = xmlURIUnescapeString(str, len, target);
    py_retval = libxml_charPtrWrap((char *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsRunic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsRunic", &code))
        return(NULL);

    c_retval = xmlUCSIsRunic(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlGetParameterEntity(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlEntityPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlGetParameterEntity", &pyobj_doc, &name))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlGetParameterEntity(doc, name);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewDocTextLen(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * content;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlNewDocTextLen", &pyobj_doc, &content, &len))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewDocTextLen(doc, content, len);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathParseName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathParseName", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = xmlXPathParseName(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlURISetPath(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    char * path;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlURISetPath", &pyobj_URI, &path))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    if (URI->path != NULL) xmlFree(URI->path);
    URI->path = (char *)xmlStrdup((const xmlChar *)path);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlErrorGetFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlErrorPtr Error;
    PyObject *pyobj_Error;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlErrorGetFile", &pyobj_Error))
        return(NULL);
    Error = (xmlErrorPtr) PyError_Get(pyobj_Error);

    c_retval = Error->file;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlGetProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlGetProp", &pyobj_node, &name))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlGetProp(node, name);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlACatalogResolveURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;
    xmlChar * URI;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlACatalogResolveURI", &pyobj_catal, &URI))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    c_retval = xmlACatalogResolveURI(catal, URI);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsVariationSelectors(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsVariationSelectors", &code))
        return(NULL);

    c_retval = xmlUCSIsVariationSelectors(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlLoadCatalog(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlLoadCatalog", &filename))
        return(NULL);

    c_retval = xmlLoadCatalog(filename);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathEval(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlChar * str;
    xmlXPathContextPtr ctx;
    PyObject *pyobj_ctx;

    if (!PyArg_ParseTuple(args, (char *)"zO:xmlXPathEval", &str, &pyobj_ctx))
        return(NULL);
    ctx = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctx);

    c_retval = xmlXPathEval(str, ctx);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsTags(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsTags", &code))
        return(NULL);

    c_retval = xmlUCSIsTags(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlNewPI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlChar * name;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlNewPI", &name, &content))
        return(NULL);

    c_retval = xmlNewPI(name, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLowSurrogates(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLowSurrogates", &code))
        return(NULL);

    c_retval = xmlUCSIsLowSurrogates(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsOsmanya(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsOsmanya", &code))
        return(NULL);

    c_retval = xmlUCSIsOsmanya(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlThrDefDoValidityCheckingDefaultValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefDoValidityCheckingDefaultValue", &v))
        return(NULL);

    c_retval = xmlThrDefDoValidityCheckingDefaultValue(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBoxDrawing(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsBoxDrawing", &code))
        return(NULL);

    c_retval = xmlUCSIsBoxDrawing(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlStrndup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * cur;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlStrndup", &cur, &len))
        return(NULL);

    c_retval = xmlStrndup(cur, len);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderIsValid(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderIsValid", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderIsValid(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsByzantineMusicalSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsByzantineMusicalSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsByzantineMusicalSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlDefaultSAXHandlerInit(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    htmlDefaultSAXHandlerInit();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED) && defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlShellPrintXPathError(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    int errorType;
    char * arg;

    if (!PyArg_ParseTuple(args, (char *)"iz:xmlShellPrintXPathError", &errorType, &arg))
        return(NULL);

    xmlShellPrintXPathError(errorType, arg);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) && defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogResolve(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * pubID;
    xmlChar * sysID;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlCatalogResolve", &pubID, &sysID))
        return(NULL);

    c_retval = xmlCatalogResolve(pubID, sysID);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstName", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstName(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaNewValidCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlSchemaValidCtxtPtr c_retval;
    xmlSchemaPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSchemaNewValidCtxt", &pyobj_schema))
        return(NULL);
    schema = (xmlSchemaPtr) PySchema_Get(pyobj_schema);

    c_retval = xmlSchemaNewValidCtxt(schema);
    py_retval = libxml_xmlSchemaValidCtxtPtrWrap((xmlSchemaValidCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsKhmer(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsKhmer", &code))
        return(NULL);

    c_retval = xmlUCSIsKhmer(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseCharRef(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseCharRef", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseCharRef(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCopyCharMultiByte(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * out;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlCopyCharMultiByte", &out, &val))
        return(NULL);

    c_retval = xmlCopyCharMultiByte(out, val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseVersionNum(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseVersionNum", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseVersionNum(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderWalker(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlTextReaderPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlReaderWalker", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlReaderWalker(doc);
    py_retval = libxml_xmlTextReaderPtrWrap((xmlTextReaderPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderNodeType(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderNodeType", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderNodeType(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlIsBlankNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlIsBlankNode", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlIsBlankNode(node);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGFree(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlRelaxNGPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlRelaxNGFree", &pyobj_schema))
        return(NULL);
    schema = (xmlRelaxNGPtr) PyrelaxNgSchema_Get(pyobj_schema);

    xmlRelaxNGFree(schema);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlFreeProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlAttrPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeProp", &pyobj_cur))
        return(NULL);
    cur = (xmlAttrPtr) PyxmlNode_Get(pyobj_cur);

    xmlFreeProp(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlStrcmp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * str1;
    xmlChar * str2;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlStrcmp", &str1, &str2))
        return(NULL);

    c_retval = xmlStrcmp(str1, str2);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
PyObject *
libxml_xmlDocSetRootElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr root;
    PyObject *pyobj_root;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlDocSetRootElement", &pyobj_doc, &pyobj_root))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    root = (xmlNodePtr) PyxmlNode_Get(pyobj_root);

    c_retval = xmlDocSetRootElement(doc, root);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED) */
PyObject *
libxml_xmlCheckVersion(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    int version;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlCheckVersion", &version))
        return(NULL);

    xmlCheckVersion(version);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_REGEXP_ENABLED)
PyObject *
libxml_xmlRegFreeRegexp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlRegexpPtr regexp;
    PyObject *pyobj_regexp;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlRegFreeRegexp", &pyobj_regexp))
        return(NULL);
    regexp = (xmlRegexpPtr) PyxmlReg_Get(pyobj_regexp);

    xmlRegFreeRegexp(regexp);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_REGEXP_ENABLED) */
PyObject *
libxml_xmlSearchNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * nameSpace;

    if (!PyArg_ParseTuple(args, (char *)"OOz:xmlSearchNs", &pyobj_doc, &pyobj_node, &nameSpace))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlSearchNs(doc, node, nameSpace);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathParserGetContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathContextPtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathParserGetContext", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = ctxt->context;
    py_retval = libxml_xmlXPathContextPtrWrap((xmlXPathContextPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderReadAttributeValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderReadAttributeValue", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderReadAttributeValue(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_XINCLUDE_ENABLED)
PyObject *
libxml_xmlXIncludeProcessTreeFlags(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr tree;
    PyObject *pyobj_tree;
    int flags;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXIncludeProcessTreeFlags", &pyobj_tree, &flags))
        return(NULL);
    tree = (xmlNodePtr) PyxmlNode_Get(pyobj_tree);

    c_retval = xmlXIncludeProcessTreeFlags(tree, flags);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XINCLUDE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGeorgian(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGeorgian", &code))
        return(NULL);

    c_retval = xmlUCSIsGeorgian(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParserSetValidate(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int validate;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlParserSetValidate", &pyobj_ctxt, &validate))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    ctxt->validate = validate;
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidNormalizeAttributeValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlValidNormalizeAttributeValue", &pyobj_doc, &pyobj_elem, &name, &value))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlValidNormalizeAttributeValue(doc, elem, name, value);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlParsePubidLiteral(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParsePubidLiteral", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParsePubidLiteral(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewCharRef(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNewCharRef", &pyobj_doc, &name))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewCharRef(doc, name);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsArabic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsArabic", &code))
        return(NULL);

    c_retval = xmlUCSIsArabic(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMiscellaneousMathematicalSymbolsB(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMiscellaneousMathematicalSymbolsB", &code))
        return(NULL);

    c_retval = xmlUCSIsMiscellaneousMathematicalSymbolsB(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTTP_ENABLED)
PyObject *
libxml_xmlNanoHTTPCleanup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlNanoHTTPCleanup();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTTP_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlParseQuotedString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseQuotedString", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseQuotedString(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCastStringToNumber(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    double c_retval;
    xmlChar * val;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlXPathCastStringToNumber", &val))
        return(NULL);

    c_retval = xmlXPathCastStringToNumber(val);
    py_retval = libxml_doubleWrap((double) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNewCString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    char * val;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlXPathNewCString", &val))
        return(NULL);

    c_retval = xmlXPathNewCString(val);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderIsNamespaceDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderIsNamespaceDecl", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderIsNamespaceDecl(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlStopParser(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlStopParser", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlStopParser(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlReadFd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    int fd;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"izzi:xmlReadFd", &fd, &URL, &encoding, &options))
        return(NULL);

    c_retval = xmlReadFd(fd, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlACatalogResolveSystem(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;
    xmlChar * sysID;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlACatalogResolveSystem", &pyobj_catal, &sysID))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    c_retval = xmlACatalogResolveSystem(catal, sysID);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlCreateDocParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlParserCtxtPtr c_retval;
    xmlChar * cur;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCreateDocParserCtxt", &cur))
        return(NULL);

    c_retval = xmlCreateDocParserCtxt(cur);
    py_retval = libxml_xmlParserCtxtPtrWrap((xmlParserCtxtPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsTaiXuanJingSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsTaiXuanJingSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsTaiXuanJingSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlDocDump(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    FILE * f;
    PyObject *pyobj_f;
    xmlDocPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:htmlDocDump", &pyobj_f, &pyobj_cur))
        return(NULL);
    f = (FILE *) PyFile_Get(pyobj_f);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = htmlDocDump(f, cur);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlTextReaderRelaxNGValidate(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    char * rng;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlTextReaderRelaxNGValidate", &pyobj_reader, &rng))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderRelaxNGValidate(reader, rng);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) && defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlFreeNodeList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeNodeList", &pyobj_cur))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlFreeNodeList(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathDivValues(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathDivValues", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathDivValues(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathPositionFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathPositionFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathPositionFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsTelugu(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsTelugu", &code))
        return(NULL);

    c_retval = xmlUCSIsTelugu(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlLsCountNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlLsCountNode", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlLsCountNode(node);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlParseCatalogFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlParseCatalogFile", &filename))
        return(NULL);

    c_retval = xmlParseCatalogFile(filename);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathGetFunctionURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathGetFunctionURI", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = ctxt->functionURI;
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatMn(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatMn", &code))
        return(NULL);

    c_retval = xmlUCSIsCatMn(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGCleanupTypes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlRelaxNGCleanupTypes();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatMe(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatMe", &code))
        return(NULL);

    c_retval = xmlUCSIsCatMe(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlGetEncodingAlias(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    char * alias;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlGetEncodingAlias", &alias))
        return(NULL);

    c_retval = xmlGetEncodingAlias(alias);
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlACatalogAdd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;
    xmlChar * type;
    xmlChar * orig;
    xmlChar * replace;

    if (!PyArg_ParseTuple(args, (char *)"Ozzz:xmlACatalogAdd", &pyobj_catal, &type, &orig, &replace))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    c_retval = xmlACatalogAdd(catal, type, orig, replace);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlNewNsPropEatName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlNewNsPropEatName", &pyobj_node, &pyobj_ns, &name, &value))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlNewNsPropEatName(node, ns, name, value);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlStrdup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * cur;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlStrdup", &cur))
        return(NULL);

    c_retval = xmlStrdup(cur);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNamespaceURIFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathNamespaceURIFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathNamespaceURIFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlCtxtReadDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * cur;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ozzzi:xmlCtxtReadDoc", &pyobj_ctxt, &cur, &URL, &encoding, &options))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlCtxtReadDoc(ctxt, cur, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderQuoteChar(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderQuoteChar", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderQuoteChar(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlInitCharEncodingHandlers(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlInitCharEncodingHandlers();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_REGEXP_ENABLED)
PyObject *
libxml_xmlRegexpCompile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlRegexpPtr c_retval;
    xmlChar * regexp;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlRegexpCompile", &regexp))
        return(NULL);

    c_retval = xmlRegexpCompile(regexp);
    py_retval = libxml_xmlRegexpPtrWrap((xmlRegexpPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_REGEXP_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathRegisteredNsCleanup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathRegisteredNsCleanup", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    xmlXPathRegisteredNsCleanup(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsKannada(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsKannada", &code))
        return(NULL);

    c_retval = xmlUCSIsKannada(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstValue", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstValue(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_DOCB_ENABLED)
PyObject *
libxml_docbDefaultSAXHandlerInit(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    docbDefaultSAXHandlerInit();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DOCB_ENABLED) */
#if defined(LIBXML_VALID_ENABLED) && defined(LIBXML_REGEXP_ENABLED)
PyObject *
libxml_xmlValidatePushCData(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * data;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlValidatePushCData", &pyobj_ctxt, &data, &len))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);

    c_retval = xmlValidatePushCData(ctxt, data, len);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) && defined(LIBXML_REGEXP_ENABLED) */
PyObject *
libxml_xmlErrorGetDomain(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlErrorPtr Error;
    PyObject *pyobj_Error;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlErrorGetDomain", &pyobj_Error))
        return(NULL);
    Error = (xmlErrorPtr) PyError_Get(pyobj_Error);

    c_retval = Error->domain;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCheckFilename(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * path;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCheckFilename", &path))
        return(NULL);

    c_retval = xmlCheckFilename(path);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathFloorFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathFloorFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathFloorFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsTibetan(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsTibetan", &code))
        return(NULL);

    c_retval = xmlUCSIsTibetan(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlNewGlobalNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * href;
    xmlChar * prefix;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlNewGlobalNs", &pyobj_doc, &href, &prefix))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewGlobalNs(doc, href, prefix);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathStringLengthFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathStringLengthFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathStringLengthFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlDocDump(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    FILE * f;
    PyObject *pyobj_f;
    xmlDocPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlDocDump", &pyobj_f, &pyobj_cur))
        return(NULL);
    f = (FILE *) PyFile_Get(pyobj_f);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlDocDump(f, cur);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextSelf(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextSelf", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextSelf(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCyrillicSupplement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCyrillicSupplement", &code))
        return(NULL);

    c_retval = xmlUCSIsCyrillicSupplement(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlURIPtr c_retval;
    char * str;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlParseURI", &str))
        return(NULL);

    c_retval = xmlParseURI(str);
    py_retval = libxml_xmlURIPtrWrap((xmlURIPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCopyProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr target;
    PyObject *pyobj_target;
    xmlAttrPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlCopyProp", &pyobj_target, &pyobj_cur))
        return(NULL);
    target = (xmlNodePtr) PyxmlNode_Get(pyobj_target);
    cur = (xmlAttrPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlCopyProp(target, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlURIGetPort(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetPort", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->port;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlSaveFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;
    xmlDocPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"zO:htmlSaveFile", &filename, &pyobj_cur))
        return(NULL);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = htmlSaveFile(filename, cur);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderCurrentDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderCurrentDoc", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderCurrentDoc(reader);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlParsePITarget(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParsePITarget", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParsePITarget(ctxt);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlURISetOpaque(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    char * opaque;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlURISetOpaque", &pyobj_URI, &opaque))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    if (URI->opaque != NULL) xmlFree(URI->opaque);
    URI->opaque = (char *)xmlStrdup((const xmlChar *)opaque);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlNewNodeEatName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNewNodeEatName", &pyobj_ns, &name))
        return(NULL);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlNewNodeEatName(ns, name);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlIsCombining(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned int ch;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsCombining", &ch))
        return(NULL);

    c_retval = xmlIsCombining(ch);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlReadFd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    int fd;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"izzi:htmlReadFd", &fd, &URL, &encoding, &options))
        return(NULL);

    c_retval = htmlReadFd(fd, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderNormalization(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderNormalization", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderNormalization(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathEvalExpression(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlChar * str;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"zO:xmlXPathEvalExpression", &str, &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = xmlXPathEvalExpression(str, ctxt);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlStrncatNew(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * str1;
    xmlChar * str2;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zzi:xmlStrncatNew", &str1, &str2, &len))
        return(NULL);

    c_retval = xmlStrncatNew(str1, str2, len);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogResolvePublic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * pubID;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCatalogResolvePublic", &pubID))
        return(NULL);

    c_retval = xmlCatalogResolvePublic(pubID);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlNewCDataBlock(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * content;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlNewCDataBlock", &pyobj_doc, &content, &len))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewCDataBlock(doc, content, len);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlURIGetServer(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetServer", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->server;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlSaveFileFormat(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    char * encoding;
    int format;

    if (!PyArg_ParseTuple(args, (char *)"zOzi:htmlSaveFileFormat", &filename, &pyobj_cur, &encoding, &format))
        return(NULL);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = htmlSaveFileFormat(filename, cur, encoding, format);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
PyObject *
libxml_xmlNodeIsText(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlNodeIsText", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlNodeIsText(node);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParserSetReplaceEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int replaceEntities;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlParserSetReplaceEntities", &pyobj_ctxt, &replaceEntities))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    ctxt->replaceEntities = replaceEntities;
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathStringEvalNumber(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    double c_retval;
    xmlChar * str;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlXPathStringEvalNumber", &str))
        return(NULL);

    c_retval = xmlXPathStringEvalNumber(str);
    py_retval = libxml_doubleWrap((double) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlUTF8Strsize(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * utf;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlUTF8Strsize", &utf, &len))
        return(NULL);

    c_retval = xmlUTF8Strsize(utf, len);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderStandalone(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderStandalone", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderStandalone(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlParseStartTag(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseStartTag", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseStartTag(ctxt);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlSetupParserForBuffer(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * buffer;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlSetupParserForBuffer", &pyobj_ctxt, &buffer, &filename))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlSetupParserForBuffer(ctxt, buffer, filename);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlNewTextReaderFilename(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlTextReaderPtr c_retval;
    char * URI;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlNewTextReaderFilename", &URI))
        return(NULL);

    c_retval = xmlNewTextReaderFilename(URI);
    py_retval = libxml_xmlTextReaderPtrWrap((xmlTextReaderPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNumberFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathNumberFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathNumberFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlLsOneNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlLsOneNode", &pyobj_output, &pyobj_node))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    xmlLsOneNode(output, node);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGreekExtended(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGreekExtended", &code))
        return(NULL);

    c_retval = xmlUCSIsGreekExtended(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlNewDocNodeEatName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlNewDocNodeEatName", &pyobj_doc, &pyobj_ns, &name, &content))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlNewDocNodeEatName(doc, ns, name, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderForDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlTextReaderPtr c_retval;
    xmlChar * cur;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"zzzi:xmlReaderForDoc", &cur, &URL, &encoding, &options))
        return(NULL);

    c_retval = xmlReaderForDoc(cur, URL, encoding, options);
    py_retval = libxml_xmlTextReaderPtrWrap((xmlTextReaderPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlAddDocEntity(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlEntityPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;
    int type;
    xmlChar * ExternalID;
    xmlChar * SystemID;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"Ozizzz:xmlAddDocEntity", &pyobj_doc, &name, &type, &ExternalID, &SystemID, &content))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlAddDocEntity(doc, name, type, ExternalID, SystemID, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMyanmar(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMyanmar", &code))
        return(NULL);

    c_retval = xmlUCSIsMyanmar(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathIsNodeType(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlXPathIsNodeType", &name))
        return(NULL);

    c_retval = xmlXPathIsNodeType(name);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathRoot(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathRoot", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathRoot(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathVariableLookup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlXPathVariableLookup", &pyobj_ctxt, &name))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = xmlXPathVariableLookup(ctxt, name);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextFollowing(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextFollowing", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextFollowing(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHangulCompatibilityJamo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHangulCompatibilityJamo", &code))
        return(NULL);

    c_retval = xmlUCSIsHangulCompatibilityJamo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlNewTextChild(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr parent;
    PyObject *pyobj_parent;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlNewTextChild", &pyobj_parent, &pyobj_ns, &name, &content))
        return(NULL);
    parent = (xmlNodePtr) PyxmlNode_Get(pyobj_parent);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlNewTextChild(parent, ns, name, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
PyObject *
libxml_xmlAddChild(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr parent;
    PyObject *pyobj_parent;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlAddChild", &pyobj_parent, &pyobj_cur))
        return(NULL);
    parent = (xmlNodePtr) PyxmlNode_Get(pyobj_parent);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlAddChild(parent, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathErr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int error;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathErr", &pyobj_ctxt, &error))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathErr(ctxt, error);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderDepth(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderDepth", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderDepth(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHiragana(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHiragana", &code))
        return(NULL);

    c_retval = xmlUCSIsHiragana(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlRelaxNGDump(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlRelaxNGPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlRelaxNGDump", &pyobj_output, &pyobj_schema))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    schema = (xmlRelaxNGPtr) PyrelaxNgSchema_Get(pyobj_schema);

    xmlRelaxNGDump(output, schema);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
PyObject *
libxml_xmlFreeURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr uri;
    PyObject *pyobj_uri;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeURI", &pyobj_uri))
        return(NULL);
    uri = (xmlURIPtr) PyURI_Get(pyobj_uri);

    xmlFreeURI(uri);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextParent(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextParent", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextParent(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsDevanagari(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsDevanagari", &code))
        return(NULL);

    c_retval = xmlUCSIsDevanagari(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlNodeGetContent(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlNodeGetContent", &pyobj_cur))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlNodeGetContent(cur);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderIsEmptyElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderIsEmptyElement", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderIsEmptyElement(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsIPAExtensions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsIPAExtensions", &code))
        return(NULL);

    c_retval = xmlUCSIsIPAExtensions(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPTR_ENABLED)
PyObject *
libxml_xmlXPtrNewContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathContextPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr here;
    PyObject *pyobj_here;
    xmlNodePtr origin;
    PyObject *pyobj_origin;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlXPtrNewContext", &pyobj_doc, &pyobj_here, &pyobj_origin))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    here = (xmlNodePtr) PyxmlNode_Get(pyobj_here);
    origin = (xmlNodePtr) PyxmlNode_Get(pyobj_origin);

    c_retval = xmlXPtrNewContext(doc, here, origin);
    py_retval = libxml_xmlXPathContextPtrWrap((xmlXPathContextPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPTR_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsYiSyllables(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsYiSyllables", &code))
        return(NULL);

    c_retval = xmlUCSIsYiSyllables(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderLookupNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlChar * prefix;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlTextReaderLookupNamespace", &pyobj_reader, &prefix))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderLookupNamespace(reader, prefix);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlNodeGetLang(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlNodeGetLang", &pyobj_cur))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlNodeGetLang(cur);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {
    PyObject *py_retval;
    xmlParserCtxtPtr c_retval;

    c_retval = xmlNewParserCtxt();
    py_retval = libxml_xmlParserCtxtPtrWrap((xmlParserCtxtPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_FTP_ENABLED)
PyObject *
libxml_xmlNanoFTPScanProxy(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    char * URL;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlNanoFTPScanProxy", &URL))
        return(NULL);

    xmlNanoFTPScanProxy(URL);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_FTP_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaFree(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlSchemaPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSchemaFree", &pyobj_schema))
        return(NULL);
    schema = (xmlSchemaPtr) PySchema_Get(pyobj_schema);

    xmlSchemaFree(schema);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderNextSibling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderNextSibling", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderNextSibling(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlClearParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlClearParserCtxt", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlClearParserCtxt(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_DEBUG_ENABLED) || defined (LIBXML_HTML_ENABLED)
PyObject *
libxml_xmlValidateNCName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;
    int space;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlValidateNCName", &value, &space))
        return(NULL);

    c_retval = xmlValidateNCName(value, space);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XPATH_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_DEBUG_ENABLED) || defined (LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlStrlen(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * str;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlStrlen", &str))
        return(NULL);

    c_retval = xmlStrlen(str);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpDocument(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlDebugDumpDocument", &pyobj_output, &pyobj_doc))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    xmlDebugDumpDocument(output, doc);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_XPTR_ENABLED)
PyObject *
libxml_xmlXPtrEval(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlChar * str;
    xmlXPathContextPtr ctx;
    PyObject *pyobj_ctx;

    if (!PyArg_ParseTuple(args, (char *)"zO:xmlXPtrEval", &str, &pyobj_ctx))
        return(NULL);
    ctx = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctx);

    c_retval = xmlXPtrEval(str, ctx);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPTR_ENABLED) */
PyObject *
libxml_xmlPopInput(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlPopInput", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlPopInput(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathBooleanFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathBooleanFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathBooleanFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderSetParserProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    int prop;
    int value;

    if (!PyArg_ParseTuple(args, (char *)"Oii:xmlTextReaderSetParserProp", &pyobj_reader, &prop, &value))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderSetParserProp(reader, prop, value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderGetRemainder(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlParserInputBufferPtr c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderGetRemainder", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderGetRemainder(reader);
    py_retval = libxml_xmlParserInputBufferPtrWrap((xmlParserInputBufferPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGujarati(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGujarati", &code))
        return(NULL);

    c_retval = xmlUCSIsGujarati(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlSetMetaEncoding(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    htmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * encoding;

    if (!PyArg_ParseTuple(args, (char *)"Oz:htmlSetMetaEncoding", &pyobj_doc, &encoding))
        return(NULL);
    doc = (htmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = htmlSetMetaEncoding(doc, encoding);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderNewDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlChar * cur;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ozzzi:xmlReaderNewDoc", &pyobj_reader, &cur, &URL, &encoding, &options))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlReaderNewDoc(reader, cur, URL, encoding, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstPrefix(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstPrefix", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstPrefix(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlRecoverDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlChar * cur;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlRecoverDoc", &cur))
        return(NULL);

    c_retval = xmlRecoverDoc(cur);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
PyObject *
libxml_xmlNormalizeWindowsPath(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * path;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlNormalizeWindowsPath", &path))
        return(NULL);

    c_retval = xmlNormalizeWindowsPath(path);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XINCLUDE_ENABLED)
PyObject *
libxml_xmlXIncludeProcessTree(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr tree;
    PyObject *pyobj_tree;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXIncludeProcessTree", &pyobj_tree))
        return(NULL);
    tree = (xmlNodePtr) PyxmlNode_Get(pyobj_tree);

    c_retval = xmlXIncludeProcessTree(tree);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XINCLUDE_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlCatalogDump(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * out;
    PyObject *pyobj_out;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlCatalogDump", &pyobj_out))
        return(NULL);
    out = (FILE *) PyFile_Get(pyobj_out);

    xmlCatalogDump(out);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextDescendantOrSelf(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextDescendantOrSelf", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextDescendantOrSelf(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlParseNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseNamespace", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseNamespace(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
PyObject *
libxml_xmlStrcasecmp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * str1;
    xmlChar * str2;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlStrcasecmp", &str1, &str2))
        return(NULL);

    c_retval = xmlStrcasecmp(str1, str2);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderForMemory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlTextReaderPtr c_retval;
    char * buffer;
    int size;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"zizzi:xmlReaderForMemory", &buffer, &size, &URL, &encoding, &options))
        return(NULL);

    c_retval = xmlReaderForMemory(buffer, size, URL, encoding, options);
    py_retval = libxml_xmlTextReaderPtrWrap((xmlTextReaderPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderByteConsumed(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    long c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderByteConsumed", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderByteConsumed(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlNewDtd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDtdPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;
    xmlChar * ExternalID;
    xmlChar * SystemID;

    if (!PyArg_ParseTuple(args, (char *)"Ozzz:xmlNewDtd", &pyobj_doc, &name, &ExternalID, &SystemID))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewDtd(doc, name, ExternalID, SystemID);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBlockElements(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsBlockElements", &code))
        return(NULL);

    c_retval = xmlUCSIsBlockElements(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlNodeGetBase(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlNodeGetBase", &pyobj_doc, &pyobj_cur))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlNodeGetBase(doc, cur);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextAncestorOrSelf(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextAncestorOrSelf", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextAncestorOrSelf(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNewString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlChar * val;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlXPathNewString", &val))
        return(NULL);

    c_retval = xmlXPathNewString(val);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlAddSibling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlAddSibling", &pyobj_cur, &pyobj_elem))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlAddSibling(cur, elem);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlScanName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlScanName", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlScanName(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
PyObject *
libxml_xmlRegisterDefaultInputCallbacks(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlRegisterDefaultInputCallbacks();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlDebugDumpEntities", &pyobj_output, &pyobj_doc))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    xmlDebugDumpEntities(output, doc);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextAncestor(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextAncestor", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextAncestor(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCastNumberToBoolean(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    double val;

    if (!PyArg_ParseTuple(args, (char *)"d:xmlXPathCastNumberToBoolean", &val))
        return(NULL);

    c_retval = xmlXPathCastNumberToBoolean(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatCs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatCs", &code))
        return(NULL);

    c_retval = xmlUCSIsCatCs(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatCf(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatCf", &code))
        return(NULL);

    c_retval = xmlUCSIsCatCf(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatCo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatCo", &code))
        return(NULL);

    c_retval = xmlUCSIsCatCo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlRecoverMemory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    char * buffer;
    int py_buffsize0;
    int size;

    if (!PyArg_ParseTuple(args, (char *)"t#i:xmlRecoverMemory", &buffer, &py_buffsize0, &size))
        return(NULL);

    c_retval = xmlRecoverMemory(buffer, size);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderIsDefault(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderIsDefault", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderIsDefault(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlParserGetWellFormed(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParserGetWellFormed", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->wellFormed;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlRemoveRef(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlAttrPtr attr;
    PyObject *pyobj_attr;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlRemoveRef", &pyobj_doc, &pyobj_attr))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    attr = (xmlAttrPtr) PyxmlNode_Get(pyobj_attr);

    c_retval = xmlRemoveRef(doc, attr);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderNewMemory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    char * buffer;
    int size;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ozizzi:xmlReaderNewMemory", &pyobj_reader, &buffer, &size, &URL, &encoding, &options))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlReaderNewMemory(reader, buffer, size, URL, encoding, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlNewProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlNewProp", &pyobj_node, &name, &value))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlNewProp(node, name, value);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_HTML_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlParserGetDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParserGetDoc", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->myDoc;
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCleanupCharEncodingHandlers(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlCleanupCharEncodingHandlers();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGValidatePopElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlRelaxNGValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlRelaxNGValidatePopElement", &pyobj_ctxt, &pyobj_doc, &pyobj_elem))
        return(NULL);
    ctxt = (xmlRelaxNGValidCtxtPtr) PyrelaxNgValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlRelaxNGValidatePopElement(ctxt, doc, elem);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlParseEntityRef(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlEntityPtr c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseEntityRef", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseEntityRef(ctxt);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlInitAutoClose(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    htmlInitAutoClose();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderReadOuterXml(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderReadOuterXml", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderReadOuterXml(reader);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsTamil(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsTamil", &code))
        return(NULL);

    c_retval = xmlUCSIsTamil(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlChar * str;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlDebugDumpString", &pyobj_output, &str))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);

    xmlDebugDumpString(output, str);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
PyObject *
libxml_xmlCleanupGlobals(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlCleanupGlobals();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlEncodeEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * input;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlEncodeEntities", &pyobj_doc, &input))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlEncodeEntities(doc, input);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlNewCatalog(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlCatalogPtr c_retval;
    int sgml;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlNewCatalog", &sgml))
        return(NULL);

    c_retval = xmlNewCatalog(sgml);
    py_retval = libxml_xmlCatalogPtrWrap((xmlCatalogPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlStrncasecmp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * str1;
    xmlChar * str2;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zzi:xmlStrncasecmp", &str1, &str2, &len))
        return(NULL);

    c_retval = xmlStrncasecmp(str1, str2, len);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCanonicPath(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * path;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCanonicPath", &path))
        return(NULL);

    c_retval = xmlCanonicPath(path);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextPrecedingSibling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextPrecedingSibling", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextPrecedingSibling(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogCleanup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlCatalogCleanup();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlNextChar(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlNextChar", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlNextChar(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlIsID(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;
    xmlAttrPtr attr;
    PyObject *pyobj_attr;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlIsID", &pyobj_doc, &pyobj_elem, &pyobj_attr))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);
    attr = (xmlAttrPtr) PyxmlNode_Get(pyobj_attr);

    c_retval = xmlIsID(doc, elem, attr);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseExtParsedEnt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseExtParsedEnt", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseExtParsedEnt(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_FTP_ENABLED)
PyObject *
libxml_xmlNanoFTPProxy(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    char * host;
    int port;
    char * user;
    char * passwd;
    int type;

    if (!PyArg_ParseTuple(args, (char *)"zizzi:xmlNanoFTPProxy", &host, &port, &user, &passwd, &type))
        return(NULL);

    xmlNanoFTPProxy(host, port, user, passwd, type);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_FTP_ENABLED) */
PyObject *
libxml_xmlStringLenDecodeEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * str;
    int len;
    int what;
    xmlChar end;
    xmlChar end2;
    xmlChar end3;

    if (!PyArg_ParseTuple(args, (char *)"Oziiccc:xmlStringLenDecodeEntities", &pyobj_ctxt, &str, &len, &what, &end, &end2, &end3))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlStringLenDecodeEntities(ctxt, str, len, what, end, end2, end3);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKUnifiedIdeographsExtensionA(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKUnifiedIdeographsExtensionA", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKUnifiedIdeographsExtensionA(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKUnifiedIdeographsExtensionB(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKUnifiedIdeographsExtensionB", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKUnifiedIdeographsExtensionB(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlCreateMemoryParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlParserCtxtPtr c_retval;
    char * buffer;
    int py_buffsize0;
    int size;

    if (!PyArg_ParseTuple(args, (char *)"t#i:htmlCreateMemoryParserCtxt", &buffer, &py_buffsize0, &size))
        return(NULL);

    c_retval = htmlCreateMemoryParserCtxt(buffer, size);
    py_retval = libxml_xmlParserCtxtPtrWrap((xmlParserCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlIsDigit(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned int ch;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsDigit", &ch))
        return(NULL);

    c_retval = xmlIsDigit(ch);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogSetDebug(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int level;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlCatalogSetDebug", &level))
        return(NULL);

    c_retval = xmlCatalogSetDebug(level);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlParserGetDirectory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    char * c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlParserGetDirectory", &filename))
        return(NULL);

    c_retval = xmlParserGetDirectory(filename);
    py_retval = libxml_charPtrWrap((char *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaCleanupTypes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlSchemaCleanupTypes();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlFreeNsList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNsPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeNsList", &pyobj_cur))
        return(NULL);
    cur = (xmlNsPtr) PyxmlNode_Get(pyobj_cur);

    xmlFreeNsList(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlParseEntityDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseEntityDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseEntityDecl(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlDocCopyNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    int extended;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlDocCopyNode", &pyobj_node, &pyobj_doc, &extended))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlDocCopyNode(node, doc, extended);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_nodePop(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:nodePop", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = nodePop(ctxt);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextDescendant(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextDescendant", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextDescendant(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNewNodeSet(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlNodePtr val;
    PyObject *pyobj_val;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathNewNodeSet", &pyobj_val))
        return(NULL);
    val = (xmlNodePtr) PyxmlNode_Get(pyobj_val);

    c_retval = xmlXPathNewNodeSet(val);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaNewParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlSchemaParserCtxtPtr c_retval;
    char * URL;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlSchemaNewParserCtxt", &URL))
        return(NULL);

    c_retval = xmlSchemaNewParserCtxt(URL);
    py_retval = libxml_xmlSchemaParserCtxtPtrWrap((xmlSchemaParserCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlInitializeCatalog(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlInitializeCatalog();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlParseEntity(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlParseEntity", &filename))
        return(NULL);

    c_retval = xmlParseEntity(filename);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
PyObject *
libxml_xmlDocGetRootElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlDocGetRootElement", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlDocGetRootElement(doc);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathPopString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathPopString", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = xmlXPathPopString(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlCreateFileParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlParserCtxtPtr c_retval;
    char * filename;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"zz:htmlCreateFileParserCtxt", &filename, &encoding))
        return(NULL);

    c_retval = htmlCreateFileParserCtxt(filename, encoding);
    py_retval = libxml_xmlParserCtxtPtrWrap((xmlParserCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstEncoding(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstEncoding", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstEncoding(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateOneAttribute(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;
    xmlAttrPtr attr;
    PyObject *pyobj_attr;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"OOOOz:xmlValidateOneAttribute", &pyobj_ctxt, &pyobj_doc, &pyobj_elem, &pyobj_attr, &value))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);
    attr = (xmlAttrPtr) PyxmlNode_Get(pyobj_attr);

    c_retval = xmlValidateOneAttribute(ctxt, doc, elem, attr, value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlAddEncodingAlias(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * name;
    char * alias;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlAddEncodingAlias", &name, &alias))
        return(NULL);

    c_retval = xmlAddEncodingAlias(name, alias);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderMoveToAttribute(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlTextReaderMoveToAttribute", &pyobj_reader, &name))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderMoveToAttribute(reader, name);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKCompatibilityForms(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKCompatibilityForms", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKCompatibilityForms(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED) && defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlTextReaderSetSchema(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlSchemaPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlTextReaderSetSchema", &pyobj_reader, &pyobj_schema))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);
    schema = (xmlSchemaPtr) PySchema_Get(pyobj_schema);

    c_retval = xmlTextReaderSetSchema(reader, schema);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) && defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlCharStrdup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    char * cur;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCharStrdup", &cur))
        return(NULL);

    c_retval = xmlCharStrdup(cur);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlElemDump(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * f;
    PyObject *pyobj_f;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlElemDump", &pyobj_f, &pyobj_doc, &pyobj_cur))
        return(NULL);
    f = (FILE *) PyFile_Get(pyobj_f);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlElemDump(f, doc, cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathConcatFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathConcatFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathConcatFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpAttrList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlAttrPtr attr;
    PyObject *pyobj_attr;
    int depth;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlDebugDumpAttrList", &pyobj_output, &pyobj_attr, &depth))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    attr = (xmlAttrPtr) PyxmlNode_Get(pyobj_attr);

    xmlDebugDumpAttrList(output, attr, depth);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderReadString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderReadString", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderReadString(reader);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLinearBIdeograms(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLinearBIdeograms", &code))
        return(NULL);

    c_retval = xmlUCSIsLinearBIdeograms(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseCharData(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int cdata;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlParseCharData", &pyobj_ctxt, &cdata))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseCharData(ctxt, cdata);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsThai(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsThai", &code))
        return(NULL);

    c_retval = xmlUCSIsThai(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlCtxtReset(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:htmlCtxtReset", &pyobj_ctxt))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    htmlCtxtReset(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlCtxtReadFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    char * filename;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ozzi:xmlCtxtReadFile", &pyobj_ctxt, &filename, &encoding, &options))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlCtxtReadFile(ctxt, filename, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogResolveSystem(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * sysID;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCatalogResolveSystem", &sysID))
        return(NULL);

    c_retval = xmlCatalogResolveSystem(sysID);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstLocalName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstLocalName", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstLocalName(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathLastFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathLastFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathLastFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsOpticalCharacterRecognition(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsOpticalCharacterRecognition", &code))
        return(NULL);

    c_retval = xmlUCSIsOpticalCharacterRecognition(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlThrDefSubstituteEntitiesDefaultValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefSubstituteEntitiesDefaultValue", &v))
        return(NULL);

    c_retval = xmlThrDefSubstituteEntitiesDefaultValue(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlNewNsProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlNewNsProp", &pyobj_node, &pyobj_ns, &name, &value))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlNewNsProp(node, ns, name, value);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlThrDefIndentTreeOutput(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefIndentTreeOutput", &v))
        return(NULL);

    c_retval = xmlThrDefIndentTreeOutput(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsYijingHexagramSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsYijingHexagramSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsYijingHexagramSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderNewFd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    int fd;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Oizzi:xmlReaderNewFd", &pyobj_reader, &fd, &URL, &encoding, &options))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlReaderNewFd(reader, fd, URL, encoding, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlCreateMemoryParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlParserCtxtPtr c_retval;
    char * buffer;
    int py_buffsize0;
    int size;

    if (!PyArg_ParseTuple(args, (char *)"t#i:xmlCreateMemoryParserCtxt", &buffer, &py_buffsize0, &size))
        return(NULL);

    c_retval = xmlCreateMemoryParserCtxt(buffer, size);
    py_retval = libxml_xmlParserCtxtPtrWrap((xmlParserCtxtPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseName", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseName(ctxt);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCopyNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    int extended;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlCopyNode", &pyobj_node, &extended))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlCopyNode(node, extended);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCastStringToBoolean(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * val;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlXPathCastStringToBoolean", &val))
        return(NULL);

    c_retval = xmlXPathCastStringToBoolean(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderMoveToElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderMoveToElement", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderMoveToElement(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlIsAutoClosed(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    htmlDocPtr doc;
    PyObject *pyobj_doc;
    htmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OO:htmlIsAutoClosed", &pyobj_doc, &pyobj_elem))
        return(NULL);
    doc = (htmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (htmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = htmlIsAutoClosed(doc, elem);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsUgaritic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsUgaritic", &code))
        return(NULL);

    c_retval = xmlUCSIsUgaritic(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKCompatibilityIdeographsSupplement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKCompatibilityIdeographsSupplement", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKCompatibilityIdeographsSupplement(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlReconciliateNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr tree;
    PyObject *pyobj_tree;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlReconciliateNs", &pyobj_doc, &pyobj_tree))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    tree = (xmlNodePtr) PyxmlNode_Get(pyobj_tree);

    c_retval = xmlReconciliateNs(doc, tree);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlNewChild(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr parent;
    PyObject *pyobj_parent;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlNewChild", &pyobj_parent, &pyobj_ns, &name, &content))
        return(NULL);
    parent = (xmlNodePtr) PyxmlNode_Get(pyobj_parent);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlNewChild(parent, ns, name, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsKangxiRadicals(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsKangxiRadicals", &code))
        return(NULL);

    c_retval = xmlUCSIsKangxiRadicals(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlCreateIntSubset(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDtdPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;
    xmlChar * ExternalID;
    xmlChar * SystemID;

    if (!PyArg_ParseTuple(args, (char *)"Ozzz:xmlCreateIntSubset", &pyobj_doc, &name, &ExternalID, &SystemID))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlCreateIntSubset(doc, name, ExternalID, SystemID);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathSubValues(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathSubValues", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathSubValues(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsArabicPresentationFormsA(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsArabicPresentationFormsA", &code))
        return(NULL);

    c_retval = xmlUCSIsArabicPresentationFormsA(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsArabicPresentationFormsB(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsArabicPresentationFormsB", &code))
        return(NULL);

    c_retval = xmlUCSIsArabicPresentationFormsB(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGeometricShapes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGeometricShapes", &code))
        return(NULL);

    c_retval = xmlUCSIsGeometricShapes(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlGetPredefinedEntity(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlEntityPtr c_retval;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlGetPredefinedEntity", &name))
        return(NULL);

    c_retval = xmlGetPredefinedEntity(name);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlSaveFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;
    xmlDocPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"zO:xmlSaveFile", &filename, &pyobj_cur))
        return(NULL);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlSaveFile(filename, cur);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextNamespace", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextNamespace(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBuhid(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsBuhid", &code))
        return(NULL);

    c_retval = xmlUCSIsBuhid(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaValidateOneElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlSchemaValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlSchemaValidateOneElement", &pyobj_ctxt, &pyobj_elem))
        return(NULL);
    ctxt = (xmlSchemaValidCtxtPtr) PySchemaValidCtxt_Get(pyobj_ctxt);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlSchemaValidateOneElement(ctxt, elem);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlReadDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlChar * cur;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"zzzi:xmlReadDoc", &cur, &URL, &encoding, &options))
        return(NULL);

    c_retval = xmlReadDoc(cur, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderNewFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    char * filename;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ozzi:xmlReaderNewFile", &pyobj_reader, &filename, &encoding, &options))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlReaderNewFile(reader, filename, encoding, options);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlFreeDtd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlDtdPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeDtd", &pyobj_cur))
        return(NULL);
    cur = (xmlDtdPtr) PyxmlNode_Get(pyobj_cur);

    xmlFreeDtd(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlSetListDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr list;
    PyObject *pyobj_list;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlSetListDoc", &pyobj_list, &pyobj_doc))
        return(NULL);
    list = (xmlNodePtr) PyxmlNode_Get(pyobj_list);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    xmlSetListDoc(list, doc);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlCtxtReadFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    char * filename;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ozzi:htmlCtxtReadFile", &pyobj_ctxt, &filename, &encoding, &options))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = htmlCtxtReadFile(ctxt, filename, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlThrDefLineNumbersDefaultValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefLineNumbersDefaultValue", &v))
        return(NULL);

    c_retval = xmlThrDefLineNumbersDefaultValue(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCombiningHalfMarks(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCombiningHalfMarks", &code))
        return(NULL);

    c_retval = xmlUCSIsCombiningHalfMarks(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatSc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatSc", &code))
        return(NULL);

    c_retval = xmlUCSIsCatSc(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatSo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatSo", &code))
        return(NULL);

    c_retval = xmlUCSIsCatSo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatSk(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatSk", &code))
        return(NULL);

    c_retval = xmlUCSIsCatSk(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathFreeContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathFreeContext", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    xmlXPathFreeContext(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlEncodeSpecialChars(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * input;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlEncodeSpecialChars", &pyobj_doc, &input))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlEncodeSpecialChars(doc, input);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_namePop(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:namePop", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = namePop(ctxt);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseContent(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseContent", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseContent(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlReadMemory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    char * buffer;
    int py_buffsize0;
    int size;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"t#izzi:xmlReadMemory", &buffer, &py_buffsize0, &size, &URL, &encoding, &options))
        return(NULL);

    c_retval = xmlReadMemory(buffer, size, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlThrDefGetWarningsDefaultValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefGetWarningsDefaultValue", &v))
        return(NULL);

    c_retval = xmlThrDefGetWarningsDefaultValue(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMongolian(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMongolian", &code))
        return(NULL);

    c_retval = xmlUCSIsMongolian(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlURIGetFragment(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetFragment", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->fragment;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKRadicalsSupplement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKRadicalsSupplement", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKRadicalsSupplement(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathSumFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathSumFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathSumFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlCopyNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xmlNsPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlCopyNamespace", &pyobj_cur))
        return(NULL);
    cur = (xmlNsPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlCopyNamespace(cur);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCyrillic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCyrillic", &code))
        return(NULL);

    c_retval = xmlUCSIsCyrillic(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlURISetFragment(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    char * fragment;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlURISetFragment", &pyobj_URI, &fragment))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    if (URI->fragment != NULL) xmlFree(URI->fragment);
    URI->fragment = (char *)xmlStrdup((const xmlChar *)fragment);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlAddChildList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr parent;
    PyObject *pyobj_parent;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlAddChildList", &pyobj_parent, &pyobj_cur))
        return(NULL);
    parent = (xmlNodePtr) PyxmlNode_Get(pyobj_parent);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlAddChildList(parent, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_PUSH_ENABLED)
PyObject *
libxml_htmlParseChunk(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    char * chunk;
    int py_buffsize0;
    int size;
    int terminate;

    if (!PyArg_ParseTuple(args, (char *)"Ot#ii:htmlParseChunk", &pyobj_ctxt, &chunk, &py_buffsize0, &size, &terminate))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = htmlParseChunk(ctxt, chunk, size, terminate);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_PUSH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathIdFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathIdFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathIdFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlCreateURLParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlParserCtxtPtr c_retval;
    char * filename;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlCreateURLParserCtxt", &filename, &options))
        return(NULL);

    c_retval = xmlCreateURLParserCtxt(filename, options);
    py_retval = libxml_xmlParserCtxtPtrWrap((xmlParserCtxtPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderRead(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderRead", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderRead(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlSaveUri(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlURIPtr uri;
    PyObject *pyobj_uri;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSaveUri", &pyobj_uri))
        return(NULL);
    uri = (xmlURIPtr) PyURI_Get(pyobj_uri);

    c_retval = xmlSaveUri(uri);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlIsChar(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned int ch;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsChar", &ch))
        return(NULL);

    c_retval = xmlIsChar(ch);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlCtxtReadFd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int fd;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Oizzi:htmlCtxtReadFd", &pyobj_ctxt, &fd, &URL, &encoding, &options))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = htmlCtxtReadFd(ctxt, fd, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlPedanticParserDefault(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlPedanticParserDefault", &val))
        return(NULL);

    c_retval = xmlPedanticParserDefault(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlParseDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlChar * cur;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlParseDoc", &cur))
        return(NULL);

    c_retval = xmlParseDoc(cur);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathParseNCName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathParseNCName", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = xmlXPathParseNCName(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlLineNumbersDefault(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlLineNumbersDefault", &val))
        return(NULL);

    c_retval = xmlLineNumbersDefault(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlConvertSGMLCatalog(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlConvertSGMLCatalog", &pyobj_catal))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    c_retval = xmlConvertSGMLCatalog(catal);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlNodeAddContent(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNodeAddContent", &pyobj_cur, &content))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeAddContent(cur, content);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlErrorGetLevel(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlErrorPtr Error;
    PyObject *pyobj_Error;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlErrorGetLevel", &pyobj_Error))
        return(NULL);
    Error = (xmlErrorPtr) PyError_Get(pyobj_Error);

    c_retval = Error->level;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNewParserContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathParserContextPtr c_retval;
    xmlChar * str;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"zO:xmlXPathNewParserContext", &str, &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = xmlXPathNewParserContext(str, ctxt);
    py_retval = libxml_xmlXPathParserContextPtrWrap((xmlXPathParserContextPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlParseDocument(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseDocument", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseDocument(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlFreeNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeNode", &pyobj_cur))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlFreeNode(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGValidatePushElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlRelaxNGValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlRelaxNGValidatePushElement", &pyobj_ctxt, &pyobj_doc, &pyobj_elem))
        return(NULL);
    ctxt = (xmlRelaxNGValidCtxtPtr) PyrelaxNgValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlRelaxNGValidatePushElement(ctxt, doc, elem);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSinhala(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSinhala", &code))
        return(NULL);

    c_retval = xmlUCSIsSinhala(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParserInputBufferPush(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserInputBufferPtr in;
    PyObject *pyobj_in;
    int len;
    char * buf;

    if (!PyArg_ParseTuple(args, (char *)"Oiz:xmlParserInputBufferPush", &pyobj_in, &len, &buf))
        return(NULL);
    in = (xmlParserInputBufferPtr) PyinputBuffer_Get(pyobj_in);

    c_retval = xmlParserInputBufferPush(in, len, buf);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlFileMatch(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlFileMatch", &filename))
        return(NULL);

    c_retval = xmlFileMatch(filename);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlStrEqual(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * str1;
    xmlChar * str2;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlStrEqual", &str1, &str2))
        return(NULL);

    c_retval = xmlStrEqual(str1, str2);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderPreserve(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderPreserve", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderPreserve(reader);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsKatakanaPhoneticExtensions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsKatakanaPhoneticExtensions", &code))
        return(NULL);

    c_retval = xmlUCSIsKatakanaPhoneticExtensions(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGNewParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlRelaxNGParserCtxtPtr c_retval;
    char * URL;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlRelaxNGNewParserCtxt", &URL))
        return(NULL);

    c_retval = xmlRelaxNGNewParserCtxt(URL);
    py_retval = libxml_xmlRelaxNGParserCtxtPtrWrap((xmlRelaxNGParserCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathSetContextDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathSetContextDoc", &pyobj_ctxt, &pyobj_doc))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    ctxt->doc = doc;
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaIsValid(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlSchemaValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSchemaIsValid", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlSchemaValidCtxtPtr) PySchemaValidCtxt_Get(pyobj_ctxt);

    c_retval = xmlSchemaIsValid(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsKanbun(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsKanbun", &code))
        return(NULL);

    c_retval = xmlUCSIsKanbun(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLatin1Supplement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLatin1Supplement", &code))
        return(NULL);

    c_retval = xmlUCSIsLatin1Supplement(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlNodeSetName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNodeSetName", &pyobj_cur, &name))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeSetName(cur, name);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
PyObject *
libxml_xmlUTF8Strloc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * utf;
    xmlChar * utfchar;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlUTF8Strloc", &utf, &utfchar))
        return(NULL);

    c_retval = xmlUTF8Strloc(utf, utfchar);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlReadFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    char * filename;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"zzi:htmlReadFile", &filename, &encoding, &options))
        return(NULL);

    c_retval = htmlReadFile(filename, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsDingbats(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsDingbats", &code))
        return(NULL);

    c_retval = xmlUCSIsDingbats(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaParse(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlSchemaPtr c_retval;
    xmlSchemaParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSchemaParse", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlSchemaParserCtxtPtr) PySchemaParserCtxt_Get(pyobj_ctxt);

    c_retval = xmlSchemaParse(ctxt);
    py_retval = libxml_xmlSchemaPtrWrap((xmlSchemaPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlThrDefDefaultBufferSize(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefDefaultBufferSize", &v))
        return(NULL);

    c_retval = xmlThrDefDefaultBufferSize(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsPrivateUse(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsPrivateUse", &code))
        return(NULL);

    c_retval = xmlUCSIsPrivateUse(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlRecoverFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlRecoverFile", &filename))
        return(NULL);

    c_retval = xmlRecoverFile(filename);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextFollowingSibling(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextFollowingSibling", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextFollowingSibling(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlIsExtender(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned int ch;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsExtender", &ch))
        return(NULL);

    c_retval = xmlIsExtender(ch);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCastBooleanToString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlXPathCastBooleanToString", &val))
        return(NULL);

    c_retval = xmlXPathCastBooleanToString(val);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlUTF8Charcmp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * utf1;
    xmlChar * utf2;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlUTF8Charcmp", &utf1, &utf2))
        return(NULL);

    c_retval = xmlUTF8Charcmp(utf1, utf2);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPTR_ENABLED)
PyObject *
libxml_xmlXPtrNewRangeNodes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlNodePtr start;
    PyObject *pyobj_start;
    xmlNodePtr end;
    PyObject *pyobj_end;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPtrNewRangeNodes", &pyobj_start, &pyobj_end))
        return(NULL);
    start = (xmlNodePtr) PyxmlNode_Get(pyobj_start);
    end = (xmlNodePtr) PyxmlNode_Get(pyobj_end);

    c_retval = xmlXPtrNewRangeNodes(start, end);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPTR_ENABLED) */
PyObject *
libxml_xmlStringDecodeEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * str;
    int what;
    xmlChar end;
    xmlChar end2;
    xmlChar end3;

    if (!PyArg_ParseTuple(args, (char *)"Oziccc:xmlStringDecodeEntities", &pyobj_ctxt, &str, &what, &end, &end2, &end3))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlStringDecodeEntities(ctxt, str, what, end, end2, end3);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNotEqualValues(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathNotEqualValues", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = xmlXPathNotEqualValues(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderMoveToAttributeNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlChar * localName;
    xmlChar * namespaceURI;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlTextReaderMoveToAttributeNs", &pyobj_reader, &localName, &namespaceURI))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderMoveToAttributeNs(reader, localName, namespaceURI);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsOgham(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsOgham", &code))
        return(NULL);

    c_retval = xmlUCSIsOgham(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlNewDocComment(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNewDocComment", &pyobj_doc, &content))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlNewDocComment(doc, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBopomofoExtended(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsBopomofoExtended", &code))
        return(NULL);

    c_retval = xmlUCSIsBopomofoExtended(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKCompatibility(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKCompatibility", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKCompatibility(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGValidateFullElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlRelaxNGValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlRelaxNGValidateFullElement", &pyobj_ctxt, &pyobj_doc, &pyobj_elem))
        return(NULL);
    ctxt = (xmlRelaxNGValidCtxtPtr) PyrelaxNgValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);

    c_retval = xmlRelaxNGValidateFullElement(ctxt, doc, elem);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateDocument(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlValidateDocument", &pyobj_ctxt, &pyobj_doc))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlValidateDocument(ctxt, doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatPc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatPc", &code))
        return(NULL);

    c_retval = xmlUCSIsCatPc(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatPf(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatPf", &code))
        return(NULL);

    c_retval = xmlUCSIsCatPf(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatPd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatPd", &code))
        return(NULL);

    c_retval = xmlUCSIsCatPd(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlCtxtReadMemory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    char * buffer;
    int py_buffsize0;
    int size;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ot#izzi:htmlCtxtReadMemory", &pyobj_ctxt, &buffer, &py_buffsize0, &size, &URL, &encoding, &options))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = htmlCtxtReadMemory(ctxt, buffer, size, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatPi(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatPi", &code))
        return(NULL);

    c_retval = xmlUCSIsCatPi(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathGetContextNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathGetContextNode", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = ctxt->node;
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatPo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatPo", &code))
        return(NULL);

    c_retval = xmlUCSIsCatPo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatPs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatPs", &code))
        return(NULL);

    c_retval = xmlUCSIsCatPs(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHighSurrogates(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHighSurrogates", &code))
        return(NULL);

    c_retval = xmlUCSIsHighSurrogates(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogResolveURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * URI;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCatalogResolveURI", &URI))
        return(NULL);

    c_retval = xmlCatalogResolveURI(URI);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlURIGetScheme(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetScheme", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->scheme;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderLocatorLineNumber(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderLocatorPtr locator;
    PyObject *pyobj_locator;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderLocatorLineNumber", &pyobj_locator))
        return(NULL);
    locator = (xmlTextReaderLocatorPtr) PyxmlTextReaderLocator_Get(pyobj_locator);

    c_retval = xmlTextReaderLocatorLineNumber(locator);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGNewMemParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlRelaxNGParserCtxtPtr c_retval;
    char * buffer;
    int size;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlRelaxNGNewMemParserCtxt", &buffer, &size))
        return(NULL);

    c_retval = xmlRelaxNGNewMemParserCtxt(buffer, size);
    py_retval = libxml_xmlRelaxNGParserCtxtPtrWrap((xmlRelaxNGParserCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderAttributeCount(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderAttributeCount", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderAttributeCount(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlCharStrndup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    char * cur;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlCharStrndup", &cur, &len))
        return(NULL);

    c_retval = xmlCharStrndup(cur, len);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseEncodingDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseEncodingDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseEncodingDecl(ctxt);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsUnifiedCanadianAboriginalSyllabics(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsUnifiedCanadianAboriginalSyllabics", &code))
        return(NULL);

    c_retval = xmlUCSIsUnifiedCanadianAboriginalSyllabics(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlCopyPropList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr target;
    PyObject *pyobj_target;
    xmlAttrPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlCopyPropList", &pyobj_target, &pyobj_cur))
        return(NULL);
    target = (xmlNodePtr) PyxmlNode_Get(pyobj_target);
    cur = (xmlAttrPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlCopyPropList(target, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlDocFormatDump(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    FILE * f;
    PyObject *pyobj_f;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    int format;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlDocFormatDump", &pyobj_f, &pyobj_cur, &format))
        return(NULL);
    f = (FILE *) PyFile_Get(pyobj_f);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlDocFormatDump(f, cur, format);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
PyObject *
libxml_xmlCtxtReset(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlCtxtReset", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlCtxtReset(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlIsRef(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;
    xmlAttrPtr attr;
    PyObject *pyobj_attr;

    if (!PyArg_ParseTuple(args, (char *)"OOO:xmlIsRef", &pyobj_doc, &pyobj_elem, &pyobj_attr))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);
    attr = (xmlAttrPtr) PyxmlNode_Get(pyobj_attr);

    c_retval = xmlIsRef(doc, elem, attr);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlUTF8Strndup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * utf;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlUTF8Strndup", &utf, &len))
        return(NULL);

    c_retval = xmlUTF8Strndup(utf, len);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathGetContextDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathGetContextDoc", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = ctxt->doc;
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsTaiLe(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsTaiLe", &code))
        return(NULL);

    c_retval = xmlUCSIsTaiLe(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseComment(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseComment", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseComment(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathSubstringAfterFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathSubstringAfterFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathSubstringAfterFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlSaveFormatFileEnc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    char * encoding;
    int format;

    if (!PyArg_ParseTuple(args, (char *)"zOzi:xmlSaveFormatFileEnc", &filename, &pyobj_cur, &encoding, &format))
        return(NULL);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlSaveFormatFileEnc(filename, cur, encoding, format);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGParse(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlRelaxNGPtr c_retval;
    xmlRelaxNGParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlRelaxNGParse", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlRelaxNGParserCtxtPtr) PyrelaxNgParserCtxt_Get(pyobj_ctxt);

    c_retval = xmlRelaxNGParse(ctxt);
    py_retval = libxml_xmlRelaxNGPtrWrap((xmlRelaxNGPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlParseNmtoken(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseNmtoken", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseNmtoken(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParserGetIsValid(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParserGetIsValid", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = ctxt->valid;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMathematicalOperators(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMathematicalOperators", &code))
        return(NULL);

    c_retval = xmlUCSIsMathematicalOperators(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpDTD(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlDtdPtr dtd;
    PyObject *pyobj_dtd;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlDebugDumpDTD", &pyobj_output, &pyobj_dtd))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    dtd = (xmlDtdPtr) PyxmlNode_Get(pyobj_dtd);

    xmlDebugDumpDTD(output, dtd);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_XPTR_ENABLED)
PyObject *
libxml_xmlXPtrNewCollapsedRange(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlNodePtr start;
    PyObject *pyobj_start;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPtrNewCollapsedRange", &pyobj_start))
        return(NULL);
    start = (xmlNodePtr) PyxmlNode_Get(pyobj_start);

    c_retval = xmlXPtrNewCollapsedRange(start);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPTR_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNotFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathNotFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathNotFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlTextConcat(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * content;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlTextConcat", &pyobj_node, &content, &len))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlTextConcat(node, content, len);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParsePI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParsePI", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParsePI(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlLoadCatalogs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    char * pathss;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlLoadCatalogs", &pathss))
        return(NULL);

    xmlLoadCatalogs(pathss);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlCtxtReadDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * cur;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ozzzi:htmlCtxtReadDoc", &pyobj_ctxt, &cur, &URL, &encoding, &options))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = htmlCtxtReadDoc(ctxt, cur, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_PUSH_ENABLED)
PyObject *
libxml_xmlParseChunk(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    char * chunk;
    int py_buffsize0;
    int size;
    int terminate;

    if (!PyArg_ParseTuple(args, (char *)"Ot#ii:xmlParseChunk", &pyobj_ctxt, &chunk, &py_buffsize0, &size, &terminate))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseChunk(ctxt, chunk, size, terminate);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_PUSH_ENABLED) */
#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlSaveFileEnc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * filename;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"zOz:htmlSaveFileEnc", &filename, &pyobj_cur, &encoding))
        return(NULL);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = htmlSaveFileEnc(filename, cur, encoding);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
PyObject *
libxml_xmlParseElementDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseElementDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseElementDecl(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlReaderForFd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlTextReaderPtr c_retval;
    int fd;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"izzi:xmlReaderForFd", &fd, &URL, &encoding, &options))
        return(NULL);

    c_retval = xmlReaderForFd(fd, URL, encoding, options);
    py_retval = libxml_xmlTextReaderPtrWrap((xmlTextReaderPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKCompatibilityIdeographs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKCompatibilityIdeographs", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKCompatibilityIdeographs(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderMoveToFirstAttribute(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderMoveToFirstAttribute", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderMoveToFirstAttribute(reader);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlNewTextReader(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlTextReaderPtr c_retval;
    xmlParserInputBufferPtr input;
    PyObject *pyobj_input;
    char * URI;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNewTextReader", &pyobj_input, &URI))
        return(NULL);
    input = (xmlParserInputBufferPtr) PyinputBuffer_Get(pyobj_input);

    c_retval = xmlNewTextReader(input, URI);
    py_retval = libxml_xmlTextReaderPtrWrap((xmlTextReaderPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderGetAttributeNo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    int no;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlTextReaderGetAttributeNo", &pyobj_reader, &no))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderGetAttributeNo(reader, no);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderGetAttributeNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlChar * localName;
    xmlChar * namespaceURI;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlTextReaderGetAttributeNs", &pyobj_reader, &localName, &namespaceURI))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderGetAttributeNs(reader, localName, namespaceURI);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlURIGetQuery(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetQuery", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->query;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsGeneralPunctuation(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsGeneralPunctuation", &code))
        return(NULL);

    c_retval = xmlUCSIsGeneralPunctuation(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsControlPictures(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsControlPictures", &code))
        return(NULL);

    c_retval = xmlUCSIsControlPictures(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlIsBooleanAttr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"z:htmlIsBooleanAttr", &name))
        return(NULL);

    c_retval = htmlIsBooleanAttr(name);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
PyObject *
libxml_xmlNodeListGetString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr list;
    PyObject *pyobj_list;
    int inLine;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlNodeListGetString", &pyobj_doc, &pyobj_list, &inLine))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    list = (xmlNodePtr) PyxmlNode_Get(pyobj_list);

    c_retval = xmlNodeListGetString(doc, list, inLine);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBengali(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsBengali", &code))
        return(NULL);

    c_retval = xmlUCSIsBengali(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlBuildQName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * ncname;
    xmlChar * prefix;
    xmlChar * memory;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zzzi:xmlBuildQName", &ncname, &prefix, &memory, &len))
        return(NULL);

    c_retval = xmlBuildQName(ncname, prefix, memory, len);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlFreePropList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlAttrPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreePropList", &pyobj_cur))
        return(NULL);
    cur = (xmlAttrPtr) PyxmlNode_Get(pyobj_cur);

    xmlFreePropList(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathStringFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathStringFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathStringFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlInitParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlInitParserCtxt", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlInitParserCtxt(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsTagbanwa(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsTagbanwa", &code))
        return(NULL);

    c_retval = xmlUCSIsTagbanwa(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstBaseUri(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstBaseUri", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstBaseUri(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsDeseret(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsDeseret", &code))
        return(NULL);

    c_retval = xmlUCSIsDeseret(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathRoundFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathRoundFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathRoundFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatSm(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatSm", &code))
        return(NULL);

    c_retval = xmlUCSIsCatSm(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderMoveToAttributeNo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    int no;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlTextReaderMoveToAttributeNo", &pyobj_reader, &no))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderMoveToAttributeNo(reader, no);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlParserHandlePEReference(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParserHandlePEReference", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParserHandlePEReference(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNewBoolean(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlXPathNewBoolean", &val))
        return(NULL);

    c_retval = xmlXPathNewBoolean(val);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsPrivateUseArea(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsPrivateUseArea", &code))
        return(NULL);

    c_retval = xmlUCSIsPrivateUseArea(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlCtxtReadFd(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int fd;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Oizzi:xmlCtxtReadFd", &pyobj_ctxt, &fd, &URL, &encoding, &options))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlCtxtReadFd(ctxt, fd, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsAlphabeticPresentationForms(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsAlphabeticPresentationForms", &code))
        return(NULL);

    c_retval = xmlUCSIsAlphabeticPresentationForms(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCypriotSyllabary(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCypriotSyllabary", &code))
        return(NULL);

    c_retval = xmlUCSIsCypriotSyllabary(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlGetNsProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;
    xmlChar * nameSpace;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlGetNsProp", &pyobj_node, &name, &nameSpace))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlGetNsProp(node, name, nameSpace);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatC(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatC", &code))
        return(NULL);

    c_retval = xmlUCSIsCatC(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatN(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatN", &code))
        return(NULL);

    c_retval = xmlUCSIsCatN(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatL(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatL", &code))
        return(NULL);

    c_retval = xmlUCSIsCatL(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatM(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatM", &code))
        return(NULL);

    c_retval = xmlUCSIsCatM(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlCtxtResetPush(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    char * chunk;
    int py_buffsize0;
    int size;
    char * filename;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"Ot#izz:xmlCtxtResetPush", &pyobj_ctxt, &chunk, &py_buffsize0, &size, &filename, &encoding))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlCtxtResetPush(ctxt, chunk, size, filename, encoding);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatS(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatS", &code))
        return(NULL);

    c_retval = xmlUCSIsCatS(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatP(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatP", &code))
        return(NULL);

    c_retval = xmlUCSIsCatP(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlCatalogGetSystem(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlChar * sysID;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCatalogGetSystem", &sysID))
        return(NULL);

    c_retval = xmlCatalogGetSystem(sysID);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatZ(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatZ", &code))
        return(NULL);

    c_retval = xmlUCSIsCatZ(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSuperscriptsandSubscripts(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSuperscriptsandSubscripts", &code))
        return(NULL);

    c_retval = xmlUCSIsSuperscriptsandSubscripts(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsTagalog(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsTagalog", &code))
        return(NULL);

    c_retval = xmlUCSIsTagalog(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlGetDtdElementDesc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlElementPtr c_retval;
    xmlDtdPtr dtd;
    PyObject *pyobj_dtd;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlGetDtdElementDesc", &pyobj_dtd, &name))
        return(NULL);
    dtd = (xmlDtdPtr) PyxmlNode_Get(pyobj_dtd);

    c_retval = xmlGetDtdElementDesc(dtd, name);
    py_retval = libxml_xmlElementPtrWrap((xmlElementPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsPhoneticExtensions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsPhoneticExtensions", &code))
        return(NULL);

    c_retval = xmlUCSIsPhoneticExtensions(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCastNodeToString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathCastNodeToString", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlXPathCastNodeToString(node);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlURISetPort(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    int port;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlURISetPort", &pyobj_URI, &port))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    URI->port = port;
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlNamespaceParseNCName(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlNamespaceParseNCName", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlNamespaceParseNCName(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
PyObject *
libxml_xmlInitGlobals(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlInitGlobals();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsEthiopic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsEthiopic", &code))
        return(NULL);

    c_retval = xmlUCSIsEthiopic(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlParseFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    char * filename;
    char * encoding;

    if (!PyArg_ParseTuple(args, (char *)"zz:htmlParseFile", &filename, &encoding))
        return(NULL);

    c_retval = htmlParseFile(filename, encoding);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugCheckDocument(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    FILE * output;
    PyObject *pyobj_output;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlDebugCheckDocument", &pyobj_output, &pyobj_doc))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlDebugCheckDocument(output, doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlReadDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    xmlChar * cur;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"zzzi:htmlReadDoc", &cur, &URL, &encoding, &options))
        return(NULL);

    c_retval = htmlReadDoc(cur, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathGetContextPosition(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathGetContextPosition", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = ctxt->proximityPosition;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlNodeDumpFileFormat(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    FILE * out;
    PyObject *pyobj_out;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    char * encoding;
    int format;

    if (!PyArg_ParseTuple(args, (char *)"OOOzi:htmlNodeDumpFileFormat", &pyobj_out, &pyobj_doc, &pyobj_cur, &encoding, &format))
        return(NULL);
    out = (FILE *) PyFile_Get(pyobj_out);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = htmlNodeDumpFileFormat(out, doc, cur, encoding, format);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstXmlLang(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstXmlLang", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstXmlLang(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCherokee(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCherokee", &code))
        return(NULL);

    c_retval = xmlUCSIsCherokee(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlNodeSetContent(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNodeSetContent", &pyobj_cur, &content))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeSetContent(cur, content);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlUnlinkNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlUnlinkNode", &pyobj_cur))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlUnlinkNode(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlBoolToText(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    int boolval;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlBoolToText", &boolval))
        return(NULL);

    c_retval = xmlBoolToText(boolval);
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
PyObject *
libxml_xmlSetCompressMode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    int mode;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlSetCompressMode", &mode))
        return(NULL);

    xmlSetCompressMode(mode);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlParseDocument(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:htmlParseDocument", &pyobj_ctxt))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = htmlParseDocument(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathSubstringFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathSubstringFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathSubstringFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED)
PyObject *
libxml_xmlDebugDumpNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlNodePtr node;
    PyObject *pyobj_node;
    int depth;

    if (!PyArg_ParseTuple(args, (char *)"OOi:xmlDebugDumpNode", &pyobj_output, &pyobj_node, &depth))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    xmlDebugDumpNode(output, node, depth);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlCopyDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    int recursive;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlCopyDoc", &pyobj_doc, &recursive))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlCopyDoc(doc, recursive);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlCtxtReadMemory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    char * buffer;
    int py_buffsize0;
    int size;
    char * URL;
    char * encoding;
    int options;

    if (!PyArg_ParseTuple(args, (char *)"Ot#izzi:xmlCtxtReadMemory", &pyobj_ctxt, &buffer, &py_buffsize0, &size, &URL, &encoding, &options))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlCtxtReadMemory(ctxt, buffer, size, URL, encoding, options);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlCreateFileParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlParserCtxtPtr c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCreateFileParserCtxt", &filename))
        return(NULL);

    c_retval = xmlCreateFileParserCtxt(filename);
    py_retval = libxml_xmlParserCtxtPtrWrap((xmlParserCtxtPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseSystemLiteral(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseSystemLiteral", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseSystemLiteral(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseAttributeListDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseAttributeListDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseAttributeListDecl(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaNewDocParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlSchemaParserCtxtPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSchemaNewDocParserCtxt", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlSchemaNewDocParserCtxt(doc);
    py_retval = libxml_xmlSchemaParserCtxtPtrWrap((xmlSchemaParserCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_nodePush(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr value;
    PyObject *pyobj_value;

    if (!PyArg_ParseTuple(args, (char *)"OO:nodePush", &pyobj_ctxt, &pyobj_value))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);
    value = (xmlNodePtr) PyxmlNode_Get(pyobj_value);

    c_retval = nodePush(ctxt, value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XINCLUDE_ENABLED)
PyObject *
libxml_xmlXIncludeProcess(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXIncludeProcess", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlXIncludeProcess(doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XINCLUDE_ENABLED) */
#if defined(LIBXML_REGEXP_ENABLED)
PyObject *
libxml_xmlRegexpIsDeterminist(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlRegexpPtr comp;
    PyObject *pyobj_comp;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlRegexpIsDeterminist", &pyobj_comp))
        return(NULL);
    comp = (xmlRegexpPtr) PyxmlReg_Get(pyobj_comp);

    c_retval = xmlRegexpIsDeterminist(comp);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_REGEXP_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlNewDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    htmlDocPtr c_retval;
    xmlChar * URI;
    xmlChar * ExternalID;

    if (!PyArg_ParseTuple(args, (char *)"zz:htmlNewDoc", &URI, &ExternalID))
        return(NULL);

    c_retval = htmlNewDoc(URI, ExternalID);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCat(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;
    char * cat;

    if (!PyArg_ParseTuple(args, (char *)"iz:xmlUCSIsCat", &code, &cat))
        return(NULL);

    c_retval = xmlUCSIsCat(code, cat);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlIsScriptAttribute(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"z:htmlIsScriptAttribute", &name))
        return(NULL);

    c_retval = htmlIsScriptAttribute(name);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlInitializePredefinedEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlInitializePredefinedEntities();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMiscellaneousTechnical(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMiscellaneousTechnical", &code))
        return(NULL);

    c_retval = xmlUCSIsMiscellaneousTechnical(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_DEBUG_ENABLED) && defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlShellPrintNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlShellPrintNode", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    xmlShellPrintNode(node);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_DEBUG_ENABLED) && defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlValidateNMToken(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;
    int space;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlValidateNMToken", &value, &space))
        return(NULL);

    c_retval = xmlValidateNMToken(value, space);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlErrorGetCode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlErrorPtr Error;
    PyObject *pyobj_Error;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlErrorGetCode", &pyobj_Error))
        return(NULL);
    Error = (xmlErrorPtr) PyError_Get(pyobj_Error);

    c_retval = Error->code;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateNameValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlValidateNameValue", &value))
        return(NULL);

    c_retval = xmlValidateNameValue(value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNewContext(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathContextPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathNewContext", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlXPathNewContext(doc);
    py_retval = libxml_xmlXPathContextPtrWrap((xmlXPathContextPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxNGNewDocParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlRelaxNGParserCtxtPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlRelaxNGNewDocParserCtxt", &pyobj_doc))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlRelaxNGNewDocParserCtxt(doc);
    py_retval = libxml_xmlRelaxNGParserCtxtPtrWrap((xmlRelaxNGParserCtxtPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlBuildRelativeURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * URI;
    xmlChar * base;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlBuildRelativeURI", &URI, &base))
        return(NULL);

    c_retval = xmlBuildRelativeURI(URI, base);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlACatalogResolvePublic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;
    xmlChar * pubID;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlACatalogResolvePublic", &pyobj_catal, &pubID))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    c_retval = xmlACatalogResolvePublic(catal, pubID);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
PyObject *
libxml_xmlThrDefParserDebugEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefParserDebugEntities", &v))
        return(NULL);

    c_retval = xmlThrDefParserDebugEntities(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlFreeCatalog(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeCatalog", &pyobj_catal))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    xmlFreeCatalog(catal);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCJKSymbolsandPunctuation(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCJKSymbolsandPunctuation", &code))
        return(NULL);

    c_retval = xmlUCSIsCJKSymbolsandPunctuation(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMusicalSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMusicalSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsMusicalSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_FTP_ENABLED)
PyObject *
libxml_xmlNanoFTPInit(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlNanoFTPInit();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_FTP_ENABLED) */
PyObject *
libxml_xmlURIGetUser(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetUser", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->user;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlGetLastChild(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr parent;
    PyObject *pyobj_parent;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlGetLastChild", &pyobj_parent))
        return(NULL);
    parent = (xmlNodePtr) PyxmlNode_Get(pyobj_parent);

    c_retval = xmlGetLastChild(parent);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlAddDtdEntity(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlEntityPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;
    int type;
    xmlChar * ExternalID;
    xmlChar * SystemID;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"Ozizzz:xmlAddDtdEntity", &pyobj_doc, &name, &type, &ExternalID, &SystemID, &content))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlAddDtdEntity(doc, name, type, ExternalID, SystemID, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseNotationDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseNotationDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseNotationDecl(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlNewDocRawNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;
    xmlChar * content;

    if (!PyArg_ParseTuple(args, (char *)"OOzz:xmlNewDocRawNode", &pyobj_doc, &pyobj_ns, &name, &content))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlNewDocRawNode(doc, ns, name, content);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaCollapseString(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlSchemaCollapseString", &value))
        return(NULL);

    c_retval = xmlSchemaCollapseString(value);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderConstNamespaceUri(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderConstNamespaceUri", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderConstNamespaceUri(reader);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsBasicLatin(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsBasicLatin", &code))
        return(NULL);

    c_retval = xmlUCSIsBasicLatin(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseMisc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseMisc", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseMisc(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlParserInputBufferGrow(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlParserInputBufferPtr in;
    PyObject *pyobj_in;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlParserInputBufferGrow", &pyobj_in, &len))
        return(NULL);
    in = (xmlParserInputBufferPtr) PyinputBuffer_Get(pyobj_in);

    c_retval = xmlParserInputBufferGrow(in, len);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextChild(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextChild", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextChild(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderGetParserProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    int prop;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlTextReaderGetParserProp", &pyobj_reader, &prop))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderGetParserProp(reader, prop);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlStrncat(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * cur;
    xmlChar * add;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zzi:xmlStrncat", &cur, &add, &len))
        return(NULL);

    c_retval = xmlStrncat(cur, add, len);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
#endif
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPatherror(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    char * file;
    int line;
    int no;

    if (!PyArg_ParseTuple(args, (char *)"Ozii:xmlXPatherror", &pyobj_ctxt, &file, &line, &no))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPatherror(ctxt, file, line, no);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlParseMemory(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    char * buffer;
    int py_buffsize0;
    int size;

    if (!PyArg_ParseTuple(args, (char *)"t#i:xmlParseMemory", &buffer, &py_buffsize0, &size))
        return(NULL);

    c_retval = xmlParseMemory(buffer, size);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
PyObject *
libxml_xmlCleanupEncodingAliases(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlCleanupEncodingAliases();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCeilingFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathCeilingFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathCeilingFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSmallFormVariants(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSmallFormVariants", &code))
        return(NULL);

    c_retval = xmlUCSIsSmallFormVariants(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlInitParser(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlInitParser();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathStartsWithFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathStartsWithFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathStartsWithFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
PyObject *
libxml_xmlSearchNsByHref(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * href;

    if (!PyArg_ParseTuple(args, (char *)"OOz:xmlSearchNsByHref", &pyobj_doc, &pyobj_node, &href))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlSearchNsByHref(doc, node, href);
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlParseTextDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseTextDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseTextDecl(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathNextPreceding(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathNextPreceding", &pyobj_ctxt, &pyobj_cur))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlXPathNextPreceding(ctxt, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathRegisterAllFunctions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathRegisterAllFunctions", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    xmlXPathRegisterAllFunctions(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathRegisteredVariablesCleanup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathRegisteredVariablesCleanup", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    xmlXPathRegisteredVariablesCleanup(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlHandleEntity(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlEntityPtr entity;
    PyObject *pyobj_entity;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlHandleEntity", &pyobj_ctxt, &pyobj_entity))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);
    entity = (xmlEntityPtr) PyxmlNode_Get(pyobj_entity);

    xmlHandleEntity(ctxt, entity);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlACatalogResolve(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;
    xmlChar * pubID;
    xmlChar * sysID;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlACatalogResolve", &pyobj_catal, &pubID, &sysID))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    c_retval = xmlACatalogResolve(catal, pubID, sysID);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaValidCtxtGetOptions(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlSchemaValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlSchemaValidCtxtGetOptions", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlSchemaValidCtxtPtr) PySchemaValidCtxt_Get(pyobj_ctxt);

    c_retval = xmlSchemaValidCtxtGetOptions(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlParseDTD(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDtdPtr c_retval;
    xmlChar * ExternalID;
    xmlChar * SystemID;

    if (!PyArg_ParseTuple(args, (char *)"zz:xmlParseDTD", &ExternalID, &SystemID))
        return(NULL);

    c_retval = xmlParseDTD(ExternalID, SystemID);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateDocumentFinal(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlValidateDocumentFinal", &pyobj_ctxt, &pyobj_doc))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlValidateDocumentFinal(ctxt, doc);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
PyObject *
libxml_xmlIsLetter(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int c;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsLetter", &c))
        return(NULL);

    c_retval = xmlIsLetter(c);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlTextMerge(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr first;
    PyObject *pyobj_first;
    xmlNodePtr second;
    PyObject *pyobj_second;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlTextMerge", &pyobj_first, &pyobj_second))
        return(NULL);
    first = (xmlNodePtr) PyxmlNode_Get(pyobj_first);
    second = (xmlNodePtr) PyxmlNode_Get(pyobj_second);

    c_retval = xmlTextMerge(first, second);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlPrintURI(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * stream;
    PyObject *pyobj_stream;
    xmlURIPtr uri;
    PyObject *pyobj_uri;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlPrintURI", &pyobj_stream, &pyobj_uri))
        return(NULL);
    stream = (FILE *) PyFile_Get(pyobj_stream);
    uri = (xmlURIPtr) PyURI_Get(pyobj_uri);

    xmlPrintURI(stream, uri);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathValueFlipSign(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathValueFlipSign", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathValueFlipSign(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlRelaxParserSetFlag(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlRelaxNGParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int flags;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlRelaxParserSetFlag", &pyobj_ctxt, &flags))
        return(NULL);
    ctxt = (xmlRelaxNGParserCtxtPtr) PyrelaxNgParserCtxt_Get(pyobj_ctxt);

    c_retval = xmlRelaxParserSetFlag(ctxt, flags);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlParserSetLoadSubset(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int loadsubset;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlParserSetLoadSubset", &pyobj_ctxt, &loadsubset))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    ctxt->loadsubset = loadsubset;
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateOneNamespace(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlValidCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr elem;
    PyObject *pyobj_elem;
    xmlChar * prefix;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"OOOzOz:xmlValidateOneNamespace", &pyobj_ctxt, &pyobj_doc, &pyobj_elem, &prefix, &pyobj_ns, &value))
        return(NULL);
    ctxt = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_ctxt);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    elem = (xmlNodePtr) PyxmlNode_Get(pyobj_elem);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlValidateOneNamespace(ctxt, doc, elem, prefix, ns, value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED)
PyObject *
libxml_xmlReplaceNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr old;
    PyObject *pyobj_old;
    xmlNodePtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlReplaceNode", &pyobj_old, &pyobj_cur))
        return(NULL);
    old = (xmlNodePtr) PyxmlNode_Get(pyobj_old);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlReplaceNode(old, cur);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_WRITER_ENABLED) */
PyObject *
libxml_xmlSetDocCompressMode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    int mode;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlSetDocCompressMode", &pyobj_doc, &mode))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    xmlSetDocCompressMode(doc, mode);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPTR_ENABLED)
PyObject *
libxml_xmlXPtrNewRange(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlNodePtr start;
    PyObject *pyobj_start;
    int startindex;
    xmlNodePtr end;
    PyObject *pyobj_end;
    int endindex;

    if (!PyArg_ParseTuple(args, (char *)"OiOi:xmlXPtrNewRange", &pyobj_start, &startindex, &pyobj_end, &endindex))
        return(NULL);
    start = (xmlNodePtr) PyxmlNode_Get(pyobj_start);
    end = (xmlNodePtr) PyxmlNode_Get(pyobj_end);

    c_retval = xmlXPtrNewRange(start, startindex, end, endindex);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPTR_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathMultValues(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathMultValues", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathMultValues(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_xmlSchemaDump(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    FILE * output;
    PyObject *pyobj_output;
    xmlSchemaPtr schema;
    PyObject *pyobj_schema;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlSchemaDump", &pyobj_output, &pyobj_schema))
        return(NULL);
    output = (FILE *) PyFile_Get(pyobj_output);
    schema = (xmlSchemaPtr) PySchema_Get(pyobj_schema);

    xmlSchemaDump(output, schema);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlParseFile(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    char * filename;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlParseFile", &filename))
        return(NULL);

    c_retval = xmlParseFile(filename);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
#if defined(LIBXML_SAX1_ENABLED)
PyObject *
libxml_xmlParseEndTag(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseEndTag", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseEndTag(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SAX1_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsHanunoo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsHanunoo", &code))
        return(NULL);

    c_retval = xmlUCSIsHanunoo(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParserSetLineNumbers(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    int linenumbers;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlParserSetLineNumbers", &pyobj_ctxt, &linenumbers))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    ctxt->linenumbers = linenumbers;
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlParsePEReference(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParsePEReference", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParsePEReference(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlDelEncodingAlias(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    char * alias;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlDelEncodingAlias", &alias))
        return(NULL);

    c_retval = xmlDelEncodingAlias(alias);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlNodeSetContentLen(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlChar * content;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"Ozi:xmlNodeSetContentLen", &pyobj_cur, &content, &len))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeSetContentLen(cur, content, len);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
PyObject *
libxml_xmlThrDefPedanticParserDefaultValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefPedanticParserDefaultValue", &v))
        return(NULL);

    c_retval = xmlThrDefPedanticParserDefaultValue(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlUTF8Strpos(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const xmlChar * c_retval;
    xmlChar * utf;
    int pos;

    if (!PyArg_ParseTuple(args, (char *)"zi:xmlUTF8Strpos", &utf, &pos))
        return(NULL);

    c_retval = xmlUTF8Strpos(utf, pos);
    py_retval = libxml_xmlCharPtrConstWrap((const xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlUnsetNsProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlNsPtr ns;
    PyObject *pyobj_ns;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"OOz:xmlUnsetNsProp", &pyobj_node, &pyobj_ns, &name))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    ns = (xmlNsPtr) PyxmlNode_Get(pyobj_ns);

    c_retval = xmlUnsetNsProp(node, ns, name);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathRegisterNs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * prefix;
    xmlChar * ns_uri;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlXPathRegisterNs", &pyobj_ctxt, &prefix, &ns_uri))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = xmlXPathRegisterNs(ctxt, prefix, ns_uri);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCurrencySymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCurrencySymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsCurrencySymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCmpNodes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr node1;
    PyObject *pyobj_node1;
    xmlNodePtr node2;
    PyObject *pyobj_node2;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlXPathCmpNodes", &pyobj_node1, &pyobj_node2))
        return(NULL);
    node1 = (xmlNodePtr) PyxmlNode_Get(pyobj_node1);
    node2 = (xmlNodePtr) PyxmlNode_Get(pyobj_node2);

    c_retval = xmlXPathCmpNodes(node1, node2);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathVariableLookupNS(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlXPathObjectPtr c_retval;
    xmlXPathContextPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * name;
    xmlChar * ns_uri;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlXPathVariableLookupNS", &pyobj_ctxt, &name, &ns_uri))
        return(NULL);
    ctxt = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctxt);

    c_retval = xmlXPathVariableLookupNS(ctxt, name, ns_uri);
    py_retval = libxml_xmlXPathObjectPtrWrap((xmlXPathObjectPtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_XINCLUDE_ENABLED)
PyObject *
libxml_xmlXIncludeProcessFlags(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    int flags;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXIncludeProcessFlags", &pyobj_doc, &flags))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlXIncludeProcessFlags(doc, flags);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XINCLUDE_ENABLED) */
PyObject *
libxml_xmlUTF8Strsub(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * utf;
    int start;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zii:xmlUTF8Strsub", &utf, &start, &len))
        return(NULL);

    c_retval = xmlUTF8Strsub(utf, start, len);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlIsMixedElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlIsMixedElement", &pyobj_doc, &name))
        return(NULL);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    c_retval = xmlIsMixedElement(doc, name);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsMiscellaneousSymbolsandArrows(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsMiscellaneousSymbolsandArrows", &code))
        return(NULL);

    c_retval = xmlUCSIsMiscellaneousSymbolsandArrows(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
PyObject *
libxml_htmlNodeDumpFormatOutput(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlOutputBufferPtr buf;
    PyObject *pyobj_buf;
    xmlDocPtr doc;
    PyObject *pyobj_doc;
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    char * encoding;
    int format;

    if (!PyArg_ParseTuple(args, (char *)"OOOzi:htmlNodeDumpFormatOutput", &pyobj_buf, &pyobj_doc, &pyobj_cur, &encoding, &format))
        return(NULL);
    buf = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_buf);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    htmlNodeDumpFormatOutput(buf, doc, cur, encoding, format);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) && defined(LIBXML_OUTPUT_ENABLED) */
PyObject *
libxml_xmlParseExternalSubset(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlChar * ExternalID;
    xmlChar * SystemID;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlParseExternalSubset", &pyobj_ctxt, &ExternalID, &SystemID))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseExternalSubset(ctxt, ExternalID, SystemID);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlURIGetOpaque(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    const char * c_retval;
    xmlURIPtr URI;
    PyObject *pyobj_URI;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlURIGetOpaque", &pyobj_URI))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    c_retval = URI->opaque;
    py_retval = libxml_charPtrConstWrap((const char *) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlDictCleanup(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlDictCleanup();
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderReadInnerXml(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderReadInnerXml", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderReadInnerXml(reader);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlThrDefKeepBlanksDefaultValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefKeepBlanksDefaultValue", &v))
        return(NULL);

    c_retval = xmlThrDefKeepBlanksDefaultValue(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_VALID_ENABLED)
PyObject *
libxml_xmlValidateNmtokensValue(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlValidateNmtokensValue", &value))
        return(NULL);

    c_retval = xmlValidateNmtokensValue(value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_VALID_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsThaana(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsThaana", &code))
        return(NULL);

    c_retval = xmlUCSIsThaana(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlStrsub(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlChar * str;
    int start;
    int len;

    if (!PyArg_ParseTuple(args, (char *)"zii:xmlStrsub", &str, &start, &len))
        return(NULL);

    c_retval = xmlStrsub(str, start, len);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsYiRadicals(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsYiRadicals", &code))
        return(NULL);

    c_retval = xmlUCSIsYiRadicals(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_LEGACY_ENABLED)
PyObject *
libxml_xmlCleanupPredefinedEntities(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlCleanupPredefinedEntities();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_LEGACY_ENABLED) */
#if defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlSchemaInitTypes(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlSchemaInitTypes();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_SCHEMAS_ENABLED) */
PyObject *
libxml_xmlParseElement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseElement", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseElement(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsOriya(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsOriya", &code))
        return(NULL);

    c_retval = xmlUCSIsOriya(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseVersionInfo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseVersionInfo", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = xmlParseVersionInfo(ctxt);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathSubstringBeforeFunction(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;
    int nargs;

    if (!PyArg_ParseTuple(args, (char *)"Oi:xmlXPathSubstringBeforeFunction", &pyobj_ctxt, &nargs))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathSubstringBeforeFunction(ctxt, nargs);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_OUTPUT_ENABLED) && defined(LIBXML_HTTP_ENABLED)
PyObject *
libxml_xmlRegisterHTTPPostCallbacks(PyObject *self ATTRIBUTE_UNUSED, PyObject *args ATTRIBUTE_UNUSED) {

    xmlRegisterHTTPPostCallbacks();
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_OUTPUT_ENABLED) && defined(LIBXML_HTTP_ENABLED) */
PyObject *
libxml_xmlSetTreeDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr tree;
    PyObject *pyobj_tree;
    xmlDocPtr doc;
    PyObject *pyobj_doc;

    if (!PyArg_ParseTuple(args, (char *)"OO:xmlSetTreeDoc", &pyobj_tree, &pyobj_doc))
        return(NULL);
    tree = (xmlNodePtr) PyxmlNode_Get(pyobj_tree);
    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);

    xmlSetTreeDoc(tree, doc);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlCopyNodeList(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlCopyNodeList", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlCopyNodeList(node);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlParseCharRef(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:htmlParseCharRef", &pyobj_ctxt))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    c_retval = htmlParseCharRef(ctxt);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsAegeanNumbers(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsAegeanNumbers", &code))
        return(NULL);

    c_retval = xmlUCSIsAegeanNumbers(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlCheckUTF8(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned char * utf;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlCheckUTF8", &utf))
        return(NULL);

    c_retval = xmlCheckUTF8(utf);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsOldItalic(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsOldItalic", &code))
        return(NULL);

    c_retval = xmlUCSIsOldItalic(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathPopNumber(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    double c_retval;
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathPopNumber", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    c_retval = xmlXPathPopNumber(ctxt);
    py_retval = libxml_doubleWrap((double) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsEnclosedAlphanumerics(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsEnclosedAlphanumerics", &code))
        return(NULL);

    c_retval = xmlUCSIsEnclosedAlphanumerics(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathCastBooleanToNumber(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    double c_retval;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlXPathCastBooleanToNumber", &val))
        return(NULL);

    c_retval = xmlXPathCastBooleanToNumber(val);
    py_retval = libxml_doubleWrap((double) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsIdeographicDescriptionCharacters(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsIdeographicDescriptionCharacters", &code))
        return(NULL);

    c_retval = xmlUCSIsIdeographicDescriptionCharacters(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlGetLineNo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    long c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlGetLineNo", &pyobj_node))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlGetLineNo(node);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlURISetUser(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlURIPtr URI;
    PyObject *pyobj_URI;
    char * user;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlURISetUser", &pyobj_URI, &user))
        return(NULL);
    URI = (xmlURIPtr) PyURI_Get(pyobj_URI);

    if (URI->user != NULL) xmlFree(URI->user);
    URI->user = (char *)xmlStrdup((const xmlChar *)user);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlCreateEntityParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlParserCtxtPtr c_retval;
    xmlChar * URL;
    xmlChar * ID;
    xmlChar * base;

    if (!PyArg_ParseTuple(args, (char *)"zzz:xmlCreateEntityParserCtxt", &URL, &ID, &base))
        return(NULL);

    c_retval = xmlCreateEntityParserCtxt(URL, ID, base);
    py_retval = libxml_xmlParserCtxtPtrWrap((xmlParserCtxtPtr) c_retval);
    return(py_retval);
}

PyObject *
libxml_xmlThrDefSaveNoEmptyTags(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int v;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlThrDefSaveNoEmptyTags", &v))
        return(NULL);

    c_retval = xmlThrDefSaveNoEmptyTags(v);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderGetAttribute(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlChar * c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlTextReaderGetAttribute", &pyobj_reader, &name))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderGetAttribute(reader, name);
    py_retval = libxml_xmlCharPtrWrap((xmlChar *) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlKeepBlanksDefault(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int val;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlKeepBlanksDefault", &val))
        return(NULL);

    c_retval = xmlKeepBlanksDefault(val);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSupplementaryPrivateUseAreaB(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSupplementaryPrivateUseAreaB", &code))
        return(NULL);

    c_retval = xmlUCSIsSupplementaryPrivateUseAreaB(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsSupplementaryPrivateUseAreaA(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsSupplementaryPrivateUseAreaA", &code))
        return(NULL);

    c_retval = xmlUCSIsSupplementaryPrivateUseAreaA(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_READER_ENABLED)
PyObject *
libxml_xmlTextReaderCurrentNode(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlNodePtr c_retval;
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlTextReaderCurrentNode", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);

    c_retval = xmlTextReaderCurrentNode(reader);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_READER_ENABLED) */
PyObject *
libxml_xmlNewDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlDocPtr c_retval;
    xmlChar * version;

    if (!PyArg_ParseTuple(args, (char *)"z:xmlNewDoc", &version))
        return(NULL);

    c_retval = xmlNewDoc(version);
    py_retval = libxml_xmlDocPtrWrap((xmlDocPtr) c_retval);
    return(py_retval);
}

#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLetterlikeSymbols(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLetterlikeSymbols", &code))
        return(NULL);

    c_retval = xmlUCSIsLetterlikeSymbols(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatZp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatZp", &code))
        return(NULL);

    c_retval = xmlUCSIsCatZp(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatZs(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatZs", &code))
        return(NULL);

    c_retval = xmlUCSIsCatZs(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsCatZl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsCatZl", &code))
        return(NULL);

    c_retval = xmlUCSIsCatZl(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_CATALOG_ENABLED)
PyObject *
libxml_xmlACatalogRemove(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlCatalogPtr catal;
    PyObject *pyobj_catal;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlACatalogRemove", &pyobj_catal, &value))
        return(NULL);
    catal = (xmlCatalogPtr) Pycatalog_Get(pyobj_catal);

    c_retval = xmlACatalogRemove(catal, value);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_CATALOG_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED)
PyObject *
libxml_xmlUnsetProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlUnsetProp", &pyobj_node, &name))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlUnsetProp(node, name);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) */
#if defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_htmlFreeParserCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    htmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:htmlFreeParserCtxt", &pyobj_ctxt))
        return(NULL);
    ctxt = (htmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    htmlFreeParserCtxt(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsVariationSelectorsSupplement(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsVariationSelectorsSupplement", &code))
        return(NULL);

    c_retval = xmlUCSIsVariationSelectorsSupplement(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
#if defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED)
PyObject *
libxml_xmlSetProp(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    xmlAttrPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar * name;
    xmlChar * value;

    if (!PyArg_ParseTuple(args, (char *)"Ozz:xmlSetProp", &pyobj_node, &name, &value))
        return(NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    c_retval = xmlSetProp(node, name, value);
    py_retval = libxml_xmlNodePtrWrap((xmlNodePtr) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_TREE_ENABLED) || defined(LIBXML_XINCLUDE_ENABLED) || defined(LIBXML_SCHEMAS_ENABLED) || defined(LIBXML_HTML_ENABLED) */
#if defined(LIBXML_TREE_ENABLED)
PyObject *
libxml_xmlNodeSetLang(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlNodePtr cur;
    PyObject *pyobj_cur;
    xmlChar * lang;

    if (!PyArg_ParseTuple(args, (char *)"Oz:xmlNodeSetLang", &pyobj_cur, &lang))
        return(NULL);
    cur = (xmlNodePtr) PyxmlNode_Get(pyobj_cur);

    xmlNodeSetLang(cur, lang);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_TREE_ENABLED) */
PyObject *
libxml_xmlFreeDoc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlDocPtr cur;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlFreeDoc", &pyobj_cur))
        return(NULL);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    xmlFreeDoc(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}

#if defined(LIBXML_XPATH_ENABLED)
PyObject *
libxml_xmlXPathEvalExpr(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlXPathParserContextPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlXPathEvalExpr", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlXPathParserContextPtr) PyxmlXPathParserContext_Get(pyobj_ctxt);

    xmlXPathEvalExpr(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

#endif /* defined(LIBXML_XPATH_ENABLED) */
#if defined(LIBXML_UNICODE_ENABLED)
PyObject *
libxml_xmlUCSIsLinearBSyllabary(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    int code;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlUCSIsLinearBSyllabary", &code))
        return(NULL);

    c_retval = xmlUCSIsLinearBSyllabary(code);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

#endif /* defined(LIBXML_UNICODE_ENABLED) */
PyObject *
libxml_xmlParseDocTypeDecl(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, (char *)"O:xmlParseDocTypeDecl", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    xmlParseDocTypeDecl(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlIsBaseChar(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    unsigned int ch;

    if (!PyArg_ParseTuple(args, (char *)"i:xmlIsBaseChar", &ch))
        return(NULL);

    c_retval = xmlIsBaseChar(ch);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

