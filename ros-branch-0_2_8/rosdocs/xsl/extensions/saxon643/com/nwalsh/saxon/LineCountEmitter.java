package com.nwalsh.saxon;

import org.xml.sax.*;
import javax.xml.transform.TransformerException;
import com.icl.saxon.output.*;
import com.icl.saxon.om.*;
import com.icl.saxon.expr.FragmentValue;

/**
 * <p>Saxon extension to count the lines in a result tree fragment.</p>
 *
 * <p>$Id: LineCountEmitter.java,v 1.1 2002/06/13 20:32:16 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://users.iclway.co.uk/mhkay/saxon/">Saxon 6.*</a>
 * implementation to count the number of lines in a result tree
 * fragment.</p>
 *
 * <p>The general design is this: the stylesheets construct a result tree
 * fragment for some verbatim environment. That result tree fragment
 * is "replayed" through the LineCountEmitter; the LineCountEmitter watches
 * characters go by and counts the number of line feeds that it sees.
 * That number is then returned.</p>
 *
 * <p><b>Change Log:</b></p>
 * <dl>
 * <dt>1.0</dt>
 * <dd><p>Initial release.</p></dd>
 * </dl>
 *
 * @see Verbatim
 *
 * @author Norman Walsh
 * <a href="mailto:ndw@nwalsh.com">ndw@nwalsh.com</a>
 *
 * @version $Id: LineCountEmitter.java,v 1.1 2002/06/13 20:32:16 chorns Exp $
 *
 */
public class LineCountEmitter extends com.icl.saxon.output.Emitter {
  /** The number of lines seen. */
  protected int numLines = 0;

  /** Construct a new LineCountEmitter. */
  public LineCountEmitter() {
    numLines = 0;
  }

  /** Reset the number of lines. */
  public void reset() {
    numLines = 0;
  }

  /** Return the number of lines. */
  public int lineCount() {
    return numLines;
  }

  /** Process characters. */
  public void characters(char[] chars, int start, int len)
    throws javax.xml.transform.TransformerException {

    if (numLines == 0) {
      // If there are any characters at all, there's at least one line
      numLines++;
    }

    for (int count = start; count < start+len; count++) {
      if (chars[count] == '\n') {
	numLines++;
      }
    }
  }

  /** Discarded. */
  public void comment(char[] chars, int start, int length)
    throws javax.xml.transform.TransformerException {
    // nop
  }

  /** Discarded. */
  public void endDocument()
    throws javax.xml.transform.TransformerException {
    // nop
  }

  /** Discarded. */
  public void endElement(int nameCode)
    throws javax.xml.transform.TransformerException {
    // nop
  }

  /** Discarded. */
  public void processingInstruction(java.lang.String name,
				    java.lang.String data)
    throws javax.xml.transform.TransformerException {
    // nop
  }

  /** Discarded. */
  public void setDocumentLocator(org.xml.sax.Locator locator) {
    // nop
  }

  /** Discarded. */
  public void setEscaping(boolean escaping)
    throws javax.xml.transform.TransformerException {
    // nop
  }

  /** Discarded. */
  public void setNamePool(NamePool namePool) {
    // nop
  }

  /** Discarded. */
  public void setUnparsedEntity(java.lang.String name, java.lang.String uri)
    throws javax.xml.transform.TransformerException {
    // nop
  }

  /** Discarded. */
  public void setWriter(java.io.Writer writer) {
    // nop
  }

  /** Discarded. */
  public void startDocument()
    throws javax.xml.transform.TransformerException {
    // nop
  }

  /** Discarded. */
  public void startElement(int nameCode,
		    org.xml.sax.Attributes attributes,
		    int[] namespaces, int nscount)
    throws javax.xml.transform.TransformerException {
    // nop
  }
}
