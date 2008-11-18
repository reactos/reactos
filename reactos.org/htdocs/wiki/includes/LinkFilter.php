<?php

/**
 * Some functions to help implement an external link filter for spam control.
 * 
 * TODO: implement the filter. Currently these are just some functions to help
 * maintenance/cleanupSpam.php remove links to a single specified domain. The
 * next thing is to implement functions for checking a given page against a big
 * list of domains.
 *
 * Another cool thing to do would be a web interface for fast spam removal.
 */
class LinkFilter {
	/**
	 * @static
	 */
	static function matchEntry( $text, $filterEntry ) {
		$regex = LinkFilter::makeRegex( $filterEntry );
		return preg_match( $regex, $text );
	}

	/**
	 * @static
	 */
	private static function makeRegex( $filterEntry ) {
		$regex = '!http://';
		if ( substr( $filterEntry, 0, 2 ) == '*.' ) {
			$regex .= '(?:[A-Za-z0-9.-]+\.|)';
			$filterEntry = substr( $filterEntry, 2 );
		}
		$regex .= preg_quote( $filterEntry, '!' ) . '!Si';
		return $regex;
	}

	/**
	 * Make a string to go after an SQL LIKE, which will match the specified
	 * string. There are several kinds of filter entry:
	 *     *.domain.com    -  Produces http://com.domain.%, matches domain.com
	 *                        and www.domain.com
	 *     domain.com      -  Produces http://com.domain./%, matches domain.com
	 *                        or domain.com/ but not www.domain.com
	 *     *.domain.com/x  -  Produces http://com.domain.%/x%, matches
	 *                        www.domain.com/xy
	 *     domain.com/x    -  Produces http://com.domain./x%, matches
	 *                        domain.com/xy but not www.domain.com/xy
	 *
	 * Asterisks in any other location are considered invalid.
	 * 
	 * @static
	 * @param $filterEntry String: domainparts
	 * @param $prot        String: protocol
	 */
	 public static function makeLike( $filterEntry , $prot = 'http://' ) {
		$db = wfGetDB( DB_MASTER );
		if ( substr( $filterEntry, 0, 2 ) == '*.' ) {
			$subdomains = true;
			$filterEntry = substr( $filterEntry, 2 );
			if ( $filterEntry == '' ) {
				// We don't want to make a clause that will match everything,
				// that could be dangerous
				return false;
			}
		} else {
			$subdomains = false;
		}
		// No stray asterisks, that could cause confusion
		// It's not simple or efficient to handle it properly so we don't
		// handle it at all.
		if ( strpos( $filterEntry, '*' ) !== false ) {
			return false;
		}
		$slash = strpos( $filterEntry, '/' );
		if ( $slash !== false ) {
			$path = substr( $filterEntry, $slash );
			$host = substr( $filterEntry, 0, $slash );
		} else {
			$path = '/';
			$host = $filterEntry;
		}
		// Reverse the labels in the hostname, convert to lower case
		// For emails reverse domainpart only
		if ( $prot == 'mailto:' && strpos($host, '@') ) {
			// complete email adress 
			$mailparts = explode( '@', $host );
			$domainpart = strtolower( implode( '.', array_reverse( explode( '.', $mailparts[1] ) ) ) );
			$host = $domainpart . '@' . $mailparts[0];
			$like = $db->escapeLike( "$prot$host" ) . "%";
		} elseif ( $prot == 'mailto:' ) {
			// domainpart of email adress only. do not add '.'
			$host = strtolower( implode( '.', array_reverse( explode( '.', $host ) ) ) );	
			$like = $db->escapeLike( "$prot$host" ) . "%";			
		} else {
			$host = strtolower( implode( '.', array_reverse( explode( '.', $host ) ) ) );	
			if ( substr( $host, -1, 1 ) !== '.' ) {
				$host .= '.';
			}
			$like = $db->escapeLike( "$prot$host" );

			if ( $subdomains ) {
				$like .= '%';
			}
			if ( !$subdomains || $path !== '/' ) {
				$like .= $db->escapeLike( $path ) . '%';
			}
		}
		return $like;
	}
}
