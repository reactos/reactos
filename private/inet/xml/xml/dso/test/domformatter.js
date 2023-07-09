function ShowXML(doc)
{
    var w = window.open("","","resizable,scrollbars,width=640,height=480");
    w.title = "XML Data";
    w.document.open();
    w.document.write("<STYLE>");
    w.document.write("  .xml	    {font-size:9pt;font-family:Arial}");
    w.document.write("  .tag	    {color:red; font-weight:bold; }");
    w.document.write("  .attribute	{color:blue; font-weight:bold; }");
    w.document.write("  .attrvalue	{color:darkorchid; font-weight:bold; }");
    w.document.write("  .comment	{color:green; font-weight:bold; }");
    w.document.write("</STYLE>");
    var xml = DumpTree(doc,0,null);
    w.document.write(xml);
    w.document.close();
}

function qname(node)
{
    var ns = node.nameSpace;
    if (ns == null)
        return node.nodeName;

    var prefix = ns.attributes.getNamedItem("prefix").nodeValue;
    return prefix + ":" + node.nodeName;
}

function aqname(node, attr)
{
    var ns2 = attr.nameSpace;
    if (ns2 == null)
        return attr.nodeName;

    var ns = node.nameSpace;
    if (ns && (ns.attributes.getNamedItem("ns").nodeValue == ns2.attributes.getNamedItem("ns").nodeValue))
        return attr.nodeName;

    var prefix;
    prefix = ns2.attributes.getNamedItem("prefix").nodeValue;

    return prefix + ":" + attr.nodeName;
}

function DumpTree(node,i,ns)
{
    if (node == null) return "";

    var type = node.nodeType;
	result = "<DL class=xml><DD>";

	if (type == 6 || type == 7 || type == 10 ) // text, cdata, entityref
	{
		result += node.nodeValue;
		result += "</DD></DL>";
		return result;
	}
	if (type == 5) {    // comment
		result += "<span class=comment>&lt;!--" 
			+ node.nodeValue + "--&gt;</span>";
		result += "</DD></DL>"
		return result;
	}
	if (type == 1)  // document
	{
		var en = node.childNodes;
        var len = en.length;
		for (var i = 0; i < len; i++)
		{
            var child = en.item(i);
			if (child.nodeType != 9)
				result += DumpTree(child,i);
		}
		result += "</DD></DL>"
		return result;
	}
	if (type == 4) // pi 
	{
		result += "<span class=pi>&lt;?" + node.nodeName + " " + 
			node.nodeValue + "?&gt;</span>";
		result += "</DD></DL>"
		return result;
	}
    if (type == 11) // doctype 
    {
        result += "<span class=decl>&lt;!DOCTYPE " + qname(node);
        var pubid = node.attributes.getNamedItem("PUBLIC");
        if (pubid != null) 
            pubid = pubid.nodeValue;
        var sys = node.attributes.getNamedItem("SYSTEM");
        if (sys != null) 
            sys = sys.nodeValue;

        if (pubid != "" && pubid != null)
            result += " PUBLIC \"" + pubid + "\"";
        else if (sys != "" && sys != null)
            result += " SYSTEM";
        if (sys != "" && sys != null) 
            result += " \"" + sys + "\"";

        var value = ""; // node.nodeValue;
        if (value != "")
        {
            result += " [" + value + "]";
        }
        result += "&gt;</span></DD></DL>";
        return result;
    }

	var num = node.childNodes.length;
    var children = node.childNodes;
    var num = children.length;
    var empty = "";
    if (num == 0) 
    {
        empty = "/";
        if (type == 11 || type == 7)
        {
            empty = "?";
        }
    }

    result += "<span class=tag>&lt;";
    if (type == 11 || type == 7) result += "?"
    result = result + qname(node) + "</span>";

    if (type == 2) // element
    {
        var attrs = node.attributes;
        var na = attrs.length;
        for (i = 0; i < na; i++) {
            var a = attrs.item(i);
            result += " <span class=attribute>" + aqname(node,a) + "</span>=<span class=attrvalue>\"" + a.nodeValue + "\"</span>";
        }
    }

    result += "<span class=tag>" + empty + "&gt;</span>";

    if (num > 0) 
    {
		for (var i = 0; i < num; i++)
		{
            var child = children.item(i);
            result += "\n";
            result += DumpTree(child,i+1);
        }
        result +=  "<span class=tag>&lt;/" + qname(node) + "&gt;</span>\n";
    }
    result += "</DD></DL>"
    return result;
}

