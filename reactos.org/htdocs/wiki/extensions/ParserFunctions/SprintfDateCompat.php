<?php

# sprintfDate support for MW<1.8 installations
class SprintfDateCompat {

	function sprintfDate( $format, $ts ) {
		global $wgContLang;

		$s = '';
		$raw = false;
		$roman = false;
		$unix = false;
		$rawToggle = false;
		for ( $p = 0; $p < strlen( $format ); $p++ ) {
			$num = false;
			$code = $format[$p];
			if ( $code == 'x' && $p < strlen( $format ) - 1 ) {
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
				case 'xg':
					$s .= $wgContLang->getMonthNameGen( substr( $ts, 4, 2 ) );
					break;
				case 'd':
					$num = substr( $ts, 6, 2 );
					break;
				case 'D':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					# Weekday abbreviations are not available in MW<1.8
					#$s .= $this->getWeekdayAbbreviation( date( 'w', $unix ) + 1 );
					$s .= date( 'D', $unix );
					break;
				case 'j':
					$num = intval( substr( $ts, 6, 2 ) );
					break;
				case 'l':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$s .= $wgContLang->getWeekdayName( date( 'w', $unix ) + 1 );
					break;
				case 'N':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$w = date( 'w', $unix );
					$num = $w ? $w : 7;
					break;
				case 'w':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = date( 'w', $unix );
					break;
				case 'z':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = date( 'z', $unix );
					break;
				case 'W':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = date( 'W', $unix );
					break;
				case 'F':
					$s .= $wgContLang->getMonthName( substr( $ts, 4, 2 ) );
					break;
				case 'm':
					$num = substr( $ts, 4, 2 );
					break;
				case 'M':
					$s .= $wgContLang->getMonthAbbreviation( substr( $ts, 4, 2 ) );
					break;
				case 'n':
					$num = intval( substr( $ts, 4, 2 ) );
					break;
				case 't':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = date( 't', $unix );
					break;
				case 'L':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$num = date( 'L', $unix );
					break;
				case 'Y':
					$num = substr( $ts, 0, 4 );
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
					$s .= date( 'c', $unix );
					break;
				case 'r':
					if ( !$unix ) $unix = wfTimestamp( TS_UNIX, $ts );
					$s .= date( 'r', $unix );
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
				} else {
					$s .= $wgContLang->formatNum( $num, true );
				}
				$num = false;
			}
		}
		return $s;
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
}
