<?php
/**
 * @defgroup Parser Parser
 *
 * @file
 * @ingroup Parser
 * File for Parser and related classes
 */


/**
 * PHP Parser - Processes wiki markup (which uses a more user-friendly
 * syntax, such as "[[link]]" for making links), and provides a one-way
 * transformation of that wiki markup it into XHTML output / markup
 * (which in turn the browser understands, and can display).
 *
 * <pre>
 * There are five main entry points into the Parser class:
 * parse()
 *   produces HTML output
 * preSaveTransform().
 *   produces altered wiki markup.
 * preprocess()
 *   removes HTML comments and expands templates
 * cleanSig()
 *   Cleans a signature before saving it to preferences
 * extractSections()
 *   Extracts sections from an article for section editing
 *
 * Globals used:
 *    objects:   $wgLang, $wgContLang
 *
 * NOT $wgArticle, $wgUser or $wgTitle. Keep them away!
 *
 * settings:
 *  $wgUseTex*, $wgUseDynamicDates*, $wgInterwikiMagic*,
 *  $wgNamespacesWithSubpages, $wgAllowExternalImages*,
 *  $wgLocaltimezone, $wgAllowSpecialInclusion*,
 *  $wgMaxArticleSize*
 *
 *  * only within ParserOptions
 * </pre>
 *
 * @ingroup Parser
 */
class Parser
{
	/**
	 * Update this version number when the ParserOutput format
	 * changes in an incompatible way, so the parser cache
	 * can automatically discard old data.
	 */
	const VERSION = '1.6.4';

	# Flags for Parser::setFunctionHook
	# Also available as global constants from Defines.php
	const SFH_NO_HASH = 1;
	const SFH_OBJECT_ARGS = 2;

