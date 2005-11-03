package com.nwalsh.xalan;

import org.xml.sax.helpers.AttributesImpl;
import org.xml.sax.SAXException;
import org.w3c.dom.*;
import org.apache.xml.utils.DOMBuilder;
import com.nwalsh.xalan.Callout;
import org.apache.xml.utils.AttList;

/**
 * <p>Utility class for the Verbatim extension (ignore this).</p>
 *
 * <p>$Id: FormatDingbatCallout.java,v 1.1 2002/06/13 20:32:18 chorns Exp $</p>
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
 * @version $Id: FormatDingbatCallout.java,v 1.1 2002/06/13 20:32:18 chorns Exp $
 **/

public class FormatDingbatCallout extends FormatCallout {
  int graphicsMax = 0;

  public FormatDingbatCallout(int max, boolean fo) {
    graphicsMax = max;
    stylesheetFO = fo;
  }

  public void formatCallout(DOMBuilder rtf,
			    Callout callout) {
    Element area = callout.getArea();
    int num = callout.getCallout();
    String label = areaLabel(area);

    try {
      if (label == null && num <= graphicsMax) {
	AttributesImpl imgAttr = new AttributesImpl();
	String ns = "";
	String prefix = "";
	String imgName = "";

	if (stylesheetFO) {
	  ns = foURI;
	  prefix = "fo:"; // FIXME: this could be a problem...
	  imgName = "inline";
	  imgAttr.addAttribute("", "font-family", "font-family", "CDATA",
			       "ZapfDingbats");
	} else {
	  ns = "";
	  prefix = "";
	  imgName = "font";
	  imgAttr.addAttribute("", "face", "face", "CDATA",
			       "ZapfDingbats");
	}

	startSpan(rtf);
	rtf.startElement(ns, imgName, prefix+imgName, imgAttr);

	char chars[] = new char[1];
	chars[0] = (char) (0x2775 + num);
	rtf.characters(chars, 0, 1);

	rtf.endElement(ns, imgName, prefix+imgName);
	endSpan(rtf);
      } else {
	formatTextCallout(rtf, callout);
      }
    } catch (SAXException e) {
      System.out.println("SAX Exception in graphics formatCallout");
    }
  }
}
