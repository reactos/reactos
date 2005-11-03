<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
                exclude-result-prefixes="doc"
                version='1.0'>

<!-- ============================================================ -->
<!-- title markup -->

<doc:mode mode="title.markup" xmlns="">
<refpurpose>Provides access to element titles</refpurpose>
<refdescription>
<para>Processing an element in the
<literal role="mode">title.markup</literal> mode produces the
title of the element. This does not include the label.
</para>
</refdescription>
</doc:mode>

<xsl:template match="*" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title[1]" mode="title.markup">
	<xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:when test="local-name(.) = 'partintro'">
      <!-- partintro's don't have titles, use the parent (part or reference)
           title instead. -->
      <xsl:apply-templates select="parent::*" mode="title.markup"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:message>
	<xsl:text>Request for title of element with no title: </xsl:text>
	<xsl:value-of select="name(.)"/>
        <xsl:if test="@id">
          <xsl:text> (id="</xsl:text>
          <xsl:value-of select="@id"/>
          <xsl:text>")</xsl:text>
        </xsl:if>
      </xsl:message>
      <xsl:text>???TITLE???</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="title" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:choose>
    <xsl:when test="$allow-anchors != 0">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates mode="no.anchor.mode"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="set" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:apply-templates select="(setinfo/title|title)[1]"
                       mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="book" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:apply-templates select="(bookinfo/title|title)[1]"
                       mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="part" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:apply-templates select="(partinfo/title|docinfo/title|title)[1]"
                       mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="preface|chapter|appendix" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>

<!--
  <xsl:message>
    <xsl:value-of select="name(.)"/>
    <xsl:text> </xsl:text>
    <xsl:value-of select="$allow-anchors"/>
  </xsl:message>
-->

  <xsl:variable name="title" select="(docinfo/title
                                      |prefaceinfo/title
                                      |chapterinfo/title
                                      |appendixinfo/title
                                      |title)[1]"/>
  <xsl:apply-templates select="$title" mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="dedication" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title" mode="title.markup">
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext">
        <xsl:with-param name="key" select="'Dedication'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="colophon" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title" mode="title.markup">
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext">
        <xsl:with-param name="key" select="'Colophon'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="article" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:variable name="title" select="(artheader/title
                                      |articleinfo/title
                                      |title)[1]"/>

  <xsl:apply-templates select="$title" mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="reference" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:apply-templates select="(referenceinfo/title|docinfo/title|title)[1]"
                       mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="refentry" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
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
      <xsl:otherwise>REFENTRY WITHOUT TITLE???</xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:copy-of select="$title"/>
</xsl:template>

<xsl:template match="refentrytitle|refname" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:choose>
    <xsl:when test="$allow-anchors != 0">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates mode="no.anchor.mode"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="section
                     |sect1|sect2|sect3|sect4|sect5
                     |refsect1|refsect2|refsect3
                     |simplesect"
              mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:variable name="title" select="(sectioninfo/title
                                      |sect1info/title
                                      |sect2info/title
                                      |sect3info/title
                                      |sect4info/title
                                      |sect5info/title
                                      |refsect1info/title
                                      |refsect2info/title
                                      |refsect3info/title
                                      |title)[1]"/>

  <xsl:apply-templates select="$title" mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="bridgehead" mode="title.markup">
  <xsl:apply-templates mode="title.markup"/>
</xsl:template>

<xsl:template match="bibliography" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:variable name="title" select="(bibliographyinfo/title|title)[1]"/>
  <xsl:choose>
    <xsl:when test="$title">
      <xsl:apply-templates select="$title" mode="title.markup">
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext">
        <xsl:with-param name="key" select="'Bibliography'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="glossary" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:variable name="title" select="(glossaryinfo/title|title)[1]"/>
  <xsl:choose>
    <xsl:when test="$title">
      <xsl:apply-templates select="$title" mode="title.markup">
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext.element.name">
        <xsl:with-param name="element.name" select="name(.)"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="index" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:variable name="title" select="(indexinfo/title|title)[1]"/>
  <xsl:choose>
    <xsl:when test="$title">
      <xsl:apply-templates select="$title" mode="title.markup">
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext">
        <xsl:with-param name="key" select="'Index'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="figure|table|example|equation" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:apply-templates select="title" mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="procedure" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:apply-templates select="title" mode="title.markup">
    <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
  </xsl:apply-templates>
</xsl:template>

<xsl:template match="abstract" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title" mode="title.markup">
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext">
        <xsl:with-param name="key" select="'Abstract'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="caution|tip|warning|important|note" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:variable name="title" select="title[1]"/>
  <xsl:choose>
    <xsl:when test="$title">
      <xsl:apply-templates select="$title" mode="title.markup">
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext">
        <xsl:with-param name="key">
          <xsl:choose>
            <xsl:when test="local-name(.)='note'">Note</xsl:when>
            <xsl:when test="local-name(.)='important'">Important</xsl:when>
            <xsl:when test="local-name(.)='caution'">Caution</xsl:when>
            <xsl:when test="local-name(.)='warning'">Warning</xsl:when>
            <xsl:when test="local-name(.)='tip'">Tip</xsl:when>
          </xsl:choose>
        </xsl:with-param>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="question" mode="title.markup">
  <!-- questions don't have titles -->
  <xsl:text>Question</xsl:text>
</xsl:template>

<xsl:template match="answer" mode="title.markup">
  <!-- answers don't have titles -->
  <xsl:text>Answer</xsl:text>
</xsl:template>

<xsl:template match="qandaentry" mode="title.markup">
  <!-- qandaentrys are represented by the first question in them -->
  <xsl:text>Question</xsl:text>
</xsl:template>

<xsl:template match="legalnotice" mode="title.markup">
  <xsl:param name="allow-anchors" select="0"/>
  <xsl:choose>
    <xsl:when test="title">
      <xsl:apply-templates select="title" mode="title.markup">
        <xsl:with-param name="allow-anchors" select="$allow-anchors"/>
      </xsl:apply-templates>
    </xsl:when>
    <xsl:otherwise>
      <xsl:call-template name="gentext">
        <xsl:with-param name="key" select="'LegalNotice'"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ============================================================ -->

<xsl:template match="*" mode="no.anchor.mode">
  <xsl:apply-templates mode="no.anchor.mode"/>
</xsl:template>

<xsl:template match="footnote" mode="no.anchor.mode">
  <!-- nop, suppressed -->
</xsl:template>

<xsl:template match="anchor" mode="no.anchor.mode">
  <!-- nop, suppressed -->
</xsl:template>

<xsl:template match="ulink" mode="no.anchor.mode">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="link" mode="no.anchor.mode">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="olink" mode="no.anchor.mode">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="indexterm" mode="no.anchor.mode">
  <!-- nop, suppressed -->
</xsl:template>

<xsl:template match="xref" mode="no.anchor.mode">
  <!-- FIXME: this should generate the text without the link... -->
</xsl:template>

<!-- ============================================================ -->

</xsl:stylesheet>

