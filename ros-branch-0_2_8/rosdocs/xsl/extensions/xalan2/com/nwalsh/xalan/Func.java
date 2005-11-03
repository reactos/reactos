// Func - Xalann extension function test

package com.nwalsh.xalan;

import org.xml.sax.SAXException;
import org.xml.sax.AttributeList;
import org.xml.sax.ContentHandler;

import org.w3c.dom.*;
import org.w3c.dom.traversal.NodeIterator;
import org.apache.xerces.dom.*;

import org.apache.xpath.objects.XObject;
import org.apache.xpath.objects.XRTreeFrag;
import org.apache.xpath.XPath;
import org.apache.xpath.NodeSet;
import org.apache.xalan.extensions.XSLProcessorContext;
import org.apache.xalan.extensions.ExpressionContext;
import org.apache.xalan.transformer.TransformerImpl;
import org.apache.xalan.templates.StylesheetRoot;
import org.apache.xalan.templates.ElemExtensionCall;
import org.apache.xalan.templates.OutputProperties;
import org.apache.xalan.res.XSLTErrorResources;

import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.TransformerException;

public class Func {
  public Func() {
  }

  public DocumentFragment doSomething(NodeIterator rtf) {
    System.out.println("Got here 2: " + rtf);

    DocumentFragment df = (DocumentFragment) rtf.nextNode();
    Element node = (Element) df.getFirstChild();

    System.out.println("node=" + node);
    System.out.println("namesp uri: " + node.getNamespaceURI());
    System.out.println("local name: " + node.getLocalName());

    return df;
  }

  public DocumentFragment doSomething(DocumentFragment rtf) {
    System.out.println("Got here: " + rtf);

    return rtf;
    /*
    Element node = (Element) rtf.getFirstChild();

    System.out.println("node=" + node);
    System.out.println("namesp uri: " + node.getNamespaceURI());
    System.out.println("local name: " + node.getLocalName());

    return rtf;
    */
  }
}
