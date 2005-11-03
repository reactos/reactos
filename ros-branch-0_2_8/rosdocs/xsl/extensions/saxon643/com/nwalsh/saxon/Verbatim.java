// Verbatim.java - Saxon extensions supporting DocBook verbatim environments

package com.nwalsh.saxon;

import java.util.Stack;
import java.util.StringTokenizer;
import org.xml.sax.*;
import org.w3c.dom.*;
import javax.xml.transform.TransformerException;
import com.icl.saxon.Controller;
import com.icl.saxon.expr.*;
import com.icl.saxon.om.*;
import com.icl.saxon.pattern.*;
import com.icl.saxon.Context;
import com.icl.saxon.tree.*;
import com.icl.saxon.functions.Extensions;
import com.nwalsh.saxon.NumberLinesEmitter;
import com.nwalsh.saxon.CalloutEmitter;

/**
 * <p>Saxon extensions supporting DocBook verbatim environments</p>
 *
 * <p>$Id: Verbatim.java,v 1.1 2002/06/13 20:32:16 chorns Exp $</p>
 *
 * <p>Copyright (C) 2000 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://users.iclway.co.uk/mhkay/saxon/">Saxon</a>
 * implementation of two features that would be impractical to
 * implement directly in XSLT: line numbering and callouts.</p>
 *
 * <p><b>Line Numbering</b></p>
 * <p>The <tt>numberLines</tt> method takes a result tree
 * fragment (assumed to contain the contents of a formatted verbatim
 * element in DocBook: programlisting, screen, address, literallayout,
 * or synopsis) and returns a result tree fragment decorated with
 * line numbers.</p>
 *
 * <p><b>Callouts</b></p>
 * <p>The <tt>insertCallouts</tt> method takes an
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
 * @version $Id: Verbatim.java,v 1.1 2002/06/13 20:32:16 chorns Exp $
 *
 */
public class Verbatim {
  /** True if the stylesheet is producing formatting objects */
  private static boolean foStylesheet = false;
  /** The modulus for line numbering (every 'modulus' line is numbered). */
  private static int modulus = 0;
  /** The width (in characters) of line numbers (for padding). */
  private static int width = 0;
  /** The separator between the line number and the verbatim text. */
  private static String separator = "";

  /** True if callouts have been setup */
  private static boolean calloutsSetup = false;
  /** The default column for callouts that have only a line or line range */
  private static int defaultColumn = 60;
  /** The path to use for graphical callout decorations. */
  private static String graphicsPath = null;
  /** The extension to use for graphical callout decorations. */
  private static String graphicsExt = null;
  /** The largest callout number that can be represented graphically. */
  private static int graphicsMax = 10;

  /** The FormatCallout object to use for formatting callouts. */
  private static FormatCallout fCallout = null;

  /**
   * <p>Constructor for Verbatim</p>
   *
   * <p>All of the methods are static, so the constructor does nothing.</p>
   */
  public Verbatim() {
  }

  /**
   * <p>Find the string value of a stylesheet variable or parameter</p>
   *
   * <p>Returns the string value of <code>varName</code> in the current
   * <code>context</code>. Returns the empty string if the variable is
   * not defined.</p>
   *
   * @param context The current stylesheet context
   * @param varName The name of the variable (without the dollar sign)
   *
   * @return The string value of the variable
   */
  protected static String getVariable(Context context, String varName) {
    Value variable = null;
    String varString = null;

    try {
      variable = Extensions.evaluate(context, "$" + varName);
      varString = variable.asString();
      return varString;
    } catch (TransformerException te) {
      System.out.println("Undefined variable: " + varName);
      return "";
    } catch (IllegalArgumentException iae) {
      System.out.println("Undefined variable: " + varName);
      return "";
    }
  }

  /**
   * <p>Setup the parameters associated with line numbering</p>
   *
   * <p>This method queries the stylesheet for the variables
   * associated with line numbering. It is called automatically before
   * lines are numbered. The context is used to retrieve the values,
   * this allows templates to redefine these variables.</p>
   *
   * <p>The following variables are queried. If the variables do not
   * exist, builtin defaults will be used (but you may also get a bunch
   * of messages from the Java interpreter).</p>
   *
   * <dl>
   * <dt><code>linenumbering.everyNth</code></dt>
   * <dd>Specifies the lines that will be numbered. The first line is
   * always numbered. (builtin default: 5).</dd>
   * <dt><code>linenumbering.width</code></dt>
   * <dd>Specifies the width of the numbers. If the specified width is too
   * narrow for the largest number needed, it will automatically be made
   * wider. (builtin default: 3).</dd>
   * <dt><code>linenumbering.separator</code></dt>
   * <dd>Specifies the string that separates line numbers from lines
   * in the program listing. (builtin default: " ").</dd>
   * <dt><code>stylesheet.result.type</code></dt>
   * <dd>Specifies the stylesheet result type. The value is either 'fo'
   * (for XSL Formatting Objects) or it isn't. (builtin default: html).</dd>
   * </dl>
   *
   * @param context The current stylesheet context
   *
   */
  private static void setupLineNumbering(Context context) {
    // Hardcoded defaults
    modulus = 5;
    width = 3;
    separator = " ";
    foStylesheet = false;

    String varString = null;

    // Get the modulus
    varString = getVariable(context, "linenumbering.everyNth");
    try {
      modulus = Integer.parseInt(varString);
    } catch (NumberFormatException nfe) {
      System.out.println("$linenumbering.everyNth is not a number: " + varString);
    }

    // Get the width
    varString = getVariable(context, "linenumbering.width");
    try {
      width = Integer.parseInt(varString);
    } catch (NumberFormatException nfe) {
      System.out.println("$linenumbering.width is not a number: " + varString);
    }

    // Get the separator
    varString = getVariable(context, "linenumbering.separator");
    separator = varString;

    // Get the stylesheet type
    varString = getVariable(context, "stylesheet.result.type");
    foStylesheet = (varString.equals("fo"));
  }

