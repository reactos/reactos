<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output method="xml" encoding="ISO-8859-1" indent="yes"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>

  <xsl:variable name="href_base" select="''"/>
  <xsl:variable name="menu_name">Main Menu</xsl:variable>
<!--
 - returns the filename associated to an ID in the original file
 -->
  <xsl:template name="tocfilename">
    <xsl:param name="name" select="string(@href)"/>
    <xsl:choose>
      <xsl:when test="$name = '#Introducti'">
        <xsl:text>intro.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Documentat'">
        <xsl:text>docs.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Reporting'">
        <xsl:text>bugs.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#help'">
        <xsl:text>help.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Help'">
        <xsl:text>help.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Downloads'">
        <xsl:text>downloads.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#News'">
        <xsl:text>news.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Contributi'">
        <xsl:text>contribs.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#xsltproc'">
        <xsl:text>xsltproc2.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#API'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#XSLT'">
        <xsl:text>XSLT.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#XML'">
        <xsl:text>XMLinfo.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Validation'">
        <xsl:text>xmldtd.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#tree'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#library'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#interface'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Example'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Entities'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#architecture'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Namespaces'">
        <xsl:text>namespaces.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#DOM'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Catalog'">
        <xsl:text>catalog.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Upgrading'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Encodings'">
        <xsl:text>encoding.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#IO'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Memory'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Thread'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#FAQ'">
        <xsl:text>FAQ.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Python'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = ''">
        <xsl:text>unknown.html</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$name"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="docfilename">
    <xsl:param name="name" select="string(@href)"/>
    <xsl:choose>
      <xsl:when test="$name = '#Introducti'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Documentat'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Reporting'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#help'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Help'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Downloads'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#News'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Contributi'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#xsltproc'">
        <xsl:text>xsltproc2.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#API'">
        <xsl:text>API.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#XSLT'">
        <xsl:text>XSLT.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#XML'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Validation'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#tree'">
        <xsl:text>tree.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#library'">
        <xsl:text>library.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#interface'">
        <xsl:text>interface.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Example'">
        <xsl:text>example.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Entities'">
        <xsl:text>entities.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#architecture'">
        <xsl:text>architecture.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Namespaces'">
        <xsl:text>namespaces.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#DOM'">
        <xsl:text>DOM.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Catalog'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Upgrading'">
        <xsl:text>upgrade.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Encodings'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#IO'">
        <xsl:text>xmlio.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Memory'">
        <xsl:text>xmlmem.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Thread'">
        <xsl:text>threads.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#FAQ'">
        <xsl:text></xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Python'">
        <xsl:text>python.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = ''">
        <xsl:text>unknown.html</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$name"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
  <xsl:template name="filename">
    <xsl:param name="name" select="string(@href)"/>
    <xsl:choose>
      <xsl:when test="$name = '#Introducti'">
        <xsl:text>intro.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Documentat'">
        <xsl:text>docs.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Reporting'">
        <xsl:text>bugs.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#help'">
        <xsl:text>help.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Help'">
        <xsl:text>help.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Downloads'">
        <xsl:text>downloads.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#News'">
        <xsl:text>news.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Contributi'">
        <xsl:text>contribs.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#xsltproc'">
        <xsl:text>xsltproc2.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#API'">
        <xsl:text>API.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#XSLT'">
        <xsl:text>XSLT.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#XML'">
        <xsl:text>XMLinfo.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Validation'">
        <xsl:text>xmldtd.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#tree'">
        <xsl:text>tree.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#library'">
        <xsl:text>library.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#interface'">
        <xsl:text>interface.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Example'">
        <xsl:text>example.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Entities'">
        <xsl:text>entities.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#architecture'">
        <xsl:text>architecture.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Namespaces'">
        <xsl:text>namespaces.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#DOM'">
        <xsl:text>DOM.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Catalog'">
        <xsl:text>catalog.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Upgrading'">
        <xsl:text>upgrade.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Encodings'">
        <xsl:text>encoding.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#IO'">
        <xsl:text>xmlio.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Memory'">
        <xsl:text>xmlmem.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Thread'">
        <xsl:text>threads.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#FAQ'">
        <xsl:text>FAQ.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = '#Python'">
        <xsl:text>python.html</xsl:text>
      </xsl:when>
      <xsl:when test="$name = ''">
        <xsl:text>unknown.html</xsl:text>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$name"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>
