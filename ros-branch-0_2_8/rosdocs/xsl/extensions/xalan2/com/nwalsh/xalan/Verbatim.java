// Verbatim.java - Xalan extensions supporting DocBook verbatim environments

package com.nwalsh.xalan;

import java.util.Stack;
import java.util.StringTokenizer;

import org.xml.sax.*;
import org.xml.sax.helpers.AttributesImpl;
import org.w3c.dom.*;
import org.w3c.dom.traversal.NodeIterator;
import org.apache.xerces.dom.*;

import org.apache.xpath.objects.XObject;
import org.apache.xpath.XPath;
import org.apache.xpath.XPathContext;
import org.apache.xpath.NodeSet;
import org.apache.xpath.DOMHelper;
import org.apache.xalan.extensions.XSLProcessorContext;
import org.apache.xalan.extensions.ExpressionContext;
import org.apache.xalan.transformer.TransformerImpl;
import org.apache.xalan.templates.StylesheetRoot;
import org.apache.xalan.templates.ElemExtensionCall;
import org.apache.xalan.templates.OutputProperties;
import org.apache.xalan.res.XSLTErrorResources;
import org.apache.xml.utils.DOMBuilder;
import org.apache.xml.utils.AttList;
import org.apache.xml.utils.QName;

import javax.xml.transform.stream.StreamResult;
import javax.xml.transform.TransformerException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import com.nwalsh.xalan.Callout;
import com.nwalsh.xalan.Params;

/**
 * <p>Xalan extensions supporting DocBook verbatim environments</p>
 *
 * <p>$Id: Verbatim.java,v 1.1 2002/06/13 20:32:17 chorns Exp $</p>
 *
 * <p>Copyright (C) 2001 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://xml.apache.org/xalan">Xalan</a>
 * implementation of two features that would be impractical to
 * implement directly in XSLT: line numbering and callouts.</p>
 *
 * <p><b>Line Numbering</b></p>
 * <p>The <tt>numberLines</tt> family of functions takes a result tree
 * fragment (assumed to contain the contents of a formatted verbatim
 * element in DocBook: programlisting, screen, address, literallayout,
 * or synopsis) and returns a result tree fragment decorated with
 * line numbers.</p>
 *
 * <p><b>Callouts</b></p>
 * <p>The <tt>insertCallouts</tt> family of functions takes an
 * <tt>areaspec</tt> and a result tree fragment
 * (assumed to contain the contents of a formatted verbatim
 * element in DocBook: programlisting, screen, address, literallayout,
 * or synopsis) and returns a result tree fragment decorated with
 * callouts.</p>
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
 * @version $Id: Verbatim.java,v 1.1 2002/06/13 20:32:17 chorns Exp $
 *
 */
public class Verbatim {
  /** A stack to hold the open elements while walking through a RTF. */
  private Stack elementStack = null;
  /** A stack to hold the temporarily closed elements. */
  private Stack tempStack = null;
  /** The current line number. */
  private int lineNumber = 0;
  /** The current column number. */
  private int colNumber = 0;
  /** The modulus for line numbering (every 'modulus' line is numbered). */
  private int modulus = 0;
  /** The width (in characters) of line numbers (for padding). */
  private int width = 0;
  /** The separator between the line number and the verbatim text. */
  private String separator = "";
  /** The (sorted) array of callouts obtained from the areaspec. */
  private Callout callout[] = null;
  /** The number of callouts in the callout array. */
  private int calloutCount = 0;
  /** A pointer used to keep track of our position in the callout array. */
  private int calloutPos = 0;
  /** The path to use for graphical callout decorations. */
  private String graphicsPath = null;
  /** The extension to use for graphical callout decorations. */
  private String graphicsExt = null;
  /** The largest callout number that can be represented graphically. */
  private int graphicsMax = 10;
  /** Should graphic callouts use fo:external-graphics or imgs. */
  private boolean graphicsFO = false;

  private static final String foURI = "http://www.w3.org/1999/XSL/Format";
  private static final String xhURI = "http://www.w3.org/1999/xhtml";

