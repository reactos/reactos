<?php
/**
 * @defgroup Language Language
 *
 * @file
 * @ingroup Language
 */

if( !defined( 'MEDIAWIKI' ) ) {
	echo "This file is part of MediaWiki, it is not a valid entry point.\n";
	exit( 1 );
}

# Read language names
global $wgLanguageNames;
require_once( dirname(__FILE__) . '/Names.php' ) ;

global $wgInputEncoding, $wgOutputEncoding;

/**
 * These are always UTF-8, they exist only for backwards compatibility
 */
$wgInputEncoding    = "UTF-8";
$wgOutputEncoding	= "UTF-8";

if( function_exists( 'mb_strtoupper' ) ) {
	mb_internal_encoding('UTF-8');
}

/**
 * a fake language converter
 *
 * @ingroup Language
 */
class FakeConverter {
	var $mLang;
	function FakeConverter($langobj) {$this->mLang = $langobj;}
	function convert($t, $i) {return $t;}
	function parserConvert($t, $p) {return $t;}
	function getVariants() { return array( $this->mLang->getCode() ); }
	function getPreferredVariant() {return $this->mLang->getCode(); }
	function findVariantLink(&$l, &$n) {}
	function getExtraHashOptions() {return '';}
	function getParsedTitle() {return '';}
	function markNoConversion($text, $noParse=false) {return $text;}
	function convertCategoryKey( $key ) {return $key; }
	function convertLinkToAllVariants($text){ return array( $this->mLang->getCode() => $text); }
	function armourMath($text){ return $text; }
}

/**
 * Internationalisation code
 * @ingroup Language
 */
class Language {
	var $mConverter, $mVariants, $mCode, $mLoaded = false;
	var $mMagicExtensions = array(), $mMagicHookDone = false;

	static public $mLocalisationKeys = array( 'fallback', 'namespaceNames',
		'skinNames', 'mathNames',
		'bookstoreList', 'magicWords', 'messages', 'rtl', 'digitTransformTable',
		'separatorTransformTable', 'fallback8bitEncoding', 'linkPrefixExtension',
		'defaultUserOptionOverrides', 'linkTrail', 'namespaceAliases',
		'dateFormats', 'datePreferences', 'datePreferenceMigrationMap',
		'defaultDateFormat', 'extraUserToggles', 'specialPageAliases',
		'imageFiles'
	);

	static public $mMergeableMapKeys = array( 'messages', 'namespaceNames', 'mathNames',
		'dateFormats', 'defaultUserOptionOverrides', 'magicWords', 'imageFiles' );

	static public $mMergeableListKeys = array( 'extraUserToggles' );

	static public $mMergeableAliasListKeys = array( 'specialPageAliases' );

	static public $mLocalisationCache = array();

	static public $mWeekdayMsgs = array(
		'sunday', 'monday', 'tuesday', 'wednesday', 'thursday',
		'friday', 'saturday'
	);

	static public $mWeekdayAbbrevMsgs = array(
		'sun', 'mon', 'tue', 'wed', 'thu', 'fri', 'sat'
	);

	static public $mMonthMsgs = array(
		'january', 'february', 'march', 'april', 'may_long', 'june',
		'july', 'august', 'september', 'october', 'november',
		'december'
	);
	static public $mMonthGenMsgs = array(
		'january-gen', 'february-gen', 'march-gen', 'april-gen', 'may-gen', 'june-gen',
		'july-gen', 'august-gen', 'september-gen', 'october-gen', 'november-gen',
		'december-gen'
	);
	static public $mMonthAbbrevMsgs = array(
		'jan', 'feb', 'mar', 'apr', 'may', 'jun', 'jul', 'aug',
		'sep', 'oct', 'nov', 'dec'
	);

	static public $mIranianCalendarMonthMsgs = array(
		'iranian-calendar-m1', 'iranian-calendar-m2', 'iranian-calendar-m3',
		'iranian-calendar-m4', 'iranian-calendar-m5', 'iranian-calendar-m6',
		'iranian-calendar-m7', 'iranian-calendar-m8', 'iranian-calendar-m9',
		'iranian-calendar-m10', 'iranian-calendar-m11', 'iranian-calendar-m12'
	);

	static public $mHebrewCalendarMonthMsgs = array(
		'hebrew-calendar-m1', 'hebrew-calendar-m2', 'hebrew-calendar-m3',
		'hebrew-calendar-m4', 'hebrew-calendar-m5', 'hebrew-calendar-m6',
		'hebrew-calendar-m7', 'hebrew-calendar-m8', 'hebrew-calendar-m9',
		'hebrew-calendar-m10', 'hebrew-calendar-m11', 'hebrew-calendar-m12',
		'hebrew-calendar-m6a', 'hebrew-calendar-m6b'
	);

	static public $mHebrewCalendarMonthGenMsgs = array(
		'hebrew-calendar-m1-gen', 'hebrew-calendar-m2-gen', 'hebrew-calendar-m3-gen',
		'hebrew-calendar-m4-gen', 'hebrew-calendar-m5-gen', 'hebrew-calendar-m6-gen',
		'hebrew-calendar-m7-gen', 'hebrew-calendar-m8-gen', 'hebrew-calendar-m9-gen',
		'hebrew-calendar-m10-gen', 'hebrew-calendar-m11-gen', 'hebrew-calendar-m12-gen',
		'hebrew-calendar-m6a-gen', 'hebrew-calendar-m6b-gen'
	);
	
	static public $mHijriCalendarMonthMsgs = array(
		'hijri-calendar-m1', 'hijri-calendar-m2', 'hijri-calendar-m3',
		'hijri-calendar-m4', 'hijri-calendar-m5', 'hijri-calendar-m6',
		'hijri-calendar-m7', 'hijri-calendar-m8', 'hijri-calendar-m9',
		'hijri-calendar-m10', 'hijri-calendar-m11', 'hijri-calendar-m12'
	);

	/**
	 * Create a language object for a given language code
	 */
	static function factory( $code ) {
		global $IP;
		static $recursionLevel = 0;

		if ( $code == 'en' ) {
			$class = 'Language';
		} else {
			$class = 'Language' . str_replace( '-', '_', ucfirst( $code ) );
			// Preload base classes to work around APC/PHP5 bug
			if ( file_exists( "$IP/languages/classes/$class.deps.php" ) ) {
				include_once("$IP/languages/classes/$class.deps.php");
			}
			if ( file_exists( "$IP/languages/classes/$class.php" ) ) {
				include_once("$IP/languages/classes/$class.php");
			}
		}

		if ( $recursionLevel > 5 ) {
			throw new MWException( "Language fallback loop detected when creating class $class\n" );
		}	

		if( ! class_exists( $class ) ) {
			$fallback = Language::getFallbackFor( $code );
			++$recursionLevel;
			$lang = Language::factory( $fallback );
			--$recursionLevel;
			$lang->setCode( $code );
		} else {
			$lang = new $class;
		}

		return $lang;
	}

	function __construct() {
		$this->mConverter = new FakeConverter($this);
		// Set the code to the name of the descendant
		if ( get_class( $this ) == 'Language' ) {
			$this->mCode = 'en';
		} else {
			$this->mCode = str_replace( '_', '-', strtolower( substr( get_class( $this ), 8 ) ) );
		}
	}

	/**
	 * Hook which will be called if this is the content language.
	 * Descendants can use this to register hook functions or modify globals
	 */
	function initContLang() {}

	/**
	 * @deprecated Use User::getDefaultOptions()
	 * @return array
	 */
	function getDefaultUserOptions() {
		wfDeprecated( __METHOD__ );
		return User::getDefaultOptions();
	}

	function getFallbackLanguageCode() {
		return self::getFallbackFor( $this->mCode );
	}

	/**
	 * Exports $wgBookstoreListEn
	 * @return array
	 */
	function getBookstoreList() {
		$this->load();
		return $this->bookstoreList;
	}

	/**
	 * @return array
	 */
	function getNamespaces() {
		$this->load();
		return $this->namespaceNames;
	}

	/**
	 * A convenience function that returns the same thing as
	 * getNamespaces() except with the array values changed to ' '
	 * where it found '_', useful for producing output to be displayed
	 * e.g. in <select> forms.
	 *
	 * @return array
	 */
	function getFormattedNamespaces() {
		$ns = $this->getNamespaces();
		foreach($ns as $k => $v) {
			$ns[$k] = strtr($v, '_', ' ');
		}
		return $ns;
	}

	/**
	 * Get a namespace value by key
	 * <code>
	 * $mw_ns = $wgContLang->getNsText( NS_MEDIAWIKI );
	 * echo $mw_ns; // prints 'MediaWiki'
	 * </code>
	 *
	 * @param $index Int: the array key of the namespace to return
	 * @return mixed, string if the namespace value exists, otherwise false
	 */
	function getNsText( $index ) {
		$ns = $this->getNamespaces();
		return isset( $ns[$index] ) ? $ns[$index] : false;
	}

	/**
	 * A convenience function that returns the same thing as
	 * getNsText() except with '_' changed to ' ', useful for
	 * producing output.
	 *
	 * @return array
	 */
	function getFormattedNsText( $index ) {
		$ns = $this->getNsText( $index );
		return strtr($ns, '_', ' ');
	}

	/**
	 * Get a namespace key by value, case insensitive.
	 * Only matches namespace names for the current language, not the
	 * canonical ones defined in Namespace.php.
	 *
	 * @param $text String
	 * @return mixed An integer if $text is a valid value otherwise false
	 */
	function getLocalNsIndex( $text ) {
		$this->load();
		$lctext = $this->lc($text);
		return isset( $this->mNamespaceIds[$lctext] ) ? $this->mNamespaceIds[$lctext] : false;
	}

	/**
	 * Get a namespace key by value, case insensitive.  Canonical namespace
	 * names override custom ones defined for the current language.
	 *
	 * @param $text String
	 * @return mixed An integer if $text is a valid value otherwise false
	 */
	function getNsIndex( $text ) {
		$this->load();
		$lctext = $this->lc($text);
		if( ( $ns = MWNamespace::getCanonicalIndex( $lctext ) ) !== null ) return $ns;
		return isset( $this->mNamespaceIds[$lctext] ) ? $this->mNamespaceIds[$lctext] : false;
	}

	/**
	 * short names for language variants used for language conversion links.
	 *
	 * @param $code String
	 * @return string
	 */
	function getVariantname( $code ) {
		return $this->getMessageFromDB( "variantname-$code" );
	}

	function specialPage( $name ) {
		$aliases = $this->getSpecialPageAliases();
		if ( isset( $aliases[$name][0] ) ) {
			$name = $aliases[$name][0];
		}
		return $this->getNsText(NS_SPECIAL) . ':' . $name;
	}

