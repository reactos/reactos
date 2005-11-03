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
 * <p>$Id: FormatUnicodeCallout.java,v 1.1 2002/06/13 20:32:17 chorns Exp $</p>
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
 * @version $Id: FormatUnicodeCallout.java,v 1.1 2002/06/13 20:32:17 chorns Exp $
 **/

public class FormatUnicodeCallout extends FormatCallout {
  int unicodeMax = 0;
  int unicodeStart = 0;
  String unicodeFont = "";

  public FormatUnicodeCallout(NamePool nPool,
			      String font,
			      int start,
			      int max,
			      boolean fo) {
    super(nPool, fo);
    unicodeFont = font;
    unicodeMax = max;
    unicodeStart = start;
  }

  public void formatCallout(Emitter rtfEmitter,
			    Callout callout) {
    Element area = callout.getArea();
    int num = callout.getCallout();
    String userLabel = areaLabel(area);
    String label = "";

    if (userLabel != null) {
      label = userLabel;
    }

    try {
      if (userLabel == null && num <= unicodeMax) {
	int inName = 0;
	AttributeCollection inAttr = null;
	int namespaces[] = new int[1];

	if (!unicodeFont.equals("")) {
	  if (foStylesheet) {
	    inName = namePool.allocate("fo", foURI, "inline");
	    inAttr = new AttributeCollection(namePool);
	    inAttr.addAttribute("", "", "font-family", "CDATA", unicodeFont);
	  } else {
	    inName = namePool.allocate("", "", "font");
	    inAttr = new AttributeCollection(namePool);
	    inAttr.addAttribute("", "", "face", "CDATA", unicodeFont);
	  }

	  startSpan(rtfEmitter);
	  rtfEmitter.startElement(inName, inAttr, namespaces, 0);
	}

	char chars[] = new char[1];
	chars[0] = (char) (unicodeStart + num - 1);
	rtfEmitter.characters(chars, 0, 1);

	if (!unicodeFont.equals("")) {
	  rtfEmitter.endElement(inName);
	  endSpan(rtfEmitter);
	}
      } else {
	formatTextCallout(rtfEmitter, callout);
      }
    } catch (TransformerException e) {
      System.out.println("Transformer Exception in graphic formatCallout");
    }
  }
}
