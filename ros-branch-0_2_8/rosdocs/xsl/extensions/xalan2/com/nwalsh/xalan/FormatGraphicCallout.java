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
 * <p>$Id: FormatGraphicCallout.java,v 1.1 2002/06/13 20:32:18 chorns Exp $</p>
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
 * @version $Id: FormatGraphicCallout.java,v 1.1 2002/06/13 20:32:18 chorns Exp $
 **/

public class FormatGraphicCallout extends FormatCallout {
  String graphicsPath = "";
  String graphicsExt = "";
  int graphicsMax = 0;

  public FormatGraphicCallout(String path, String ext, int max, boolean fo) {
    graphicsPath = path;
    graphicsExt = ext;
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
	  imgName = "external-graphic";
	  imgAttr.addAttribute("", "src", "src", "CDATA",
			       graphicsPath + num + graphicsExt);
	  imgAttr.addAttribute("", "alt", "alt", "CDATA", label);
	} else {
	  ns = "";
	  prefix = "";
	  imgName = "img";
	  imgAttr.addAttribute("", "src", "src", "CDATA",
			       graphicsPath + num + graphicsExt);
	  imgAttr.addAttribute("", "alt", "alt", "CDATA", label);
	}

	startSpan(rtf);
	rtf.startElement(ns, imgName, prefix+imgName, imgAttr);
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