	function getQuickbarSettings() {
		return array(
			$this->getMessage( 'qbsettings-none' ),
			$this->getMessage( 'qbsettings-fixedleft' ),
			$this->getMessage( 'qbsettings-fixedright' ),
			$this->getMessage( 'qbsettings-floatingleft' ),
			$this->getMessage( 'qbsettings-floatingright' )
		);
	}

	function getSkinNames() {
		$this->load();
		return $this->skinNames;
	}

	function getMathNames() {
		$this->load();
		return $this->mathNames;
	}

	function getDatePreferences() {
		$this->load();
		return $this->datePreferences;
	}
	
	function getDateFormats() {
		$this->load();
		return $this->dateFormats;
	}

	function getDefaultDateFormat() {
		$this->load();
		return $this->defaultDateFormat;
	}

	function getDatePreferenceMigrationMap() {
		$this->load();
		return $this->datePreferenceMigrationMap;
	}

	function getImageFile( $image ) {
		$this->load();
		return $this->imageFiles[$image];
	}

	function getDefaultUserOptionOverrides() {
		$this->load();
		# XXX - apparently some languageas get empty arrays, didn't get to it yet -- midom
		if (is_array($this->defaultUserOptionOverrides)) {
			return $this->defaultUserOptionOverrides;
		} else {
			return array();
		}
	}

	function getExtraUserToggles() {
		$this->load();
		return $this->extraUserToggles;
	}

	function getUserToggle( $tog ) {
		return $this->getMessageFromDB( "tog-$tog" );
	}

	/**
	 * Get language names, indexed by code.
	 * If $customisedOnly is true, only returns codes with a messages file
	 */
	public static function getLanguageNames( $customisedOnly = false ) {
		global $wgLanguageNames, $wgExtraLanguageNames;
		$allNames = $wgExtraLanguageNames + $wgLanguageNames;
		if ( !$customisedOnly ) {
			return $allNames;
		}
		
		global $IP;
		$names = array();
		$dir = opendir( "$IP/languages/messages" );
		while( false !== ( $file = readdir( $dir ) ) ) {
			$m = array();
			if( preg_match( '/Messages([A-Z][a-z_]+)\.php$/', $file, $m ) ) {
				$code = str_replace( '_', '-', strtolower( $m[1] ) );
				if ( isset( $allNames[$code] ) ) {
					$names[$code] = $allNames[$code];
				}
			}
		}
		closedir( $dir );
		return $names;
	}

	/**
	 * Ugly hack to get a message maybe from the MediaWiki namespace, if this
	 * language object is the content or user language.
	 */
	function getMessageFromDB( $msg ) {
		global $wgContLang, $wgLang;
		if ( $wgContLang->getCode() == $this->getCode() ) {
			# Content language
			return wfMsgForContent( $msg );
		} elseif ( $wgLang->getCode() == $this->getCode() ) {
			# User language
			return wfMsg( $msg );
		} else {
			# Neither, get from localisation
			return $this->getMessage( $msg );
		}
	}

	function getLanguageName( $code ) {
		$names = self::getLanguageNames();
		if ( !array_key_exists( $code, $names ) ) {
			return '';
		}
		return $names[$code];
	}

	function getMonthName( $key ) {
		return $this->getMessageFromDB( self::$mMonthMsgs[$key-1] );
	}

	function getMonthNameGen( $key ) {
		return $this->getMessageFromDB( self::$mMonthGenMsgs[$key-1] );
	}

	function getMonthAbbreviation( $key ) {
		return $this->getMessageFromDB( self::$mMonthAbbrevMsgs[$key-1] );
	}

	function getWeekdayName( $key ) {
		return $this->getMessageFromDB( self::$mWeekdayMsgs[$key-1] );
	}

	function getWeekdayAbbreviation( $key ) {
		return $this->getMessageFromDB( self::$mWeekdayAbbrevMsgs[$key-1] );
	}

	function getIranianCalendarMonthName( $key ) {
		return $this->getMessageFromDB( self::$mIranianCalendarMonthMsgs[$key-1] );
	}

	function getHebrewCalendarMonthName( $key ) {
		return $this->getMessageFromDB( self::$mHebrewCalendarMonthMsgs[$key-1] );
	}

	function getHebrewCalendarMonthNameGen( $key ) {
		return $this->getMessageFromDB( self::$mHebrewCalendarMonthGenMsgs[$key-1] );
	}
	
	function getHijriCalendarMonthName( $key ) {
		return $this->getMessageFromDB( self::$mHijriCalendarMonthMsgs[$key-1] );
	}
	
	/**
	 * Used by date() and time() to adjust the time output.
	 *
	 * @param $ts Int the time in date('YmdHis') format
	 * @param $tz Mixed: adjust the time by this amount (default false, mean we
	 *            get user timecorrection setting)
	 * @return int
	 */
	function userAdjust( $ts, $tz = false )	{
		global $wgUser, $wgLocalTZoffset;

		if (!$tz) {
			$tz = $wgUser->getOption( 'timecorrection' );
		}

		# minutes and hours differences:
		$minDiff = 0;
		$hrDiff  = 0;

		if ( $tz === '' ) {
			# Global offset in minutes.
			if( isset($wgLocalTZoffset) ) {
				if( $wgLocalTZoffset >= 0 ) {
					$hrDiff = floor($wgLocalTZoffset / 60);
				} else {
					$hrDiff = ceil($wgLocalTZoffset / 60);
				}
				$minDiff = $wgLocalTZoffset % 60;
			}
		} elseif ( strpos( $tz, ':' ) !== false ) {
			$tzArray = explode( ':', $tz );
			$hrDiff = intval($tzArray[0]);
			$minDiff = intval($hrDiff < 0 ? -$tzArray[1] : $tzArray[1]);
		} else {
			$hrDiff = intval( $tz );
		}

		# No difference ? Return time unchanged
		if ( 0 == $hrDiff && 0 == $minDiff ) { return $ts; }

		wfSuppressWarnings(); // E_STRICT system time bitching
		# Generate an adjusted date
		$t = mktime( (
		  (int)substr( $ts, 8, 2) ) + $hrDiff, # Hours
		  (int)substr( $ts, 10, 2 ) + $minDiff, # Minutes
		  (int)substr( $ts, 12, 2 ), # Seconds
		  (int)substr( $ts, 4, 2 ), # Month
		  (int)substr( $ts, 6, 2 ), # Day
		  (int)substr( $ts, 0, 4 ) ); #Year
		
		$date = date( 'YmdHis', $t );
		wfRestoreWarnings();
		
		return $date;
	}

