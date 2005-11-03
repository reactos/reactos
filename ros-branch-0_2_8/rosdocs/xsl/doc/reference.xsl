<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version="1.0">

<xsl:import href="../html/docbook.xsl"/>

<xsl:param name="html.stylesheet" select="'reference.css'"/>

<xsl:template match="/legalnotice">
  <xsl:apply-templates select="." mode="titlepage.mode"/>
</xsl:template>

<xsl:template match="olink[@type='title']">
  <xsl:variable name="xml"
                select="document(unparsed-entity-uri(@targetdocent),.)"/>
  <xsl:variable name="title" select="($xml/*/title[1]
                                      |$xml/*/bookinfo/title[1]
                                      |$xml/*/referenceinfo/title[1])[1]"/>
  <i>
    <a href="{@localinfo}">
      <xsl:apply-templates select="$title/*|$title/text()"/>
    </a>
  </i>
</xsl:template>

<xsl:template match="copyright" mode="titlepage.mode">
  <xsl:variable name="years" select="year"/>
  <xsl:variable name="holders" select="holder"/>

  <p class="{name(.)}">
    <a href="copyright.html">
      <xsl:call-template name="gentext.element.name"/>
    </a>
    <xsl:call-template name="gentext.space"/>
    <xsl:call-template name="dingbat">
      <xsl:with-param name="dingbat">copyright</xsl:with-param>
    </xsl:call-template>
    <xsl:call-template name="gentext.space"/>
    <xsl:call-template name="copyright.years">
      <xsl:with-param name="years" select="year"/>
      <xsl:with-param name="print.ranges" select="1"/>
      <xsl:with-param name="single.year.ranges"
                      select="$make.single.year.ranges"/>
    </xsl:call-template>
    <xsl:call-template name="gentext.space"/>
    <xsl:call-template name="gentext.by"/>
    <xsl:call-template name="gentext.space"/>
    <xsl:apply-templates select="$holders" mode="titlepage.mode"/>
    <xsl:text>. </xsl:text>
    <a href="warranty.html">No Warranty</a>
    <xsl:text>.</xsl:text>
  </p>
</xsl:template>

</xsl:stylesheet>
