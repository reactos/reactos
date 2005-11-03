<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
                exclude-result-prefixes="doc"
                version='1.0'>

<!-- ============================================================ -->
<!-- label markup -->

<doc:mode mode="label.markup" xmlns="">
<refpurpose>Provides access to element labels</refpurpose>
<refdescription>
<para>Processing an element in the
<literal role="mode">label.markup</literal> mode produces the
element label.</para>
<para>Trailing punctuation is not added to the label.
</para>
</refdescription>
</doc:mode>

<xsl:template match="*" mode="intralabel.punctuation">
  <xsl:text>.</xsl:text>
</xsl:template>

<xsl:template match="*" mode="label.markup">
  <xsl:message>
    <xsl:text>Request for label of unexpected element: </xsl:text>
    <xsl:value-of select="name(.)"/>
  </xsl:message>
</xsl:template>

<xsl:template match="set|book" mode="label.markup">
  <xsl:if test="@label">
    <xsl:value-of select="@label"/>
  </xsl:if>
</xsl:template>

<xsl:template match="part" mode="label.markup">
  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$part.autolabel != 0">
      <xsl:number from="book" count="part" format="I"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="partintro" mode="label.markup">
  <!-- no label -->
</xsl:template>

<xsl:template match="preface" mode="label.markup">
  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$preface.autolabel != 0">
      <xsl:choose>
        <xsl:when test="$label.from.part != 0 and ancestor::part">
          <xsl:number from="part" count="preface" format="1" level="any"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:number from="book" count="preface" format="1" level="any"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="chapter" mode="label.markup">
  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$chapter.autolabel != 0">
      <xsl:choose>
        <xsl:when test="$label.from.part != 0 and ancestor::part">
          <xsl:number from="part" count="chapter" format="1" level="any"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:number from="book" count="chapter" format="1" level="any"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="appendix" mode="label.markup">
  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$appendix.autolabel != 0">
      <xsl:choose>
        <xsl:when test="$label.from.part != 0 and ancestor::part">
          <xsl:number from="part" count="appendix" format="A" level="any"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:number from="book|article"
                      count="appendix" format="A" level="any"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="article" mode="label.markup">
  <xsl:if test="@label">
    <xsl:value-of select="@label"/>
  </xsl:if>
</xsl:template>

<xsl:template match="dedication|colophon" mode="label.markup">
  <xsl:if test="@label">
    <xsl:value-of select="@label"/>
  </xsl:if>
</xsl:template>

<xsl:template match="reference" mode="label.markup">
  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$part.autolabel != 0">
      <xsl:number from="book" count="reference" format="I" level="any"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="refentry" mode="label.markup">
  <xsl:if test="@label">
    <xsl:value-of select="@label"/>
  </xsl:if>
</xsl:template>

<xsl:template match="section" mode="label.markup">
  <!-- if this is a nested section, label the parent -->
  <xsl:if test="local-name(..) = 'section'">
    <xsl:variable name="parent.section.label">
      <xsl:apply-templates select=".." mode="label.markup"/>
    </xsl:variable>
    <xsl:if test="$parent.section.label != ''">
      <xsl:apply-templates select=".." mode="label.markup"/>
      <xsl:apply-templates select=".." mode="intralabel.punctuation"/>
    </xsl:if>
  </xsl:if>

  <!-- if the parent is a component, maybe label that too -->
  <xsl:variable name="parent.is.component">
    <xsl:call-template name="is.component">
      <xsl:with-param name="node" select=".."/>
    </xsl:call-template>
  </xsl:variable>

  <!-- does this section get labelled? -->
  <xsl:variable name="label">
    <xsl:call-template name="label.this.section">
      <xsl:with-param name="section" select="."/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="$section.label.includes.component.label != 0
                and $parent.is.component != 0">
    <xsl:variable name="parent.label">
      <xsl:apply-templates select=".." mode="label.markup"/>
    </xsl:variable>
    <xsl:if test="$parent.label != ''">
      <xsl:apply-templates select=".." mode="label.markup"/>
      <xsl:apply-templates select=".." mode="intralabel.punctuation"/>
    </xsl:if>
  </xsl:if>

<!--
  <xsl:message>
    <xsl:value-of select="$label"/>, <xsl:number count="section"/>
  </xsl:message>
