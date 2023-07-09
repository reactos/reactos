/*
 * @(#)ContentModel.java 1.0 6/3/97
 * 
 * Copyright (c) 1997 Microsoft, Corp. All Rights Reserved.
 * 
 */
 
package com.ms.xml.parser;

import com.ms.xml.om.Element;
import com.ms.xml.om.ElementImpl;
import com.ms.xml.util.Name;
import com.ms.xml.util.Atom;
import com.ms.xml.util.XMLOutputStream;
import com.ms.xml.parser.ElementDecl;
import com.ms.xml.parser.Entity;

import java.lang.String;
import java.util.Vector;
import java.util.BitSet;
import java.util.Hashtable;
import java.util.Enumeration;
import java.io.IOException;

/**
 * This class represents the content model definition for a given
 * XML element. The content model is defined in the element
 * declaration in the Document Type Definition (DTD); for example, 
 * (a,(b|c)*,d). The
 * content model is stored in an expression tree of <code>Node</code> objects
 * for use by the XML parser during validation.
 * 
 */
public class ContentModel
{
    /**
     * content model
     * points to syntax tree 
     * @see Node
     */    
    Node content;

    /**
     * end node
     */
    Terminal end;

    /** 
     * terminal nodes
     */
    Vector terminalnodes;

    /**
     * unique terminal names
     */
    Hashtable symboltable;

    /**
     * symbol array
    */
    Vector symbols;

    /**
     * transition table
     */
    Vector Dtrans;

    /**
     * content type
     */    
    public static final byte EMPTY       = 1;
    public static final byte ANY         = 2;
    public static final byte ELEMENTS    = 4;
    byte type;

    public byte getType()
    {
        return type;
    }

    /**
     * Retrieves a string representation of the content type.
     * @return a <A>String</A>. 
     */    
    public String toString()
    {
        String s;
        switch(type)
        {
            case EMPTY:
                s = "EMPTY";
                break;
            case ANY:
                s = "ANY";
                break;
            case ELEMENTS:
                s = "ELEMENTS";
                break;
            default:
                s = "*UNKNOWN*";
        }
        return "Content: type=" + s;
    }
    
    /**
     *  parse content model in an element declaration, and build up the state
     *  transation table for element content checking
     */
    final void parseModel(Parser parser) throws ParseException
    {
        terminalnodes = new Vector();
        symboltable = new Hashtable();
        symbols = new Vector();
        // create parse tree for content expression
        content = parseRootNode(parser);
        // add end node
        end = new Terminal(this, null);
        content = new Sequence(content, end);

        // calculate followpos for terminal nodes
        int terminals = terminalnodes.size();
        BitSet[] followpos = new BitSet[terminals];
        for (int i = 0; i < terminals; i++)
        {
            followpos[i] = new BitSet(terminals);
        }
        content.calcfollowpos(followpos);

        // state table
        Vector Dstates = new Vector();
        // transition table
        Dtrans = new Vector();
        // lists unmarked states
        Vector unmarked = new Vector();
        // state lookup table
        Hashtable statetable = new Hashtable();

        BitSet empty = new BitSet(terminals);
        statetable.put(empty, new Integer(-1));
    
        // current state processed
        int state = 0;                

        // start with firstpos at the root                
        BitSet set = content.firstpos(terminals);
        statetable.put(set, new Integer(Dstates.size()));
        unmarked.addElement(set);
        Dstates.addElement(set);
        int[] a = new int[symbols.size() + 1];
        Dtrans.addElement(a);
        if (set.get(end.pos))
        {
            a[symbols.size()] = 1;   // accepting
        }

        // check all unmarked states
        while (unmarked.size() > 0)
        {
            int[] t = (int[])Dtrans.elementAt(state);
        
            set = (BitSet)unmarked.elementAt(0);
            unmarked.removeElementAt(0);

            // check all input symbols
            for (int sym = 0; sym < symbols.size(); sym++)
            {
                Name n = (Name)symbols.elementAt(sym);

                BitSet newset = new BitSet(terminals);
                // if symbol is in the set add followpos to new set
                for (int i = 0; i < terminals; i++)
                {
                    if (set.get(i) && ((Terminal)terminalnodes.elementAt(i)).name == n)
                    {
                        newset.or(followpos[i]);
                    }
                }

                Integer lookup = (Integer)statetable.get(newset);                        
                // this state will transition to
                int transitionTo;
                // if new set is not in states add it                        
                if (lookup == null)
                {
                    transitionTo = Dstates.size();            
                    statetable.put(newset, new Integer(transitionTo));
                    unmarked.addElement(newset);
                    Dstates.addElement(newset);
                    a = new int[symbols.size() + 1];
                    Dtrans.addElement(a);
                    if (newset.get(end.pos))
                    {
                        a[symbols.size()] = 1;   // accepting
                    }
                }
                else
                {
                    transitionTo = lookup.intValue();                            
                }
                // set the transition for the symbol
                t[sym] = transitionTo;
            }
            state++;
        }
    }

