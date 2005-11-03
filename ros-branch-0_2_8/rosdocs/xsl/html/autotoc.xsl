<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                version='1.0'>

<!-- ********************************************************************
     $Id: autotoc.xsl,v 1.1 2002/06/13 20:32:33 chorns Exp $
     ********************************************************************

     This file is part of the XSL DocBook Stylesheet distribution.
     See ../README or http://nwalsh.com/docbook/xsl/ for copyright
     and other information.

     ******************************************************************** -->

<xsl:variable name="toc.listitem.type">
  <xsl:choose>
    <xsl:when test="$toc.list.type = 'dl'">dt</xsl:when>
    <xsl:otherwise>li</xsl:otherwise>
  </xsl:choose>
</xsl:variable>

<!-- this is just hack because dl and ul aren't completely isomorphic -->
<xsl:variable name="toc.dd.type">
  <xsl:choose>
    <xsl:when test="$toc.list.type = 'dl'">dd</xsl:when>
    <xsl:otherwise></xsl:otherwise>
  </xsl:choose>
</xsl:variable>

<xsl:template name="set.toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="toc.title">
    <p>
      <b>
        <xsl:call-template name="gentext">
          <xsl:with-param name="key">TableofContents</xsl:with-param>
        </xsl:call-template>
      </b>
    </p>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$manual.toc != ''">
      <xsl:variable name="id">
        <xsl:call-template name="object.id"/>
      </xsl:variable>
      <xsl:variable name="toc" select="document($manual.toc, .)"/>
      <xsl:variable name="tocentry" select="$toc//tocentry[@linkend=$id]"/>
      <xsl:if test="$tocentry and $tocentry/*">
        <div class="toc">
          <xsl:copy-of select="$toc.title"/>
          <xsl:element name="{$toc.list.type}">
            <xsl:call-template name="manual-toc">
              <xsl:with-param name="tocentry" select="$tocentry/*[1]"/>
            </xsl:call-template>
          </xsl:element>
        </div>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="nodes" select="book|setindex"/>

      <xsl:if test="$nodes">
        <div class="toc">
          <xsl:copy-of select="$toc.title"/>
          <xsl:element name="{$toc.list.type}">
            <xsl:apply-templates select="$nodes" mode="toc">
              <xsl:with-param name="toc-context" select="$toc-context"/>
            </xsl:apply-templates>
          </xsl:element>
        </div>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="division.toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="toc.title">
    <p>
      <b>
        <xsl:call-template name="gentext">
          <xsl:with-param name="key">TableofContents</xsl:with-param>
        </xsl:call-template>
      </b>
    </p>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$manual.toc != ''">
      <xsl:variable name="id">
        <xsl:call-template name="object.id"/>
      </xsl:variable>
      <xsl:variable name="toc" select="document($manual.toc, .)"/>
      <xsl:variable name="tocentry" select="$toc//tocentry[@linkend=$id]"/>
      <xsl:if test="$tocentry and $tocentry/*">
        <div class="toc">
          <xsl:copy-of select="$toc.title"/>
          <xsl:element name="{$toc.list.type}">
            <xsl:call-template name="manual-toc">
              <xsl:with-param name="tocentry" select="$tocentry/*[1]"/>
            </xsl:call-template>
          </xsl:element>
        </div>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="nodes" select="part|reference
                                         |preface|chapter|appendix
                                         |article
                                         |bibliography|glossary|index
                                         |refentry
                                         |bridgehead"/>
      <xsl:if test="$nodes">
        <div class="toc">
          <xsl:copy-of select="$toc.title"/>
          <xsl:element name="{$toc.list.type}">
            <xsl:apply-templates select="$nodes" mode="toc">
              <xsl:with-param name="toc-context" select="$toc-context"/>
            </xsl:apply-templates>
          </xsl:element>
        </div>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="component.toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="toc.title">
    <p>
      <b>
        <xsl:call-template name="gentext">
          <xsl:with-param name="key">TableofContents</xsl:with-param>
        </xsl:call-template>
      </b>
    </p>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$manual.toc != ''">
      <xsl:variable name="id">
        <xsl:call-template name="object.id"/>
      </xsl:variable>
      <xsl:variable name="toc" select="document($manual.toc, .)"/>
      <xsl:variable name="tocentry" select="$toc//tocentry[@linkend=$id]"/>
      <xsl:if test="$tocentry and $tocentry/*">
        <div class="toc">
          <xsl:copy-of select="$toc.title"/>
          <xsl:element name="{$toc.list.type}">
            <xsl:call-template name="manual-toc">
              <xsl:with-param name="tocentry" select="$tocentry/*[1]"/>
            </xsl:call-template>
          </xsl:element>
        </div>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="nodes" select="section|sect1|refentry
                                         |article|bibliography|glossary
                                         |appendix|bridgehead[not(@renderas)]
                                         |.//bridgehead[@renderas='sect1']"/>
      <xsl:if test="$nodes">
        <div class="toc">
          <xsl:copy-of select="$toc.title"/>
          <xsl:element name="{$toc.list.type}">
            <xsl:apply-templates select="$nodes" mode="toc">
              <xsl:with-param name="toc-context" select="$toc-context"/>
            </xsl:apply-templates>
          </xsl:element>
        </div>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template name="section.toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="toc.title">
    <p>
      <b>
        <xsl:call-template name="gentext">
          <xsl:with-param name="key">TableofContents</xsl:with-param>
        </xsl:call-template>
      </b>
    </p>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$manual.toc != ''">
      <xsl:variable name="id">
        <xsl:call-template name="object.id"/>
      </xsl:variable>
      <xsl:variable name="toc" select="document($manual.toc, .)"/>
      <xsl:variable name="tocentry" select="$toc//tocentry[@linkend=$id]"/>
      <xsl:if test="$tocentry and $tocentry/*">
        <div class="toc">
          <xsl:copy-of select="$toc.title"/>
          <xsl:element name="{$toc.list.type}">
            <xsl:call-template name="manual-toc">
              <xsl:with-param name="tocentry" select="$tocentry/*[1]"/>
            </xsl:call-template>
          </xsl:element>
        </div>
      </xsl:if>
    </xsl:when>
    <xsl:otherwise>
      <xsl:variable name="nodes"
                    select="section|sect1|sect2|sect3|sect4|sect5|refentry
                            |bridgehead"/>
      <xsl:if test="$nodes">
        <div class="toc">
          <xsl:copy-of select="$toc.title"/>
          <xsl:element name="{$toc.list.type}">
            <xsl:apply-templates select="$nodes" mode="toc">
              <xsl:with-param name="toc-context" select="$toc-context"/>
            </xsl:apply-templates>
          </xsl:element>
        </div>
      </xsl:if>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="subtoc">
  <xsl:param name="toc-context" select="."/>
  <xsl:param name="nodes" select="NOT-AN-ELEMENT"/>

  <xsl:variable name="subtoc">
    <xsl:element name="{$toc.list.type}">
      <xsl:apply-templates mode="toc" select="$nodes">
        <xsl:with-param name="toc-context" select="$toc-context"/>
      </xsl:apply-templates>
    </xsl:element>
  </xsl:variable>

  <xsl:variable name="depth">
    <xsl:choose>
      <xsl:when test="local-name(.) = 'section'">
        <xsl:value-of select="count(ancestor::section) + 1"/>
      </xsl:when>
      <xsl:when test="local-name(.) = 'sect1'">1</xsl:when>
      <xsl:when test="local-name(.) = 'sect2'">2</xsl:when>
      <xsl:when test="local-name(.) = 'sect3'">3</xsl:when>
      <xsl:when test="local-name(.) = 'sect4'">4</xsl:when>
      <xsl:when test="local-name(.) = 'sect5'">5</xsl:when>
      <xsl:when test="local-name(.) = 'refsect1'">1</xsl:when>
      <xsl:when test="local-name(.) = 'refsect2'">2</xsl:when>
      <xsl:when test="local-name(.) = 'refsect3'">3</xsl:when>
      <xsl:when test="local-name(.) = 'simplesect'">
        <!-- sigh... -->
        <xsl:choose>
          <xsl:when test="local-name(..) = 'section'">
            <xsl:value-of select="count(ancestor::section)"/>
          </xsl:when>
          <xsl:when test="local-name(..) = 'sect1'">2</xsl:when>
          <xsl:when test="local-name(..) = 'sect2'">3</xsl:when>
          <xsl:when test="local-name(..) = 'sect3'">4</xsl:when>
          <xsl:when test="local-name(..) = 'sect4'">5</xsl:when>
          <xsl:when test="local-name(..) = 'sect5'">6</xsl:when>
          <xsl:when test="local-name(..) = 'refsect1'">2</xsl:when>
          <xsl:when test="local-name(..) = 'refsect2'">3</xsl:when>
          <xsl:when test="local-name(..) = 'refsect3'">4</xsl:when>
          <xsl:otherwise>1</xsl:otherwise>
        </xsl:choose>
      </xsl:when>
      <xsl:otherwise>0</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="subtoc.list">
    <xsl:choose>
      <xsl:when test="$toc.dd.type = ''">
        <xsl:copy-of select="$subtoc"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:element name="{$toc.dd.type}">
          <xsl:copy-of select="$subtoc"/>
        </xsl:element>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:element name="{$toc.listitem.type}">
    <xsl:variable name="label">
      <xsl:apply-templates select="." mode="label.markup"/>
    </xsl:variable>
    <xsl:copy-of select="$label"/>
    <xsl:if test="$label != ''">
      <xsl:value-of select="$autotoc.label.separator"/>
    </xsl:if>

    <a>
      <xsl:attribute name="href">
        <xsl:call-template name="href.target">
          <xsl:with-param name="context" select="$toc-context"/>
        </xsl:call-template>
      </xsl:attribute>
      <xsl:apply-templates select="." mode="title.markup"/>
    </a>
    <xsl:if test="$toc.listitem.type = 'li'
                  and $toc.section.depth > $depth and count($nodes)&gt;0">
      <xsl:copy-of select="$subtoc.list"/>
    </xsl:if>
  </xsl:element>
  <xsl:if test="$toc.listitem.type != 'li'
                and $toc.section.depth > $depth and count($nodes)&gt;0">
    <xsl:copy-of select="$subtoc.list"/>
  </xsl:if>