-->

  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$label != 0">
      <xsl:number count="section"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="sect1" mode="label.markup">
  <!-- if the parent is a component, maybe label that too -->
  <xsl:variable name="parent.is.component">
    <xsl:call-template name="is.component">
      <xsl:with-param name="node" select=".."/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="$section.label.includes.component.label != 0
                and $parent.is.component">
    <xsl:variable name="parent.label">
      <xsl:apply-templates select=".." mode="label.markup"/>
    </xsl:variable>
    <xsl:if test="$parent.label != ''">
      <xsl:apply-templates select=".." mode="label.markup"/>
      <xsl:apply-templates select=".." mode="intralabel.punctuation"/>
    </xsl:if>
  </xsl:if>

  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$section.autolabel != 0">
      <xsl:number count="sect1"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="sect2|sect3|sect4|sect5" mode="label.markup">
  <!-- label the parent -->
  <xsl:variable name="parent.label">
    <xsl:apply-templates select=".." mode="label.markup"/>
  </xsl:variable>
  <xsl:if test="$parent.label != ''">
    <xsl:apply-templates select=".." mode="label.markup"/>
    <xsl:apply-templates select=".." mode="intralabel.punctuation"/>
  </xsl:if>

  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$section.autolabel != 0">
      <xsl:choose>
        <xsl:when test="local-name(.) = 'sect2'">
	  <xsl:number count="sect2"/>
	</xsl:when>
	<xsl:when test="local-name(.) = 'sect3'">
	  <xsl:number count="sect3"/>
	</xsl:when>
	<xsl:when test="local-name(.) = 'sect4'">
	  <xsl:number count="sect4"/>
	</xsl:when>
	<xsl:when test="local-name(.) = 'sect5'">
	  <xsl:number count="sect5"/>
	</xsl:when>
	<xsl:otherwise>
	  <xsl:message>label.markup: this can't happen!</xsl:message>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="refsect1" mode="label.markup">
  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$section.autolabel != 0">
      <xsl:number count="refsect1"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="refsect2|refsect3" mode="label.markup">
  <!-- label the parent -->
  <xsl:variable name="parent.label">
    <xsl:apply-templates select=".." mode="label.markup"/>
  </xsl:variable>
  <xsl:if test="$parent.label != ''">
    <xsl:apply-templates select=".." mode="label.markup"/>
    <xsl:apply-templates select=".." mode="intralabel.punctuation"/>
  </xsl:if>

  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$section.autolabel != 0">
      <xsl:choose>
        <xsl:when test="local-name(.) = 'refsect2'">
	  <xsl:number count="refsect2"/>
	</xsl:when>
        <xsl:otherwise>
	  <xsl:number count="refsect3"/>
	</xsl:otherwise>
      </xsl:choose>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="simplesect" mode="label.markup">
  <!-- if this is a nested section, label the parent -->
  <xsl:if test="local-name(..) = 'section'
                or local-name(..) = 'sect1'
                or local-name(..) = 'sect2'
                or local-name(..) = 'sect3'
                or local-name(..) = 'sect4'
                or local-name(..) = 'sect5'">
    <xsl:variable name="parent.section.label">
      <xsl:apply-templates select=".." mode="label.markup"/>
    </xsl:variable>
    <xsl:if test="$parent.section.label != ''">
      <xsl:apply-templates select=".." mode="label.markup"/>
      <xsl:apply-templates select=".." mode="intralabel.punctuation"/>
    </xsl:if>
  </xsl:if>

  <!-- if the parent is a component, maybe label that too -->
  <xsl:variable name="parent.is.component">
    <xsl:call-template name="is.component">
      <xsl:with-param name="node" select=".."/>
    </xsl:call-template>
  </xsl:variable>

  <!-- does this section get labelled? -->
  <xsl:variable name="label">
    <xsl:call-template name="label.this.section">
      <xsl:with-param name="section" select="."/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:if test="$section.label.includes.component.label != 0
                and $parent.is.component != 0">
    <xsl:variable name="parent.label">
      <xsl:apply-templates select=".." mode="label.markup"/>
    </xsl:variable>
    <xsl:if test="$parent.label != ''">
      <xsl:apply-templates select=".." mode="label.markup"/>
      <xsl:apply-templates select=".." mode="intralabel.punctuation"/>
    </xsl:if>
  </xsl:if>

  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$label != 0">
      <xsl:number count="simplesect"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="qandadiv" mode="label.markup">
  <xsl:variable name="lparent" select="(ancestor::set
                                       |ancestor::book
                                       |ancestor::chapter
                                       |ancestor::appendix
                                       |ancestor::preface
                                       |ancestor::section
                                       |ancestor::simplesect
                                       |ancestor::sect1
                                       |ancestor::sect2
                                       |ancestor::sect3
                                       |ancestor::sect4
                                       |ancestor::sect5
                                       |ancestor::refsect1
                                       |ancestor::refsect2
                                       |ancestor::refsect3)[last()]"/>

  <xsl:variable name="lparent.prefix">
    <xsl:apply-templates select="$lparent" mode="label.markup"/>
  </xsl:variable>

  <xsl:variable name="prefix">
    <xsl:if test="$qanda.inherit.numeration != 0">
      <xsl:if test="$lparent.prefix != ''">
        <xsl:apply-templates select="$lparent" mode="label.markup"/>
        <xsl:apply-templates select="$lparent" mode="intralabel.punctuation"/>
      </xsl:if>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="$prefix"/>
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:when test="$qandadiv.autolabel != 0">
      <xsl:value-of select="$prefix"/>
      <xsl:number level="multiple" count="qandadiv" format="1"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="question|answer" mode="label.markup">
  <xsl:variable name="lparent" select="(ancestor::set
                                       |ancestor::book
                                       |ancestor::chapter
                                       |ancestor::appendix
                                       |ancestor::preface
                                       |ancestor::section
                                       |ancestor::simplesect
                                       |ancestor::sect1
                                       |ancestor::sect2
                                       |ancestor::sect3
                                       |ancestor::sect4
                                       |ancestor::sect5
                                       |ancestor::refsect1
                                       |ancestor::refsect2
                                       |ancestor::refsect3)[last()]"/>

  <xsl:variable name="lparent.prefix">
    <xsl:apply-templates select="$lparent" mode="label.markup"/>
  </xsl:variable>

  <xsl:variable name="prefix">
    <xsl:if test="$qanda.inherit.numeration != 0">
      <xsl:if test="$lparent.prefix != ''">
        <xsl:apply-templates select="$lparent" mode="label.markup"/>
        <xsl:apply-templates select="$lparent" mode="intralabel.punctuation"/>
      </xsl:if>
      <xsl:if test="ancestor::qandadiv">
        <xsl:apply-templates select="ancestor::qandadiv[1]" mode="label.markup"/>
        <xsl:apply-templates select="ancestor::qandadiv[1]"
                             mode="intralabel.punctuation"/>
      </xsl:if>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="inhlabel"
                select="ancestor-or-self::qandaset/@defaultlabel[1]"/>

  <xsl:variable name="deflabel">
    <xsl:choose>
      <xsl:when test="$inhlabel != ''">
        <xsl:value-of select="$inhlabel"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="$qanda.defaultlabel"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:variable name="label" select="label"/>

  <xsl:choose>
    <xsl:when test="count($label)>0">
      <xsl:apply-templates select="$label"/>
    </xsl:when>

    <xsl:when test="$deflabel = 'qanda' and local-name(.) = 'question'">
      <xsl:call-template name="gentext">
        <xsl:with-param name="key" select="'Question'"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:when test="$deflabel = 'qanda' and local-name(.) = 'answer'">
      <xsl:call-template name="gentext">
        <xsl:with-param name="key" select="'Answer'"/>
      </xsl:call-template>
    </xsl:when>

    <xsl:when test="$deflabel = 'number' and local-name(.) = 'question'">
      <xsl:value-of select="$prefix"/>
      <xsl:number level="multiple" count="qandaentry" format="1"/>
    </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="bibliography|glossary|index" mode="label.markup">
  <xsl:if test="@label">
    <xsl:value-of select="@label"/>
  </xsl:if>