<!--
 - The global title
 -->
  <xsl:variable name="globaltitle" select="string(/html/body/h1[1])"/>
<!--
 - The table of content
 -->
  <xsl:variable name="toc">
    <form action="search.php"
          enctype="application/x-www-form-urlencoded" method="get">
      <input name="query" type="text" size="20" value=""/>
      <input name="submit" type="submit" value="Search ..."/>
    </form>
    <ul><!-- style="margin-left: -1em" -->
      <li><a href="index.html">Home</a></li>
      <li><a href="http://xmlsoft.org/wiki">Wiki</a></li>
      <li><a href="html/index.html">Reference Manual</a></li>
      <xsl:for-each select="/html/body/h2">
        <xsl:variable name="filename">
          <xsl:call-template name="tocfilename">
            <xsl:with-param name="name" select="concat('#', string(a[1]/@name))"/>
          </xsl:call-template>
        </xsl:variable>
	<xsl:if test="$filename != ''">
	  <li>
	    <xsl:element name="a">
	      <xsl:attribute name="href">
	        <xsl:value-of select="$filename"/>
	      </xsl:attribute>
	      <xsl:if test="$filename = 'docs.html'">
		  <xsl:attribute name="style">font-weight:bold</xsl:attribute>
	      </xsl:if>
	      <xsl:value-of select="."/>
	    </xsl:element>
	  </li>
	</xsl:if>
      </xsl:for-each>
      <li><a href="examples/index.html" style="font-weight:bold">Code Examples</a></li>
      <li><a href="html/index.html" style="font-weight:bold">API Menu</a></li>
      <li><a href="guidelines.html">XML Guidelines</a></li>
      <li><a href="ChangeLog.html">Recent Changes</a></li>
    </ul>
  </xsl:variable>
  <xsl:variable name="develtoc">
    <form action="search.php"
          enctype="application/x-www-form-urlencoded" method="get">
      <input name="query" type="text" size="20" value=""/>
      <input name="submit" type="submit" value="Search ..."/>
    </form>
    <ul><!-- style="margin-left: -1em" -->
      <li><a href="index.html" style="font-weight:bold">Main Menu</a></li>
      <li><a href="html/index.html" style="font-weight:bold">Reference Manual</a></li>
      <li><a href="examples/index.html" style="font-weight:bold">Code Examples</a></li>
      <li><a href="guidelines.html">XML Guidelines</a></li>
      <li><a href="tutorial/index.html">Tutorial</a></li>
      <li><a href="xmlreader.html">The Reader Interface</a></li>
      <li><a href="ChangeLog.html">ChangeLog</a></li>
      <xsl:for-each select="/html/body/h2">
        <xsl:variable name="filename">
          <xsl:call-template name="docfilename">
            <xsl:with-param name="name" select="concat('#', string(a[1]/@name))"/>
          </xsl:call-template>
        </xsl:variable>
	<xsl:if test="$filename != ''">
	  <li>
	    <xsl:element name="a">
	      <xsl:attribute name="href">
	        <xsl:value-of select="$filename"/>
	      </xsl:attribute>
	      <xsl:value-of select="."/>
	    </xsl:element>
	  </li>
	</xsl:if>
      </xsl:for-each>
      <li><a href="xml.html">flat page</a>, <a href="site.xsl">stylesheet</a></li>
    </ul>
  </xsl:variable>
  <xsl:variable name="api">
    <ul><!-- style="margin-left: -1em" -->
      <li><a href="{$href_base}APIchunk0.html">Alphabetic</a></li>
      <li><a href="{$href_base}APIconstructors.html">Constructors</a></li>
      <li><a href="{$href_base}APIfunctions.html">Functions/Types</a></li>
      <li><a href="{$href_base}APIfiles.html">Modules</a></li>
      <li><a href="{$href_base}APIsymbols.html">Symbols</a></li>
    </ul>
  </xsl:variable>
  <xsl:variable name="related">
    <ul><!-- style="margin-left: -1em" -->
      <li><a href="http://mail.gnome.org/archives/xml/">Mail archive</a></li>
      <li><a href="http://xmlsoft.org/XSLT/">XSLT libxslt</a></li>
      <li><a href="http://phd.cs.unibo.it/gdome2/">DOM gdome2</a></li>
      <li><a href="http://www.aleksey.com/xmlsec/">XML-DSig xmlsec</a></li>
      <li><a href="ftp://xmlsoft.org/">FTP</a></li>
      <li><a href="http://www.zlatkovic.com/projects/libxml/">Windows binaries</a></li>
      <li><a href="http://www.blastwave.org/packages.php/libxml2">Solaris binaries</a></li>
      <li><a href="http://www.explain.com.au/oss/libxml2xslt.html">MacOsX binaries</a></li>
      <li><a href="http://libxmlplusplus.sourceforge.net/">C++ bindings</a></li>
      <li><a href="http://www.zend.com/php5/articles/php5-xmlphp.php#Heading4">PHP bindings</a></li>
      <li><a href="http://sourceforge.net/projects/libxml2-pas/">Pascal bindings</a></li>
      <li><a href="http://rubyforge.org/projects/xml-tools/">Ruby bindings</a></li>
      <li><a href="http://tclxml.sourceforge.net/">Tcl bindings</a></li>
      <li><a href="http://bugzilla.gnome.org/buglist.cgi?product=libxml2">Bug Tracker</a></li>
    </ul>
  </xsl:variable>
  <xsl:template name="toc">
    <table border="0" cellspacing="0" cellpadding="1" width="100%" bgcolor="#000000">
      <tr>
        <td>
          <table width="100%" border="0" cellspacing="1" cellpadding="3">
            <tr>
              <td colspan="1" bgcolor="#eecfa1" align="center">
                <center>
                  <b><xsl:value-of select="$menu_name"/></b>
                </center>
              </td>
            </tr>
            <tr>
              <td bgcolor="#fffacd">
                <xsl:copy-of select="$toc"/>
              </td>
            </tr>
          </table>
          <table width="100%" border="0" cellspacing="1" cellpadding="3">
            <tr>
              <td colspan="1" bgcolor="#eecfa1" align="center">
                <center>
                  <b>Related links</b>
                </center>
              </td>
            </tr>
            <tr>
              <td bgcolor="#fffacd">
                <xsl:copy-of select="$related"/>
              </td>
            </tr>
          </table>
        </td>
      </tr>
    </table>
  </xsl:template>
  <xsl:template name="develtoc">
    <table border="0" cellspacing="0" cellpadding="1" width="100%" bgcolor="#000000">
      <tr>
        <td>
          <table width="100%" border="0" cellspacing="1" cellpadding="3">
            <tr>
              <td colspan="1" bgcolor="#eecfa1" align="center">
                <center>
                  <b>Developer Menu</b>
                </center>
              </td>
            </tr>
            <tr>
              <td bgcolor="#fffacd">
                <xsl:copy-of select="$develtoc"/>
              </td>
            </tr>
          </table>
          <table width="100%" border="0" cellspacing="1" cellpadding="3">
            <tr>
              <td colspan="1" bgcolor="#eecfa1" align="center">
                <center>
                  <b>API Indexes</b>
                </center>
              </td>
            </tr>
            <tr>
              <td bgcolor="#fffacd">
                <xsl:copy-of select="$api"/>
              </td>
            </tr>
          </table>
          <table width="100%" border="0" cellspacing="1" cellpadding="3">
            <tr>
              <td colspan="1" bgcolor="#eecfa1" align="center">
                <center>
                  <b>Related links</b>
                </center>
              </td>
            </tr>
            <tr>
              <td bgcolor="#fffacd">
                <xsl:copy-of select="$related"/>
              </td>
            </tr>
          </table>
        </td>
      </tr>
    </table>
  </xsl:template>
  <xsl:template mode="head" match="title">
    <title>
      <xsl:apply-templates/>
    </title>
  </xsl:template>
  <xsl:template mode="head" match="meta">
