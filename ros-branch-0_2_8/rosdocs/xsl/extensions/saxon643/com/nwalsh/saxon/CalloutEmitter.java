package com.nwalsh.saxon;

import java.util.Stack;
import java.util.StringTokenizer;
import org.xml.sax.*;
import org.w3c.dom.*;
import javax.xml.transform.TransformerException;
import com.icl.saxon.Controller;
import com.icl.saxon.om.NamePool;
import com.icl.saxon.output.Emitter;
import com.icl.saxon.tree.AttributeCollection;

/**
 * <p>Saxon extension to decorate a result tree fragment with callouts.</p>
 *
 * <p>$Id: CalloutEmitter.java,v 1.1 2002/06/13 20:32:15 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides the guts of a
 * <a href="http://users.iclway.co.uk/mhkay/saxon/">Saxon 6.*</a>
 * implementation of callouts for verbatim environments. (It is used
 * by the Verbatim class.)</p>
 *
 * <p>The general design is this: the stylesheets construct a result tree
 * fragment for some verbatim environment. The Verbatim class initializes
 * a CalloutEmitter with information about the callouts that should be applied
 * to the verbatim environment in question. Then the result tree fragment
 * is "replayed" through the CalloutEmitter; the CalloutEmitter builds a
 * new result tree fragment from this event stream, decorated with callouts,
 * and that is returned.</p>
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
 * @version $Id: CalloutEmitter.java,v 1.1 2002/06/13 20:32:15 chorns Exp $
 *
 */
public class CalloutEmitter extends CopyEmitter {
  /** A stack for the preserving information about open elements. */
  protected Stack elementStack = null;

  /** A stack for holding information about temporarily closed elements. */
  protected Stack tempStack = null;

  /** Is the next element absolutely the first element in the fragment? */
  protected boolean firstElement = false;

  /** The FO namespace name. */
  protected static String foURI = "http://www.w3.org/1999/XSL/Format";

  /** The XHTML namespace name. */
  protected static String xhURI = "http://www.w3.org/1999/xhtml";

  /** The default column for callouts that specify only a line. */
  protected int defaultColumn = 60;

  /** Is the stylesheet currently running an FO stylesheet? */
  protected boolean foStylesheet = false;

  /** The current line number. */
  private static int lineNumber = 0;

  /** The current column number. */
  private static int colNumber = 0;

  /** The (sorted) array of callouts obtained from the areaspec. */
  private static Callout callout[] = null;

  /** The number of callouts in the callout array. */
  private static int calloutCount = 0;

  /** A pointer used to keep track of our position in the callout array. */
  private static int calloutPos = 0;

  /** The FormatCallout object to use for formatting callouts. */
  private static FormatCallout fCallout = null;

  /** <p>Constructor for the CalloutEmitter.</p>
   *
   * @param namePool The name pool to use for constructing elements and attributes.
   * @param graphicsPath The path to callout number graphics.
   * @param graphicsExt The extension for callout number graphics.
   * @param graphicsMax The largest callout number that can be represented as a graphic.
   * @param defaultColumn The default column for callouts.
   * @param foStylesheet Is this an FO stylesheet?
   */
  public CalloutEmitter(Controller controller,
			NamePool namePool,
			int defaultColumn,
			boolean foStylesheet,
			FormatCallout fCallout) {
    super(controller, namePool);
    elementStack = new Stack();
    firstElement = true;

    this.defaultColumn = defaultColumn;
    this.foStylesheet = foStylesheet;
    this.fCallout = fCallout;
  }

  /**
   * <p>Examine the areaspec and determine the number and position of 
   * callouts.</p>
   *
   * <p>The <code><a href="http://docbook.org/tdg/html/areaspec.html">areaspecNodeSet</a></code>
   * is examined and a sorted list of the callouts is constructed.</p>
   *
   * <p>This data structure is used to augment the result tree fragment
   * with callout bullets.</p>
   *
   * @param areaspecNodeSet The source document &lt;areaspec&gt; element.
   *
   */
  public void setupCallouts (NodeList areaspecNodeList) {
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
    Node areaspec = areaspecNodeList.item(0);
    NodeList children = areaspec.getChildNodes();

    for (int count = 0; count < children.getLength(); count++) {
      Node node = children.item(count);
      if (node.getNodeType() == Node.ELEMENT_NODE) {
	if (node.getNodeName().equalsIgnoreCase("areaset")) {
	  coNum++;
	  NodeList areas = node.getChildNodes();
	  for (int acount = 0; acount < areas.getLength(); acount++) {
	    Node area = areas.item(acount);
	    if (area.getNodeType() == Node.ELEMENT_NODE) {
	      if (area.getNodeName().equalsIgnoreCase("area")) {
		addCallout(coNum, area, defaultColumn);
	      } else {
		System.out.println("Unexpected element in areaset: "
				   + area.getNodeName());
	      }
	    }
	  }
	} else if (node.getNodeName().equalsIgnoreCase("area")) {
	  coNum++;
	  addCallout(coNum, node, defaultColumn);
	} else {
	  System.out.println("Unexpected element in areaspec: "
			     + node.getNodeName());
	}
      }
    }

    // Now sort them
    java.util.Arrays.sort(callout, 0, calloutCount);
  }

