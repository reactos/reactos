<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: toc.xsl,v 1.1 2002/06/13 20:32:26 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<!-- FIXME: in the contexts where <toc> can occur, I think it's always
     the case that a page-sequence is required. Is that true? -->

<xsl:template match="toc">
  <xsl:variable name="master-reference">
    <xsl:call-template name="select.pagemaster"/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="*">
      <xsl:if test="$process.source.toc != 0">
        <!-- if the toc isn't empty, process it -->
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
            <fo:block xsl:use-attribute-sets="toc.margin.properties">
              <xsl:call-template name="table.of.contents.titlepage"/>
              <xsl:apply-templates/>
            </fo:block>
          </fo:flow>
        </fo:page-sequence>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:if test="$process.empty.source.toc != 0">
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
            <xsl:choose>
              <xsl:when test="parent::section
                              or parent::sect1
                              or parent::sect2
                              or parent::sect3
                              or parent::sect4
                              or parent::sect5">
                <xsl:apply-templates select="parent::*"
                                     mode="toc.for.section"/>
              </xsl:when>
              <xsl:when test="parent::article">
                <xsl:apply-templates select="parent::*"
                                     mode="toc.for.component"/>
              </xsl:when>
              <xsl:when test="parent::book
                              or parent::part">
                <xsl:apply-templates select="parent::*"
                                     mode="toc.for.division"/>
              </xsl:when>
              <xsl:when test="parent::set">
                <xsl:apply-templates select="parent::*"
                                     mode="toc.for.set"/>
              </xsl:when>
              <!-- there aren't any other contexts that allow toc -->
              <xsl:otherwise>
                <xsl:message>
                  <xsl:text>I don't know how to make a TOC in this context!</xsl:text>
                </xsl:message>
              </xsl:otherwise>
            </xsl:choose>
          </fo:flow>
        </fo:page-sequence>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="tocpart|tocchap
                     |toclevel1|toclevel2|toclevel3|toclevel4|toclevel5">
  <xsl:apply-templates select="tocentry"/>
  <xsl:if test="tocchap|toclevel1|toclevel2|toclevel3|toclevel4|toclevel5">
    <fo:block start-indent="{count(ancestor::*)*2}pc">
      <xsl:apply-templates select="tocchap|toclevel1|toclevel2|toclevel3|toclevel4|toclevel5"/>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="tocentry|tocfront|tocback">
  <fo:block text-align-last="justify"
            end-indent="2pc"
            last-line-end-indent="-2pc">
    <fo:inline keep-with-next.within-line="always">
      <xsl:choose>
        <xsl:when test="@linkend">
          <fo:basic-link internal-destination="{@linkend}">
            <xsl:apply-templates/>
          </fo:basic-link>
        </xsl:when>
        <xsl:otherwise>
          <xsl:apply-templates/>
        </xsl:otherwise>
      </xsl:choose>
    </fo:inline>

    <xsl:choose>
      <xsl:when test="@linkend">
        <fo:inline keep-together.within-line="always">
          <xsl:text> </xsl:text>
          <fo:leader leader-pattern="dots"
                     keep-with-next.within-line="always"/>
          <xsl:text> </xsl:text>
          <fo:basic-link internal-destination="{@linkend}">
            <xsl:choose>
              <xsl:when test="@pagenum">
                <xsl:value-of select="@pagenum"/>
              </xsl:when>
              <xsl:otherwise>
                <fo:page-number-citation ref-id="{@linkend}"/>
              </xsl:otherwise>
            </xsl:choose>
          </fo:basic-link>
        </fo:inline>
      </xsl:when>
      <xsl:when test="@pagenum">
        <fo:inline keep-together.within-line="always">
          <xsl:text> </xsl:text>
          <fo:leader leader-pattern="dots"
                     keep-with-next.within-line="always"/>
          <xsl:text> </xsl:text>
          <xsl:value-of select="@pagenum"/>
        </fo:inline>
      </xsl:when>
      <xsl:otherwise>
        <!-- just the leaders, what else can I do? -->
        <fo:inline keep-together.within-line="always">
          <xsl:text> </xsl:text>
          <fo:leader leader-pattern="space"
                     keep-with-next.within-line="always"/>
        </fo:inline>
      </xsl:otherwise>
    </xsl:choose>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="toc.for.section">
<!--
  <xsl:call-template name="section.toc"/>
-->
</xsl:template>

<xsl:template match="*" mode="toc.for.component">
  <xsl:call-template name="component.toc"/>
</xsl:template>

<xsl:template match="*" mode="toc.for.section">
<!--
  <xsl:call-template name="section.toc"/>
-->
</xsl:template>

<xsl:template match="*" mode="toc.for.division">
  <xsl:call-template name="division.toc"/>
</xsl:template>

<xsl:template match="*" mode="toc.for.set">
<!--
  <xsl:call-template name="set.toc"/>
-->
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="lot|lotentry">
</xsl:template>

</xsl:stylesheet>