    /**
     *  parse a list of content nodes
     */
    final Node parseList(Parser parser) throws ParseException
    {
        //use Hashtable to check name uniqueness  
        Hashtable symbols = new Hashtable(); 
        symbols.put(parser.name, parser.name);

        Node n = parseNode(parser);
        int cpType = parser.token;

        switch (parser.token)
        {
            case Parser.COMMA:
                parser.nextToken();
                n = new Sequence(n, parseNode(parser));
                break;
            case Parser.OR:
                parser.nextToken();
                if (parser.token == parser.NAME) {
                    if (symbols.contains(parser.name)) 
                        parser.error("Warning: Repeated element in content model: " + parser.name );
                    else symbols.put(parser.name, parser.name);
                }
                n = new Choice(n, parseNode(parser));
                break;
            case Parser.RPAREN:
                return n;
            default:
                parser.error("Illegal token in content model: " + parser.tokenString(parser.token));
        }

        for (;;)
        {
            if (parser.token == Parser.RPAREN)
                return n;
            else if (cpType == Parser.COMMA && parser.token == Parser.COMMA)
            {
                parser.nextToken();
                n = new Sequence(n, parseNode(parser));
            }
            else if (cpType == Parser.OR && parser.token == Parser.OR)
            {
                parser.nextToken();
                if (parser.token == parser.NAME) {
                    if (symbols.contains(parser.name)) 
                        parser.error("Repeated element in content model: " + parser.name );
                    else symbols.put(parser.name, parser.name);
                }
                n = new Choice(n, parseNode(parser));
            }
            else parser.error("Illegal token in content model: " + parser.tokenString(parser.token));
        }
    }

    /**
     *  parse a node in an element content model declaration 
     */
    final Node parseNode(Parser parser) throws ParseException
    {
        Node n;

        switch (parser.token)
        {
            case Parser.LPAREN:
                parser.nextToken();
                n = parseList(parser);
                break;
            case Parser.NAME:
                n = new Terminal(this, parser.name);
                break;
            default:
                n = null;
                parser.error("Illegal token in content model: " + parser.tokenString(parser.token));
        }
        return finishNode(parser, n);
    }

    final Node finishNode(Parser parser, Node m) throws ParseException
    {
        Node n;

        switch (parser.lookahead) 
        {
            case Parser.ASTERISK:
                parser.nextToken();
                n = new Closure(m);
                break;
            case Parser.PLUS:
                parser.nextToken();
                n = new ClosurePlus(m);
                break;
            case Parser.QMARK:
                parser.nextToken();
                n = new Qmark(m);
                break;
            default:
                n = m;
        }
        parser.nextToken();
        return n;
    }