	/**
	 * This is a workalike of PHP's date() function, but with better
	 * internationalisation, a reduced set of format characters, and a better 
	 * escaping format.
	 *
	 * Supported format characters are dDjlNwzWFmMntLYyaAgGhHiscrU. See the 
	 * PHP manual for definitions. There are a number of extensions, which 
	 * start with "x":
	 *
	 *    xn   Do not translate digits of the next numeric format character
	 *    xN   Toggle raw digit (xn) flag, stays set until explicitly unset
	 *    xr   Use roman numerals for the next numeric format character
	 *    xh   Use hebrew numerals for the next numeric format character
	 *    xx   Literal x
	 *    xg   Genitive month name
	 *
	 *    xij  j (day number) in Iranian calendar
	 *    xiF  F (month name) in Iranian calendar
	 *    xin  n (month number) in Iranian calendar
	 *    xiY  Y (full year) in Iranian calendar
	 *
	 *    xjj  j (day number) in Hebrew calendar
	 *    xjF  F (month name) in Hebrew calendar
	 *    xjt  t (days in month) in Hebrew calendar
	 *    xjx  xg (genitive month name) in Hebrew calendar
	 *    xjn  n (month number) in Hebrew calendar
	 *    xjY  Y (full year) in Hebrew calendar
	 *
	 *   xmj  j (day number) in Hijri calendar
	 *   xmF  F (month name) in Hijri calendar
	 *   xmn  n (month number) in Hijri calendar
	 *   xmY  Y (full year) in Hijri calendar
	 *
	 *    xkY  Y (full year) in Thai solar calendar. Months and days are
	 *                       identical to the Gregorian calendar
	 *
	 * Characters enclosed in double quotes will be considered literal (with
	 * the quotes themselves removed). Unmatched quotes will be considered
	 * literal quotes. Example:
	 *
	 * "The month is" F       => The month is January
	 * i's"                   => 20'11"
	 *
	 * Backslash escaping is also supported.
	 *
	 * Input timestamp is assumed to be pre-normalized to the desired local
	 * time zone, if any.
	 * 
	 * @param $format String
	 * @param $ts String: 14-character timestamp
	 *      YYYYMMDDHHMMSS
	 *      01234567890123
	 */
	function sprintfDate( $format, $ts ) {
		$s = '';
		$raw = false;
		$roman = false;
		$hebrewNum = false;
		$unix = false;
		$rawToggle = false;
		$iranian = false;
		$hebrew = false;
		$hijri = false;
		$thai = false;
		for ( $p = 0; $p < strlen( $format ); $p++ ) {
			$num = false;
			$code = $format[$p];
			if ( $code == 'x' && $p < strlen( $format ) - 1 ) {
				$code .= $format[++$p];
			}

			if ( ( $code === 'xi' || $code == 'xj' || $code == 'xk' || $code == 'xm' ) && $p < strlen( $format ) - 1 ) {
				$code .= $format[++$p];
			}

			switch ( $code ) {
				case 'xx':
					$s .= 'x';
					break;
				case 'xn':
					$raw = true;
					break;
				case 'xN':
					$rawToggle = !$rawToggle;
					break;
				case 'xr':
					$roman = true;
					break;
				case 'xh':
					$hebrewNum = true;
					break;
				case 'xg':
					$s .= $this->getMonthNameGen( substr( $ts, 4, 2 ) );
					break;
				case 'xjx':
					if ( !$hebrew ) $hebrew = self::tsToHebrew( $ts );
					$s .= $this->getHebrewCalendarMonthNameGen( $hebrew[1] );
					break;
				case 'd':
					$num = substr( $ts, 6, 2 );
					break;
				case 'D':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$s .= $this->getWeekdayAbbreviation( gmdate( 'w', $unix ) + 1 );
					break;
				case 'j':
					$num = intval( substr( $ts, 6, 2 ) );
					break;
				case 'xij':
					if ( !$iranian ) $iranian = self::tsToIranian( $ts );
					$num = $iranian[2];
					break;
				case 'xmj':
					if ( !$hijri ) $hijri = self::tsToHijri( $ts );
					$num = $hijri[2];
					break;
				case 'xjj':
					if ( !$hebrew ) $hebrew = self::tsToHebrew( $ts );
					$num = $hebrew[2];
					break;
				case 'l':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$s .= $this->getWeekdayName( gmdate( 'w', $unix ) + 1 );
					break;
				case 'N':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$w = gmdate( 'w', $unix );
					$num = $w ? $w : 7;
					break;
				case 'w':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = gmdate( 'w', $unix );
					break;
				case 'z':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = gmdate( 'z', $unix );
					break;
				case 'W':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = gmdate( 'W', $unix );
					break;
				case 'F':
					$s .= $this->getMonthName( substr( $ts, 4, 2 ) );
					break;
				case 'xiF':
					if ( !$iranian ) $iranian = self::tsToIranian( $ts );
					$s .= $this->getIranianCalendarMonthName( $iranian[1] );
					break;
				case 'xmF':
					if ( !$hijri ) $hijri = self::tsToHijri( $ts );
					$s .= $this->getHijriCalendarMonthName( $hijri[1] );
					break;
				case 'xjF':
					if ( !$hebrew ) $hebrew = self::tsToHebrew( $ts );
					$s .= $this->getHebrewCalendarMonthName( $hebrew[1] );
					break;
				case 'm':
					$num = substr( $ts, 4, 2 );
					break;
				case 'M':
					$s .= $this->getMonthAbbreviation( substr( $ts, 4, 2 ) );
					break;
				case 'n':
					$num = intval( substr( $ts, 4, 2 ) );
					break;
				case 'xin':
					if ( !$iranian ) $iranian = self::tsToIranian( $ts );
					$num = $iranian[1];
					break;
				case 'xmn':
					if ( !$hijri ) $hijri = self::tsToHijri ( $ts );
					$num = $hijri[1];
					break;
				case 'xjn':
					if ( !$hebrew ) $hebrew = self::tsToHebrew( $ts );
					$num = $hebrew[1];
					break;
				case 't':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = gmdate( 't', $unix );
					break;
				case 'xjt':
					if ( !$hebrew ) $hebrew = self::tsToHebrew( $ts );
					$num = $hebrew[3];
					break;
				case 'L':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = gmdate( 'L', $unix );
					break;
				case 'Y':
					$num = substr( $ts, 0, 4 );
					break;
				case 'xiY':
					if ( !$iranian ) $iranian = self::tsToIranian( $ts );
					$num = $iranian[0];
					break;
				case 'xmY':
					if ( !$hijri ) $hijri = self::tsToHijri( $ts );
					$num = $hijri[0];
					break;
				case 'xjY':
					if ( !$hebrew ) $hebrew = self::tsToHebrew( $ts );
					$num = $hebrew[0];
					break;
				case 'xkY':
					if ( !$thai ) $thai = self::tsToThai( $ts );
					$num = $thai[0];
					break;
				case 'y':
					$num = substr( $ts, 2, 2 );
					break;
				case 'a':
					$s .= intval( substr( $ts, 8, 2 ) ) < 12 ? 'am' : 'pm';
					break;
				case 'A':
					$s .= intval( substr( $ts, 8, 2 ) ) < 12 ? 'AM' : 'PM';
					break;
				case 'g':
					$h = substr( $ts, 8, 2 );
					$num = $h % 12 ? $h % 12 : 12;
					break;
				case 'G':
					$num = intval( substr( $ts, 8, 2 ) );
					break;
				case 'h':
					$h = substr( $ts, 8, 2 );
					$num = sprintf( '%02d', $h % 12 ? $h % 12 : 12 );
					break;					
				case 'H':
					$num = substr( $ts, 8, 2 );
					break;
				case 'i':
					$num = substr( $ts, 10, 2 );
					break;
				case 's':
					$num = substr( $ts, 12, 2 );
					break;
				case 'c':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$s .= gmdate( 'c', $unix );
					break;
				case 'r':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$s .= gmdate( 'r', $unix );
					break;
				case 'U':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = $unix;
					break;
				case '\\':
					# Backslash escaping
					if ( $p < strlen( $format ) - 1 ) {
						$s .= $format[++$p];
					} else {
						$s .= '\\';
					}
					break;
				case '"':
					# Quoted literal
					if ( $p < strlen( $format ) - 1 ) {
						$endQuote = strpos( $format, '"', $p + 1 );
						if ( $endQuote === false ) {
							# No terminating quote, assume literal "
							$s .= '"';
						} else {
							$s .= substr( $format, $p + 1, $endQuote - $p - 1 );
							$p = $endQuote;
						}
					} else {
						# Quote at end of string, assume literal "
						$s .= '"';
					}
					break;
				default:
					$s .= $format[$p];
			}
			if ( $num !== false ) {
				if ( $rawToggle || $raw ) {
					$s .= $num;
					$raw = false;
				} elseif ( $roman ) {
					$s .= self::romanNumeral( $num );
					$roman = false;
				} elseif( $hebrewNum ) {
					$s .= self::hebrewNumeral( $num );
					$hebrewNum = false;
				} else {
					$s .= $this->formatNum( $num, true );
				}
				$num = false;
			}
		}
		return $s;
	}

	private static $GREG_DAYS = array( 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 );
	private static $IRANIAN_DAYS = array( 31, 31, 31, 31, 31, 31, 30, 30, 30, 30, 30, 29 );
	/**
	 * Algorithm by Roozbeh Pournader and Mohammad Toossi to convert 
	 * Gregorian dates to Iranian dates. Originally written in C, it
	 * is released under the terms of GNU Lesser General Public
	 * License. Conversion to PHP was performed by Niklas Laxström.
	 * 
	 * Link: http://www.farsiweb.info/jalali/jalali.c
	 */
	private static function tsToIranian( $ts ) {
		$gy = substr( $ts, 0, 4 ) -1600;
		$gm = substr( $ts, 4, 2 ) -1;
		$gd = substr( $ts, 6, 2 ) -1;

		# Days passed from the beginning (including leap years)
		$gDayNo = 365*$gy
			+ floor(($gy+3) / 4)
			- floor(($gy+99) / 100)
			+ floor(($gy+399) / 400);


		// Add days of the past months of this year
		for( $i = 0; $i < $gm; $i++ ) {
			$gDayNo += self::$GREG_DAYS[$i];
		}

		// Leap years
		if ( $gm > 1 && (($gy%4===0 && $gy%100!==0 || ($gy%400==0)))) {
			$gDayNo++;
		}

		// Days passed in current month
		$gDayNo += $gd;
		
		$jDayNo = $gDayNo - 79;

		$jNp = floor($jDayNo / 12053);
		$jDayNo %= 12053;

		$jy = 979 + 33*$jNp + 4*floor($jDayNo/1461);
		$jDayNo %= 1461;

		if ( $jDayNo >= 366 ) {
			$jy += floor(($jDayNo-1)/365);
			$jDayNo = floor(($jDayNo-1)%365);
		}

		for ( $i = 0; $i < 11 && $jDayNo >= self::$IRANIAN_DAYS[$i]; $i++ ) {
			$jDayNo -= self::$IRANIAN_DAYS[$i];
		}

		$jm= $i+1;
		$jd= $jDayNo+1;

		return array($jy, $jm, $jd);
	}
	/**
	 * Converting Gregorian dates to Hijri dates.
	 *
	 * Based on a PHP-Nuke block by Sharjeel which is released under GNU/GPL license
	 *
	 * @link http://phpnuke.org/modules.php?name=News&file=article&sid=8234&mode=thread&order=0&thold=0
	 */
	private static function tsToHijri ( $ts ) {
		$year = substr( $ts, 0, 4 );
		$month = substr( $ts, 4, 2 );
		$day = substr( $ts, 6, 2 );
		
		$zyr = $year;
		$zd=$day;
		$zm=$month;
		$zy=$zyr;



		if (($zy>1582)||(($zy==1582)&&($zm>10))||(($zy==1582)&&($zm==10)&&($zd>14)))
			{
	
	
				    $zjd=(int)((1461*($zy + 4800 + (int)( ($zm-14) /12) ))/4) + (int)((367*($zm-2-12*((int)(($zm-14)/12))))/12)-(int)((3*(int)(( ($zy+4900+(int)(($zm-14)/12))/100)))/4)+$zd-32075;
				    }
		 else
			{
				    $zjd = 367*$zy-(int)((7*($zy+5001+(int)(($zm-9)/7)))/4)+(int)((275*$zm)/9)+$zd+1729777;
			}
		
		$zl=$zjd-1948440+10632;
		$zn=(int)(($zl-1)/10631);
		$zl=$zl-10631*$zn+354;
		$zj=((int)((10985-$zl)/5316))*((int)((50*$zl)/17719))+((int)($zl/5670))*((int)((43*$zl)/15238));
		$zl=$zl-((int)((30-$zj)/15))*((int)((17719*$zj)/50))-((int)($zj/16))*((int)((15238*$zj)/43))+29;
		$zm=(int)((24*$zl)/709);
		$zd=$zl-(int)((709*$zm)/24);
		$zy=30*$zn+$zj-30;

		return array ($zy, $zm, $zd);
	}

