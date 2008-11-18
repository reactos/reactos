<?php

/**
 * Various core parser functions, registered in Parser::firstCallInit()
 * @ingroup Parser
 */
class CoreParserFunctions {
	static function register( $parser ) {
		global $wgAllowDisplayTitle, $wgAllowSlowParserFunctions;

		# Syntax for arguments (see self::setFunctionHook):
		#  "name for lookup in localized magic words array",
		#  function callback,
		#  optional SFH_NO_HASH to omit the hash from calls (e.g. {{int:...}
		#    instead of {{#int:...}})

		$parser->setFunctionHook( 'int',              array( __CLASS__, 'intFunction'      ), SFH_NO_HASH );
		$parser->setFunctionHook( 'ns',               array( __CLASS__, 'ns'               ), SFH_NO_HASH );
		$parser->setFunctionHook( 'urlencode',        array( __CLASS__, 'urlencode'        ), SFH_NO_HASH );
		$parser->setFunctionHook( 'lcfirst',          array( __CLASS__, 'lcfirst'          ), SFH_NO_HASH );
		$parser->setFunctionHook( 'ucfirst',          array( __CLASS__, 'ucfirst'          ), SFH_NO_HASH );
		$parser->setFunctionHook( 'lc',               array( __CLASS__, 'lc'               ), SFH_NO_HASH );
		$parser->setFunctionHook( 'uc',               array( __CLASS__, 'uc'               ), SFH_NO_HASH );
		$parser->setFunctionHook( 'localurl',         array( __CLASS__, 'localurl'         ), SFH_NO_HASH );
		$parser->setFunctionHook( 'localurle',        array( __CLASS__, 'localurle'        ), SFH_NO_HASH );
		$parser->setFunctionHook( 'fullurl',          array( __CLASS__, 'fullurl'          ), SFH_NO_HASH );
		$parser->setFunctionHook( 'fullurle',         array( __CLASS__, 'fullurle'         ), SFH_NO_HASH );
		$parser->setFunctionHook( 'formatnum',        array( __CLASS__, 'formatnum'        ), SFH_NO_HASH );
		$parser->setFunctionHook( 'grammar',          array( __CLASS__, 'grammar'          ), SFH_NO_HASH );
		$parser->setFunctionHook( 'plural',           array( __CLASS__, 'plural'           ), SFH_NO_HASH );
		$parser->setFunctionHook( 'numberofpages',    array( __CLASS__, 'numberofpages'    ), SFH_NO_HASH );
		$parser->setFunctionHook( 'numberofusers',    array( __CLASS__, 'numberofusers'    ), SFH_NO_HASH );
		$parser->setFunctionHook( 'numberofarticles', array( __CLASS__, 'numberofarticles' ), SFH_NO_HASH );
		$parser->setFunctionHook( 'numberoffiles',    array( __CLASS__, 'numberoffiles'    ), SFH_NO_HASH );
		$parser->setFunctionHook( 'numberofadmins',   array( __CLASS__, 'numberofadmins'   ), SFH_NO_HASH );
		$parser->setFunctionHook( 'numberofedits',    array( __CLASS__, 'numberofedits'    ), SFH_NO_HASH );
		$parser->setFunctionHook( 'language',         array( __CLASS__, 'language'         ), SFH_NO_HASH );
		$parser->setFunctionHook( 'padleft',          array( __CLASS__, 'padleft'          ), SFH_NO_HASH );
		$parser->setFunctionHook( 'padright',         array( __CLASS__, 'padright'         ), SFH_NO_HASH );
		$parser->setFunctionHook( 'anchorencode',     array( __CLASS__, 'anchorencode'     ), SFH_NO_HASH );
		$parser->setFunctionHook( 'special',          array( __CLASS__, 'special'          ) );
		$parser->setFunctionHook( 'defaultsort',      array( __CLASS__, 'defaultsort'      ), SFH_NO_HASH );
		$parser->setFunctionHook( 'filepath',         array( __CLASS__, 'filepath'         ), SFH_NO_HASH );
		$parser->setFunctionHook( 'pagesincategory',  array( __CLASS__, 'pagesincategory'  ), SFH_NO_HASH );
		$parser->setFunctionHook( 'pagesize',         array( __CLASS__, 'pagesize'         ), SFH_NO_HASH );
		$parser->setFunctionHook( 'tag',              array( __CLASS__, 'tagObj'           ), SFH_OBJECT_ARGS );

		if ( $wgAllowDisplayTitle ) {
			$parser->setFunctionHook( 'displaytitle', array( __CLASS__, 'displaytitle' ), SFH_NO_HASH );
		}
		if ( $wgAllowSlowParserFunctions ) {
			$parser->setFunctionHook( 'pagesinnamespace', array( __CLASS__, 'pagesinnamespace' ), SFH_NO_HASH );
		}
	}