    /**
     *  parse the root node in an element content model declaration
     */
    final Node parseRootNode(Parser parser) throws ParseException
    {
        Node n;

        switch (parser.token)
        {
            case Parser.LPAREN:
                parser.nextToken();
                if (parser.token == Parser.HASH) 
                    return parseMixed(parser); 
                else n = parseList(parser);
                break;
            case Parser.NAME:
                n = new Terminal(this, parser.name);
                break;
            default:
                n = null;
                parser.error("Expected ANY, EMPTY or '(' instead of: " + parser.tokenString(parser.token));
        }
        return finishNode(parser, n);
    }

    /**
     *  parser mixed element content model 
     */
    final Node parseMixed(Parser parser) throws ParseException
    {
        Node n;
        Hashtable symbols= new Hashtable();
        parser.parseKeyword(Parser.PCDATA, "PCDATA");
        n = new Terminal(this, parser.name);
        symbols.put(parser.name, parser.name);
        parser.nextToken();
        switch (parser.token)
        {
            case Parser.RPAREN:
                // create a closure even if there is no asterisk.
                n = new Closure(n);
                if (parser.lookahead == Parser.ASTERISK)
                {
                    parser.nextToken();
                }
                break;
            case Parser.OR:             
                for (;;)
                {
                    if (parser.token == Parser.OR)
                    {
                        parser.nextToken();
                        if (parser.token == parser.NAME) {
                            if (symbols.contains(parser.name)) 
                                parser.error("Repeated element in content model: " + parser.name);
                            else symbols.put(parser.name, parser.name);
                        }
                        n = new Choice(n, parseNode(parser));
                    }
                    else if (parser.token == Parser.RPAREN) {
                        if (parser.lookahead == Parser.ASTERISK)
                        {
                            parser.nextToken();
                            n = new Closure(n);
                        }
                        else parser.error("Expected '*' instead of: " + parser.tokenString(parser.token));
                        break;
                    }
                    else {
                        parser.error("Illegal token in content model: " + parser.tokenString(parser.token));
                        break;
                    }
                }
                break;
            default:
                parser.error("Illegal token in content model: " + parser.tokenString(parser.token));
        }
        parser.nextToken();
        return n;
    }

    /**
     *  set inital state for context
     */
    final void initContent(Context context, Parser parser) throws ParseException
    {
        context.state = 0;
        if (Dtrans != null && Dtrans.size() > 0)
        {
            context.matched = ((int[])Dtrans.elementAt(context.state))[symbols.size()] > 0;
        }
        else
        {
            context.matched = true;
        }
    }

    /**
     * check whether the content model allows empty content
     */
    final boolean acceptEmpty(Context context)
    {
        if (type == ANY || type == EMPTY)
            return true;

        return (((int[])Dtrans.elementAt(context.state))[symbols.size()] > 0);
    }


    /**
     *  returns names of all legal elements following the specified state
     */
    final Vector expectedElements(int state)
    {
        int[] t = (int[])Dtrans.elementAt(state);
        Vector names = new Vector();

        for (Enumeration e = terminalnodes.elements(); e.hasMoreElements();)
        {
            Name n = ((Terminal)e.nextElement()).name;
            if (n != null && !names.contains(n))
            {
                Integer lookup = (Integer)symboltable.get(n);
                if (lookup != null && t[lookup.intValue()] != -1)
                {
                    names.addElement(n);
                }
            }
        }
       
        return names;
    }