	/**
	 * Converting Gregorian dates to Hebrew dates.
	 *
	 * Based on a JavaScript code by Abu Mami and Yisrael Hersch
	 * (abu-mami@kaluach.net, http://www.kaluach.net), who permitted
	 * to translate the relevant functions into PHP and release them under
	 * GNU GPL.
	 */
	private static function tsToHebrew( $ts ) {
		# Parse date
		$year = substr( $ts, 0, 4 );
		$month = substr( $ts, 4, 2 );
		$day = substr( $ts, 6, 2 );

		# Calculate Hebrew year
		$hebrewYear = $year + 3760;

		# Month number when September = 1, August = 12
		$month += 4;
		if( $month > 12 ) {
			# Next year
			$month -= 12;
			$year++;
			$hebrewYear++;
		}

		# Calculate day of year from 1 September
		$dayOfYear = $day;
		for( $i = 1; $i < $month; $i++ ) {
			if( $i == 6 ) {
				# February
				$dayOfYear += 28;
				# Check if the year is leap
				if( $year % 400 == 0 || ( $year % 4 == 0 && $year % 100 > 0 ) ) {
					$dayOfYear++;
				}
			} elseif( $i == 8 || $i == 10 || $i == 1 || $i == 3 ) {
				$dayOfYear += 30;
			} else {
				$dayOfYear += 31;
			}
		}

		# Calculate the start of the Hebrew year
		$start = self::hebrewYearStart( $hebrewYear );

		# Calculate next year's start
		if( $dayOfYear <= $start ) {
			# Day is before the start of the year - it is the previous year
			# Next year's start
			$nextStart = $start;
			# Previous year
			$year--;
			$hebrewYear--;
			# Add days since previous year's 1 September
			$dayOfYear += 365;
			if( ( $year % 400 == 0 ) || ( $year % 100 != 0 && $year % 4 == 0 ) ) {
				# Leap year
				$dayOfYear++;
			}
			# Start of the new (previous) year
			$start = self::hebrewYearStart( $hebrewYear );
		} else {
			# Next year's start
			$nextStart = self::hebrewYearStart( $hebrewYear + 1 );
		}

		# Calculate Hebrew day of year
		$hebrewDayOfYear = $dayOfYear - $start;

		# Difference between year's days
		$diff = $nextStart - $start;
		# Add 12 (or 13 for leap years) days to ignore the difference between
		# Hebrew and Gregorian year (353 at least vs. 365/6) - now the
		# difference is only about the year type
		if( ( $year % 400 == 0 ) || ( $year % 100 != 0 && $year % 4 == 0 ) ) {
			$diff += 13;
		} else {
			$diff += 12;
		}

		# Check the year pattern, and is leap year
		# 0 means an incomplete year, 1 means a regular year, 2 means a complete year
		# This is mod 30, to work on both leap years (which add 30 days of Adar I)
		# and non-leap years
		$yearPattern = $diff % 30;
		# Check if leap year
		$isLeap = $diff >= 30;

		# Calculate day in the month from number of day in the Hebrew year
		# Don't check Adar - if the day is not in Adar, we will stop before;
		# if it is in Adar, we will use it to check if it is Adar I or Adar II
		$hebrewDay = $hebrewDayOfYear;
		$hebrewMonth = 1;
		$days = 0;
		while( $hebrewMonth <= 12 ) {
			# Calculate days in this month
			if( $isLeap && $hebrewMonth == 6 ) {
				# Adar in a leap year
				if( $isLeap ) {
					# Leap year - has Adar I, with 30 days, and Adar II, with 29 days
					$days = 30;
					if( $hebrewDay <= $days ) {
						# Day in Adar I
						$hebrewMonth = 13;
					} else {
						# Subtract the days of Adar I
						$hebrewDay -= $days;
						# Try Adar II
						$days = 29;
						if( $hebrewDay <= $days ) {
							# Day in Adar II
							$hebrewMonth = 14;
						}
					}
				}
			} elseif( $hebrewMonth == 2 && $yearPattern == 2 ) {
				# Cheshvan in a complete year (otherwise as the rule below)
				$days = 30;
			} elseif( $hebrewMonth == 3 && $yearPattern == 0 ) {
				# Kislev in an incomplete year (otherwise as the rule below)
				$days = 29;
			} else {
				# Odd months have 30 days, even have 29
				$days = 30 - ( $hebrewMonth - 1 ) % 2;
			}
			if( $hebrewDay <= $days ) {
				# In the current month
				break;
			} else {
				# Subtract the days of the current month
				$hebrewDay -= $days;
				# Try in the next month
				$hebrewMonth++;
			}
		}

		return array( $hebrewYear, $hebrewMonth, $hebrewDay, $days );
	}

	/**
	 * This calculates the Hebrew year start, as days since 1 September.
	 * Based on Carl Friedrich Gauss algorithm for finding Easter date.
	 * Used for Hebrew date.
	 */
	private static function hebrewYearStart( $year ) {
		$a = intval( ( 12 * ( $year - 1 ) + 17 ) % 19 );
		$b = intval( ( $year - 1 ) % 4 );
		$m = 32.044093161144 + 1.5542417966212 * $a +  $b / 4.0 - 0.0031777940220923 * ( $year - 1 );
		if( $m < 0 ) {
			$m--;
		}
		$Mar = intval( $m );
		if( $m < 0 ) {
			$m++;
		}
		$m -= $Mar;

		$c = intval( ( $Mar + 3 * ( $year - 1 ) + 5 * $b + 5 ) % 7);
		if( $c == 0 && $a > 11 && $m >= 0.89772376543210 ) {
			$Mar++;
		} else if( $c == 1 && $a > 6 && $m >= 0.63287037037037 ) {
			$Mar += 2;
		} else if( $c == 2 || $c == 4 || $c == 6 ) {
			$Mar++;
		}

		$Mar += intval( ( $year - 3761 ) / 100 ) - intval( ( $year - 3761 ) / 400 ) - 24;
		return $Mar;
	}

	/**
	 * Algorithm to convert Gregorian dates to Thai solar dates.
	 *
	 * Link: http://en.wikipedia.org/wiki/Thai_solar_calendar
	 *
	 * @param $ts String: 14-character timestamp
	 * @return array converted year, month, day
	 */
	private static function tsToThai( $ts ) {
		$gy = substr( $ts, 0, 4 );
		$gm = substr( $ts, 4, 2 );
		$gd = substr( $ts, 6, 2 );

		# Add 543 years to the Gregorian calendar
		# Months and days are identical
		$gy_thai = $gy + 543;

		return array( $gy_thai, $gm, $gd );
	}


	/**
	 * Roman number formatting up to 3000
	 */
	static function romanNumeral( $num ) {
		static $table = array(
			array( '', 'I', 'II', 'III', 'IV', 'V', 'VI', 'VII', 'VIII', 'IX', 'X' ),
			array( '', 'X', 'XX', 'XXX', 'XL', 'L', 'LX', 'LXX', 'LXXX', 'XC', 'C' ),
			array( '', 'C', 'CC', 'CCC', 'CD', 'D', 'DC', 'DCC', 'DCCC', 'CM', 'M' ),
			array( '', 'M', 'MM', 'MMM' )
		);
			
		$num = intval( $num );
		if ( $num > 3000 || $num <= 0 ) {
			return $num;
		}

		$s = '';
		for ( $pow10 = 1000, $i = 3; $i >= 0; $pow10 /= 10, $i-- ) {
			if ( $num >= $pow10 ) {
				$s .= $table[$i][floor($num / $pow10)];
			}
			$num = $num % $pow10;
		}
		return $s;
	}

 	/**
	 * Hebrew Gematria number formatting up to 9999
	 */
	static function hebrewNumeral( $num ) {
		static $table = array(
			array( '', 'א', 'ב', 'ג', 'ד', 'ה', 'ו', 'ז', 'ח', 'ט', 'י' ),
			array( '', 'י', 'כ', 'ל', 'מ', 'נ', 'ס', 'ע', 'פ', 'צ', 'ק' ),
			array( '', 'ק', 'ר', 'ש', 'ת', 'תק', 'תר', 'תש', 'תת', 'תתק', 'תתר' ),
			array( '', 'א', 'ב', 'ג', 'ד', 'ה', 'ו', 'ז', 'ח', 'ט', 'י' )
		);

		$num = intval( $num );
		if ( $num > 9999 || $num <= 0 ) {
			return $num;
		}

		$s = '';
		for ( $pow10 = 1000, $i = 3; $i >= 0; $pow10 /= 10, $i-- ) {
			if ( $num >= $pow10 ) {
				if ( $num == 15 || $num == 16 ) {
					$s .= $table[0][9] . $table[0][$num - 9];
					$num = 0;
				} else {
					$s .= $table[$i][intval( ( $num / $pow10 ) )];
					if( $pow10 == 1000 ) {
						$s .= "'";
					}
				}
			}
			$num = $num % $pow10;
		}
		if( strlen( $s ) == 2 ) {
			$str = $s . "'";
		} else  {
			$str = substr( $s, 0, strlen( $s ) - 2 ) . '"';
			$str .= substr( $s, strlen( $s ) - 2, 2 );
		}
		$start = substr( $str, 0, strlen( $str ) - 2 );
		$end = substr( $str, strlen( $str ) - 2 );
		switch( $end ) {
			case 'כ':
				$str = $start . 'ך';
				break;
			case 'מ':
				$str = $start . 'ם';
				break;
			case 'נ':
				$str = $start . 'ן';
				break;
			case 'פ':
				$str = $start . 'ף';
				break;
			case 'צ':
				$str = $start . 'ץ';
				break;
		}
		return $str;
	}

	/**
	 * This is meant to be used by time(), date(), and timeanddate() to get
	 * the date preference they're supposed to use, it should be used in
	 * all children.
	 *
	 *<code>
	 * function timeanddate([...], $format = true) {
	 * 	$datePreference = $this->dateFormat($format);
	 * [...]
	 * }
	 *</code>
	 *
	 * @param $usePrefs Mixed: if true, the user's preference is used
	 *                         if false, the site/language default is used
	 *                         if int/string, assumed to be a format.
	 * @return string
	 */
	function dateFormat( $usePrefs = true ) {
		global $wgUser;

		if( is_bool( $usePrefs ) ) {
			if( $usePrefs ) {
				$datePreference = $wgUser->getDatePreference();
			} else {
				$options = User::getDefaultOptions();
				$datePreference = (string)$options['date'];
			}
		} else {
			$datePreference = (string)$usePrefs;
		}

		// return int
		if( $datePreference == '' ) {
			return 'default';
		}
		
		return $datePreference;
	}

	/**
	 * @param $ts Mixed: the time format which needs to be turned into a
	 *            date('YmdHis') format with wfTimestamp(TS_MW,$ts)
	 * @param $adj Bool: whether to adjust the time output according to the
	 *             user configured offset ($timecorrection)
	 * @param $format Mixed: true to use user's date format preference
	 * @param $timecorrection String: the time offset as returned by
	 *                        validateTimeZone() in Special:Preferences
	 * @return string
	 */
	function date( $ts, $adj = false, $format = true, $timecorrection = false ) {
		$this->load();
		if ( $adj ) { 
			$ts = $this->userAdjust( $ts, $timecorrection ); 
		}

		$pref = $this->dateFormat( $format );
		if( $pref == 'default' || !isset( $this->dateFormats["$pref date"] ) ) {
			$pref = $this->defaultDateFormat;
		}
		return $this->sprintfDate( $this->dateFormats["$pref date"], $ts );
	}

	/**
	 * @param $ts Mixed: the time format which needs to be turned into a
	 *            date('YmdHis') format with wfTimestamp(TS_MW,$ts)
	 * @param $adj Bool: whether to adjust the time output according to the
	 *             user configured offset ($timecorrection)
	 * @param $format Mixed: true to use user's date format preference
	 * @param $timecorrection String: the time offset as returned by
	 *                        validateTimeZone() in Special:Preferences
	 * @return string
	 */
	function time( $ts, $adj = false, $format = true, $timecorrection = false ) {
		$this->load();
		if ( $adj ) { 
			$ts = $this->userAdjust( $ts, $timecorrection ); 
		}

		$pref = $this->dateFormat( $format );
		if( $pref == 'default' || !isset( $this->dateFormats["$pref time"] ) ) {
			$pref = $this->defaultDateFormat;
		}
		return $this->sprintfDate( $this->dateFormats["$pref time"], $ts );
	}

