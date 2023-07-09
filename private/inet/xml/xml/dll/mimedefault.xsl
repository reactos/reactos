<?xml version="1.0"?>
<x:stylesheet xmlns:x="http://www.w3.org/TR/WD-xsl" xmlns:dt="urn:schemas-microsoft-com:datatypes" xmlns:d2="uuid:C2F41010-65B3-11d1-A29F-00AA00C14882">
<x:template match="/">
<HTML><HEAD>
<STYLE>BODY{font:x-small 'Verdana';margin-right:1.5em}
.c{cursor:hand}
.b{color:red;font-family:'Courier New';font-weight:bold;text-decoration:none}
.e{margin-left:1em;text-indent:-1em;margin-right:1em}
.k{margin-left:1em;text-indent:-1em;margin-right:1em}
.t{color:#990000}
.xt{color:#990099}
.ns{color:red}
.dt{color:green}
.m{color:blue}
.tx{font-weight:bold}
.db{text-indent:0px;margin-left:1em;margin-top:0px;margin-bottom:0px;padding-left:.3em;border-left:1px solid #CCCCCC;font:small Courier}
.di{font:small Courier}
.d{color:blue}
.pi{color:blue}
.cb{text-indent:0px;margin-left:1em;margin-top:0px;margin-bottom:0px;padding-left:.3em;font:small Courier;color:#888888}
.ci{font:small Courier;color:#888888}
PRE{margin:0px;display:inline}</STYLE>
<SCRIPT><x:comment>
function f(e){
if (e.className=="ci"){if (e.children(0).innerText.indexOf("\n")&gt;0) fix(e,"cb");}
if (e.className=="di"){if (e.children(0).innerText.indexOf("\n")&gt;0) fix(e,"db");}
e.id="";
}
function fix(e,cl){
e.className=cl;
e.style.display="block";
j=e.parentElement.children(0);
j.className="c";
k=j.children(0);
k.style.visibility="visible";
k.href="#";
}
function ch(e){
mark=e.children(0).children(0);
if (mark.innerText=="+"){
mark.innerText="-";
for (var i=1;i&lt;e.children.length;i++)
e.children(i).style.display="block";
}
else if (mark.innerText=="-"){
mark.innerText="+";
for (var i=1;i&lt;e.children.length;i++)
e.children(i).style.display="none";
}}
function ch2(e){
mark=e.children(0).children(0);
contents=e.children(1);
if (mark.innerText=="+"){
mark.innerText="-";
if (contents.className=="db"||contents.className=="cb")
contents.style.display="block";
else contents.style.display="inline";
}
else if (mark.innerText=="-"){
mark.innerText="+";
contents.style.display="none";
}}
function cl(){
e=window.event.srcElement;
if (e.className!="c"){e=e.parentElement;if (e.className!="c"){return;}}
e=e.parentElement;
if (e.className=="e") ch(e);
if (e.className=="k") ch2(e);
}
function ex(){}
function h(){window.status=" ";}
document.onclick=cl;
</x:comment></SCRIPT>
</HEAD>
<BODY class="st"><x:apply-templates/></BODY>
</HTML>
</x:template>
<x:template match="node()[nodeType()=10]">
<DIV class="e"><SPAN>
<SPAN class="b"><x:entity-ref name="nbsp"/></SPAN>
<SPAN class="d">&lt;!DOCTYPE <x:node-name/><I> (View Source for full doctype...)</I>&gt;</SPAN>
</SPAN></DIV>
</x:template>
<x:template match="pi()">
<DIV class="e">
<SPAN class="b"><x:entity-ref name="nbsp"/></SPAN>
<SPAN class="m">&lt;?</SPAN><SPAN class="pi"><x:node-name/> <x:value-of/></SPAN><SPAN class="m">?&gt;</SPAN>
</DIV>
</x:template>
<x:template match="pi('xml')">
<DIV class="e">
<SPAN class="b"><x:entity-ref name="nbsp"/></SPAN>
<SPAN class="m">&lt;?</SPAN><SPAN class="pi">xml <x:for-each select="@*"><x:node-name/>="<x:value-of/>" </x:for-each></SPAN><SPAN class="m">?&gt;</SPAN>
</DIV>
</x:template>
<x:template match="@*" xml:space="preserve"><SPAN><x:attribute name="class"><x:if match="x:*/@*">x</x:if>t</x:attribute> <x:node-name/></SPAN><SPAN class="m">="</SPAN><B><x:value-of/></B><SPAN class="m">"</SPAN></x:template>
<x:template match="@xmlns:*|@xmlns|@xml:*"><SPAN class="ns"> <x:node-name/></SPAN><SPAN class="m">="</SPAN><B class="ns"><x:value-of/></B><SPAN class="m">"</SPAN></x:template>
<x:template match="@dt:*|@d2:*"><SPAN class="dt"> <x:node-name/></SPAN><SPAN class="m">="</SPAN><B class="dt"><x:value-of/></B><SPAN class="m">"</SPAN></x:template>
<x:template match="textnode()">
<DIV class="e">
<SPAN class="b"><x:entity-ref name="nbsp"/></SPAN>
<SPAN class="tx"><x:value-of/></SPAN>
</DIV>
</x:template>
<x:template match="comment()">
<DIV class="k">
<SPAN><A class="b" onclick="return false" onfocus="h()" STYLE="visibility:hidden">-</A> <SPAN class="m">&lt;!--</SPAN></SPAN>
<SPAN id="clean" class="ci"><PRE><x:value-of/></PRE></SPAN>
<SPAN class="b"><x:entity-ref name="nbsp"/></SPAN> <SPAN class="m">--&gt;</SPAN>
<SCRIPT>f(clean);</SCRIPT></DIV>
</x:template>
<x:template match="cdata()">
<DIV class="k">
<SPAN><A class="b" onclick="return false" onfocus="h()" STYLE="visibility:hidden">-</A> <SPAN class="m">&lt;![CDATA[</SPAN></SPAN>
<SPAN id="clean" class="di"><PRE><x:value-of/></PRE></SPAN>
<SPAN class="b"><x:entity-ref name="nbsp"/></SPAN> <SPAN class="m">]]&gt;</SPAN>
<SCRIPT>f(clean);</SCRIPT></DIV>
</x:template>
<x:template match="*">
<DIV class="e"><DIV STYLE="margin-left:1em;text-indent:-2em">
<SPAN class="b"><x:entity-ref name="nbsp"/></SPAN>
<SPAN class="m">&lt;</SPAN><SPAN><x:attribute name="class"><x:if match="x:*">x</x:if>t</x:attribute><x:node-name/></SPAN> <x:apply-templates select="@*"/><SPAN class="m"> /&gt;</SPAN>
</DIV></DIV>
</x:template>
<x:template match="*[node()]">
<DIV class="e">
<DIV class="c"><A href="#" onclick="return false" onfocus="h()" class="b">-</A> <SPAN class="m">&lt;</SPAN><SPAN><x:attribute name="class"><x:if match="x:*">x</x:if>t</x:attribute><x:node-name/></SPAN><x:apply-templates select="@*"/> <SPAN class="m">&gt;</SPAN></DIV>
<DIV><x:apply-templates/>
<DIV><SPAN class="b"><x:entity-ref name="nbsp"/></SPAN> <SPAN class="m">&lt;/</SPAN><SPAN><x:attribute name="class"><x:if match="x:*">x</x:if>t</x:attribute><x:node-name/></SPAN><SPAN class="m">&gt;</SPAN></DIV>
</DIV></DIV>
</x:template>
<x:template match="*[textnode()$and$$not$(comment()$or$pi()$or$cdata())]">
<DIV class="e"><DIV STYLE="margin-left:1em;text-indent:-2em">
<SPAN class="b"><x:entity-ref name="nbsp"/></SPAN> <SPAN class="m">&lt;</SPAN><SPAN><x:attribute name="class"><x:if match="x:*">x</x:if>t</x:attribute><x:node-name/></SPAN><x:apply-templates select="@*"/>
<SPAN class="m">&gt;</SPAN><SPAN class="tx"><x:value-of/></SPAN><SPAN class="m">&lt;/</SPAN><SPAN><x:attribute name="class"><x:if match="x:*">x</x:if>t</x:attribute><x:node-name/></SPAN><SPAN class="m">&gt;</SPAN>
</DIV></DIV>
</x:template>
<x:template match="*[*]">
<DIV class="e">
<DIV class="c" STYLE="margin-left:1em;text-indent:-2em"><A href="#" onclick="return false" onfocus="h()" class="b">-</A> <SPAN class="m">&lt;</SPAN><SPAN><x:attribute name="class"><x:if match="x:*">x</x:if>t</x:attribute><x:node-name/></SPAN><x:apply-templates select="@*"/> <SPAN class="m">&gt;</SPAN></DIV>
<DIV><x:apply-templates/>
<DIV><SPAN class="b"><x:entity-ref name="nbsp"/></SPAN> <SPAN class="m">&lt;/</SPAN><SPAN><x:attribute name="class"><x:if match="x:*">x</x:if>t</x:attribute><x:node-name/></SPAN><SPAN class="m">&gt;</SPAN></DIV>
</DIV></DIV>
</x:template>
</x:stylesheet>
