package com.nwalsh.saxon;

import org.xml.sax.SAXException;
import org.w3c.dom.*;

import javax.xml.transform.TransformerException;

import com.icl.saxon.om.NamePool;
import com.icl.saxon.output.Emitter;
import com.icl.saxon.tree.AttributeCollection;

import com.nwalsh.saxon.Callout;

/**
 * <p>Utility class for the Verbatim extension (ignore this).</p>
 *
 * <p>$Id: FormatCallout.java,v 1.1 2002/06/13 20:32:17 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000, 2001 Norman Walsh.</p>
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
 * @see Verbatim
 *
 * @version $Id: FormatCallout.java,v 1.1 2002/06/13 20:32:17 chorns Exp $
 **/

public abstract class FormatCallout {
  protected static final String foURI = "http://www.w3.org/1999/XSL/Format";
  protected static final String xhURI = "http://www.w3.org/1999/xhtml";
  protected boolean foStylesheet = false;
  protected NamePool namePool = null;

  public FormatCallout(NamePool nPool, boolean fo) {
    namePool = nPool;
    foStylesheet = fo;
  }

  public String areaLabel(Element area) {
    String label = null;

    if (area.hasAttribute("label")) {
      // If this area has a label, use it
      label = area.getAttribute("label");
    } else {
      // Otherwise, if its parent is an areaset and it has a label, use that
      Element parent = (Element) area.getParentNode();
      if (parent != null
	  && parent.getLocalName().equalsIgnoreCase("areaset")
	  && parent.hasAttribute("label")) {
	label = parent.getAttribute("label");
      }
    }

    return label;
  }

  public void startSpan(Emitter rtf)
    throws TransformerException {
    // no point in doing this for FO, right?
    if (!foStylesheet && namePool != null) {
      int spanName = namePool.allocate("", "", "span");
      AttributeCollection spanAttr = new AttributeCollection(namePool);
      int namespaces[] = new int[1];
      spanAttr.addAttribute("", "", "class", "CDATA", "co");
      rtf.startElement(spanName, spanAttr, namespaces, 0);
    }
  }

  public void endSpan(Emitter rtf)
    throws TransformerException {
    // no point in doing this for FO, right?
    if (!foStylesheet && namePool != null) {
      int spanName = namePool.allocate("", "", "span");
      rtf.endElement(spanName);
    }
  }

  public void formatTextCallout(Emitter rtfEmitter,
				Callout callout) {
    Element area = callout.getArea();
    int num = callout.getCallout();
    String userLabel = areaLabel(area);
    String label = "(" + num + ")";

    if (userLabel != null) {
      label = userLabel;
    }

    char chars[] = label.toCharArray();

    try {
      startSpan(rtfEmitter);
      rtfEmitter.characters(chars, 0, label.length());
      endSpan(rtfEmitter);
    } catch (TransformerException e) {
      System.out.println("Transformer Exception in formatTextCallout");
    }
  }

  public abstract void formatCallout(Emitter rtfEmitter,
				     Callout callout);
}