  /**
   * <p>Constructor for Verbatim</p>
   *
   * <p>All of the methods are static, so the constructor does nothing.</p>
   */
  public Verbatim() {
  }

  /**
   * <p>Number lines in a verbatim environment.</p>
   *
   * <p>This method adds line numbers to a result tree fragment. Each
   * newline that occurs in a text node is assumed to start a new line.
   * The first line is always numbered, every subsequent xalanMod line
   * is numbered (so if xalanMod=5, lines 1, 5, 10, 15, etc. will be
   * numbered. If there are fewer than xalanMod lines in the environment,
   * every line is numbered.</p>
   *
   * <p>xalanMod is taken from the $linenumbering.everyNth parameter.</p>
   *
   * <p>Every line number will be right justified in a string xalanWidth
   * characters long. If the line number of the last line in the
   * environment is too long to fit in the specified width, the width
   * is automatically increased to the smallest value that can hold the
   * number of the last line. (In other words, if you specify the value 2
   * and attempt to enumerate the lines of an environment that is 100 lines
   * long, the value 3 will automatically be used for every line in the
   * environment.)</p>
   *
   * <p>xalanWidth is taken from the $linenumbering.width parameter.</p>
   *
   * <p>The xalanSep string is inserted between the line
   * number and the original program listing. Lines that aren't numbered
   * are preceded by a xalanWidth blank string and the separator.</p>
   *
   * <p>xalanSep is taken from the $linenumbering.separator parameter.</p>
   *
   * <p>If inline markup extends across line breaks, markup changes are
   * required. All the open elements are closed before the line break and
   * "reopened" afterwards. The reopened elements will have the same
   * attributes as the originals, except that 'name' and 'id' attributes
   * are not duplicated.</p>
   *
   * @param xalanRTF The result tree fragment of the verbatim environment.
   *
   * @return The modified result tree fragment.
   */
  public DocumentFragment numberLines (ExpressionContext context,
				       NodeIterator xalanNI) {

    int xalanMod = Params.getInt(context, "linenumbering.everyNth");
    int xalanWidth = Params.getInt(context, "linenumbering.width");
    String xalanSep = Params.getString(context, "linenumbering.separator");

    DocumentFragment xalanRTF = (DocumentFragment) xalanNI.nextNode();
    int numLines = countLineBreaks(xalanRTF) + 1;

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
    DOMBuilder db = new DOMBuilder(doc, df);

    elementStack = new Stack();
    lineNumber = 0;
    modulus = numLines < xalanMod ? 1 : xalanMod;
    width = xalanWidth;
    separator = xalanSep;

    double log10numLines = Math.log(numLines) / Math.log(10);

    if (width < log10numLines + 1) {
      width = (int) Math.floor(log10numLines + 1);
    }

    lineNumberFragment(db, xalanRTF);
    return df;
  }

  /**
   * <p>Count the number of lines in a verbatim environment.</p>
   *
   * <p>This method walks over the nodes of a DocumentFragment and
   * returns the number of lines breaks that it contains.</p>
   *
   * @param node The root of the tree walk over.
   */
  private int countLineBreaks(Node node) {
    int numLines = 0;

    if (node.getNodeType() == Node.DOCUMENT_FRAGMENT_NODE
	|| node.getNodeType() == Node.DOCUMENT_NODE
	|| node.getNodeType() == Node.ELEMENT_NODE) {
      Node child = node.getFirstChild();
      while (child != null) {
	numLines += countLineBreaks(child);
	child = child.getNextSibling();
      }
    } else if (node.getNodeType() == Node.TEXT_NODE) {
      String text = node.getNodeValue();

      // Walk through the text node looking for newlines
      int pos = 0;
      for (int count = 0; count < text.length(); count++) {
	if (text.charAt(count) == '\n') {
	  numLines++;
	}
      }
    } else {
      // nop
    }

    return numLines;
  }