</xsl:template>

<xsl:template match="book|setindex" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="part|reference
                                         |preface|chapter|appendix
                                         |article
                                         |bibliography|glossary|index
                                         |refentry
                                         |bridgehead"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="part|reference" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="appendix|chapter|article
                                         |index|glossary|bibliography
                                         |preface|reference|refentry
                                         |bridgehead"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="preface|chapter|appendix|article" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="section|sect1|bridgehead"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="sect1" mode="toc">
  <xsl:param name="toc-context" select="."/>
  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="sect2|bridgehead"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="sect2" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="sect3|bridgehead"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="sect3" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="sect4|bridgehead"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="sect4" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="sect5|bridgehead"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="sect5" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="section" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
    <xsl:with-param name="nodes" select="section|bridgehead"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="bridgehead" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:if test="$bridgehead.in.toc != 0">
    <xsl:call-template name="subtoc">
      <xsl:with-param name="toc-context" select="$toc-context"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template match="bibliography|glossary" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:call-template name="subtoc">
    <xsl:with-param name="toc-context" select="$toc-context"/>
  </xsl:call-template>
</xsl:template>

<xsl:template match="index" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <!-- If the index tag is empty, don't point at it from the TOC -->
  <xsl:if test="* or $generate.index">
    <xsl:call-template name="subtoc">
      <xsl:with-param name="toc-context" select="$toc-context"/>
    </xsl:call-template>
  </xsl:if>
