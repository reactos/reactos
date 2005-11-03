// Verbatim.java - Xalan extensions supporting DocBook verbatim environments

package com.nwalsh.xalan;

import java.util.Hashtable;
import org.xml.sax.*;
import org.xml.sax.helpers.AttributesImpl;
import org.w3c.dom.*;
import org.w3c.dom.traversal.NodeIterator;

import javax.xml.transform.TransformerException;

import org.apache.xpath.objects.XObject;
import org.apache.xpath.XPathContext;
import org.apache.xalan.extensions.ExpressionContext;
import org.apache.xml.utils.DOMBuilder;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.apache.xml.utils.QName;
import org.apache.xpath.DOMHelper;
import org.apache.xml.utils.AttList;

/**
 * <p>Xalan extensions supporting Tables</p>
 *
 * <p>$Id: Table.java,v 1.1 2002/06/13 20:32:18 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://xml.apache.org/xalan/">Xalan</a>
 * implementation of some code to adjust CALS Tables to HTML
 * Tables.</p>
 *
 * <p><b>Column Widths</b></p>
 * <p>The <tt>adjustColumnWidths</tt> method takes a result tree
 * fragment (assumed to contain the colgroup of an HTML Table)
 * and returns the result tree fragment with the column widths
 * adjusted to HTML terms.</p>
 *
 * <p><b>Convert Lengths</b></p>
 * <p>The <tt>convertLength</tt> method takes a length specification
 * of the form 9999.99xx (where "xx" is a unit) and returns that length
 * as an integral number of pixels. For convenience, percentage lengths
 * are returned unchanged.</p>
 * <p>The recognized units are: inches (in), centimeters (cm),
 * millimeters (mm), picas (pc, 1pc=12pt), points (pt), and pixels (px).
 * A number with no units is assumed to be pixels.</p>
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
 * @version $Id: Table.java,v 1.1 2002/06/13 20:32:18 chorns Exp $
 *
 */
public class Table {
  /** The number of pixels per inch */
  private static int pixelsPerInch = 96;

  /** The hash used to associate units with a length in pixels. */
  protected static Hashtable unitHash = null;

  /** The FO namespace name. */
  protected static String foURI = "http://www.w3.org/1999/XSL/Format";

  /**
   * <p>Constructor for Verbatim</p>
   *
   * <p>All of the methods are static, so the constructor does nothing.</p>
   */
  public Table() {
  }

  /** Initialize the internal hash table with proper values. */
  protected static void initializeHash() {
    unitHash = new Hashtable();
    unitHash.put("in", new Float(pixelsPerInch));
    unitHash.put("cm", new Float(pixelsPerInch / 2.54));
    unitHash.put("mm", new Float(pixelsPerInch / 25.4));
    unitHash.put("pc", new Float((pixelsPerInch / 72) * 12));
    unitHash.put("pt", new Float(pixelsPerInch / 72));
    unitHash.put("px", new Float(1));
  }

  /** Set the pixels-per-inch value. Only positive values are legal. */
  public static void setPixelsPerInch(int value) {
    if (value > 0) {
      pixelsPerInch = value;
      initializeHash();
    }
  }

  /** Return the current pixels-per-inch value. */
  public int getPixelsPerInch() {
    return pixelsPerInch;
  }

  /**
   * <p>Convert a length specification to a number of pixels.</p>
   *
   * <p>The specified length should be of the form [+/-]999.99xx,
   * where xx is a valid unit.</p>
   */
  public static int convertLength(String length) {
    // The format of length should be 999.999xx
    int sign = 1;
    String digits = "";
    String units = "";
    char lench[] = length.toCharArray();
    float flength = 0;
    boolean done = false;
    int pos = 0;
    float factor = 1;
    int pixels = 0;

    if (unitHash == null) {
      initializeHash();
    }

    if (lench[pos] == '+' || lench[pos] == '-') {
      if (lench[pos] == '-') {
	sign = -1;
      }
      pos++;
    }

    while (!done) {
      if (pos >= lench.length) {
	done = true;
      } else {
	if ((lench[pos] > '9' || lench[pos] < '0') && lench[pos] != '.') {
	  done = true;
	  units = length.substring(pos);
	} else {
	  digits += lench[pos++];
	}
      }
    }

    try {
      flength = Float.parseFloat(digits);
    } catch (NumberFormatException e) {
      System.out.println(digits + " is not a number; 1 used instead.");
      flength = 1;
    }

    Float f = null;

    if (!units.equals("")) {
      f = (Float) unitHash.get(units);
      if (f == null) {
	System.out.println(units + " is not a known unit; 1 used instead.");
	factor = 1;
      } else {
	factor = f.floatValue();
      }
    } else {
      factor = 1;
    }

    f = new Float(flength * factor);

    pixels = f.intValue() * sign;

    return pixels;
  }

