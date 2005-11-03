package com.nwalsh.xalan;

import java.io.*;
import java.awt.*;
import java.awt.image.*;
import java.lang.Thread;
import java.util.StringTokenizer;

import org.apache.xalan.extensions.ExpressionContext;

/**
 * <p>Saxon extension to examine intrinsic size of images</p>
 *
 * <p>$Id: ImageIntrinsics.java,v 1.1 2002/06/13 20:32:18 chorns Exp $</p>
 *
 * <p>Copyright (C) 2002 Norman Walsh.</p>
 *
 * <p>This class provides a
 * <a href="http://xml.apache.org/">Xalan</a>
 * extension to find the intrinsic size of images.</p>
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
 * @version $Id: ImageIntrinsics.java,v 1.1 2002/06/13 20:32:18 chorns Exp $
 *
 */
public class ImageIntrinsics implements ImageObserver {
  boolean imageLoaded = false;
  boolean imageFailed = false;
  Image image = null;
  int width = -1;
  int depth = -1;

  /**
   * <p>Constructor for ImageIntrinsics</p>
   */
  public ImageIntrinsics(ExpressionContext context, String imageFn) {
    image = Toolkit.getDefaultToolkit().getImage (imageFn);
    width = image.getWidth(this);

    while (!imageFailed && (width == -1 || depth == -1)) {
      try {
	java.lang.Thread.currentThread().sleep(50);
      } catch (Exception e) {
	// nop;
      }
      width = image.getWidth(this);
      depth = image.getHeight(this);
    }

    if (imageFailed) {
      // Maybe it's an EPS or PDF?
      // FIXME: this code is crude
      BufferedReader ir = null;
      String line = null;
      int lineLimit = 100;

      try {
	ir = new BufferedReader(new FileReader(new File(imageFn)));
	line = ir.readLine();

	if (line != null && line.startsWith("%PDF-")) {
	  // We've got a PDF!
	  while (lineLimit > 0 && line != null) {
	    lineLimit--;
	    if (line.startsWith("/CropBox [")) {
	      line = line.substring(10);
	      if (line.indexOf("]") >= 0) {
		line = line.substring(0, line.indexOf("]"));
	      }
	      parseBox(line);
	      lineLimit = 0;
	    }
	    line = ir.readLine();
	  }
	} else if (line != null && line.startsWith("%!") && line.indexOf(" EPSF-") > 0) {
	  // We've got an EPS!
	  while (lineLimit > 0 && line != null) {
	    lineLimit--;
	    if (line.startsWith("%%BoundingBox: ")) {
	      line = line.substring(15);
	      parseBox(line);
	      lineLimit = 0;
	    }
	    line = ir.readLine();
	  }
	}
      } catch (Exception e) {
	// nop;
      }

      if (ir != null) {
	try {
	  ir.close();
	} catch (Exception e) {
	  // nop;
	}
      }
    }
  }

  public int getWidth(ExpressionContext context, int defaultWidth) {
    if (width >= 0) {
      return width;
    } else {
      return defaultWidth;
    }
  }

  public int getDepth(ExpressionContext context, int defaultDepth) {
    if (depth >= 0) {
      return depth;
    } else {
      return defaultDepth;
    }
  }

  private void parseBox(String line) {
    int [] corners = new int [4];
    int count = 0;

    StringTokenizer st = new StringTokenizer(line);
    while (count < 4 && st.hasMoreTokens()) {
      try {
	corners[count++] = Integer.parseInt(st.nextToken());
      } catch (Exception e) {
	// nop;
      }
    }

    width = corners[2] - corners[0];
    depth = corners[3] - corners[1];
  }

  public boolean imageUpdate(Image img, int infoflags,
			     int x, int y, int width, int height) {
    if ((infoflags & ImageObserver.ERROR) == ImageObserver.ERROR) {
      imageFailed = true;
      return false;
    }

    // I really only care about the width and height, but if I return false as
    // soon as those are available, the BufferedInputStream behind the loader
    // gets closed too early.
    int flags = ImageObserver.ALLBITS;
    if ((infoflags & flags) == flags) {
      return false;
    } else {
      return true;
    }
  }
}
