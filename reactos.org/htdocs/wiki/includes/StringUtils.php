<?php
/**
 * A collection of static methods to play with strings.
 */
class StringUtils {
	/**
	 * Perform an operation equivalent to
	 *
	 *     preg_replace( "!$startDelim(.*?)$endDelim!", $replace, $subject );
	 *
	 * except that it's worst-case O(N) instead of O(N^2)
	 *
	 * Compared to delimiterReplace(), this implementation is fast but memory-
	 * hungry and inflexible. The memory requirements are such that I don't
	 * recommend using it on anything but guaranteed small chunks of text.
	 */
	static function hungryDelimiterReplace( $startDelim, $endDelim, $replace, $subject ) {
		$segments = explode( $startDelim, $subject );
		$output = array_shift( $segments );
		foreach ( $segments as $s ) {
			$endDelimPos = strpos( $s, $endDelim );
			if ( $endDelimPos === false ) {
				$output .= $startDelim . $s;
			} else {
				$output .= $replace . substr( $s, $endDelimPos + strlen( $endDelim ) );
			}
		}
		return $output;
	}

	/**
	 * Perform an operation equivalent to
	 *
	 *   preg_replace_callback( "!$startDelim(.*)$endDelim!s$flags", $callback, $subject )
	 *
	 * This implementation is slower than hungryDelimiterReplace but uses far less
	 * memory. The delimiters are literal strings, not regular expressions.
	 *
	 * @param string $flags Regular expression flags
	 */
	# If the start delimiter ends with an initial substring of the end delimiter,
	# e.g. in the case of C-style comments, the behaviour differs from the model
	# regex. In this implementation, the end must share no characters with the
	# start, so e.g. /*/ is not considered to be both the start and end of a
	# comment. /*/xy/*/ is considered to be a single comment with contents /xy/.
	static function delimiterReplaceCallback( $startDelim, $endDelim, $callback, $subject, $flags = '' ) {
		$inputPos = 0;
		$outputPos = 0;
		$output = '';
		$foundStart = false;
		$encStart = preg_quote( $startDelim, '!' );
		$encEnd = preg_quote( $endDelim, '!' );
		$strcmp = strpos( $flags, 'i' ) === false ? 'strcmp' : 'strcasecmp';
		$endLength = strlen( $endDelim );
		$m = array();

		while ( $inputPos < strlen( $subject ) &&
		  preg_match( "!($encStart)|($encEnd)!S$flags", $subject, $m, PREG_OFFSET_CAPTURE, $inputPos ) )
		{
			$tokenOffset = $m[0][1];
			if ( $m[1][0] != '' ) {
				if ( $foundStart &&
				  $strcmp( $endDelim, substr( $subject, $tokenOffset, $endLength ) ) == 0 )
				{
					# An end match is present at the same location
					$tokenType = 'end';
					$tokenLength = $endLength;
				} else {
					$tokenType = 'start';
					$tokenLength = strlen( $m[0][0] );
				}
			} elseif ( $m[2][0] != '' ) {
				$tokenType = 'end';
				$tokenLength = strlen( $m[0][0] );
			} else {
				throw new MWException( 'Invalid delimiter given to ' . __METHOD__ );
			}

			if ( $tokenType == 'start' ) {
				$inputPos = $tokenOffset + $tokenLength;
				# Only move the start position if we haven't already found a start
				# This means that START START END matches outer pair
				if ( !$foundStart ) {
					# Found start
					# Write out the non-matching section
					$output .= substr( $subject, $outputPos, $tokenOffset - $outputPos );
					$outputPos = $tokenOffset;
					$contentPos = $inputPos;
					$foundStart = true;
				}
			} elseif ( $tokenType == 'end' ) {
				if ( $foundStart ) {
					# Found match
					$output .= call_user_func( $callback, array(
						substr( $subject, $outputPos, $tokenOffset + $tokenLength - $outputPos ),
						substr( $subject, $contentPos, $tokenOffset - $contentPos )
					));
					$foundStart = false;
				} else {
					# Non-matching end, write it out
					$output .= substr( $subject, $inputPos, $tokenOffset + $tokenLength - $outputPos );
				}
				$inputPos = $outputPos = $tokenOffset + $tokenLength;
			} else {
				throw new MWException( 'Invalid delimiter given to ' . __METHOD__ );
			}
		}
		if ( $outputPos < strlen( $subject ) ) {
			$output .= substr( $subject, $outputPos );
		}
		return $output;
	}

