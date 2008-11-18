<?php

/**
 * Date formatter, recognises dates in plain text and formats them accoding to user preferences.
 * @todo preferences, OutputPage
 * @ingroup Parser
 */
class DateFormatter
{
	var $mSource, $mTarget;
	var $monthNames = '', $rxDM, $rxMD, $rxDMY, $rxYDM, $rxMDY, $rxYMD;

	var $regexes, $pDays, $pMonths, $pYears;
	var $rules, $xMonths, $preferences;

	const ALL = -1;
	const NONE = 0;
	const MDY = 1;
	const DMY = 2;
	const YMD = 3;
	const ISO1 = 4;
	const LASTPREF = 4;
	const ISO2 = 5;
	const YDM = 6;
	const DM = 7;
	const MD = 8;
	const LAST = 8;

	/**
	 * @todo document
	 */
	function DateFormatter() {
		global $wgContLang;

		$this->monthNames = $this->getMonthRegex();
		for ( $i=1; $i<=12; $i++ ) {
			$this->xMonths[$wgContLang->lc( $wgContLang->getMonthName( $i ) )] = $i;
			$this->xMonths[$wgContLang->lc( $wgContLang->getMonthAbbreviation( $i ) )] = $i;
		}

		$this->regexTrail = '(?![a-z])/iu';

		# Partial regular expressions
		$this->prxDM = '\[\[(\d{1,2})[ _](' . $this->monthNames . ')]]';
		$this->prxMD = '\[\[(' . $this->monthNames . ')[ _](\d{1,2})]]';
		$this->prxY = '\[\[(\d{1,4}([ _]BC|))]]';
		$this->prxISO1 = '\[\[(-?\d{4})]]-\[\[(\d{2})-(\d{2})]]';
		$this->prxISO2 = '\[\[(-?\d{4})-(\d{2})-(\d{2})]]';

		# Real regular expressions
		$this->regexes[self::DMY] = "/{$this->prxDM} *,? *{$this->prxY}{$this->regexTrail}";
		$this->regexes[self::YDM] = "/{$this->prxY} *,? *{$this->prxDM}{$this->regexTrail}";
		$this->regexes[self::MDY] = "/{$this->prxMD} *,? *{$this->prxY}{$this->regexTrail}";
		$this->regexes[self::YMD] = "/{$this->prxY} *,? *{$this->prxMD}{$this->regexTrail}";
		$this->regexes[self::DM] = "/{$this->prxDM}{$this->regexTrail}";
		$this->regexes[self::MD] = "/{$this->prxMD}{$this->regexTrail}";
		$this->regexes[self::ISO1] = "/{$this->prxISO1}{$this->regexTrail}";
		$this->regexes[self::ISO2] = "/{$this->prxISO2}{$this->regexTrail}";

		# Extraction keys
		# See the comments in replace() for the meaning of the letters
		$this->keys[self::DMY] = 'jFY';
		$this->keys[self::YDM] = 'Y jF';
		$this->keys[self::MDY] = 'FjY';
		$this->keys[self::YMD] = 'Y Fj';
		$this->keys[self::DM] = 'jF';
		$this->keys[self::MD] = 'Fj';
		$this->keys[self::ISO1] = 'ymd'; # y means ISO year
		$this->keys[self::ISO2] = 'ymd';

		# Target date formats
		$this->targets[self::DMY] = '[[F j|j F]] [[Y]]';
		$this->targets[self::YDM] = '[[Y]], [[F j|j F]]';
		$this->targets[self::MDY] = '[[F j]], [[Y]]';
		$this->targets[self::YMD] = '[[Y]] [[F j]]';
		$this->targets[self::DM] = '[[F j|j F]]';
		$this->targets[self::MD] = '[[F j]]';
		$this->targets[self::ISO1] = '[[Y|y]]-[[F j|m-d]]';
		$this->targets[self::ISO2] = '[[y-m-d]]';

		# Rules
		#            pref    source 	  target
		$this->rules[self::DMY][self::MD] 	= self::DM;
		$this->rules[self::ALL][self::MD] 	= self::MD;
		$this->rules[self::MDY][self::DM] 	= self::MD;
		$this->rules[self::ALL][self::DM] 	= self::DM;
		$this->rules[self::NONE][self::ISO2] 	= self::ISO1;

		$this->preferences = array(
			'default' => self::NONE,
			'dmy' => self::DMY,
			'mdy' => self::MDY,
			'ymd' => self::YMD,
			'ISO 8601' => self::ISO1,
		);
	}

	/**
	 * @static
	 */
	function &getInstance() {
		global $wgMemc;
		static $dateFormatter = false;
		if ( !$dateFormatter ) {
			$dateFormatter = $wgMemc->get( wfMemcKey( 'dateformatter' ) );
			if ( !$dateFormatter ) {
				$dateFormatter = new DateFormatter;
				$wgMemc->set( wfMemcKey( 'dateformatter' ), $dateFormatter, 3600 );
			}
		}
		return $dateFormatter;
	}

