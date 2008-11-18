<?php

/**
 * Set options of the Parser
 * @todo document
 * @ingroup Parser
 */
class ParserOptions
{
	# All variables are supposed to be private in theory, although in practise this is not the case.
	var $mUseTeX;                    # Use texvc to expand <math> tags
	var $mUseDynamicDates;           # Use DateFormatter to format dates
	var $mInterwikiMagic;            # Interlanguage links are removed and returned in an array
	var $mAllowExternalImages;       # Allow external images inline
	var $mAllowExternalImagesFrom;   # If not, any exception?
	var $mSkin;                      # Reference to the preferred skin
	var $mDateFormat;                # Date format index
	var $mEditSection;               # Create "edit section" links
	var $mNumberHeadings;            # Automatically number headings
	var $mAllowSpecialInclusion;     # Allow inclusion of special pages
	var $mTidy;                      # Ask for tidy cleanup
	var $mInterfaceMessage;          # Which lang to call for PLURAL and GRAMMAR
	var $mTargetLanguage;            # Overrides above setting with arbitrary language
	var $mMaxIncludeSize;            # Maximum size of template expansions, in bytes
	var $mMaxPPNodeCount;            # Maximum number of nodes touched by PPFrame::expand()
	var $mMaxPPExpandDepth;          # Maximum recursion depth in PPFrame::expand()
	var $mMaxTemplateDepth;          # Maximum recursion depth for templates within templates
	var $mRemoveComments;            # Remove HTML comments. ONLY APPLIES TO PREPROCESS OPERATIONS
	var $mTemplateCallback;          # Callback for template fetching
	var $mEnableLimitReport;         # Enable limit report in an HTML comment on output
	var $mTimestamp;                 # Timestamp used for {{CURRENTDAY}} etc.

	var $mUser;                      # Stored user object, just used to initialise the skin

	function getUseTeX()                        { return $this->mUseTeX; }
	function getUseDynamicDates()               { return $this->mUseDynamicDates; }
	function getInterwikiMagic()                { return $this->mInterwikiMagic; }
	function getAllowExternalImages()           { return $this->mAllowExternalImages; }
	function getAllowExternalImagesFrom()       { return $this->mAllowExternalImagesFrom; }
	function getEditSection()                   { return $this->mEditSection; }
	function getNumberHeadings()                { return $this->mNumberHeadings; }
	function getAllowSpecialInclusion()         { return $this->mAllowSpecialInclusion; }
	function getTidy()                          { return $this->mTidy; }
	function getInterfaceMessage()              { return $this->mInterfaceMessage; }
	function getTargetLanguage()                { return $this->mTargetLanguage; }
	function getMaxIncludeSize()                { return $this->mMaxIncludeSize; }
	function getMaxPPNodeCount()                { return $this->mMaxPPNodeCount; }
	function getMaxTemplateDepth()              { return $this->mMaxTemplateDepth; }
	function getRemoveComments()                { return $this->mRemoveComments; }
	function getTemplateCallback()              { return $this->mTemplateCallback; }
	function getEnableLimitReport()             { return $this->mEnableLimitReport; }

	function getSkin() {
		if ( !isset( $this->mSkin ) ) {
			$this->mSkin = $this->mUser->getSkin();
		}
		return $this->mSkin;
	}

	function getDateFormat() {
		if ( !isset( $this->mDateFormat ) ) {
			$this->mDateFormat = $this->mUser->getDatePreference();
		}
		return $this->mDateFormat;
	}

	function getTimestamp() {
		if ( !isset( $this->mTimestamp ) ) {
			$this->mTimestamp = wfTimestampNow();
		}
		return $this->mTimestamp;
	}

