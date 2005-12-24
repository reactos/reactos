<?xml version="1.0"?>
<!--
  Stylesheet generating the XSA entry for libxml2 based on the 
  latest News entry.
  See http://www.garshol.priv.no/download/xsa/ for a description of XSA
 -->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0"
     xmlns:xhtml="http://www.w3.org/1999/xhtml" exclude-result-prefixes="xhtml">
  <xsl:output method="xml"
      doctype-public="-//LM Garshol//DTD XML Software Autoupdate 1.0//EN//XML"
      doctype-system="http://www.garshol.priv.no/download/xsa/xsa.dtd"
      indent="yes"/>

  <xsl:template match="/">
<xsa>
  <vendor>
    <name>Daniel Veillard</name>
    <email>daniel@veillard.com</email>
    <url>http://veillard.com/</url>
  </vendor>
  <product id="libxml2">
    <name>libxml2</name>
    <version><xsl:value-of select="substring-before(//xhtml:h3[2], ':')"/></version>
    <last-release><xsl:value-of select="substring-after(//xhtml:h3[2], ':')"/></last-release>
    <info-url>http://xmlsoft.org/</info-url>
    <changes>
    <xsl:apply-templates select="//xhtml:h3[2]/following-sibling::*[1]"/>
    </changes>
  </product>
</xsa>
  </xsl:template>
  <xsl:template match="xhtml:h3">
  </xsl:template>
  <xsl:template match="xhtml:ul">
    <xsl:apply-templates select=".//xhtml:li"/>
    <xsl:text>
</xsl:text>
  </xsl:template>
  <xsl:template match="xhtml:li">
    <xsl:text>   - </xsl:text>
    <xsl:value-of select="."/>
    <xsl:text>
</xsl:text>
  </xsl:template>
  <xsl:template match="xhtml:a">
    <xsl:value-of select="."/>
    <xsl:text> at 
</xsl:text>
    <xsl:value-of select="@href"/>
    <xsl:text>
</xsl:text>
  </xsl:template>
</xsl:stylesheet>

