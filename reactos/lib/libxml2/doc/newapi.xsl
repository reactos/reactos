<?xml version="1.0"?>
<!--
  Stylesheet to generate the HTML documentation from an XML API descriptions:
  xsltproc newapi.xsl libxml2-api.xml

  Daniel Veillard
-->
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:exsl="http://exslt.org/common"
  xmlns:str="http://exslt.org/strings"
  extension-element-prefixes="exsl str"
  exclude-result-prefixes="exsl str">

  <!-- Import the main part of the site stylesheets -->
  <xsl:import href="site.xsl"/>

  <!-- Generate XHTML-1.0 transitional -->
  <xsl:output method="xml" encoding="ISO-8859-1" indent="yes"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"/>

  <!-- Build keys for all symbols -->
  <xsl:key name="symbols" match="/api/symbols/*" use="@name"/>

  <!-- the target directory for the HTML output -->
  <xsl:variable name="htmldir">html</xsl:variable>
  <xsl:variable name="href_base">../</xsl:variable>

  <!-- The table of content for the HTML API pages -->
  <xsl:variable name="menu_name">API Menu</xsl:variable>
  <xsl:variable name="apitoc">
    <form action="../search.php"
          enctype="application/x-www-form-urlencoded" method="get">
      <input name="query" type="text" size="20" value=""/>
      <input name="submit" type="submit" value="Search ..."/>
    </form>
    <ul><!-- style="margin-left: -1em" -->
      <li><a style="font-weight:bold"
             href="{$href_base}index.html">Main Menu</a></li>
      <li><a style="font-weight:bold" 
             href="{$href_base}docs.html">Developer Menu</a></li>
      <li><a style="font-weight:bold" 
             href="{$href_base}examples/index.html">Code Examples</a></li>
      <li><a style="font-weight:bold"
             href="index.html">API Menu</a></li>
      <li><a href="libxml-parser.html">Parser API</a></li>
      <li><a href="libxml-tree.html">Tree API</a></li>
      <li><a href="libxml-xmlreader.html">Reader API</a></li>
      <li><a href="{$href_base}guidelines.html">XML Guidelines</a></li>
      <li><a href="{$href_base}ChangeLog.html">ChangeLog</a></li>
    </ul>
  </xsl:variable>
  <xsl:template name="apitoc">
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
                <xsl:copy-of select="$apitoc"/>
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

  <xsl:template name="docstyle">
    <style type="text/css">
      div.deprecated pre.programlisting {border-style: double;border-color:red}
      pre.programlisting {border-style: double;background: #EECFA1}
    </style>
  </xsl:template>
  <xsl:template name="navbar">
    <xsl:variable name="previous" select="preceding-sibling::file[1]"/>
    <xsl:variable name="next" select="following-sibling::file[1]"/>
    <table class="navigation" width="100%" summary="Navigation header"
           cellpadding="2" cellspacing="2">
      <tr valign="middle">
        <xsl:if test="$previous">
          <td><a accesskey="p" href="libxml-{$previous/@name}.html"><img src="left.png" width="24" height="24" border="0" alt="Prev"></img></a></td>
	  <th align="left"><a href="libxml-{$previous/@name}.html"><xsl:value-of select="$previous/@name"/></a></th>
	</xsl:if>
        <td><a accesskey="u" href="index.html"><img src="up.png" width="24" height="24" border="0" alt="Up"></img></a></td>
	<th align="left"><a href="index.html">API documentation</a></th>
        <td><a accesskey="h" href="../index.html"><img src="home.png" width="24" height="24" border="0" alt="Home"></img></a></td>
        <th align="center"><a href="../index.html">The XML C parser and toolkit of Gnome</a></th>
        <xsl:if test="$next">
	  <th align="right"><a href="libxml-{$next/@name}.html"><xsl:value-of select="$next/@name"/></a></th>
          <td><a accesskey="n" href="libxml-{$next/@name}.html"><img src="right.png" width="24" height="24" border="0" alt="Next"></img></a></td>
        </xsl:if>
      </tr>
    </table>
  </xsl:template>

  <!-- This is convoluted but needed to force the current document to
       be the API one and not the result tree from the tokenize() result,
       because the keys are only defined on the main document -->
  <xsl:template mode="dumptoken" match='*'>
    <xsl:param name="token"/>
    <xsl:variable name="ref" select="key('symbols', $token)"/>
    <xsl:choose>
      <xsl:when test="$ref">
        <a href="libxml-{$ref/@file}.html#{$ref/@name}"><xsl:value-of select="$token"/></a>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$token"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- dumps a string, making cross-reference links -->
  <xsl:template name="dumptext">
    <xsl:param name="text"/>
    <xsl:variable name="ctxt" select='.'/>
    <!-- <xsl:value-of select="$text"/> -->
    <xsl:for-each select="str:tokenize($text, ' &#9;')">
      <xsl:apply-templates select="$ctxt" mode='dumptoken'>
        <xsl:with-param name="token" select="string(.)"/>
      </xsl:apply-templates>
      <xsl:if test="position() != last()">
        <xsl:text> </xsl:text>
      </xsl:if>
    </xsl:for-each>
  </xsl:template>

  <xsl:template match="macro" mode="toc">
    <pre class="programlisting">
    <xsl:text>#define </xsl:text><a href="#{@name}"><xsl:value-of select="@name"/></a>
    </pre>
  </xsl:template>

  <xsl:template match="variable" mode="toc">
    <pre class="programlisting">
    <xsl:text>Variable </xsl:text>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="string(@type)"/>
    </xsl:call-template>
    <xsl:text> </xsl:text>
    <a name="{@name}"></a>
    <xsl:value-of select="@name"/>
    <xsl:text>

</xsl:text>
    </pre>
  </xsl:template>

  <xsl:template match="typedef" mode="toc">
    <xsl:variable name="name" select="string(@name)"/>
    <pre class="programlisting">
    <xsl:choose>
      <xsl:when test="@type = 'enum'">
	<xsl:text>Enum </xsl:text>
	<a href="#{$name}"><xsl:value-of select="$name"/></a>
	<xsl:text>
</xsl:text>
      </xsl:when>
      <xsl:otherwise>
	<xsl:text>Typedef </xsl:text>
	<xsl:call-template name="dumptext">
	  <xsl:with-param name="text" select="@type"/>
	</xsl:call-template>
	<xsl:text> </xsl:text>
	<a name="{$name}"><xsl:value-of select="$name"/></a>
	<xsl:text>
</xsl:text>
      </xsl:otherwise>
    </xsl:choose>
    </pre>
  </xsl:template>

  <xsl:template match="typedef[@type = 'enum']">
    <xsl:variable name="name" select="string(@name)"/>
    <h3>Enum <a name="{$name}"><xsl:value-of select="$name"/></a></h3>
    <pre class="programlisting">
      <xsl:text>Enum </xsl:text>
      <xsl:value-of select="$name"/>
      <xsl:text> {
</xsl:text>
      <xsl:for-each select="/api/symbols/enum[@type = $name]">
        <xsl:sort select="@value" data-type="number" order="ascending"/>
        <xsl:text>    </xsl:text>
        <a name="{@name}"><xsl:value-of select="@name"/></a>
        <xsl:text> = </xsl:text>
        <xsl:value-of select="@value"/>
        <xsl:if test="@info != ''">
	  <xsl:text> : </xsl:text>
	  <xsl:call-template name="dumptext">
	    <xsl:with-param name="text" select="@info"/>
	  </xsl:call-template>
        </xsl:if>
        <xsl:text>
</xsl:text>
      </xsl:for-each>
      <xsl:text>}
</xsl:text>
    </pre>
  </xsl:template>

  <xsl:template match="struct" mode="toc">
    <pre class="programlisting">
    <xsl:text>Structure </xsl:text><a href="#{@name}"><xsl:value-of select="@name"/></a><br/>
    <xsl:value-of select="@type"/><xsl:text>
</xsl:text>
    <xsl:if test="not(field)">
      <xsl:text>The content of this structure is not made public by the API.
</xsl:text>
    </xsl:if>
    </pre>
  </xsl:template>

  <xsl:template match="struct">
    <h3><a name="{@name}">Structure <xsl:value-of select="@name"/></a></h3>
    <pre class="programlisting">
    <xsl:text>Structure </xsl:text><xsl:value-of select="@name"/><br/>
    <xsl:value-of select="@type"/><xsl:text> {
</xsl:text>
    <xsl:if test="not(field)">
      <xsl:text>The content of this structure is not made public by the API.
</xsl:text>
    </xsl:if>
    <xsl:for-each select="field">
        <xsl:text>    </xsl:text>
	<xsl:call-template name="dumptext">
	  <xsl:with-param name="text" select="@type"/>
	</xsl:call-template>
	<xsl:text>&#9;</xsl:text>
	<xsl:value-of select="@name"/>
	<xsl:if test="@info != ''">
	  <xsl:text>&#9;: </xsl:text>
	  <xsl:call-template name="dumptext">
	    <xsl:with-param name="text" select="substring(@info, 1, 40)"/>
	  </xsl:call-template>
	</xsl:if>
	<xsl:text>
</xsl:text>
    </xsl:for-each>
    <xsl:text>}</xsl:text>
    </pre>
  </xsl:template>

  <xsl:template match="macro">
    <xsl:variable name="name" select="string(@name)"/>
    <h3><a name="{$name}"></a>Macro: <xsl:value-of select="$name"/></h3>
    <pre><xsl:text>#define </xsl:text><xsl:value-of select="$name"/></pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p><xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="function" mode="toc">
    <xsl:variable name="name" select="string(@name)"/>
    <xsl:variable name="nlen" select="string-length($name)"/>
    <xsl:variable name="tlen" select="string-length(return/@type)"/>
    <xsl:variable name="blen" select="(($nlen + 8) - (($nlen + 8) mod 8)) + (($tlen + 8) - (($tlen + 8) mod 8))"/>
    <pre class="programlisting">
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="return/@type"/>
    </xsl:call-template>
    <xsl:text>&#9;</xsl:text>
    <a href="#{@name}"><xsl:value-of select="@name"/></a>
    <xsl:if test="$blen - 40 &lt; -8">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:if test="$blen - 40 &lt; 0">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;(</xsl:text>
    <xsl:if test="not(arg)">
      <xsl:text>void</xsl:text>
    </xsl:if>
    <xsl:for-each select="arg">
      <xsl:call-template name="dumptext">
        <xsl:with-param name="text" select="@type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="position() != last()">
        <xsl:text>, </xsl:text><br/>
	<xsl:if test="$blen - 40 &gt; 8">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:if test="$blen - 40 &gt; 0">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:text>&#9;&#9;&#9;&#9;&#9; </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>)</xsl:text>
    </pre><xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="functype" mode="toc">
    <xsl:variable name="name" select="string(@name)"/>
    <xsl:variable name="nlen" select="string-length($name)"/>
    <xsl:variable name="tlen" select="string-length(return/@type)"/>
    <xsl:variable name="blen" select="(($nlen + 8) - (($nlen + 8) mod 8)) + (($tlen + 8) - (($tlen + 8) mod 8))"/>
    <pre class="programlisting">
    <xsl:text>Function type: </xsl:text>
    <a href="#{$name}"><xsl:value-of select="$name"/></a>
    <xsl:text>
</xsl:text>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="return/@type"/>
    </xsl:call-template>
    <xsl:text>&#9;</xsl:text>
    <a href="#{$name}"><xsl:value-of select="$name"/></a>
    <xsl:if test="$blen - 40 &lt; -8">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:if test="$blen - 40 &lt; 0">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;(</xsl:text>
    <xsl:if test="not(arg)">
      <xsl:text>void</xsl:text>
    </xsl:if>
    <xsl:for-each select="arg">
      <xsl:call-template name="dumptext">
        <xsl:with-param name="text" select="@type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="position() != last()">
        <xsl:text>, </xsl:text><br/>
	<xsl:if test="$blen - 40 &gt; 8">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:if test="$blen - 40 &gt; 0">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:text>&#9;&#9;&#9;&#9;&#9; </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>)
</xsl:text>
    </pre>
    <xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="functype">
    <xsl:variable name="name" select="string(@name)"/>
    <xsl:variable name="nlen" select="string-length($name)"/>
    <xsl:variable name="tlen" select="string-length(return/@type)"/>
    <xsl:variable name="blen" select="(($nlen + 8) - (($nlen + 8) mod 8)) + (($tlen + 8) - (($tlen + 8) mod 8))"/>
    <h3>
      <a name="{$name}"></a>
      <xsl:text>Function type: </xsl:text>
      <xsl:value-of select="$name"/>
    </h3>
    <pre class="programlisting">
    <xsl:text>Function type: </xsl:text>
    <xsl:value-of select="$name"/>
    <xsl:text>
</xsl:text>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="return/@type"/>
    </xsl:call-template>
    <xsl:text>&#9;</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:if test="$blen - 40 &lt; -8">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:if test="$blen - 40 &lt; 0">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;(</xsl:text>
    <xsl:if test="not(arg)">
      <xsl:text>void</xsl:text>
    </xsl:if>
    <xsl:for-each select="arg">
      <xsl:call-template name="dumptext">
        <xsl:with-param name="text" select="@type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="position() != last()">
        <xsl:text>, </xsl:text><br/>
	<xsl:if test="$blen - 40 &gt; 8">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:if test="$blen - 40 &gt; 0">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:text>&#9;&#9;&#9;&#9;&#9; </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>)
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p>
    <xsl:if test="arg | return">
      <div class="variablelist"><table border="0"><col align="left"/><tbody>
      <xsl:for-each select="arg">
        <tr>
          <td><span class="term"><i><tt><xsl:value-of select="@name"/></tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:for-each>
      <xsl:if test="return/@info">
        <tr>
          <td><span class="term"><i><tt>Returns</tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="return/@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:if>
      </tbody></table></div>
    </xsl:if>
    <br/>
    <xsl:text>
</xsl:text>
  </xsl:template>

  <xsl:template match="function">
    <xsl:variable name="name" select="string(@name)"/>
    <xsl:variable name="nlen" select="string-length($name)"/>
    <xsl:variable name="tlen" select="string-length(return/@type)"/>
    <xsl:variable name="blen" select="(($nlen + 8) - (($nlen + 8) mod 8)) + (($tlen + 8) - (($tlen + 8) mod 8))"/>
    <h3><a name="{$name}"></a>Function: <xsl:value-of select="$name"/></h3>
    <pre class="programlisting">
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="return/@type"/>
    </xsl:call-template>
    <xsl:text>&#9;</xsl:text>
    <xsl:value-of select="@name"/>
    <xsl:if test="$blen - 40 &lt; -8">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:if test="$blen - 40 &lt; 0">
      <xsl:text>&#9;</xsl:text>
    </xsl:if>
    <xsl:text>&#9;(</xsl:text>
    <xsl:if test="not(arg)">
      <xsl:text>void</xsl:text>
    </xsl:if>
    <xsl:for-each select="arg">
      <xsl:call-template name="dumptext">
        <xsl:with-param name="text" select="@type"/>
      </xsl:call-template>
      <xsl:text> </xsl:text>
      <xsl:value-of select="@name"/>
      <xsl:if test="position() != last()">
        <xsl:text>, </xsl:text><br/>
	<xsl:if test="$blen - 40 &gt; 8">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:if test="$blen - 40 &gt; 0">
	  <xsl:text>&#9;</xsl:text>
	</xsl:if>
	<xsl:text>&#9;&#9;&#9;&#9;&#9; </xsl:text>
      </xsl:if>
    </xsl:for-each>
    <xsl:text>)</xsl:text><br/>
    <xsl:text>
</xsl:text>
    </pre>
    <p>
    <xsl:call-template name="dumptext">
      <xsl:with-param name="text" select="info"/>
    </xsl:call-template>
    </p><xsl:text>
</xsl:text>
    <xsl:if test="arg | return/@info">
      <div class="variablelist"><table border="0"><col align="left"/><tbody>
      <xsl:for-each select="arg">
        <tr>
          <td><span class="term"><i><tt><xsl:value-of select="@name"/></tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:for-each>
      <xsl:if test="return/@info">
        <tr>
          <td><span class="term"><i><tt>Returns</tt></i>:</span></td>
	  <td>
	    <xsl:call-template name="dumptext">
	      <xsl:with-param name="text" select="return/@info"/>
	    </xsl:call-template>
	  </td>
        </tr>
      </xsl:if>
      </tbody></table></div>
    </xsl:if>
  </xsl:template>

  <xsl:template match="exports" mode="toc">
    <xsl:apply-templates select="key('symbols', string(@symbol))[1]" mode="toc"/>
  </xsl:template>

  <xsl:template match="exports">
    <xsl:apply-templates select="key('symbols', string(@symbol))[1]"/>
  </xsl:template>

  <xsl:template name="description">
    <xsl:if test="deprecated">
      <h2 style="font-weight:bold;color:red;text-align:center">This module is deprecated</h2>
    </xsl:if>
    <xsl:if test="description">
      <p><xsl:value-of select="description"/></p>
    </xsl:if>
  </xsl:template>

  <xsl:template name="docomponents">
    <xsl:param name="mode"/>
    <xsl:apply-templates select="exports[@type='macro']" mode="$mode">
      <xsl:sort select='@symbol'/>
    </xsl:apply-templates>
    <xsl:apply-templates select="exports[@type='enum']" mode="$mode">
      <xsl:sort select='@symbol'/>
    </xsl:apply-templates>
    <xsl:apply-templates select="exports[@type='typedef']" mode="$mode">
      <xsl:sort select='@symbol'/>
    </xsl:apply-templates>
    <xsl:apply-templates select="exports[@type='struct']" mode="$mode">
      <xsl:sort select='@symbol'/>
    </xsl:apply-templates>
    <xsl:apply-templates select="exports[@type='function']" mode="$mode">
      <xsl:sort select='@symbol'/>
    </xsl:apply-templates>
  </xsl:template>
  
  <xsl:template match="file">
    <xsl:variable name="name" select="@name"/>
    <xsl:variable name="title">Module <xsl:value-of select="$name"/> from <xsl:value-of select="/api/@name"/></xsl:variable>
    <xsl:document href="{$htmldir}/libxml-{$name}.html" method="xml" encoding="ISO-8859-1"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
	<html>
	  <head>
	    <xsl:call-template name="style"/>
	    <xsl:call-template name="docstyle"/>
	    <title><xsl:value-of select="$title"/></title>
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
		      <xsl:call-template name="apitoc"/>
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
	    <xsl:call-template name="navbar"/>
	    <xsl:call-template name="description"/>
	    <xsl:choose>
	      <xsl:when test="deprecated">
	        <div class="deprecated">
		  <h2>Table of Contents</h2>
		  <xsl:apply-templates select="exports" mode="toc"/>
		  <h2>Description</h2>
		  <xsl:text>
</xsl:text>
		  <xsl:apply-templates select="exports"/>
		</div>
	      </xsl:when>
	      <xsl:otherwise>
		<h2>Table of Contents</h2>
		<xsl:apply-templates select="exports[@type='macro']" mode="toc">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<xsl:apply-templates select="exports[@type='enum']" mode="toc">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<xsl:apply-templates select="exports[@type='typedef']" mode="toc">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<xsl:apply-templates select="exports[@type='struct']" mode="toc">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<xsl:apply-templates select="exports[@type='function']" mode="toc">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<h2>Description</h2>
		<xsl:text>
</xsl:text>
		<xsl:apply-templates select="exports[@type='macro']">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<xsl:apply-templates select="exports[@type='enum']">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<xsl:apply-templates select="exports[@type='typedef']">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<xsl:apply-templates select="exports[@type='struct']">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
		<xsl:apply-templates select="exports[@type='function']">
		  <xsl:sort select='@symbol'/>
		</xsl:apply-templates>
	      </xsl:otherwise>
	    </xsl:choose>
					<p><a href="{$href_base}bugs.html">Daniel Veillard</a></p>
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

  <xsl:template match="file" mode="toc">
    <xsl:variable name="name" select="@name"/>
    <li>
      <a href="libxml-{$name}.html"><xsl:value-of select="$name"/></a>
      <xsl:text>: </xsl:text>
      <xsl:value-of select="summary"/>
    </li>
  </xsl:template>

  <xsl:template name="mainpage">
    <xsl:param name="file" select="concat($htmldir, '/index.html')"/>
    <xsl:variable name="title">Reference Manual for <xsl:value-of select="/api/@name"/></xsl:variable>
    <xsl:document href="{$file}" method="xml" encoding="ISO-8859-1"
      doctype-public="-//W3C//DTD XHTML 1.0 Transitional//EN"
      doctype-system="http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
	<html>
	  <head>
	    <xsl:call-template name="style"/>
	    <xsl:call-template name="docstyle"/>
	    <title><xsl:value-of select="$title"/></title>
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
		      <xsl:call-template name="apitoc"/>
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
	    <h2>Table of Contents</h2>
	    <ul>
	    <xsl:apply-templates select="/api/files/file" mode="toc"/>
	    </ul>
					<p><a href="{$href_base}bugs.html">Daniel Veillard</a></p>
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

  <xsl:template match="/">
    <!-- Save the main index.html as well as a couple of copies -->
    <xsl:call-template name="mainpage"/>
    <xsl:call-template name="mainpage">
      <xsl:with-param name="file" select="concat($htmldir, '/book1.html')"/>
    </xsl:call-template>
    <xsl:call-template name="mainpage">
      <xsl:with-param name="file" select="concat($htmldir, '/libxml-lib.html')"/>
    </xsl:call-template>
    <!-- now build the file for each of the modules -->
    <xsl:apply-templates select="/api/files/file"/>
  </xsl:template>

</xsl:stylesheet>