	/**
	 * @param $ts Mixed: the time format which needs to be turned into a
	 *            date('YmdHis') format with wfTimestamp(TS_MW,$ts)
	 * @param $adj Bool: whether to adjust the time output according to the
	 *             user configured offset ($timecorrection)
	 * @param $format Mixed: what format to return, if it's false output the
	 *                default one (default true)
	 * @param $timecorrection String: the time offset as returned by
	 *                        validateTimeZone() in Special:Preferences
	 * @return string
	 */
	function timeanddate( $ts, $adj = false, $format = true, $timecorrection = false) {
		$this->load();

		$ts = wfTimestamp( TS_MW, $ts );

		if ( $adj ) { 
			$ts = $this->userAdjust( $ts, $timecorrection ); 
		}

		$pref = $this->dateFormat( $format );
		if( $pref == 'default' || !isset( $this->dateFormats["$pref both"] ) ) {
			$pref = $this->defaultDateFormat;
		}

		return $this->sprintfDate( $this->dateFormats["$pref both"], $ts );
	}

	function getMessage( $key ) {
		$this->load();
		return isset( $this->messages[$key] ) ? $this->messages[$key] : null;
	}

	function getAllMessages() {
		$this->load();
		return $this->messages;
	}

	function iconv( $in, $out, $string ) {
		# For most languages, this is a wrapper for iconv
		return iconv( $in, $out . '//IGNORE', $string );
	}

	// callback functions for uc(), lc(), ucwords(), ucwordbreaks()
	function ucwordbreaksCallbackAscii($matches){
		return $this->ucfirst($matches[1]);
	}
	
	function ucwordbreaksCallbackMB($matches){
		return mb_strtoupper($matches[0]);
	}
	
	function ucCallback($matches){
		list( $wikiUpperChars ) = self::getCaseMaps();
		return strtr( $matches[1], $wikiUpperChars );
	}
	
	function lcCallback($matches){
		list( , $wikiLowerChars ) = self::getCaseMaps();
		return strtr( $matches[1], $wikiLowerChars );
	}
	
	function ucwordsCallbackMB($matches){
		return mb_strtoupper($matches[0]);
	}
	
	function ucwordsCallbackWiki($matches){
		list( $wikiUpperChars ) = self::getCaseMaps();
		return strtr( $matches[0], $wikiUpperChars );
	}

	function ucfirst( $str ) {
		if ( empty($str) ) return $str;
		if ( ord($str[0]) < 128 ) return ucfirst($str);
		else return self::uc($str,true); // fall back to more complex logic in case of multibyte strings
	}

	function uc( $str, $first = false ) {
		if ( function_exists( 'mb_strtoupper' ) ) {
			if ( $first ) {
				if ( self::isMultibyte( $str ) ) {
					return mb_strtoupper( mb_substr( $str, 0, 1 ) ) . mb_substr( $str, 1 );
				} else {
					return ucfirst( $str );
				}
			} else {
				return self::isMultibyte( $str ) ? mb_strtoupper( $str ) : strtoupper( $str );
			}
		} else {
			if ( self::isMultibyte( $str ) ) {
				list( $wikiUpperChars ) = $this->getCaseMaps();
				$x = $first ? '^' : '';
				return preg_replace_callback(
					"/$x([a-z]|[\\xc0-\\xff][\\x80-\\xbf]*)/",
					array($this,"ucCallback"),
					$str
				);
			} else {
				return $first ? ucfirst( $str ) : strtoupper( $str );
			}
		}
	}
	
	function lcfirst( $str ) {
		if ( empty($str) ) return $str;
		if ( is_string( $str ) && ord($str[0]) < 128 ) {
			// editing string in place = cool
			$str[0]=strtolower($str[0]);
			return $str;
		}
		else return self::lc( $str, true );
	}

	function lc( $str, $first = false ) {
		if ( function_exists( 'mb_strtolower' ) )
			if ( $first )
				if ( self::isMultibyte( $str ) )
					return mb_strtolower( mb_substr( $str, 0, 1 ) ) . mb_substr( $str, 1 );
				else
					return strtolower( substr( $str, 0, 1 ) ) . substr( $str, 1 );
			else
				return self::isMultibyte( $str ) ? mb_strtolower( $str ) : strtolower( $str );
		else
			if ( self::isMultibyte( $str ) ) {
				list( , $wikiLowerChars ) = self::getCaseMaps();
				$x = $first ? '^' : '';
				return preg_replace_callback(
					"/$x([A-Z]|[\\xc0-\\xff][\\x80-\\xbf]*)/",
					array($this,"lcCallback"),
					$str
				);
			} else
				return $first ? strtolower( substr( $str, 0, 1 ) ) . substr( $str, 1 ) : strtolower( $str );
	}

	function isMultibyte( $str ) {
		return (bool)preg_match( '/[\x80-\xff]/', $str );
	}

	function ucwords($str) {
		if ( self::isMultibyte( $str ) ) {
			$str = self::lc($str);

			// regexp to find first letter in each word (i.e. after each space)
			$replaceRegexp = "/^([a-z]|[\\xc0-\\xff][\\x80-\\xbf]*)| ([a-z]|[\\xc0-\\xff][\\x80-\\xbf]*)/";

			// function to use to capitalize a single char
			if ( function_exists( 'mb_strtoupper' ) )
				return preg_replace_callback(
					$replaceRegexp,
					array($this,"ucwordsCallbackMB"),
					$str
				);
			else 
				return preg_replace_callback(
					$replaceRegexp,
					array($this,"ucwordsCallbackWiki"),
					$str
				);
		}
		else
			return ucwords( strtolower( $str ) );
	}

  # capitalize words at word breaks
	function ucwordbreaks($str){
		if (self::isMultibyte( $str ) ) {
			$str = self::lc($str);

			// since \b doesn't work for UTF-8, we explicitely define word break chars
			$breaks= "[ \-\(\)\}\{\.,\?!]";

			// find first letter after word break
			$replaceRegexp = "/^([a-z]|[\\xc0-\\xff][\\x80-\\xbf]*)|$breaks([a-z]|[\\xc0-\\xff][\\x80-\\xbf]*)/";

			if ( function_exists( 'mb_strtoupper' ) )
				return preg_replace_callback(
					$replaceRegexp,
					array($this,"ucwordbreaksCallbackMB"),
					$str
				);
			else 
				return preg_replace_callback(
					$replaceRegexp,
					array($this,"ucwordsCallbackWiki"),
					$str
				);
		}
		else
			return preg_replace_callback(
			'/\b([\w\x80-\xff]+)\b/',
			array($this,"ucwordbreaksCallbackAscii"),
			$str );
	}

	/**
	 * Return a case-folded representation of $s
	 *
	 * This is a representation such that caseFold($s1)==caseFold($s2) if $s1 
	 * and $s2 are the same except for the case of their characters. It is not
	 * necessary for the value returned to make sense when displayed.
	 *
	 * Do *not* perform any other normalisation in this function. If a caller
	 * uses this function when it should be using a more general normalisation
	 * function, then fix the caller.
	 */
	function caseFold( $s ) {
		return $this->uc( $s );
	}

	function checkTitleEncoding( $s ) {
		if( is_array( $s ) ) {
			wfDebugDieBacktrace( 'Given array to checkTitleEncoding.' );
		}
		# Check for non-UTF-8 URLs
		$ishigh = preg_match( '/[\x80-\xff]/', $s);
		if(!$ishigh) return $s;

		$isutf8 = preg_match( '/^([\x00-\x7f]|[\xc0-\xdf][\x80-\xbf]|' .
                '[\xe0-\xef][\x80-\xbf]{2}|[\xf0-\xf7][\x80-\xbf]{3})+$/', $s );
		if( $isutf8 ) return $s;

		return $this->iconv( $this->fallback8bitEncoding(), "utf-8", $s );
	}

	function fallback8bitEncoding() {
		$this->load();
		return $this->fallback8bitEncoding;
	}
	
	/**
	 * Some languages have special punctuation to strip out
	 * or characters which need to be converted for MySQL's
	 * indexing to grok it correctly. Make such changes here.
	 *
	 * @param $string String
	 * @return String
	 */
	function stripForSearch( $string ) {
		global $wgDBtype;
		if ( $wgDBtype != 'mysql' ) {
			return $string;
		}

		# MySQL fulltext index doesn't grok utf-8, so we
		# need to fold cases and convert to hex

		wfProfileIn( __METHOD__ );
		if( function_exists( 'mb_strtolower' ) ) {
			$out = preg_replace(
				"/([\\xc0-\\xff][\\x80-\\xbf]*)/e",
				"'U8' . bin2hex( \"$1\" )",
				mb_strtolower( $string ) );
		} else {
			list( , $wikiLowerChars ) = self::getCaseMaps();
			$out = preg_replace(
				"/([\\xc0-\\xff][\\x80-\\xbf]*)/e",
				"'U8' . bin2hex( strtr( \"\$1\", \$wikiLowerChars ) )",
				$string );
		}
		wfProfileOut( __METHOD__ );
		return $out;
	}

	function convertForSearchResult( $termsArray ) {
		# some languages, e.g. Chinese, need to do a conversion
		# in order for search results to be displayed correctly
		return $termsArray;
	}

	/**
	 * Get the first character of a string. 
	 *
	 * @param $s string
	 * @return string
	 */
	function firstChar( $s ) {
		$matches = array();
		preg_match( '/^([\x00-\x7f]|[\xc0-\xdf][\x80-\xbf]|' .
		'[\xe0-\xef][\x80-\xbf]{2}|[\xf0-\xf7][\x80-\xbf]{3})/', $s, $matches);

		if ( isset( $matches[1] ) ) {
			if ( strlen( $matches[1] ) != 3 ) {
				return $matches[1];
			}
			
			// Break down Hangul syllables to grab the first jamo
			$code = utf8ToCodepoint( $matches[1] );
			if ( $code < 0xac00 || 0xd7a4 <= $code) {
				return $matches[1];
			} elseif ( $code < 0xb098 ) {
				return "\xe3\x84\xb1";
			} elseif ( $code < 0xb2e4 ) {
				return "\xe3\x84\xb4";
			} elseif ( $code < 0xb77c ) {
				return "\xe3\x84\xb7";
			} elseif ( $code < 0xb9c8 ) {
				return "\xe3\x84\xb9";
			} elseif ( $code < 0xbc14 ) {
				return "\xe3\x85\x81";
			} elseif ( $code < 0xc0ac ) {
				return "\xe3\x85\x82";
			} elseif ( $code < 0xc544 ) {
				return "\xe3\x85\x85";
			} elseif ( $code < 0xc790 ) {
				return "\xe3\x85\x87";
			} elseif ( $code < 0xcc28 ) {
				return "\xe3\x85\x88";
			} elseif ( $code < 0xce74 ) {
				return "\xe3\x85\x8a";
			} elseif ( $code < 0xd0c0 ) {
				return "\xe3\x85\x8b";
			} elseif ( $code < 0xd30c ) {
				return "\xe3\x85\x8c";
			} elseif ( $code < 0xd558 ) {
				return "\xe3\x85\x8d";
			} else {
				return "\xe3\x85\x8e";
			}
		} else {
			return "";
		}
	}