  /** Process characters. */
  public void characters(char[] chars, int start, int len)
    throws TransformerException {

    // If we hit characters, then there's no first element...
    firstElement = false;

    if (lineNumber == 0) {
      // if there are any text nodes, there's at least one line
      lineNumber++;
      colNumber = 1;
    }

    // Walk through the text node looking for callout positions
    char[] newChars = new char[len];
    int pos = 0;
    for (int count = start; count < start+len; count++) {
      if (calloutPos < calloutCount
	  && callout[calloutPos].getLine() == lineNumber
	  && callout[calloutPos].getColumn() == colNumber) {
	if (pos > 0) {
	  rtfEmitter.characters(newChars, 0, pos);
	  pos = 0;
	}

	closeOpenElements(rtfEmitter);

	while (calloutPos < calloutCount
	       && callout[calloutPos].getLine() == lineNumber
	       && callout[calloutPos].getColumn() == colNumber) {
	  fCallout.formatCallout(rtfEmitter, callout[calloutPos]);
	  calloutPos++;
	}

	openClosedElements(rtfEmitter);
      }

      if (chars[count] == '\n') {
	// What if we need to pad this line?
	if (calloutPos < calloutCount
	    && callout[calloutPos].getLine() == lineNumber
	    && callout[calloutPos].getColumn() > colNumber) {

	  if (pos > 0) {
	    rtfEmitter.characters(newChars, 0, pos);
	    pos = 0;
	  }

	  closeOpenElements(rtfEmitter);

	  while (calloutPos < calloutCount
		 && callout[calloutPos].getLine() == lineNumber
		 && callout[calloutPos].getColumn() > colNumber) {
	    formatPad(callout[calloutPos].getColumn() - colNumber);
	    colNumber = callout[calloutPos].getColumn();
	    while (calloutPos < calloutCount
		   && callout[calloutPos].getLine() == lineNumber
		   && callout[calloutPos].getColumn() == colNumber) {
	      fCallout.formatCallout(rtfEmitter, callout[calloutPos]);
	      calloutPos++;
	    }
	  }

	  openClosedElements(rtfEmitter);
	}

	lineNumber++;
	colNumber = 1;
      } else {
	colNumber++;
      }
      newChars[pos++] = chars[count];
    }

    if (pos > 0) {
      rtfEmitter.characters(newChars, 0, pos);
    }
  }

