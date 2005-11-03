// Text - Saxon extension element for inserting text

package com.nwalsh.saxon;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.InputStream;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.net.URL;
import java.net.MalformedURLException;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerConfigurationException;
import com.icl.saxon.*;
import com.icl.saxon.style.*;
import com.icl.saxon.expr.*;
import com.icl.saxon.output.*;
import org.xml.sax.AttributeList;

/**
 * <p>Saxon extension element for inserting text
 *
 * <p>$Id: Text.java,v 1.1 2002/06/13 20:32:16 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://users.iclway.co.uk/mhkay/saxon/">Saxon</a>
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
 * @version $Id: Text.java,v 1.1 2002/06/13 20:32:16 chorns Exp $
 *
 */
public class Text extends StyleElement {
  /**
   * <p>Constructor for Text</p>
   *
   * <p>Does nothing.</p>
   */
  public Text() {
  }

  /**
   * <p>Is this element an instruction?</p>
   *
   * <p>Yes, it is.</p>
   *
   * @return true
   */
  public boolean isInstruction() {
    return true;
  }

    /**
    * <p>Can this element contain a template-body?</p>
    *
    * <p>Yes, it can, but only so that it can contain xsl:fallback.</p>
    *
    * @return true
    */
  public boolean mayContainTemplateBody() {
    return true;
  }

  /**
   * <p>Validate the arguments</p>
   *
   * <p>The element must have an href attribute.</p>
   */
  public void prepareAttributes() throws TransformerConfigurationException {
    // Get mandatory href attribute
    String fnAtt = getAttribute("href");
    if (fnAtt == null) {
      reportAbsence("href");
    }
  }

  /** Validate that the element occurs in a reasonable place. */
  public void validate() throws TransformerConfigurationException {
    checkWithinTemplate();
  }

  /**
   * <p>Insert the text of the file into the result tree</p>
   *
   * <p>Processing this element inserts the contents of the URL named
   * by the href attribute into the result tree as plain text.</p>
   *
   */
  public void process( Context context ) throws TransformerException {
    Outputter out = context.getOutputter();

    String hrefAtt = getAttribute("href");
    Expression hrefExpr = makeAttributeValueTemplate(hrefAtt);
    String href = hrefExpr.evaluateAsString(context);
    URL fileURL = null;

    try {
      try {
	fileURL = new URL(href);
      } catch (MalformedURLException e1) {
	try {
	  fileURL = new URL("file:" + href);
	} catch (MalformedURLException e2) {
	  System.out.println("Cannot open " + href);
	  return;
	}
      }

      InputStreamReader isr = new InputStreamReader(fileURL.openStream());
      BufferedReader is = new BufferedReader(isr);

      char chars[] = new char[4096];
      int len = 0;
      while ((len = is.read(chars)) > 0) {
	out.writeContent(chars, 0, len);
      }
      is.close();
    } catch (Exception e) {
      System.out.println("Cannot read " + href);
    }
  }
}