</xsl:template>

<xsl:template match="refentry" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:variable name="refmeta" select=".//refmeta"/>
  <xsl:variable name="refentrytitle" select="$refmeta//refentrytitle"/>
  <xsl:variable name="refnamediv" select=".//refnamediv"/>
  <xsl:variable name="refname" select="$refnamediv//refname"/>
  <xsl:variable name="title">
    <xsl:choose>
      <xsl:when test="$refentrytitle">
        <xsl:apply-templates select="$refentrytitle[1]" mode="title.markup"/>
      </xsl:when>
      <xsl:when test="$refname">
        <xsl:apply-templates select="$refname[1]" mode="title.markup"/>
      </xsl:when>
      <xsl:otherwise></xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:element name="{$toc.listitem.type}">
    <a>
      <xsl:attribute name="href">
        <xsl:call-template name="href.target"/>
      </xsl:attribute>
      <xsl:copy-of select="$title"/>
    </a>
    <xsl:if test="$annotate.toc != 0">
      <xsl:text> - </xsl:text>
      <xsl:value-of select="refnamediv/refpurpose"/>
    </xsl:if>
  </xsl:element>
</xsl:template>

<xsl:template match="title" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <a>
    <xsl:attribute name="href">
      <xsl:call-template name="href.target">
        <xsl:with-param name="object" select=".."/>
      </xsl:call-template>
    </xsl:attribute>
    <xsl:apply-templates/>
  </a>
