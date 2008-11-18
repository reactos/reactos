<?php
/**
 * Walloon (Walon)
 *
 * @ingroup Language
 */

# NOTE: cweri après "NOTE:" po des racsegnes so des ratournaedjes
# k' i gn a.

class LanguageWa extends Language {
	###
	### Dates in Walloon are "1î d' <monthname>" for 1st of the month,
	### "<day> di <monthname>" for months starting by a consoun, and
	### "<day> d' <monthname>" for months starting with a vowel
	###
	function date( $ts, $adj = false, $format = true, $tc = false ) {
		global $wgUser;

		if ( $adj ) { $ts = $this->userAdjust( $ts, $tc ); }
		$datePreference = $this->dateFormat( $format );

		# ISO (YYYY-mm-dd) format
		#
		# we also output this format for YMD (eg: 2001 January 15)
		if ( $datePreference == 'ISO 8601' ) {
		       $d = substr($ts, 0, 4). '-' . substr($ts, 4, 2). '-' .substr($ts, 6, 2);
		       return $d;
		}

		# dd/mm/YYYY format
		if ( $datePreference == 'walloon short' ) {
		       $d = substr($ts, 6, 2). '/' . substr($ts, 4, 2). '/' .substr($ts, 0, 4);
		       return $d;
		}

		# Walloon format
		#
		# we output this in all other cases
		$m = substr( $ts, 4, 2 );
		$n = substr( $ts, 6, 2 );
		if ($n == 1) {
		    $d = "1î d' " . $this->getMonthName( $m ) .
			" " .  substr( $ts, 0, 4 );
		} else if ($n == 2 || $n == 3 || $n == 20 || $n == 22 || $n == 23) {
		    $d = (0 + $n) . " d' " . $this->getMonthName( $m ) .
			" " .  substr( $ts, 0, 4 );
		} else if ($m == 4 || $m == 8 || $m == 10) {
		    $d = (0 + $n) . " d' " . $this->getMonthName( $m ) .
			" " .  substr( $ts, 0, 4 );
		} else {
		    $d = (0 + $n) . " di " . $this->getMonthName( $m ) .
			" " .  substr( $ts, 0, 4 );
		}
		return $d;
	}

	function timeanddate( $ts, $adj = false, $format = true, $tc = false ) {
		if ( $adj ) { $ts = $this->userAdjust( $ts, $tc ); }
		$datePreference = $this->dateFormat( $format );
		if ( $datePreference == 'ISO 8601' ) {
			return parent::timeanddate( $ts, $adj, $format, $tc );
		} else {
			return $this->date( $ts, $adj, $format, $tc ) . ' a ' .
				$this->time( $ts, $adj, $format, $tc );
		}
	}
}