	function initEncoding() {
		# Some languages may have an alternate char encoding option
		# (Esperanto X-coding, Japanese furigana conversion, etc)
		# If this language is used as the primary content language,
		# an override to the defaults can be set here on startup.
	}

	function recodeForEdit( $s ) {
		# For some languages we'll want to explicitly specify
		# which characters make it into the edit box raw
		# or are converted in some way or another.
		# Note that if wgOutputEncoding is different from
		# wgInputEncoding, this text will be further converted
		# to wgOutputEncoding.
		global $wgEditEncoding;
		if( $wgEditEncoding == '' or
		  $wgEditEncoding == 'UTF-8' ) {
			return $s;
		} else {
			return $this->iconv( 'UTF-8', $wgEditEncoding, $s );
		}
	}

	function recodeInput( $s ) {
		# Take the previous into account.
		global $wgEditEncoding;
		if($wgEditEncoding != "") {
			$enc = $wgEditEncoding;
		} else {
			$enc = 'UTF-8';
		}
		if( $enc == 'UTF-8' ) {
			return $s;
		} else {
			return $this->iconv( $enc, 'UTF-8', $s );
		}
	}

	/**
	 * For right-to-left language support
	 *
	 * @return bool
	 */
	function isRTL() { 
		$this->load();
		return $this->rtl;
	}

	/**
	 * A hidden direction mark (LRM or RLM), depending on the language direction
	 *
	 * @return string
	 */
	function getDirMark() {
		return $this->isRTL() ? "\xE2\x80\x8F" : "\xE2\x80\x8E";
	}

	/**
	 * An arrow, depending on the language direction
	 *
	 * @return string
	 */
	function getArrow() {
		return $this->isRTL() ? '←' : '→';
	}

	/**
	 * To allow "foo[[bar]]" to extend the link over the whole word "foobar"
	 *
	 * @return bool
	 */
	function linkPrefixExtension() {
		$this->load();
		return $this->linkPrefixExtension;
	}

	function &getMagicWords() {
		$this->load();
		return $this->magicWords;
	}

	# Fill a MagicWord object with data from here
	function getMagic( &$mw ) {
		if ( !$this->mMagicHookDone ) {
			$this->mMagicHookDone = true;
			wfRunHooks( 'LanguageGetMagic', array( &$this->mMagicExtensions, $this->getCode() ) );
		}
		if ( isset( $this->mMagicExtensions[$mw->mId] ) ) {
			$rawEntry = $this->mMagicExtensions[$mw->mId];
		} else {
			$magicWords =& $this->getMagicWords();
			if ( isset( $magicWords[$mw->mId] ) ) {
				$rawEntry = $magicWords[$mw->mId];
			} else {
				# Fall back to English if local list is incomplete
				$magicWords =& Language::getMagicWords();
				if ( !isset($magicWords[$mw->mId]) ) { throw new MWException("Magic word not found" ); }
				$rawEntry = $magicWords[$mw->mId];
			}
		}

		if( !is_array( $rawEntry ) ) {
			error_log( "\"$rawEntry\" is not a valid magic thingie for \"$mw->mId\"" );
		} else {
			$mw->mCaseSensitive = $rawEntry[0];
			$mw->mSynonyms = array_slice( $rawEntry, 1 );
		}
	}

	/**
	 * Add magic words to the extension array
	 */
	function addMagicWordsByLang( $newWords ) {
		$code = $this->getCode();
		$fallbackChain = array();
		while ( $code && !in_array( $code, $fallbackChain ) ) {
			$fallbackChain[] = $code;
			$code = self::getFallbackFor( $code );
		}
		if ( !in_array( 'en', $fallbackChain ) ) {
			$fallbackChain[] = 'en';
		}
		$fallbackChain = array_reverse( $fallbackChain );
		foreach ( $fallbackChain as $code ) {
			if ( isset( $newWords[$code] ) ) {
				$this->mMagicExtensions = $newWords[$code] + $this->mMagicExtensions;
			}
		}
	}

	/**
	 * Get special page names, as an associative array
	 *   case folded alias => real name
	 */
	function getSpecialPageAliases() {
		$this->load();

		// Cache aliases because it may be slow to load them
		if ( !isset( $this->mExtendedSpecialPageAliases ) ) {

			// Initialise array
			$this->mExtendedSpecialPageAliases = $this->specialPageAliases;

			global $wgExtensionAliasesFiles;
			foreach ( $wgExtensionAliasesFiles as $file ) {

				// Fail fast
				if ( !file_exists($file) )
					throw new MWException( 'Aliases file does not exist' );

				$aliases = array();
				require($file);

				// Check the availability of aliases
				if ( !isset($aliases['en']) )
					throw new MWException( 'Malformed aliases file' );

				// Merge all aliases in fallback chain
				$code = $this->getCode();
				do {
					if ( !isset($aliases[$code]) ) continue;

					$aliases[$code] = $this->fixSpecialPageAliases( $aliases[$code] );
					/* Merge the aliases, THIS will break if there is special page name
					* which looks like a numerical key, thanks to PHP...
					* See the comments for wfArrayMerge in GlobalSettings.php. */
					$this->mExtendedSpecialPageAliases = array_merge_recursive(
						$this->mExtendedSpecialPageAliases, $aliases[$code] );

				} while ( $code = self::getFallbackFor( $code ) );
			}

			wfRunHooks( 'LanguageGetSpecialPageAliases',
				array( &$this->mExtendedSpecialPageAliases, $this->getCode() ) );
		}

		return $this->mExtendedSpecialPageAliases;
	}

	/**
	 * Function to fix special page aliases. Will convert the first letter to
	 * upper case and spaces to underscores. Can be given a full aliases array,
	 * in which case it will recursively fix all aliases.
	 */
	public function fixSpecialPageAliases( $mixed ) {
		// Work recursively until in string level
		if ( is_array($mixed) ) {
			$callback = array( $this, 'fixSpecialPageAliases' );
			return array_map( $callback, $mixed );
		}
		return str_replace( ' ', '_', $this->ucfirst( $mixed ) );
	}

	/**
	 * Italic is unsuitable for some languages
	 *
	 * @param $text String: the text to be emphasized.
	 * @return string
	 */
	function emphasize( $text ) {
		return "<em>$text</em>";
	}

	 /**
	  * Normally we output all numbers in plain en_US style, that is
	  * 293,291.235 for twohundredninetythreethousand-twohundredninetyone
	  * point twohundredthirtyfive. However this is not sutable for all
	  * languages, some such as Pakaran want ੨੯੩,੨੯੫.੨੩੫ and others such as
	  * Icelandic just want to use commas instead of dots, and dots instead
	  * of commas like "293.291,235".
	  *
	  * An example of this function being called:
	  * <code>
	  * wfMsg( 'message', $wgLang->formatNum( $num ) )
	  * </code>
	  *
	  * See LanguageGu.php for the Gujarati implementation and
	  * LanguageIs.php for the , => . and . => , implementation.
	  *
	  * @todo check if it's viable to use localeconv() for the decimal
	  *       seperator thing.
	  * @param $number Mixed: the string to be formatted, should be an integer
	  *        or a floating point number.
	  * @param $nocommafy Bool: set to true for special numbers like dates
	  * @return string
	  */
	function formatNum( $number, $nocommafy = false ) {
		global $wgTranslateNumerals;
		if (!$nocommafy) {
			$number = $this->commafy($number);
			$s = $this->separatorTransformTable();
			if (!is_null($s)) { $number = strtr($number, $s); }
		}

		if ($wgTranslateNumerals) {
			$s = $this->digitTransformTable();
			if (!is_null($s)) { $number = strtr($number, $s); }
		}

		return $number;
	}

	function parseFormattedNumber( $number ) {
		$s = $this->digitTransformTable();
		if (!is_null($s)) { $number = strtr($number, array_flip($s)); }

		$s = $this->separatorTransformTable();
		if (!is_null($s)) { $number = strtr($number, array_flip($s)); }

		$number = strtr( $number, array (',' => '') );
		return $number;
	}

	/**
	 * Adds commas to a given number
	 *
	 * @param $_ mixed
	 * @return string
	 */
	function commafy($_) {
		return strrev((string)preg_replace('/(\d{3})(?=\d)(?!\d*\.)/','$1,',strrev($_)));
	}

	function digitTransformTable() {
		$this->load();
		return $this->digitTransformTable;
	}

	function separatorTransformTable() {
		$this->load();
		return $this->separatorTransformTable;
	}


	/**
	 * For the credit list in includes/Credits.php (action=credits)
	 *
	 * @param $l Array
	 * @return string
	 */
	function listToText( $l ) {
		$s = '';
		$m = count($l) - 1;
		for ($i = $m; $i >= 0; $i--) {
			if ($i == $m) {
				$s = $l[$i];
			} else if ($i == $m - 1) {
				$s = $l[$i] . ' ' . $this->getMessageFromDB( 'and' ) . ' ' . $s;
			} else {
				$s = $l[$i] . ', ' . $s;
			}
		}
		return $s;
	}

