<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:src="http://nwalsh.com/xmlns/litprog/fragment"
                xmlns:verb="com.nwalsh.saxon.Verbatim"
                exclude-result-prefixes="src verb"
                version="1.0">

<xsl:import href="../../litprog/html/cldocbook.xsl"/>

<xsl:param name="use.id.as.filename" select="1"/>
<xsl:param name="refentry.separator" select="0"/>

<!-- n.b. reference pages are one directory down, so we point back up -->
<xsl:param name="html.stylesheet" select="'../ref.css'"/>

<xsl:template match="src:fragment" mode="label.markup">
  <xsl:text>&#xA7;</xsl:text>
  <xsl:number from="/" level="any" format="1"/>
</xsl:template>

<xsl:template match="src:fragment" mode="xref-to">
  <xsl:variable name="section" select="ancestor::refentry[1]"/>

  <i>
    <xsl:text>&#xA7;</xsl:text>
    <xsl:apply-templates select="$section" mode="label.markup"/>
    <xsl:number from="/" level="any"/>
    <xsl:text>. </xsl:text>
    <xsl:apply-templates select="$section" mode="title.markup"/>
  </i>
</xsl:template>

<xsl:template match="src:fragment" mode="xref-to-section">
  <xsl:variable name="section" select="ancestor::refentry[1]"/>

  <i>
    <xsl:text>&#xA7;</xsl:text>
    <xsl:apply-templates select="$section" mode="label.markup"/>
    <xsl:number from="/" level="any"/>
  </i>
</xsl:template>

<xsl:template match="src:fragment">
  <xsl:param name="suppress-numbers" select="'0'"/>
  <xsl:param name="linenumbering" select="'numbered'"/>

  <xsl:variable name="section" select="ancestor::section[1]"/>
  <xsl:variable name="id" select="@id"/>
  <xsl:variable name="referents"
                select="//src:fragment[.//src:fragref[@linkend=$id]]"/>

  <a name="{@id}"/>
  <table border="1" width="100%">
    <tr>
      <td>
        <p>
          <b>
            <xsl:apply-templates select="." mode="label.markup"/>
          </b>
          <xsl:if test="$referents">
            <xsl:text>: </xsl:text>
            <xsl:for-each select="$referents">
              <xsl:if test="position() &gt; 1">, </xsl:if>
              <a href="#{@id}">
                <xsl:apply-templates select="." mode="label.markup"/>
              </a>
            </xsl:for-each>
          </xsl:if>
        </p>
      </td>
    </tr>
    <tr>
      <td>
        <xsl:choose>
          <xsl:when test="$suppress-numbers = '0'
                          and $linenumbering = 'numbered'
                          and $use.extensions != '0'
                          and $linenumbering.extension != '0'">
            <xsl:variable name="rtf">
              <xsl:apply-templates/>
            </xsl:variable>
            <pre class="{name(.)}">
              <xsl:copy-of select="verb:numberLines($rtf)"/>
            </pre>
          </xsl:when>
          <xsl:otherwise>
            <pre class="{name(.)}">
              <xsl:apply-templates/>
            </pre>
          </xsl:otherwise>
        </xsl:choose>
      </td>
    </tr>
  </table>
</xsl:template>

</xsl:stylesheet>
