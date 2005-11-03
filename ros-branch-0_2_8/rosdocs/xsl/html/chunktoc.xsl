<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:doc="http://nwalsh.com/xsl/documentation/1.0"
		version="1.0"
                exclude-result-prefixes="doc">

<xsl:import href="docbook.xsl"/>
<xsl:import href="chunk-common.xsl"/>

<xsl:template name="chunk">
  <xsl:param name="node" select="."/>
  <!-- returns 1 if $node is a chunk -->

  <xsl:variable name="id">
    <xsl:call-template name="object.id">
      <xsl:with-param name="object" select="$node"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="chunks" select="document($chunk.toc,$node)"/>

  <xsl:choose>
    <xsl:when test="$chunks//tocentry[@linkend=$id]">1</xsl:when>
    <xsl:otherwise>0</xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="*" mode="chunk-filename">
  <!-- returns the filename of a chunk -->

  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="chunks" select="document($chunk.toc,.)"/>

  <xsl:variable name="chunk" select="$chunks//tocentry[@linkend=$id]"/>
  <xsl:variable name="filename">
    <xsl:call-template name="dbhtml-filename">
      <xsl:with-param name="pis" select="$chunk/processing-instruction('dbhtml')"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$chunk">
      <xsl:value-of select="$filename"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="parent::*" mode="chunk-filename"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template name="process-chunk">
  <xsl:variable name="id">
    <xsl:call-template name="object.id"/>
  </xsl:variable>

  <xsl:variable name="chunks" select="document($chunk.toc,.)"/>

  <xsl:variable name="chunk" select="$chunks//tocentry[@linkend=$id]"/>
  <xsl:variable name="prev-id"
                select="($chunk/preceding::tocentry
                         |$chunk/ancestor::tocentry)[last()]/@linkend"/>
  <xsl:variable name="next-id"
                select="($chunk/following::tocentry
                         |$chunk/child::tocentry)[1]/@linkend"/>

  <xsl:variable name="prev" select="key('id',$prev-id)"/>
  <xsl:variable name="next" select="key('id',$next-id)"/>

  <xsl:variable name="ischunk">
    <xsl:call-template name="chunk"/>
  </xsl:variable>

  <xsl:variable name="chunkfn">
    <xsl:if test="$ischunk='1'">
      <xsl:apply-templates mode="chunk-filename" select="."/>
    </xsl:if>
  </xsl:variable>

  <xsl:variable name="filename">
    <xsl:call-template name="make-relative-filename">
      <xsl:with-param name="base.dir" select="$base.dir"/>
      <xsl:with-param name="base.name" select="$chunkfn"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$ischunk = 0">
      <xsl:apply-imports/>
    </xsl:when>

    <xsl:otherwise>
      <xsl:call-template name="write.chunk">
        <xsl:with-param name="filename" select="$filename"/>
        <xsl:with-param name="content">
          <xsl:call-template name="chunk-element-content">
            <xsl:with-param name="prev" select="$prev"/>
            <xsl:with-param name="next" select="$next"/>
          </xsl:call-template>
        </xsl:with-param>
        <xsl:with-param name="quiet" select="$chunk.quietly"/>
      </xsl:call-template>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="set">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="book">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="book/appendix">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="book/glossary">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="book/bibliography">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="dedication" mode="dedication">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="preface|chapter">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="part|reference">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="refentry">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="colophon">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="article">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="article/appendix">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="article/glossary">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="article/bibliography">
  <xsl:call-template name="process-chunk"/>
</xsl:template>

<xsl:template match="sect1|sect2|sect3|sect4|sect5|section">
  <xsl:variable name="ischunk">
    <xsl:call-template name="chunk"/>
  </xsl:variable>

  <xsl:choose>
    <xsl:when test="$ischunk != 0">
      <xsl:call-template name="process-chunk"/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-imports/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="setindex
                     |book/index
                     |article/index">
  <!-- some implementations use completely empty index tags to indicate -->
  <!-- where an automatically generated index should be inserted. so -->
  <!-- if the index is completely empty, skip it. -->
  <xsl:if test="count(*)>0 or $generate.index != '0'">
    <xsl:call-template name="process-chunk"/>
  </xsl:if>
</xsl:template>

<!-- ==================================================================== -->

<xsl:template match="/">
  <xsl:choose>
    <xsl:when test="$chunk.toc = ''">
      <xsl:message terminate="yes">
        <xsl:text>The chunk.toc file is not set.</xsl:text>
      </xsl:message>
    </xsl:when>

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
          <xsl:apply-templates select="key('id',$rootid)"/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:when>

    <xsl:otherwise>
      <xsl:apply-templates select="/" mode="process.root"/>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>

<xsl:template match="*" mode="process.root">
  <xsl:apply-templates select="."/>
</xsl:template>

</xsl:stylesheet>
