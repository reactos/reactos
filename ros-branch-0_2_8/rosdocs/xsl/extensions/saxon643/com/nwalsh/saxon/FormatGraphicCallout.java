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
 * <p>$Id: FormatGraphicCallout.java,v 1.1 2002/06/13 20:32:17 chorns Exp $</p>
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
 * @version $Id: FormatGraphicCallout.java,v 1.1 2002/06/13 20:32:17 chorns Exp $
 **/

public class FormatGraphicCallout extends FormatCallout {
  String graphicsPath = "";
  String graphicsExt = "";
  int graphicsMax = 0;

  public FormatGraphicCallout(NamePool nPool, String path, String ext, int max, boolean fo) {
    super(nPool, fo);
    graphicsPath = path;
    graphicsExt = ext;
    graphicsMax = max;
  }

  public void formatCallout(Emitter rtfEmitter,
			    Callout callout) {
    Element area = callout.getArea();
    int num = callout.getCallout();
    String userLabel = areaLabel(area);
    String label = "(" + num + ")";

    if (userLabel != null) {
      label = userLabel;
    }

    try {
      if (userLabel == null && num <= graphicsMax) {
	int imgName = 0;
	AttributeCollection imgAttr = null;
	int namespaces[] = new int[1];

	if (foStylesheet) {
	  imgName = namePool.allocate("fo", foURI, "external-graphic");
	  imgAttr = new AttributeCollection(namePool);
	  imgAttr.addAttribute("", "", "src", "CDATA",
			       graphicsPath + num + graphicsExt);
	} else {
	  imgName = namePool.allocate("", "", "img");
	  imgAttr = new AttributeCollection(namePool);
	  imgAttr.addAttribute("", "", "src", "CDATA",
			       graphicsPath + num + graphicsExt);
	  imgAttr.addAttribute("", "", "alt", "CDATA", label);
	}

	startSpan(rtfEmitter);
	rtfEmitter.startElement(imgName, imgAttr, namespaces, 0);
	rtfEmitter.endElement(imgName);
	endSpan(rtfEmitter);
      } else {
	formatTextCallout(rtfEmitter, callout);
      }
    } catch (TransformerException e) {
      System.out.println("Transformer Exception in graphic formatCallout");
    }
  }
}
