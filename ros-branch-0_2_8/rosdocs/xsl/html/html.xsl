<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>

<!-- ********************************************************************
     $Id: html.xsl,v 1.1 2002/06/13 20:32:36 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<xsl:template name="anchor">
  <xsl:param name="node" select="."/>
  <xsl:param name="conditional" select="1"/>
  <xsl:variable name="id">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$node"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:if test="$conditional = 0 or $node/@id">
    <a name="{$id}"/>
  </xsl:if>
</xsl:template>

<xsl:template name="href.target.uri">
  <xsl:param name="context" select="."/>
  <xsl:param name="object" select="."/>
  <xsl:text>#</xsl:text>
  <xsl:call-template name="object.id">
    <xsl:with-param name="object" select="$object"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="href.target">
  <xsl:param name="context" select="."/>
  <xsl:param name="object" select="."/>
  <xsl:text>#</xsl:text>
  <xsl:call-template name="object.id">
    <xsl:with-param name="object" select="$object"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="dingbat">
  <xsl:param name="dingbat">bullet</xsl:param>
  <xsl:call-template name="dingbat.characters">
    <xsl:with-param name="dingbat" select="$dingbat"/>
  </xsl:call-template>
</xsl:template>

<xsl:template name="dingbat.characters">
  <!-- now that I'm using the real serializer, all that dingbat malarky -->
  <!-- isn't necessary anymore... -->
  <xsl:param name="dingbat">bullet</xsl:param>
  <xsl:choose>
    <xsl:when test="$dingbat='bullet'">&#x2022;</xsl:when>
    <xsl:when test="$dingbat='copyright'">&#x00A9;</xsl:when>
    <xsl:when test="$dingbat='trademark'">&#x2122;</xsl:when>
    <xsl:when test="$dingbat='trade'">&#x2122;</xsl:when>
    <xsl:when test="$dingbat='registered'">&#x00AE;</xsl:when>
    <xsl:when test="$dingbat='service'">(SM)</xsl:when>
    <xsl:when test="$dingbat='nbsp'">&#x00A0;</xsl:when>
    <xsl:when test="$dingbat='ldquo'">&#x201C;</xsl:when>
    <xsl:when test="$dingbat='rdquo'">&#x201D;</xsl:when>
    <xsl:when test="$dingbat='lsquo'">&#x2018;</xsl:when>
    <xsl:when test="$dingbat='rsquo'">&#x2019;</xsl:when>
    <xsl:when test="$dingbat='em-dash'">&#x2014;</xsl:when>
    <xsl:when test="$dingbat='mdash'">&#x2014;</xsl:when>
    <xsl:when test="$dingbat='en-dash'">&#x2013;</xsl:when>
    <xsl:when test="$dingbat='ndash'">&#x2013;</xsl:when>
    <xsl:otherwise>
      <xsl:text>&#x2022;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

</xsl:stylesheet>