</xsl:template>
<!--
 - Write the styles in the head
 -->
  <xsl:template name="style">
    <link rel="SHORTCUT ICON" href="/favicon.ico"/> 
    <style type="text/css">
TD {font-family: Verdana,Arial,Helvetica}
BODY {font-family: Verdana,Arial,Helvetica; margin-top: 2em; margin-left: 0em; margin-right: 0em}
H1 {font-family: Verdana,Arial,Helvetica}
H2 {font-family: Verdana,Arial,Helvetica}
H3 {font-family: Verdana,Arial,Helvetica}
A:link, A:visited, A:active { text-decoration: underline }
</style>
  </xsl:template>
<!--
 - Write the title box on top
 -->
  <xsl:template name="titlebox">
    <xsl:param name="title" select="'Main Page'"/>
    <table border="0" width="100%" cellpadding="5" cellspacing="0" align="center">
    <tr>
    <td width="120">
    <a href="http://swpat.ffii.org/"><img src="{$href_base}epatents.png" alt="Action against software patents"/></a>
    </td>
    <td width="180">
    <a href="http://www.gnome.org/"><img src="{$href_base}gnome2.png" alt="Gnome2 Logo"/></a>
    <a href="http://www.w3.org/Status"><img src="{$href_base}w3c.png" alt="W3C Logo"/></a>
    <a href="http://www.redhat.com/"><img src="{$href_base}redhat.gif" alt="Red Hat Logo"/></a>
    <div align="left">
    <a href="http://xmlsoft.org/"><img src="{$href_base}Libxml2-Logo-180x168.gif" alt="Made with Libxml2 Logo"/></a>
    </div>
    </td>
    <td>
    <table border="0" width="90%" cellpadding="2" cellspacing="0" align="center" bgcolor="#000000">
      <tr>
        <td>
          <table width="100%" border="0" cellspacing="1" cellpadding="3" bgcolor="#fffacd">
            <tr>
              <td align="center">
                <xsl:element name="h1">
                  <xsl:value-of select="$globaltitle"/>
                </xsl:element>
                <xsl:element name="h2">
                  <xsl:value-of select="$title"/>
                </xsl:element>
              </td>
            </tr>
          </table>
        </td>
      </tr>
    </table>
    </td>
    </tr>
    </table>
  </xsl:template>
