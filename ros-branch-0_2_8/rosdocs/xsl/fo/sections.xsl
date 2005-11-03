<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: sections.xsl,v 1.1 2002/06/13 20:32:23 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template match="section">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="section.titlepage"/>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="/section">
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
      <xsl:call-template name="section.titlepage"/>
      <xsl:apply-templates/>
   </fo:flow>
  </fo:page-sequence>
</xsl:template>

<xsl:template match="section/title
                     |simplesect/title
                     |sect1/title
                     |sect2/title
                     |sect3/title
                     |sect4/title
                     |sect5/title"
              mode="titlepage.mode"
              priority="2">
  <xsl:variable name="section" select="parent::*"/>
  <fo:block keep-with-next.within-column="always">
    <xsl:variable name="id">
      <xsl:call-template name="object.id">
        <xsl:with-param name="object" select="$section"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="level">
      <xsl:call-template name="section.level">
        <xsl:with-param name="node" select="$section"/>
      </xsl:call-template>
    </xsl:variable>

    <xsl:variable name="title">
      <xsl:apply-templates select="$section" mode="object.title.markup">
        <xsl:with-param name="allow-anchors" select="1"/>
      </xsl:apply-templates>
    </xsl:variable>

    <xsl:if test="$passivetex.extensions != 0">
      <fotex:bookmark xmlns:fotex="http://www.tug.org/fotex" 
                      fotex-bookmark-level="{$level + 1}" 
                      fotex-bookmark-label="{$id}">
        <xsl:value-of select="$title"/>
      </fotex:bookmark>
    </xsl:if>

    <xsl:call-template name="section.heading">
      <xsl:with-param name="level" select="$level"/>
      <xsl:with-param name="title" select="$title"/>
    </xsl:call-template>
  </fo:block>
</xsl:template>

<xsl:template match="sect1">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="sect1.titlepage"/>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="/sect1">
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
      <xsl:call-template name="sect1.titlepage"/>
      <xsl:apply-templates/>
   </fo:flow>
  </fo:page-sequence>
</xsl:template>

<xsl:template match="sect2">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="sect2.titlepage"/>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="sect3">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="sect3.titlepage"/>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="sect4">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="sect4.titlepage"/>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="sect5">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="sect5.titlepage"/>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="simplesect">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="simplesect.titlepage"/>
    <xsl:apply-templates/>
  </fo:block>
</xsl:template>

<xsl:template match="sectioninfo"></xsl:template>
<xsl:template match="section/title"></xsl:template>
<xsl:template match="section/titleabbrev"></xsl:template>
<xsl:template match="section/subtitle"></xsl:template>

<xsl:template match="sect1info"></xsl:template>
<xsl:template match="sect1/title"></xsl:template>
<xsl:template match="sect1/titleabbrev"></xsl:template>
<xsl:template match="sect1/subtitle"></xsl:template>

<xsl:template match="sect2info"></xsl:template>
<xsl:template match="sect2/title"></xsl:template>
<xsl:template match="sect2/titleabbrev"></xsl:template>
<xsl:template match="sect2/subtitle"></xsl:template>

<xsl:template match="sect3info"></xsl:template>
<xsl:template match="sect3/title"></xsl:template>
<xsl:template match="sect3/titleabbrev"></xsl:template>
<xsl:template match="sect3/subtitle"></xsl:template>

<xsl:template match="sect4info"></xsl:template>
<xsl:template match="sect4/title"></xsl:template>
<xsl:template match="sect4/titleabbrev"></xsl:template>
<xsl:template match="sect4/subtitle"></xsl:template>

<xsl:template match="sect5info"></xsl:template>
<xsl:template match="sect5/title"></xsl:template>
<xsl:template match="sect5/titleabbrev"></xsl:template>
<xsl:template match="sect5/subtitle"></xsl:template>

