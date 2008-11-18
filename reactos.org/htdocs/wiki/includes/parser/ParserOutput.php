<?php
/**
 * @todo document
 * @ingroup Parser
 */
class ParserOutput
{
	var $mText,             # The output text
		$mLanguageLinks,    # List of the full text of language links, in the order they appear
		$mCategories,       # Map of category names to sort keys
		$mContainsOldMagic, # Boolean variable indicating if the input contained variables like {{CURRENTDAY}}
		$mCacheTime,        # Time when this object was generated, or -1 for uncacheable. Used in ParserCache.
		$mVersion,          # Compatibility check
		$mTitleText,        # title text of the chosen language variant
		$mLinks,            # 2-D map of NS/DBK to ID for the links in the document. ID=zero for broken.
		$mTemplates,        # 2-D map of NS/DBK to ID for the template references. ID=zero for broken.
		$mTemplateIds,      # 2-D map of NS/DBK to rev ID for the template references. ID=zero for broken.
		$mImages,           # DB keys of the images used, in the array key only
		$mExternalLinks,    # External link URLs, in the key only
		$mNewSection,       # Show a new section link?
		$mNoGallery,        # No gallery on category page? (__NOGALLERY__)
		$mHeadItems,        # Items to put in the <head> section
		$mOutputHooks,      # Hook tags as per $wgParserOutputHooks
		$mWarnings,         # Warning text to be returned to the user. Wikitext formatted, in the key only
		$mSections,         # Table of contents
		$mProperties;       # Name/value pairs to be cached in the DB

	/**
	 * Overridden title for display
	 */
	private $displayTitle = false;

	function ParserOutput( $text = '', $languageLinks = array(), $categoryLinks = array(),
		$containsOldMagic = false, $titletext = '' )
	{
		$this->mText = $text;
		$this->mLanguageLinks = $languageLinks;
		$this->mCategories = $categoryLinks;
		$this->mContainsOldMagic = $containsOldMagic;
		$this->mCacheTime = '';
		$this->mVersion = Parser::VERSION;
		$this->mTitleText = $titletext;
		$this->mSections = array();
		$this->mLinks = array();
		$this->mTemplates = array();
		$this->mImages = array();
		$this->mExternalLinks = array();
		$this->mNewSection = false;
		$this->mNoGallery = false;
		$this->mHeadItems = array();
		$this->mTemplateIds = array();
		$this->mOutputHooks = array();
		$this->mWarnings = array();
		$this->mProperties = array();
	}

	function getText()                   { return $this->mText; }
	function &getLanguageLinks()         { return $this->mLanguageLinks; }
	function getCategoryLinks()          { return array_keys( $this->mCategories ); }
	function &getCategories()            { return $this->mCategories; }
	function getCacheTime()              { return $this->mCacheTime; }
	function getTitleText()              { return $this->mTitleText; }
	function getSections()               { return $this->mSections; }
	function &getLinks()                 { return $this->mLinks; }
	function &getTemplates()             { return $this->mTemplates; }
	function &getImages()                { return $this->mImages; }
	function &getExternalLinks()         { return $this->mExternalLinks; }
	function getNoGallery()              { return $this->mNoGallery; }
	function getSubtitle()               { return $this->mSubtitle; }
	function getOutputHooks()            { return (array)$this->mOutputHooks; }
	function getWarnings()               { return array_keys( $this->mWarnings ); }

	function containsOldMagic()          { return $this->mContainsOldMagic; }
	function setText( $text )            { return wfSetVar( $this->mText, $text ); }
	function setLanguageLinks( $ll )     { return wfSetVar( $this->mLanguageLinks, $ll ); }
	function setCategoryLinks( $cl )     { return wfSetVar( $this->mCategories, $cl ); }
	function setContainsOldMagic( $com ) { return wfSetVar( $this->mContainsOldMagic, $com ); }
	function setCacheTime( $t )          { return wfSetVar( $this->mCacheTime, $t ); }
	function setTitleText( $t )          { return wfSetVar( $this->mTitleText, $t ); }
	function setSections( $toc )         { return wfSetVar( $this->mSections, $toc ); }