	/**
	 * Truncate a string to a specified length in bytes, appending an optional
	 * string (e.g. for ellipses)
	 *
	 * The database offers limited byte lengths for some columns in the database;
	 * multi-byte character sets mean we need to ensure that only whole characters
	 * are included, otherwise broken characters can be passed to the user
	 *
	 * If $length is negative, the string will be truncated from the beginning
	 *	 
	 * @param $string String to truncate
	 * @param $length Int: maximum length (excluding ellipses)
	 * @param $ellipsis String to append to the truncated text
	 * @return string
	 */
	function truncate( $string, $length, $ellipsis = "" ) {
		if( $length == 0 ) {
			return $ellipsis;
		}
		if ( strlen( $string ) <= abs( $length ) ) {
			return $string;
		}
		if( $length > 0 ) {
			$string = substr( $string, 0, $length );
			$char = ord( $string[strlen( $string ) - 1] );
			$m = array();
			if ($char >= 0xc0) {
				# We got the first byte only of a multibyte char; remove it.
				$string = substr( $string, 0, -1 );
			} elseif( $char >= 0x80 &&
			          preg_match( '/^(.*)(?:[\xe0-\xef][\x80-\xbf]|' .
			                      '[\xf0-\xf7][\x80-\xbf]{1,2})$/', $string, $m ) ) {
			    # We chopped in the middle of a character; remove it
				$string = $m[1];
			}
			return $string . $ellipsis;
		} else {
			$string = substr( $string, $length );
			$char = ord( $string[0] );
			if( $char >= 0x80 && $char < 0xc0 ) {
				# We chopped in the middle of a character; remove the whole thing
				$string = preg_replace( '/^[\x80-\xbf]+/', '', $string );
			}
			return $ellipsis . $string;
		}
	}

