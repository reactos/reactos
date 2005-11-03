<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: refentry.xsl,v 1.1 2002/06/13 20:32:23 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="reference">
   <xsl:if test="not(partintro)">
    <xsl:variable name="id">
      <xsl:call-template name="object.id"/>
    </xsl:variable>
    <xsl:variable name="master-reference">
      <xsl:call-template name="select.pagemaster"/>
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
        <xsl:call-template name="reference.titlepage"/>
      </fo:flow>
    </fo:page-sequence>
  </xsl:if>
  <xsl:apply-templates select="partintro|refentry"/>
</xsl:template>

<xsl:template match="reference" mode="reference.titlepage.mode">
  <xsl:call-template name="reference.titlepage"/>
</xsl:template>

<xsl:template match="reference/partintro">
  <xsl:variable name="id">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="ancestor::reference"/>
    </xsl:call-template>
  </xsl:variable>
  <xsl:variable name="master-reference">
    <xsl:call-template name="select.pagemaster"/>
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
      <xsl:apply-templates select=".." mode="reference.titlepage.mode"/>
      <xsl:if test="title">
        <xsl:call-template name="partintro.titlepage"/>
      </xsl:if>
      <xsl:apply-templates/>
    </fo:flow>
  </fo:page-sequence>
</xsl:template>

<xsl:template match="reference/docinfo"></xsl:template>
<xsl:template match="reference/title"></xsl:template>
<xsl:template match="reference/subtitle"></xsl:template>

<!-- ==================================================================== -->

<xsl:template match="refentry">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="master-reference">
    <xsl:call-template name="select.pagemaster"/>
  </xsl:variable>

  <xsl:variable name="refentry.content">
<!-- FIXME: what should this be?
    <fo:block font-size="20pt" font-weight="bold">
      <xsl:choose>
        <xsl:when test="refmeta/refentrytitle">
          <xsl:apply-templates select="refmeta/refentrytitle" mode="title"/>
        </xsl:when>
        <xsl:when test="refnamediv/refname">
          <xsl:apply-templates select="refnamediv/refname" mode="title"/>
        </xsl:when>
      </xsl:choose>
    </fo:block>
-->
    <fo:block id="{$id}">
      <xsl:apply-templates/>
    </fo:block>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="not(parent::*) or parent::reference">
      <!-- make a page sequence -->
      <fo:page-sequence hyphenate="{$hyphenate}"
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
          <xsl:copy-of select="$refentry.content"/>
        </fo:flow>
      </fo:page-sequence>
    </xsl:when>
    <xsl:otherwise>
      <fo:block break-before="page">
        <xsl:copy-of select="$refentry.content"/>
      </fo:block>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="refmeta">
</xsl:template>

<xsl:template match="manvolnum">
  <xsl:if test="$refentry.xref.manvolnum != 0">
    <xsl:text>(</xsl:text>
    <xsl:apply-templates/>
    <xsl:text>)</xsl:text>
  </xsl:if>
</xsl:template>

<xsl:template match="refmiscinfo">
</xsl:template>

<xsl:template match="refentrytitle">
  <xsl:call-template name="inline.charseq"/>
</xsl:template>

<xsl:template match="refnamediv">
  <fo:block>
    <xsl:choose>
      <xsl:when test="$refentry.generate.name != 0">
        <fo:block font-size="18pt" font-weight="bold">
          <xsl:call-template name="gentext">
            <xsl:with-param name="key" select="'RefName'"/>
          </xsl:call-template>
        </fo:block>
      </xsl:when>
      <xsl:when test="$refentry.generate.title != 0">
        <fo:block font-size="18pt" font-weight="bold">
          <xsl:choose>
            <xsl:when test="../refmeta/refentrytitle">
              <xsl:apply-templates select="../refmeta/refentrytitle"/>
            </xsl:when>
            <xsl:otherwise>
              <xsl:apply-templates select="refname[1]"/>
            </xsl:otherwise>
          </xsl:choose>
        </fo:block>
      </xsl:when>
    </xsl:choose>

    <fo:block>
      <xsl:choose>
        <xsl:when test="../refmeta/refentrytitle">
          <xsl:apply-templates select="../refmeta/refentrytitle"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates select="refname[1]"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="refpurpose"/>
    </fo:block>

    <fo:block>
      <xsl:for-each select="refname">
        <xsl:apply-templates select="."/>
        <xsl:if test="following-sibling::refname">
          <xsl:text>, </xsl:text>
        </xsl:if>
      </xsl:for-each>
    </fo:block>
  </fo:block>
</xsl:template>

<xsl:template match="refname">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="refpurpose">
  <xsl:text> </xsl:text>
  <xsl:call-template name="dingbat">
    <xsl:with-param name="dingbat">em-dash</xsl:with-param>
  </xsl:call-template>
  <xsl:text> </xsl:text>
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="refdescriptor">
  <!-- todo: finish this -->
</xsl:template>

<xsl:template match="refclass">
  <fo:block font-weight="bold">
    <xsl:if test="@role">
      <xsl:value-of select="@role"/>
      <xsl:text>: </xsl:text>
    </xsl:if>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="refsynopsisdiv">
  <fo:block>
    <xsl:attribute name="id">
      <xsl:call-template name="object.id"/>
    </xsl:attribute>
    <fo:block font-size="18pt" font-weight="bold">
      <xsl:choose>
        <xsl:when test="refsynopsisdiv/title|title">
          <xsl:apply-templates select="(refsynopsisdiv/title|title)[1]"
                               mode="titlepage.mode"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="gentext">
            <xsl:with-param name="key" select="'RefSynopsisDiv'"/>
          </xsl:call-template>
        </xsl:otherwise>
      </xsl:choose>
    </fo:block>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="refsynopsisdiv/title">
</xsl:template>

<xsl:template match="refsect1|refsect2|refsect3">
  <xsl:call-template name="block.object"/>
</xsl:template>

<xsl:template match="refsect1/title">
  <fo:block font-size="18pt" font-weight="bold">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="refsect2/title">
  <fo:block font-size="16pt" font-weight="bold">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="refsect3/title">
  <fo:block font-size="14pt" font-weight="bold">
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>