	static function intFunction( $parser, $part1 = '' /*, ... */ ) {
		if ( strval( $part1 ) !== '' ) {
			$args = array_slice( func_get_args(), 2 );
			return wfMsgReal( $part1, $args, true );
		} else {
			return array( 'found' => false );
		}
	}

	static function ns( $parser, $part1 = '' ) {
		global $wgContLang;
		$found = false;
		if ( intval( $part1 ) || $part1 == "0" ) {
			$text = $wgContLang->getNsText( intval( $part1 ) );
			$found = true;
		} else {
			$param = str_replace( ' ', '_', strtolower( $part1 ) );
			$index = MWNamespace::getCanonicalIndex( strtolower( $param ) );
			if ( !is_null( $index ) ) {
				$text = $wgContLang->getNsText( $index );
				$found = true;
			}
		}
		if ( $found ) {
			return $text;
		} else {
			return array( 'found' => false );
		}
	}

	static function urlencode( $parser, $s = '' ) {
		return urlencode( $s );
	}

	static function lcfirst( $parser, $s = '' ) {
		global $wgContLang;
		return $wgContLang->lcfirst( $s );
	}

	static function ucfirst( $parser, $s = '' ) {
		global $wgContLang;
		return $wgContLang->ucfirst( $s );
	}

	static function lc( $parser, $s = '' ) {
		global $wgContLang;
		if ( is_callable( array( $parser, 'markerSkipCallback' ) ) ) {
			return $parser->markerSkipCallback( $s, array( $wgContLang, 'lc' ) );
		} else {
			return $wgContLang->lc( $s );
		}
	}

	static function uc( $parser, $s = '' ) {
		global $wgContLang;
		if ( is_callable( array( $parser, 'markerSkipCallback' ) ) ) {
			return $parser->markerSkipCallback( $s, array( $wgContLang, 'uc' ) );
		} else {
			return $wgContLang->uc( $s );
		}
	}

	static function localurl( $parser, $s = '', $arg = null ) { return self::urlFunction( 'getLocalURL', $s, $arg ); }
	static function localurle( $parser, $s = '', $arg = null ) { return self::urlFunction( 'escapeLocalURL', $s, $arg ); }
	static function fullurl( $parser, $s = '', $arg = null ) { return self::urlFunction( 'getFullURL', $s, $arg ); }
	static function fullurle( $parser, $s = '', $arg = null ) { return self::urlFunction( 'escapeFullURL', $s, $arg ); }

	static function urlFunction( $func, $s = '', $arg = null ) {
		$title = Title::newFromText( $s );
		# Due to order of execution of a lot of bits, the values might be encoded
		# before arriving here; if that's true, then the title can't be created
		# and the variable will fail. If we can't get a decent title from the first
		# attempt, url-decode and try for a second.
		if( is_null( $title ) )
			$title = Title::newFromUrl( urldecode( $s ) );
		if ( !is_null( $title ) ) {
			if ( !is_null( $arg ) ) {
				$text = $title->$func( $arg );
			} else {
				$text = $title->$func();
			}
			return $text;
		} else {
			return array( 'found' => false );
		}
	}

