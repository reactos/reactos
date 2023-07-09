
import com.ms.xml.parser.*;
import com.ms.xml.om.Document;
import com.ms.xml.om.Element;
import com.ms.xml.util.XMLOutputStream;
import com.ms.xml.util.Name;
import com.ms.xml.om.ElementEnumeration;
import com.ms.xml.om.ElementImpl;

import java.util.Enumeration;
import java.io.*;
import java.io.PrintStream;
import java.net.*;


import com.ms.xml.util.Name;
import com.ms.xml.om.ElementFactory;

//-------------------------------------------------------------------
class NullElementFactory implements ElementFactory
{
    public NullElementFactory()
    {
    }

    public Element createElement(Element parent, int type, Name tag, String text)
    {
        return null;
    }

    public void parsed(Element elem) 
    {
    }

    public void parsedAttribute(Element e, Name name, Object value)
    {
    }
}    


//-------------------------------------------------------------------
class msxml
{

    public static void main(String args[])
    {        
        parseArgs(args);
        if (fileName == null && omtest == false) 
        {
            printUsage(System.out);
        } 
        else 
        {
            if (omtest) omTest();
            if (fileName == null) return;

            URL url = createURL(fileName);    

            Document d = null;
            try 
            {
                for (long i = 0; i < loops; i++) 
                {
                    if (fast)
                        d = new Document(new NullElementFactory()); 
                    else
                        d = new Document(); 
                    Start();
                    InputStream instream = null;
                    d.setCaseInsensitive(caseInsensitive);
                    d.setLoadExternal(loadexternal);
					d.setShortEndTags(shortendtags);
                    if (stream) 
                    {
                        try 
                        {
                            instream = url.openStream();
                        } 
                        catch (IOException e) 
                        {
                            System.out.println("Error opening input stream for \"" + 
                                url.toString() + "\": " + e.toString());
                            System.exit(0);
                        }
                        d.load(instream);
                    } 
                    else 
                    {
                        d.load(url);
                    }
                    Stop();
                    if (i < loops - 1)
                    {
                        // Force garbage collection now.
                        d = null;
                        System.gc();
                    }
                }
//                if (timeit) reportTimes(System.out);

            } 
            catch (ParseException e) 
            {
                d.reportError(e, System.out);
                d = null; // don't dump tree then.
            }

            if (d != null) 
            {
                if (tree) 
                {
                    try 
                    {
                        dumpTree(d,new PrintStream(out),"");
                    } 
                    catch( IOException e ) 
                    {
                        System.out.println( "Error saving output stream." );
                    }
                } 
                else if (schema)
                {
                    try 
                    {
                        dumpSchema(d,d.createOutputStream(out));
                    } 
                    catch( IOException e ) 
                    {
                        System.out.println( "Error saving output stream." );
                    }
                }
                else if (output ) 
                {
                    if( outputEncoding != null )                    
                        d.setEncoding( outputEncoding );
                    d.setOutputStyle(style);
                    try 
                    {             
                        XMLOutputStream xout = d.createOutputStream(out);
                        d.save(xout);
                    } 
                    catch( IOException e ) 
                    {
                        System.out.println( "Error saving output stream." );
                    }
                }

//                System.out.println(d.getText());
            }
        }   
        System.exit(0);
    } 

    static void printUsage(PrintStream o)
    {
        o.println("Usage:  jview msxml [-d|-d1|-d2|-f] [-t num] [-e encoding] [-o filename] [-c|-p] [-i] [-m] [-s] filename");
        o.println(); 
        o.println("Version: 1.0.9");
        o.println(); 
        o.println("This program parses the given XML file and optionally dumps");
        o.println("the resulting data structures to standard output.\n");
        o.println("Arguments:");
        o.println("-d\tWrite parsed XML in original XML format.");
        o.println("-d1\tWrite parsed XML in a tree format.");
        o.println("-d2\tWrite schema representation of DTD.");
        o.println("-f\tFast parsing that bypasses tree building.");
        o.println("-t\tSpecifies how many iterations to time the parser.");
        o.println("-o\tProvides a filename for dumping output.");
        o.println("\t(This is needed if data is in Unicode.)");
        o.println("-e\tCharacter encoding for output other than that of the input.");
        o.println("-c\tOutput in compact mode (no newlines or tabs).");
        o.println("-p\tOutput in pretty mode (inserts newlines & tabs).");
        o.println("-i\tSwitch to case insensitive.");
        o.println("-m\tPerform object model test, which creates test file 'test.xml'.");
        o.println("-s\tStandalone - do not load external DTD's or entities.\n");
		o.println("-x\tShort end tags");
    }
    
