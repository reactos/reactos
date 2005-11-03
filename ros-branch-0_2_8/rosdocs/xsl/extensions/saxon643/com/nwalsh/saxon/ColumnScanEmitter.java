package com.nwalsh.saxon;

import org.xml.sax.*;
import javax.xml.transform.TransformerException;
import com.icl.saxon.output.*;
import com.icl.saxon.om.*;
import com.icl.saxon.expr.FragmentValue;

/**
 * <p>Saxon extension to scan the column widthsin a result tree fragment.</p>
 *
 * <p>$Id: ColumnScanEmitter.java,v 1.1 2002/06/13 20:32:15 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://users.iclway.co.uk/mhkay/saxon/">Saxon 6.*</a>
 * implementation to scan the column widths in a result tree
 * fragment.</p>
 *
 * <p>The general design is this: the stylesheets construct a result tree
 * fragment for some colgroup environment. That result tree fragment
 * is "replayed" through the ColumnScanEmitter; the ColumnScanEmitter watches
 * the cols go by and extracts the column widths that it sees. These
 * widths are then made available.</p>
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
 * @version $Id: ColumnScanEmitter.java,v 1.1 2002/06/13 20:32:15 chorns Exp $
 *
 */
public class ColumnScanEmitter extends com.icl.saxon.output.Emitter {
  /** The number of columns seen. */
  protected int numColumns = 0;
  protected String width[] = new String[5];
  protected NamePool namePool = null;

  /** The FO namespace name. */
  protected static String foURI = "http://www.w3.org/1999/XSL/Format";

  /** Construct a new ColumnScanEmitter. */
  public ColumnScanEmitter(NamePool namePool) {
    numColumns = 0;
    this.namePool = namePool;
  }

  /** Return the number of columns. */
  public int columnCount() {
    return numColumns;
  }

  /** Return the number of columns. */
  public String[] columnWidths() {
    return width;
  }

  /** Discarded. */
  public void characters(char[] chars, int start, int len)
    throws TransformerException {
    // nop
  }

  /** Discarded. */
  public void comment(char[] chars, int start, int length)
    throws TransformerException {
    // nop
  }

  /** Discarded. */
  public void endDocument()
    throws TransformerException {
    // nop
  }

  /** Discarded. */
  public void endElement(int nameCode)
    throws TransformerException {
    // nop
  }

  /** Discarded. */
  public void processingInstruction(java.lang.String name,
				    java.lang.String data)
    throws TransformerException {
    // nop
  }

  /** Discarded. */
  public void setDocumentLocator(org.xml.sax.Locator locator) {
    // nop
  }

  /** Discarded. */
  public void setEscaping(boolean escaping)
    throws TransformerException {
    // nop
  }

  /** Discarded. */
  public void setNamePool(NamePool namePool) {
    // nop
  }

  /** Discarded. */
  public void setUnparsedEntity(java.lang.String name, java.lang.String uri)
    throws TransformerException {
    // nop
  }

  /** Discarded. */
  public void setWriter(java.io.Writer writer) {
    // nop
  }

  /** Discarded. */
  public void startDocument()
    throws TransformerException {
    // nop
  }

  /** Examine for column info. */
  public void startElement(int nameCode,
		    org.xml.sax.Attributes attributes,
		    int[] namespaces, int nscount)
    throws TransformerException {

    int thisFingerprint = namePool.getFingerprint(nameCode);
    int colFingerprint = namePool.getFingerprint("", "col");
    int foColFingerprint = namePool.getFingerprint(foURI, "table-column");

    if (thisFingerprint == colFingerprint
	|| thisFingerprint == foColFingerprint) {
      if (numColumns >= width.length) {
	String newWidth[] = new String[width.length+10];
	for (int count = 0; count < width.length; count++) {
	  newWidth[count] = width[count];
	}
	width = newWidth;
      }

      if (thisFingerprint == colFingerprint) {
	if (attributes.getValue("width") == null) {
	  width[numColumns++] = "1*";
	} else {
	  width[numColumns++] = attributes.getValue("width");
	}
      } else {
	if (attributes.getValue("column-width") == null) {
	  width[numColumns++] = "1*";
	} else {
	  width[numColumns++] = attributes.getValue("column-width");
	}
      }
    }
  }
}