<!--
 - Handling of nodes in the body before the first H2, table of content
 - Everything is just copied over, except href which may get rewritten
 - and h1/h2/a at the top level
 -->
  <xsl:template priority="2" mode="subcontent" match="a">
    <xsl:variable name="filename">
      <xsl:call-template name="filename">
        <xsl:with-param name="name" select="string(@href)"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:copy>
      <xsl:attribute name="href">
        <xsl:value-of select="$filename"/>
      </xsl:attribute>
      <xsl:apply-templates mode="subcontent" select="node()"/>
    </xsl:copy>
  </xsl:template>
  <xsl:template mode="subcontent" match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates mode="subcontent" select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
  <xsl:template mode="content" match="@*|node()">
    <xsl:if test="name() != 'h1' and name() != 'h2'">
      <xsl:copy>
        <xsl:apply-templates mode="subcontent" select="@*|node()"/>
      </xsl:copy>
    </xsl:if>
  </xsl:template>
<!--
 - Handling of nodes in the body after an H2
 - Open a new file and dump all the siblings up to the next H2
 -->
  <xsl:template name="subfile">
    <xsl:param name="header" select="following-sibling::h2[1]"/>
    <xsl:variable name="filename">
      <xsl:call-template name="filename">
        <xsl:with-param name="name" select="concat('#', string($header/a[1]/@name))"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="tocfilename">
      <xsl:call-template name="tocfilename">
        <xsl:with-param name="name" select="concat('#', string($header/a[1]/@name))"/>
      </xsl:call-template>
    </xsl:variable>
    <xsl:variable name="title">
      <xsl:value-of select="$header"/>
    </xsl:variable>
    <xsl:document href="{$filename}" method="xml" encoding="ISO-8859-1"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
      <html>
        <head>
          <xsl:call-template name="style"/>
          <xsl:element name="title">
            <xsl:value-of select="$title"/>
          </xsl:element>
        </head>
        <body bgcolor="#8b7765" text="#000000" link="#a06060" vlink="#000000">
          <xsl:call-template name="titlebox">
            <xsl:with-param name="title" select="$title"/>
          </xsl:call-template>
          <table border="0" cellpadding="4" cellspacing="0" width="100%" align="center">
            <tr>
              <td bgcolor="#8b7765">
                <table border="0" cellspacing="0" cellpadding="2" width="100%">
                  <tr>
                    <td valign="top" width="200" bgcolor="#8b7765">
		      <xsl:choose>
		        <xsl:when test="$filename = 'docs.html'">
                          <xsl:call-template name="develtoc"/>
		        </xsl:when>
		        <xsl:when test="$tocfilename = ''">
                          <xsl:call-template name="develtoc"/>
		        </xsl:when>
		        <xsl:otherwise>
                          <xsl:call-template name="toc"/>
		        </xsl:otherwise>
		      </xsl:choose>
                    </td>
                    <td valign="top" bgcolor="#8b7765">
                      <table border="0" cellspacing="0" cellpadding="1" width="100%">
                        <tr>
                          <td>
                            <table border="0" cellspacing="0" cellpadding="1" width="100%" bgcolor="#000000">
                              <tr>
                                <td>
                                  <table border="0" cellpadding="3" cellspacing="1" width="100%">
                                    <tr>
                                      <td bgcolor="#fffacd">
                                        <xsl:apply-templates mode="subfile" select="$header/following-sibling::*[preceding-sibling::h2[1] = $header         and name() != 'h2' ]"/>
					<p><a href="bugs.html">Daniel Veillard</a></p>
                                      </td>
                                    </tr>
                                  </table>
                                </td>
                              </tr>
                            </table>
                          </td>
                        </tr>
                      </table>
                    </td>
                  </tr>
                </table>
              </td>
            </tr>
          </table>
        </body>
      </html>
    </xsl:document>
  </xsl:template>
  <xsl:template mode="subfile" match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates mode="content" select="@*|node()"/>
    </xsl:copy>
  </xsl:template>