  /**
   * <p>Build a DocumentFragment with numbered lines.</p>
   *
   * <p>This is the method that actually does the work of numbering
   * lines in a verbatim environment. It recursively walks through a
   * tree of nodes, copying the structure into the rtf. Text nodes
   * are examined for new lines and modified as requested by the
   * global line numbering parameters.</p>
   *
   * <p>When called, rtf should be an empty DocumentFragment and node
   * should be the first child of the result tree fragment that contains
   * the existing, formatted verbatim text.</p>
   *
   * @param rtf The resulting verbatim environment with numbered lines.
   * @param node The root of the tree to copy.
   */
  private void lineNumberFragment(DOMBuilder rtf,
				  Node node) {
    try {
      if (node.getNodeType() == Node.DOCUMENT_FRAGMENT_NODE
	  || node.getNodeType() == Node.DOCUMENT_NODE) {
	Node child = node.getFirstChild();
	while (child != null) {
	  lineNumberFragment(rtf, child);
	  child = child.getNextSibling();
	}
      } else if (node.getNodeType() == Node.ELEMENT_NODE) {
	String ns = node.getNamespaceURI();
	String localName = node.getLocalName();
	String name = ((Element) node).getTagName();

	rtf.startElement(ns, localName, name,
			 copyAttributes((Element) node));

	elementStack.push(node);

	Node child = node.getFirstChild();
	while (child != null) {
	  lineNumberFragment(rtf, child);
	  child = child.getNextSibling();
	}
      } else if (node.getNodeType() == Node.TEXT_NODE) {
	String text = node.getNodeValue();

	if (lineNumber == 0) {
	  // The first line is always numbered
	  formatLineNumber(rtf, ++lineNumber);
	}

	// Walk through the text node looking for newlines
	char chars[] = text.toCharArray();
	int pos = 0;
	for (int count = 0; count < text.length(); count++) {
	  if (text.charAt(count) == '\n') {
	    // This is the tricky bit; if we find a newline, make sure
	    // it doesn't occur inside any markup.

	    if (pos > 0) {
	      rtf.characters(chars, 0, pos);
	      pos = 0;
	    }

	    closeOpenElements(rtf);

	    // Copy the newline to the output
	    chars[pos++] = text.charAt(count);
	    rtf.characters(chars, 0, pos);
	    pos = 0;

	    // Add the line number
	    formatLineNumber(rtf, ++lineNumber);

	    openClosedElements(rtf);
	  } else {
	    chars[pos++] = text.charAt(count);
	  }
	}

	if (pos > 0) {
	  rtf.characters(chars, 0, pos);
	}
      } else if (node.getNodeType() == Node.COMMENT_NODE) {
	String text = node.getNodeValue();
	char chars[] = text.toCharArray();
	rtf.comment(chars, 0, text.length());
      } else if (node.getNodeType() == Node.PROCESSING_INSTRUCTION_NODE) {
	rtf.processingInstruction(node.getNodeName(), node.getNodeValue());
      } else {
	System.out.println("Warning: unexpected node type in lineNumberFragment");
      }

      if (node.getNodeType() == Node.ELEMENT_NODE) {
	String ns = node.getNamespaceURI();
	String localName = node.getLocalName();
	String name = ((Element) node).getTagName();
	rtf.endElement(ns, localName, name);
	elementStack.pop();
      }
    } catch (SAXException e) {
      System.out.println("SAX Exception in lineNumberFragment");
    }
  }

  /**
   * <p>Add a formatted line number to the result tree fragment.</p>
   *
   * <p>This method examines the global parameters that control line
   * number presentation (modulus, width, and separator) and adds
   * the appropriate text to the result tree fragment.</p>
   *
   * @param rtf The resulting verbatim environment with numbered lines.
   * @param lineNumber The number of the current line.
   */
  private void formatLineNumber(DOMBuilder rtf,
				int lineNumber) {
    char ch = 160;
    String lno = "";
    if (lineNumber == 1
	|| (modulus >= 1 && (lineNumber % modulus == 0))) {
      lno = "" + lineNumber;
    }

    while (lno.length() < width) {
      lno = ch + lno;
    }

    lno += separator;

    char chars[] = lno.toCharArray();
    try {
      rtf.characters(chars, 0, lno.length());
    } catch (SAXException e) {
      System.out.println("SAX Exception in formatLineNumber");
    }
  }