  /**
   * <p>Number lines in a verbatim environment</p>
   *
   * <p>The extension function expects the following variables to be
   * available in the calling context: $linenumbering.everyNth,
   * $linenumbering.width, $linenumbering.separator, and
   * $stylesheet.result.type.</p>
   *
   * <p>This method adds line numbers to a result tree fragment. Each
   * newline that occurs in a text node is assumed to start a new line.
   * The first line is always numbered, every subsequent 'everyNth' line
   * is numbered (so if everyNth=5, lines 1, 5, 10, 15, etc. will be
   * numbered. If there are fewer than everyNth lines in the environment,
   * every line is numbered.</p>
   *
   * <p>Every line number will be right justified in a string 'width'
   * characters long. If the line number of the last line in the
   * environment is too long to fit in the specified width, the width
   * is automatically increased to the smallest value that can hold the
   * number of the last line. (In other words, if you specify the value 2
   * and attempt to enumerate the lines of an environment that is 100 lines
   * long, the value 3 will automatically be used for every line in the
   * environment.)</p>
   *
   * <p>The 'separator' string is inserted between the line
   * number and the original program listing. Lines that aren't numbered
   * are preceded by a 'width' blank string and the separator.</p>
   *
   * <p>If inline markup extends across line breaks, markup changes are
   * required. All the open elements are closed before the line break and
   * "reopened" afterwards. The reopened elements will have the same
   * attributes as the originals, except that 'name' and 'id' attributes
   * are not duplicated if the stylesheet.result.type is "html" and
   * 'id' attributes will not be duplicated if the result type is "fo".</p>
   *
   * @param rtf The result tree fragment of the verbatim environment.
   *
   * @return The modified result tree fragment.
   */
  public static NodeSetValue numberLines (Context context,
					  NodeSetValue rtf_ns) {

    FragmentValue rtf = (FragmentValue) rtf_ns;

    setupLineNumbering(context);

    try {
      LineCountEmitter lcEmitter = new LineCountEmitter();
      rtf.replay(lcEmitter);
      int numLines = lcEmitter.lineCount();

      int listingModulus = numLines < modulus ? 1 : modulus;

      double log10numLines = Math.log(numLines) / Math.log(10);

      int listingWidth = width < log10numLines+1
	? (int) Math.floor(log10numLines + 1)
	: width;

      Controller controller = context.getController();
      NamePool namePool = controller.getNamePool();
      NumberLinesEmitter nlEmitter = new NumberLinesEmitter(controller,
							    namePool,
							    listingModulus,
							    listingWidth,
							    separator,
							    foStylesheet);
      rtf.replay(nlEmitter);
      return nlEmitter.getResultTreeFragment();
    } catch (TransformerException e) {
      // This "can't" happen.
      System.out.println("Transformer Exception in numberLines");
      return rtf;
    }
  }

