package com.nwalsh.saxon;

import java.util.Stack;
import java.util.StringTokenizer;
import org.xml.sax.*;
import org.w3c.dom.*;
import javax.xml.transform.TransformerException;
import com.icl.saxon.Context;
import com.icl.saxon.expr.*;
import com.icl.saxon.expr.FragmentValue;
import com.icl.saxon.Controller;
import com.icl.saxon.functions.Extensions;
import com.icl.saxon.om.*;
import com.icl.saxon.output.*;
import com.icl.saxon.pattern.*;
import com.icl.saxon.tree.*;

/**
 * <p>A Saxon 6.0 Emitter that clones its input.</p>
 *
 * <p>$Id: CopyEmitter.java,v 1.1 2002/06/13 20:32:16 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://users.iclway.co.uk/mhkay/saxon/">Saxon 6.*</a>
 * implementation of an emitter that manufactures a cloned result
 * tree fragment.</p>
 *
 * <p>The purpose of this emitter is to provide something for
 * CalloutEmitter and NumberLinesEmitter to extend.
 * This emitter simply copies all input to a new result tree fragment.</p>
 *
 * <p><b>Change Log:</b></p>
 * <dl>
 * <dt>1.0</dt>
 * <dd><p>Initial release.</p></dd>
 * </dl>
 *
 * @see CalloutEmitter
 * @see NumberLinesEmitter
 *
 * @author Norman Walsh
 * <a href="mailto:ndw@nwalsh.com">ndw@nwalsh.com</a>
 *
 * @version $Id: CopyEmitter.java,v 1.1 2002/06/13 20:32:16 chorns Exp $
 *
 */
public class CopyEmitter extends com.icl.saxon.output.Emitter {
  /** The result tree fragment containing the copied fragment. */
  protected FragmentValue rtf = null;
  protected Emitter rtfEmitter = null;

  /** <p>The namePool.</p>
   *
   * <p>Copied from the caller, it should be the runtime name pool.</p>
   */
  protected NamePool namePool = null;

  /** <p>Constructor for the CopyEmitter.</p>
   *
   * @param namePool The name pool to use for constructing elements and attributes.
   */
  public CopyEmitter(Controller controller, NamePool namePool) {
    rtf = new FragmentValue(controller);
    rtfEmitter = rtf.getEmitter();
    this.namePool = namePool;
  }

  /**
   * <p>Return the result tree fragment constructed by replaying events
   * through this emitter.</p>
   */
  public FragmentValue getResultTreeFragment() {
    return rtf;
  }

  /** Copy characters. */
  public void characters(char[] chars, int start, int len)
    throws TransformerException {
    rtfEmitter.characters(chars, start, len);
  }

  /** Copy comments. */
  public void comment(char[] chars, int start, int length)
    throws TransformerException {
    rtfEmitter.comment(chars, start, length);
  }

  /** Copy end document events. */
  public void endDocument()
    throws TransformerException {
    rtfEmitter.endDocument();
  }

  /** Copy end element events. */
  public void endElement(int nameCode)
    throws TransformerException {
    rtfEmitter.endElement(nameCode);
  }

  /** Copy processing instructions. */
  public void processingInstruction(java.lang.String name,
				    java.lang.String data)
    throws TransformerException {
    rtfEmitter.processingInstruction(name, data);
  }

  /** Copy set document locator events. */
  public void setDocumentLocator(org.xml.sax.Locator locator) {
    rtfEmitter.setDocumentLocator(locator);
  }

  /** Copy set escaping events. */
  public void setEscaping(boolean escaping)
    throws TransformerException {
    rtfEmitter.setEscaping(escaping);
  }

  /** Copy set name pool events. */
  public void setNamePool(NamePool namePool) {
    rtfEmitter.setNamePool(namePool);
  }

  /** Copy set unparsed entity events. */
  public void setUnparsedEntity(java.lang.String name, java.lang.String uri)
    throws TransformerException {
    rtfEmitter.setUnparsedEntity(name, uri);
  }

  /** Copy set writer events. */
  public void setWriter(java.io.Writer writer) {
    rtfEmitter.setWriter(writer);
  }

  /** Copy start document events. */
  public void startDocument()
    throws TransformerException {
    rtfEmitter.startDocument();
  }

  /** Copy start element events. */
  public void startElement(int nameCode,
			   org.xml.sax.Attributes attributes,
			   int[] namespaces,
			   int nscount)
    throws TransformerException {
    rtfEmitter.startElement(nameCode, attributes, namespaces, nscount);
  }
}