	static function formatNum( $parser, $num = '', $raw = null) {
		if ( self::israw( $raw ) ) {
			return $parser->getFunctionLang()->parseFormattedNumber( $num );
		} else {
			return $parser->getFunctionLang()->formatNum( $num );
		}
	}

	static function grammar( $parser, $case = '', $word = '' ) {
		return $parser->getFunctionLang()->convertGrammar( $word, $case );
	}

	static function plural( $parser, $text = '') {
		$forms = array_slice( func_get_args(), 2);
		$text = $parser->getFunctionLang()->parseFormattedNumber( $text );
		return $parser->getFunctionLang()->convertPlural( $text, $forms );
	}

	/**
	 * Override the title of the page when viewed, provided we've been given a
	 * title which will normalise to the canonical title
	 *
	 * @param Parser $parser Parent parser
	 * @param string $text Desired title text
	 * @return string
	 */
	static function displaytitle( $parser, $text = '' ) {
		$text = trim( Sanitizer::decodeCharReferences( $text ) );
		$title = Title::newFromText( $text );
		if( $title instanceof Title && $title->getFragment() == '' && $title->equals( $parser->mTitle ) )
			$parser->mOutput->setDisplayTitle( $text );
		return '';
	}

	static function isRaw( $param ) {
		static $mwRaw;
		if ( !$mwRaw ) {
			$mwRaw =& MagicWord::get( 'rawsuffix' );
		}
		if ( is_null( $param ) ) {
			return false;
		} else {
			return $mwRaw->match( $param );
		}
	}

	static function formatRaw( $num, $raw ) {
		if( self::isRaw( $raw ) ) {
			return $num;
		} else {
			global $wgContLang;
			return $wgContLang->formatNum( $num );
		}
	}
	static function numberofpages( $parser, $raw = null ) {
		return self::formatRaw( SiteStats::pages(), $raw );
	}
	static function numberofusers( $parser, $raw = null ) {
		return self::formatRaw( SiteStats::users(), $raw );
	}
	static function numberofarticles( $parser, $raw = null ) {
		return self::formatRaw( SiteStats::articles(), $raw );
	}
	static function numberoffiles( $parser, $raw = null ) {
		return self::formatRaw( SiteStats::images(), $raw );
	}
	static function numberofadmins( $parser, $raw = null ) {
		return self::formatRaw( SiteStats::admins(), $raw );
	}
	static function numberofedits( $parser, $raw = null ) {
		return self::formatRaw( SiteStats::edits(), $raw );
	}
	static function pagesinnamespace( $parser, $namespace = 0, $raw = null ) {
		return self::formatRaw( SiteStats::pagesInNs( intval( $namespace ) ), $raw );
	}

	/**
	 * Return the number of pages in the given category, or 0 if it's nonexis-
	 * tent.  This is an expensive parser function and can't be called too many
	 * times per page.
	 */
	static function pagesincategory( $parser, $name = '', $raw = null ) {
		static $cache = array();
		$category = Category::newFromName( $name );

		if( !is_object( $category ) ) {
			$cache[$name] = 0;
			return self::formatRaw( 0, $raw );
		}

		# Normalize name for cache
		$name = $category->getName();

		$count = 0;
		if( isset( $cache[$name] ) ) {
			$count = $cache[$name];
		} elseif( $parser->incrementExpensiveFunctionCount() ) {
			$count = $cache[$name] = (int)$category->getPageCount();
		}
		return self::formatRaw( $count, $raw );
	}

