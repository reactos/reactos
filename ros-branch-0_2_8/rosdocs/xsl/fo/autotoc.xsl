<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:fo="http://www.w3.org/1999/XSL/Format"
                version='1.0'>

<!-- ********************************************************************
     $Id: autotoc.xsl,v 1.1 2002/06/13 20:32:18 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<!-- ==================================================================== -->

<xsl:template name="set.toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="nodes" select="book|setindex"/>

  <xsl:if test="$nodes">
    <fo:block id="toc...{$id}"
              xsl:use-attribute-sets="toc.margin.properties">
      <xsl:call-template name="table.of.contents.titlepage"/>
      <xsl:apply-templates select="$nodes" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template name="division.toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="nodes"
                select="part|reference|preface
                        |chapter|appendix
                        |article
                        |bibliography|glossary|index"/>
  <xsl:if test="$nodes">
    <fo:block id="toc...{$id}"
              xsl:use-attribute-sets="toc.margin.properties">
      <xsl:call-template name="table.of.contents.titlepage"/>
      <xsl:apply-templates select="$nodes" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template name="component.toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="nodes" select="section|sect1|refentry
                                     |article|bibliography|glossary
                                     |appendix"/>
  <xsl:if test="$nodes">
    <fo:block id="toc...{$id}"
              xsl:use-attribute-sets="toc.margin.properties">
      <xsl:call-template name="table.of.contents.titlepage"/>
      <xsl:apply-templates select="$nodes" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="toc.line">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="label">
    <xsl:apply-templates select="." mode="label.markup"/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$fop.extensions != 0">
      <fo:block text-align="start">
        <fo:basic-link internal-destination="{$id}">
          <fo:inline keep-with-next.within-line="always">
            <xsl:apply-templates select="." mode="object.title.markup"/>
          </fo:inline>
          <fo:inline keep-together.within-line="always" font-style="italic">
            <xsl:text>&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;&#xA0;</xsl:text>
            <fo:page-number-citation ref-id="{$id}"/>
            <xsl:text></xsl:text>
          </fo:inline>
        </fo:basic-link>
      </fo:block>
    </xsl:when>
    <xsl:otherwise>
      <fo:block text-align-last="justify"
                end-indent="{$toc.indent.width}pt"
                last-line-end-indent="-{$toc.indent.width}pt">
        <fo:inline keep-with-next.within-line="always">
          <fo:basic-link internal-destination="{$id}">
            <xsl:if test="$label != ''">
              <xsl:copy-of select="$label"/>
              <xsl:value-of select="$autotoc.label.separator"/>
            </xsl:if>
            <xsl:apply-templates select="." mode="title.markup"/>
          </fo:basic-link>
        </fo:inline>
        <fo:inline keep-together.within-line="always">
          <xsl:text> </xsl:text>
          <fo:leader leader-pattern="dots"
                     keep-with-next.within-line="always"/>
          <xsl:text> </xsl:text>
          <fo:basic-link internal-destination="{$id}">
            <fo:page-number-citation ref-id="{$id}"/>
          </fo:basic-link>
        </fo:inline>
      </fo:block>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="book|setindex" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="toc.line"/>

  <xsl:variable name="nodes" select="glossary|bibliography|preface|chapter
                                     |reference|part|article|appendix|index"/>

  <xsl:if test="$toc.section.depth &gt; 0 and $nodes">
    <fo:block id="toc.{$cid}.{$id}"
              start-indent="{count(ancestor::*)*$toc.indent.width}pt">
      <xsl:apply-templates select="$nodes" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="part" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="toc.line"/>

  <xsl:variable name="nodes" select="chapter|appendix|preface|reference"/>

  <xsl:if test="$toc.section.depth &gt; 0 and $nodes">
    <fo:block id="toc.{$cid}.{$id}"
              start-indent="{count(ancestor::*)*$toc.indent.width}pt">
      <xsl:apply-templates select="$nodes" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="reference" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="toc.line"/>

  <xsl:if test="$toc.section.depth &gt; 0 and refentry">
    <fo:block id="toc.{$cid}.{$id}"
              start-indent="{count(ancestor::*)*$toc.indent.width}pt">
      <xsl:apply-templates select="refentry" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="refentry" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="toc.line"/>