<!--
 - Handling of the initial body and head HTML document
 -->
  <xsl:template match="body">
    <xsl:variable name="firsth2" select="./h2[1]"/>
    <xsl:variable name="rest2" select="./h2[position()&gt;1]"/>
    <body bgcolor="#8b7765" text="#000000" link="#a06060" vlink="#000000">
      <xsl:call-template name="titlebox">
        <xsl:with-param name="title" select="'libxml'"/>
      </xsl:call-template>
      <table border="0" cellpadding="4" cellspacing="0" width="100%" align="center">
        <tr>
          <td bgcolor="#8b7765">
            <table border="0" cellspacing="0" cellpadding="2" width="100%">
              <tr>
                <td valign="top" width="200" bgcolor="#8b7765">
                  <xsl:call-template name="toc"/>
                </td>
                <td valign="top" bgcolor="#8b7765">
                  <table border="0" cellspacing="0" cellpadding="1" width="100%">
                    <tr>
                      <td>
                        <table border="0" cellspacing="0" cellpadding="1" width="100%" bgcolor="#000000">
                          <tr>
                            <td>
                              <table border="0" cellpadding="3" cellspacing="1" width="100%">
                                <tr>
                                  <td bgcolor="#fffacd">
                                    <xsl:apply-templates mode="content" select="($firsth2/preceding-sibling::*)"/>
                                    <xsl:for-each select="./h2">
                                      <xsl:call-template name="subfile">
                                        <xsl:with-param name="header" select="."/>
                                      </xsl:call-template>
                                    </xsl:for-each>
				    <p><a href="bugs.html">Daniel Veillard</a></p>
                                  </td>
                                </tr>
                              </table>
                            </td>
                          </tr>
                        </table>
                      </td>
                    </tr>
                  </table>
                </td>
              </tr>
            </table>
          </td>
        </tr>
      </table>
    </body>
  </xsl:template>
  <xsl:template match="head">
    <head>
      <xsl:call-template name="style"/>
      <xsl:apply-templates mode="head"/>
    </head>
  </xsl:template>
  <xsl:template match="html">
    <xsl:message>Generating the Web pages</xsl:message>
    <html>
      <xsl:apply-templates/>
    </html>
  </xsl:template>
</xsl:stylesheet>