	/**
	 * @param string $preference User preference
	 * @param string $text Text to reformat
	 */
	function reformat( $preference, $text ) {
		if ( isset( $this->preferences[$preference] ) ) {
			$preference = $this->preferences[$preference];
		} else {
			$preference = self::NONE;
		}
		for ( $i=1; $i<=self::LAST; $i++ ) {
			$this->mSource = $i;
			if ( isset ( $this->rules[$preference][$i] ) ) {
				# Specific rules
				$this->mTarget = $this->rules[$preference][$i];
			} elseif ( isset ( $this->rules[self::ALL][$i] ) ) {
				# General rules
				$this->mTarget = $this->rules[self::ALL][$i];
			} elseif ( $preference ) {
				# User preference
				$this->mTarget = $preference;
			} else {
				# Default
				$this->mTarget = $i;
			}
			$text = preg_replace_callback( $this->regexes[$i], array( &$this, 'replace' ), $text );
		}
		return $text;
	}

	/**
	 * @param $matches
	 */
	function replace( $matches ) {
		# Extract information from $matches
		$bits = array();
		$key = $this->keys[$this->mSource];
		for ( $p=0; $p < strlen($key); $p++ ) {
			if ( $key{$p} != ' ' ) {
				$bits[$key{$p}] = $matches[$p+1];
			}
		}

		$format = $this->targets[$this->mTarget];

		# Construct new date
		$text = '';
		$fail = false;

		for ( $p=0; $p < strlen( $format ); $p++ ) {
			$char = $format{$p};
			switch ( $char ) {
				case 'd': # ISO day of month
					if ( !isset($bits['d']) ) {
						$text .= sprintf( '%02d', $bits['j'] );
					} else {
						$text .= $bits['d'];
					}
					break;
				case 'm': # ISO month
					if ( !isset($bits['m']) ) {
						$m = $this->makeIsoMonth( $bits['F'] );
						if ( !$m || $m == '00' ) {
							$fail = true;
						} else {
							$text .= $m;
						}
					} else {
						$text .= $bits['m'];
					}
					break;
				case 'y': # ISO year
					if ( !isset( $bits['y'] ) ) {
						$text .= $this->makeIsoYear( $bits['Y'] );
					} else {
						$text .= $bits['y'];
					}
					break;
				case 'j': # ordinary day of month
					if ( !isset($bits['j']) ) {
						$text .= intval( $bits['d'] );
					} else {
						$text .= $bits['j'];
					}
					break;
				case 'F': # long month
					if ( !isset( $bits['F'] ) ) {
						$m = intval($bits['m']);
						if ( $m > 12 || $m < 1 ) {
							$fail = true;
						} else {
							global $wgContLang;
							$text .= $wgContLang->getMonthName( $m );
						}
					} else {
						$text .= ucfirst( $bits['F'] );
					}
					break;
				case 'Y': # ordinary (optional BC) year
					if ( !isset( $bits['Y'] ) ) {
						$text .= $this->makeNormalYear( $bits['y'] );
					} else {
						$text .= $bits['Y'];
					}
					break;
				default:
					$text .= $char;
			}
		}
		if ( $fail ) {
			$text = $matches[0];
		}
		return $text;
	}

	/**
	 * @todo document
	 */
	function getMonthRegex() {
		global $wgContLang;
		$names = array();
		for( $i = 1; $i <= 12; $i++ ) {
			$names[] = $wgContLang->getMonthName( $i );
			$names[] = $wgContLang->getMonthAbbreviation( $i );
		}
		return implode( '|', $names );
	}

	/**
	 * Makes an ISO month, e.g. 02, from a month name
	 * @param $monthName String: month name
	 * @return string ISO month name
	 */
	function makeIsoMonth( $monthName ) {
		global $wgContLang;

		$n = $this->xMonths[$wgContLang->lc( $monthName )];
		return sprintf( '%02d', $n );
	}

	/**
	 * @todo document
	 * @param $year String: Year name
	 * @return string ISO year name
	 */
	function makeIsoYear( $year ) {
		# Assumes the year is in a nice format, as enforced by the regex
		if ( substr( $year, -2 ) == 'BC' ) {
			$num = intval(substr( $year, 0, -3 )) - 1;
			# PHP bug note: sprintf( "%04d", -1 ) fails poorly
			$text = sprintf( '-%04d', $num );

		} else {
			$text = sprintf( '%04d', $year );
		}
		return $text;
	}

	/**
	 * @todo document
	 */
	function makeNormalYear( $iso ) {
		if ( $iso{0} == '-' ) {
			$text = (intval( substr( $iso, 1 ) ) + 1) . ' BC';
		} else {
			$text = intval( $iso );
		}
		return $text;
	}
}
