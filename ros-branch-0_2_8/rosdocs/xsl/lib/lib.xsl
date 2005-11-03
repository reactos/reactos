<?xml version="1.0" encoding="utf-8"?>

<!-- ********************************************************************
     $Id: lib.xsl,v 1.1 2002/06/13 20:32:53 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     This module implements DTD-independent functions

     ******************************************************************** -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:src="http://nwalsh.com/xmlns/litprog/fragment" exclude-result-prefixes="src" version="1.0">


<xsl:template name="dot.count">
  <!-- Returns the number of "." characters in a string -->
  <xsl:param name="string"/>
  <xsl:param name="count" select="0"/>
  <xsl:choose>
    <xsl:when test="contains($string, '.')">
      <xsl:call-template name="dot.count">
        <xsl:with-param name="string" select="substring-after($string, '.')"/>
        <xsl:with-param name="count" select="$count+1"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$count"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="copy-string">
  <!-- returns 'count' copies of 'string' -->
  <xsl:param name="string"/>
  <xsl:param name="count" select="0"/>
  <xsl:param name="result"/>

  <xsl:choose>
    <xsl:when test="$count&gt;0">
      <xsl:call-template name="copy-string">
        <xsl:with-param name="string" select="$string"/>
        <xsl:with-param name="count" select="$count - 1"/>
        <xsl:with-param name="result">
          <xsl:value-of select="$result"/>
          <xsl:value-of select="$string"/>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$result"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="string.subst">
  <xsl:param name="string"/>
  <xsl:param name="target"/>
  <xsl:param name="replacement"/>

  <xsl:choose>
    <xsl:when test="contains($string, $target)">
      <xsl:variable name="rest">
        <xsl:call-template name="string.subst">
          <xsl:with-param name="string" select="substring-after($string, $target)"/>
          <xsl:with-param name="target" select="$target"/>
          <xsl:with-param name="replacement" select="$replacement"/>
        </xsl:call-template>
      </xsl:variable>
      <xsl:value-of select="concat(substring-before($string, $target),                                    $replacement,                                    $rest)"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$string"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="xpointer.idref">
  <xsl:param name="xpointer">http://...</xsl:param>
  <xsl:choose>
    <xsl:when test="starts-with($xpointer, '#xpointer(id(')">
      <xsl:variable name="rest" select="substring-after($xpointer, '#xpointer(id(')"/>
      <xsl:variable name="quote" select="substring($rest, 1, 1)"/>
      <xsl:value-of select="substring-before(substring-after($xpointer, $quote), $quote)"/>
    </xsl:when>
    <xsl:when test="starts-with($xpointer, '#')">
      <xsl:value-of select="substring-after($xpointer, '#')"/>
    </xsl:when>
    <!-- otherwise it's a pointer to some other document -->
  </xsl:choose>
</xsl:template>


<xsl:template name="length-magnitude">
  <xsl:param name="length" select="'0pt'"/>

  <xsl:choose>
    <xsl:when test="string-length($length) = 0"/>
    <xsl:when test="substring($length,1,1) = '0'                     or substring($length,1,1) = '1'                     or substring($length,1,1) = '2'                     or substring($length,1,1) = '3'                     or substring($length,1,1) = '4'                     or substring($length,1,1) = '5'                     or substring($length,1,1) = '6'                     or substring($length,1,1) = '7'                     or substring($length,1,1) = '8'                     or substring($length,1,1) = '9'                     or substring($length,1,1) = '.'">
      <xsl:value-of select="substring($length,1,1)"/>
      <xsl:call-template name="length-magnitude">
        <xsl:with-param name="length" select="substring($length,2)"/>
      </xsl:call-template>
    </xsl:when>
  </xsl:choose>
</xsl:template>