    /**
     * check whether the specified element matches current context, and if matched, change
     * the context state
     */
    final boolean checkContent(Context context, Element e, Parser parser) throws ParseException
    {
        if (type == ANY)
        {
            context.matched = true;
            return true;
        }
        Name n = null;
        if (e.getType() == Element.ENTITYREF)
        {
            Entity en = parser.dtd.findEntity(e.getTagName());
            if (en.getLength() == -1)
                n = Parser.namePCDATA; // this entity is PCDATA
            else
                return true; // have to wait until entity itself is parsed.
        }
        else if (e.getType() == Element.PCDATA)
        {
            n = Parser.namePCDATA;
        } else {
            n = e.getTagName();
        }

        if (n != null)
        {
            Integer lookup = (Integer)symboltable.get(n);
            if (lookup != null)
            {
                int sym = lookup.intValue();
                int state = ((int[])Dtrans.elementAt(context.state))[sym];
                if (state == -1)
                {
                    parser.error("Pattern mismatch in content of '" + e.getParent().getTagName() + "'. Expected " + expectedElements(context.state));
                }
                else
                {
                    context.state = state;
                    context.matched = ((int[])Dtrans.elementAt(context.state))[symbols.size()] > 0;
                }
                return true;
            }
            else
            {
                Vector v = expectedElements(context.state);
                if (v.isEmpty())
                    parser.error("Invalid element '" + n + "' in content of '" + context.ed.name + "'.  Expected closing tag.");
                else
                    parser.error("Invalid element '" + n + "' in content of '" + context.ed.name + "'.  Expected " + v);
            }
        }
        else
        {
            parser.error("Invalid element in content of '" + context.ed.name + "'");
        }
        return false;
    }

    public Element toSchema()
    {
        Element elementContent = null;

        switch (type)
        {
            case EMPTY:
                elementContent = new ElementImpl( nameEMPTY, Element.ELEMENT ); 
                break;
            case ANY:
                elementContent = new ElementImpl( nameANY, Element.ELEMENT );
                break;
            case ELEMENTS:
                if (content != null) 
                {                    
                    Element dummyNode = new ElementImpl( Name.create("DUMMYNODE"), Element.ELEMENT );
                    ((Sequence)content).left.toSchema(Parser.HASH, 0, dummyNode); 
                    elementContent = dummyNode.getChild(0);
                }
                break;
        }

        return elementContent;
    }

    static Name nameEMPTY = Name.create("EMPTY","XML");
    static Name nameANY = Name.create("ANY","XML");

    public void save(Atom ns, XMLOutputStream o) throws IOException
    {
        switch (type)
        {
            case EMPTY:
                o.writeChars("EMPTY");
                break;
            case ANY:
                o.writeChars("ANY");
                break;
            case ELEMENTS:
                if (content != null) 
                    ((Sequence)content).left.save(o, Parser.HASH, 0, ns); // skip outer sequence node
                break;
        }
    }
}
    
/**
 * This is the node object on the syntax tree describing element 
 * content model
 */
class Node
{
    /**
     * firstpos
     */    
    BitSet first;
    
    /**
     * lastpos
     */    
    BitSet last;

    boolean nullable() 
    {
        return true;
    }
    
    Node clone(ContentModel cm)
    {
        return new Node();
    }

    BitSet firstpos(int positions)
    {
        if (empty == null)
        {
            empty = new BitSet(positions);
        }
        return empty;
    }
    
    BitSet lastpos(int positions)
    {
        return firstpos(positions);
    }
    
    void calcfollowpos(BitSet[] followpos)
    {
    }

    Element toSchema(int parentType, int level, Element currElement)
    {      
        return null;
    }

    void save(XMLOutputStream o, int parentType, int level, Atom ns) throws IOException
    {
    }
    
    static BitSet empty;

    static Name namePCDATA = Name.create("PCDATA");
}    

class Terminal extends Node
{
    /**
     * numbering the node
     */
    int pos;
    
    /**
     * name it refers to
     */    
    Name name;

    
    Terminal(ContentModel cm, Name name)
    {
        this.name = name;
        this.pos = cm.terminalnodes.size();
        
        cm.terminalnodes.addElement(this);
        if (name != null && cm.symboltable.get(name) == null)
        {
            cm.symboltable.put(name, new Integer(cm.symbols.size()));
            cm.symbols.addElement(name);
        }
    }
    
    Node clone(ContentModel cm)
    {
        return new Terminal(cm, name);
    }

    boolean nullable() 
    {
        if (name == null)
            return true;
        else return false;
    }
    
