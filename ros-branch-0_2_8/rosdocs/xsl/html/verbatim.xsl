<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:sverb="http://nwalsh.com/xslt/ext/com.nwalsh.saxon.Verbatim"
                xmlns:xverb="com.nwalsh.xalan.Verbatim"
                xmlns:lxslt="http://xml.apache.org/xslt"
                exclude-result-prefixes="sverb xverb lxslt"
                version='1.0'>

<!-- ********************************************************************
     $Id: verbatim.xsl,v 1.1 2002/06/13 20:32:42 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<lxslt:component prefix="xverb"
                 functions="numberLines"/>

<xsl:template match="programlisting|screen|synopsis">
  <xsl:param name="suppress-numbers" select="'0'"/>
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:call-template name="anchor"/>

  <xsl:variable name="content">
    <xsl:choose>
      <xsl:when test="$suppress-numbers = '0'
                      and @linenumbering = 'numbered'
                      and $use.extensions != '0'
                      and $linenumbering.extension != '0'">
        <xsl:variable name="rtf">
          <xsl:apply-templates/>
        </xsl:variable>
        <pre class="{name(.)}">
          <xsl:call-template name="number.rtf.lines">
            <xsl:with-param name="rtf" select="$rtf"/>
          </xsl:call-template>
        </pre>
      </xsl:when>
      <xsl:otherwise>
        <pre class="{name(.)}">
          <xsl:apply-templates/>
        </pre>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$shade.verbatim != 0">
      <table xsl:use-attribute-sets="shade.verbatim.style">
        <tr>
          <td>
            <xsl:copy-of select="$content"/>
          </td>
        </tr>
      </table>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$content"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="literallayout">
  <xsl:param name="suppress-numbers" select="'0'"/>

  <xsl:variable name="rtf">
    <xsl:apply-templates/>
  </xsl:variable>

  <xsl:variable name="content">
    <xsl:choose>
      <xsl:when test="$suppress-numbers = '0'
                      and @linenumbering = 'numbered'
                      and $use.extensions != '0'
                      and $linenumbering.extension != '0'">
        <xsl:choose>
          <xsl:when test="@class='monospaced'">
            <pre class="{name(.)}">
              <xsl:call-template name="number.rtf.lines">
                <xsl:with-param name="rtf" select="$rtf"/>
              </xsl:call-template>
            </pre>
          </xsl:when>
          <xsl:otherwise>
            <div class="{name(.)}">
              <p>
                <xsl:call-template name="number.rtf.lines">
                  <xsl:with-param name="rtf">
                    <xsl:call-template name="make-verbatim">
                      <xsl:with-param name="text" select="translate($rtf,' ','&#160;')"/>
                    </xsl:call-template>
                  </xsl:with-param>
                </xsl:call-template>
              </p>
            </div>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>

      <xsl:otherwise>
        <xsl:choose>
          <xsl:when test="@class='monospaced'">
            <pre class="{name(.)}">
              <xsl:copy-of select="$rtf"/>
            </pre>
          </xsl:when>
          <xsl:otherwise>
            <div class="{name(.)}">
              <p>
                <xsl:call-template name="make-verbatim">
                  <xsl:with-param name="text" select="translate($rtf,' ','&#160;')"/>
                </xsl:call-template>
              </p>
            </div>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$shade.verbatim != 0 and @class='monospaced'">
      <table xsl:use-attribute-sets="shade.verbatim.style">
        <tr>
          <td>
            <xsl:copy-of select="$content"/>
          </td>
        </tr>
      </table>
    </xsl:when>
    <xsl:otherwise>
      <xsl:copy-of select="$content"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="literallayout[not(@class)
                                   or @class != 'monospaced']//text()">
  <xsl:call-template name="make-verbatim">
    <xsl:with-param name="text" select="translate(.,' ','&#160;')"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="address">
  <xsl:param name="suppress-numbers" select="'0'"/>

  <xsl:variable name="rtf">
    <xsl:apply-templates/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$suppress-numbers = '0'
                    and @linenumbering = 'numbered'
                    and $use.extensions != '0'
                    and $linenumbering.extension != '0'">
      <div class="{name(.)}">
        <p>
          <xsl:call-template name="number.rtf.lines">
            <xsl:with-param name="rtf" select="$rtf"/>
          </xsl:call-template>
        </p>
      </div>
    </xsl:when>

    <xsl:otherwise>
      <div class="{name(.)}">
        <p>
          <xsl:apply-templates/>
        </p>
      </div>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="number.rtf.lines">
  <xsl:param name="rtf" select="''"/>
  <xsl:param name="pi.context" select="."/>

  <!-- Save the global values -->
  <xsl:variable name="global.linenumbering.everyNth"
                select="$linenumbering.everyNth"/>

  <xsl:variable name="global.linenumbering.separator"
                select="$linenumbering.separator"/>

  <xsl:variable name="global.linenumbering.width"
                select="$linenumbering.width"/>

  <!-- Extract the <?dbhtml linenumbering.*?> PI values -->
  <xsl:variable name="pi.linenumbering.everyNth">
    <xsl:call-template name="dbhtml-attribute">
      <xsl:with-param name="pis"
                      select="$pi.context/processing-instruction('dbhtml')"/>
      <xsl:with-param name="attribute" select="'linenumbering.everyNth'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="pi.linenumbering.separator">
    <xsl:call-template name="dbhtml-attribute">
      <xsl:with-param name="pis"
                      select="$pi.context/processing-instruction('dbhtml')"/>
      <xsl:with-param name="attribute" select="'linenumbering.separator'"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="pi.linenumbering.width">
    <xsl:call-template name="dbhtml-attribute">
      <xsl:with-param name="pis"
                      select="$pi.context/processing-instruction('dbhtml')"/>
      <xsl:with-param name="attribute" select="'linenumbering.width'"/>
    </xsl:call-template>
  </xsl:variable>

  <!-- Construct the 'in-context' values -->
  <xsl:variable name="linenumbering.everyNth">
    <xsl:choose>
      <xsl:when test="$pi.linenumbering.everyNth != ''">
        <xsl:value-of select="$pi.linenumbering.everyNth"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$global.linenumbering.everyNth"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="linenumbering.separator">
    <xsl:choose>
      <xsl:when test="$pi.linenumbering.separator != ''">
        <xsl:value-of select="$pi.linenumbering.separator"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$global.linenumbering.separator"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="linenumbering.width">
    <xsl:choose>
      <xsl:when test="$pi.linenumbering.width != ''">
        <xsl:value-of select="$pi.linenumbering.width"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$global.linenumbering.width"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="function-available('sverb:numberLines')">
      <xsl:copy-of select="sverb:numberLines($rtf)"/>
    </xsl:when>
    <xsl:when test="function-available('xverb:numberLines')">
      <xsl:copy-of select="xverb:numberLines($rtf)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message terminate="yes">
        <xsl:text>No numberLines function available.</xsl:text>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="address//text()">
  <xsl:call-template name="make-verbatim">
    <xsl:with-param name="text" select="translate(.,' ','&#160;')"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="make-verbatim">
  <xsl:param name="text" select="''"/>

  <xsl:call-template name="make-verbatim-recursive">
    <xsl:with-param name="text" select="translate($text, ' ', '&#160;')"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="make-verbatim-recursive">
  <xsl:param name="text" select="''"/>

  <xsl:choose>
    <xsl:when test="not(contains($text, '&#xA;'))">
      <xsl:value-of select="$text"/>
    </xsl:when>

    <xsl:otherwise>
      <xsl:variable name="len" select="string-length($text)"/>

      <xsl:choose>
        <xsl:when test="$len = 1">
          <br/><xsl:text>&#xA;</xsl:text>
        </xsl:when>

        <xsl:otherwise>
    	  <xsl:variable name="half" select="$len div 2"/>
    	  <xsl:call-template name="make-verbatim-recursive">
    	    <xsl:with-param name="text" select="substring($text, 1, $half)"/>
    	  </xsl:call-template>
    	  <xsl:call-template name="make-verbatim-recursive">
    	    <xsl:with-param name="text"
    			    select="substring($text, ($half + 1), $len)"/>
    	  </xsl:call-template>
    	</xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>