    static void parseArgs(String args[])
    {
        for (int i = 0; i < args.length; i++)
        {
            if (args[i].equals("-d")) 
            {
                output = true;
            } 
            else if (args[i].equals("-d1")) 
            {
                tree = true;
                output = true;
            } 
            else if (args[i].equals("-d2")) 
            {
                schema = true;
                output = true;
            } 
            else if (args[i].equals("-t")) 
            {
                i++;
                Integer value = new Integer(args[i]);
                loops = value.intValue();
                timeit = true;
            } 
            else if (args[i].equals("-i")) 
            {
                caseInsensitive = true;
            } 
            else if (args[i].equals("-f")) 
            {
                fast = true;
            } 
            else if (args[i].equals("-o")) 
            {
                i++;
                try 
                {
                    out = new FileOutputStream( args[i] );
                } 
                catch( IOException e ) 
                {
                    out = System.out;
                }
            } 
            else if (args[i].equals("-e")) 
            {
                i++;
                outputEncoding = args[i];
            } 
            else if (args[i].equals("-c")) 
            {
                style = XMLOutputStream.COMPACT;
            } 
            else if (args[i].equals("-p")) 
            {
                style = XMLOutputStream.PRETTY;
            } 
            else if (args[i].equals("-m")) 
            {
                omtest = true;
            } 
            else if (args[i].equals("-s")) 
            {
                loadexternal = false;
            } 
            else if (args[i].equals("-x"))
			{
				shortendtags = true;
			}
			else
            {
                fileName = args[i];
            }
        }        
    }
    
    static void Start() 
    { 
        start = System.currentTimeMillis(); 
    }

    static void Stop() 
    { 
        long end = System.currentTimeMillis(); 
        long time = end-start;
        if (timeit)
            System.out.println("time=" + time); //  + " created=" + Name.created + " reused=" + Name.reused);
//        Name.created = 0;
//        Name.reused = 0;
        if (min == 0 || time < min) min = time;
        if (max == 0 || time > max) max = time;
        sum += (end-start);
        count++;
    }

    static void reportTimes(PrintStream o) 
    {
        o.println("Parsed " + count + " times:");
        o.println("\tMin="+ min + " milliseconds.");
        o.println("\tMax="+ max + " milliseconds.");
        o.println("\tAverage="+ (sum/count) + " milliseconds.");
    }

    static URL createURL(String fileName) 
    {
        URL url = null;
        try 
        {
            url = new URL(fileName);
        } 
        catch (MalformedURLException ex) 
        {
            File f = new File(fileName);
            try 
            {
                String path = f.getAbsolutePath();
                // This is a bunch of weird code that is required to
                // make a valid URL on the Windows platform, due
                // to inconsistencies in what getAbsolutePath returns.
                String fs = System.getProperty("file.separator");
                if (fs.length() == 1) 
                {
                    char sep = fs.charAt(0);
                    if (sep != '/')
                        path = path.replace(sep, '/');
                    if (path.charAt(0) != '/')
                        path = '/' + path;
                }
                path = "file://" + path;
                url = new URL(path);
            } 
            catch (MalformedURLException e) 
            {
                System.out.println("Cannot create url for: " + fileName);
                System.exit(0);
            }
        }
        return url;
    }

    // dumpTree dumps a tree out in the following format:
    // 
    // root
    // |---node
    // |---node2
    // |   |---foo
    // |   +---bar.
    // +---lastnode
    //     |---apple
    //     +---orange.
    //

