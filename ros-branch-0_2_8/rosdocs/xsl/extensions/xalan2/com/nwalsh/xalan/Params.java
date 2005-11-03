// Params.java - Read stylesheet parameters in Xalan

package com.nwalsh.xalan;

import org.apache.xpath.objects.XObject;
import org.apache.xpath.XPathContext;
import org.apache.xalan.extensions.ExpressionContext;
import org.apache.xml.utils.QName;

import javax.xml.transform.TransformerException;

public class Params {

  public static String getString(ExpressionContext context,
				 String varName) {
    try {
      XObject var = context.getVariableOrParam(new QName(varName));
      if (var != null) {
	return var.toString();
      } else {
	System.out.println("$" + varName + " is not a defined parameter.");
	return "";
      }
    } catch (TransformerException te) {
      System.out.println("Transformer exception getting value of $" + varName);
      return "";
    }
  }

  public static int getInt(ExpressionContext context,
			   String varName) {
    String stringValue = getString(context, varName);
    if (stringValue != null) {
      try {
	int value = Integer.parseInt(stringValue);
	return value;
      } catch (NumberFormatException e) {
	System.out.println("$" + varName + " is not an integer.");
      }
    }
    return 0;
  }

  public static boolean getBoolean(ExpressionContext context,
				   String varName) {
    String stringValue = getString(context, varName);
    if (stringValue != null) {
      if (stringValue.equals("0") || stringValue.equals("")) {
	return false;
      } else {
	return true;
      }
    } else {
      return false;
    }
  }
}