	function addCategory( $c, $sort )    { $this->mCategories[$c] = $sort; }
	function addLanguageLink( $t )       { $this->mLanguageLinks[] = $t; }
	function addExternalLink( $url )     { $this->mExternalLinks[$url] = 1; }
	function addWarning( $s )            { $this->mWarnings[$s] = 1; }

	function addOutputHook( $hook, $data = false ) {
		$this->mOutputHooks[] = array( $hook, $data );
	}

	function setNewSection( $value ) {
		$this->mNewSection = (bool)$value;
	}
	function getNewSection() {
		return (bool)$this->mNewSection;
	}

	function addLink( $title, $id = null ) {
		$ns = $title->getNamespace();
		$dbk = $title->getDBkey();
		if ( !isset( $this->mLinks[$ns] ) ) {
			$this->mLinks[$ns] = array();
		}
		if ( is_null( $id ) ) {
			$id = $title->getArticleID();
		}
		$this->mLinks[$ns][$dbk] = $id;
	}

	function addImage( $name ) {
		$this->mImages[$name] = 1;
	}

	function addTemplate( $title, $page_id, $rev_id ) {
		$ns = $title->getNamespace();
		$dbk = $title->getDBkey();
		if ( !isset( $this->mTemplates[$ns] ) ) {
			$this->mTemplates[$ns] = array();
		}
		$this->mTemplates[$ns][$dbk] = $page_id;
		if ( !isset( $this->mTemplateIds[$ns] ) ) {
			$this->mTemplateIds[$ns] = array();
		}
		$this->mTemplateIds[$ns][$dbk] = $rev_id; // For versioning
	}

	/**
	 * Return true if this cached output object predates the global or
	 * per-article cache invalidation timestamps, or if it comes from
	 * an incompatible older version.
	 *
	 * @param string $touched the affected article's last touched timestamp
	 * @return bool
	 * @public
	 */
	function expired( $touched ) {
		global $wgCacheEpoch;
		return $this->getCacheTime() == -1 || // parser says it's uncacheable
		       $this->getCacheTime() < $touched ||
		       $this->getCacheTime() <= $wgCacheEpoch ||
		       !isset( $this->mVersion ) ||
		       version_compare( $this->mVersion, Parser::VERSION, "lt" );
	}

	/**
	 * Add some text to the <head>.
	 * If $tag is set, the section with that tag will only be included once
	 * in a given page.
	 */
	function addHeadItem( $section, $tag = false ) {
		if ( $tag !== false ) {
			$this->mHeadItems[$tag] = $section;
		} else {
			$this->mHeadItems[] = $section;
		}
	}

	/**
	 * Override the title to be used for display
	 * -- this is assumed to have been validated
	 * (check equal normalisation, etc.)
	 *
	 * @param string $text Desired title text
	 */
	public function setDisplayTitle( $text ) {
		$this->displayTitle = $text;
	}

	/**
	 * Get the title to be used for display
	 *
	 * @return string
	 */
	public function getDisplayTitle() {
		return $this->displayTitle;
	}

	/**
	 * Fairly generic flag setter thingy.
	 */
	public function setFlag( $flag ) {
		$this->mFlags[$flag] = true;
	}

	public function getFlag( $flag ) {
		return isset( $this->mFlags[$flag] );
	}

	/**
	 * Set a property to be cached in the DB
	 */
	public function setProperty( $name, $value ) {
		$this->mProperties[$name] = $value;
	}

	public function getProperty( $name ){
		return isset( $this->mProperties[$name] ) ? $this->mProperties[$name] : false;
	}

	public function getProperties() {
		if ( !isset( $this->mProperties ) ) {
			$this->mProperties = array();
		}
		return $this->mProperties;
	}
}