  /**
   * <p>Adjust column widths in an HTML table.</p>
   *
   * <p>The specification of column widths in CALS (a relative width
   * plus an optional absolute width) are incompatible with HTML column
   * widths. This method adjusts CALS column width specifiers in an
   * attempt to produce equivalent HTML specifiers.</p>
   *
   * <p>In order for this method to work, the CALS width specifications
   * should be placed in the "width" attribute of the &lt;col>s within
   * a &lt;colgroup>. Then the colgroup result tree fragment is passed
   * to this method.</p>
   *
   * <p>This method makes use of two parameters from the XSL stylesheet
   * that calls it: <code>nominal.table.width</code> and
   * <code>table.width</code>. The value of <code>nominal.table.width</code>
   * must be an absolute distance. The value of <code>table.width</code>
   * can be either absolute or relative.</p>
   *
   * <p>Presented with a mixture of relative and
   * absolute lengths, the table width is used to calculate
   * appropriate values. If the <code>table.width</code> is relative,
   * the nominal width is used for this calculation.</p>
   *
   * <p>There are three possible combinations of values:</p>
   *
   * <ol>
   * <li>There are no relative widths; in this case the absolute widths
   * are used in the HTML table.</li>
   * <li>There are no absolute widths; in this case the relative widths
   * are used in the HTML table.</li>
   * <li>There are a mixture of absolute and relative widths:
   *   <ol>
   *     <li>If the table width is absolute, all widths become absolute.</li>
   *     <li>If the table width is relative, make all the widths absolute
   *         relative to the nominal table width then turn them all
   *         back into relative widths.</li>
   *   </ol>
   * </li>
   * </ol>
   *
   * @param context The stylesheet context; supplied automatically by Xalan
   * @param rtf The result tree fragment containing the colgroup.
   *
   * @return The result tree fragment containing the adjusted colgroup.
   *
   */

