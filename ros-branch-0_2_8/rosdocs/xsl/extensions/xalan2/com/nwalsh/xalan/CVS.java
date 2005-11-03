package com.nwalsh.xalan;

import java.io.*;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Date;
import java.util.Locale;
import java.util.TimeZone;
import java.text.DateFormat;
import java.text.ParseException;

/**
 * <p>Saxon extension to convert CVS date strings into local time</p>
 *
 * <p>$Id: CVS.java,v 1.1 2002/06/13 20:32:18 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://users.iclway.co.uk/mhkay/saxon/">Saxon</a>
 * extension to turn the CVS date strings, which are UTC:</p>
 *
 * <pre>&#36;Date: 2000/11/09 02:34:20 &#36;</pre>
 *
 * <p>into legibly formatted local time:</p>
 *
 * <pre>Wed Nov 08 18:34:20 PST 2000</pre>
 *
 * <p>(I happened to be in California when I wrote this documentation.)</p>

 * <p><b>Change Log:</b></p>
 * <dl>
 * <dt>1.0</dt>
 * <dd><p>Initial release.</p></dd>
 * </dl>
 *
 * @author Norman Walsh
 * <a href="mailto:ndw@nwalsh.com">ndw@nwalsh.com</a>
 *
 * @version $Id: CVS.java,v 1.1 2002/06/13 20:32:18 chorns Exp $
 *
 */
public class CVS {
  /**
   * <p>Constructor for CVS</p>
   *
   * <p>All of the methods are static, so the constructor does nothing.</p>
   */
  public CVS() {
  }

  /**
   * <p>Convert a CVS date string into local time.</p>
   *
   * @param cvsDate The CVS date string.
   *
   * @return The date, converted to local time and reformatted.
   */
  public String localTime (String cvsDate) {
    // A cvsDate has the following form "$Date: 2002/06/13 20:32:18 $"
    if (!cvsDate.startsWith("$Date: ")) {
      return cvsDate;
    }

    String yrS = cvsDate.substring(7,11);
    String moS = cvsDate.substring(12,14);
    String daS = cvsDate.substring(15,17);
    String hrS = cvsDate.substring(18,20);
    String miS = cvsDate.substring(21,23);
    String seS = cvsDate.substring(24,26);

    TimeZone tz = TimeZone.getTimeZone("GMT+0");
    GregorianCalendar gmtCal = new GregorianCalendar(tz);

    try {
      gmtCal.set(Integer.parseInt(yrS),
		 Integer.parseInt(moS)-1,
		 Integer.parseInt(daS),
		 Integer.parseInt(hrS),
		 Integer.parseInt(miS),
		 Integer.parseInt(seS));
    } catch (NumberFormatException e) {
      // nop
    }

    Date d = gmtCal.getTime();

    return d.toString();
  }
}