<xsl:template match="simplesect/title"></xsl:template>
<xsl:template match="simplesect/titleabbrev"></xsl:template>
<xsl:template match="simplesect/subtitle"></xsl:template>

<!-- ==================================================================== -->

<xsl:template name="section.heading">
  <xsl:param name="level">1</xsl:param>
  <xsl:param name="title"></xsl:param>
  <xsl:variable name="fsize">
    <xsl:choose>
      <xsl:when test="$level=1">18</xsl:when>
      <xsl:when test="$level=2">16</xsl:when>
      <xsl:when test="$level=3">14</xsl:when>
      <xsl:when test="$level=4">12</xsl:when>
      <xsl:when test="$level=5">12</xsl:when>
      <xsl:otherwise>10</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>
  <fo:block font-size="{$fsize}pt"
            font-weight="bold"
            font-family="{$title.font.family}"
            margin-left="4pc"
            text-indent="-4pc"
            keep-with-next.within-column="always"
            space-before.minimum="1em"
            space-before.optimum="1.5em"
            space-before.maximum="2em">
    <xsl:copy-of select="$title"/>
  </fo:block>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="bridgehead">
  <xsl:variable name="container"
                select="(ancestor::appendix
                        |ancestor::article
                        |ancestor::bibliography
                        |ancestor::chapter
                        |ancestor::glossary
                        |ancestor::glossdiv
                        |ancestor::index
                        |ancestor::partintro
                        |ancestor::preface
                        |ancestor::refsect1
                        |ancestor::refsect2
                        |ancestor::refsect3
                        |ancestor::sect1
                        |ancestor::sect2
                        |ancestor::sect3
                        |ancestor::sect4
                        |ancestor::sect5
                        |ancestor::section
                        |ancestor::setindex
                        |ancestor::simplesect)[last()]"/>

  <xsl:variable name="clevel">
    <xsl:choose>
      <xsl:when test="local-name($container) = 'appendix'
                      or local-name($container) = 'chapter'
                      or local-name($container) = 'article'
                      or local-name($container) = 'bibliography'
                      or local-name($container) = 'glossary'
                      or local-name($container) = 'index'
                      or local-name($container) = 'partintro'
                      or local-name($container) = 'preface'
                      or local-name($container) = 'setindex'">2</xsl:when>
      <xsl:when test="local-name($container) = 'glossdiv'">
        <xsl:value-of select="count(ancestor::glossdiv)+2"/>
      </xsl:when>
      <xsl:when test="local-name($container) = 'sect1'
                      or local-name($container) = 'sect2'
                      or local-name($container) = 'sect3'
                      or local-name($container) = 'sect4'
                      or local-name($container) = 'sect5'
                      or local-name($container) = 'refsect1'
                      or local-name($container) = 'refsect2'
                      or local-name($container) = 'refsect3'
                      or local-name($container) = 'section'
                      or local-name($container) = 'simplesect'">
        <xsl:variable name="slevel">
          <xsl:call-template name="section.level">
            <xsl:with-param name="node" select="$container"/>
          </xsl:call-template>
        </xsl:variable>
        <xsl:value-of select="$slevel + 1"/>
      </xsl:when>
      <xsl:otherwise>2</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="level">
    <xsl:choose>
      <xsl:when test="@renderas = 'sect1'">2</xsl:when>
      <xsl:when test="@renderas = 'sect2'">3</xsl:when>
      <xsl:when test="@renderas = 'sect3'">4</xsl:when>
      <xsl:when test="@renderas = 'sect4'">5</xsl:when>
      <xsl:when test="@renderas = 'sect5'">6</xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$clevel"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <fo:block id="{$id}">
    <xsl:call-template name="section.heading">
      <xsl:with-param name="level" select="$level"/>
      <xsl:with-param name="title">
        <xsl:apply-templates/>
      </xsl:with-param>
    </xsl:call-template>
  </fo:block>
</xsl:template>

</xsl:stylesheet>

