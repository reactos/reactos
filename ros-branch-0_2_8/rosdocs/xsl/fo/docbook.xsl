<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
                exclude-result-prefixes="doc"
                version='1.0'>

<!-- It is important to use indent="no" here, otherwise verbatim -->
<!-- environments get broken by indented tags...at least when the -->
<!-- callout extension is used...at least with some processors -->
<xsl:output method="xml" indent="no"/>

<!-- ********************************************************************
     $Id: docbook.xsl,v 1.1 2002/06/13 20:32:20 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:include href="../VERSION"/>
<xsl:include href="param.xsl"/>
<xsl:include href="../lib/lib.xsl"/>
<xsl:include href="../common/l10n.xsl"/>
<xsl:include href="../common/common.xsl"/>
<xsl:include href="../common/labels.xsl"/>
<xsl:include href="../common/titles.xsl"/>
<xsl:include href="../common/subtitles.xsl"/>
<xsl:include href="../common/gentext.xsl"/>
<xsl:include href="autotoc.xsl"/>
<xsl:include href="autoidx.xsl"/>
<xsl:include href="lists.xsl"/>
<xsl:include href="callout.xsl"/>
<xsl:include href="verbatim.xsl"/>
<xsl:include href="graphics.xsl"/>
<xsl:include href="xref.xsl"/>
<xsl:include href="formal.xsl"/>
<xsl:include href="table.xsl"/>
<xsl:include href="sections.xsl"/>
<xsl:include href="inline.xsl"/>
<xsl:include href="footnote.xsl"/>
<xsl:include href="fo.xsl"/>
<xsl:include href="fo-rtf.xsl"/>
<xsl:include href="info.xsl"/>
<xsl:include href="keywords.xsl"/>
<xsl:include href="division.xsl"/>
<xsl:include href="index.xsl"/>
<xsl:include href="toc.xsl"/>
<xsl:include href="refentry.xsl"/>
<xsl:include href="math.xsl"/>
<xsl:include href="admon.xsl"/>
<xsl:include href="component.xsl"/>
<xsl:include href="biblio.xsl"/>
<xsl:include href="glossary.xsl"/>
<xsl:include href="block.xsl"/>
<xsl:include href="qandaset.xsl"/>
<xsl:include href="synop.xsl"/>
<xsl:include href="titlepage.xsl"/>
<xsl:include href="titlepage.templates.xsl"/>
<xsl:include href="pagesetup.xsl"/>
<xsl:include href="pi.xsl"/>
<xsl:include href="ebnf.xsl"/>

<xsl:include href="fop.xsl"/>
<xsl:include href="passivetex.xsl"/>
<xsl:include href="xep.xsl"/>

<xsl:param name="stylesheet.result.type" select="'fo'"/>

<!-- ==================================================================== -->

<xsl:key name="id" match="*" use="@id"/>

<!-- ==================================================================== -->

<xsl:template match="*">
  <xsl:message>
    <xsl:value-of select="name(.)"/>
    <xsl:text> encountered, but no template matches.</xsl:text>
  </xsl:message>
  <fo:block color="red">
    <xsl:text>&lt;</xsl:text>
    <xsl:value-of select="name(.)"/>
    <xsl:text>&gt;</xsl:text>
    <xsl:apply-templates/> 
    <xsl:text>&lt;/</xsl:text>
    <xsl:value-of select="name(.)"/>
    <xsl:text>&gt;</xsl:text>
  </fo:block>
</xsl:template>

<xsl:template match="/">
  <xsl:message>
    <xsl:text>Making </xsl:text>
    <xsl:value-of select="$page.orientation"/>
    <xsl:text> pages on </xsl:text>
    <xsl:value-of select="$paper.type"/>
    <xsl:text> paper (</xsl:text>
    <xsl:value-of select="$page.width"/>
    <xsl:text>x</xsl:text>
    <xsl:value-of select="$page.height"/>
    <xsl:text>)</xsl:text>
  </xsl:message>

  <xsl:variable name="document.element" select="*[1]"/>
  <xsl:variable name="title">
    <xsl:choose>
      <xsl:when test="$document.element/title[1]">
        <xsl:value-of select="$document.element/title[1]"/>
      </xsl:when>
      <xsl:otherwise>[could not find document title]</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <fo:root font-family="{$body.font.family}"
           font-size="{$body.font.size}"
           text-align="{$alignment}"
           line-height="{$line-height}">
    <xsl:attribute name="language">
      <xsl:call-template name="l10n.language">
        <xsl:with-param name="target" select="/*[1]"/>
      </xsl:call-template>
    </xsl:attribute>

    <xsl:if test="$xep.extensions != 0">
      <xsl:call-template name="xep-document-information"/>
    </xsl:if>
    <xsl:call-template name="setup.pagemasters"/>
    <xsl:choose>
      <xsl:when test="$rootid != ''">
        <xsl:choose>
          <xsl:when test="count(key('id',$rootid)) = 0">
            <xsl:message terminate="yes">
              <xsl:text>ID '</xsl:text>
              <xsl:value-of select="$rootid"/>
              <xsl:text>' not found in document.</xsl:text>
            </xsl:message>
          </xsl:when>
          <xsl:otherwise>
            <xsl:if test="$fop.extensions != 0">
              <xsl:apply-templates select="key('id',$rootid)" mode="fop.outline"/>
            </xsl:if>
            <xsl:if test="$xep.extensions != 0">
              <xsl:variable name="bookmarks">
                <xsl:apply-templates select="key('id',$rootid)" mode="xep.outline"/>
              </xsl:variable>
              <xsl:if test="string($bookmarks) != ''">
                <rx:outline xmlns:rx="http://www.renderx.com/XSL/Extensions">
                  <xsl:copy-of select="$bookmarks"/>
                </rx:outline>
              </xsl:if>
            </xsl:if>
            <xsl:apply-templates select="key('id',$rootid)"/>
          </xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="$fop.extensions != 0">
          <xsl:apply-templates mode="fop.outline"/>
        </xsl:if>
        <xsl:if test="$xep.extensions != 0">
          <xsl:variable name="bookmarks">
            <xsl:apply-templates mode="xep.outline"/>
          </xsl:variable>
          <xsl:if test="string($bookmarks) != ''">
            <rx:outline xmlns:rx="http://www.renderx.com/XSL/Extensions">
              <xsl:copy-of select="$bookmarks"/>
            </rx:outline>
          </xsl:if>
        </xsl:if>
        <xsl:apply-templates/>
      </xsl:otherwise>
    </xsl:choose>

  </fo:root>
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>