  /**
   * <p>Insert text callouts into a verbatim environment.</p>
   *
   * <p>This method examines the <tt>areaset</tt> and <tt>area</tt> elements
   * in the supplied <tt>areaspec</tt> and decorates the supplied
   * result tree fragment with appropriate callout markers.</p>
   *
   * <p>If a <tt>label</tt> attribute is supplied on an <tt>area</tt>,
   * its content will be used for the label, otherwise the callout
   * number will be used, surrounded by parenthesis. Callouts are
   * numbered in document order. All of the <tt>area</tt>s in an
   * <tt>areaset</tt> get the same number.</p>
   *
   * <p>Only the <tt>linecolumn</tt> and <tt>linerange</tt> units are
   * supported. If no unit is specifed, <tt>linecolumn</tt> is assumed.
   * If only a line is specified, the callout decoration appears in
   * the defaultColumn. Lines will be padded with blanks to reach the
   * necessary column, but callouts that are located beyond the last
   * line of the verbatim environment will be ignored.</p>
   *
   * <p>Callouts are inserted before the character at the line/column
   * where they are to occur.</p>
   *
   * @param areaspecNodeSet The source node set that contains the areaspec.
   * @param xalanRTF The result tree fragment of the verbatim environment.
   * @param defaultColumn The column for callouts that specify only a line.
   *
   * @return The modified result tree fragment.  */

  /**
   * <p>Insert graphical callouts into a verbatim environment.</p>
   *
   * <p>This method examines the <tt>areaset</tt> and <tt>area</tt> elements
   * in the supplied <tt>areaspec</tt> and decorates the supplied
   * result tree fragment with appropriate callout markers.</p>
   *
   * <p>If a <tt>label</tt> attribute is supplied on an <tt>area</tt>,
   * its content will be used for the label, otherwise the callout
   * number will be used. Callouts are
   * numbered in document order. All of the <tt>area</tt>s in an
   * <tt>areaset</tt> get the same number.</p>
   *
   * <p>If the callout number is not greater than <tt>gMax</tt>, the
   * callout generated will be:</p>
   *
   * <pre>
   * &lt;img src="$gPath/conumber$gExt" alt="conumber">
   * </pre>
   *
   * <p>Otherwise, it will be the callout number surrounded by
   * parenthesis.</p>
   *
   * <p>Only the <tt>linecolumn</tt> and <tt>linerange</tt> units are
   * supported. If no unit is specifed, <tt>linecolumn</tt> is assumed.
   * If only a line is specified, the callout decoration appears in
   * the defaultColumn. Lines will be padded with blanks to reach the
   * necessary column, but callouts that are located beyond the last
   * line of the verbatim environment will be ignored.</p>
   *
   * <p>Callouts are inserted before the character at the line/column
   * where they are to occur.</p>
   *
   * @param areaspecNodeSet The source node set that contains the areaspec.
   * @param xalanRTF The result tree fragment of the verbatim environment.
   * @param defaultColumn The column for callouts that specify only a line.
   * @param gPath The path to use for callout graphics.
   * @param gExt The extension to use for callout graphics.
   * @param gMax The largest number that can be represented as a graphic.
   * @param useFO Should fo:external-graphics be produced, as opposed to
   * HTML imgs. This is bogus, the extension should figure it out, but I
   * haven't figured out how to do that yet.
   *
   * @return The modified result tree fragment.
   */