</xsl:template>

<xsl:template match="figure|table|example|equation|procedure"
              mode="label.markup">
  <xsl:variable name="pchap"
                select="ancestor::chapter
                        |ancestor::appendix
                        |ancestor::article[ancestor::book]"/>

  <xsl:variable name="prefix">
    <xsl:if test="count($pchap) &gt; 0">
      <xsl:apply-templates select="$pchap" mode="label.markup"/>
    </xsl:if>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="@label">
      <xsl:value-of select="@label"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:choose>
        <xsl:when test="count($pchap)>0">
          <xsl:if test="$prefix != ''">
            <xsl:apply-templates select="$pchap" mode="label.markup"/>
            <xsl:apply-templates select="$pchap" mode="intralabel.punctuation"/>
          </xsl:if>
          <xsl:number format="1" from="chapter|appendix" level="any"/>
        </xsl:when>
        <xsl:otherwise>
          <xsl:number format="1" from="book|article" level="any"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="orderedlist/listitem" mode="label.markup">
  <xsl:variable name="numeration">
    <xsl:call-template name="list.numeration">
      <xsl:with-param name="node" select="parent::orderedlist"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="type">
    <xsl:choose>
      <xsl:when test="$numeration='arabic'">1</xsl:when>
      <xsl:when test="$numeration='loweralpha'">a</xsl:when>
      <xsl:when test="$numeration='lowerroman'">i</xsl:when>
      <xsl:when test="$numeration='upperalpha'">A</xsl:when>
      <xsl:when test="$numeration='upperroman'">I</xsl:when>
      <!-- What!? This should never happen -->
      <xsl:otherwise>
        <xsl:message>
          <xsl:text>Unexpected numeration: </xsl:text>
          <xsl:value-of select="$numeration"/>
        </xsl:message>
        <xsl:value-of select="1."/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:variable>

  <xsl:number count="listitem" format="{$type}"/>
</xsl:template>

<xsl:template match="abstract" mode="label.markup">
  <!-- nop -->
</xsl:template>

<!-- ============================================================ -->

<xsl:template name="label.this.section">
  <xsl:param name="section" select="."/>
  <xsl:value-of select="$section.autolabel"/>
</xsl:template>

<doc:template name="label.this.section" xmlns="">
<refpurpose>Returns true if $section should be labelled</refpurpose>
<refdescription>
<para>Returns true if the specified section should be labelled.
By default, this template simply returns $section.autolabel, but
custom stylesheets may override it to get more selective behavior.</para>
</refdescription>
</doc:template>

<!-- ============================================================ -->

</xsl:stylesheet>