	/**
	 * Grammatical transformations, needed for inflected languages
	 * Invoked by putting {{grammar:case|word}} in a message
	 *
	 * @param $word string
	 * @param $case string
	 * @return string
	 */
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;
		if ( isset($wgGrammarForms[$this->getCode()][$case][$word]) ) {
			return $wgGrammarForms[$this->getCode()][$case][$word];
		}
		return $word;
	}

	/**
	 * Plural form transformations, needed for some languages.
	 * For example, there are 3 form of plural in Russian and Polish,
	 * depending on "count mod 10". See [[w:Plural]]
	 * For English it is pretty simple.
	 *
	 * Invoked by putting {{plural:count|wordform1|wordform2}}
	 * or {{plural:count|wordform1|wordform2|wordform3}}
	 *
	 * Example: {{plural:{{NUMBEROFARTICLES}}|article|articles}}
	 *
	 * @param $count Integer: non-localized number
	 * @param $forms Array: different plural forms
	 * @return string Correct form of plural for $count in this language
	 */
	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 2 );

		return ( abs($count) == 1 ) ? $forms[0] : $forms[1];
	}

	/**
	 * Checks that convertPlural was given an array and pads it to requested
	 * amound of forms by copying the last one.
	 *
	 * @param $count Integer: How many forms should there be at least
	 * @param $forms Array of forms given to convertPlural
	 * @return array Padded array of forms or an exception if not an array
	 */
	protected function preConvertPlural( /* Array */ $forms, $count ) {
		while ( count($forms) < $count ) {
			$forms[] = $forms[count($forms)-1];
		}
		return $forms;
	}

	/**
	 * For translaing of expiry times
	 * @param $str String: the validated block time in English
	 * @return Somehow translated block time
	 * @see LanguageFi.php for example implementation
	 */
	function translateBlockExpiry( $str ) {

		$scBlockExpiryOptions = $this->getMessageFromDB( 'ipboptions' );

		if ( $scBlockExpiryOptions == '-') {
			return $str;
		}

		foreach (explode(',', $scBlockExpiryOptions) as $option) {
			if ( strpos($option, ":") === false )
				continue;
			list($show, $value) = explode(":", $option);
			if ( strcmp ( $str, $value) == 0 ) {
				return htmlspecialchars( trim( $show ) );
			}
		}

		return $str;
	}

	/**
	 * languages like Chinese need to be segmented in order for the diff
	 * to be of any use
	 *
	 * @param $text String
	 * @return String
	 */
	function segmentForDiff( $text ) {
		return $text;
	}

	/**
	 * and unsegment to show the result
	 *
	 * @param $text String
	 * @return String
	 */
	function unsegmentForDiff( $text ) {
		return $text;
	}

	# convert text to different variants of a language.
	function convert( $text, $isTitle = false) {
		return $this->mConverter->convert($text, $isTitle);
	}

	# Convert text from within Parser
	function parserConvert( $text, &$parser ) {
		return $this->mConverter->parserConvert( $text, $parser );
	}

	# Check if this is a language with variants
	function hasVariants(){
		return sizeof($this->getVariants())>1;
	}

	# Put custom tags (e.g. -{ }-) around math to prevent conversion
	function armourMath($text){ 
		return $this->mConverter->armourMath($text);
	}


	/**
	 * Perform output conversion on a string, and encode for safe HTML output.
	 * @param $text String
	 * @param $isTitle Bool -- wtf?
	 * @return string
	 * @todo this should get integrated somewhere sane
	 */
	function convertHtml( $text, $isTitle = false ) {
		return htmlspecialchars( $this->convert( $text, $isTitle ) );
	}

	function convertCategoryKey( $key ) {
		return $this->mConverter->convertCategoryKey( $key );
	}

	/**
	 * get the list of variants supported by this langauge
	 * see sample implementation in LanguageZh.php
	 *
	 * @return array an array of language codes
	 */
	function getVariants() {
		return $this->mConverter->getVariants();
	}


	function getPreferredVariant( $fromUser = true ) {
		return $this->mConverter->getPreferredVariant( $fromUser );
	}

	/**
	 * if a language supports multiple variants, it is
	 * possible that non-existing link in one variant
	 * actually exists in another variant. this function
	 * tries to find it. See e.g. LanguageZh.php
	 *
	 * @param $link String: the name of the link
	 * @param $nt Mixed: the title object of the link
	 * @return null the input parameters may be modified upon return
	 */
	function findVariantLink( &$link, &$nt ) {
		$this->mConverter->findVariantLink($link, $nt);
	}

	/**
	 * If a language supports multiple variants, converts text
	 * into an array of all possible variants of the text:
	 *  'variant' => text in that variant
	 */

	function convertLinkToAllVariants($text){
		return $this->mConverter->convertLinkToAllVariants($text);
	}


	/**
	 * returns language specific options used by User::getPageRenderHash()
	 * for example, the preferred language variant
	 *
	 * @return string
	 */
	function getExtraHashOptions() {
		return $this->mConverter->getExtraHashOptions();
	}

	/**
	 * for languages that support multiple variants, the title of an
	 * article may be displayed differently in different variants. this
	 * function returns the apporiate title defined in the body of the article.
	 *
	 * @return string
	 */
	function getParsedTitle() {
		return $this->mConverter->getParsedTitle();
	}

	/**
	 * Enclose a string with the "no conversion" tag. This is used by
	 * various functions in the Parser
	 *
	 * @param $text String: text to be tagged for no conversion
	 * @param $noParse
	 * @return string the tagged text
	 */
	function markNoConversion( $text, $noParse=false ) {
		return $this->mConverter->markNoConversion( $text, $noParse );
	}

	/**
	 * A regular expression to match legal word-trailing characters
	 * which should be merged onto a link of the form [[foo]]bar.
	 *
	 * @return string
	 */
	function linkTrail() {
		$this->load();
		return $this->linkTrail;
	}

	function getLangObj() {
		return $this;
	}

	/**
	 * Get the RFC 3066 code for this language object
	 */
	function getCode() {
		return $this->mCode;
	}

	function setCode( $code ) {
		$this->mCode = $code;
	}

	static function getFileName( $prefix = 'Language', $code, $suffix = '.php' ) {
		return $prefix . str_replace( '-', '_', ucfirst( $code ) ) . $suffix;
	}

	static function getMessagesFileName( $code ) {
		global $IP;
		return self::getFileName( "$IP/languages/messages/Messages", $code, '.php' );
	}

	static function getClassFileName( $code ) {
		global $IP;
		return self::getFileName( "$IP/languages/classes/Language", $code, '.php' );
	}
	
	static function getLocalisationArray( $code, $disableCache = false ) {
		self::loadLocalisation( $code, $disableCache );
		return self::$mLocalisationCache[$code];
	}

	/**
	 * Load localisation data for a given code into the static cache
	 *
	 * @return array Dependencies, map of filenames to mtimes
	 */
	static function loadLocalisation( $code, $disableCache = false ) {
		static $recursionGuard = array();
		global $wgMemc, $wgCheckSerialized;

		if ( !$code ) {
			throw new MWException( "Invalid language code requested" );
		}

		if ( !$disableCache ) {
			# Try the per-process cache
			if ( isset( self::$mLocalisationCache[$code] ) ) {
				return self::$mLocalisationCache[$code]['deps'];
			}

			wfProfileIn( __METHOD__ );

			# Try the serialized directory
			$cache = wfGetPrecompiledData( self::getFileName( "Messages", $code, '.ser' ) );
			if ( $cache ) {
				if ( $wgCheckSerialized && self::isLocalisationOutOfDate( $cache ) ) {
					$cache = false;
					wfDebug( "Language::loadLocalisation(): precompiled data file for $code is out of date\n" );
				} else {
					self::$mLocalisationCache[$code] = $cache;
					wfDebug( "Language::loadLocalisation(): got localisation for $code from precompiled data file\n" );
					wfProfileOut( __METHOD__ );
					return self::$mLocalisationCache[$code]['deps'];
				}
			}

			# Try the global cache
			$memcKey = wfMemcKey('localisation', $code );
			$fbMemcKey = wfMemcKey('fallback', $cache['fallback'] );
			$cache = $wgMemc->get( $memcKey );
			if ( $cache ) {
				if ( self::isLocalisationOutOfDate( $cache ) ) {
					$wgMemc->delete( $memcKey );
					$wgMemc->delete( $fbMemcKey );
					$cache = false;
					wfDebug( "Language::loadLocalisation(): localisation cache for $code had expired\n" );
				} else {
					self::$mLocalisationCache[$code] = $cache;
					wfDebug( "Language::loadLocalisation(): got localisation for $code from cache\n" );
					wfProfileOut( __METHOD__ );
					return $cache['deps'];
				}
			}
		} else {
			wfProfileIn( __METHOD__ );
		}

		# Default fallback, may be overridden when the messages file is included
		if ( $code != 'en' ) {
			$fallback = 'en';
		} else {
			$fallback = false;
		}

		# Load the primary localisation from the source file
		$filename = self::getMessagesFileName( $code );
		if ( !file_exists( $filename ) ) {
			wfDebug( "Language::loadLocalisation(): no localisation file for $code, using implicit fallback to en\n" );
			$cache = compact( self::$mLocalisationKeys ); // Set correct fallback
			$deps = array();
		} else {
			$deps = array( $filename => filemtime( $filename ) );
			require( $filename );
			$cache = compact( self::$mLocalisationKeys );
			wfDebug( "Language::loadLocalisation(): got localisation for $code from source\n" );
		}

		if ( !empty( $fallback ) ) {
			# Load the fallback localisation, with a circular reference guard
			if ( isset( $recursionGuard[$code] ) ) {
				throw new MWException( "Error: Circular fallback reference in language code $code" );
			}
			$recursionGuard[$code] = true;
			$newDeps = self::loadLocalisation( $fallback, $disableCache );
			unset( $recursionGuard[$code] );

			$secondary = self::$mLocalisationCache[$fallback];
			$deps = array_merge( $deps, $newDeps );

			# Merge the fallback localisation with the current localisation
			foreach ( self::$mLocalisationKeys as $key ) {
				if ( isset( $cache[$key] ) ) {
					if ( isset( $secondary[$key] ) ) {
						if ( in_array( $key, self::$mMergeableMapKeys ) ) {
							$cache[$key] = $cache[$key] + $secondary[$key];
						} elseif ( in_array( $key, self::$mMergeableListKeys ) ) {
							$cache[$key] = array_merge( $secondary[$key], $cache[$key] );
						} elseif ( in_array( $key, self::$mMergeableAliasListKeys ) ) {
							$cache[$key] = array_merge_recursive( $cache[$key], $secondary[$key] );
						}
					}
				} else {
					$cache[$key] = $secondary[$key];
				}
			}

			# Merge bookstore lists if requested
			if ( !empty( $cache['bookstoreList']['inherit'] ) ) {
				$cache['bookstoreList'] = array_merge( $cache['bookstoreList'], $secondary['bookstoreList'] );
			}
			if ( isset( $cache['bookstoreList']['inherit'] ) ) {
				unset( $cache['bookstoreList']['inherit'] );
			}
		}
		
		# Add dependencies to the cache entry
		$cache['deps'] = $deps;

		# Replace spaces with underscores in namespace names
		$cache['namespaceNames'] = str_replace( ' ', '_', $cache['namespaceNames'] );

		# And do the same for specialpage aliases. $page is an array.
		foreach ( $cache['specialPageAliases'] as &$page ) {
			$page = str_replace( ' ', '_', $page );
		}
		# Decouple the reference to prevent accidental damage
		unset($page);
		
		# Save to both caches
		self::$mLocalisationCache[$code] = $cache;
		if ( !$disableCache ) {
			$wgMemc->set( $memcKey, $cache );
			$wgMemc->set( $fbMemcKey, (string) $cache['fallback'] );
		}

		wfProfileOut( __METHOD__ );
		return $deps;
	}

	/**
	 * Test if a given localisation cache is out of date with respect to the 
	 * source Messages files. This is done automatically for the global cache
	 * in $wgMemc, but is only done on certain occasions for the serialized 
	 * data file.
	 *
	 * @param $cache mixed Either a language code or a cache array
	 */
	static function isLocalisationOutOfDate( $cache ) {
		if ( !is_array( $cache ) ) {
			self::loadLocalisation( $cache );
			$cache = self::$mLocalisationCache[$cache];
		}
		$expired = false;
		foreach ( $cache['deps'] as $file => $mtime ) {
			if ( !file_exists( $file ) || filemtime( $file ) > $mtime ) {
				$expired = true;
				break;
			}
		}
		return $expired;
	}
	
	/**
	 * Get the fallback for a given language
	 */
	static function getFallbackFor( $code ) {
		// Shortcut
		if ( $code === 'en' ) return false;

		// Local cache
		static $cache = array();
		// Quick return
		if ( isset($cache[$code]) ) return $cache[$code];

		// Try memcache
		global $wgMemc;
		$memcKey = wfMemcKey( 'fallback', $code );
		$fbcode = $wgMemc->get( $memcKey );

		if ( is_string($fbcode) ) {
			// False is stored as a string to detect failures in memcache properly
			if ( $fbcode === '' ) $fbcode = false;

			// Update local cache and return
			$cache[$code] = $fbcode;
			return $fbcode;
		}

		// Nothing in caches, load and and update both caches
		self::loadLocalisation( $code );
		$fbcode = self::$mLocalisationCache[$code]['fallback'];

		$cache[$code] = $fbcode;
		$wgMemc->set( $memcKey, (string) $fbcode );

		return $fbcode;
	}

	/** 
	 * Get all messages for a given language
	 */
	static function getMessagesFor( $code ) {
		self::loadLocalisation( $code );
		return self::$mLocalisationCache[$code]['messages'];
	}

	/** 
	 * Get a message for a given language
	 */
	static function getMessageFor( $key, $code ) {
		self::loadLocalisation( $code );
		return isset( self::$mLocalisationCache[$code]['messages'][$key] ) ? self::$mLocalisationCache[$code]['messages'][$key] : null;
	}

	/**
	 * Load localisation data for this object
	 */
	function load() {
		if ( !$this->mLoaded ) {
			self::loadLocalisation( $this->getCode() );
			$cache =& self::$mLocalisationCache[$this->getCode()];
			foreach ( self::$mLocalisationKeys as $key ) {
				$this->$key = $cache[$key];
			}
			$this->mLoaded = true;

			$this->fixUpSettings();
		}
	}

	/**
	 * Do any necessary post-cache-load settings adjustment
	 */
	function fixUpSettings() {
		global $wgExtraNamespaces, $wgMetaNamespace, $wgMetaNamespaceTalk,
			$wgNamespaceAliases, $wgAmericanDates;
		wfProfileIn( __METHOD__ );
		if ( $wgExtraNamespaces ) {
			$this->namespaceNames = $wgExtraNamespaces + $this->namespaceNames;
		}

		$this->namespaceNames[NS_PROJECT] = $wgMetaNamespace;
		if ( $wgMetaNamespaceTalk ) {
			$this->namespaceNames[NS_PROJECT_TALK] = $wgMetaNamespaceTalk;
		} else {
			$talk = $this->namespaceNames[NS_PROJECT_TALK];
			$talk = str_replace( '$1', $wgMetaNamespace, $talk );

			# Allow grammar transformations
			# Allowing full message-style parsing would make simple requests 
			# such as action=raw much more expensive than they need to be. 
			# This will hopefully cover most cases.
			$talk = preg_replace_callback( '/{{grammar:(.*?)\|(.*?)}}/i', 
				array( &$this, 'replaceGrammarInNamespace' ), $talk );
			$talk = str_replace( ' ', '_', $talk );
			$this->namespaceNames[NS_PROJECT_TALK] = $talk;
		}
		
		# The above mixing may leave namespaces out of canonical order.
		# Re-order by namespace ID number...
		ksort( $this->namespaceNames );

		# Put namespace names and aliases into a hashtable.
		# If this is too slow, then we should arrange it so that it is done 
		# before caching. The catch is that at pre-cache time, the above
		# class-specific fixup hasn't been done.
		$this->mNamespaceIds = array();
		foreach ( $this->namespaceNames as $index => $name ) {
			$this->mNamespaceIds[$this->lc($name)] = $index;
		}
		if ( $this->namespaceAliases ) {
			foreach ( $this->namespaceAliases as $name => $index ) {
				$this->mNamespaceIds[$this->lc($name)] = $index;
			}
		}
		if ( $wgNamespaceAliases ) {
			foreach ( $wgNamespaceAliases as $name => $index ) {
				$this->mNamespaceIds[$this->lc($name)] = $index;
			}
		}

		if ( $this->defaultDateFormat == 'dmy or mdy' ) {
			$this->defaultDateFormat = $wgAmericanDates ? 'mdy' : 'dmy';
		}
		wfProfileOut( __METHOD__ );
	}

	function replaceGrammarInNamespace( $m ) {
		return $this->convertGrammar( trim( $m[2] ), trim( $m[1] ) );
	}

	static function getCaseMaps() {
		static $wikiUpperChars, $wikiLowerChars;
		if ( isset( $wikiUpperChars ) ) {
			return array( $wikiUpperChars, $wikiLowerChars );
		}

		wfProfileIn( __METHOD__ );
		$arr = wfGetPrecompiledData( 'Utf8Case.ser' );
		if ( $arr === false ) {
			throw new MWException( 
				"Utf8Case.ser is missing, please run \"make\" in the serialized directory\n" );
		}
		extract( $arr );
		wfProfileOut( __METHOD__ );
		return array( $wikiUpperChars, $wikiLowerChars );
	}

	function formatTimePeriod( $seconds ) {
		if ( $seconds < 10 ) {
			return $this->formatNum( sprintf( "%.1f", $seconds ) ) . wfMsg( 'seconds-abbrev' );
		} elseif ( $seconds < 60 ) {
			return $this->formatNum( round( $seconds ) ) . wfMsg( 'seconds-abbrev' );
		} elseif ( $seconds < 3600 ) {
			return $this->formatNum( floor( $seconds / 60 ) ) . wfMsg( 'minutes-abbrev' ) . 
				$this->formatNum( round( fmod( $seconds, 60 ) ) ) . wfMsg( 'seconds-abbrev' );
		} else {
			$hours = floor( $seconds / 3600 );
			$minutes = floor( ( $seconds - $hours * 3600 ) / 60 );
			$secondsPart = round( $seconds - $hours * 3600 - $minutes * 60 );
			return $this->formatNum( $hours ) . wfMsg( 'hours-abbrev' ) . 
				$this->formatNum( $minutes ) . wfMsg( 'minutes-abbrev' ) .
				$this->formatNum( $secondsPart ) . wfMsg( 'seconds-abbrev' );
		}
	}

	function formatBitrate( $bps ) {
		$units = array( 'bps', 'kbps', 'Mbps', 'Gbps' );
		if ( $bps <= 0 ) {
			return $this->formatNum( $bps ) . $units[0];
		}
		$unitIndex = floor( log10( $bps ) / 3 );
		$mantissa = $bps / pow( 1000, $unitIndex );
		if ( $mantissa < 10 ) {
			$mantissa = round( $mantissa, 1 );
		} else {
			$mantissa = round( $mantissa );
		}
		return $this->formatNum( $mantissa ) . $units[$unitIndex];
	}

	/**
	 * Format a size in bytes for output, using an appropriate
	 * unit (B, KB, MB or GB) according to the magnitude in question
	 *
	 * @param $size Size to format
	 * @return string Plain text (not HTML)
	 */
	function formatSize( $size ) {
		// For small sizes no decimal places necessary
		$round = 0;
		if( $size > 1024 ) {
			$size = $size / 1024;
			if( $size > 1024 ) {
				$size = $size / 1024;
				// For MB and bigger two decimal places are smarter
				$round = 2;
				if( $size > 1024 ) {
					$size = $size / 1024;
					$msg = 'size-gigabytes';
				} else {
					$msg = 'size-megabytes';
				}
			} else {
				$msg = 'size-kilobytes';
			}
		} else {
			$msg = 'size-bytes';
		}
		$size = round( $size, $round );
		$text = $this->getMessageFromDB( $msg );
		return str_replace( '$1', $this->formatNum( $size ), $text );
	}
}