    BitSet firstpos(int positions)
    {
        if (first == null)
        {
            first = new BitSet(positions);
            first.set(pos);
        }

        return first;
    }
    
    BitSet lastpos(int positions)
    {
        if (last == null)
        {
            last = new BitSet(positions);
            last.set(pos);
        }

        return last;
    }
    
    void calcfollowpos(BitSet[] followpos)
    {
    }

    Element toSchema(int parentType, int level, Element currElement)
    {
        if (name.getName() == namePCDATA.getName()) 
        {
            if (level == 0 || level == 1 && parentType == Parser.ASTERISK)
            {
                currElement.addChild(new ElementImpl(Name.create("PCDATA","XML"), Element.ELEMENT), null);
                return currElement;
            }
            else 
            {
                Element mixedElement = new ElementImpl( Name.create("MIXED","XML"), Element.ELEMENT );
                currElement.addChild(mixedElement, null);
                return mixedElement;
            }
        }
        else 
        {   
            Element eltElement = new ElementImpl( Name.create("ELT","XML"), Element.ELEMENT );

            eltElement.setAttribute(Name.create("HREF","XML"), "#" + name);

            if (parentType == Parser.QMARK)
                eltElement.setAttribute(Name.create("OCCURS","XML"), "OPTIONAL");
            else if (parentType == Parser.ASTERISK)
                eltElement.setAttribute(Name.create("OCCURS","XML"), "STAR");
            else if (parentType == Parser.PLUS)
                eltElement.setAttribute(Name.create("OCCURS","XML"), "PLUS");

            currElement.addChild(eltElement, null);
            return currElement;
        }
    }

    void save(XMLOutputStream o, int parentType, int level, Atom ns) throws IOException
    {
        if (name.getName() == namePCDATA.getName()) 
        {
            if (level == 0 || level == 1 && parentType == Parser.ASTERISK)
                o.writeChars("(#PCDATA)");
            else o.writeChars("#PCDATA");
        }
        else 
        {
            if (parentType == Parser.HASH || level == 1 && (parentType == Parser.QMARK || parentType == Parser.ASTERISK)) {
                o.writeChars("(");
                o.writeQualifiedName(name, ns);
                o.writeChars(")");
            }
            else o.writeQualifiedName(name, ns);
        }
    }

}

class Sequence extends Node
{
    /**
     * left node
     */    
    Node left;
    
    /**
     * right node
     */    
    Node right;
    
    Sequence(Node left, Node right)
    {
        this.left = left;
        this.right = right;
    }
    
    Node clone(ContentModel cm)
    {
        return new Sequence(left.clone(cm), right.clone(cm));
    }

    boolean nullable() 
    {
        return left.nullable() && right.nullable();
    }
    
    BitSet firstpos(int positions)
    {
        if (first == null)
        {
            if (left.nullable())
            {
                first = (BitSet)left.firstpos(positions).clone();
                first.or(right.firstpos(positions));
            }
            else
            {
                first = left.firstpos(positions);
            }
        }

        return first;
    }
    
    BitSet lastpos(int positions)
    {
        if (last == null)
        {
            if (right.nullable())
            {
                last = (BitSet)left.lastpos(positions).clone();
                last.or(right.lastpos(positions));
            }
            else
            {
                last = right.lastpos(positions);
            }
        }

        return last;
    }
    
    void calcfollowpos(BitSet[] followpos)
    {
        left.calcfollowpos(followpos);
        right.calcfollowpos(followpos);
        
        int l = followpos.length;        
        BitSet lp = left.lastpos(l);
        BitSet fp = right.firstpos(l);        
        for (int i = followpos.length - 1; i >= 0; i--)
        {
            if (lp.get(i))
            {
                followpos[i].or(fp);
            }
        }
    }