	/**
	 * Return the size of the given page, or 0 if it's nonexistent.  This is an
	 * expensive parser function and can't be called too many times per page.
	 *
	 * @FIXME This doesn't work correctly on preview for getting the size of
	 *   the current page.
	 * @FIXME Title::getLength() documentation claims that it adds things to
	 *   the link cache, so the local cache here should be unnecessary, but in
	 *   fact calling getLength() repeatedly for the same $page does seem to
	 *   run one query for each call?
	 */
	static function pagesize( $parser, $page = '', $raw = null ) {
		static $cache = array();
		$title = Title::newFromText($page);

		if( !is_object( $title ) ) {
			$cache[$page] = 0;
			return self::formatRaw( 0, $raw );
		}

		# Normalize name for cache
		$page = $title->getPrefixedText();

		$length = 0;
		if( isset( $cache[$page] ) ) {
			$length = $cache[$page];
		} elseif( $parser->incrementExpensiveFunctionCount() ) {
			$length = $cache[$page] = $title->getLength();
	
			// Register dependency in templatelinks
			$id = $title->getArticleId();
			$revid = Revision::newFromTitle($title);
			$parser->mOutput->addTemplate($title, $id, $revid);
		}	
		return self::formatRaw( $length, $raw );
	}

	static function language( $parser, $arg = '' ) {
		global $wgContLang;
		$lang = $wgContLang->getLanguageName( strtolower( $arg ) );
		return $lang != '' ? $lang : $arg;
	}

	static function pad( $string = '', $length = 0, $char = 0, $direction = STR_PAD_RIGHT ) {
		$length = min( max( $length, 0 ), 500 );
		$char = substr( $char, 0, 1 );
		return ( $string !== '' && (int)$length > 0 && strlen( trim( (string)$char ) ) > 0 )
				? str_pad( $string, $length, (string)$char, $direction )
				: $string;
	}

	static function padleft( $parser, $string = '', $length = 0, $char = 0 ) {
		return self::pad( $string, $length, $char, STR_PAD_LEFT );
	}

	static function padright( $parser, $string = '', $length = 0, $char = 0 ) {
		return self::pad( $string, $length, $char );
	}

	static function anchorencode( $parser, $text ) {
		$a = urlencode( $text );
		$a = strtr( $a, array( '%' => '.', '+' => '_' ) );
		# leave colons alone, however
		$a = str_replace( '.3A', ':', $a );
		return $a;
	}

	static function special( $parser, $text ) {
		$title = SpecialPage::getTitleForAlias( $text );
		if ( $title ) {
			return $title->getPrefixedText();
		} else {
			return wfMsgForContent( 'nosuchspecialpage' );
		}
	}

	public static function defaultsort( $parser, $text ) {
		$text = trim( $text );
		if( strlen( $text ) > 0 )
			$parser->setDefaultSort( $text );
		return '';
	}

	public static function filepath( $parser, $name='', $option='' ) {
		$file = wfFindFile( $name );
		if( $file ) {
			$url = $file->getFullUrl();
			if( $option == 'nowiki' ) {
				return "<nowiki>$url</nowiki>";
			}
			return $url;
		} else {
			return '';
		}
	}

	/**
	 * Parser function to extension tag adaptor
	 */
	public static function tagObj( $parser, $frame, $args ) {
		$xpath = false;
		if ( !count( $args ) ) {
			return '';
		}
		$tagName = strtolower( trim( $frame->expand( array_shift( $args ) ) ) );

		if ( count( $args ) ) {
			$inner = $frame->expand( array_shift( $args ) );
		} else {
			$inner = null;
		}

		$stripList = $parser->getStripList();
		if ( !in_array( $tagName, $stripList ) ) {
			return '<span class="error">' .
				wfMsg( 'unknown_extension_tag', $tagName ) .
				'</span>';
		}

		$attributes = array();
		foreach ( $args as $arg ) {
			$bits = $arg->splitArg();
			if ( strval( $bits['index'] ) === '' ) {
				$name = $frame->expand( $bits['name'], PPFrame::STRIP_COMMENTS );
				$value = trim( $frame->expand( $bits['value'] ) );
				if ( preg_match( '/^(?:["\'](.+)["\']|""|\'\')$/s', $value, $m ) ) {
					$value = isset( $m[1] ) ? $m[1] : '';
				}
				$attributes[$name] = $value;
			}
		}

		$params = array(
			'name' => $tagName,
			'inner' => $inner,
			'attributes' => $attributes,
			'close' => "</$tagName>",
		);
		return $parser->extensionSubstitution( $params, $frame );
	}
}