<xsl:template name="length-units">
  <xsl:param name="length" select="'0pt'"/>
  <xsl:param name="default.units" select="'px'"/>
  <xsl:variable name="magnitude">
    <xsl:call-template name="length-magnitude">
      <xsl:with-param name="length" select="$length"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="units">
    <xsl:value-of select="substring($length, string-length($magnitude)+1)"/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$units = ''">
      <xsl:value-of select="$default.units"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$units"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="length-spec">
  <xsl:param name="length" select="'0pt'"/>
  <xsl:param name="default.units" select="'px'"/>

  <xsl:variable name="magnitude">
    <xsl:call-template name="length-magnitude">
      <xsl:with-param name="length" select="$length"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="units">
    <xsl:value-of select="substring($length, string-length($magnitude)+1)"/>
  </xsl:variable>

  <xsl:value-of select="$magnitude"/>
  <xsl:choose>
    <xsl:when test="$units='cm'                     or $units='mm'                     or $units='in'                     or $units='pt'                     or $units='pc'                     or $units='px'                     or $units='em'">
      <xsl:value-of select="$units"/>
    </xsl:when>
    <xsl:when test="$units = ''">
      <xsl:value-of select="$default.units"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>
        <xsl:text>Unrecognized unit of measure: </xsl:text>
        <xsl:value-of select="$units"/>
        <xsl:text>.</xsl:text>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="length-in-points">
  <xsl:param name="length" select="'0pt'"/>
  <xsl:param name="em.size" select="10"/>
  <xsl:param name="pixels.per.inch" select="90"/>

  <xsl:variable name="magnitude">
    <xsl:call-template name="length-magnitude">
      <xsl:with-param name="length" select="$length"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="units">
    <xsl:value-of select="substring($length, string-length($magnitude)+1)"/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$units = 'pt'">
      <xsl:value-of select="$magnitude"/>
    </xsl:when>
    <xsl:when test="$units = 'cm'">
      <xsl:value-of select="$magnitude div 2.54 * 72.0"/>
    </xsl:when>
    <xsl:when test="$units = 'mm'">
      <xsl:value-of select="$magnitude div 25.4 * 72.0"/>
    </xsl:when>
    <xsl:when test="$units = 'in'">
      <xsl:value-of select="$magnitude * 72.0"/>
    </xsl:when>
    <xsl:when test="$units = 'pc'">
      <xsl:value-of select="$magnitude div 6.0 * 72.0"/>
    </xsl:when>
    <xsl:when test="$units = 'px'">
      <xsl:value-of select="$magnitude div $pixels.per.inch * 72.0"/>
    </xsl:when>
    <xsl:when test="$units = 'em'">
      <xsl:value-of select="$magnitude * $em.size"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>
        <xsl:text>Unrecognized unit of measure: </xsl:text>
        <xsl:value-of select="$units"/>
        <xsl:text>.</xsl:text>
      </xsl:message>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="pi-attribute">
  <xsl:param name="pis" select="processing-instruction('')"/>
  <xsl:param name="attribute">filename</xsl:param>
  <xsl:param name="count">1</xsl:param>

  <xsl:choose>
    <xsl:when test="$count&gt;count($pis)">
      <!-- not found -->
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="pi">
        <xsl:value-of select="$pis[$count]"/>
      </xsl:variable>
      <xsl:choose>
        <xsl:when test="contains($pi,concat($attribute, '='))">
          <xsl:variable name="rest" select="substring-after($pi,concat($attribute,'='))"/>
          <xsl:variable name="quote" select="substring($rest,1,1)"/>
          <xsl:value-of select="substring-before(substring($rest,2),$quote)"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="pi-attribute">
            <xsl:with-param name="pis" select="$pis"/>
            <xsl:with-param name="attribute" select="$attribute"/>
            <xsl:with-param name="count" select="$count + 1"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="lookup.key">
  <xsl:param name="key" select="''"/>
  <xsl:param name="table" select="''"/>

  <xsl:if test="contains($table, ' ')">
    <xsl:choose>
      <xsl:when test="substring-before($table, ' ') = $key">
        <xsl:variable name="rest" select="substring-after($table, ' ')"/>
        <xsl:choose>
          <xsl:when test="contains($rest, ' ')">
            <xsl:value-of select="substring-before($rest, ' ')"/>
          </xsl:when>
          <xsl:otherwise>
            <xsl:value-of select="$rest"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:call-template name="lookup.key">
          <xsl:with-param name="key" select="$key"/>
          <xsl:with-param name="table" select="substring-after(substring-after($table,' '), ' ')"/>
        </xsl:call-template>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:if>
</xsl:template>


<xsl:template name="xpath.location">
  <xsl:param name="node" select="."/>
  <xsl:param name="path" select="''"/>

  <xsl:variable name="next.path">
    <xsl:value-of select="local-name($node)"/>
    <xsl:if test="$path != ''">/</xsl:if>
    <xsl:value-of select="$path"/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$node/parent::*">
      <xsl:call-template name="xpath.location">
        <xsl:with-param name="node" select="$node/parent::*"/>
        <xsl:with-param name="path" select="$next.path"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>/</xsl:text>
      <xsl:value-of select="$next.path"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="count.uri.path.depth">
  <xsl:param name="filename" select="''"/>
  <xsl:param name="count" select="0"/>

  <xsl:choose>
    <xsl:when test="contains($filename, '/')">
      <xsl:call-template name="count.uri.path.depth">
        <xsl:with-param name="filename" select="substring-after($filename, '/')"/>
        <xsl:with-param name="count" select="$count + 1"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="$count"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


<xsl:template name="trim.common.uri.paths">
  <xsl:param name="uriA" select="''"/>
  <xsl:param name="uriB" select="''"/>
  <xsl:param name="return" select="'A'"/>

  <xsl:choose>
    <xsl:when test="contains($uriA, '/') and contains($uriB, '/')                     and substring-before($uriA, '/') = substring-before($uriB, '/')">
      <xsl:call-template name="trim.common.uri.paths">
        <xsl:with-param name="uriA" select="substring-after($uriA, '/')"/>
        <xsl:with-param name="uriB" select="substring-after($uriB, '/')"/>
        <xsl:with-param name="return" select="$return"/>
      </xsl:call-template>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="$return = 'A'">
          <xsl:value-of select="$uriA"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:value-of select="$uriB"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>


</xsl:stylesheet>
