package com.nwalsh.saxon;

import org.w3c.dom.*;

/**
 * <p>A class for maintaining information about callouts.</p>
 *
 * <p>To make processing callouts easier, they are parsed out of the
 * input structure and stored in a sorted array. (The array is sorted
 * according to the order in which the callouts occur.)</p>
 *
 * <p>This class is just the little record
 * that we store in the array for each callout.</p>
 */
public class Callout implements Comparable {
  /** The callout number. */
  private int callout = 0;
  /** The area Element item that generated this callout. */
  private Element area = null;
  /** The line on which this callout occurs. */
  private int line = 0;
  /** The column in which this callout appears. */
  private int col = 0;

  /** The constructor; initialize the private data structures. */
  public Callout(int callout, Element area, int line, int col) {
    this.callout = callout;
    this.area = area;
    this.line = line;
    this.col = col;
  }

  /**
   * <p>The compareTo method compares this Callout with another.</p>
   *
   * <p>Given two Callouts, A and B, A < B if:</p>
   *
   * <ol>
   * <li>A.line < B.line, or</li>
   * <li>A.line = B.line && A.col < B.col, or</li>
   * <li>A.line = B.line && A.col = B.col && A.callout < B.callout</li>
   * <li>Otherwise, they're equal.</li>
   * </ol>
   */
  public int compareTo (Object o) {
    Callout c = (Callout) o;

    if (line == c.getLine()) {
	if (col > c.getColumn()) {
	  return 1;
	} else if (col < c.getColumn()) {
	  return -1;
	} else {
	  if (callout < c.getCallout()) {
	    return -1;
	  } else if (callout > c.getCallout()) {
	    return 1;
	  } else {
	    return 0;
	  }
	}
    } else {
	if (line > c.getLine()) {
	  return 1;
	} else {
	  return -1;
	}
    }
  }

  /** Access the Callout's area. */
  public Element getArea() {
    return area;
  }

  /** Access the Callout's line. */
  public int getLine() {
    return line;
  }

  /** Access the Callout's column. */
  public int getColumn() {
    return col;
  }

  /** Access the Callout's callout number. */
  public int getCallout() {
    return callout;
  }
}