</xsl:template>

<xsl:template name="manual-toc">
  <xsl:param name="toc-context" select="."/>
  <xsl:param name="tocentry"/>

  <!-- be careful, we don't want to change the current document to the other tree! -->

  <xsl:if test="$tocentry">
    <xsl:variable name="node" select="key('id', $tocentry/@linkend)"/>

    <xsl:element name="{$toc.listitem.type}">
      <xsl:variable name="label">
        <xsl:apply-templates select="$node" mode="label.markup"/>
      </xsl:variable>
      <xsl:copy-of select="$label"/>
      <xsl:if test="$label != ''">
        <xsl:value-of select="$autotoc.label.separator"/>
      </xsl:if>
      <a>
        <xsl:attribute name="href">
          <xsl:call-template name="href.target">
            <xsl:with-param name="object" select="$node"/>
          </xsl:call-template>
        </xsl:attribute>
        <xsl:apply-templates select="$node" mode="title.markup"/>
      </a>
    </xsl:element>

    <xsl:if test="$tocentry/*">
      <xsl:element name="{$toc.list.type}">
        <xsl:call-template name="manual-toc">
          <xsl:with-param name="tocentry" select="$tocentry/*[1]"/>
        </xsl:call-template>
      </xsl:element>
    </xsl:if>

    <xsl:if test="$tocentry/following-sibling::*">
      <xsl:call-template name="manual-toc">
        <xsl:with-param name="tocentry" select="$tocentry/following-sibling::*[1]"/>
      </xsl:call-template>
    </xsl:if>
  </xsl:if>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="list.of.titles">
  <xsl:param name="toc-context" select="."/>
  <xsl:param name="titles" select="'table'"/>
  <xsl:param name="nodes" select=".//table"/>

  <xsl:if test="$nodes">
    <div class="list-of-{$titles}s">
      <p>
        <b>
          <xsl:call-template name="gentext">
            <xsl:with-param name="key">
              <xsl:choose>
                <xsl:when test="$titles='table'">ListofTables</xsl:when>
                <xsl:when test="$titles='figure'">ListofFigures</xsl:when>
                <xsl:when test="$titles='equation'">ListofEquations</xsl:when>
                <xsl:when test="$titles='example'">ListofExamples</xsl:when>
                <xsl:otherwise>ListofUnknowns</xsl:otherwise>
              </xsl:choose>
            </xsl:with-param>
          </xsl:call-template>
        </b>
      </p>

      <xsl:element name="{$toc.list.type}">
        <xsl:apply-templates select="$nodes" mode="toc">
          <xsl:with-param name="toc-context" select="$toc-context"/>
        </xsl:apply-templates>
      </xsl:element>
    </div>
  </xsl:if>
</xsl:template>

<xsl:template match="figure|table|example|equation" mode="toc">
  <xsl:param name="toc-context" select="."/>

  <xsl:element name="{$toc.listitem.type}">
    <xsl:variable name="label">
      <xsl:apply-templates select="." mode="label.markup"/>
    </xsl:variable>
    <xsl:copy-of select="$label"/>
    <xsl:if test="$label != ''">
      <xsl:value-of select="$autotoc.label.separator"/>
    </xsl:if>
    <a>
      <xsl:attribute name="href">
        <xsl:call-template name="href.target"/>
      </xsl:attribute>
      <xsl:apply-templates select="." mode="title.markup"/>
    </a>
  </xsl:element>
</xsl:template>

</xsl:stylesheet>