  /**
   * <p>Setup the parameters associated with callouts</p>
   *
   * <p>This method queries the stylesheet for the variables
   * associated with line numbering. It is called automatically before
   * callouts are processed. The context is used to retrieve the values,
   * this allows templates to redefine these variables.</p>
   *
   * <p>The following variables are queried. If the variables do not
   * exist, builtin defaults will be used (but you may also get a bunch
   * of messages from the Java interpreter).</p>
   *
   * <dl>
   * <dt><code>callout.graphics</code></dt>
   * <dd>Are we using callout graphics? A value of 0 or "" is false,
   * any other value is true. If callout graphics are not used, the
   * parameters related to graphis are not queried.</dd>
   * <dt><code>callout.graphics.path</code></dt>
   * <dd>Specifies the path to callout graphics.</dd>
   * <dt><code>callout.graphics.extension</code></dt>
   * <dd>Specifies the extension ot use for callout graphics.</dd>
   * <dt><code>callout.graphics.number.limit</code></dt>
   * <dd>Identifies the largest number that can be represented as a
   * graphic. Larger callout numbers will be represented using text.</dd>
   * <dt><code>callout.defaultcolumn</code></dt>
   * <dd>Specifies the default column for callout bullets that do not
   * specify a column.</dd>
   * <dt><code>stylesheet.result.type</code></dt>
   * <dd>Specifies the stylesheet result type. The value is either 'fo'
   * (for XSL Formatting Objects) or it isn't. (builtin default: html).</dd>
   * </dl>
   *
   * @param context The current stylesheet context
   *
   */
  private static void setupCallouts(Context context) {
    NamePool namePool = context.getController().getNamePool();

    boolean useGraphics = false;
    boolean useUnicode = false;

    int unicodeStart = 49;
    int unicodeMax = 0;

    String unicodeFont = "";

    // Hardcoded defaults
    defaultColumn = 60;
    graphicsPath = null;
    graphicsExt = null;
    graphicsMax = 0;
    foStylesheet = false;
    calloutsSetup = true;

    Value variable = null;
    String varString = null;

    // Get the stylesheet type
    varString = getVariable(context, "stylesheet.result.type");
    foStylesheet = (varString.equals("fo"));

    // Get the default column
    varString = getVariable(context, "callout.defaultcolumn");
    try {
      defaultColumn = Integer.parseInt(varString);
    } catch (NumberFormatException nfe) {
      System.out.println("$callout.defaultcolumn is not a number: "
			 + varString);
    }

    // Use graphics at all?
    varString = getVariable(context, "callout.graphics");
    useGraphics = !(varString.equals("0") || varString.equals(""));

    // Use unicode at all?
    varString = getVariable(context, "callout.unicode");
    useUnicode = !(varString.equals("0") || varString.equals(""));

    if (useGraphics) {
      // Get the graphics path
      varString = getVariable(context, "callout.graphics.path");
      graphicsPath = varString;

      // Get the graphics extension
      varString = getVariable(context, "callout.graphics.extension");
      graphicsExt = varString;

      // Get the number limit
      varString = getVariable(context, "callout.graphics.number.limit");
      try {
	graphicsMax = Integer.parseInt(varString);
      } catch (NumberFormatException nfe) {
	System.out.println("$callout.graphics.number.limit is not a number: "
			   + varString);
	graphicsMax = 0;
      }

      fCallout = new FormatGraphicCallout(namePool,
					  graphicsPath,
					  graphicsExt,
					  graphicsMax,
					  foStylesheet);
    } else if (useUnicode) {
      // Get the starting character
      varString = getVariable(context, "callout.unicode.start.character");
      try {
	unicodeStart = Integer.parseInt(varString);
      } catch (NumberFormatException nfe) {
	System.out.println("$callout.unicode.start.character is not a number: "
			   + varString);
	unicodeStart = 48;
      }

      // Get the number limit
      varString = getVariable(context, "callout.unicode.number.limit");
      try {
	unicodeMax = Integer.parseInt(varString);
      } catch (NumberFormatException nfe) {
	System.out.println("$callout.unicode.number.limit is not a number: "
			   + varString);
	unicodeStart = 0;
      }

      // Get the font
      unicodeFont = getVariable(context, "callout.unicode.font");
      if (unicodeFont == null) {
	unicodeFont = "";
      }

      fCallout = new FormatUnicodeCallout(namePool,
					  unicodeFont,
					  unicodeStart,
					  unicodeMax,
					  foStylesheet);
    } else {
      fCallout = new FormatTextCallout(namePool, foStylesheet);
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
   * number will be used, surrounded by parenthesis. Callout numbers may
   * also be represented as graphics. Callouts are
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
   * <p>If graphical callouts are used, and the callout number is less
   * than or equal to the $callout.graphics.number.limit, the following image
   * will be generated for HTML:
   *
   * <pre>
   * &lt;img src="$callout.graphics.path/999$callout.graphics.ext"
   *         alt="conumber">
   * </pre>
   *
   * If the $stylesheet.result.type is 'fo', the following image will
   * be generated:
   *
   * <pre>
   * &lt;fo:external-graphic src="$callout.graphics.path/999$callout.graphics.ext"/>
   * </pre>
   *
   * <p>If the callout number exceeds $callout.graphics.number.limit,
   * the callout will be the callout number surrounded by
   * parenthesis.</p>
   *
   * @param context The stylesheet context.
   * @param areaspecNodeSet The source node set that contains the areaspec.
   * @param rtf The result tree fragment of the verbatim environment.
   *
   * @return The modified result tree fragment.
   */

  public static NodeSetValue insertCallouts (Context context,
					     NodeList areaspecNodeList,
					     NodeSetValue rtf_ns) {

    FragmentValue rtf = (FragmentValue) rtf_ns;

    setupCallouts(context);

    try {
      Controller controller = context.getController();
      NamePool namePool = controller.getNamePool();
      CalloutEmitter cEmitter = new CalloutEmitter(controller,
						   namePool,
						   defaultColumn,
						   foStylesheet,
						   fCallout);
      cEmitter.setupCallouts(areaspecNodeList);
      rtf.replay(cEmitter);
      return cEmitter.getResultTreeFragment();
    } catch (TransformerException e) {
      // This "can't" happen.
      System.out.println("Transformer Exception in insertCallouts");
      return rtf;
    }
  }
}