  public DocumentFragment adjustColumnWidths (ExpressionContext context,
					      NodeIterator xalanNI) {

    int nominalWidth = convertLength(Params.getString(context,
						      "nominal.table.width"));
    String tableWidth = Params.getString(context, "table.width");
    String styleType = Params.getString(context, "stylesheet.result.type");
    boolean foStylesheet = styleType.equals("fo");

    DocumentFragment xalanRTF = (DocumentFragment) xalanNI.nextNode();
    Element colgroup = (Element) xalanRTF.getFirstChild();

    // N.B. ...stree.ElementImpl doesn't implement getElementsByTagName()

    Node firstCol = null;
    // If this is an FO tree, there might be no colgroup...
    if (colgroup.getLocalName().equals("colgroup")) {
      firstCol = colgroup.getFirstChild();
    } else {
      firstCol = colgroup;
    }

    // Count the number of columns...
    Node child = firstCol;
    int numColumns = 0;
    while (child != null) {
      if (child.getNodeType() == Node.ELEMENT_NODE
	  && (child.getNodeName().equals("col")
	      || (child.getNamespaceURI().equals(foURI)
		  && child.getLocalName().equals("table-column")))) {
	numColumns++;
      }

      child = child.getNextSibling();
    }

    String widths[] = new String[numColumns];
    Element columns[] = new Element[numColumns];
    int colnum = 0;

    child = firstCol;
    while (child != null) {
      if (child.getNodeType() == Node.ELEMENT_NODE
	  && (child.getNodeName().equals("col")
	      || (child.getNamespaceURI().equals(foURI)
		  && child.getLocalName().equals("table-column")))) {
	Element col = (Element) child;

	columns[colnum] = col;

	if (foStylesheet) {
	  if (col.getAttribute("column-width") == null) {
	    widths[colnum] = "1*";
	  } else {
	    widths[colnum] = col.getAttribute("column-width");
	  }
	} else {
	  if (col.getAttribute("width") == null) {
	    widths[colnum] = "1*";
	  } else {
	    widths[colnum] = col.getAttribute("width");
	  }
	}

	colnum++;
      }
      child = child.getNextSibling();
    }

    float relTotal = 0;
    float relParts[] = new float[numColumns];

    float absTotal = 0;
    float absParts[] = new float[numColumns];

    for (int count = 0; count < numColumns; count++) {
      String width = widths[count];
      int pos = width.indexOf("*");
      if (pos >= 0) {
	String relPart = width.substring(0, pos);
	String absPart = width.substring(pos+1);

	try {
	  float rel = Float.parseFloat(relPart);
	  relTotal += rel;
	  relParts[count] = rel;
	} catch (NumberFormatException e) {
	  System.out.println(relPart + " is not a valid relative unit.");
	}

	int pixels = 0;
	if (absPart != null && !absPart.equals("")) {
	  pixels = convertLength(absPart);
	}

	absTotal += pixels;
	absParts[count] = pixels;
      } else {
	relParts[count] = 0;

	int pixels = 0;
	if (width != null && !width.equals("")) {
	  pixels = convertLength(width);
	}

	absTotal += pixels;
	absParts[count] = pixels;
      }
    }

    // Ok, now we have the relative widths and absolute widths in
    // two parallel arrays.
    //
    // - If there are no relative widths, output the absolute widths
    // - If there are no absolute widths, output the relative widths
    // - If there are a mixture of relative and absolute widths,
    //   - If the table width is absolute, turn these all into absolute
    //     widths.
    //   - If the table width is relative, turn these all into absolute
    //     widths in the nominalWidth and then turn them back into
    //     percentages.

    if (relTotal == 0) {
      for (int count = 0; count < numColumns; count++) {
	Float f = new Float(absParts[count]);
	if (foStylesheet) {
	  int pixels = f.intValue();
	  float inches = (float) pixels / pixelsPerInch;
	  widths[count] = inches + "in";
	} else {
	  widths[count] = Integer.toString(f.intValue());
	}
      }
    } else if (absTotal == 0) {
      for (int count = 0; count < numColumns; count++) {
	float rel = relParts[count] / relTotal * 100;
	Float f = new Float(rel);
	widths[count] = Integer.toString(f.intValue()) + "%";
      }
    } else {
      int pixelWidth = nominalWidth;

      if (tableWidth.indexOf("%") <= 0) {
	pixelWidth = convertLength(tableWidth);
      }

      if (pixelWidth <= absTotal) {
	System.out.println("Table is wider than table width.");
      } else {
	pixelWidth -= absTotal;
      }

      absTotal = 0;
      for (int count = 0; count < numColumns; count++) {
	float rel = relParts[count] / relTotal * pixelWidth;
	relParts[count] = rel + absParts[count];
	absTotal += rel + absParts[count];
      }

      if (tableWidth.indexOf("%") <= 0) {
	for (int count = 0; count < numColumns; count++) {
	  Float f = new Float(relParts[count]);
	  if (foStylesheet) {
	    int pixels = f.intValue();
	    float inches = (float) pixels / pixelsPerInch;
	    widths[count] = inches + "in";
	  } else {
	    widths[count] = Integer.toString(f.intValue());
	  }
	}
      } else {
	for (int count = 0; count < numColumns; count++) {
	  float rel = relParts[count] / absTotal * 100;
	  Float f = new Float(rel);
	  widths[count] = Integer.toString(f.intValue()) + "%";
	}
      }
    }

    // Now rebuild the colgroup with the right widths

    DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
    DocumentBuilder docBuilder = null;

    try {
      docBuilder = docFactory.newDocumentBuilder();
    } catch (ParserConfigurationException e) {
      System.out.println("PCE!");
      return xalanRTF;
    }
    Document doc = docBuilder.newDocument();
    DocumentFragment df = doc.createDocumentFragment();
    DOMBuilder rtf = new DOMBuilder(doc, df);

    try {
      String ns = colgroup.getNamespaceURI();
      String localName = colgroup.getLocalName();
      String name = colgroup.getTagName();

      if (colgroup.getLocalName().equals("colgroup")) {
	rtf.startElement(ns, localName, name,
			 copyAttributes(colgroup));
      }

      for (colnum = 0; colnum < numColumns; colnum++) {
	Element col = columns[colnum];

	NamedNodeMap domAttr = col.getAttributes();

	AttributesImpl attr = new AttributesImpl();
	for (int acount = 0; acount < domAttr.getLength(); acount++) {
	  Node a = domAttr.item(acount);
	  String a_ns = a.getNamespaceURI();
	  String a_localName = a.getLocalName();

	  if ((foStylesheet && !a_localName.equals("column-width"))
	      || !a_localName.equalsIgnoreCase("width")) {
	    attr.addAttribute(a.getNamespaceURI(),
			      a.getLocalName(),
			      a.getNodeName(),
			      "CDATA",
			      a.getNodeValue());
	  }
	}

	if (foStylesheet) {
	  attr.addAttribute("", "column-width", "column-width", "CDATA", widths[colnum]);
	} else {
	  attr.addAttribute("", "width", "width", "CDATA", widths[colnum]);
	}

	rtf.startElement(col.getNamespaceURI(),
			 col.getLocalName(),
			 col.getTagName(),
			 attr);
	rtf.endElement(col.getNamespaceURI(),
		       col.getLocalName(),
		       col.getTagName());
      }

      if (colgroup.getLocalName().equals("colgroup")) {
	rtf.endElement(ns, localName, name);
      }
    } catch (SAXException se) {
      System.out.println("SE!");
      return xalanRTF;
    }

    return df;
  }

  private Attributes copyAttributes(Element node) {
    AttributesImpl attrs = new AttributesImpl();
    NamedNodeMap nnm = node.getAttributes();
    for (int count = 0; count < nnm.getLength(); count++) {
      Attr attr = (Attr) nnm.item(count);
      String name = attr.getName();
      if (name.startsWith("xmlns:") || name.equals("xmlns")) {
	// Skip it; (don't ya just love it!!)
      } else {
	attrs.addAttribute(attr.getNamespaceURI(), attr.getName(),
			   attr.getName(), "CDATA", attr.getValue());
      }
    }
    return attrs;
  }
}