  public DocumentFragment insertCallouts (ExpressionContext context,
					  NodeIterator areaspecNodeSet,
					  NodeIterator xalanNI) {
    String type = Params.getString(context, "stylesheet.result.type");
    boolean useFO = type.equals("fo");
    int defaultColumn = Params.getInt(context, "callout.defaultcolumn");

    if (Params.getBoolean(context, "callout.graphics")) {
      String gPath = Params.getString(context, "callout.graphics.path");
      String gExt = Params.getString(context, "callout.graphics.extension");
      int gMax = Params.getInt(context, "callout.graphics.number.limit");
      return insertGraphicCallouts(areaspecNodeSet, xalanNI, defaultColumn,
				   gPath, gExt, gMax, useFO);
    } else if (Params.getBoolean(context, "callout.unicode")) {
      int uStart = Params.getInt(context, "callout.unicode.start.character");
      int uMax = Params.getInt(context, "callout.unicode.number.limit");
      return insertUnicodeCallouts(areaspecNodeSet, xalanNI, defaultColumn,
				   uStart, uMax, useFO);
    } else if (Params.getBoolean(context, "callout.dingbats")) {
      int dMax = 10;
      return insertDingbatCallouts(areaspecNodeSet, xalanNI, defaultColumn,
				   dMax, useFO);
    } else {
      return insertTextCallouts(areaspecNodeSet, xalanNI, defaultColumn, useFO);
    }
  }

  public DocumentFragment insertGraphicCallouts (NodeIterator areaspecNodeSet,
						 NodeIterator xalanNI,
						 int defaultColumn,
						 String gPath,
						 String gExt,
						 int gMax,
						 boolean useFO) {
    FormatGraphicCallout fgc = new FormatGraphicCallout(gPath,gExt,gMax,useFO);
    return insertCallouts(areaspecNodeSet, xalanNI, defaultColumn, fgc);
  }

  public DocumentFragment insertUnicodeCallouts (NodeIterator areaspecNodeSet,
						 NodeIterator xalanNI,
						 int defaultColumn,
						 int uStart,
						 int uMax,
						 boolean useFO) {
    FormatUnicodeCallout fuc = new FormatUnicodeCallout(uStart, uMax, useFO);
    return insertCallouts(areaspecNodeSet, xalanNI, defaultColumn, fuc);
  }

  public DocumentFragment insertDingbatCallouts (NodeIterator areaspecNodeSet,
						 NodeIterator xalanNI,
						 int defaultColumn,
						 int gMax,
						 boolean useFO) {
    FormatDingbatCallout fdc = new FormatDingbatCallout(gMax,useFO);
    return insertCallouts(areaspecNodeSet, xalanNI, defaultColumn, fdc);
  }

  public DocumentFragment insertTextCallouts (NodeIterator areaspecNodeSet,
					      NodeIterator xalanNI,
					      int defaultColumn,
					      boolean useFO) {
    FormatTextCallout ftc = new FormatTextCallout(useFO);
    return insertCallouts(areaspecNodeSet, xalanNI, defaultColumn, ftc);
  }

