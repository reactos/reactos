package com.nwalsh.xalan;

import org.xml.sax.SAXException;
import org.xml.sax.helpers.AttributesImpl;
import org.w3c.dom.*;
import org.apache.xpath.DOMHelper;
import org.apache.xml.utils.DOMBuilder;
import org.apache.xml.utils.AttList;
import com.nwalsh.xalan.Callout;

/**
 * <p>Utility class for the Verbatim extension (ignore this).</p>
 *
 * <p>$Id: FormatCallout.java,v 1.1 2002/06/13 20:32:18 chorns Exp $</p>
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
 * @version $Id: FormatCallout.java,v 1.1 2002/06/13 20:32:18 chorns Exp $
 **/

public abstract class FormatCallout {
  protected static final String foURI = "http://www.w3.org/1999/XSL/Format";
  protected static final String xhURI = "http://www.w3.org/1999/xhtml";
  protected boolean stylesheetFO = false;
  protected DOMHelper dh = null;

  public FormatCallout() {
    //nop;
  }

  public String areaLabel(Element area) {
    NamedNodeMap domAttr = area.getAttributes();
    AttList attr = new AttList(domAttr, dh);
    String label = null;

    if (attr.getValue("label") != null) {
      // If this area has a label, use it
      label = attr.getValue("label");
    } else {
      // Otherwise, if its parent is an areaset and it has a label, use that
      Element parent = (Element) area.getParentNode();
      NamedNodeMap pdomAttr = parent.getAttributes();
      AttList pAttr = new AttList(pdomAttr, dh);
      if (parent != null
	  && parent.getNodeName().equals("areaset")
	  && pAttr.getValue("label") != null) {
	label = pAttr.getValue("label");
      }
    }

    return label;
  }

  public void startSpan(DOMBuilder rtf)
    throws SAXException {
    // no point in doing this for FO, right?
    if (!stylesheetFO) {
      AttributesImpl spanAttr = new AttributesImpl();
      spanAttr.addAttribute("", "class", "class", "CDATA", "co");
      rtf.startElement("", "span", "span", spanAttr);
    }
  }

  public void endSpan(DOMBuilder rtf) 
    throws SAXException {
    // no point in doing this for FO, right?
    if (!stylesheetFO) {
      rtf.endElement("", "span", "span");
    }
  }

  public void formatTextCallout(DOMBuilder rtf,
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
      startSpan(rtf);
      rtf.characters(chars, 0, label.length());
      endSpan(rtf);
    } catch (SAXException e) {
      System.out.println("SAX Exception in text formatCallout");
    }
  }

  public abstract void formatCallout(DOMBuilder rtf,
				     Callout callout);
}

