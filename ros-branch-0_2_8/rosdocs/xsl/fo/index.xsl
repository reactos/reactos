<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: index.xsl,v 1.1 2002/06/13 20:32:21 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="index">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="component.separator"/>
    <xsl:call-template name="index.titlepage"/>
    <xsl:apply-templates/>
    <xsl:if test="count(indexentry) = 0 and count(indexdiv) = 0">
      <xsl:call-template name="generate-index"/>
    </xsl:if>
  </fo:block>
</xsl:template>

<xsl:template match="book/index">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>
  <xsl:variable name="master-reference">
    <xsl:call-template name="select.pagemaster">
      <xsl:with-param name="column.count"
                      select="$column.count.of.index"/>
    </xsl:call-template>
  </xsl:variable>

  <fo:page-sequence id="{$id}"
                    hyphenate="{$hyphenate}"
                    master-reference="{$master-reference}">
    <xsl:attribute name="language">
      <xsl:call-template name="l10n.language"/>
    </xsl:attribute>
    <xsl:if test="$double.sided != 0">
      <xsl:attribute name="force-page-count">end-on-even</xsl:attribute>
    </xsl:if>

    <xsl:apply-templates select="." mode="running.head.mode">
      <xsl:with-param name="master-reference" select="$master-reference"/>
    </xsl:apply-templates>
    <xsl:apply-templates select="." mode="running.foot.mode">
      <xsl:with-param name="master-reference" select="$master-reference"/>
    </xsl:apply-templates>

    <fo:flow flow-name="xsl-region-body">
      <xsl:call-template name="index.titlepage"/>
      <xsl:apply-templates/>
      <xsl:if test="count(indexentry) = 0 and count(indexdiv) = 0">
        <xsl:call-template name="generate-index"/>
      </xsl:if>
    </fo:flow>
  </fo:page-sequence>
</xsl:template>

<xsl:template match="index/title"></xsl:template>
<xsl:template match="index/subtitle"></xsl:template>
<xsl:template match="index/titleabbrev"></xsl:template>

<!-- ==================================================================== -->

<xsl:template match="indexdiv">
  <fo:block>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="indexdiv/title">
  <xsl:variable name="id">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select=".."/>
    </xsl:call-template>
  </xsl:variable>
  <fo:block font-size="16pt" font-weight="bold">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="indexterm">
  <fo:wrapper>
    <xsl:attribute name="id">
      <xsl:call-template name="object.id"/>
    </xsl:attribute>
    <xsl:comment>
      <xsl:value-of select="primary"/>
      <xsl:if test="secondary">
        <xsl:text>, </xsl:text>
        <xsl:value-of select="secondary"/>
      </xsl:if>
      <xsl:if test="tertiary">
        <xsl:text>, </xsl:text>
        <xsl:value-of select="tertiary"/>
      </xsl:if>
    </xsl:comment>
  </fo:wrapper>
</xsl:template>

<xsl:template match="indexentry">
</xsl:template>

</xsl:stylesheet>
