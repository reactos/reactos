package com.nwalsh.saxon;

import java.util.Stack;
import java.util.StringTokenizer;
import org.xml.sax.*;
import org.w3c.dom.*;
import javax.xml.transform.TransformerException;
import com.icl.saxon.output.*;
import com.icl.saxon.om.*;
import com.icl.saxon.Controller;
import com.icl.saxon.tree.AttributeCollection;
import com.icl.saxon.expr.FragmentValue;

/**
 * <p>Saxon extension to decorate a result tree fragment with line numbers.</p>
 *
 * <p>$Id: NumberLinesEmitter.java,v 1.1 2002/06/13 20:32:16 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides the guts of a
 * <a href="http://users.iclway.co.uk/mhkay/saxon/">Saxon 6.*</a>
 * implementation of line numbering for verbatim environments. (It is used
 * by the Verbatim class.)</p>
 *
 * <p>The general design is this: the stylesheets construct a result tree
 * fragment for some verbatim environment. The Verbatim class initializes
 * a NumberLinesEmitter with information about what lines should be
 * numbered and how. Then the result tree fragment
 * is "replayed" through the NumberLinesEmitter; the NumberLinesEmitter
 * builds a
 * new result tree fragment from this event stream, decorated with line
 * numbers,
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
 * @version $Id: NumberLinesEmitter.java,v 1.1 2002/06/13 20:32:16 chorns Exp $
 *
 */
public class NumberLinesEmitter extends CopyEmitter {
  /** A stack for the preserving information about open elements. */
  protected Stack elementStack = null;

  /** The current line number. */
  protected int lineNumber = 0;

  /** Is the next element absolutely the first element in the fragment? */
  protected boolean firstElement = false;

  /** The FO namespace name. */
  protected static String foURI = "http://www.w3.org/1999/XSL/Format";

  /** The XHTML namespace name. */
  protected static String xhURI = "http://www.w3.org/1999/xhtml";

  /** Every <code>modulus</code> line will be numbered. */
  protected int modulus = 5;

  /** Line numbers are <code>width</code> characters wide. */
  protected int width = 3;

  /** Line numbers are separated from the listing by <code>separator</code>. */
  protected String separator = " ";

  /** Is the stylesheet currently running an FO stylesheet? */
  protected boolean foStylesheet = false;

  /** <p>Constructor for the NumberLinesEmitter.</p>
   *
   * @param namePool The name pool to use for constructing elements and attributes.
   * @param modulus The modulus to use for this listing.
   * @param width The width to use for line numbers in this listing.
   * @param separator The separator to use for this listing.
   * @param foStylesheet Is this an FO stylesheet?
   */
  public NumberLinesEmitter(Controller controller,
			    NamePool namePool,
			    int modulus,
			    int width,
			    String separator,
			    boolean foStylesheet) {
    super(controller,namePool);
    elementStack = new Stack();
    firstElement = true;

    this.modulus = modulus;
    this.width = width;
    this.separator = separator;
    this.foStylesheet = foStylesheet;
  }

  /** Process characters. */
  public void characters(char[] chars, int start, int len)
    throws TransformerException {

    // If we hit characters, then there's no first element...
    firstElement = false;

    if (lineNumber == 0) {
      // The first line is always numbered
      formatLineNumber(++lineNumber);
    }

    // Walk through the text node looking for newlines
    char[] newChars = new char[len];
    int pos = 0;
    for (int count = start; count < start+len; count++) {
      if (chars[count] == '\n') {
	// This is the tricky bit; if we find a newline, make sure
	// it doesn't occur inside any markup.

	if (pos > 0) {
	  // Output any characters that preceded this newline
	  rtfEmitter.characters(newChars, 0, pos);
	  pos = 0;
	}

	// Close all the open elements...
	Stack tempStack = new Stack();
	while (!elementStack.empty()) {
	  StartElementInfo elem = (StartElementInfo) elementStack.pop();
	  rtfEmitter.endElement(elem.getNameCode());
	  tempStack.push(elem);
	}

	// Copy the newline to the output
	newChars[pos++] = chars[count];
	rtfEmitter.characters(newChars, 0, pos);
	pos = 0;

	// Add the line number
	formatLineNumber(++lineNumber);

	// Now "reopen" the elements that we closed...
	while (!tempStack.empty()) {
	  StartElementInfo elem = (StartElementInfo) tempStack.pop();
	  AttributeCollection attr = (AttributeCollection)elem.getAttributes();
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
      } else {
	newChars[pos++] = chars[count];
      }
    }

    if (pos > 0) {
      rtfEmitter.characters(newChars, 0, pos);
      pos = 0;
    }
  }

  /**
   * <p>Add a formatted line number to the result tree fragment.</p>
   *
   * @param lineNumber The number of the current line.
   */
  protected void formatLineNumber(int lineNumber) 
    throws TransformerException {

    char ch = 160; // &nbsp;

    String lno = "";
    if (lineNumber == 1
	|| (modulus >= 1 && (lineNumber % modulus == 0))) {
      lno = "" + lineNumber;
    }

    while (lno.length() < width) {
      lno = ch + lno;
    }

    lno += separator;

    char chars[] = new char[lno.length()];
    for (int count = 0; count < lno.length(); count++) {
      chars[count] = lno.charAt(count);
    }

    characters(chars, 0, lno.length());
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