	# Constants needed for external link processing
	# Everything except bracket, space, or control characters
	const EXT_LINK_URL_CLASS = '[^][<>"\\x00-\\x20\\x7F]';
	const EXT_IMAGE_REGEX = '/^(http:\/\/|https:\/\/)([^][<>"\\x00-\\x20\\x7F]+)
		\\/([A-Za-z0-9_.,~%\\-+&;#*?!=()@\\x80-\\xFF]+)\\.((?i)gif|png|jpg|jpeg)$/Sx';

	// State constants for the definition list colon extraction
	const COLON_STATE_TEXT = 0;
	const COLON_STATE_TAG = 1;
	const COLON_STATE_TAGSTART = 2;
	const COLON_STATE_CLOSETAG = 3;
	const COLON_STATE_TAGSLASH = 4;
	const COLON_STATE_COMMENT = 5;
	const COLON_STATE_COMMENTDASH = 6;
	const COLON_STATE_COMMENTDASHDASH = 7;

	// Flags for preprocessToDom
	const PTD_FOR_INCLUSION = 1;

	// Allowed values for $this->mOutputType
	// Parameter to startExternalParse().
	const OT_HTML = 1;
	const OT_WIKI = 2;
	const OT_PREPROCESS = 3;
	const OT_MSG = 3;

	// Marker Suffix needs to be accessible staticly.
	const MARKER_SUFFIX = "-QINU\x7f";

	/**#@+
	 * @private
	 */
	# Persistent:
	var $mTagHooks, $mTransparentTagHooks, $mFunctionHooks, $mFunctionSynonyms, $mVariables,
		$mImageParams, $mImageParamsMagicArray, $mStripList, $mMarkerIndex, $mPreprocessor,
		$mExtLinkBracketedRegex, $mDefaultStripList, $mVarCache, $mConf;


	# Cleared with clearState():
	var $mOutput, $mAutonumber, $mDTopen, $mStripState;
	var $mIncludeCount, $mArgStack, $mLastSection, $mInPre;
	var $mInterwikiLinkHolders, $mLinkHolders;
	var $mIncludeSizes, $mPPNodeCount, $mDefaultSort;
	var $mTplExpandCache; // empty-frame expansion cache
	var $mTplRedirCache, $mTplDomCache, $mHeadings, $mDoubleUnderscores;
	var $mExpensiveFunctionCount; // number of expensive parser function calls

	# Temporary
	# These are variables reset at least once per parse regardless of $clearState
	var $mOptions,      // ParserOptions object
		$mTitle,        // Title context, used for self-link rendering and similar things
		$mOutputType,   // Output type, one of the OT_xxx constants
		$ot,            // Shortcut alias, see setOutputType()
		$mRevisionId,   // ID to display in {{REVISIONID}} tags
		$mRevisionTimestamp, // The timestamp of the specified revision ID
		$mRevIdForTs;   // The revision ID which was used to fetch the timestamp

	/**#@-*/

	/**
	 * Constructor
	 *
	 * @public
	 */
	function __construct( $conf = array() ) {
		$this->mConf = $conf;
		$this->mTagHooks = array();
		$this->mTransparentTagHooks = array();
		$this->mFunctionHooks = array();
		$this->mFunctionSynonyms = array( 0 => array(), 1 => array() );
		$this->mDefaultStripList = $this->mStripList = array( 'nowiki', 'gallery' );
		$this->mExtLinkBracketedRegex = '/\[(\b(' . wfUrlProtocols() . ')'.
			'[^][<>"\\x00-\\x20\\x7F]+) *([^\]\\x0a\\x0d]*?)\]/S';
		$this->mVarCache = array();
		if ( isset( $conf['preprocessorClass'] ) ) {
			$this->mPreprocessorClass = $conf['preprocessorClass'];
		} elseif ( extension_loaded( 'domxml' ) ) {
			// PECL extension that conflicts with the core DOM extension (bug 13770)
			wfDebug( "Warning: you have the obsolete domxml extension for PHP. Please remove it!\n" );
			$this->mPreprocessorClass = 'Preprocessor_Hash';
		} elseif ( extension_loaded( 'dom' ) ) {
			$this->mPreprocessorClass = 'Preprocessor_DOM';
		} else {
			$this->mPreprocessorClass = 'Preprocessor_Hash';
		}
		$this->mMarkerIndex = 0;
		$this->mFirstCall = true;
	}

	/**
	 * Do various kinds of initialisation on the first call of the parser
	 */
	function firstCallInit() {
		if ( !$this->mFirstCall ) {
			return;
		}
		$this->mFirstCall = false;

		wfProfileIn( __METHOD__ );

		$this->setHook( 'pre', array( $this, 'renderPreTag' ) );
		CoreParserFunctions::register( $this );
		$this->initialiseVariables();

		wfRunHooks( 'ParserFirstCallInit', array( &$this ) );
		wfProfileOut( __METHOD__ );
	}

	/**
	 * Clear Parser state
	 *
	 * @private
	 */
	function clearState() {
		wfProfileIn( __METHOD__ );
		if ( $this->mFirstCall ) {
			$this->firstCallInit();
		}
		$this->mOutput = new ParserOutput;
		$this->mAutonumber = 0;
		$this->mLastSection = '';
		$this->mDTopen = false;
		$this->mIncludeCount = array();
		$this->mStripState = new StripState;
		$this->mArgStack = false;
		$this->mInPre = false;
		$this->mInterwikiLinkHolders = array(
			'texts' => array(),
			'titles' => array()
		);
		$this->mLinkHolders = array(
			'namespaces' => array(),
			'dbkeys' => array(),
			'queries' => array(),
			'texts' => array(),
			'titles' => array()
		);
		$this->mRevisionTimestamp = $this->mRevisionId = null;

		/**
		 * Prefix for temporary replacement strings for the multipass parser.
		 * \x07 should never appear in input as it's disallowed in XML.
		 * Using it at the front also gives us a little extra robustness
		 * since it shouldn't match when butted up against identifier-like
		 * string constructs.
		 *
		 * Must not consist of all title characters, or else it will change
		 * the behaviour of <nowiki> in a link.
		 */
		#$this->mUniqPrefix = "\x07UNIQ" . Parser::getRandomString();
		# Changed to \x7f to allow XML double-parsing -- TS
		$this->mUniqPrefix = "\x7fUNIQ" . Parser::getRandomString();


		# Clear these on every parse, bug 4549
		$this->mTplExpandCache = $this->mTplRedirCache = $this->mTplDomCache = array();

		$this->mShowToc = true;
		$this->mForceTocPosition = false;
		$this->mIncludeSizes = array(
			'post-expand' => 0,
			'arg' => 0,
		);
		$this->mPPNodeCount = 0;
		$this->mDefaultSort = false;
		$this->mHeadings = array();
		$this->mDoubleUnderscores = array();
		$this->mExpensiveFunctionCount = 0;

		# Fix cloning
		if ( isset( $this->mPreprocessor ) && $this->mPreprocessor->parser !== $this ) {
			$this->mPreprocessor = null;
		}

		wfRunHooks( 'ParserClearState', array( &$this ) );
		wfProfileOut( __METHOD__ );
	}

	function setOutputType( $ot ) {
		$this->mOutputType = $ot;
		// Shortcut alias
		$this->ot = array(
			'html' => $ot == self::OT_HTML,
			'wiki' => $ot == self::OT_WIKI,
			'pre' => $ot == self::OT_PREPROCESS,
		);
	}

	/**
	 * Set the context title
	 */
	function setTitle( $t ) {
		if ( !$t || $t instanceof FakeTitle ) {
			$t = Title::newFromText( 'NO TITLE' );
		}
		if ( strval( $t->getFragment() ) !== '' ) {
			# Strip the fragment to avoid various odd effects
			$this->mTitle = clone $t;
			$this->mTitle->setFragment( '' );
		} else {
			$this->mTitle = $t;
		}
	}

	/**
	 * Accessor for mUniqPrefix.
	 *
	 * @public
	 */
	function uniqPrefix() {
		if( !isset( $this->mUniqPrefix ) ) {
			// @fixme this is probably *horribly wrong*
			// LanguageConverter seems to want $wgParser's uniqPrefix, however
			// if this is called for a parser cache hit, the parser may not
			// have ever been initialized in the first place.
			// Not really sure what the heck is supposed to be going on here.
			return '';
			//throw new MWException( "Accessing uninitialized mUniqPrefix" );
		}
		return $this->mUniqPrefix;
	}

	/**
	 * Convert wikitext to HTML
	 * Do not call this function recursively.
	 *
	 * @param string $text Text we want to parse
	 * @param Title &$title A title object
	 * @param array $options
	 * @param boolean $linestart
	 * @param boolean $clearState
	 * @param int $revid number to pass in {{REVISIONID}}
	 * @return ParserOutput a ParserOutput
	 */
	public function parse( $text, &$title, $options, $linestart = true, $clearState = true, $revid = null ) {
		/**
		 * First pass--just handle <nowiki> sections, pass the rest off
		 * to internalParse() which does all the real work.
		 */

		global $wgUseTidy, $wgAlwaysUseTidy, $wgContLang;
		$fname = 'Parser::parse-' . wfGetCaller();
		wfProfileIn( __METHOD__ );
		wfProfileIn( $fname );

		if ( $clearState ) {
			$this->clearState();
		}

		$this->mOptions = $options;
		$this->setTitle( $title );
		$oldRevisionId = $this->mRevisionId;
		$oldRevisionTimestamp = $this->mRevisionTimestamp;
		if( $revid !== null ) {
			$this->mRevisionId = $revid;
			$this->mRevisionTimestamp = null;
		}
		$this->setOutputType( self::OT_HTML );
		wfRunHooks( 'ParserBeforeStrip', array( &$this, &$text, &$this->mStripState ) );
		# No more strip!
		wfRunHooks( 'ParserAfterStrip', array( &$this, &$text, &$this->mStripState ) );
		$text = $this->internalParse( $text );
		$text = $this->mStripState->unstripGeneral( $text );

		# Clean up special characters, only run once, next-to-last before doBlockLevels
		$fixtags = array(
			# french spaces, last one Guillemet-left
			# only if there is something before the space
			'/(.) (?=\\?|:|;|!|%|\\302\\273)/' => '\\1&nbsp;\\2',
			# french spaces, Guillemet-right
			'/(\\302\\253) /' => '\\1&nbsp;',
			'/&nbsp;(!\s*important)/' => ' \\1', #Beware of CSS magic word !important, bug #11874.
		);
		$text = preg_replace( array_keys($fixtags), array_values($fixtags), $text );

		# only once and last
		$text = $this->doBlockLevels( $text, $linestart );

		$this->replaceLinkHolders( $text );

		# the position of the parserConvert() call should not be changed. it
		# assumes that the links are all replaced and the only thing left
		# is the <nowiki> mark.
		# Side-effects: this calls $this->mOutput->setTitleText()
		$text = $wgContLang->parserConvert( $text, $this );

		$text = $this->mStripState->unstripNoWiki( $text );

		wfRunHooks( 'ParserBeforeTidy', array( &$this, &$text ) );

//!JF Move to its own function

		$uniq_prefix = $this->mUniqPrefix;
		$matches = array();
		$elements = array_keys( $this->mTransparentTagHooks );
		$text = Parser::extractTagsAndParams( $elements, $text, $matches, $uniq_prefix );

		foreach( $matches as $marker => $data ) {
			list( $element, $content, $params, $tag ) = $data;
			$tagName = strtolower( $element );
			if( isset( $this->mTransparentTagHooks[$tagName] ) ) {
				$output = call_user_func_array( $this->mTransparentTagHooks[$tagName],
					array( $content, $params, $this ) );
			} else {
				$output = $tag;
			}
			$this->mStripState->general->setPair( $marker, $output );
		}
		$text = $this->mStripState->unstripGeneral( $text );

		$text = Sanitizer::normalizeCharReferences( $text );

		if (($wgUseTidy and $this->mOptions->mTidy) or $wgAlwaysUseTidy) {
			$text = Parser::tidy($text);
		} else {
			# attempt to sanitize at least some nesting problems
			# (bug #2702 and quite a few others)
			$tidyregs = array(
				# ''Something [http://www.cool.com cool''] -->
				# <i>Something</i><a href="http://www.cool.com"..><i>cool></i></a>
				'/(<([bi])>)(<([bi])>)?([^<]*)(<\/?a[^<]*>)([^<]*)(<\/\\4>)?(<\/\\2>)/' =>
				'\\1\\3\\5\\8\\9\\6\\1\\3\\7\\8\\9',
				# fix up an anchor inside another anchor, only
				# at least for a single single nested link (bug 3695)
				'/(<a[^>]+>)([^<]*)(<a[^>]+>[^<]*)<\/a>(.*)<\/a>/' =>
				'\\1\\2</a>\\3</a>\\1\\4</a>',
				# fix div inside inline elements- doBlockLevels won't wrap a line which
				# contains a div, so fix it up here; replace
				# div with escaped text
				'/(<([aib]) [^>]+>)([^<]*)(<div([^>]*)>)(.*)(<\/div>)([^<]*)(<\/\\2>)/' =>
				'\\1\\3&lt;div\\5&gt;\\6&lt;/div&gt;\\8\\9',
				# remove empty italic or bold tag pairs, some
				# introduced by rules above
				'/<([bi])><\/\\1>/' => '',
			);

			$text = preg_replace(
				array_keys( $tidyregs ),
				array_values( $tidyregs ),
				$text );
		}
		global $wgExpensiveParserFunctionLimit;
		if ( $this->mExpensiveFunctionCount > $wgExpensiveParserFunctionLimit ) {
			$this->limitationWarn( 'expensive-parserfunction', $this->mExpensiveFunctionCount, $wgExpensiveParserFunctionLimit );
		}

		wfRunHooks( 'ParserAfterTidy', array( &$this, &$text ) );

		# Information on include size limits, for the benefit of users who try to skirt them
		if ( $this->mOptions->getEnableLimitReport() ) {
			global $wgExpensiveParserFunctionLimit;
			$max = $this->mOptions->getMaxIncludeSize();
			$PFreport = "Expensive parser function count: {$this->mExpensiveFunctionCount}/$wgExpensiveParserFunctionLimit\n";
			$limitReport =
				"NewPP limit report\n" .
				"Preprocessor node count: {$this->mPPNodeCount}/{$this->mOptions->mMaxPPNodeCount}\n" .
				"Post-expand include size: {$this->mIncludeSizes['post-expand']}/$max bytes\n" .
				"Template argument size: {$this->mIncludeSizes['arg']}/$max bytes\n".
				$PFreport;
			wfRunHooks( 'ParserLimitReport', array( $this, &$limitReport ) );
			$text .= "\n<!-- \n$limitReport-->\n";
		}
		$this->mOutput->setText( $text );
		$this->mRevisionId = $oldRevisionId;
		$this->mRevisionTimestamp = $oldRevisionTimestamp;
		wfProfileOut( $fname );
		wfProfileOut( __METHOD__ );

		return $this->mOutput;
	}

	/**
	 * Recursive parser entry point that can be called from an extension tag
	 * hook.
	 */
	function recursiveTagParse( $text ) {
		wfProfileIn( __METHOD__ );
		wfRunHooks( 'ParserBeforeStrip', array( &$this, &$text, &$this->mStripState ) );
		wfRunHooks( 'ParserAfterStrip', array( &$this, &$text, &$this->mStripState ) );
		$text = $this->internalParse( $text );
		wfProfileOut( __METHOD__ );
		return $text;
	}

	/**
	 * Expand templates and variables in the text, producing valid, static wikitext.
	 * Also removes comments.
	 */
	function preprocess( $text, $title, $options, $revid = null ) {
		wfProfileIn( __METHOD__ );
		$this->clearState();
		$this->setOutputType( self::OT_PREPROCESS );
		$this->mOptions = $options;
		$this->setTitle( $title );
		if( $revid !== null ) {
			$this->mRevisionId = $revid;
		}
		wfRunHooks( 'ParserBeforeStrip', array( &$this, &$text, &$this->mStripState ) );
		wfRunHooks( 'ParserAfterStrip', array( &$this, &$text, &$this->mStripState ) );
		$text = $this->replaceVariables( $text );
		$text = $this->mStripState->unstripBoth( $text );
		wfProfileOut( __METHOD__ );
		return $text;
	}

	/**
	 * Get a random string
	 *
	 * @private
	 * @static
	 */
	function getRandomString() {
		return dechex(mt_rand(0, 0x7fffffff)) . dechex(mt_rand(0, 0x7fffffff));
	}

	function &getTitle() { return $this->mTitle; }
	function getOptions() { return $this->mOptions; }
	function getRevisionId() { return $this->mRevisionId; }

	function getFunctionLang() {
		global $wgLang, $wgContLang;

		$target = $this->mOptions->getTargetLanguage();
		if ( $target !== null ) {
			return $target;
		} else {
			return $this->mOptions->getInterfaceMessage() ? $wgLang : $wgContLang;
		}
	}

	/**
	 * Get a preprocessor object
	 */
	function getPreprocessor() {
		if ( !isset( $this->mPreprocessor ) ) {
			$class = $this->mPreprocessorClass;
			$this->mPreprocessor = new $class( $this );
		}
		return $this->mPreprocessor;
	}

	/**
	 * Replaces all occurrences of HTML-style comments and the given tags
	 * in the text with a random marker and returns the next text. The output
	 * parameter $matches will be an associative array filled with data in
	 * the form:
	 *   'UNIQ-xxxxx' => array(
	 *     'element',
	 *     'tag content',
	 *     array( 'param' => 'x' ),
	 *     '<element param="x">tag content</element>' ) )
	 *
	 * @param $elements list of element names. Comments are always extracted.
	 * @param $text Source text string.
	 * @param $uniq_prefix
	 *
	 * @public
	 * @static
	 */
	function extractTagsAndParams($elements, $text, &$matches, $uniq_prefix = ''){
		static $n = 1;
		$stripped = '';
		$matches = array();

		$taglist = implode( '|', $elements );
		$start = "/<($taglist)(\\s+[^>]*?|\\s*?)(\/?>)|<(!--)/i";

		while ( '' != $text ) {
			$p = preg_split( $start, $text, 2, PREG_SPLIT_DELIM_CAPTURE );
			$stripped .= $p[0];
			if( count( $p ) < 5 ) {
				break;
			}
			if( count( $p ) > 5 ) {
				// comment
				$element    = $p[4];
				$attributes = '';
				$close      = '';
				$inside     = $p[5];
			} else {
				// tag
				$element    = $p[1];
				$attributes = $p[2];
				$close      = $p[3];
				$inside     = $p[4];
			}

			$marker = "$uniq_prefix-$element-" . sprintf('%08X', $n++) . self::MARKER_SUFFIX;
			$stripped .= $marker;

			if ( $close === '/>' ) {
				// Empty element tag, <tag />
				$content = null;
				$text = $inside;
				$tail = null;
			} else {
				if( $element == '!--' ) {
					$end = '/(-->)/';
				} else {
					$end = "/(<\\/$element\\s*>)/i";
				}
				$q = preg_split( $end, $inside, 2, PREG_SPLIT_DELIM_CAPTURE );
				$content = $q[0];
				if( count( $q ) < 3 ) {
					# No end tag -- let it run out to the end of the text.
					$tail = '';
					$text = '';
				} else {
					$tail = $q[1];
					$text = $q[2];
				}
			}

			$matches[$marker] = array( $element,
				$content,
				Sanitizer::decodeTagAttributes( $attributes ),
				"<$element$attributes$close$content$tail" );
		}
		return $stripped;
	}

	/**
	 * Get a list of strippable XML-like elements
	 */
	function getStripList() {
		global $wgRawHtml;
		$elements = $this->mStripList;
		if( $wgRawHtml ) {
			$elements[] = 'html';
		}
		if( $this->mOptions->getUseTeX() ) {
			$elements[] = 'math';
		}
		return $elements;
	}

	/**
	 * @deprecated use replaceVariables
	 */
	function strip( $text, $state, $stripcomments = false , $dontstrip = array () ) {
		return $text;
	}

	/**
	 * Restores pre, math, and other extensions removed by strip()
	 *
	 * always call unstripNoWiki() after this one
	 * @private
	 * @deprecated use $this->mStripState->unstrip()
	 */
	function unstrip( $text, $state ) {
		return $state->unstripGeneral( $text );
	}

	/**
	 * Always call this after unstrip() to preserve the order
	 *
	 * @private
	 * @deprecated use $this->mStripState->unstrip()
	 */
	function unstripNoWiki( $text, $state ) {
		return $state->unstripNoWiki( $text );
	}

	/**
	 * @deprecated use $this->mStripState->unstripBoth()
	 */
	function unstripForHTML( $text ) {
		return $this->mStripState->unstripBoth( $text );
	}

	/**
	 * Add an item to the strip state
	 * Returns the unique tag which must be inserted into the stripped text
	 * The tag will be replaced with the original text in unstrip()
	 *
	 * @private
	 */
	function insertStripItem( $text ) {
		$rnd = "{$this->mUniqPrefix}-item-{$this->mMarkerIndex}-" . self::MARKER_SUFFIX;
		$this->mMarkerIndex++;
		$this->mStripState->general->setPair( $rnd, $text );
		return $rnd;
	}

	/**
	 * Interface with html tidy, used if $wgUseTidy = true.
	 * If tidy isn't able to correct the markup, the original will be
	 * returned in all its glory with a warning comment appended.
	 *
	 * Either the external tidy program or the in-process tidy extension
	 * will be used depending on availability. Override the default
	 * $wgTidyInternal setting to disable the internal if it's not working.
	 *
	 * @param string $text Hideous HTML input
	 * @return string Corrected HTML output
	 * @public
	 * @static
	 */
	function tidy( $text ) {
		global $wgTidyInternal;
		$wrappedtext = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"'.
' "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"><html>'.
'<head><title>test</title></head><body>'.$text.'</body></html>';
		if( $wgTidyInternal ) {
			$correctedtext = Parser::internalTidy( $wrappedtext );
		} else {
			$correctedtext = Parser::externalTidy( $wrappedtext );
		}
		if( is_null( $correctedtext ) ) {
			wfDebug( "Tidy error detected!\n" );
			return $text . "\n<!-- Tidy found serious XHTML errors -->\n";
		}
		return $correctedtext;
	}

	/**
	 * Spawn an external HTML tidy process and get corrected markup back from it.
	 *
	 * @private
	 * @static
	 */
	function externalTidy( $text ) {
		global $wgTidyConf, $wgTidyBin, $wgTidyOpts;
		$fname = 'Parser::externalTidy';
		wfProfileIn( $fname );

		$cleansource = '';
		$opts = ' -utf8';

		$descriptorspec = array(
			0 => array('pipe', 'r'),
			1 => array('pipe', 'w'),
			2 => array('file', wfGetNull(), 'a')
		);
		$pipes = array();
		$process = proc_open("$wgTidyBin -config $wgTidyConf $wgTidyOpts$opts", $descriptorspec, $pipes);
		if (is_resource($process)) {
			// Theoretically, this style of communication could cause a deadlock
			// here. If the stdout buffer fills up, then writes to stdin could
			// block. This doesn't appear to happen with tidy, because tidy only
			// writes to stdout after it's finished reading from stdin. Search
			// for tidyParseStdin and tidySaveStdout in console/tidy.c
			fwrite($pipes[0], $text);
			fclose($pipes[0]);
			while (!feof($pipes[1])) {
				$cleansource .= fgets($pipes[1], 1024);
			}
			fclose($pipes[1]);
			proc_close($process);
		}

		wfProfileOut( $fname );

		if( $cleansource == '' && $text != '') {
			// Some kind of error happened, so we couldn't get the corrected text.
			// Just give up; we'll use the source text and append a warning.
			return null;
		} else {
			return $cleansource;
		}
	}

	/**
	 * Use the HTML tidy PECL extension to use the tidy library in-process,
	 * saving the overhead of spawning a new process.
	 *
	 * 'pear install tidy' should be able to compile the extension module.
	 *
	 * @private
	 * @static
	 */
	function internalTidy( $text ) {
		global $wgTidyConf, $IP, $wgDebugTidy;
		$fname = 'Parser::internalTidy';
		wfProfileIn( $fname );

		$tidy = new tidy;
		$tidy->parseString( $text, $wgTidyConf, 'utf8' );
		$tidy->cleanRepair();
		if( $tidy->getStatus() == 2 ) {
			// 2 is magic number for fatal error
			// http://www.php.net/manual/en/function.tidy-get-status.php
			$cleansource = null;
		} else {
			$cleansource = tidy_get_output( $tidy );
		}
		if ( $wgDebugTidy && $tidy->getStatus() > 0 ) {
			$cleansource .= "<!--\nTidy reports:\n" .
				str_replace( '-->', '--&gt;', $tidy->errorBuffer ) .
				"\n-->";
		}

		wfProfileOut( $fname );
		return $cleansource;
	}

	/**
	 * parse the wiki syntax used to render tables
	 *
	 * @private
	 */
	function doTableStuff ( $text ) {
		$fname = 'Parser::doTableStuff';
		wfProfileIn( $fname );

		$lines = explode ( "\n" , $text );
		$td_history = array (); // Is currently a td tag open?
		$last_tag_history = array (); // Save history of last lag activated (td, th or caption)
		$tr_history = array (); // Is currently a tr tag open?
		$tr_attributes = array (); // history of tr attributes
		$has_opened_tr = array(); // Did this table open a <tr> element?
		$indent_level = 0; // indent level of the table
		foreach ( $lines as $key => $line )
		{
			$line = trim ( $line );

			if( $line == '' ) { // empty line, go to next line
				continue;
			}
			$first_character = $line{0};
			$matches = array();

			if ( preg_match( '/^(:*)\{\|(.*)$/' , $line , $matches ) ) {
				// First check if we are starting a new table
				$indent_level = strlen( $matches[1] );

				$attributes = $this->mStripState->unstripBoth( $matches[2] );
				$attributes = Sanitizer::fixTagAttributes ( $attributes , 'table' );

				$lines[$key] = str_repeat( '<dl><dd>' , $indent_level ) . "<table{$attributes}>";
				array_push ( $td_history , false );
				array_push ( $last_tag_history , '' );
				array_push ( $tr_history , false );
				array_push ( $tr_attributes , '' );
				array_push ( $has_opened_tr , false );
			} else if ( count ( $td_history ) == 0 ) {
				// Don't do any of the following
				continue;
			} else if ( substr ( $line , 0 , 2 ) == '|}' ) {
				// We are ending a table
				$line = '</table>' . substr ( $line , 2 );
				$last_tag = array_pop ( $last_tag_history );

				if ( !array_pop ( $has_opened_tr ) ) {
					$line = "<tr><td></td></tr>{$line}";
				}

				if ( array_pop ( $tr_history ) ) {
					$line = "</tr>{$line}";
				}

				if ( array_pop ( $td_history ) ) {
					$line = "</{$last_tag}>{$line}";
				}
				array_pop ( $tr_attributes );
				$lines[$key] = $line . str_repeat( '</dd></dl>' , $indent_level );
			} else if ( substr ( $line , 0 , 2 ) == '|-' ) {
				// Now we have a table row
				$line = preg_replace( '#^\|-+#', '', $line );

				// Whats after the tag is now only attributes
				$attributes = $this->mStripState->unstripBoth( $line );
				$attributes = Sanitizer::fixTagAttributes ( $attributes , 'tr' );
				array_pop ( $tr_attributes );
				array_push ( $tr_attributes , $attributes );

				$line = '';
				$last_tag = array_pop ( $last_tag_history );
				array_pop ( $has_opened_tr );
				array_push ( $has_opened_tr , true );

				if ( array_pop ( $tr_history ) ) {
					$line = '</tr>';
				}

				if ( array_pop ( $td_history ) ) {
					$line = "</{$last_tag}>{$line}";
				}

				$lines[$key] = $line;
				array_push ( $tr_history , false );
				array_push ( $td_history , false );
				array_push ( $last_tag_history , '' );
			}
			else if ( $first_character == '|' || $first_character == '!' || substr ( $line , 0 , 2 )  == '|+' ) {
				// This might be cell elements, td, th or captions
				if ( substr ( $line , 0 , 2 ) == '|+' ) {
					$first_character = '+';
					$line = substr ( $line , 1 );
				}

				$line = substr ( $line , 1 );

				if ( $first_character == '!' ) {
					$line = str_replace ( '!!' , '||' , $line );
				}

				// Split up multiple cells on the same line.
				// FIXME : This can result in improper nesting of tags processed
				// by earlier parser steps, but should avoid splitting up eg
				// attribute values containing literal "||".
				$cells = StringUtils::explodeMarkup( '||' , $line );

				$lines[$key] = '';

				// Loop through each table cell
				foreach ( $cells as $cell )
				{
					$previous = '';
					if ( $first_character != '+' )
					{
						$tr_after = array_pop ( $tr_attributes );
						if ( !array_pop ( $tr_history ) ) {
							$previous = "<tr{$tr_after}>\n";
						}
						array_push ( $tr_history , true );
						array_push ( $tr_attributes , '' );
						array_pop ( $has_opened_tr );
						array_push ( $has_opened_tr , true );
					}

					$last_tag = array_pop ( $last_tag_history );

					if ( array_pop ( $td_history ) ) {
						$previous = "</{$last_tag}>{$previous}";
					}

					if ( $first_character == '|' ) {
						$last_tag = 'td';
					} else if ( $first_character == '!' ) {
						$last_tag = 'th';
					} else if ( $first_character == '+' ) {
						$last_tag = 'caption';
					} else {
						$last_tag = '';
					}

					array_push ( $last_tag_history , $last_tag );

					// A cell could contain both parameters and data
					$cell_data = explode ( '|' , $cell , 2 );

					// Bug 553: Note that a '|' inside an invalid link should not
					// be mistaken as delimiting cell parameters
					if ( strpos( $cell_data[0], '[[' ) !== false ) {
						$cell = "{$previous}<{$last_tag}>{$cell}";
					} else if ( count ( $cell_data ) == 1 )
						$cell = "{$previous}<{$last_tag}>{$cell_data[0]}";
					else {
						$attributes = $this->mStripState->unstripBoth( $cell_data[0] );
						$attributes = Sanitizer::fixTagAttributes( $attributes , $last_tag );
						$cell = "{$previous}<{$last_tag}{$attributes}>{$cell_data[1]}";
					}

					$lines[$key] .= $cell;
					array_push ( $td_history , true );
				}
			}
		}

		// Closing open td, tr && table
		while ( count ( $td_history ) > 0 )
		{
			if ( array_pop ( $td_history ) ) {
				$lines[] = '</td>' ;
			}
			if ( array_pop ( $tr_history ) ) {
				$lines[] = '</tr>' ;
			}
			if ( !array_pop ( $has_opened_tr ) ) {
				$lines[] = "<tr><td></td></tr>" ;
			}

			$lines[] = '</table>' ;
		}

		$output = implode ( "\n" , $lines ) ;

		// special case: don't return empty table
		if( $output == "<table>\n<tr><td></td></tr>\n</table>" ) {
			$output = '';
		}

		wfProfileOut( $fname );

		return $output;
	}

	/**
	 * Helper function for parse() that transforms wiki markup into
	 * HTML. Only called for $mOutputType == self::OT_HTML.
	 *
	 * @private
	 */
	function internalParse( $text ) {
		$isMain = true;
		$fname = 'Parser::internalParse';
		wfProfileIn( $fname );

		# Hook to suspend the parser in this state
		if ( !wfRunHooks( 'ParserBeforeInternalParse', array( &$this, &$text, &$this->mStripState ) ) ) {
			wfProfileOut( $fname );
			return $text ;
		}

		$text = $this->replaceVariables( $text );
		$text = Sanitizer::removeHTMLtags( $text, array( &$this, 'attributeStripCallback' ), false, array_keys( $this->mTransparentTagHooks ) );
		wfRunHooks( 'InternalParseBeforeLinks', array( &$this, &$text, &$this->mStripState ) );

		// Tables need to come after variable replacement for things to work
		// properly; putting them before other transformations should keep
		// exciting things like link expansions from showing up in surprising
		// places.
		$text = $this->doTableStuff( $text );

		$text = preg_replace( '/(^|\n)-----*/', '\\1<hr />', $text );

		$text = $this->doDoubleUnderscore( $text );
		$text = $this->doHeadings( $text );
		if($this->mOptions->getUseDynamicDates()) {
			$df = DateFormatter::getInstance();
			$text = $df->reformat( $this->mOptions->getDateFormat(), $text );
		}
		$text = $this->doAllQuotes( $text );
		$text = $this->replaceInternalLinks( $text );
		$text = $this->replaceExternalLinks( $text );

		# replaceInternalLinks may sometimes leave behind
		# absolute URLs, which have to be masked to hide them from replaceExternalLinks
		$text = str_replace($this->mUniqPrefix."NOPARSE", "", $text);

		$text = $this->doMagicLinks( $text );
		$text = $this->formatHeadings( $text, $isMain );

		wfProfileOut( $fname );
		return $text;
	}

	/**
	 * Replace special strings like "ISBN xxx" and "RFC xxx" with
	 * magic external links.
	 *
	 * @private
	 */
	function doMagicLinks( $text ) {
		wfProfileIn( __METHOD__ );
		$text = preg_replace_callback(
			'!(?:                           # Start cases
			    <a.*?</a> |                 # Skip link text
			    <.*?> |                     # Skip stuff inside HTML elements
			    (?:RFC|PMID)\s+([0-9]+) |   # RFC or PMID, capture number as m[1]
			    ISBN\s+(\b                  # ISBN, capture number as m[2]
                                     (?: 97[89] [\ \-]? )?   # optional 13-digit ISBN prefix
                                     (?: [0-9]  [\ \-]? ){9} # 9 digits with opt. delimiters
                                     [0-9Xx]                 # check digit
                                   \b)
			)!x', array( &$this, 'magicLinkCallback' ), $text );
		wfProfileOut( __METHOD__ );
		return $text;
	}

	function magicLinkCallback( $m ) {
		if ( substr( $m[0], 0, 1 ) == '<' ) {
			# Skip HTML element
			return $m[0];
		} elseif ( substr( $m[0], 0, 4 ) == 'ISBN' ) {
			$isbn = $m[2];
			$num = strtr( $isbn, array(
				'-' => '',
				' ' => '',
				'x' => 'X',
			));
			$titleObj = SpecialPage::getTitleFor( 'Booksources', $num );
			$text = '<a href="' .
				$titleObj->escapeLocalUrl() .
				"\" class=\"internal\">ISBN $isbn</a>";
		} else {
			if ( substr( $m[0], 0, 3 ) == 'RFC' ) {
				$keyword = 'RFC';
				$urlmsg = 'rfcurl';
				$id = $m[1];
			} elseif ( substr( $m[0], 0, 4 ) == 'PMID' ) {
				$keyword = 'PMID';
				$urlmsg = 'pubmedurl';
				$id = $m[1];
			} else {
				throw new MWException( __METHOD__.': unrecognised match type "' .
					substr($m[0], 0, 20 ) . '"' );
			}

			$url = wfMsg( $urlmsg, $id);
			$sk = $this->mOptions->getSkin();
			$la = $sk->getExternalLinkAttributes( $url, $keyword.$id );
			$text = "<a href=\"{$url}\"{$la}>{$keyword} {$id}</a>";
		}
		return $text;
	}

	/**
	 * Parse headers and return html
	 *
	 * @private
	 */
	function doHeadings( $text ) {
		$fname = 'Parser::doHeadings';
		wfProfileIn( $fname );
		for ( $i = 6; $i >= 1; --$i ) {
			$h = str_repeat( '=', $i );
			$text = preg_replace( "/^$h(.+)$h\\s*$/m",
			  "<h$i>\\1</h$i>", $text );
		}
		wfProfileOut( $fname );
		return $text;
	}

	/**
	 * Replace single quotes with HTML markup
	 * @private
	 * @return string the altered text
	 */
	function doAllQuotes( $text ) {
		$fname = 'Parser::doAllQuotes';
		wfProfileIn( $fname );
		$outtext = '';
		$lines = explode( "\n", $text );
		foreach ( $lines as $line ) {
			$outtext .= $this->doQuotes ( $line ) . "\n";
		}
		$outtext = substr($outtext, 0,-1);
		wfProfileOut( $fname );
		return $outtext;
	}

	/**
	 * Helper function for doAllQuotes()
	 */
	public function doQuotes( $text ) {
		$arr = preg_split( "/(''+)/", $text, -1, PREG_SPLIT_DELIM_CAPTURE );
		if ( count( $arr ) == 1 )
			return $text;
		else
		{
			# First, do some preliminary work. This may shift some apostrophes from
			# being mark-up to being text. It also counts the number of occurrences
			# of bold and italics mark-ups.
			$i = 0;
			$numbold = 0;
			$numitalics = 0;
			foreach ( $arr as $r )
			{
				if ( ( $i % 2 ) == 1 )
				{
					# If there are ever four apostrophes, assume the first is supposed to
					# be text, and the remaining three constitute mark-up for bold text.
					if ( strlen( $arr[$i] ) == 4 )
					{
						$arr[$i-1] .= "'";
						$arr[$i] = "'''";
					}
					# If there are more than 5 apostrophes in a row, assume they're all
					# text except for the last 5.
					else if ( strlen( $arr[$i] ) > 5 )
					{
						$arr[$i-1] .= str_repeat( "'", strlen( $arr[$i] ) - 5 );
						$arr[$i] = "'''''";
					}
					# Count the number of occurrences of bold and italics mark-ups.
					# We are not counting sequences of five apostrophes.
					if ( strlen( $arr[$i] ) == 2 )      { $numitalics++;             }
					else if ( strlen( $arr[$i] ) == 3 ) { $numbold++;                }
					else if ( strlen( $arr[$i] ) == 5 ) { $numitalics++; $numbold++; }
				}
				$i++;
			}

			# If there is an odd number of both bold and italics, it is likely
			# that one of the bold ones was meant to be an apostrophe followed
			# by italics. Which one we cannot know for certain, but it is more
			# likely to be one that has a single-letter word before it.
			if ( ( $numbold % 2 == 1 ) && ( $numitalics % 2 == 1 ) )
			{
				$i = 0;
				$firstsingleletterword = -1;
				$firstmultiletterword = -1;
				$firstspace = -1;
				foreach ( $arr as $r )
				{
					if ( ( $i % 2 == 1 ) and ( strlen( $r ) == 3 ) )
					{
						$x1 = substr ($arr[$i-1], -1);
						$x2 = substr ($arr[$i-1], -2, 1);
						if ($x1 == ' ') {
							if ($firstspace == -1) $firstspace = $i;
						} else if ($x2 == ' ') {
							if ($firstsingleletterword == -1) $firstsingleletterword = $i;
						} else {
							if ($firstmultiletterword == -1) $firstmultiletterword = $i;
						}
					}
					$i++;
				}

				# If there is a single-letter word, use it!
				if ($firstsingleletterword > -1)
				{
					$arr [ $firstsingleletterword ] = "''";
					$arr [ $firstsingleletterword-1 ] .= "'";
				}
				# If not, but there's a multi-letter word, use that one.
				else if ($firstmultiletterword > -1)
				{
					$arr [ $firstmultiletterword ] = "''";
					$arr [ $firstmultiletterword-1 ] .= "'";
				}
				# ... otherwise use the first one that has neither.
				# (notice that it is possible for all three to be -1 if, for example,
				# there is only one pentuple-apostrophe in the line)
				else if ($firstspace > -1)
				{
					$arr [ $firstspace ] = "''";
					$arr [ $firstspace-1 ] .= "'";
				}
			}

			# Now let's actually convert our apostrophic mush to HTML!
			$output = '';
			$buffer = '';
			$state = '';
			$i = 0;
			foreach ($arr as $r)
			{
				if (($i % 2) == 0)
				{
					if ($state == 'both')
						$buffer .= $r;
					else
						$output .= $r;
				}
				else
				{
					if (strlen ($r) == 2)
					{
						if ($state == 'i')
						{ $output .= '</i>'; $state = ''; }
						else if ($state == 'bi')
						{ $output .= '</i>'; $state = 'b'; }
						else if ($state == 'ib')
						{ $output .= '</b></i><b>'; $state = 'b'; }
						else if ($state == 'both')
						{ $output .= '<b><i>'.$buffer.'</i>'; $state = 'b'; }
						else # $state can be 'b' or ''
						{ $output .= '<i>'; $state .= 'i'; }
					}
					else if (strlen ($r) == 3)
					{
						if ($state == 'b')
						{ $output .= '</b>'; $state = ''; }
						else if ($state == 'bi')
						{ $output .= '</i></b><i>'; $state = 'i'; }
						else if ($state == 'ib')
						{ $output .= '</b>'; $state = 'i'; }
						else if ($state == 'both')
						{ $output .= '<i><b>'.$buffer.'</b>'; $state = 'i'; }
						else # $state can be 'i' or ''
						{ $output .= '<b>'; $state .= 'b'; }
					}
					else if (strlen ($r) == 5)
					{
						if ($state == 'b')
						{ $output .= '</b><i>'; $state = 'i'; }
						else if ($state == 'i')
						{ $output .= '</i><b>'; $state = 'b'; }
						else if ($state == 'bi')
						{ $output .= '</i></b>'; $state = ''; }
						else if ($state == 'ib')
						{ $output .= '</b></i>'; $state = ''; }
						else if ($state == 'both')
						{ $output .= '<i><b>'.$buffer.'</b></i>'; $state = ''; }
						else # ($state == '')
						{ $buffer = ''; $state = 'both'; }
					}
				}
				$i++;
			}
			# Now close all remaining tags.  Notice that the order is important.
			if ($state == 'b' || $state == 'ib')
				$output .= '</b>';
			if ($state == 'i' || $state == 'bi' || $state == 'ib')
				$output .= '</i>';
			if ($state == 'bi')
				$output .= '</b>';
			# There might be lonely ''''', so make sure we have a buffer
			if ($state == 'both' && $buffer)
				$output .= '<b><i>'.$buffer.'</i></b>';
			return $output;
		}
	}

	/**
	 * Replace external links
	 *
 	 * Note: this is all very hackish and the order of execution matters a lot.
	 * Make sure to run maintenance/parserTests.php if you change this code.
	 *
	 * @private
	 */
	function replaceExternalLinks( $text ) {
		global $wgContLang;
		$fname = 'Parser::replaceExternalLinks';
		wfProfileIn( $fname );

		$sk = $this->mOptions->getSkin();

		$bits = preg_split( $this->mExtLinkBracketedRegex, $text, -1, PREG_SPLIT_DELIM_CAPTURE );

		$s = $this->replaceFreeExternalLinks( array_shift( $bits ) );

		$i = 0;
		while ( $i<count( $bits ) ) {
			$url = $bits[$i++];
			$protocol = $bits[$i++];
			$text = $bits[$i++];
			$trail = $bits[$i++];

			# The characters '<' and '>' (which were escaped by
			# removeHTMLtags()) should not be included in
			# URLs, per RFC 2396.
			$m2 = array();
			if (preg_match('/&(lt|gt);/', $url, $m2, PREG_OFFSET_CAPTURE)) {
				$text = substr($url, $m2[0][1]) . ' ' . $text;
				$url = substr($url, 0, $m2[0][1]);
			}

			# If the link text is an image URL, replace it with an <img> tag
			# This happened by accident in the original parser, but some people used it extensively
			$img = $this->maybeMakeExternalImage( $text );
			if ( $img !== false ) {
				$text = $img;
			}

			$dtrail = '';

			# Set linktype for CSS - if URL==text, link is essentially free
			$linktype = ($text == $url) ? 'free' : 'text';

			# No link text, e.g. [http://domain.tld/some.link]
			if ( $text == '' ) {
				# Autonumber if allowed. See bug #5918
				if ( strpos( wfUrlProtocols(), substr($protocol, 0, strpos($protocol, ':')) ) !== false ) {
					$text = '[' . ++$this->mAutonumber . ']';
					$linktype = 'autonumber';
				} else {
					# Otherwise just use the URL
					$text = htmlspecialchars( $url );
					$linktype = 'free';
				}
			} else {
				# Have link text, e.g. [http://domain.tld/some.link text]s
				# Check for trail
				list( $dtrail, $trail ) = Linker::splitTrail( $trail );
			}

			$text = $wgContLang->markNoConversion($text);

			$url = Sanitizer::cleanUrl( $url );

			# Process the trail (i.e. everything after this link up until start of the next link),
			# replacing any non-bracketed links
			$trail = $this->replaceFreeExternalLinks( $trail );

			# Use the encoded URL
			# This means that users can paste URLs directly into the text
			# Funny characters like &ouml; aren't valid in URLs anyway
			# This was changed in August 2004
			$s .= $sk->makeExternalLink( $url, $text, false, $linktype, $this->mTitle->getNamespace() ) . $dtrail . $trail;

			# Register link in the output object.
			# Replace unnecessary URL escape codes with the referenced character
			# This prevents spammers from hiding links from the filters
			$pasteurized = Parser::replaceUnusualEscapes( $url );
			$this->mOutput->addExternalLink( $pasteurized );
		}

		wfProfileOut( $fname );
		return $s;
	}

	/**
	 * Replace anything that looks like a URL with a link
	 * @private
	 */
	function replaceFreeExternalLinks( $text ) {
		global $wgContLang;
		$fname = 'Parser::replaceFreeExternalLinks';
		wfProfileIn( $fname );

		$bits = preg_split( '/(\b(?:' . wfUrlProtocols() . '))/S', $text, -1, PREG_SPLIT_DELIM_CAPTURE );
		$s = array_shift( $bits );
		$i = 0;

		$sk = $this->mOptions->getSkin();

		while ( $i < count( $bits ) ){
			$protocol = $bits[$i++];
			$remainder = $bits[$i++];

			$m = array();
			if ( preg_match( '/^('.self::EXT_LINK_URL_CLASS.'+)(.*)$/s', $remainder, $m ) ) {
				# Found some characters after the protocol that look promising
				$url = $protocol . $m[1];
				$trail = $m[2];

				# special case: handle urls as url args:
				# http://www.example.com/foo?=http://www.example.com/bar
				if(strlen($trail) == 0 &&
					isset($bits[$i]) &&
					preg_match('/^'. wfUrlProtocols() . '$/S', $bits[$i]) &&
					preg_match( '/^('.self::EXT_LINK_URL_CLASS.'+)(.*)$/s', $bits[$i + 1], $m ))
				{
					# add protocol, arg
					$url .= $bits[$i] . $m[1]; # protocol, url as arg to previous link
					$i += 2;
					$trail = $m[2];
				}

				# The characters '<' and '>' (which were escaped by
				# removeHTMLtags()) should not be included in
				# URLs, per RFC 2396.
				$m2 = array();
				if (preg_match('/&(lt|gt);/', $url, $m2, PREG_OFFSET_CAPTURE)) {
					$trail = substr($url, $m2[0][1]) . $trail;
					$url = substr($url, 0, $m2[0][1]);
				}

				# Move trailing punctuation to $trail
				$sep = ',;\.:!?';
				# If there is no left bracket, then consider right brackets fair game too
				if ( strpos( $url, '(' ) === false ) {
					$sep .= ')';
				}

				$numSepChars = strspn( strrev( $url ), $sep );
				if ( $numSepChars ) {
					$trail = substr( $url, -$numSepChars ) . $trail;
					$url = substr( $url, 0, -$numSepChars );
				}

				$url = Sanitizer::cleanUrl( $url );

				# Is this an external image?
				$text = $this->maybeMakeExternalImage( $url );
				if ( $text === false ) {
					# Not an image, make a link
					$text = $sk->makeExternalLink( $url, $wgContLang->markNoConversion($url), true, 'free', $this->mTitle->getNamespace() );
					# Register it in the output object...
					# Replace unnecessary URL escape codes with their equivalent characters
					$pasteurized = Parser::replaceUnusualEscapes( $url );
					$this->mOutput->addExternalLink( $pasteurized );
				}
				$s .= $text . $trail;
			} else {
				$s .= $protocol . $remainder;
			}
		}
		wfProfileOut( $fname );
		return $s;
	}

	/**
	 * Replace unusual URL escape codes with their equivalent characters
	 * @param string
	 * @return string
	 * @static
	 * @todo  This can merge genuinely required bits in the path or query string,
	 *        breaking legit URLs. A proper fix would treat the various parts of
	 *        the URL differently; as a workaround, just use the output for
	 *        statistical records, not for actual linking/output.
	 */
	static function replaceUnusualEscapes( $url ) {
		return preg_replace_callback( '/%[0-9A-Fa-f]{2}/',
			array( 'Parser', 'replaceUnusualEscapesCallback' ), $url );
	}

	/**
	 * Callback function used in replaceUnusualEscapes().
	 * Replaces unusual URL escape codes with their equivalent character
	 * @static
	 * @private
	 */
	private static function replaceUnusualEscapesCallback( $matches ) {
		$char = urldecode( $matches[0] );
		$ord = ord( $char );
		// Is it an unsafe or HTTP reserved character according to RFC 1738?
		if ( $ord > 32 && $ord < 127 && strpos( '<>"#{}|\^~[]`;/?', $char ) === false ) {
			// No, shouldn't be escaped
			return $char;
		} else {
			// Yes, leave it escaped
			return $matches[0];
		}
	}

	/**
	 * make an image if it's allowed, either through the global
	 * option or through the exception
	 * @private
	 */
	function maybeMakeExternalImage( $url ) {
		$sk = $this->mOptions->getSkin();
		$imagesfrom = $this->mOptions->getAllowExternalImagesFrom();
		$imagesexception = !empty($imagesfrom);
		$text = false;
		if ( $this->mOptions->getAllowExternalImages()
		     || ( $imagesexception && strpos( $url, $imagesfrom ) === 0 ) ) {
			if ( preg_match( self::EXT_IMAGE_REGEX, $url ) ) {
				# Image found
				$text = $sk->makeExternalImage( $url );
			}
		}
		return $text;
	}

	/**
	 * Process [[ ]] wikilinks
	 *
	 * @private
	 */
	function replaceInternalLinks( $s ) {
		global $wgContLang;
		static $fname = 'Parser::replaceInternalLinks' ;

		wfProfileIn( $fname );

		wfProfileIn( $fname.'-setup' );
		static $tc = FALSE;
		# the % is needed to support urlencoded titles as well
		if ( !$tc ) { $tc = Title::legalChars() . '#%'; }

		$sk = $this->mOptions->getSkin();

		#split the entire text string on occurences of [[
		$a = explode( '[[', ' ' . $s );
		#get the first element (all text up to first [[), and remove the space we added
		$s = array_shift( $a );
		$s = substr( $s, 1 );

		# Match a link having the form [[namespace:link|alternate]]trail
		static $e1 = FALSE;
		if ( !$e1 ) { $e1 = "/^([{$tc}]+)(?:\\|(.+?))?]](.*)\$/sD"; }
		# Match cases where there is no "]]", which might still be images
		static $e1_img = FALSE;
		if ( !$e1_img ) { $e1_img = "/^([{$tc}]+)\\|(.*)\$/sD"; }

		$useLinkPrefixExtension = $wgContLang->linkPrefixExtension();
		$e2 = null;
		if ( $useLinkPrefixExtension ) {
			# Match the end of a line for a word that's not followed by whitespace,
			# e.g. in the case of 'The Arab al[[Razi]]', 'al' will be matched
			$e2 = wfMsgForContent( 'linkprefix' );
		}

		if( is_null( $this->mTitle ) ) {
			wfProfileOut( $fname );
			wfProfileOut( $fname.'-setup' );
			throw new MWException( __METHOD__.": \$this->mTitle is null\n" );
		}
		$nottalk = !$this->mTitle->isTalkPage();

		if ( $useLinkPrefixExtension ) {
			$m = array();
			if ( preg_match( $e2, $s, $m ) ) {
				$first_prefix = $m[2];
			} else {
				$first_prefix = false;
			}
		} else {
			$prefix = '';
		}

		if($wgContLang->hasVariants()) {
			$selflink = $wgContLang->convertLinkToAllVariants($this->mTitle->getPrefixedText());
		} else {
			$selflink = array($this->mTitle->getPrefixedText());
		}
		$useSubpages = $this->areSubpagesAllowed();
		wfProfileOut( $fname.'-setup' );

		# Loop for each link
		for ($k = 0; isset( $a[$k] ); $k++) {
			$line = $a[$k];
			if ( $useLinkPrefixExtension ) {
				wfProfileIn( $fname.'-prefixhandling' );
				if ( preg_match( $e2, $s, $m ) ) {
					$prefix = $m[2];
					$s = $m[1];
				} else {
					$prefix='';
				}
				# first link
				if($first_prefix) {
					$prefix = $first_prefix;
					$first_prefix = false;
				}
				wfProfileOut( $fname.'-prefixhandling' );
			}

			$might_be_img = false;

			wfProfileIn( "$fname-e1" );
			if ( preg_match( $e1, $line, $m ) ) { # page with normal text or alt
				$text = $m[2];
				# If we get a ] at the beginning of $m[3] that means we have a link that's something like:
				# [[Image:Foo.jpg|[http://example.com desc]]] <- having three ] in a row fucks up,
				# the real problem is with the $e1 regex
				# See bug 1300.
				#
				# Still some problems for cases where the ] is meant to be outside punctuation,
				# and no image is in sight. See bug 2095.
				#
				if( $text !== '' &&
					substr( $m[3], 0, 1 ) === ']' &&
					strpos($text, '[') !== false
				)
				{
					$text .= ']'; # so that replaceExternalLinks($text) works later
					$m[3] = substr( $m[3], 1 );
				}
				# fix up urlencoded title texts
				if( strpos( $m[1], '%' ) !== false ) {
					# Should anchors '#' also be rejected?
					$m[1] = str_replace( array('<', '>'), array('&lt;', '&gt;'), urldecode($m[1]) );
				}
				$trail = $m[3];
			} elseif( preg_match($e1_img, $line, $m) ) { # Invalid, but might be an image with a link in its caption
				$might_be_img = true;
				$text = $m[2];
				if ( strpos( $m[1], '%' ) !== false ) {
					$m[1] = urldecode($m[1]);
				}
				$trail = "";
			} else { # Invalid form; output directly
				$s .= $prefix . '[[' . $line ;
				wfProfileOut( "$fname-e1" );
				continue;
			}
			wfProfileOut( "$fname-e1" );
			wfProfileIn( "$fname-misc" );

			# Don't allow internal links to pages containing
			# PROTO: where PROTO is a valid URL protocol; these
			# should be external links.
			if (preg_match('/^\b(?:' . wfUrlProtocols() . ')/', $m[1])) {
				$s .= $prefix . '[[' . $line ;
				wfProfileOut( "$fname-misc" );
				continue;
			}

			# Make subpage if necessary
			if( $useSubpages ) {
				$link = $this->maybeDoSubpageLink( $m[1], $text );
			} else {
				$link = $m[1];
			}

			$noforce = (substr($m[1], 0, 1) != ':');
			if (!$noforce) {
				# Strip off leading ':'
				$link = substr($link, 1);
			}

			wfProfileOut( "$fname-misc" );
			wfProfileIn( "$fname-title" );
			$nt = Title::newFromText( $this->mStripState->unstripNoWiki($link) );
			if( !$nt ) {
				$s .= $prefix . '[[' . $line;
				wfProfileOut( "$fname-title" );
				continue;
			}

			$ns = $nt->getNamespace();
			$iw = $nt->getInterWiki();
			wfProfileOut( "$fname-title" );

			if ($might_be_img) { # if this is actually an invalid link
				wfProfileIn( "$fname-might_be_img" );
				if ($ns == NS_IMAGE && $noforce) { #but might be an image
					$found = false;
					while (isset ($a[$k+1]) ) {
						#look at the next 'line' to see if we can close it there
						$spliced = array_splice( $a, $k + 1, 1 );
						$next_line = array_shift( $spliced );
						$m = explode( ']]', $next_line, 3 );
						if ( count( $m ) == 3 ) {
							# the first ]] closes the inner link, the second the image
							$found = true;
							$text .= "[[{$m[0]}]]{$m[1]}";
							$trail = $m[2];
							break;
						} elseif ( count( $m ) == 2 ) {
							#if there's exactly one ]] that's fine, we'll keep looking
							$text .= "[[{$m[0]}]]{$m[1]}";
						} else {
							#if $next_line is invalid too, we need look no further
							$text .= '[[' . $next_line;
							break;
						}
					}
					if ( !$found ) {
						# we couldn't find the end of this imageLink, so output it raw
						#but don't ignore what might be perfectly normal links in the text we've examined
						$text = $this->replaceInternalLinks($text);
						$s .= "{$prefix}[[$link|$text";
						# note: no $trail, because without an end, there *is* no trail
						wfProfileOut( "$fname-might_be_img" );
						continue;
					}
				} else { #it's not an image, so output it raw
					$s .= "{$prefix}[[$link|$text";
					# note: no $trail, because without an end, there *is* no trail
					wfProfileOut( "$fname-might_be_img" );
					continue;
				}
				wfProfileOut( "$fname-might_be_img" );
			}

			$wasblank = ( '' == $text );
			if( $wasblank ) $text = $link;

			# Link not escaped by : , create the various objects
			if( $noforce ) {

				# Interwikis
				wfProfileIn( "$fname-interwiki" );
				if( $iw && $this->mOptions->getInterwikiMagic() && $nottalk && $wgContLang->getLanguageName( $iw ) ) {
					$this->mOutput->addLanguageLink( $nt->getFullText() );
					$s = rtrim($s . $prefix);
					$s .= trim($trail, "\n") == '' ? '': $prefix . $trail;
					wfProfileOut( "$fname-interwiki" );
					continue;
				}
				wfProfileOut( "$fname-interwiki" );

				if ( $ns == NS_IMAGE ) {
					wfProfileIn( "$fname-image" );
					if ( !wfIsBadImage( $nt->getDBkey(), $this->mTitle ) ) {
						# recursively parse links inside the image caption
						# actually, this will parse them in any other parameters, too,
						# but it might be hard to fix that, and it doesn't matter ATM
						$text = $this->replaceExternalLinks($text);
						$text = $this->replaceInternalLinks($text);

						# cloak any absolute URLs inside the image markup, so replaceExternalLinks() won't touch them
						$s .= $prefix . $this->armorLinks( $this->makeImage( $nt, $text ) ) . $trail;
						$this->mOutput->addImage( $nt->getDBkey() );

						wfProfileOut( "$fname-image" );
						continue;
					} else {
						# We still need to record the image's presence on the page
						$this->mOutput->addImage( $nt->getDBkey() );
					}
					wfProfileOut( "$fname-image" );

				}

				if ( $ns == NS_CATEGORY ) {
					wfProfileIn( "$fname-category" );
					$s = rtrim($s . "\n"); # bug 87

					if ( $wasblank ) {
						$sortkey = $this->getDefaultSort();
					} else {
						$sortkey = $text;
					}
					$sortkey = Sanitizer::decodeCharReferences( $sortkey );
					$sortkey = str_replace( "\n", '', $sortkey );
					$sortkey = $wgContLang->convertCategoryKey( $sortkey );
					$this->mOutput->addCategory( $nt->getDBkey(), $sortkey );

					/**
					 * Strip the whitespace Category links produce, see bug 87
					 * @todo We might want to use trim($tmp, "\n") here.
					 */
					$s .= trim($prefix . $trail, "\n") == '' ? '': $prefix . $trail;

					wfProfileOut( "$fname-category" );
					continue;
				}
			}

			# Self-link checking
			if( $nt->getFragment() === '' ) {
				if( in_array( $nt->getPrefixedText(), $selflink, true ) ) {
					$s .= $prefix . $sk->makeSelfLinkObj( $nt, $text, '', $trail );
					continue;
				}
			}

			# Special and Media are pseudo-namespaces; no pages actually exist in them
			if( $ns == NS_MEDIA ) {
				# Give extensions a chance to select the file revision for us
				$skip = $time = false;
				wfRunHooks( 'BeforeParserMakeImageLinkObj', array( &$this, &$nt, &$skip, &$time ) );
				if ( $skip ) {
					$link = $sk->makeLinkObj( $nt );
				} else {
					$link = $sk->makeMediaLinkObj( $nt, $text, $time );
				}
				# Cloak with NOPARSE to avoid replacement in replaceExternalLinks
				$s .= $prefix . $this->armorLinks( $link ) . $trail;
				$this->mOutput->addImage( $nt->getDBkey() );
				continue;
			} elseif( $ns == NS_SPECIAL ) {
				if( SpecialPage::exists( $nt->getDBkey() ) ) {
					$s .= $this->makeKnownLinkHolder( $nt, $text, '', $trail, $prefix );
				} else {
					$s .= $this->makeLinkHolder( $nt, $text, '', $trail, $prefix );
				}
				continue;
			} elseif( $ns == NS_IMAGE ) {
				$img = wfFindFile( $nt );
				if( $img ) {
					// Force a blue link if the file exists; may be a remote
					// upload on the shared repository, and we want to see its
					// auto-generated page.
					$s .= $this->makeKnownLinkHolder( $nt, $text, '', $trail, $prefix );
					$this->mOutput->addLink( $nt );
					continue;
				}
			}
			$s .= $this->makeLinkHolder( $nt, $text, '', $trail, $prefix );
		}
		wfProfileOut( $fname );
		return $s;
	}

	/**
	 * Make a link placeholder. The text returned can be later resolved to a real link with
	 * replaceLinkHolders(). This is done for two reasons: firstly to avoid further
	 * parsing of interwiki links, and secondly to allow all existence checks and
	 * article length checks (for stub links) to be bundled into a single query.
	 *
	 */
	function makeLinkHolder( &$nt, $text = '', $query = '', $trail = '', $prefix = '' ) {
		wfProfileIn( __METHOD__ );
		if ( ! is_object($nt) ) {
			# Fail gracefully
			$retVal = "<!-- ERROR -->{$prefix}{$text}{$trail}";
		} else {
			# Separate the link trail from the rest of the link
			list( $inside, $trail ) = Linker::splitTrail( $trail );

			if ( $nt->isExternal() ) {
				$nr = array_push( $this->mInterwikiLinkHolders['texts'], $prefix.$text.$inside );
				$this->mInterwikiLinkHolders['titles'][] = $nt;
				$retVal = '<!--IWLINK '. ($nr-1) ."-->{$trail}";
			} else {
				$nr = array_push( $this->mLinkHolders['namespaces'], $nt->getNamespace() );
				$this->mLinkHolders['dbkeys'][] = $nt->getDBkey();
				$this->mLinkHolders['queries'][] = $query;
				$this->mLinkHolders['texts'][] = $prefix.$text.$inside;
				$this->mLinkHolders['titles'][] = $nt;

				$retVal = '<!--LINK '. ($nr-1) ."-->{$trail}";
			}
		}
		wfProfileOut( __METHOD__ );
		return $retVal;
	}

	/**
	 * Render a forced-blue link inline; protect against double expansion of
	 * URLs if we're in a mode that prepends full URL prefixes to internal links.
	 * Since this little disaster has to split off the trail text to avoid
	 * breaking URLs in the following text without breaking trails on the
	 * wiki links, it's been made into a horrible function.
	 *
	 * @param Title $nt
	 * @param string $text
	 * @param string $query
	 * @param string $trail
	 * @param string $prefix
	 * @return string HTML-wikitext mix oh yuck
	 */
	function makeKnownLinkHolder( $nt, $text = '', $query = '', $trail = '', $prefix = '' ) {
		list( $inside, $trail ) = Linker::splitTrail( $trail );
		$sk = $this->mOptions->getSkin();
		$link = $sk->makeKnownLinkObj( $nt, $text, $query, $inside, $prefix );
		return $this->armorLinks( $link ) . $trail;
	}

	/**
	 * Insert a NOPARSE hacky thing into any inline links in a chunk that's
	 * going to go through further parsing steps before inline URL expansion.
	 *
	 * In particular this is important when using action=render, which causes
	 * full URLs to be included.
	 *
	 * Oh man I hate our multi-layer parser!
	 *
	 * @param string more-or-less HTML
	 * @return string less-or-more HTML with NOPARSE bits
	 */
	function armorLinks( $text ) {
		return preg_replace( '/\b(' . wfUrlProtocols() . ')/',
			"{$this->mUniqPrefix}NOPARSE$1", $text );
	}

	/**
	 * Return true if subpage links should be expanded on this page.
	 * @return bool
	 */
	function areSubpagesAllowed() {
		# Some namespaces don't allow subpages
		return MWNamespace::hasSubpages( $this->mTitle->getNamespace() );
	}

	/**
	 * Handle link to subpage if necessary
	 * @param string $target the source of the link
	 * @param string &$text the link text, modified as necessary
	 * @return string the full name of the link
	 * @private
	 */
	function maybeDoSubpageLink($target, &$text) {
		# Valid link forms:
		# Foobar -- normal
		# :Foobar -- override special treatment of prefix (images, language links)
		# /Foobar -- convert to CurrentPage/Foobar
		# /Foobar/ -- convert to CurrentPage/Foobar, strip the initial / from text
		# ../ -- convert to CurrentPage, from CurrentPage/CurrentSubPage
		# ../Foobar -- convert to CurrentPage/Foobar, from CurrentPage/CurrentSubPage

		$fname = 'Parser::maybeDoSubpageLink';
		wfProfileIn( $fname );
		$ret = $target; # default return value is no change

		# Some namespaces don't allow subpages,
		# so only perform processing if subpages are allowed
		if( $this->areSubpagesAllowed() ) {
			$hash = strpos( $target, '#' );
			if( $hash !== false ) {
				$suffix = substr( $target, $hash );
				$target = substr( $target, 0, $hash );
			} else {
				$suffix = '';
			}
			# bug 7425
			$target = trim( $target );
			# Look at the first character
			if( $target != '' && $target{0} == '/' ) {
				# / at end means we don't want the slash to be shown
				$m = array();
				$trailingSlashes = preg_match_all( '%(/+)$%', $target, $m );
				if( $trailingSlashes ) {
					$noslash = $target = substr( $target, 1, -strlen($m[0][0]) );
				} else {
					$noslash = substr( $target, 1 );
				}

				$ret = $this->mTitle->getPrefixedText(). '/' . trim($noslash) . $suffix;
				if( '' === $text ) {
					$text = $target . $suffix;
				} # this might be changed for ugliness reasons
			} else {
				# check for .. subpage backlinks
				$dotdotcount = 0;
				$nodotdot = $target;
				while( strncmp( $nodotdot, "../", 3 ) == 0 ) {
					++$dotdotcount;
					$nodotdot = substr( $nodotdot, 3 );
				}
				if($dotdotcount > 0) {
					$exploded = explode( '/', $this->mTitle->GetPrefixedText() );
					if( count( $exploded ) > $dotdotcount ) { # not allowed to go below top level page
						$ret = implode( '/', array_slice( $exploded, 0, -$dotdotcount ) );
						# / at the end means don't show full path
						if( substr( $nodotdot, -1, 1 ) == '/' ) {
							$nodotdot = substr( $nodotdot, 0, -1 );
							if( '' === $text ) {
								$text = $nodotdot . $suffix;
							}
						}
						$nodotdot = trim( $nodotdot );
						if( $nodotdot != '' ) {
							$ret .= '/' . $nodotdot;
						}
						$ret .= $suffix;
					}
				}
			}
		}

		wfProfileOut( $fname );
		return $ret;
	}

	/**#@+
	 * Used by doBlockLevels()
	 * @private
	 */
	/* private */ function closeParagraph() {
		$result = '';
		if ( '' != $this->mLastSection ) {
			$result = '</' . $this->mLastSection  . ">\n";
		}
		$this->mInPre = false;
		$this->mLastSection = '';
		return $result;
	}
	# getCommon() returns the length of the longest common substring
	# of both arguments, starting at the beginning of both.
	#
	/* private */ function getCommon( $st1, $st2 ) {
		$fl = strlen( $st1 );
		$shorter = strlen( $st2 );
		if ( $fl < $shorter ) { $shorter = $fl; }

		for ( $i = 0; $i < $shorter; ++$i ) {
			if ( $st1{$i} != $st2{$i} ) { break; }
		}
		return $i;
	}
	# These next three functions open, continue, and close the list
	# element appropriate to the prefix character passed into them.
	#
	/* private */ function openList( $char ) {
		$result = $this->closeParagraph();

		if ( '*' == $char ) { $result .= '<ul><li>'; }
		else if ( '#' == $char ) { $result .= '<ol><li>'; }
		else if ( ':' == $char ) { $result .= '<dl><dd>'; }
		else if ( ';' == $char ) {
			$result .= '<dl><dt>';
			$this->mDTopen = true;
		}
		else { $result = '<!-- ERR 1 -->'; }

		return $result;
	}

	/* private */ function nextItem( $char ) {
		if ( '*' == $char || '#' == $char ) { return '</li><li>'; }
		else if ( ':' == $char || ';' == $char ) {
			$close = '</dd>';
			if ( $this->mDTopen ) { $close = '</dt>'; }
			if ( ';' == $char ) {
				$this->mDTopen = true;
				return $close . '<dt>';
			} else {
				$this->mDTopen = false;
				return $close . '<dd>';
			}
		}
		return '<!-- ERR 2 -->';
	}

	/* private */ function closeList( $char ) {
		if ( '*' == $char ) { $text = '</li></ul>'; }
		else if ( '#' == $char ) { $text = '</li></ol>'; }
		else if ( ':' == $char ) {
			if ( $this->mDTopen ) {
				$this->mDTopen = false;
				$text = '</dt></dl>';
			} else {
				$text = '</dd></dl>';
			}
		}
		else {	return '<!-- ERR 3 -->'; }
		return $text."\n";
	}
	/**#@-*/

	/**
	 * Make lists from lines starting with ':', '*', '#', etc.
	 *
	 * @private
	 * @return string the lists rendered as HTML
	 */
	function doBlockLevels( $text, $linestart ) {
		$fname = 'Parser::doBlockLevels';
		wfProfileIn( $fname );

		# Parsing through the text line by line.  The main thing
		# happening here is handling of block-level elements p, pre,
		# and making lists from lines starting with * # : etc.
		#
		$textLines = explode( "\n", $text );

		$lastPrefix = $output = '';
		$this->mDTopen = $inBlockElem = false;
		$prefixLength = 0;
		$paragraphStack = false;

		if ( !$linestart ) {
			$output .= array_shift( $textLines );
		}
		foreach ( $textLines as $oLine ) {
			$lastPrefixLength = strlen( $lastPrefix );
			$preCloseMatch = preg_match('/<\\/pre/i', $oLine );
			$preOpenMatch = preg_match('/<pre/i', $oLine );
			if ( !$this->mInPre ) {
				# Multiple prefixes may abut each other for nested lists.
				$prefixLength = strspn( $oLine, '*#:;' );
				$pref = substr( $oLine, 0, $prefixLength );

				# eh?
				$pref2 = str_replace( ';', ':', $pref );
				$t = substr( $oLine, $prefixLength );
				$this->mInPre = !empty($preOpenMatch);
			} else {
				# Don't interpret any other prefixes in preformatted text
				$prefixLength = 0;
				$pref = $pref2 = '';
				$t = $oLine;
			}

			# List generation
			if( $prefixLength && 0 == strcmp( $lastPrefix, $pref2 ) ) {
				# Same as the last item, so no need to deal with nesting or opening stuff
				$output .= $this->nextItem( substr( $pref, -1 ) );
				$paragraphStack = false;

				if ( substr( $pref, -1 ) == ';') {
					# The one nasty exception: definition lists work like this:
					# ; title : definition text
					# So we check for : in the remainder text to split up the
					# title and definition, without b0rking links.
					$term = $t2 = '';
					if ($this->findColonNoLinks($t, $term, $t2) !== false) {
						$t = $t2;
						$output .= $term . $this->nextItem( ':' );
					}
				}
			} elseif( $prefixLength || $lastPrefixLength ) {
				# Either open or close a level...
				$commonPrefixLength = $this->getCommon( $pref, $lastPrefix );
				$paragraphStack = false;

				while( $commonPrefixLength < $lastPrefixLength ) {
					$output .= $this->closeList( $lastPrefix{$lastPrefixLength-1} );
					--$lastPrefixLength;
				}
				if ( $prefixLength <= $commonPrefixLength && $commonPrefixLength > 0 ) {
					$output .= $this->nextItem( $pref{$commonPrefixLength-1} );
				}
				while ( $prefixLength > $commonPrefixLength ) {
					$char = substr( $pref, $commonPrefixLength, 1 );
					$output .= $this->openList( $char );

					if ( ';' == $char ) {
						# FIXME: This is dupe of code above
						if ($this->findColonNoLinks($t, $term, $t2) !== false) {
							$t = $t2;
							$output .= $term . $this->nextItem( ':' );
						}
					}
					++$commonPrefixLength;
				}
				$lastPrefix = $pref2;
			}
			if( 0 == $prefixLength ) {
				wfProfileIn( "$fname-paragraph" );
				# No prefix (not in list)--go to paragraph mode
				// XXX: use a stack for nestable elements like span, table and div
				$openmatch = preg_match('/(?:<table|<blockquote|<h1|<h2|<h3|<h4|<h5|<h6|<pre|<tr|<p|<ul|<ol|<li|<\\/tr|<\\/td|<\\/th)/iS', $t );
				$closematch = preg_match(
					'/(?:<\\/table|<\\/blockquote|<\\/h1|<\\/h2|<\\/h3|<\\/h4|<\\/h5|<\\/h6|'.
					'<td|<th|<\\/?div|<hr|<\\/pre|<\\/p|'.$this->mUniqPrefix.'-pre|<\\/li|<\\/ul|<\\/ol|<\\/?center)/iS', $t );
				if ( $openmatch or $closematch ) {
					$paragraphStack = false;
					# TODO bug 5718: paragraph closed
					$output .= $this->closeParagraph();
					if ( $preOpenMatch and !$preCloseMatch ) {
						$this->mInPre = true;
					}
					if ( $closematch ) {
						$inBlockElem = false;
					} else {
						$inBlockElem = true;
					}
				} else if ( !$inBlockElem && !$this->mInPre ) {
					if ( ' ' == $t{0} and ( $this->mLastSection == 'pre' or trim($t) != '' ) ) {
						// pre
						if ($this->mLastSection != 'pre') {
							$paragraphStack = false;
							$output .= $this->closeParagraph().'<pre>';
							$this->mLastSection = 'pre';
						}
						$t = substr( $t, 1 );
					} else {
						// paragraph
						if ( '' == trim($t) ) {
							if ( $paragraphStack ) {
								$output .= $paragraphStack.'<br />';
								$paragraphStack = false;
								$this->mLastSection = 'p';
							} else {
								if ($this->mLastSection != 'p' ) {
									$output .= $this->closeParagraph();
									$this->mLastSection = '';
									$paragraphStack = '<p>';
								} else {
									$paragraphStack = '</p><p>';
								}
							}
						} else {
							if ( $paragraphStack ) {
								$output .= $paragraphStack;
								$paragraphStack = false;
								$this->mLastSection = 'p';
							} else if ($this->mLastSection != 'p') {
								$output .= $this->closeParagraph().'<p>';
								$this->mLastSection = 'p';
							}
						}
					}
				}
				wfProfileOut( "$fname-paragraph" );
			}
			// somewhere above we forget to get out of pre block (bug 785)
			if($preCloseMatch && $this->mInPre) {
				$this->mInPre = false;
			}
			if ($paragraphStack === false) {
				$output .= $t."\n";
			}
		}
		while ( $prefixLength ) {
			$output .= $this->closeList( $pref2{$prefixLength-1} );
			--$prefixLength;
		}
		if ( '' != $this->mLastSection ) {
			$output .= '</' . $this->mLastSection . '>';
			$this->mLastSection = '';
		}

		wfProfileOut( $fname );
		return $output;
	}

	/**
	 * Split up a string on ':', ignoring any occurences inside tags
	 * to prevent illegal overlapping.
	 * @param string $str the string to split
	 * @param string &$before set to everything before the ':'
	 * @param string &$after set to everything after the ':'
	 * return string the position of the ':', or false if none found
	 */
	function findColonNoLinks($str, &$before, &$after) {
		$fname = 'Parser::findColonNoLinks';
		wfProfileIn( $fname );

		$pos = strpos( $str, ':' );
		if( $pos === false ) {
			// Nothing to find!
			wfProfileOut( $fname );
			return false;
		}

		$lt = strpos( $str, '<' );
		if( $lt === false || $lt > $pos ) {
			// Easy; no tag nesting to worry about
			$before = substr( $str, 0, $pos );
			$after = substr( $str, $pos+1 );
			wfProfileOut( $fname );
			return $pos;
		}

		// Ugly state machine to walk through avoiding tags.
		$state = self::COLON_STATE_TEXT;
		$stack = 0;
		$len = strlen( $str );
		for( $i = 0; $i < $len; $i++ ) {
			$c = $str{$i};

			switch( $state ) {
			// (Using the number is a performance hack for common cases)
			case 0: // self::COLON_STATE_TEXT:
				switch( $c ) {
				case "<":
					// Could be either a <start> tag or an </end> tag
					$state = self::COLON_STATE_TAGSTART;
					break;
				case ":":
					if( $stack == 0 ) {
						// We found it!
						$before = substr( $str, 0, $i );
						$after = substr( $str, $i + 1 );
						wfProfileOut( $fname );
						return $i;
					}
					// Embedded in a tag; don't break it.
					break;
				default:
					// Skip ahead looking for something interesting
					$colon = strpos( $str, ':', $i );
					if( $colon === false ) {
						// Nothing else interesting
						wfProfileOut( $fname );
						return false;
					}
					$lt = strpos( $str, '<', $i );
					if( $stack === 0 ) {
						if( $lt === false || $colon < $lt ) {
							// We found it!
							$before = substr( $str, 0, $colon );
							$after = substr( $str, $colon + 1 );
							wfProfileOut( $fname );
							return $i;
						}
					}
					if( $lt === false ) {
						// Nothing else interesting to find; abort!
						// We're nested, but there's no close tags left. Abort!
						break 2;
					}
					// Skip ahead to next tag start
					$i = $lt;
					$state = self::COLON_STATE_TAGSTART;
				}
				break;
			case 1: // self::COLON_STATE_TAG:
				// In a <tag>
				switch( $c ) {
				case ">":
					$stack++;
					$state = self::COLON_STATE_TEXT;
					break;
				case "/":
					// Slash may be followed by >?
					$state = self::COLON_STATE_TAGSLASH;
					break;
				default:
					// ignore
				}
				break;
			case 2: // self::COLON_STATE_TAGSTART:
				switch( $c ) {
				case "/":
					$state = self::COLON_STATE_CLOSETAG;
					break;
				case "!":
					$state = self::COLON_STATE_COMMENT;
					break;
				case ">":
					// Illegal early close? This shouldn't happen D:
					$state = self::COLON_STATE_TEXT;
					break;
				default:
					$state = self::COLON_STATE_TAG;
				}
				break;
			case 3: // self::COLON_STATE_CLOSETAG:
				// In a </tag>
				if( $c == ">" ) {
					$stack--;
					if( $stack < 0 ) {
						wfDebug( "Invalid input in $fname; too many close tags\n" );
						wfProfileOut( $fname );
						return false;
					}
					$state = self::COLON_STATE_TEXT;
				}
				break;
			case self::COLON_STATE_TAGSLASH:
				if( $c == ">" ) {
					// Yes, a self-closed tag <blah/>
					$state = self::COLON_STATE_TEXT;
				} else {
					// Probably we're jumping the gun, and this is an attribute
					$state = self::COLON_STATE_TAG;
				}
				break;
			case 5: // self::COLON_STATE_COMMENT:
				if( $c == "-" ) {
					$state = self::COLON_STATE_COMMENTDASH;
				}
				break;
			case self::COLON_STATE_COMMENTDASH:
				if( $c == "-" ) {
					$state = self::COLON_STATE_COMMENTDASHDASH;
				} else {
					$state = self::COLON_STATE_COMMENT;
				}
				break;
			case self::COLON_STATE_COMMENTDASHDASH:
				if( $c == ">" ) {
					$state = self::COLON_STATE_TEXT;
				} else {
					$state = self::COLON_STATE_COMMENT;
				}
				break;
			default:
				throw new MWException( "State machine error in $fname" );
			}
		}
		if( $stack > 0 ) {
			wfDebug( "Invalid input in $fname; not enough close tags (stack $stack, state $state)\n" );
			return false;
		}
		wfProfileOut( $fname );
		return false;
	}

	/**
	 * Return value of a magic variable (like PAGENAME)
	 *
	 * @private
	 */
	function getVariableValue( $index ) {
		global $wgContLang, $wgSitename, $wgServer, $wgServerName, $wgScriptPath;

		/**
		 * Some of these require message or data lookups and can be
		 * expensive to check many times.
		 */
		if ( wfRunHooks( 'ParserGetVariableValueVarCache', array( &$this, &$this->mVarCache ) ) ) {
			if ( isset( $this->mVarCache[$index] ) ) {
				return $this->mVarCache[$index];
			}
		}

		$ts = wfTimestamp( TS_UNIX, $this->mOptions->getTimestamp() );
		wfRunHooks( 'ParserGetVariableValueTs', array( &$this, &$ts ) );

		# Use the time zone
		global $wgLocaltimezone;
		if ( isset( $wgLocaltimezone ) ) {
			$oldtz = getenv( 'TZ' );
			putenv( 'TZ='.$wgLocaltimezone );
		}

		wfSuppressWarnings(); // E_STRICT system time bitching
		$localTimestamp = date( 'YmdHis', $ts );
		$localMonth = date( 'm', $ts );
		$localMonthName = date( 'n', $ts );
		$localDay = date( 'j', $ts );
		$localDay2 = date( 'd', $ts );
		$localDayOfWeek = date( 'w', $ts );
		$localWeek = date( 'W', $ts );
		$localYear = date( 'Y', $ts );
		$localHour = date( 'H', $ts );
		if ( isset( $wgLocaltimezone ) ) {
			putenv( 'TZ='.$oldtz );
		}
		wfRestoreWarnings();

		switch ( $index ) {
			case 'currentmonth':
				return $this->mVarCache[$index] = $wgContLang->formatNum( gmdate( 'm', $ts ) );
			case 'currentmonthname':
				return $this->mVarCache[$index] = $wgContLang->getMonthName( gmdate( 'n', $ts ) );
			case 'currentmonthnamegen':
				return $this->mVarCache[$index] = $wgContLang->getMonthNameGen( gmdate( 'n', $ts ) );
			case 'currentmonthabbrev':
				return $this->mVarCache[$index] = $wgContLang->getMonthAbbreviation( gmdate( 'n', $ts ) );
			case 'currentday':
				return $this->mVarCache[$index] = $wgContLang->formatNum( gmdate( 'j', $ts ) );
			case 'currentday2':
				return $this->mVarCache[$index] = $wgContLang->formatNum( gmdate( 'd', $ts ) );
			case 'localmonth':
				return $this->mVarCache[$index] = $wgContLang->formatNum( $localMonth );
			case 'localmonthname':
				return $this->mVarCache[$index] = $wgContLang->getMonthName( $localMonthName );
			case 'localmonthnamegen':
				return $this->mVarCache[$index] = $wgContLang->getMonthNameGen( $localMonthName );
			case 'localmonthabbrev':
				return $this->mVarCache[$index] = $wgContLang->getMonthAbbreviation( $localMonthName );
			case 'localday':
				return $this->mVarCache[$index] = $wgContLang->formatNum( $localDay );
			case 'localday2':
				return $this->mVarCache[$index] = $wgContLang->formatNum( $localDay2 );
			case 'pagename':
				return wfEscapeWikiText( $this->mTitle->getText() );
			case 'pagenamee':
				return $this->mTitle->getPartialURL();
			case 'fullpagename':
				return wfEscapeWikiText( $this->mTitle->getPrefixedText() );
			case 'fullpagenamee':
				return $this->mTitle->getPrefixedURL();
			case 'subpagename':
				return wfEscapeWikiText( $this->mTitle->getSubpageText() );
			case 'subpagenamee':
				return $this->mTitle->getSubpageUrlForm();
			case 'basepagename':
				return wfEscapeWikiText( $this->mTitle->getBaseText() );
			case 'basepagenamee':
				return wfUrlEncode( str_replace( ' ', '_', $this->mTitle->getBaseText() ) );
			case 'talkpagename':
				if( $this->mTitle->canTalk() ) {
					$talkPage = $this->mTitle->getTalkPage();
					return wfEscapeWikiText( $talkPage->getPrefixedText() );
				} else {
					return '';
				}
			case 'talkpagenamee':
				if( $this->mTitle->canTalk() ) {
					$talkPage = $this->mTitle->getTalkPage();
					return $talkPage->getPrefixedUrl();
				} else {
					return '';
				}
			case 'subjectpagename':
				$subjPage = $this->mTitle->getSubjectPage();
				return wfEscapeWikiText( $subjPage->getPrefixedText() );
			case 'subjectpagenamee':
				$subjPage = $this->mTitle->getSubjectPage();
				return $subjPage->getPrefixedUrl();
			case 'revisionid':
				// Let the edit saving system know we should parse the page
				// *after* a revision ID has been assigned.
				$this->mOutput->setFlag( 'vary-revision' );
				wfDebug( __METHOD__ . ": {{REVISIONID}} used, setting vary-revision...\n" );
				return $this->mRevisionId;
			case 'revisionday':
				// Let the edit saving system know we should parse the page
				// *after* a revision ID has been assigned. This is for null edits.
				$this->mOutput->setFlag( 'vary-revision' );
				wfDebug( __METHOD__ . ": {{REVISIONDAY}} used, setting vary-revision...\n" );
				return intval( substr( $this->getRevisionTimestamp(), 6, 2 ) );
			case 'revisionday2':
				// Let the edit saving system know we should parse the page
				// *after* a revision ID has been assigned. This is for null edits.
				$this->mOutput->setFlag( 'vary-revision' );
				wfDebug( __METHOD__ . ": {{REVISIONDAY2}} used, setting vary-revision...\n" );
				return substr( $this->getRevisionTimestamp(), 6, 2 );
			case 'revisionmonth':
				// Let the edit saving system know we should parse the page
				// *after* a revision ID has been assigned. This is for null edits.
				$this->mOutput->setFlag( 'vary-revision' );
				wfDebug( __METHOD__ . ": {{REVISIONMONTH}} used, setting vary-revision...\n" );
				return intval( substr( $this->getRevisionTimestamp(), 4, 2 ) );
			case 'revisionyear':
				// Let the edit saving system know we should parse the page
				// *after* a revision ID has been assigned. This is for null edits.
				$this->mOutput->setFlag( 'vary-revision' );
				wfDebug( __METHOD__ . ": {{REVISIONYEAR}} used, setting vary-revision...\n" );
				return substr( $this->getRevisionTimestamp(), 0, 4 );
			case 'revisiontimestamp':
				// Let the edit saving system know we should parse the page
				// *after* a revision ID has been assigned. This is for null edits.
				$this->mOutput->setFlag( 'vary-revision' );
				wfDebug( __METHOD__ . ": {{REVISIONTIMESTAMP}} used, setting vary-revision...\n" );
				return $this->getRevisionTimestamp();
			case 'namespace':
				return str_replace('_',' ',$wgContLang->getNsText( $this->mTitle->getNamespace() ) );
			case 'namespacee':
				return wfUrlencode( $wgContLang->getNsText( $this->mTitle->getNamespace() ) );
			case 'talkspace':
				return $this->mTitle->canTalk() ? str_replace('_',' ',$this->mTitle->getTalkNsText()) : '';
			case 'talkspacee':
				return $this->mTitle->canTalk() ? wfUrlencode( $this->mTitle->getTalkNsText() ) : '';
			case 'subjectspace':
				return $this->mTitle->getSubjectNsText();
			case 'subjectspacee':
				return( wfUrlencode( $this->mTitle->getSubjectNsText() ) );
			case 'currentdayname':
				return $this->mVarCache[$index] = $wgContLang->getWeekdayName( gmdate( 'w', $ts ) + 1 );
			case 'currentyear':
				return $this->mVarCache[$index] = $wgContLang->formatNum( gmdate( 'Y', $ts ), true );
			case 'currenttime':
				return $this->mVarCache[$index] = $wgContLang->time( wfTimestamp( TS_MW, $ts ), false, false );
			case 'currenthour':
				return $this->mVarCache[$index] = $wgContLang->formatNum( gmdate( 'H', $ts ), true );
			case 'currentweek':
				// @bug 4594 PHP5 has it zero padded, PHP4 does not, cast to
				// int to remove the padding
				return $this->mVarCache[$index] = $wgContLang->formatNum( (int)gmdate( 'W', $ts ) );
			case 'currentdow':
				return $this->mVarCache[$index] = $wgContLang->formatNum( gmdate( 'w', $ts ) );
			case 'localdayname':
				return $this->mVarCache[$index] = $wgContLang->getWeekdayName( $localDayOfWeek + 1 );
			case 'localyear':
				return $this->mVarCache[$index] = $wgContLang->formatNum( $localYear, true );
			case 'localtime':
				return $this->mVarCache[$index] = $wgContLang->time( $localTimestamp, false, false );
			case 'localhour':
				return $this->mVarCache[$index] = $wgContLang->formatNum( $localHour, true );
			case 'localweek':
				// @bug 4594 PHP5 has it zero padded, PHP4 does not, cast to
				// int to remove the padding
				return $this->mVarCache[$index] = $wgContLang->formatNum( (int)$localWeek );
			case 'localdow':
				return $this->mVarCache[$index] = $wgContLang->formatNum( $localDayOfWeek );
			case 'numberofarticles':
				return $this->mVarCache[$index] = $wgContLang->formatNum( SiteStats::articles() );
			case 'numberoffiles':
				return $this->mVarCache[$index] = $wgContLang->formatNum( SiteStats::images() );
			case 'numberofusers':
				return $this->mVarCache[$index] = $wgContLang->formatNum( SiteStats::users() );
			case 'numberofpages':
				return $this->mVarCache[$index] = $wgContLang->formatNum( SiteStats::pages() );
			case 'numberofadmins':
				return $this->mVarCache[$index] = $wgContLang->formatNum( SiteStats::admins() );
			case 'numberofedits':
				return $this->mVarCache[$index] = $wgContLang->formatNum( SiteStats::edits() );
			case 'currenttimestamp':
				return $this->mVarCache[$index] = wfTimestamp( TS_MW, $ts );
			case 'localtimestamp':
				return $this->mVarCache[$index] = $localTimestamp;
			case 'currentversion':
				return $this->mVarCache[$index] = SpecialVersion::getVersion();
			case 'sitename':
				return $wgSitename;
			case 'server':
				return $wgServer;
			case 'servername':
				return $wgServerName;
			case 'scriptpath':
				return $wgScriptPath;
			case 'directionmark':
				return $wgContLang->getDirMark();
			case 'contentlanguage':
				global $wgContLanguageCode;
				return $wgContLanguageCode;
			default:
				$ret = null;
				if ( wfRunHooks( 'ParserGetVariableValueSwitch', array( &$this, &$this->mVarCache, &$index, &$ret ) ) )
					return $ret;
				else
					return null;
		}
	}

	/**
	 * initialise the magic variables (like CURRENTMONTHNAME)
	 *
	 * @private
	 */
	function initialiseVariables() {
		$fname = 'Parser::initialiseVariables';
		wfProfileIn( $fname );
		$variableIDs = MagicWord::getVariableIDs();

		$this->mVariables = new MagicWordArray( $variableIDs );
		wfProfileOut( $fname );
	}

	/**
	 * Preprocess some wikitext and return the document tree.
	 * This is the ghost of replace_variables().
	 *
	 * @param string $text The text to parse
	 * @param integer flags Bitwise combination of:
	 *          self::PTD_FOR_INCLUSION    Handle <noinclude>/<includeonly> as if the text is being
	 *                                     included. Default is to assume a direct page view.
	 *
	 * The generated DOM tree must depend only on the input text and the flags.
	 * The DOM tree must be the same in OT_HTML and OT_WIKI mode, to avoid a regression of bug 4899.
	 *
	 * Any flag added to the $flags parameter here, or any other parameter liable to cause a
	 * change in the DOM tree for a given text, must be passed through the section identifier
	 * in the section edit link and thus back to extractSections().
	 *
	 * The output of this function is currently only cached in process memory, but a persistent
	 * cache may be implemented at a later date which takes further advantage of these strict
	 * dependency requirements.
	 *
	 * @private
	 */
	function preprocessToDom ( $text, $flags = 0 ) {
		$dom = $this->getPreprocessor()->preprocessToObj( $text, $flags );
		return $dom;
	}

	/*
	 * Return a three-element array: leading whitespace, string contents, trailing whitespace
	 */
	public static function splitWhitespace( $s ) {
		$ltrimmed = ltrim( $s );
		$w1 = substr( $s, 0, strlen( $s ) - strlen( $ltrimmed ) );
		$trimmed = rtrim( $ltrimmed );
		$diff = strlen( $ltrimmed ) - strlen( $trimmed );
		if ( $diff > 0 ) {
			$w2 = substr( $ltrimmed, -$diff );
		} else {
			$w2 = '';
		}
		return array( $w1, $trimmed, $w2 );
	}

	/**
	 * Replace magic variables, templates, and template arguments
	 * with the appropriate text. Templates are substituted recursively,
	 * taking care to avoid infinite loops.
	 *
	 * Note that the substitution depends on value of $mOutputType:
	 *  self::OT_WIKI: only {{subst:}} templates
	 *  self::OT_PREPROCESS: templates but not extension tags
	 *  self::OT_HTML: all templates and extension tags
	 *
	 * @param string $tex The text to transform
	 * @param PPFrame $frame Object describing the arguments passed to the template. 
	 *        Arguments may also be provided as an associative array, as was the usual case before MW1.12.
	 *        Providing arguments this way may be useful for extensions wishing to perform variable replacement explicitly.
	 * @param bool $argsOnly Only do argument (triple-brace) expansion, not double-brace expansion
	 * @private
	 */
	function replaceVariables( $text, $frame = false, $argsOnly = false ) {
		# Prevent too big inclusions
		if( strlen( $text ) > $this->mOptions->getMaxIncludeSize() ) {
			return $text;
		}

		$fname = __METHOD__;
		wfProfileIn( $fname );

		if ( $frame === false ) {
			$frame = $this->getPreprocessor()->newFrame();
		} elseif ( !( $frame instanceof PPFrame ) ) {
			wfDebug( __METHOD__." called using plain parameters instead of a PPFrame instance. Creating custom frame.\n" );
			$frame = $this->getPreprocessor()->newCustomFrame($frame);
		}

		$dom = $this->preprocessToDom( $text );
		$flags = $argsOnly ? PPFrame::NO_TEMPLATES : 0;
		$text = $frame->expand( $dom, $flags );

		wfProfileOut( $fname );
		return $text;
	}

	/// Clean up argument array - refactored in 1.9 so parserfunctions can use it, too.
	static function createAssocArgs( $args ) {
		$assocArgs = array();
		$index = 1;
		foreach( $args as $arg ) {
			$eqpos = strpos( $arg, '=' );
			if ( $eqpos === false ) {
				$assocArgs[$index++] = $arg;
			} else {
				$name = trim( substr( $arg, 0, $eqpos ) );
				$value = trim( substr( $arg, $eqpos+1 ) );
				if ( $value === false ) {
					$value = '';
				}
				if ( $name !== false ) {
					$assocArgs[$name] = $value;
				}
			}
		}

		return $assocArgs;
	}

	/**
	 * Warn the user when a parser limitation is reached
	 * Will warn at most once the user per limitation type
	 *
	 * @param string $limitationType, should be one of:
	 *   'expensive-parserfunction' (corresponding messages: 'expensive-parserfunction-warning', 'expensive-parserfunction-category')
	 *   'post-expand-template-argument' (corresponding messages: 'post-expand-template-argument-warning', 'post-expand-template-argument-category')
	 *   'post-expand-template-inclusion' (corresponding messages: 'post-expand-template-inclusion-warning', 'post-expand-template-inclusion-category')
	 * @params int $current, $max When an explicit limit has been
	 *	 exceeded, provide the values (optional)
	 */
	function limitationWarn( $limitationType, $current=null, $max=null) {
		$msgName = $limitationType . '-warning';
		//does no harm if $current and $max are present but are unnecessary for the message
		$warning = wfMsg( $msgName, $current, $max); 
		$this->mOutput->addWarning( $warning );
		$cat = Title::makeTitleSafe( NS_CATEGORY, wfMsgForContent( $limitationType . '-category' ) );
		if ( $cat ) {
			$this->mOutput->addCategory( $cat->getDBkey(), $this->getDefaultSort() );
		}
	}

	/**
	 * Return the text of a template, after recursively
	 * replacing any variables or templates within the template.
	 *
	 * @param array $piece The parts of the template
	 *  $piece['title']: the title, i.e. the part before the |
	 *  $piece['parts']: the parameter array
	 *  $piece['lineStart']: whether the brace was at the start of a line
	 * @param PPFrame The current frame, contains template arguments
	 * @return string the text of the template
	 * @private
	 */
	function braceSubstitution( $piece, $frame ) {
		global $wgContLang, $wgLang, $wgAllowDisplayTitle, $wgNonincludableNamespaces;
		$fname = __METHOD__;
		wfProfileIn( $fname );
		wfProfileIn( __METHOD__.'-setup' );

		# Flags
		$found = false;             # $text has been filled
		$nowiki = false;            # wiki markup in $text should be escaped
		$isHTML = false;            # $text is HTML, armour it against wikitext transformation
		$forceRawInterwiki = false; # Force interwiki transclusion to be done in raw mode not rendered
		$isChildObj = false;        # $text is a DOM node needing expansion in a child frame
		$isLocalObj = false;        # $text is a DOM node needing expansion in the current frame

		# Title object, where $text came from
		$title = NULL;

		# $part1 is the bit before the first |, and must contain only title characters.
		# Various prefixes will be stripped from it later.
		$titleWithSpaces = $frame->expand( $piece['title'] );
		$part1 = trim( $titleWithSpaces );
		$titleText = false;

		# Original title text preserved for various purposes
		$originalTitle = $part1;

		# $args is a list of argument nodes, starting from index 0, not including $part1
		$args = (null == $piece['parts']) ? array() : $piece['parts'];
		wfProfileOut( __METHOD__.'-setup' );

		# SUBST
		wfProfileIn( __METHOD__.'-modifiers' );
		if ( !$found ) {
			$mwSubst = MagicWord::get( 'subst' );
			if ( $mwSubst->matchStartAndRemove( $part1 ) xor $this->ot['wiki'] ) {
				# One of two possibilities is true:
				# 1) Found SUBST but not in the PST phase
				# 2) Didn't find SUBST and in the PST phase
				# In either case, return without further processing
				$text = $frame->virtualBracketedImplode( '{{', '|', '}}', $titleWithSpaces, $args );
				$isLocalObj = true;
				$found = true;
			}
		}

		# Variables
		if ( !$found && $args->getLength() == 0 ) {
			$id = $this->mVariables->matchStartToEnd( $part1 );
			if ( $id !== false ) {
				$text = $this->getVariableValue( $id );
				if (MagicWord::getCacheTTL($id)>-1)
					$this->mOutput->mContainsOldMagic = true;
				$found = true;
			}
		}

		# MSG, MSGNW and RAW
		if ( !$found ) {
			# Check for MSGNW:
			$mwMsgnw = MagicWord::get( 'msgnw' );
			if ( $mwMsgnw->matchStartAndRemove( $part1 ) ) {
				$nowiki = true;
			} else {
				# Remove obsolete MSG:
				$mwMsg = MagicWord::get( 'msg' );
				$mwMsg->matchStartAndRemove( $part1 );
			}

			# Check for RAW:
			$mwRaw = MagicWord::get( 'raw' );
			if ( $mwRaw->matchStartAndRemove( $part1 ) ) {
				$forceRawInterwiki = true;
			}
		}
		wfProfileOut( __METHOD__.'-modifiers' );

		# Parser functions
		if ( !$found ) {
			wfProfileIn( __METHOD__ . '-pfunc' );

			$colonPos = strpos( $part1, ':' );
			if ( $colonPos !== false ) {
				# Case sensitive functions
				$function = substr( $part1, 0, $colonPos );
				if ( isset( $this->mFunctionSynonyms[1][$function] ) ) {
					$function = $this->mFunctionSynonyms[1][$function];
				} else {
					# Case insensitive functions
					$function = strtolower( $function );
					if ( isset( $this->mFunctionSynonyms[0][$function] ) ) {
						$function = $this->mFunctionSynonyms[0][$function];
					} else {
						$function = false;
					}
				}
				if ( $function ) {
					list( $callback, $flags ) = $this->mFunctionHooks[$function];
					$initialArgs = array( &$this );
					$funcArgs = array( trim( substr( $part1, $colonPos + 1 ) ) );
					if ( $flags & SFH_OBJECT_ARGS ) {
						# Add a frame parameter, and pass the arguments as an array
						$allArgs = $initialArgs;
						$allArgs[] = $frame;
						for ( $i = 0; $i < $args->getLength(); $i++ ) {
							$funcArgs[] = $args->item( $i );
						}
						$allArgs[] = $funcArgs;
					} else {
						# Convert arguments to plain text
						for ( $i = 0; $i < $args->getLength(); $i++ ) {
							$funcArgs[] = trim( $frame->expand( $args->item( $i ) ) );
						}
						$allArgs = array_merge( $initialArgs, $funcArgs );
					}

					# Workaround for PHP bug 35229 and similar
					if ( !is_callable( $callback ) ) {
						throw new MWException( "Tag hook for $name is not callable\n" );
					}
					$result = call_user_func_array( $callback, $allArgs );
					$found = true;
					$noparse = true;
					$preprocessFlags = 0;
					
					if ( is_array( $result ) ) {
						if ( isset( $result[0] ) ) {
							$text = $result[0];
							unset( $result[0] );
						}

						// Extract flags into the local scope
						// This allows callers to set flags such as nowiki, found, etc.
						extract( $result );
					} else {
						$text = $result;
					}
					if ( !$noparse ) {
						$text = $this->preprocessToDom( $text, $preprocessFlags );
						$isChildObj = true;
					}
				}
			}
			wfProfileOut( __METHOD__ . '-pfunc' );
		}

		# Finish mangling title and then check for loops.
		# Set $title to a Title object and $titleText to the PDBK
		if ( !$found ) {
			$ns = NS_TEMPLATE;
			# Split the title into page and subpage
			$subpage = '';
			$part1 = $this->maybeDoSubpageLink( $part1, $subpage );
			if ($subpage !== '') {
				$ns = $this->mTitle->getNamespace();
			}
			$title = Title::newFromText( $part1, $ns );
			if ( $title ) {
				$titleText = $title->getPrefixedText();
				# Check for language variants if the template is not found
				if($wgContLang->hasVariants() && $title->getArticleID() == 0){
					$wgContLang->findVariantLink($part1, $title);
				}
				# Do infinite loop check
				if ( !$frame->loopCheck( $title ) ) {
					$found = true;
					$text = "<span class=\"error\">Template loop detected: [[$titleText]]</span>";
					wfDebug( __METHOD__.": template loop broken at '$titleText'\n" );
				}
				# Do recursion depth check
				$limit = $this->mOptions->getMaxTemplateDepth();
				if ( $frame->depth >= $limit ) {
					$found = true;
					$text = "<span class=\"error\">Template recursion depth limit exceeded ($limit)</span>";
				}
			}
		}

		# Load from database
		if ( !$found && $title ) {
			wfProfileIn( __METHOD__ . '-loadtpl' );
			if ( !$title->isExternal() ) {
				if ( $title->getNamespace() == NS_SPECIAL && $this->mOptions->getAllowSpecialInclusion() && $this->ot['html'] ) {
					$text = SpecialPage::capturePath( $title );
					if ( is_string( $text ) ) {
						$found = true;
						$isHTML = true;
						$this->disableCache();
					}
				} else if ( $wgNonincludableNamespaces && in_array( $title->getNamespace(), $wgNonincludableNamespaces ) ) {
					$found = false; //access denied
					wfDebug( "$fname: template inclusion denied for " . $title->getPrefixedDBkey() );
				} else {
					list( $text, $title ) = $this->getTemplateDom( $title );
					if ( $text !== false ) {
						$found = true;
						$isChildObj = true;
					}
				}

				# If the title is valid but undisplayable, make a link to it
				if ( !$found && ( $this->ot['html'] || $this->ot['pre'] ) ) {
					$text = "[[:$titleText]]";
					$found = true;
				}
			} elseif ( $title->isTrans() ) {
				// Interwiki transclusion
				if ( $this->ot['html'] && !$forceRawInterwiki ) {
					$text = $this->interwikiTransclude( $title, 'render' );
					$isHTML = true;
				} else {
					$text = $this->interwikiTransclude( $title, 'raw' );
					// Preprocess it like a template
					$text = $this->preprocessToDom( $text, self::PTD_FOR_INCLUSION );
					$isChildObj = true;
				}
				$found = true;
			}
			wfProfileOut( __METHOD__ . '-loadtpl' );
		}

		# If we haven't found text to substitute by now, we're done
		# Recover the source wikitext and return it
		if ( !$found ) {
			$text = $frame->virtualBracketedImplode( '{{', '|', '}}', $titleWithSpaces, $args );
			wfProfileOut( $fname );
			return array( 'object' => $text );
		}

		# Expand DOM-style return values in a child frame
		if ( $isChildObj ) {
			# Clean up argument array
			$newFrame = $frame->newChild( $args, $title );

			if ( $nowiki ) {
				$text = $newFrame->expand( $text, PPFrame::RECOVER_ORIG );
			} elseif ( $titleText !== false && $newFrame->isEmpty() ) {
				# Expansion is eligible for the empty-frame cache
				if ( isset( $this->mTplExpandCache[$titleText] ) ) {
					$text = $this->mTplExpandCache[$titleText];
				} else {
					$text = $newFrame->expand( $text );
					$this->mTplExpandCache[$titleText] = $text;
				}
			} else {
				# Uncached expansion
				$text = $newFrame->expand( $text );
			}
		}
		if ( $isLocalObj && $nowiki ) {
			$text = $frame->expand( $text, PPFrame::RECOVER_ORIG );
			$isLocalObj = false;
		}

		# Replace raw HTML by a placeholder
		# Add a blank line preceding, to prevent it from mucking up
		# immediately preceding headings
		if ( $isHTML ) {
			$text = "\n\n" . $this->insertStripItem( $text );
		}
		# Escape nowiki-style return values
		elseif ( $nowiki && ( $this->ot['html'] || $this->ot['pre'] ) ) {
			$text = wfEscapeWikiText( $text );
		}
		# Bug 529: if the template begins with a table or block-level
		# element, it should be treated as beginning a new line.
		# This behaviour is somewhat controversial.
		elseif ( is_string( $text ) && !$piece['lineStart'] && preg_match('/^(?:{\\||:|;|#|\*)/', $text)) /*}*/{
			$text = "\n" . $text;
		}

		if ( is_string( $text ) && !$this->incrementIncludeSize( 'post-expand', strlen( $text ) ) ) {
			# Error, oversize inclusion
			$text = "[[$originalTitle]]" .
				$this->insertStripItem( '<!-- WARNING: template omitted, post-expand include size too large -->' );
			$this->limitationWarn( 'post-expand-template-inclusion' );
		}

		if ( $isLocalObj ) {
			$ret = array( 'object' => $text );
		} else {
			$ret = array( 'text' => $text );
		}

		wfProfileOut( $fname );
		return $ret;
	}

	/**
	 * Get the semi-parsed DOM representation of a template with a given title,
	 * and its redirect destination title. Cached.
	 */
	function getTemplateDom( $title ) {
		$cacheTitle = $title;
		$titleText = $title->getPrefixedDBkey();

		if ( isset( $this->mTplRedirCache[$titleText] ) ) {
			list( $ns, $dbk ) = $this->mTplRedirCache[$titleText];
			$title = Title::makeTitle( $ns, $dbk );
			$titleText = $title->getPrefixedDBkey();
		}
		if ( isset( $this->mTplDomCache[$titleText] ) ) {
			return array( $this->mTplDomCache[$titleText], $title );
		}

		// Cache miss, go to the database
		list( $text, $title ) = $this->fetchTemplateAndTitle( $title );

		if ( $text === false ) {
			$this->mTplDomCache[$titleText] = false;
			return array( false, $title );
		}

		$dom = $this->preprocessToDom( $text, self::PTD_FOR_INCLUSION );
		$this->mTplDomCache[ $titleText ] = $dom;

		if (! $title->equals($cacheTitle)) {
			$this->mTplRedirCache[$cacheTitle->getPrefixedDBkey()] =
				array( $title->getNamespace(),$cdb = $title->getDBkey() );
		}

		return array( $dom, $title );
	}

	/**
	 * Fetch the unparsed text of a template and register a reference to it.
	 */
	function fetchTemplateAndTitle( $title ) {
		$templateCb = $this->mOptions->getTemplateCallback();
		$stuff = call_user_func( $templateCb, $title, $this );
		$text = $stuff['text'];
		$finalTitle = isset( $stuff['finalTitle'] ) ? $stuff['finalTitle'] : $title;
		if ( isset( $stuff['deps'] ) ) {
			foreach ( $stuff['deps'] as $dep ) {
				$this->mOutput->addTemplate( $dep['title'], $dep['page_id'], $dep['rev_id'] );
			}
		}
		return array($text,$finalTitle);
	}

	function fetchTemplate( $title ) {
		$rv = $this->fetchTemplateAndTitle($title);
		return $rv[0];
	}

	/**
	 * Static function to get a template
	 * Can be overridden via ParserOptions::setTemplateCallback().
	 */
	static function statelessFetchTemplate( $title, $parser=false ) {
		$text = $skip = false;
		$finalTitle = $title;
		$deps = array();

		// Loop to fetch the article, with up to 1 redirect
		for ( $i = 0; $i < 2 && is_object( $title ); $i++ ) {
			# Give extensions a chance to select the revision instead
			$id = false; // Assume current
			wfRunHooks( 'BeforeParserFetchTemplateAndtitle', array( $parser, &$title, &$skip, &$id ) );

			if( $skip ) {
				$text = false;
				$deps[] = array(
					'title' => $title,
					'page_id' => $title->getArticleID(),
					'rev_id' => null );
				break;
			}
			$rev = $id ? Revision::newFromId( $id ) : Revision::newFromTitle( $title );
			$rev_id = $rev ? $rev->getId() : 0;
			// If there is no current revision, there is no page
			if( $id === false && !$rev ) {
				$linkCache = LinkCache::singleton();
				$linkCache->addBadLinkObj( $title );
			}

			$deps[] = array(
				'title' => $title,
				'page_id' => $title->getArticleID(),
				'rev_id' => $rev_id );

			if( $rev ) {
				$text = $rev->getText();
			} elseif( $title->getNamespace() == NS_MEDIAWIKI ) {
				global $wgLang;
				$message = $wgLang->lcfirst( $title->getText() );
				$text = wfMsgForContentNoTrans( $message );
				if( wfEmptyMsg( $message, $text ) ) {
					$text = false;
					break;
				}
			} else {
				break;
			}
			if ( $text === false ) {
				break;
			}
			// Redirect?
			$finalTitle = $title;
			$title = Title::newFromRedirect( $text );
		}
		return array(
			'text' => $text,
			'finalTitle' => $finalTitle,
			'deps' => $deps );
	}

	/**
	 * Transclude an interwiki link.
	 */
	function interwikiTransclude( $title, $action ) {
		global $wgEnableScaryTranscluding;

		if (!$wgEnableScaryTranscluding)
			return wfMsg('scarytranscludedisabled');

		$url = $title->getFullUrl( "action=$action" );

		if (strlen($url) > 255)
			return wfMsg('scarytranscludetoolong');
		return $this->fetchScaryTemplateMaybeFromCache($url);
	}

	function fetchScaryTemplateMaybeFromCache($url) {
		global $wgTranscludeCacheExpiry;
		$dbr = wfGetDB(DB_SLAVE);
		$obj = $dbr->selectRow('transcache', array('tc_time', 'tc_contents'),
				array('tc_url' => $url));
		if ($obj) {
			$time = $obj->tc_time;
			$text = $obj->tc_contents;
			if ($time && time() < $time + $wgTranscludeCacheExpiry ) {
				return $text;
			}
		}

		$text = Http::get($url);
		if (!$text)
			return wfMsg('scarytranscludefailed', $url);

		$dbw = wfGetDB(DB_MASTER);
		$dbw->replace('transcache', array('tc_url'), array(
			'tc_url' => $url,
			'tc_time' => time(),
			'tc_contents' => $text));
		return $text;
	}


	/**
	 * Triple brace replacement -- used for template arguments
	 * @private
	 */
	function argSubstitution( $piece, $frame ) {
		wfProfileIn( __METHOD__ );

		$error = false;
		$parts = $piece['parts'];
		$nameWithSpaces = $frame->expand( $piece['title'] );
		$argName = trim( $nameWithSpaces );
		$object = false;
		$text = $frame->getArgument( $argName );
		if (  $text === false && $parts->getLength() > 0
		  && (
		    $this->ot['html']
		    || $this->ot['pre']
		    || ( $this->ot['wiki'] && $frame->isTemplate() )
		  )
		) {
			# No match in frame, use the supplied default
			$object = $parts->item( 0 )->getChildren();
		}
		if ( !$this->incrementIncludeSize( 'arg', strlen( $text ) ) ) {
			$error = '<!-- WARNING: argument omitted, expansion size too large -->';
			$this->limitationWarn( 'post-expand-template-argument' );
		}

		if ( $text === false && $object === false ) {
			# No match anywhere
			$object = $frame->virtualBracketedImplode( '{{{', '|', '}}}', $nameWithSpaces, $parts );
		}
		if ( $error !== false ) {
			$text .= $error;
		}
		if ( $object !== false ) {
			$ret = array( 'object' => $object );
		} else {
			$ret = array( 'text' => $text );
		}

		wfProfileOut( __METHOD__ );
		return $ret;
	}

	/**
	 * Return the text to be used for a given extension tag.
	 * This is the ghost of strip().
	 *
	 * @param array $params Associative array of parameters:
	 *     name       PPNode for the tag name
	 *     attr       PPNode for unparsed text where tag attributes are thought to be
	 *     attributes Optional associative array of parsed attributes
	 *     inner      Contents of extension element
	 *     noClose    Original text did not have a close tag
	 * @param PPFrame $frame
	 */
	function extensionSubstitution( $params, $frame ) {
		global $wgRawHtml, $wgContLang;

		$name = $frame->expand( $params['name'] );
		$attrText = !isset( $params['attr'] ) ? null : $frame->expand( $params['attr'] );
		$content = !isset( $params['inner'] ) ? null : $frame->expand( $params['inner'] );

		$marker = "{$this->mUniqPrefix}-$name-" . sprintf('%08X', $this->mMarkerIndex++) . self::MARKER_SUFFIX;

		if ( $this->ot['html'] ) {
			$name = strtolower( $name );

			$attributes = Sanitizer::decodeTagAttributes( $attrText );
			if ( isset( $params['attributes'] ) ) {
				$attributes = $attributes + $params['attributes'];
			}
			switch ( $name ) {
				case 'html':
					if( $wgRawHtml ) {
						$output = $content;
						break;
					} else {
						throw new MWException( '<html> extension tag encountered unexpectedly' );
					}
				case 'nowiki':
					$output = Xml::escapeTagsOnly( $content );
					break;
				case 'math':
					$output = $wgContLang->armourMath(
						MathRenderer::renderMath( $content, $attributes ) );
					break;
				case 'gallery':
					$output = $this->renderImageGallery( $content, $attributes );
					break;
				default:
					if( isset( $this->mTagHooks[$name] ) ) {
						# Workaround for PHP bug 35229 and similar
						if ( !is_callable( $this->mTagHooks[$name] ) ) {
							throw new MWException( "Tag hook for $name is not callable\n" );
						}
						$output = call_user_func_array( $this->mTagHooks[$name],
							array( $content, $attributes, $this ) );
					} else {
						$output = '<span class="error">Invalid tag extension name: ' .
							htmlspecialchars( $name ) . '</span>';
					}
			}
		} else {
			if ( is_null( $attrText ) ) {
				$attrText = '';
			}
			if ( isset( $params['attributes'] ) ) {
				foreach ( $params['attributes'] as $attrName => $attrValue ) {
					$attrText .= ' ' . htmlspecialchars( $attrName ) . '="' .
						htmlspecialchars( $attrValue ) . '"';
				}
			}
			if ( $content === null ) {
				$output = "<$name$attrText/>";
			} else {
				$close = is_null( $params['close'] ) ? '' : $frame->expand( $params['close'] );
				$output = "<$name$attrText>$content$close";
			}
		}

		if ( $name == 'html' || $name == 'nowiki' ) {
			$this->mStripState->nowiki->setPair( $marker, $output );
		} else {
			$this->mStripState->general->setPair( $marker, $output );
		}
		return $marker;
	}

	/**
	 * Increment an include size counter
	 *
	 * @param string $type The type of expansion
	 * @param integer $size The size of the text
	 * @return boolean False if this inclusion would take it over the maximum, true otherwise
	 */
	function incrementIncludeSize( $type, $size ) {
		if ( $this->mIncludeSizes[$type] + $size > $this->mOptions->getMaxIncludeSize( $type ) ) {
			return false;
		} else {
			$this->mIncludeSizes[$type] += $size;
			return true;
		}
	}

	/**
	 * Increment the expensive function count
	 *
	 * @return boolean False if the limit has been exceeded
	 */
	function incrementExpensiveFunctionCount() {
		global $wgExpensiveParserFunctionLimit;
		$this->mExpensiveFunctionCount++;
		if($this->mExpensiveFunctionCount <= $wgExpensiveParserFunctionLimit) {
			return true;
		}
		return false;
	}

	/**
	 * Strip double-underscore items like __NOGALLERY__ and __NOTOC__
	 * Fills $this->mDoubleUnderscores, returns the modified text
	 */
	function doDoubleUnderscore( $text ) {
		// The position of __TOC__ needs to be recorded
		$mw = MagicWord::get( 'toc' );
		if( $mw->match( $text ) ) {
			$this->mShowToc = true;
			$this->mForceTocPosition = true;

			// Set a placeholder. At the end we'll fill it in with the TOC.
			$text = $mw->replace( '<!--MWTOC-->', $text, 1 );

			// Only keep the first one.
			$text = $mw->replace( '', $text );
		}

		// Now match and remove the rest of them
		$mwa = MagicWord::getDoubleUnderscoreArray();
		$this->mDoubleUnderscores = $mwa->matchAndRemove( $text );

		if ( isset( $this->mDoubleUnderscores['nogallery'] ) ) {
			$this->mOutput->mNoGallery = true;
		}
		if ( isset( $this->mDoubleUnderscores['notoc'] ) && !$this->mForceTocPosition ) {
			$this->mShowToc = false;
		}
		if ( isset( $this->mDoubleUnderscores['hiddencat'] ) && $this->mTitle->getNamespace() == NS_CATEGORY ) {
			$this->mOutput->setProperty( 'hiddencat', 'y' );

			$containerCategory = Title::makeTitleSafe( NS_CATEGORY, wfMsgForContent( 'hidden-category-category' ) );
			if ( $containerCategory ) {
				$this->mOutput->addCategory( $containerCategory->getDBkey(), $this->getDefaultSort() );
			} else {
				wfDebug( __METHOD__.": [[MediaWiki:hidden-category-category]] is not a valid title!\n" );
			}
		}
		return $text;
	}

	/**
	 * This function accomplishes several tasks:
	 * 1) Auto-number headings if that option is enabled
	 * 2) Add an [edit] link to sections for users who have enabled the option and can edit the page
	 * 3) Add a Table of contents on the top for users who have enabled the option
	 * 4) Auto-anchor headings
	 *
	 * It loops through all headlines, collects the necessary data, then splits up the
	 * string and re-inserts the newly formatted headlines.
	 *
	 * @param string $text
	 * @param boolean $isMain
	 * @private
	 */
	function formatHeadings( $text, $isMain=true ) {
		global $wgMaxTocLevel, $wgContLang;

		$doNumberHeadings = $this->mOptions->getNumberHeadings();
		if( !$this->mTitle->quickUserCan( 'edit' ) ) {
			$showEditLink = 0;
		} else {
			$showEditLink = $this->mOptions->getEditSection();
		}

		# Inhibit editsection links if requested in the page
		if ( isset( $this->mDoubleUnderscores['noeditsection'] ) ) {
			$showEditLink = 0;
		}

		# Get all headlines for numbering them and adding funky stuff like [edit]
		# links - this is for later, but we need the number of headlines right now
		$matches = array();
		$numMatches = preg_match_all( '/<H(?P<level>[1-6])(?P<attrib>.*?'.'>)(?P<header>.*?)<\/H[1-6] *>/i', $text, $matches );

		# if there are fewer than 4 headlines in the article, do not show TOC
		# unless it's been explicitly enabled.
		$enoughToc = $this->mShowToc &&
			(($numMatches >= 4) || $this->mForceTocPosition);

		# Allow user to stipulate that a page should have a "new section"
		# link added via __NEWSECTIONLINK__
		if ( isset( $this->mDoubleUnderscores['newsectionlink'] ) ) {
			$this->mOutput->setNewSection( true );
		}

		# if the string __FORCETOC__ (not case-sensitive) occurs in the HTML,
		# override above conditions and always show TOC above first header
		if ( isset( $this->mDoubleUnderscores['forcetoc'] ) ) {
			$this->mShowToc = true;
			$enoughToc = true;
		}

		# We need this to perform operations on the HTML
		$sk = $this->mOptions->getSkin();

		# headline counter
		$headlineCount = 0;
		$numVisible = 0;

		# Ugh .. the TOC should have neat indentation levels which can be
		# passed to the skin functions. These are determined here
		$toc = '';
		$full = '';
		$head = array();
		$sublevelCount = array();
		$levelCount = array();
		$toclevel = 0;
		$level = 0;
		$prevlevel = 0;
		$toclevel = 0;
		$prevtoclevel = 0;
		$markerRegex = "{$this->mUniqPrefix}-h-(\d+)-" . self::MARKER_SUFFIX;
		$baseTitleText = $this->mTitle->getPrefixedDBkey();
		$tocraw = array();

		foreach( $matches[3] as $headline ) {
			$isTemplate = false;
			$titleText = false;
			$sectionIndex = false;
			$numbering = '';
			$markerMatches = array();
			if (preg_match("/^$markerRegex/", $headline, $markerMatches)) {
				$serial = $markerMatches[1];
				list( $titleText, $sectionIndex ) = $this->mHeadings[$serial];
				$isTemplate = ($titleText != $baseTitleText);
				$headline = preg_replace("/^$markerRegex/", "", $headline);
			}

			if( $toclevel ) {
				$prevlevel = $level;
				$prevtoclevel = $toclevel;
			}
			$level = $matches[1][$headlineCount];

			if( $doNumberHeadings || $enoughToc ) {

				if ( $level > $prevlevel ) {
					# Increase TOC level
					$toclevel++;
					$sublevelCount[$toclevel] = 0;
					if( $toclevel<$wgMaxTocLevel ) {
						$prevtoclevel = $toclevel;
						$toc .= $sk->tocIndent();
						$numVisible++;
					}
				}
				elseif ( $level < $prevlevel && $toclevel > 1 ) {
					# Decrease TOC level, find level to jump to

					if ( $toclevel == 2 && $level <= $levelCount[1] ) {
						# Can only go down to level 1
						$toclevel = 1;
					} else {
						for ($i = $toclevel; $i > 0; $i--) {
							if ( $levelCount[$i] == $level ) {
								# Found last matching level
								$toclevel = $i;
								break;
							}
							elseif ( $levelCount[$i] < $level ) {
								# Found first matching level below current level
								$toclevel = $i + 1;
								break;
							}
						}
					}
					if( $toclevel<$wgMaxTocLevel ) {
						if($prevtoclevel < $wgMaxTocLevel) {
							# Unindent only if the previous toc level was shown :p
							$toc .= $sk->tocUnindent( $prevtoclevel - $toclevel );
							$prevtoclevel = $toclevel;
						} else {
							$toc .= $sk->tocLineEnd();
						}
					}
				}
				else {
					# No change in level, end TOC line
					if( $toclevel<$wgMaxTocLevel ) {
						$toc .= $sk->tocLineEnd();
					}
				}

				$levelCount[$toclevel] = $level;

				# count number of headlines for each level
				@$sublevelCount[$toclevel]++;
				$dot = 0;
				for( $i = 1; $i <= $toclevel; $i++ ) {
					if( !empty( $sublevelCount[$i] ) ) {
						if( $dot ) {
							$numbering .= '.';
						}
						$numbering .= $wgContLang->formatNum( $sublevelCount[$i] );
						$dot = 1;
					}
				}
			}

			# The safe header is a version of the header text safe to use for links
			# Avoid insertion of weird stuff like <math> by expanding the relevant sections
			$safeHeadline = $this->mStripState->unstripBoth( $headline );

			# Remove link placeholders by the link text.
			#     <!--LINK number-->
			# turns into
			#     link text with suffix
			$safeHeadline = preg_replace( '/<!--LINK ([0-9]*)-->/e',
							    "\$this->mLinkHolders['texts'][\$1]",
							    $safeHeadline );
			$safeHeadline = preg_replace( '/<!--IWLINK ([0-9]*)-->/e',
							    "\$this->mInterwikiLinkHolders['texts'][\$1]",
							    $safeHeadline );

			# Strip out HTML (other than plain <sup> and <sub>: bug 8393)
			$tocline = preg_replace(
				array( '#<(?!/?(sup|sub)).*?'.'>#', '#<(/?(sup|sub)).*?'.'>#' ),
				array( '',                          '<$1>'),
				$safeHeadline
			);
			$tocline = trim( $tocline );

			# For the anchor, strip out HTML-y stuff period
			$safeHeadline = preg_replace( '/<.*?'.'>/', '', $safeHeadline );
			$safeHeadline = trim( $safeHeadline );

			# Save headline for section edit hint before it's escaped
			$headlineHint = $safeHeadline;
			$safeHeadline = Sanitizer::escapeId( $safeHeadline );
			# HTML names must be case-insensitively unique (bug 10721)
			$arrayKey = strtolower( $safeHeadline );

			# count how many in assoc. array so we can track dupes in anchors
			isset( $refers[$arrayKey] ) ? $refers[$arrayKey]++ : $refers[$arrayKey] = 1;
			$refcount[$headlineCount] = $refers[$arrayKey];

			# Don't number the heading if it is the only one (looks silly)
			if( $doNumberHeadings && count( $matches[3] ) > 1) {
				# the two are different if the line contains a link
				$headline=$numbering . ' ' . $headline;
			}

			# Create the anchor for linking from the TOC to the section
			$anchor = $safeHeadline;
			if($refcount[$headlineCount] > 1 ) {
				$anchor .= '_' . $refcount[$headlineCount];
			}
			if( $enoughToc && ( !isset($wgMaxTocLevel) || $toclevel<$wgMaxTocLevel ) ) {
				$toc .= $sk->tocLine($anchor, $tocline, $numbering, $toclevel);
				$tocraw[] = array( 'toclevel' => $toclevel, 'level' => $level, 'line' => $tocline, 'number' => $numbering );
			}
			# give headline the correct <h#> tag
			if( $showEditLink && $sectionIndex !== false ) {
				if( $isTemplate ) {
					# Put a T flag in the section identifier, to indicate to extractSections()
					# that sections inside <includeonly> should be counted.
					$editlink = $sk->editSectionLinkForOther($titleText, "T-$sectionIndex");
				} else {
					$editlink = $sk->editSectionLink($this->mTitle, $sectionIndex, $headlineHint);
				}
			} else {
				$editlink = '';
			}
			$head[$headlineCount] = $sk->makeHeadline( $level, $matches['attrib'][$headlineCount], $anchor, $headline, $editlink );

			$headlineCount++;
		}

		$this->mOutput->setSections( $tocraw );

		# Never ever show TOC if no headers
		if( $numVisible < 1 ) {
			$enoughToc = false;
		}

		if( $enoughToc ) {
			if( $prevtoclevel > 0 && $prevtoclevel < $wgMaxTocLevel ) {
				$toc .= $sk->tocUnindent( $prevtoclevel - 1 );
			}
			$toc = $sk->tocList( $toc );
		}

		# split up and insert constructed headlines

		$blocks = preg_split( '/<H[1-6].*?' . '>.*?<\/H[1-6]>/i', $text );
		$i = 0;

		foreach( $blocks as $block ) {
			if( $showEditLink && $headlineCount > 0 && $i == 0 && $block != "\n" ) {
				# This is the [edit] link that appears for the top block of text when
				# section editing is enabled

				# Disabled because it broke block formatting
				# For example, a bullet point in the top line
				# $full .= $sk->editSectionLink(0);
			}
			$full .= $block;
			if( $enoughToc && !$i && $isMain && !$this->mForceTocPosition ) {
				# Top anchor now in skin
				$full = $full.$toc;
			}

			if( !empty( $head[$i] ) ) {
				$full .= $head[$i];
			}
			$i++;
		}
		if( $this->mForceTocPosition ) {
			return str_replace( '<!--MWTOC-->', $toc, $full );
		} else {
			return $full;
		}
	}

	/**
	 * Transform wiki markup when saving a page by doing \r\n -> \n
	 * conversion, substitting signatures, {{subst:}} templates, etc.
	 *
	 * @param string $text the text to transform
	 * @param Title &$title the Title object for the current article
	 * @param User &$user the User object describing the current user
	 * @param ParserOptions $options parsing options
	 * @param bool $clearState whether to clear the parser state first
	 * @return string the altered wiki markup
	 * @public
	 */
	function preSaveTransform( $text, &$title, $user, $options, $clearState = true ) {
		$this->mOptions = $options;
		$this->setTitle( $title );
		$this->setOutputType( self::OT_WIKI );

		if ( $clearState ) {
			$this->clearState();
		}

		$pairs = array(
			"\r\n" => "\n",
		);
		$text = str_replace( array_keys( $pairs ), array_values( $pairs ), $text );
		$text = $this->pstPass2( $text, $user );
		$text = $this->mStripState->unstripBoth( $text );
		return $text;
	}

	/**
	 * Pre-save transform helper function
	 * @private
	 */
	function pstPass2( $text, $user ) {
		global $wgContLang, $wgLocaltimezone;

		/* Note: This is the timestamp saved as hardcoded wikitext to
		 * the database, we use $wgContLang here in order to give
		 * everyone the same signature and use the default one rather
		 * than the one selected in each user's preferences.
		 *
		 * (see also bug 12815)
		 */
		$ts = $this->mOptions->getTimestamp();
		$tz = wfMsgForContent( 'timezone-utc' );
		if ( isset( $wgLocaltimezone ) ) {
			$unixts = wfTimestamp( TS_UNIX, $ts );
			$oldtz = getenv( 'TZ' );
			putenv( 'TZ='.$wgLocaltimezone );
			$ts = date( 'YmdHis', $unixts );
			$tz = date( 'T', $unixts );  # might vary on DST changeover!
			putenv( 'TZ='.$oldtz );
		}

		$d = $wgContLang->timeanddate( $ts, false, false ) . " ($tz)";

		# Variable replacement
		# Because mOutputType is OT_WIKI, this will only process {{subst:xxx}} type tags
		$text = $this->replaceVariables( $text );

		# Signatures
		$sigText = $this->getUserSig( $user );
		$text = strtr( $text, array(
			'~~~~~' => $d,
			'~~~~' => "$sigText $d",
			'~~~' => $sigText
		) );

		# Context links: [[|name]] and [[name (context)|]]
		#
		global $wgLegalTitleChars;
		$tc = "[$wgLegalTitleChars]";
		$nc = '[ _0-9A-Za-z\x80-\xff-]'; # Namespaces can use non-ascii!

		$p1 = "/\[\[(:?$nc+:|:|)($tc+?)( \\($tc+\\))\\|]]/";		# [[ns:page (context)|]]
		$p3 = "/\[\[(:?$nc+:|:|)($tc+?)( \\($tc+\\)|)(, $tc+|)\\|]]/";	# [[ns:page (context), context|]]
		$p2 = "/\[\[\\|($tc+)]]/";					# [[|page]]

		# try $p1 first, to turn "[[A, B (C)|]]" into "[[A, B (C)|A, B]]"
		$text = preg_replace( $p1, '[[\\1\\2\\3|\\2]]', $text );
		$text = preg_replace( $p3, '[[\\1\\2\\3\\4|\\2]]', $text );

		$t = $this->mTitle->getText();
		$m = array();
		if ( preg_match( "/^($nc+:|)$tc+?( \\($tc+\\))$/", $t, $m ) ) {
			$text = preg_replace( $p2, "[[$m[1]\\1$m[2]|\\1]]", $text );
		} elseif ( preg_match( "/^($nc+:|)$tc+?(, $tc+|)$/", $t, $m ) && '' != "$m[1]$m[2]" ) {
			$text = preg_replace( $p2, "[[$m[1]\\1$m[2]|\\1]]", $text );
		} else {
			# if there's no context, don't bother duplicating the title
			$text = preg_replace( $p2, '[[\\1]]', $text );
		}

		# Trim trailing whitespace
		$text = rtrim( $text );

		return $text;
	}

	/**
	 * Fetch the user's signature text, if any, and normalize to
	 * validated, ready-to-insert wikitext.
	 *
	 * @param User $user
	 * @return string
	 * @private
	 */
	function getUserSig( &$user ) {
		global $wgMaxSigChars;

		$username = $user->getName();
		$nickname = $user->getOption( 'nickname' );
		$nickname = $nickname === '' ? $username : $nickname;

		if( mb_strlen( $nickname ) > $wgMaxSigChars ) {
			$nickname = $username;
			wfDebug( __METHOD__ . ": $username has overlong signature.\n" );
		} elseif( $user->getBoolOption( 'fancysig' ) !== false ) {
			# Sig. might contain markup; validate this
			if( $this->validateSig( $nickname ) !== false ) {
				# Validated; clean up (if needed) and return it
				return $this->cleanSig( $nickname, true );
			} else {
				# Failed to validate; fall back to the default
				$nickname = $username;
				wfDebug( "Parser::getUserSig: $username has bad XML tags in signature.\n" );
			}
		}

		// Make sure nickname doesnt get a sig in a sig
		$nickname = $this->cleanSigInSig( $nickname );

		# If we're still here, make it a link to the user page
		$userText = wfEscapeWikiText( $username );
		$nickText = wfEscapeWikiText( $nickname );
		if ( $user->isAnon() )  {
			return wfMsgExt( 'signature-anon', array( 'content', 'parsemag' ), $userText, $nickText );
		} else {
			return wfMsgExt( 'signature', array( 'content', 'parsemag' ), $userText, $nickText );
		}
	}

	/**
	 * Check that the user's signature contains no bad XML
	 *
	 * @param string $text
	 * @return mixed An expanded string, or false if invalid.
	 */
	function validateSig( $text ) {
		return( wfIsWellFormedXmlFragment( $text ) ? $text : false );
	}

	/**
	 * Clean up signature text
	 *
	 * 1) Strip ~~~, ~~~~ and ~~~~~ out of signatures @see cleanSigInSig
	 * 2) Substitute all transclusions
	 *
	 * @param string $text
	 * @param $parsing Whether we're cleaning (preferences save) or parsing
	 * @return string Signature text
	 */
	function cleanSig( $text, $parsing = false ) {
		if ( !$parsing ) {
			global $wgTitle;
			$this->clearState();
			$this->setTitle( $wgTitle );
			$this->mOptions = new ParserOptions;
			$this->setOutputType = self::OT_PREPROCESS;
		}

		# FIXME: regex doesn't respect extension tags or nowiki
		#  => Move this logic to braceSubstitution()
		$substWord = MagicWord::get( 'subst' );
		$substRegex = '/\{\{(?!(?:' . $substWord->getBaseRegex() . '))/x' . $substWord->getRegexCase();
		$substText = '{{' . $substWord->getSynonym( 0 );

		$text = preg_replace( $substRegex, $substText, $text );
		$text = $this->cleanSigInSig( $text );
		$dom = $this->preprocessToDom( $text );
		$frame = $this->getPreprocessor()->newFrame();
		$text = $frame->expand( $dom );

		if ( !$parsing ) {
			$text = $this->mStripState->unstripBoth( $text );
		}

		return $text;
	}

	/**
	 * Strip ~~~, ~~~~ and ~~~~~ out of signatures
	 * @param string $text
	 * @return string Signature text with /~{3,5}/ removed
	 */
	function cleanSigInSig( $text ) {
		$text = preg_replace( '/~{3,5}/', '', $text );
		return $text;
	}

	/**
	 * Set up some variables which are usually set up in parse()
	 * so that an external function can call some class members with confidence
	 * @public
	 */
	function startExternalParse( &$title, $options, $outputType, $clearState = true ) {
		$this->setTitle( $title );
		$this->mOptions = $options;
		$this->setOutputType( $outputType );
		if ( $clearState ) {
			$this->clearState();
		}
	}

	/**
	 * Wrapper for preprocess()
	 *
	 * @param string $text the text to preprocess
	 * @param ParserOptions $options  options
	 * @return string
	 * @public
	 */
	function transformMsg( $text, $options ) {
		global $wgTitle;
		static $executing = false;

		$fname = "Parser::transformMsg";

		# Guard against infinite recursion
		if ( $executing ) {
			return $text;
		}
		$executing = true;

		wfProfileIn($fname);
		$text = $this->preprocess( $text, $wgTitle, $options );

		$executing = false;
		wfProfileOut($fname);
		return $text;
	}

	/**
	 * Create an HTML-style tag, e.g. <yourtag>special text</yourtag>
	 * The callback should have the following form:
	 *    function myParserHook( $text, $params, &$parser ) { ... }
	 *
	 * Transform and return $text. Use $parser for any required context, e.g. use
	 * $parser->getTitle() and $parser->getOptions() not $wgTitle or $wgOut->mParserOptions
	 *
	 * @public
	 *
	 * @param mixed $tag The tag to use, e.g. 'hook' for <hook>
	 * @param mixed $callback The callback function (and object) to use for the tag
	 *
	 * @return The old value of the mTagHooks array associated with the hook
	 */
	function setHook( $tag, $callback ) {
		$tag = strtolower( $tag );
		$oldVal = isset( $this->mTagHooks[$tag] ) ? $this->mTagHooks[$tag] : null;
		$this->mTagHooks[$tag] = $callback;
		if( !in_array( $tag, $this->mStripList ) ) {
			$this->mStripList[] = $tag;
		}

		return $oldVal;
	}

	function setTransparentTagHook( $tag, $callback ) {
		$tag = strtolower( $tag );
		$oldVal = isset( $this->mTransparentTagHooks[$tag] ) ? $this->mTransparentTagHooks[$tag] : null;
		$this->mTransparentTagHooks[$tag] = $callback;

		return $oldVal;
	}

	/**
	 * Remove all tag hooks
	 */
	function clearTagHooks() {
		$this->mTagHooks = array();
		$this->mStripList = $this->mDefaultStripList;
	}

	/**
	 * Create a function, e.g. {{sum:1|2|3}}
	 * The callback function should have the form:
	 *    function myParserFunction( &$parser, $arg1, $arg2, $arg3 ) { ... }
	 *
	 * Or with SFH_OBJECT_ARGS:
	 *    function myParserFunction( $parser, $frame, $args ) { ... }
	 *
	 * The callback may either return the text result of the function, or an array with the text
	 * in element 0, and a number of flags in the other elements. The names of the flags are
	 * specified in the keys. Valid flags are:
	 *   found                     The text returned is valid, stop processing the template. This
	 *                             is on by default.
	 *   nowiki                    Wiki markup in the return value should be escaped
	 *   isHTML                    The returned text is HTML, armour it against wikitext transformation
	 *
	 * @public
	 *
	 * @param string $id The magic word ID
	 * @param mixed $callback The callback function (and object) to use
	 * @param integer $flags a combination of the following flags:
	 *     SFH_NO_HASH   No leading hash, i.e. {{plural:...}} instead of {{#if:...}}
	 *
	 *     SFH_OBJECT_ARGS   Pass the template arguments as PPNode objects instead of text. This 
	 *     allows for conditional expansion of the parse tree, allowing you to eliminate dead
	 *     branches and thus speed up parsing. It is also possible to analyse the parse tree of 
	 *     the arguments, and to control the way they are expanded.
	 *
	 *     The $frame parameter is a PPFrame. This can be used to produce expanded text from the
	 *     arguments, for instance:
	 *         $text = isset( $args[0] ) ? $frame->expand( $args[0] ) : '';
	 *
	 *     For technical reasons, $args[0] is pre-expanded and will be a string. This may change in 
	 *     future versions. Please call $frame->expand() on it anyway so that your code keeps
	 *     working if/when this is changed.
	 *
	 *     If you want whitespace to be trimmed from $args, you need to do it yourself, post-
	 *     expansion.
	 *
	 *     Please read the documentation in includes/parser/Preprocessor.php for more information 
	 *     about the methods available in PPFrame and PPNode.
	 *
	 * @return The old callback function for this name, if any
	 */
	function setFunctionHook( $id, $callback, $flags = 0 ) {
		$oldVal = isset( $this->mFunctionHooks[$id] ) ? $this->mFunctionHooks[$id][0] : null;
		$this->mFunctionHooks[$id] = array( $callback, $flags );

		# Add to function cache
		$mw = MagicWord::get( $id );
		if( !$mw )
			throw new MWException( 'Parser::setFunctionHook() expecting a magic word identifier.' );

		$synonyms = $mw->getSynonyms();
		$sensitive = intval( $mw->isCaseSensitive() );

		foreach ( $synonyms as $syn ) {
			# Case
			if ( !$sensitive ) {
				$syn = strtolower( $syn );
			}
			# Add leading hash
			if ( !( $flags & SFH_NO_HASH ) ) {
				$syn = '#' . $syn;
			}
			# Remove trailing colon
			if ( substr( $syn, -1, 1 ) == ':' ) {
				$syn = substr( $syn, 0, -1 );
			}
			$this->mFunctionSynonyms[$sensitive][$syn] = $id;
		}
		return $oldVal;
	}

	/**
	 * Get all registered function hook identifiers
	 *
	 * @return array
	 */
	function getFunctionHooks() {
		return array_keys( $this->mFunctionHooks );
	}

	/**
	 * Replace <!--LINK--> link placeholders with actual links, in the buffer
	 * Placeholders created in Skin::makeLinkObj()
	 * Returns an array of link CSS classes, indexed by PDBK.
	 * $options is a bit field, RLH_FOR_UPDATE to select for update
	 */
	function replaceLinkHolders( &$text, $options = 0 ) {
		global $wgUser;
		global $wgContLang;

		$fname = 'Parser::replaceLinkHolders';
		wfProfileIn( $fname );

		$pdbks = array();
		$colours = array();
		$linkcolour_ids = array();
		$sk = $this->mOptions->getSkin();
		$linkCache = LinkCache::singleton();

		if ( !empty( $this->mLinkHolders['namespaces'] ) ) {
			wfProfileIn( $fname.'-check' );
			$dbr = wfGetDB( DB_SLAVE );
			$page = $dbr->tableName( 'page' );
			$threshold = $wgUser->getOption('stubthreshold');

			# Sort by namespace
			asort( $this->mLinkHolders['namespaces'] );

			# Generate query
			$query = false;
			$current = null;
			foreach ( $this->mLinkHolders['namespaces'] as $key => $ns ) {
				# Make title object
				$title = $this->mLinkHolders['titles'][$key];

				# Skip invalid entries.
				# Result will be ugly, but prevents crash.
				if ( is_null( $title ) ) {
					continue;
				}
				$pdbk = $pdbks[$key] = $title->getPrefixedDBkey();

				# Check if it's a static known link, e.g. interwiki
				if ( $title->isAlwaysKnown() ) {
					$colours[$pdbk] = '';
				} elseif ( ( $id = $linkCache->getGoodLinkID( $pdbk ) ) != 0 ) {
					$colours[$pdbk] = '';
					$this->mOutput->addLink( $title, $id );
				} elseif ( $linkCache->isBadLink( $pdbk ) ) {
					$colours[$pdbk] = 'new';
				} elseif ( $title->getNamespace() == NS_SPECIAL && !SpecialPage::exists( $pdbk ) ) {
					$colours[$pdbk] = 'new';
				} else {
					# Not in the link cache, add it to the query
					if ( !isset( $current ) ) {
						$current = $ns;
						$query =  "SELECT page_id, page_namespace, page_title, page_is_redirect, page_len";
						$query .= " FROM $page WHERE (page_namespace=$ns AND page_title IN(";
					} elseif ( $current != $ns ) {
						$current = $ns;
						$query .= ")) OR (page_namespace=$ns AND page_title IN(";
					} else {
						$query .= ', ';
					}

					$query .= $dbr->addQuotes( $this->mLinkHolders['dbkeys'][$key] );
				}
			}
			if ( $query ) {
				$query .= '))';
				if ( $options & RLH_FOR_UPDATE ) {
					$query .= ' FOR UPDATE';
				}

				$res = $dbr->query( $query, $fname );

				# Fetch data and form into an associative array
				# non-existent = broken
				while ( $s = $dbr->fetchObject($res) ) {
					$title = Title::makeTitle( $s->page_namespace, $s->page_title );
					$pdbk = $title->getPrefixedDBkey();
					$linkCache->addGoodLinkObj( $s->page_id, $title, $s->page_len, $s->page_is_redirect );
					$this->mOutput->addLink( $title, $s->page_id );
					$colours[$pdbk] = $sk->getLinkColour( $title, $threshold );
					//add id to the extension todolist
					$linkcolour_ids[$s->page_id] = $pdbk;
				}
				//pass an array of page_ids to an extension
				wfRunHooks( 'GetLinkColours', array( $linkcolour_ids, &$colours ) );
			}
			wfProfileOut( $fname.'-check' );

			# Do a second query for different language variants of links and categories
			if($wgContLang->hasVariants()){
				$linkBatch = new LinkBatch();
				$variantMap = array(); // maps $pdbkey_Variant => $keys (of link holders)
				$categoryMap = array(); // maps $category_variant => $category (dbkeys)
				$varCategories = array(); // category replacements oldDBkey => newDBkey

				$categories = $this->mOutput->getCategoryLinks();

				// Add variants of links to link batch
				foreach ( $this->mLinkHolders['namespaces'] as $key => $ns ) {
					$title = $this->mLinkHolders['titles'][$key];
					if ( is_null( $title ) )
						continue;

					$pdbk = $title->getPrefixedDBkey();
					$titleText = $title->getText();

					// generate all variants of the link title text
					$allTextVariants = $wgContLang->convertLinkToAllVariants($titleText);

					// if link was not found (in first query), add all variants to query
					if ( !isset($colours[$pdbk]) ){
						foreach($allTextVariants as $textVariant){
							if($textVariant != $titleText){
								$variantTitle = Title::makeTitle( $ns, $textVariant );
								if(is_null($variantTitle)) continue;
								$linkBatch->addObj( $variantTitle );
								$variantMap[$variantTitle->getPrefixedDBkey()][] = $key;
							}
						}
					}
				}

				// process categories, check if a category exists in some variant
				foreach( $categories as $category ){
					$variants = $wgContLang->convertLinkToAllVariants($category);
					foreach($variants as $variant){
						if($variant != $category){
							$variantTitle = Title::newFromDBkey( Title::makeName(NS_CATEGORY,$variant) );
							if(is_null($variantTitle)) continue;
							$linkBatch->addObj( $variantTitle );
							$categoryMap[$variant] = $category;
						}
					}
				}


				if(!$linkBatch->isEmpty()){
					// construct query
					$titleClause = $linkBatch->constructSet('page', $dbr);

					$variantQuery =  "SELECT page_id, page_namespace, page_title, page_is_redirect, page_len";

					$variantQuery .= " FROM $page WHERE $titleClause";
					if ( $options & RLH_FOR_UPDATE ) {
						$variantQuery .= ' FOR UPDATE';
					}

					$varRes = $dbr->query( $variantQuery, $fname );

					// for each found variants, figure out link holders and replace
					while ( $s = $dbr->fetchObject($varRes) ) {

						$variantTitle = Title::makeTitle( $s->page_namespace, $s->page_title );
						$varPdbk = $variantTitle->getPrefixedDBkey();
						$vardbk = $variantTitle->getDBkey();

						$holderKeys = array();
						if(isset($variantMap[$varPdbk])){
							$holderKeys = $variantMap[$varPdbk];
							$linkCache->addGoodLinkObj( $s->page_id, $variantTitle, $s->page_len, $s->page_is_redirect );
							$this->mOutput->addLink( $variantTitle, $s->page_id );
						}

						// loop over link holders
						foreach($holderKeys as $key){
							$title = $this->mLinkHolders['titles'][$key];
							if ( is_null( $title ) ) continue;

							$pdbk = $title->getPrefixedDBkey();

							if(!isset($colours[$pdbk])){
								// found link in some of the variants, replace the link holder data
								$this->mLinkHolders['titles'][$key] = $variantTitle;
								$this->mLinkHolders['dbkeys'][$key] = $variantTitle->getDBkey();

								// set pdbk and colour
								$pdbks[$key] = $varPdbk;
								$colours[$varPdbk] = $sk->getLinkColour( $variantTitle, $threshold );
								$linkcolour_ids[$s->page_id] = $pdbk;
							}
							wfRunHooks( 'GetLinkColours', array( $linkcolour_ids, &$colours ) );
						}

						// check if the object is a variant of a category
						if(isset($categoryMap[$vardbk])){
							$oldkey = $categoryMap[$vardbk];
							if($oldkey != $vardbk)
								$varCategories[$oldkey]=$vardbk;
						}
					}

					// rebuild the categories in original order (if there are replacements)
					if(count($varCategories)>0){
						$newCats = array();
						$originalCats = $this->mOutput->getCategories();
						foreach($originalCats as $cat => $sortkey){
							// make the replacement
							if( array_key_exists($cat,$varCategories) )
								$newCats[$varCategories[$cat]] = $sortkey;
							else $newCats[$cat] = $sortkey;
						}
						$this->mOutput->setCategoryLinks($newCats);
					}
				}
			}

			# Construct search and replace arrays
			wfProfileIn( $fname.'-construct' );
			$replacePairs = array();
			foreach ( $this->mLinkHolders['namespaces'] as $key => $ns ) {
				$pdbk = $pdbks[$key];
				$searchkey = "<!--LINK $key-->";
				$title = $this->mLinkHolders['titles'][$key];
				if ( !isset( $colours[$pdbk] ) || $colours[$pdbk] == 'new' ) {
					$linkCache->addBadLinkObj( $title );
					$colours[$pdbk] = 'new';
					$this->mOutput->addLink( $title, 0 );
					$replacePairs[$searchkey] = $sk->makeBrokenLinkObj( $title,
									$this->mLinkHolders['texts'][$key],
									$this->mLinkHolders['queries'][$key] );
				} else {
					$replacePairs[$searchkey] = $sk->makeColouredLinkObj( $title, $colours[$pdbk],
									$this->mLinkHolders['texts'][$key],
									$this->mLinkHolders['queries'][$key] );
				}
			}
			$replacer = new HashtableReplacer( $replacePairs, 1 );
			wfProfileOut( $fname.'-construct' );

			# Do the thing
			wfProfileIn( $fname.'-replace' );
			$text = preg_replace_callback(
				'/(<!--LINK .*?-->)/',
				$replacer->cb(),
				$text);

			wfProfileOut( $fname.'-replace' );
		}

		# Now process interwiki link holders
		# This is quite a bit simpler than internal links
		if ( !empty( $this->mInterwikiLinkHolders['texts'] ) ) {
			wfProfileIn( $fname.'-interwiki' );
			# Make interwiki link HTML
			$replacePairs = array();
			foreach( $this->mInterwikiLinkHolders['texts'] as $key => $link ) {
				$title = $this->mInterwikiLinkHolders['titles'][$key];
				$replacePairs[$key] = $sk->makeLinkObj( $title, $link );
			}
			$replacer = new HashtableReplacer( $replacePairs, 1 );

			$text = preg_replace_callback(
				'/<!--IWLINK (.*?)-->/',
				$replacer->cb(),
				$text );
			wfProfileOut( $fname.'-interwiki' );
		}

		wfProfileOut( $fname );
		return $colours;
	}

	/**
	 * Replace <!--LINK--> link placeholders with plain text of links
	 * (not HTML-formatted).
	 * @param string $text
	 * @return string
	 */
	function replaceLinkHoldersText( $text ) {
		$fname = 'Parser::replaceLinkHoldersText';
		wfProfileIn( $fname );

		$text = preg_replace_callback(
			'/<!--(LINK|IWLINK) (.*?)-->/',
			array( &$this, 'replaceLinkHoldersTextCallback' ),
			$text );

		wfProfileOut( $fname );
		return $text;
	}

	/**
	 * @param array $matches
	 * @return string
	 * @private
	 */
	function replaceLinkHoldersTextCallback( $matches ) {
		$type = $matches[1];
		$key  = $matches[2];
		if( $type == 'LINK' ) {
			if( isset( $this->mLinkHolders['texts'][$key] ) ) {
				return $this->mLinkHolders['texts'][$key];
			}
		} elseif( $type == 'IWLINK' ) {
			if( isset( $this->mInterwikiLinkHolders['texts'][$key] ) ) {
				return $this->mInterwikiLinkHolders['texts'][$key];
			}
		}
		return $matches[0];
	}

	/**
	 * Tag hook handler for 'pre'.
	 */
	function renderPreTag( $text, $attribs ) {
		// Backwards-compatibility hack
		$content = StringUtils::delimiterReplace( '<nowiki>', '</nowiki>', '$1', $text, 'i' );

		$attribs = Sanitizer::validateTagAttributes( $attribs, 'pre' );
		return wfOpenElement( 'pre', $attribs ) .
			Xml::escapeTagsOnly( $content ) .
			'</pre>';
	}

	/**
	 * Renders an image gallery from a text with one line per image.
	 * text labels may be given by using |-style alternative text. E.g.
	 *   Image:one.jpg|The number "1"
	 *   Image:tree.jpg|A tree
	 * given as text will return the HTML of a gallery with two images,
	 * labeled 'The number "1"' and
	 * 'A tree'.
	 */
	function renderImageGallery( $text, $params ) {
		$ig = new ImageGallery();
		$ig->setContextTitle( $this->mTitle );
		$ig->setShowBytes( false );
		$ig->setShowFilename( false );
		$ig->setParser( $this );
		$ig->setHideBadImages();
		$ig->setAttributes( Sanitizer::validateTagAttributes( $params, 'table' ) );
		$ig->useSkin( $this->mOptions->getSkin() );
		$ig->mRevisionId = $this->mRevisionId;

		if( isset( $params['caption'] ) ) {
			$caption = $params['caption'];
			$caption = htmlspecialchars( $caption );
			$caption = $this->replaceInternalLinks( $caption );
			$ig->setCaptionHtml( $caption );
		}
		if( isset( $params['perrow'] ) ) {
			$ig->setPerRow( $params['perrow'] );
		}
		if( isset( $params['widths'] ) ) {
			$ig->setWidths( $params['widths'] );
		}
		if( isset( $params['heights'] ) ) {
			$ig->setHeights( $params['heights'] );
		}

		wfRunHooks( 'BeforeParserrenderImageGallery', array( &$this, &$ig ) );

		$lines = explode( "\n", $text );
		foreach ( $lines as $line ) {
			# match lines like these:
			# Image:someimage.jpg|This is some image
			$matches = array();
			preg_match( "/^([^|]+)(\\|(.*))?$/", $line, $matches );
			# Skip empty lines
			if ( count( $matches ) == 0 ) {
				continue;
			}
			
			if ( strpos( $matches[0], '%' ) !== false )
				$matches[1] = urldecode( $matches[1] );
			$tp = Title::newFromText( $matches[1] );
			$nt =& $tp;
			if( is_null( $nt ) ) {
				# Bogus title. Ignore these so we don't bomb out later.
				continue;
			}
			if ( isset( $matches[3] ) ) {
				$label = $matches[3];
			} else {
				$label = '';
			}

			$html = $this->recursiveTagParse( trim( $label ) );

			$ig->add( $nt, $html );

			# Only add real images (bug #5586)
			if ( $nt->getNamespace() == NS_IMAGE ) {
				$this->mOutput->addImage( $nt->getDBkey() );
			}
		}
		return $ig->toHTML();
	}

	function getImageParams( $handler ) {
		if ( $handler ) {
			$handlerClass = get_class( $handler );
		} else {
			$handlerClass = '';
		}
		if ( !isset( $this->mImageParams[$handlerClass]  ) ) {
			// Initialise static lists
			static $internalParamNames = array(
				'horizAlign' => array( 'left', 'right', 'center', 'none' ),
				'vertAlign' => array( 'baseline', 'sub', 'super', 'top', 'text-top', 'middle',
					'bottom', 'text-bottom' ),
				'frame' => array( 'thumbnail', 'manualthumb', 'framed', 'frameless',
					'upright', 'border' ),
			);
			static $internalParamMap;
			if ( !$internalParamMap ) {
				$internalParamMap = array();
				foreach ( $internalParamNames as $type => $names ) {
					foreach ( $names as $name ) {
						$magicName = str_replace( '-', '_', "img_$name" );
						$internalParamMap[$magicName] = array( $type, $name );
					}
				}
			}

			// Add handler params
			$paramMap = $internalParamMap;
			if ( $handler ) {
				$handlerParamMap = $handler->getParamMap();
				foreach ( $handlerParamMap as $magic => $paramName ) {
					$paramMap[$magic] = array( 'handler', $paramName );
				}
			}
			$this->mImageParams[$handlerClass] = $paramMap;
			$this->mImageParamsMagicArray[$handlerClass] = new MagicWordArray( array_keys( $paramMap ) );
		}
		return array( $this->mImageParams[$handlerClass], $this->mImageParamsMagicArray[$handlerClass] );
	}

	/**
	 * Parse image options text and use it to make an image
	 */
	function makeImage( $title, $options ) {
		# Check if the options text is of the form "options|alt text"
		# Options are:
		#  * thumbnail       	make a thumbnail with enlarge-icon and caption, alignment depends on lang
		#  * left		no resizing, just left align. label is used for alt= only
		#  * right		same, but right aligned
		#  * none		same, but not aligned
		#  * ___px		scale to ___ pixels width, no aligning. e.g. use in taxobox
		#  * center		center the image
		#  * framed		Keep original image size, no magnify-button.
		#  * frameless		like 'thumb' but without a frame. Keeps user preferences for width
		#  * upright		reduce width for upright images, rounded to full __0 px
		#  * border		draw a 1px border around the image
		# vertical-align values (no % or length right now):
		#  * baseline
		#  * sub
		#  * super
		#  * top
		#  * text-top
		#  * middle
		#  * bottom
		#  * text-bottom

		$parts = array_map( 'trim', explode( '|', $options) );
		$sk = $this->mOptions->getSkin();

		# Give extensions a chance to select the file revision for us
		$skip = $time = $descQuery = false;
		wfRunHooks( 'BeforeParserMakeImageLinkObj', array( &$this, &$title, &$skip, &$time, &$descQuery ) );

		if ( $skip ) {
			return $sk->makeLinkObj( $title );
		}

		# Get parameter map
		$file = wfFindFile( $title, $time );
		$handler = $file ? $file->getHandler() : false;

		list( $paramMap, $mwArray ) = $this->getImageParams( $handler );

		# Process the input parameters
		$caption = '';
		$params = array( 'frame' => array(), 'handler' => array(),
			'horizAlign' => array(), 'vertAlign' => array() );
		foreach( $parts as $part ) {
			list( $magicName, $value ) = $mwArray->matchVariableStartToEnd( $part );
			$validated = false;
			if( isset( $paramMap[$magicName] ) ) {
				list( $type, $paramName ) = $paramMap[$magicName];

				// Special case; width and height come in one variable together
				if( $type == 'handler' && $paramName == 'width' ) {
					$m = array();
					# (bug 13500) In both cases (width/height and width only),
					# permit trailing "px" for backward compatibility.
					if ( preg_match( '/^([0-9]*)x([0-9]*)\s*(?:px)?\s*$/', $value, $m ) ) {
						$width = intval( $m[1] );
						$height = intval( $m[2] );
						if ( $handler->validateParam( 'width', $width ) ) {
							$params[$type]['width'] = $width;
							$validated = true;
						}
						if ( $handler->validateParam( 'height', $height ) ) {
							$params[$type]['height'] = $height;
							$validated = true;
						}
					} elseif ( preg_match( '/^[0-9]*\s*(?:px)?\s*$/', $value ) ) {
						$width = intval( $value );
						if ( $handler->validateParam( 'width', $width ) ) {
							$params[$type]['width'] = $width;
							$validated = true;
						}
					} // else no validation -- bug 13436
				} else {
					if ( $type == 'handler' ) {
						# Validate handler parameter
						$validated = $handler->validateParam( $paramName, $value );
					} else {
						# Validate internal parameters
						switch( $paramName ) {
						case "manualthumb":
							/// @fixme - possibly check validity here?
							/// downstream behavior seems odd with missing manual thumbs.
							$validated = true;
							break;
						default:
							// Most other things appear to be empty or numeric...
							$validated = ( $value === false || is_numeric( trim( $value ) ) );
						}
					}

					if ( $validated ) {
						$params[$type][$paramName] = $value;
					}
				}
			}
			if ( !$validated ) {
				$caption = $part;
			}
		}

		# Process alignment parameters
		if ( $params['horizAlign'] ) {
			$params['frame']['align'] = key( $params['horizAlign'] );
		}
		if ( $params['vertAlign'] ) {
			$params['frame']['valign'] = key( $params['vertAlign'] );
		}

		# Strip bad stuff out of the alt text
		$alt = $this->replaceLinkHoldersText( $caption );

		# make sure there are no placeholders in thumbnail attributes
		# that are later expanded to html- so expand them now and
		# remove the tags
		$alt = $this->mStripState->unstripBoth( $alt );
		$alt = Sanitizer::stripAllTags( $alt );

		$params['frame']['alt'] = $alt;
		$params['frame']['caption'] = $caption;

		wfRunHooks( 'ParserMakeImageParams', array( $title, $file, &$params ) );

		# Linker does the rest
		$ret = $sk->makeImageLink2( $title, $file, $params['frame'], $params['handler'], $time, $descQuery );

		# Give the handler a chance to modify the parser object
		if ( $handler ) {
			$handler->parserTransformHook( $this, $file );
		}

		return $ret;
	}

	/**
	 * Set a flag in the output object indicating that the content is dynamic and
	 * shouldn't be cached.
	 */
	function disableCache() {
		wfDebug( "Parser output marked as uncacheable.\n" );
		$this->mOutput->mCacheTime = -1;
	}

	/**#@+
	 * Callback from the Sanitizer for expanding items found in HTML attribute
	 * values, so they can be safely tested and escaped.
	 * @param string $text
	 * @param PPFrame $frame
	 * @return string
	 * @private
	 */
	function attributeStripCallback( &$text, $frame = false ) {
		$text = $this->replaceVariables( $text, $frame );
		$text = $this->mStripState->unstripBoth( $text );
		return $text;
	}

	/**#@-*/

	/**#@+
	 * Accessor/mutator
	 */
	function Title( $x = NULL ) { return wfSetVar( $this->mTitle, $x ); }
	function Options( $x = NULL ) { return wfSetVar( $this->mOptions, $x ); }
	function OutputType( $x = NULL ) { return wfSetVar( $this->mOutputType, $x ); }
	/**#@-*/

	/**#@+
	 * Accessor
	 */
	function getTags() { return array_merge( array_keys($this->mTransparentTagHooks), array_keys( $this->mTagHooks ) ); }
	/**#@-*/


	/**
	 * Break wikitext input into sections, and either pull or replace
	 * some particular section's text.
	 *
	 * External callers should use the getSection and replaceSection methods.
	 *
	 * @param string $text Page wikitext
	 * @param string $section A section identifier string of the form:
	 *   <flag1> - <flag2> - ... - <section number>
	 *
	 * Currently the only recognised flag is "T", which means the target section number
	 * was derived during a template inclusion parse, in other words this is a template
	 * section edit link. If no flags are given, it was an ordinary section edit link.
	 * This flag is required to avoid a section numbering mismatch when a section is
	 * enclosed by <includeonly> (bug 6563).
	 *
	 * The section number 0 pulls the text before the first heading; other numbers will
	 * pull the given section along with its lower-level subsections. If the section is
	 * not found, $mode=get will return $newtext, and $mode=replace will return $text.
	 *
	 * @param string $mode One of "get" or "replace"
	 * @param string $newText Replacement text for section data.
	 * @return string for "get", the extracted section text.
	 *                for "replace", the whole page with the section replaced.
	 */
	private function extractSections( $text, $section, $mode, $newText='' ) {
		global $wgTitle;
		$this->clearState();
		$this->setTitle( $wgTitle ); // not generally used but removes an ugly failure mode
		$this->mOptions = new ParserOptions;
		$this->setOutputType( self::OT_WIKI );
		$outText = '';
		$frame = $this->getPreprocessor()->newFrame();

		// Process section extraction flags
		$flags = 0;
		$sectionParts = explode( '-', $section );
		$sectionIndex = array_pop( $sectionParts );
		foreach ( $sectionParts as $part ) {
			if ( $part == 'T' ) {
				$flags |= self::PTD_FOR_INCLUSION;
			}
		}
		// Preprocess the text
		$root = $this->preprocessToDom( $text, $flags );

		// <h> nodes indicate section breaks
		// They can only occur at the top level, so we can find them by iterating the root's children
		$node = $root->getFirstChild();

		// Find the target section
		if ( $sectionIndex == 0 ) {
			// Section zero doesn't nest, level=big
			$targetLevel = 1000;
		} else {
            while ( $node ) {
                if ( $node->getName() == 'h' ) {
                    $bits = $node->splitHeading();
					if ( $bits['i'] == $sectionIndex ) {
        				$targetLevel = $bits['level'];
						break;
					}
				}
				if ( $mode == 'replace' ) {
					$outText .= $frame->expand( $node, PPFrame::RECOVER_ORIG );
				}
				$node = $node->getNextSibling();
			}
		}

		if ( !$node ) {
			// Not found
			if ( $mode == 'get' ) {
				return $newText;
			} else {
				return $text;
			}
		}

		// Find the end of the section, including nested sections
		do {
			if ( $node->getName() == 'h' ) {
				$bits = $node->splitHeading();
				$curLevel = $bits['level'];
				if ( $bits['i'] != $sectionIndex && $curLevel <= $targetLevel ) {
					break;
				}
			}
			if ( $mode == 'get' ) {
				$outText .= $frame->expand( $node, PPFrame::RECOVER_ORIG );
			}
			$node = $node->getNextSibling();
		} while ( $node );

		// Write out the remainder (in replace mode only)
		if ( $mode == 'replace' ) {
			// Output the replacement text
			// Add two newlines on -- trailing whitespace in $newText is conventionally
			// stripped by the editor, so we need both newlines to restore the paragraph gap
			$outText .= $newText . "\n\n";
			while ( $node ) {
				$outText .= $frame->expand( $node, PPFrame::RECOVER_ORIG );
				$node = $node->getNextSibling();
			}
		}

		if ( is_string( $outText ) ) {
			// Re-insert stripped tags
			$outText = rtrim( $this->mStripState->unstripBoth( $outText ) );
		}

		return $outText;
	}

	/**
	 * This function returns the text of a section, specified by a number ($section).
	 * A section is text under a heading like == Heading == or \<h1\>Heading\</h1\>, or
	 * the first section before any such heading (section 0).
	 *
	 * If a section contains subsections, these are also returned.
	 *
	 * @param string $text text to look in
	 * @param string $section section identifier
	 * @param string $deftext default to return if section is not found
	 * @return string text of the requested section
	 */
	public function getSection( $text, $section, $deftext='' ) {
		return $this->extractSections( $text, $section, "get", $deftext );
	}

	public function replaceSection( $oldtext, $section, $text ) {
		return $this->extractSections( $oldtext, $section, "replace", $text );
	}

	/**
	 * Get the timestamp associated with the current revision, adjusted for
	 * the default server-local timestamp
	 */
	function getRevisionTimestamp() {
		if ( is_null( $this->mRevisionTimestamp ) ) {
			wfProfileIn( __METHOD__ );
			global $wgContLang;
			$dbr = wfGetDB( DB_SLAVE );
			$timestamp = $dbr->selectField( 'revision', 'rev_timestamp',
					array( 'rev_id' => $this->mRevisionId ), __METHOD__ );

			// Normalize timestamp to internal MW format for timezone processing.
			// This has the added side-effect of replacing a null value with
			// the current time, which gives us more sensible behavior for
			// previews.
			$timestamp = wfTimestamp( TS_MW, $timestamp );

			// The cryptic '' timezone parameter tells to use the site-default
			// timezone offset instead of the user settings.
			//
			// Since this value will be saved into the parser cache, served
			// to other users, and potentially even used inside links and such,
			// it needs to be consistent for all visitors.
			$this->mRevisionTimestamp = $wgContLang->userAdjust( $timestamp, '' );

			wfProfileOut( __METHOD__ );
		}
		return $this->mRevisionTimestamp;
	}

	/**
	 * Mutator for $mDefaultSort
	 *
	 * @param $sort New value
	 */
	public function setDefaultSort( $sort ) {
		$this->mDefaultSort = $sort;
	}

	/**
	 * Accessor for $mDefaultSort
	 * Will use the title/prefixed title if none is set
	 *
	 * @return string
	 */
	public function getDefaultSort() {
		if( $this->mDefaultSort !== false ) {
			return $this->mDefaultSort;
		} else {
			return $this->mTitle->getNamespace() == NS_CATEGORY
					? $this->mTitle->getText()
					: $this->mTitle->getPrefixedText();
		}
	}

	/**
	 * Try to guess the section anchor name based on a wikitext fragment
	 * presumably extracted from a heading, for example "Header" from
	 * "== Header ==".
	 */
	public function guessSectionNameFromWikiText( $text ) {
		# Strip out wikitext links(they break the anchor)
		$text = $this->stripSectionName( $text );
		$headline = Sanitizer::decodeCharReferences( $text );
		# strip out HTML
		$headline = StringUtils::delimiterReplace( '<', '>', '', $headline );
		$headline = trim( $headline );
		$sectionanchor = '#' . urlencode( str_replace( ' ', '_', $headline ) );
		$replacearray = array(
			'%3A' => ':',
			'%' => '.'
		);
		return str_replace(
			array_keys( $replacearray ),
			array_values( $replacearray ),
			$sectionanchor );
	}

	/**
	 * Strips a text string of wikitext for use in a section anchor
	 *
	 * Accepts a text string and then removes all wikitext from the
	 * string and leaves only the resultant text (i.e. the result of
	 * [[User:WikiSysop|Sysop]] would be "Sysop" and the result of
	 * [[User:WikiSysop]] would be "User:WikiSysop") - this is intended
	 * to create valid section anchors by mimicing the output of the
	 * parser when headings are parsed.
	 *
	 * @param $text string Text string to be stripped of wikitext
	 * for use in a Section anchor
	 * @return Filtered text string
	 */
	public function stripSectionName( $text ) {
		# Strip internal link markup
		$text = preg_replace('/\[\[:?([^[|]+)\|([^[]+)\]\]/','$2',$text);
		$text = preg_replace('/\[\[:?([^[]+)\|?\]\]/','$1',$text);

		# Strip external link markup (FIXME: Not Tolerant to blank link text
		# I.E. [http://www.mediawiki.org] will render as [1] or something depending
		# on how many empty links there are on the page - need to figure that out.
		$text = preg_replace('/\[(?:' . wfUrlProtocols() . ')([^ ]+?) ([^[]+)\]/','$2',$text);

		# Parse wikitext quotes (italics & bold)
		$text = $this->doQuotes($text);

		# Strip HTML tags
		$text = StringUtils::delimiterReplace( '<', '>', '', $text );
		return $text;
	}

	function srvus( $text ) {
		return $this->testSrvus( $text, $this->mOutputType );
	}

	/**
	 * strip/replaceVariables/unstrip for preprocessor regression testing
	 */
	function testSrvus( $text, $title, $options, $outputType = self::OT_HTML ) {
		$this->clearState();
		if ( ! ( $title instanceof Title ) ) {
			$title = Title::newFromText( $title );
		}
		$this->mTitle = $title;
		$this->mOptions = $options;
		$this->setOutputType( $outputType );
		$text = $this->replaceVariables( $text );
		$text = $this->mStripState->unstripBoth( $text );
		$text = Sanitizer::removeHTMLtags( $text );
		return $text;
	}

	function testPst( $text, $title, $options ) {
		global $wgUser;
		if ( ! ( $title instanceof Title ) ) {
			$title = Title::newFromText( $title );
		}
		return $this->preSaveTransform( $text, $title, $wgUser, $options );
	}

	function testPreprocess( $text, $title, $options ) {
		if ( ! ( $title instanceof Title ) ) {
			$title = Title::newFromText( $title );
		}
		return $this->testSrvus( $text, $title, $options, self::OT_PREPROCESS );
	}

	function markerSkipCallback( $s, $callback ) {
		$i = 0;
		$out = '';
		while ( $i < strlen( $s ) ) {
			$markerStart = strpos( $s, $this->mUniqPrefix, $i );
			if ( $markerStart === false ) {
				$out .= call_user_func( $callback, substr( $s, $i ) );
				break;
			} else {
				$out .= call_user_func( $callback, substr( $s, $i, $markerStart - $i ) );
				$markerEnd = strpos( $s, self::MARKER_SUFFIX, $markerStart );
				if ( $markerEnd === false ) {
					$out .= substr( $s, $markerStart );
					break;
				} else {
					$markerEnd += strlen( self::MARKER_SUFFIX );
					$out .= substr( $s, $markerStart, $markerEnd - $markerStart );
					$i = $markerEnd;
				}
			}
		}
		return $out;
	}
}

/**
 * @todo document, briefly.
 * @ingroup Parser
 */
class StripState {
	var $general, $nowiki;

	function __construct() {
		$this->general = new ReplacementArray;
		$this->nowiki = new ReplacementArray;
	}

	function unstripGeneral( $text ) {
		wfProfileIn( __METHOD__ );
		do {
			$oldText = $text;
			$text = $this->general->replace( $text );
		} while ( $text != $oldText );
		wfProfileOut( __METHOD__ );
		return $text;
	}

	function unstripNoWiki( $text ) {
		wfProfileIn( __METHOD__ );
		do {
			$oldText = $text;
			$text = $this->nowiki->replace( $text );
		} while ( $text != $oldText );
		wfProfileOut( __METHOD__ );
		return $text;
	}

	function unstripBoth( $text ) {
		wfProfileIn( __METHOD__ );
		do {
			$oldText = $text;
			$text = $this->general->replace( $text );
			$text = $this->nowiki->replace( $text );
		} while ( $text != $oldText );
		wfProfileOut( __METHOD__ );
		return $text;
	}
}

/**
 * @todo document, briefly.
 * @ingroup Parser
 */
class OnlyIncludeReplacer {
	var $output = '';

	function replace( $matches ) {
		if ( substr( $matches[1], -1 ) == "\n" ) {
			$this->output .= substr( $matches[1], 0, -1 );
		} else {
			$this->output .= $matches[1];
		}
	}
}