</xsl:template>

<xsl:template match="preface|chapter|appendix|article"
              mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="toc.line"/>

  <xsl:variable name="nodes" select="section|sect1"/>

  <xsl:if test="$toc.section.depth &gt; 0 and $nodes">
    <fo:block id="toc.{$cid}.{$id}"
              start-indent="{count(ancestor::*)*$toc.indent.width}pt">
      <xsl:apply-templates select="$nodes" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="sect1" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="toc.line"/>

  <xsl:if test="$toc.section.depth &gt; 1 and sect2">
    <fo:block id="toc.{$cid}.{$id}"
              start-indent="{count(ancestor::*)*$toc.indent.width}pt">
      <xsl:apply-templates select="sect2" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="sect2" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="toc.line"/>

  <xsl:if test="$toc.section.depth &gt; 2 and sect3">
    <fo:block id="toc.{$cid}.{$id}"
              start-indent="{count(ancestor::*)*$toc.indent.width}pt">
      <xsl:apply-templates select="sect3" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="sect3" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="toc.line"/>

  <xsl:if test="$toc.section.depth &gt; 3 and sect4">
    <fo:block id="toc.{$cid}.{$id}"
              start-indent="{count(ancestor::*)*$toc.indent.width}pt">
      <xsl:apply-templates select="sect4" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="sect4" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:call-template name="toc.line"/>

  <xsl:if test="$toc.section.depth &gt; 4 and sect5">
    <fo:block id="toc.{$cid}.{$id}"
              start-indent="{count(ancestor::*)*$toc.indent.width}pt">
      <xsl:apply-templates select="sect5" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="sect5" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="toc.line"/>
</xsl:template>

<xsl:template match="section" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="cid">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$toc-context"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="depth" select="count(ancestor::section) + 1"/>

  <xsl:if test="$toc.section.depth &gt;= $depth">
    <xsl:call-template name="toc.line"/>

    <xsl:if test="$toc.section.depth &gt; $depth and section">
      <fo:block id="toc.{$cid}.{$id}"
                start-indent="{count(ancestor::*)*$toc.indent.width}pt">
        <xsl:apply-templates select="section" mode="toc">
          <xsl:with-param name="toc-context" select="$toc-context"/>
        </xsl:apply-templates>
      </fo:block>
    </xsl:if>
  </xsl:if>
</xsl:template>

<xsl:template match="bibliography|glossary"
              mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="toc.line"/>
</xsl:template>

<xsl:template match="index"
              mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:if test="* or $generate.index">
    <xsl:call-template name="toc.line"/>
  </xsl:if>
</xsl:template>

<xsl:template match="title" mode="toc">
  <xsl:apply-templates/>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="list.of.titles">
  <xsl:param name="titles" select="'table'"/>
  <xsl:param name="nodes" select=".//table"/>
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:if test="$nodes">
    <fo:block id="lot...{$titles}...{$id}">
      <xsl:choose>
        <xsl:when test="$titles='table'">
          <xsl:call-template name="list.of.tables.titlepage"/>
        </xsl:when>
        <xsl:when test="$titles='figure'">
          <xsl:call-template name="list.of.figures.titlepage"/>
        </xsl:when>
        <xsl:when test="$titles='equation'">
          <xsl:call-template name="list.of.equations.titlepage"/>
        </xsl:when>
        <xsl:when test="$titles='example'">
          <xsl:call-template name="list.of.examples.titlepage"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:call-template name="list.of.unknowns.titlepage"/>
        </xsl:otherwise>
      </xsl:choose>
      <xsl:apply-templates select="$nodes" mode="toc">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </fo:block>
  </xsl:if>
</xsl:template>

<xsl:template match="figure|table|example|equation" mode="toc">
  <xsl:call-template name="toc.line"/>
</xsl:template>

<!-- ==================================================================== -->

</xsl:stylesheet>