    Element toSchema( int parentType, int level, Element currElement)
    {
        Element tempCurr;
        Element seqElement;       

        level++;
        if (parentType == Parser.COMMA) {
            currElement = left.toSchema(Parser.COMMA, level, currElement);
            right.toSchema(Parser.COMMA, level, currElement);            
        }
        else {
            seqElement = new ElementImpl( Name.create("GROUP","XML"), Element.ELEMENT );        
            seqElement.setAttribute(Name.create("GROUPTYPE","XML"), "SEQ");
            
            if (parentType == Parser.QMARK)
                seqElement.setAttribute(Name.create("OCCURS","XML"), "OPTIONAL");
            else if (parentType == Parser.ASTERISK)
                seqElement.setAttribute(Name.create("OCCURS","XML"), "STAR");
            else if (parentType == Parser.PLUS)
                seqElement.setAttribute(Name.create("OCCURS","XML"), "PLUS");
            
            seqElement = left.toSchema(Parser.COMMA, level, seqElement);
            right.toSchema(Parser.COMMA, level, seqElement);

            currElement.addChild(seqElement, null);
        }     
        level--;

        return currElement;
    }

    void save(XMLOutputStream o, int parentType, int level, Atom ns) throws IOException
    {
        level++;
        if (parentType == Parser.COMMA) {
            left.save(o, Parser.COMMA, level, ns);
            o.write(',');
            right.save(o, Parser.COMMA, level, ns);
        }
        else {
            o.write('(');
            left.save(o, Parser.COMMA, level, ns);
            o.write(',');
            right.save(o, Parser.COMMA, level, ns);
            o.write(')');
        }
        level--;
    }
}    

class Choice extends Node
{
    /**
     * left node
     */    
    Node left;
    
    /**
     * right node
     */    
    Node right;
    
    Choice(Node left, Node right)
    {
        this.left = left;
        this.right = right;
    }
    
    Node clone(ContentModel cm)
    {
        return new Choice(left.clone(cm), right.clone(cm));
    }

    boolean nullable() 
    {
        return left.nullable() || right.nullable();
    }
    
    BitSet firstpos(int positions)
    {
        if (first == null)
        {
            first = (BitSet)left.firstpos(positions).clone();
            first.or(right.firstpos(positions));
        }
        return first;
    }
    
    BitSet lastpos(int positions)
    {
        if (last == null)
        {
            last = (BitSet)left.lastpos(positions).clone();
            last.or(right.lastpos(positions));
        }

        return last;
    }
    
    void calcfollowpos(BitSet[] followpos)
    {
        left.calcfollowpos(followpos);
        right.calcfollowpos(followpos);
    }

    Element toSchema(int parentType, int level, Element currElement)
    {   
        Element tempCurr;
        Element orElement;

        if (parentType == Parser.OR) {
            currElement = left.toSchema(Parser.OR, level,currElement);
            right.toSchema(Parser.OR, level,currElement);
        }
        else {

            orElement = new ElementImpl( Name.create("GROUP","XML"), Element.ELEMENT );
            orElement.setAttribute(Name.create("GROUPTYPE","XML"), "OR");

            if (parentType == Parser.QMARK)
                orElement.setAttribute(Name.create("OCCURS","XML"), "OPTIONAL");
            else if (parentType == Parser.ASTERISK)
                orElement.setAttribute(Name.create("OCCURS","XML"), "STAR");
            else if (parentType == Parser.PLUS)
                orElement.setAttribute(Name.create("OCCURS","XML"), "PLUS");

            orElement = left.toSchema(Parser.OR, level, orElement);
            right.toSchema(Parser.OR, level, orElement);

            currElement.addChild(orElement, null);
        }     
        level--;

        return currElement;
    }

    void save(XMLOutputStream o, int parentType, int level, Atom ns) throws IOException
    {
        level++;
        if (parentType == Parser.OR) {
            left.save(o, Parser.OR, level, ns);
            o.write('|');
            right.save(o, Parser.OR, level, ns);
        }
        else {
            o.write('(');
            left.save(o, Parser.OR, level, ns);
            o.write('|');
            right.save(o, Parser.OR, level, ns);
            o.write(')');
        }        
        level--;
    }
}    