  public DocumentFragment insertCallouts (NodeIterator areaspecNodeSet,
					  NodeIterator xalanNI,
					  int defaultColumn,
					  FormatCallout fCallout) {

    DocumentFragment xalanRTF = (DocumentFragment) xalanNI.nextNode();

    callout = new Callout[10];
    calloutCount = 0;
    calloutPos = 0;
    lineNumber = 1;
    colNumber = 1;

    // First we walk through the areaspec to calculate the position
    // of the callouts
    //  <areaspec>
    //  <areaset id="ex.plco.const" coords="">
    //    <area id="ex.plco.c1" coords="4"/>
    //    <area id="ex.plco.c2" coords="8"/>
    //  </areaset>
    //  <area id="ex.plco.ret" coords="12"/>
    //  <area id="ex.plco.dest" coords="12"/>
    //  </areaspec>
    int pos = 0;
    int coNum = 0;
    boolean inAreaSet = false;
    Node node = areaspecNodeSet.nextNode();
    node = node.getFirstChild();
    while (node != null) {
      if (node.getNodeType() == Node.ELEMENT_NODE) {
	if (node.getNodeName().equals("areaset")) {
	  coNum++;
	  Node area = node.getFirstChild();
	  while (area != null) {
	    if (area.getNodeType() == Node.ELEMENT_NODE) {
	      if (area.getNodeName().equals("area")) {
		addCallout(coNum, area, defaultColumn);
	      } else {
		System.out.println("Unexpected element in areaset: "
				   + area.getNodeName());
	      }
	    }
	    area = area.getNextSibling();
	  }
	} else if (node.getNodeName().equalsIgnoreCase("area")) {
	  coNum++;
	  addCallout(coNum, node, defaultColumn);
	} else {
	  System.out.println("Unexpected element in areaspec: "
			     + node.getNodeName());
	}
      }

      node = node.getNextSibling();
    }

    // Now sort them
    java.util.Arrays.sort(callout, 0, calloutCount);

    DocumentBuilderFactory docFactory = DocumentBuilderFactory.newInstance();
    DocumentBuilder docBuilder = null;

    try {
      docBuilder = docFactory.newDocumentBuilder();
    } catch (ParserConfigurationException e) {
      System.out.println("PCE 2!");
      return xalanRTF;
    }
    Document doc = docBuilder.newDocument();
    DocumentFragment df = doc.createDocumentFragment();
    DOMBuilder db = new DOMBuilder(doc, df);

    elementStack = new Stack();
    calloutFragment(db, xalanRTF, fCallout);
    return df;
  }

  /**
   * <p>Build a FragmentValue with callout decorations.</p>
   *
   * <p>This is the method that actually does the work of adding
   * callouts to a verbatim environment. It recursively walks through a
   * tree of nodes, copying the structure into the rtf. Text nodes
   * are examined for the position of callouts as described by the
   * global callout parameters.</p>
   *
   * <p>When called, rtf should be an empty FragmentValue and node
   * should be the first child of the result tree fragment that contains
   * the existing, formatted verbatim text.</p>
   *
   * @param rtf The resulting verbatim environment with numbered lines.
   * @param node The root of the tree to copy.
   */
  private void calloutFragment(DOMBuilder rtf,
			       Node node,
			       FormatCallout fCallout) {
    try {
      if (node.getNodeType() == Node.DOCUMENT_FRAGMENT_NODE
	|| node.getNodeType() == Node.DOCUMENT_NODE) {
	Node child = node.getFirstChild();
	while (child != null) {
	  calloutFragment(rtf, child, fCallout);
	  child = child.getNextSibling();
	}
      } else if (node.getNodeType() == Node.ELEMENT_NODE) {
	String ns = node.getNamespaceURI();
	String localName = node.getLocalName();
	String name = ((Element) node).getTagName();

	rtf.startElement(ns, localName, name,
			 copyAttributes((Element) node));

	elementStack.push(node);

	Node child = node.getFirstChild();
	while (child != null) {
	  calloutFragment(rtf, child, fCallout);
	  child = child.getNextSibling();
	}
      } else if (node.getNodeType() == Node.TEXT_NODE) {
	String text = node.getNodeValue();

	char chars[] = text.toCharArray();
	int pos = 0;
	for (int count = 0; count < text.length(); count++) {
	  if (calloutPos < calloutCount
	      && callout[calloutPos].getLine() == lineNumber
	      && callout[calloutPos].getColumn() == colNumber) {
	    if (pos > 0) {
	      rtf.characters(chars, 0, pos);
	      pos = 0;
	    }

	    closeOpenElements(rtf);

	    while (calloutPos < calloutCount
		   && callout[calloutPos].getLine() == lineNumber
		   && callout[calloutPos].getColumn() == colNumber) {
	      fCallout.formatCallout(rtf, callout[calloutPos]);
	      calloutPos++;
	    }

	    openClosedElements(rtf);
	  }

	  if (text.charAt(count) == '\n') {
	    // What if we need to pad this line?
	    if (calloutPos < calloutCount
		&& callout[calloutPos].getLine() == lineNumber
		&& callout[calloutPos].getColumn() > colNumber) {

	      if (pos > 0) {
		rtf.characters(chars, 0, pos);
		pos = 0;
	      }

	      closeOpenElements(rtf);

	      while (calloutPos < calloutCount
		     && callout[calloutPos].getLine() == lineNumber
		     && callout[calloutPos].getColumn() > colNumber) {
		formatPad(rtf, callout[calloutPos].getColumn() - colNumber);
		colNumber = callout[calloutPos].getColumn();
		while (calloutPos < calloutCount
		       && callout[calloutPos].getLine() == lineNumber
		       && callout[calloutPos].getColumn() == colNumber) {
		  fCallout.formatCallout(rtf, callout[calloutPos]);
		  calloutPos++;
		}
	      }

	      openClosedElements(rtf);
	    }

	    lineNumber++;
	    colNumber = 1;
	  } else {
	    colNumber++;
	  }
	  chars[pos++] = text.charAt(count);
	}

	if (pos > 0) {
	  rtf.characters(chars, 0, pos);
	}
      } else if (node.getNodeType() == Node.COMMENT_NODE) {
	String text = node.getNodeValue();
	char chars[] = text.toCharArray();
	rtf.comment(chars, 0, text.length());
      } else if (node.getNodeType() == Node.PROCESSING_INSTRUCTION_NODE) {
	rtf.processingInstruction(node.getNodeName(), node.getNodeValue());
      } else {
	System.out.println("Warning: unexpected node type in calloutFragment: " + node.getNodeType() + ": " + node.getNodeName());
      }

      if (node.getNodeType() == Node.ELEMENT_NODE) {
	String ns = node.getNamespaceURI();
	String localName = node.getLocalName();
	String name = ((Element) node).getTagName();
	rtf.endElement(ns, localName, name);
	elementStack.pop();
      } else {
	// nop
      }
    } catch (SAXException e) {
      System.out.println("SAX Exception in calloutFragment");
    }
  }