	function setUseTeX( $x )                    { return wfSetVar( $this->mUseTeX, $x ); }
	function setUseDynamicDates( $x )           { return wfSetVar( $this->mUseDynamicDates, $x ); }
	function setInterwikiMagic( $x )            { return wfSetVar( $this->mInterwikiMagic, $x ); }
	function setAllowExternalImages( $x )       { return wfSetVar( $this->mAllowExternalImages, $x ); }
	function setAllowExternalImagesFrom( $x )   { return wfSetVar( $this->mAllowExternalImagesFrom, $x ); }
	function setDateFormat( $x )                { return wfSetVar( $this->mDateFormat, $x ); }
	function setEditSection( $x )               { return wfSetVar( $this->mEditSection, $x ); }
	function setNumberHeadings( $x )            { return wfSetVar( $this->mNumberHeadings, $x ); }
	function setAllowSpecialInclusion( $x )     { return wfSetVar( $this->mAllowSpecialInclusion, $x ); }
	function setTidy( $x )                      { return wfSetVar( $this->mTidy, $x); }
	function setSkin( $x )                      { $this->mSkin = $x; }
	function setInterfaceMessage( $x )          { return wfSetVar( $this->mInterfaceMessage, $x); }
	function setTargetLanguage( $x )            { return wfSetVar( $this->mTargetLanguage, $x); }
	function setMaxIncludeSize( $x )            { return wfSetVar( $this->mMaxIncludeSize, $x ); }
	function setMaxPPNodeCount( $x )            { return wfSetVar( $this->mMaxPPNodeCount, $x ); }
	function setMaxTemplateDepth( $x )          { return wfSetVar( $this->mMaxTemplateDepth, $x ); }
	function setRemoveComments( $x )            { return wfSetVar( $this->mRemoveComments, $x ); }
	function setTemplateCallback( $x )          { return wfSetVar( $this->mTemplateCallback, $x ); }
	function enableLimitReport( $x = true )     { return wfSetVar( $this->mEnableLimitReport, $x ); }
	function setTimestamp( $x )                 { return wfSetVar( $this->mTimestamp, $x ); }

	function __construct( $user = null ) {
		$this->initialiseFromUser( $user );
	}

	/**
	 * Get parser options
	 * @static
	 */
	static function newFromUser( $user ) {
		return new ParserOptions( $user );
	}

	/** Get user options */
	function initialiseFromUser( $userInput ) {
		global $wgUseTeX, $wgUseDynamicDates, $wgInterwikiMagic, $wgAllowExternalImages;
		global $wgAllowExternalImagesFrom, $wgAllowSpecialInclusion, $wgMaxArticleSize;
		global $wgMaxPPNodeCount, $wgMaxTemplateDepth, $wgMaxPPExpandDepth;
		$fname = 'ParserOptions::initialiseFromUser';
		wfProfileIn( $fname );
		if ( !$userInput ) {
			global $wgUser;
			if ( isset( $wgUser ) ) {
				$user = $wgUser;
			} else {
				$user = new User;
			}
		} else {
			$user =& $userInput;
		}

		$this->mUser = $user;

		$this->mUseTeX = $wgUseTeX;
		$this->mUseDynamicDates = $wgUseDynamicDates;
		$this->mInterwikiMagic = $wgInterwikiMagic;
		$this->mAllowExternalImages = $wgAllowExternalImages;
		$this->mAllowExternalImagesFrom = $wgAllowExternalImagesFrom;
		$this->mSkin = null; # Deferred
		$this->mDateFormat = null; # Deferred
		$this->mEditSection = true;
		$this->mNumberHeadings = $user->getOption( 'numberheadings' );
		$this->mAllowSpecialInclusion = $wgAllowSpecialInclusion;
		$this->mTidy = false;
		$this->mInterfaceMessage = false;
		$this->mTargetLanguage = null; // default depends on InterfaceMessage setting
		$this->mMaxIncludeSize = $wgMaxArticleSize * 1024;
		$this->mMaxPPNodeCount = $wgMaxPPNodeCount;
		$this->mMaxPPExpandDepth = $wgMaxPPExpandDepth;
		$this->mMaxTemplateDepth = $wgMaxTemplateDepth;
		$this->mRemoveComments = true;
		$this->mTemplateCallback = array( 'Parser', 'statelessFetchTemplate' );
		$this->mEnableLimitReport = false;
		wfProfileOut( $fname );
	}
}
