package com.ms.xml.om;
import java.beans.*;
import java.lang.reflect.Method;

/**
 * This class provides specialized JavaBeans support for the
 * <code>Document</code> class to resolve which overloaded functions
 * are accessible through scripting.
 */
public class DocumentBeanInfo extends SimpleBeanInfo {
    private final static Class beanClass =
        Document.class;

/**
 * Returns the method descriptions for the methods that are 
 * to be made available through scripting.
 * @return an array of <code>MethodDescriptor</code> objects.
*/
    public MethodDescriptor[] getMethodDescriptors() 
    {
        // First find the "method" objects.
        Method addChildMethod;
        Method removeChildMethod;
        Method createElement1Method;  
        Method createElement2Method;  
        Method parsedMethod;  
        Method loadMethod;  
//        Method reportErrorMethod;  
//        Method createOutputStreamMethod;  
//        Method saveMethod;  
        Method elementDeclarationsMethod;  
        Method clearMethod;  

        Class args[] = { };
        Class addChildArgs[] = { Element.class, Element.class };
        Class elementArgs[] = { Element.class };
        Class createElement1Args[] = { int.class, String.class };
        Class createElement2Args[] = { int.class };
        Class loadArgs[] = { String.class };
//        Class reportErrorArgs[] = { ParseException.class, OutputStream.class };
//        Class createOutputStreamArgs[] = { OutputStream.class };
//        Class saveArgs[] = { XMLOutputStream.class };

        try {
            addChildMethod = Document.class.getMethod("addChild", addChildArgs);
            removeChildMethod = Document.class.getMethod("removeChild", elementArgs);
            createElement1Method = Document.class.getMethod("createElement", createElement1Args);
            createElement2Method = Document.class.getMethod("createElement", createElement2Args);
            parsedMethod = Document.class.getMethod("parsed", elementArgs);
            loadMethod = Document.class.getMethod("load", loadArgs);
//            reportErrorMethod = Document.class.getMethod("reportError", reportErrorArgs);
//            createOutputStreamMethod = Document.class.getMethod("createOutputStream", createOutputStreamArgs);
//            saveMethod = Document.class.getMethod("save", saveArgs);
            elementDeclarationsMethod = Document.class.getMethod("elementDeclarations", args);
            clearMethod = Document.class.getMethod("clear", args);        
        } catch (Exception ex) {
            // "should never happen"
            throw new Error("Missing method: " + ex);
        }

        // Now create the MethodDescriptor array
        // with visible event response methods:
        MethodDescriptor result[] = { 
            new MethodDescriptor(addChildMethod),
            new MethodDescriptor(removeChildMethod),
            new MethodDescriptor(createElement1Method),
            new MethodDescriptor(createElement2Method),
            new MethodDescriptor(parsedMethod),
            new MethodDescriptor(loadMethod),
//            new MethodDescriptor(reportErrorMethod),
//            new MethodDescriptor(createOutputStreamMethod),
//            new MethodDescriptor(saveMethod),
            new MethodDescriptor(elementDeclarationsMethod),
            new MethodDescriptor(clearMethod),
        };          

        return result;
    }

}