  /**
   * <p>Add a callout to the global callout array</p>
   *
   * <p>This method examines a callout <tt>area</tt> and adds it to
   * the global callout array if it can be interpreted.</p>
   *
   * <p>Only the <tt>linecolumn</tt> and <tt>linerange</tt> units are
   * supported. If no unit is specifed, <tt>linecolumn</tt> is assumed.
   * If only a line is specified, the callout decoration appears in
   * the <tt>defaultColumn</tt>.</p>
   *
   * @param coNum The callout number.
   * @param node The <tt>area</tt>.
   * @param defaultColumn The default column for callouts.
   */
  private void addCallout (int coNum,
			   Node node,
			   int defaultColumn) {
    Element area = (Element) node;

    String units = area.getAttribute("units");
    String otherUnits = area.getAttribute("otherunits");
    String coords = area.getAttribute("coords");
    int type = 0;
    String otherType = null;

    if (units == null || units.equals("linecolumn")) {
      type = Callout.LINE_COLUMN; // the default
    } else if (units.equals("linerange")) {
      type = Callout.LINE_RANGE;
    } else if (units.equals("linecolumnpair")) {
      type = Callout.LINE_COLUMN_PAIR;
    } else if (units.equals("calspair")) {
      type = Callout.CALS_PAIR;
    } else {
      type = Callout.OTHER;
      otherType = otherUnits;
    }

    if (type != Callout.LINE_COLUMN
	&& type != Callout.LINE_RANGE) {
      System.out.println("Only linecolumn and linerange units are supported");
      return;
    }

    if (coords == null) {
      System.out.println("Coords must be specified");
      return;
    }

    // Now let's see if we can interpret the coordinates...
    StringTokenizer st = new StringTokenizer(coords);
    int tokenCount = 0;
    int c1 = 0;
    int c2 = 0;
    while (st.hasMoreTokens()) {
      tokenCount++;
      if (tokenCount > 2) {
	System.out.println("Unparseable coordinates");
	return;
      }
      try {
	String token = st.nextToken();
	int coord = Integer.parseInt(token);
	c2 = coord;
	if (tokenCount == 1) {
	  c1 = coord;
	}
      } catch (NumberFormatException e) {
	System.out.println("Unparseable coordinate");
	return;
      }
    }

    // Make sure we aren't going to blow past the end of our array
    if (calloutCount == callout.length) {
      Callout bigger[] = new Callout[calloutCount+10];
      for (int count = 0; count < callout.length; count++) {
	bigger[count] = callout[count];
      }
      callout = bigger;
    }

    // Ok, add the callout
    if (tokenCount == 2) {
      if (type == Callout.LINE_RANGE) {
	for (int count = c1; count <= c2; count++) {
	  callout[calloutCount++] = new Callout(coNum, area,
						count, defaultColumn,
						type);
	}
      } else {
	// assume linecolumn
	callout[calloutCount++] = new Callout(coNum, area, c1, c2, type);
      }
    } else {
      // if there's only one number, assume it's the line
      callout[calloutCount++] = new Callout(coNum, area, c1, defaultColumn, type);
    }
  }