  /**
   * <p>Add blanks to the result tree fragment.</p>
   *
   * <p>This method adds <tt>numBlanks</tt> to the result tree fragment.
   * It's used to pad lines when callouts occur after the last existing
   * characater in a line.</p>
   *
   * @param numBlanks The number of blanks to add.
   */
  protected void formatPad(int numBlanks) {
    char chars[] = new char[numBlanks];
    for (int count = 0; count < numBlanks; count++) {
      chars[count] = ' ';
    }

    try {
      rtfEmitter.characters(chars, 0, numBlanks);
    } catch (TransformerException e) {
      System.out.println("Transformer Exception in formatPad");
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
  protected void addCallout (int coNum,
			     Node node,
			     int defaultColumn) {

    Element area  = (Element) node;
    String units  = null;
    String coords = null;

    if (area.hasAttribute("units")) {
      units = area.getAttribute("units");
    }

    if (area.hasAttribute("coords")) {
      coords = area.getAttribute("coords");
    }

    if (units != null
	&& !units.equalsIgnoreCase("linecolumn")
	&& !units.equalsIgnoreCase("linerange")) {
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
      if (units != null && units.equalsIgnoreCase("linerange")) {
	for (int count = c1; count <= c2; count++) {
	  callout[calloutCount++] = new Callout(coNum, area,
						count, defaultColumn);
	}
      } else {
	// assume linecolumn
	callout[calloutCount++] = new Callout(coNum, area, c1, c2);
      }
    } else {
      // if there's only one number, assume it's the line
      callout[calloutCount++] = new Callout(coNum, area, c1, defaultColumn);
    }
  }

  /** Process end element events. */
  public void endElement(int nameCode)
    throws TransformerException {

    if (!elementStack.empty()) {
      // if we didn't push the very first element (an fo:block or
      // pre or div surrounding the whole block), then the stack will
      // be empty when we get to the end of the first element...
      elementStack.pop();
    }
    rtfEmitter.endElement(nameCode);
  }

  /** Process start element events. */
  public void startElement(int nameCode,
			   org.xml.sax.Attributes attributes,
			   int[] namespaces,
			   int nscount)
    throws TransformerException {

    if (!skipThisElement(nameCode)) {
      StartElementInfo sei = new StartElementInfo(nameCode, attributes,
						  namespaces, nscount);
      elementStack.push(sei);
    }

    firstElement = false;

    rtfEmitter.startElement(nameCode, attributes, namespaces, nscount);
  }

  /**
   * <p>Protect the outer-most block wrapper.</p>
   *
   * <p>Open elements in the result tree fragment are closed and reopened
   * around callouts (so that callouts don't appear inside links or other
   * environments). But if the result tree fragment is a single block
   * (a div or pre in HTML, an fo:block in FO), that outer-most block is
   * treated specially.</p>
   *
   * <p>This method returns true if the element in question is that
   * outermost block.</p>
   *
   * @param nameCode The name code for the element
   *
   * @return True if the element is the outer-most block, false otherwise.
   */
  protected boolean skipThisElement(int nameCode) {
    // FIXME: This is such a gross hack...
    if (firstElement) {
      int thisFingerprint    = namePool.getFingerprint(nameCode);
      int foBlockFingerprint = namePool.getFingerprint(foURI, "block");
      int htmlPreFingerprint = namePool.getFingerprint("", "pre");
      int htmlDivFingerprint = namePool.getFingerprint("", "div");
      int xhtmlPreFingerprint = namePool.getFingerprint(xhURI, "pre");
      int xhtmlDivFingerprint = namePool.getFingerprint(xhURI, "div");

      if ((foStylesheet && thisFingerprint == foBlockFingerprint)
	  || (!foStylesheet && (thisFingerprint == htmlPreFingerprint
				|| thisFingerprint == htmlDivFingerprint
				|| thisFingerprint == xhtmlPreFingerprint
				|| thisFingerprint == xhtmlDivFingerprint))) {
	// Don't push the outer-most wrapping div, pre, or fo:block
	return true;
      }
    }

    return false;
  }

  private void closeOpenElements(Emitter rtfEmitter)
    throws TransformerException {
    // Close all the open elements...
    tempStack = new Stack();
    while (!elementStack.empty()) {
      StartElementInfo elem = (StartElementInfo) elementStack.pop();
      rtfEmitter.endElement(elem.getNameCode());
      tempStack.push(elem);
    }
  }

  private void openClosedElements(Emitter rtfEmitter)
    throws TransformerException {
    // Now "reopen" the elements that we closed...
    while (!tempStack.empty()) {
      StartElementInfo elem = (StartElementInfo) tempStack.pop();
      AttributeCollection attr = (AttributeCollection) elem.getAttributes();
      AttributeCollection newAttr = new AttributeCollection(namePool);

      for (int acount = 0; acount < attr.getLength(); acount++) {
	String localName = attr.getLocalName(acount);
	int nameCode = attr.getNameCode(acount);
	String type = attr.getType(acount);
	String value = attr.getValue(acount);
	String uri = attr.getURI(acount);
	String prefix = "";

	if (localName.indexOf(':') > 0) {
	  prefix = localName.substring(0, localName.indexOf(':'));
	  localName = localName.substring(localName.indexOf(':')+1);
	}

	if (uri.equals("")
	    && ((foStylesheet
		 && localName.equals("id"))
		|| (!foStylesheet
		    && (localName.equals("id")
			|| localName.equals("name"))))) {
	  // skip this attribute
	} else {
	  newAttr.addAttribute(prefix, uri, localName, type, value);
	}
      }

      rtfEmitter.startElement(elem.getNameCode(),
			      newAttr,
			      elem.getNamespaces(),
			      elem.getNSCount());

      elementStack.push(elem);
    }
  }

  /**
   * <p>A private class for maintaining the information required to call
   * the startElement method.</p>
   *
   * <p>In order to close and reopen elements, information about those
   * elements has to be maintained. This class is just the little record
   * that we push on the stack to keep track of that info.</p>
   */
  private class StartElementInfo {
    private int _nameCode;
    org.xml.sax.Attributes _attributes;
    int[] _namespaces;
    int _nscount;

    public StartElementInfo(int nameCode,
			    org.xml.sax.Attributes attributes,
			    int[] namespaces,
			    int nscount) {
      _nameCode = nameCode;
      _attributes = attributes;
      _namespaces = namespaces;
      _nscount = nscount;
    }

    public int getNameCode() {
      return _nameCode;
    }

    public org.xml.sax.Attributes getAttributes() {
      return _attributes;
    }

    public int[] getNamespaces() {
      return _namespaces;
    }

    public int getNSCount() {
      return _nscount;
    }
  }
}
