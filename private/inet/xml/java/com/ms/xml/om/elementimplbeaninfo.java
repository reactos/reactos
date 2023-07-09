package com.ms.xml.om;
import java.beans.*;
import java.lang.reflect.Method;

/**
 * This class provides specialized JavaBeans support for the
 * <code>ElementImpl</code> class to resolve which overloaded functions
 * are accessible through scripting.
 */
public class ElementImplBeanInfo extends SimpleBeanInfo {
    private final static Class beanClass =
        ElementImpl.class;

/**
 * Returns the method descriptions for the methods that are 
 * to be made available through scripting.
 * @return an array of <code>MethodDescriptor</code> objects.
 */
    public MethodDescriptor[] getMethodDescriptors() 
    {
        // First find the "method" objects.
        Method numElementMethod;
        Method addChildMethod;
        Method getChildMethod;
        Method removeChildMethod;
        Method numAttributesMethod;
        Method getAttributeMethod;
        Method setAttributeMethod;
        Method removeAttributeMethod;
        Method qualifyNameMethod;
//        Method saveMethod;
        
        Class args[] = { };
        Class addChildArgs[] = { Element.class, int.class, int.class };
        Class getChildArgs[] = { int.class };
        Class removeChildArgs[] = { Element.class };
        Class getAttributeArgs[] = { String.class };
        Class setAttributeArgs[] = { String.class , Object.class };
        Class qualifyNameArgs[] = { String.class };
//        Class saveArgs[] = { XMLOutputStream.class };

        try {
            numElementMethod = ElementImpl.class.getMethod("numElements", args);
            addChildMethod = ElementImpl.class.getMethod("addChild", addChildArgs);
            getChildMethod = ElementImpl.class.getMethod("getChild", getChildArgs);
            removeChildMethod = ElementImpl.class.getMethod("removeChild", removeChildArgs);
            numAttributesMethod = ElementImpl.class.getMethod("numAttributes", args);
            getAttributeMethod = ElementImpl.class.getMethod("getAttribute", getAttributeArgs);
            setAttributeMethod = ElementImpl.class.getMethod("setAttribute", setAttributeArgs);
            removeAttributeMethod = ElementImpl.class.getMethod("removeAttribute", getAttributeArgs);
            qualifyNameMethod = ElementImpl.class.getMethod("qualifyName", qualifyNameArgs);
//            saveMethod = ElementImpl.class.getMethod("save", saveArgs);
        } catch (Exception ex) {
            // "should never happen"
            throw new Error("Missing method: " + ex);
        }

        // Now create the MethodDescriptor array
        // with visible event response methods:
        MethodDescriptor result[] = { 
            new MethodDescriptor(numElementMethod),
            new MethodDescriptor(addChildMethod),
            new MethodDescriptor(getChildMethod),
            new MethodDescriptor(removeChildMethod),
            new MethodDescriptor(numAttributesMethod),
            new MethodDescriptor(getAttributeMethod),
            new MethodDescriptor(setAttributeMethod),
            new MethodDescriptor(removeAttributeMethod),
            new MethodDescriptor(qualifyNameMethod)
//            new MethodDescriptor(saveMethod)
        };          

        return result;
    }

}