  /**
   * <p>Add blanks to the result tree fragment.</p>
   *
   * <p>This method adds <tt>numBlanks</tt> to the result tree fragment.
   * It's used to pad lines when callouts occur after the last existing
   * characater in a line.</p>
   *
   * @param rtf The resulting verbatim environment with numbered lines.
   * @param numBlanks The number of blanks to add.
   */
  private void formatPad(DOMBuilder rtf,
			 int numBlanks) {
    char chars[] = new char[numBlanks];
    for (int count = 0; count < numBlanks; count++) {
      chars[count] = ' ';
    }

    try {
      rtf.characters(chars, 0, numBlanks);
    } catch (SAXException e) {
      System.out.println("SAX Exception in formatCallout");
    }
  }

  private void closeOpenElements(DOMBuilder rtf)
    throws SAXException {
    // Close all the open elements...
    tempStack = new Stack();
    while (!elementStack.empty()) {
      Node elem = (Node) elementStack.pop();

      String ns = elem.getNamespaceURI();
      String localName = elem.getLocalName();
      String name = ((Element) elem).getTagName();

      // If this is the bottom of the stack and it's an fo:block
      // or an HTML pre or div, don't duplicate it...
      if (elementStack.empty()
	  && (((ns != null)
	       && ns.equals(foURI)
	       && localName.equals("block"))
	      || (((ns == null)
		   && localName.equalsIgnoreCase("pre"))
		  || ((ns != null)
		      && ns.equals(xhURI)
		      && localName.equals("pre")))
	      || (((ns == null)
		   && localName.equalsIgnoreCase("div"))
		  || ((ns != null)
		      && ns.equals(xhURI)
		      && localName.equals("div"))))) {
	elementStack.push(elem);
	break;
      } else {
	rtf.endElement(ns, localName, name);
	tempStack.push(elem);
      }
    }
  }

  private void openClosedElements(DOMBuilder rtf)
    throws SAXException {
    // Now "reopen" the elements that we closed...
    while (!tempStack.empty()) {
      Node elem = (Node) tempStack.pop();

      String ns = elem.getNamespaceURI();
      String localName = elem.getLocalName();
      String name = ((Element) elem).getTagName();
      NamedNodeMap domAttr = elem.getAttributes();

      AttributesImpl attr = new AttributesImpl();
      for (int acount = 0; acount < domAttr.getLength(); acount++) {
	Node a = domAttr.item(acount);

	if (((ns == null || ns == "http://www.w3.org/1999/xhtml")
	     && localName.equalsIgnoreCase("a"))
	    || (a.getLocalName().equalsIgnoreCase("id"))) {
	  // skip this attribute
	} else {
	  attr.addAttribute(a.getNamespaceURI(),
			    a.getLocalName(),
			    a.getNodeName(),
			    "CDATA",
			    a.getNodeValue());
	}
      }

      rtf.startElement(ns, localName, name, attr);
      elementStack.push(elem);
    }

    tempStack = null;
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