    static void dumpTree(Element e, PrintStream o, String indent) throws IOException
    {
        if (indent.length() > 0)
        {
            o.print(indent + "---");
        }
        // Once we've printed the '+', from then on we are
        // to print a blank space, since the '+' means we've
        // reached the end of that branch.
        String lines = indent.replace('+',' ');
        boolean dumpText = false;
        boolean dumpTagName = true;
        boolean dumpAttributes = false;

        switch (e.getType()) {
        case Element.CDATA:
            o.print("CDATA");
            dumpText = true;
            break;
        case Element.COMMENT:
            o.print("COMMENT");
            break;
        case Element.DOCUMENT:
            o.print("DOCUMENT");
            break;
        case Element.DTD:
            o.print("DOCTYPE");
            dumpAttributes = true;
            dumpTagName = false;            
            break;
        case Element.ELEMENT:
            o.print("ELEMENT");
            break;
        case Element.ENTITY:
            o.print(e.getTagName().getName());
            dumpTagName = false;
            if (e.numElements() == 0) dumpText = true;
            break;
        case Element.ENTITYREF:
            o.print("ENTITYREF");
            dumpText = true;
            break;
        case Element.NOTATION:
            o.print("NOTATION");
            dumpText = true;
            break;
        case Element.ELEMENTDECL:
            o.print("ELEMENTDECL");
            break;
        case Element.PCDATA:
            o.print("PCDATA");
            dumpText = true;
            break;
        case Element.PI:
            if (e.getTagName().getName().toString().equals("XML"))
            {
                o.print("XMLDECL");
                dumpAttributes = true;
                dumpTagName = false;
            } else {
                o.print("PI");
                dumpText = true;
            }
            break;
        case Element.NAMESPACE:
            o.print("NAMESPACE");
            dumpAttributes = true;
            dumpTagName = false;
            break;
        case Element.WHITESPACE:
            {
                o.print("WHITESPACE");
                String text = e.getText();
                int len = text.length();
                for (int i = 0; i < len; i++) 
                {
                    int c = text.charAt(i);
                    o.print(" 0x" + Integer.toHexString(c));
                }
                dumpAttributes = false;
                dumpTagName = false;
            }
            break;
        }
            
        if (dumpTagName) {
            Name n = e.getTagName();
            if (n != null) 
            {
                o.print(" " + n.toString());
            }
        }
        if (e.getType() == Element.ENTITY) 
        {
            Entity en = (Entity)e;
            o.print(" " + en.getName());
        } 
        else if (e.getType() == Element.ELEMENTDECL)
        {
            ElementDecl ed = (ElementDecl)e;
            o.print(" ");
            XMLOutputStream out = new XMLOutputStream(o);
            ed.getContent().save(null, out);
            out.flush();
        }
        if (dumpAttributes)
        {
            o.print(" ");
            XMLOutputStream s = new XMLOutputStream(o);
            ((ElementImpl)e).saveAttributes(e.getTagName().getNameSpace(), s);
            s.flush();
        }

        if (dumpText && e.getText() != null) 
        {
            o.print(" \"" + e.getText() + "\"");
        }
        o.println("");

        String newLines = "";
        if (lines.length() > 0) 
        {
            newLines = lines + "   |";
        } 
        else 
        {
            newLines = "|";
        }
        for (ElementEnumeration en = new ElementEnumeration(e); en.hasMoreElements(); ) 
        {
            Element child = (Element)en.nextElement();
            if (! en.hasMoreElements()) 
            {
                if (lines.length() > 0) 
                {
                    newLines = lines + "   +";
                } 
                else 
                {
                    newLines = "+";
                }
            }
            dumpTree(child,o,newLines);
        }
    }

    static void dumpSchema(Document d, XMLOutputStream s) throws IOException
    {
        Element schema = d.getDTD().getSchema();
        s.setOutputStyle(XMLOutputStream.PRETTY);
        schema.save(s);
    }

    static void omTest()
    {
        try {
            Document d = new Document();
            d.setStandalone("yes");
            d.createElement(d,Element.WHITESPACE,null,"\r\n");
            Element root = d.createElement(d,Element.ELEMENT,Name.create("ROOT"),null);
            d.createElement(root,Element.WHITESPACE,null,"\r\n");
            Element child = d.createElement(root,Element.ELEMENT,Name.create("CHILD"),null);
            d.createElement(child,Element.PCDATA,null,"This is a test");
            d.createElement(child,Element.WHITESPACE,null,"\r\n");
            d.createElement(child,Element.COMMENT,null,"comment goes here");
            d.createElement(child,Element.WHITESPACE,null,"\r\n");
            d.createElement(child,Element.PI,Name.create("PI"),"processing instruction");
            d.createElement(child,Element.WHITESPACE,null,"\r\n");

            FileOutputStream out = new FileOutputStream("test.xml");
            d.save(d.createOutputStream(out));
        } catch (Exception e)
        {
            System.out.println("Exception writing text.xml: " + e.toString());
        }
    }

    static String fileName;    
    static boolean output = false;
    static boolean tree = false;
    static boolean schema = false;
    static int loops = 1;
    static boolean timeit = false;
    static boolean caseInsensitive = false;
    static boolean stream = false;
    static long start = 0;
    static long min =0;
    static long max = 0;
    static long count = 0;
    static long sum = 0;
    static int style = 0;
    static OutputStream out = System.out;
    static String outputEncoding = null;
    static boolean fast = false;
    static boolean omtest = false;
    static boolean loadexternal = true;
	static boolean shortendtags = false;
}    