class Qmark extends Node
{
    /**
     * node
     */
    Node node;
    
    Qmark(Node node)
    {
        this.node = node;
    }
    
    Node clone(ContentModel cm)
    {
        return new Qmark(node.clone(cm));
    }

    boolean nullable() 
    {
        return true;
    }
    
    BitSet firstpos(int positions)
    {
        if (first == null)
        {
            first = node.firstpos(positions);
        }
        return first;
    }
    
    BitSet lastpos(int positions)
    {
        if (last == null)
        {
            last = node.lastpos(positions);
        }

        return last;
    }
    
    void calcfollowpos(BitSet[] followpos)
    {
        node.calcfollowpos(followpos);
    }

    Element toSchema(int parentType, int level, Element currElement)
    {
        level++;       
        node.toSchema(Parser.QMARK, level, currElement);
        level--;

        return currElement;
    }

    void save(XMLOutputStream o, int parentType, int level, Atom ns) throws IOException
    {
        level++;
        if (parentType == Parser.QMARK || parentType == Parser.ASTERISK)
            o.writeChars("(");          
        node.save(o, Parser.QMARK, level, ns);
        o.write('?');
        if (parentType == Parser.QMARK || parentType == Parser.ASTERISK)
            o.writeChars(")");
        level--;
    }
}


class Closure extends Node
{
    /**
     * node
     */    
    Node node;
    
    Closure(Node node)
    {
        this.node = node;
    }
    
    Closure()
    {
    }

    Node clone(ContentModel cm)
    {
        return new Closure(node.clone(cm));
    }

    boolean nullable() 
    {
        return true;
    }
    
    BitSet firstpos(int positions)
    {
        if (first == null)
        {
            first = node.firstpos(positions);
        }
        return first;
    }
    
    BitSet lastpos(int positions)
    {
        if (last == null)
        {
           last = node.lastpos(positions);
        }
        return last;
    }
    
    void calcfollowpos(BitSet[] followpos)
    {
        node.calcfollowpos(followpos);
        
        int l = followpos.length;        
        lastpos(l);
        firstpos(l);        

        for (int i = followpos.length - 1; i >= 0; i--)
        {
            if (last.get(i))
            {
                followpos[i].or(first);
            }
        }
    }

    Element toSchema(int parentType, int level, Element currElement)
    {

        level++;       
        node.toSchema(Parser.ASTERISK, level, currElement);
        level--;

        return currElement;
    }

    void save(XMLOutputStream o, int parentType, int level, Atom ns) throws IOException
    {
        if (parentType == Parser.QMARK || parentType == Parser.ASTERISK)
            o.writeChars("(");
        level++;
        node.save(o, Parser.ASTERISK, level, ns);
        level--; 

        o.write('*');
        if (parentType == Parser.QMARK || parentType == Parser.ASTERISK)
            o.writeChars(")");
    }
}    


class ClosurePlus extends Closure
{
    ClosurePlus(Node node)
    {
        this.node = node;
    }
    
    Node clone(ContentModel cm)
    {
        return new ClosurePlus(node.clone(cm));
    }

    boolean nullable() 
    {
        return node.nullable();
    }    

    Element toSchema(int parentType, int level, Element currElement)
    {
        level++;       
        node.toSchema(Parser.PLUS, level, currElement);
        level--;

        return currElement;
    }

    void save(XMLOutputStream o, int parentType, int level, Atom ns) throws IOException
    {
        if (parentType == Parser.QMARK || parentType == Parser.ASTERISK)
            o.writeChars("(");
        level++;
        node.save(o, Parser.ASTERISK, level, ns);
        level--; 
        o.write('+');
        if (parentType == Parser.QMARK || parentType == Parser.ASTERISK)
            o.writeChars(")");
    }
}