	/*
	 * Perform an operation equivalent to
	 *
	 *   preg_replace( "!$startDelim(.*)$endDelim!$flags", $replace, $subject )
	 *
	 * @param string $startDelim Start delimiter regular expression
	 * @param string $endDelim End delimiter regular expression
	 * @param string $replace Replacement string. May contain $1, which will be
	 *               replaced by the text between the delimiters
	 * @param string $subject String to search
	 * @return string The string with the matches replaced
	 */
	static function delimiterReplace( $startDelim, $endDelim, $replace, $subject, $flags = '' ) {
		$replacer = new RegexlikeReplacer( $replace );
		return self::delimiterReplaceCallback( $startDelim, $endDelim,
			$replacer->cb(), $subject, $flags );
	}

	/**
	 * More or less "markup-safe" explode()
	 * Ignores any instances of the separator inside <...>
	 * @param string $separator
	 * @param string $text
	 * @return array
	 */
	static function explodeMarkup( $separator, $text ) {
		$placeholder = "\x00";

		// Remove placeholder instances
		$text = str_replace( $placeholder, '', $text );

		// Replace instances of the separator inside HTML-like tags with the placeholder
		$replacer = new DoubleReplacer( $separator, $placeholder );
		$cleaned = StringUtils::delimiterReplaceCallback( '<', '>', $replacer->cb(), $text );

		// Explode, then put the replaced separators back in
		$items = explode( $separator, $cleaned );
		foreach( $items as $i => $str ) {
			$items[$i] = str_replace( $placeholder, $separator, $str );
		}

		return $items;
	}

	/**
	 * Escape a string to make it suitable for inclusion in a preg_replace()
	 * replacement parameter.
	 *
	 * @param string $string
	 * @return string
	 */
	static function escapeRegexReplacement( $string ) {
		$string = str_replace( '\\', '\\\\', $string );
		$string = str_replace( '$', '\\$', $string );
		return $string;
	}
}

/**
 * Base class for "replacers", objects used in preg_replace_callback() and
 * StringUtils::delimiterReplaceCallback()
 */
class Replacer {
	function cb() {
		return array( &$this, 'replace' );
	}
}

/**
 * Class to replace regex matches with a string similar to that used in preg_replace()
 */
class RegexlikeReplacer extends Replacer {
	var $r;
	function __construct( $r ) {
		$this->r = $r;
	}

	function replace( $matches ) {
		$pairs = array();
		foreach ( $matches as $i => $match ) {
			$pairs["\$$i"] = $match;
		}
		return strtr( $this->r, $pairs );
	}

}

/**
 * Class to perform secondary replacement within each replacement string
 */
class DoubleReplacer extends Replacer {
	function __construct( $from, $to, $index = 0 ) {
		$this->from = $from;
		$this->to = $to;
		$this->index = $index;
	}

	function replace( $matches ) {
		return str_replace( $this->from, $this->to, $matches[$this->index] );
	}
}

/**
 * Class to perform replacement based on a simple hashtable lookup
 */
class HashtableReplacer extends Replacer {
	var $table, $index;

	function __construct( $table, $index = 0 ) {
		$this->table = $table;
		$this->index = $index;
	}

	function replace( $matches ) {
		return $this->table[$matches[$this->index]];
	}
}

/**
 * Replacement array for FSS with fallback to strtr()
 * Supports lazy initialisation of FSS resource
 */
class ReplacementArray {
	/*mostly private*/ var $data = false;
	/*mostly private*/ var $fss = false;

	/**
	 * Create an object with the specified replacement array
	 * The array should have the same form as the replacement array for strtr()
	 */
	function __construct( $data = array() ) {
		$this->data = $data;
	}

	function __sleep() {
		return array( 'data' );
	}

	function __wakeup() {
		$this->fss = false;
	}

	/**
	 * Set the whole replacement array at once
	 */
	function setArray( $data ) {
		$this->data = $data;
		$this->fss = false;
	}

	function getArray() {
		return $this->data;
	}

	/**
	 * Set an element of the replacement array
	 */
	function setPair( $from, $to ) {
		$this->data[$from] = $to;
		$this->fss = false;
	}

	function mergeArray( $data ) {
		$this->data = array_merge( $this->data, $data );
		$this->fss = false;
	}

	function merge( $other ) {
		$this->data = array_merge( $this->data, $other->data );
		$this->fss = false;
	}

	function removePair( $from ) {
		unset($this->data[$from]);
		$this->fss = false;
	}

	function removeArray( $data ) {
		foreach( $data as $from => $to )
			$this->removePair( $from );
		$this->fss = false;
	}

	function replace( $subject ) {
		if ( function_exists( 'fss_prep_replace' ) ) {
			wfProfileIn( __METHOD__.'-fss' );
			if ( $this->fss === false ) {
				$this->fss = fss_prep_replace( $this->data );
			}
			$result = fss_exec_replace( $this->fss, $subject );
			wfProfileOut( __METHOD__.'-fss' );
		} else {
			wfProfileIn( __METHOD__.'-strtr' );
			$result = strtr( $subject, $this->data );
			wfProfileOut( __METHOD__.'-strtr' );
		}
		return $result;
	}
}
