// Text - Xalan extension element for inserting text

package com.nwalsh.xalan;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.net.URL;
import java.net.MalformedURLException;

import org.xml.sax.SAXException;
import org.xml.sax.AttributeList;
import org.xml.sax.ContentHandler;

import org.w3c.dom.*;
import org.apache.xerces.dom.*;

import org.apache.xpath.objects.XObject;
import org.apache.xpath.XPath;
import org.apache.xpath.NodeSet;
import org.apache.xalan.extensions.XSLProcessorContext;
import org.apache.xalan.transformer.TransformerImpl;
import org.apache.xalan.templates.StylesheetRoot;
import org.apache.xalan.templates.ElemExtensionCall;
import org.apache.xalan.templates.OutputProperties;
import org.apache.xalan.res.XSLTErrorResources;

import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.TransformerException;

/**
 * <p>Xalan extension element for inserting text
 *
 * <p>$Id: Text.java,v 1.1 2002/06/13 20:32:17 chorns Exp $</p>
 *
 * <p>Copyright (C) 2001 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://xml.apache.org/xalan/">Xalan</a>
 * extension element for inserting text into a result tree.</p>
 *
 * <p><b>Change Log:</b></p>
 * <dl>
 * <dt>1.0</dt>
 * <dd><p>Initial release.</p></dd>
 * </dl>
 *
 * @author Norman Walsh
 * <a href="mailto:ndw@nwalsh.com">ndw@nwalsh.com</a>
 *
 * @version $Id: Text.java,v 1.1 2002/06/13 20:32:17 chorns Exp $
 *
 */
public class Text {
  /**
   * <p>Constructor for Text</p>
   *
   * <p>Does nothing.</p>
   */
  public Text() {
  }

  public NodeList insertfile(XSLProcessorContext context,
			     ElemExtensionCall elem)
    throws MalformedURLException,
           FileNotFoundException,
           IOException,
	   TransformerException {
    String href = getFilename(context, elem);

    NodeSet textNodes = new NodeSet();
    Document textDoc = DOMImplementationImpl.getDOMImplementation().createDocument(null, "tmpDoc", null);

    URL fileURL = null;

    try {
      try {
	fileURL = new URL(href);
      } catch (MalformedURLException e1) {
	try {
	  fileURL = new URL("file:" + href);
	} catch (MalformedURLException e2) {
	  System.out.println("Cannot open " + href);
	  return null;
	}
      }

      InputStreamReader isr = new InputStreamReader(fileURL.openStream());
      BufferedReader is = new BufferedReader(isr);

      char chars[] = new char[4096];
      int len = 0;
      while ((len = is.read(chars)) > 0) {
	String s = new String(chars, 0, len);
	// Does it matter that this produces multiple, adjacent text
	// nodes? I don't think so...
	textNodes.addNode(textDoc.createTextNode(s));
      }
      is.close();
    } catch (Exception e) {
      System.out.println("Cannot read " + href);
    }

    return textNodes;
  }

  private String getFilename(XSLProcessorContext context, ElemExtensionCall elem)
    throws java.net.MalformedURLException,
	   java.io.FileNotFoundException,
	   java.io.IOException,
	   javax.xml.transform.TransformerException {

    String fileName;

    fileName = ((ElemExtensionCall)elem).getAttribute ("href",
						       context.getContextNode(),
						       context.getTransformer());

    if(fileName == null) {
      context.getTransformer().getMsgMgr().error(elem,
						 "No 'href' on text, or not a filename");
    }

    return fileName;
  }
}
