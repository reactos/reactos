package com.nwalsh.xalan;

import org.w3c.dom.*;
import org.apache.xml.utils.DOMBuilder;
import com.nwalsh.xalan.Callout;
import org.apache.xml.utils.AttList;

/**
 * <p>Utility class for the Verbatim extension (ignore this).</p>
 *
 * <p>$Id: FormatTextCallout.java,v 1.1 2002/06/13 20:32:18 chorns Exp $</p>
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
 * @version $Id: FormatTextCallout.java,v 1.1 2002/06/13 20:32:18 chorns Exp $
 **/

public class FormatTextCallout extends FormatCallout {
  public FormatTextCallout(boolean fo) {
    stylesheetFO = fo;
  }

  public void formatCallout(DOMBuilder rtf,
			    Callout callout) {
    formatTextCallout(rtf, callout);
  }
}
