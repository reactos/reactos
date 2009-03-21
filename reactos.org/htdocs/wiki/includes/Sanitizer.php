<?php
/**
 * XHTML sanitizer for MediaWiki
 *
 * Copyright (C) 2002-2005 Brion Vibber <brion@pobox.com> et al
 * http://www.mediawiki.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @file
 * @ingroup Parser
 */

/**
 * Regular expression to match various types of character references in
 * Sanitizer::normalizeCharReferences and Sanitizer::decodeCharReferences
 */
define( 'MW_CHAR_REFS_REGEX',
	'/&([A-Za-z0-9\x80-\xff]+);
	 |&\#([0-9]+);
	 |&\#x([0-9A-Za-z]+);
	 |&\#X([0-9A-Za-z]+);
	 |(&)/x' );

/**
 * Regular expression to match HTML/XML attribute pairs within a tag.
 * Allows some... latitude.
 * Used in Sanitizer::fixTagAttributes and Sanitizer::decodeTagAttributes
 */
$attrib = '[A-Za-z0-9]';
$space = '[\x09\x0a\x0d\x20]';
define( 'MW_ATTRIBS_REGEX',
	"/(?:^|$space)($attrib+)
	  ($space*=$space*
		(?:
		 # The attribute value: quoted or alone
		  \"([^<\"]*)\"
		 | '([^<']*)'
		 |  ([a-zA-Z0-9!#$%&()*,\\-.\\/:;<>?@[\\]^_`{|}~]+)
		 |  (\#[0-9a-fA-F]+) # Technically wrong, but lots of
							 # colors are specified like this.
							 # We'll be normalizing it.
		)
	   )?(?=$space|\$)/sx" );

/**
 * List of all named character entities defined in HTML 4.01
 * http://www.w3.org/TR/html4/sgml/entities.html
 * @private
 */
global $wgHtmlEntities;
$wgHtmlEntities = array(
	'Aacute'   => 193,
	'aacute'   => 225,
	'Acirc'    => 194,
	'acirc'    => 226,
	'acute'    => 180,
	'AElig'    => 198,
	'aelig'    => 230,
	'Agrave'   => 192,
	'agrave'   => 224,
	'alefsym'  => 8501,
	'Alpha'    => 913,
	'alpha'    => 945,
	'amp'      => 38,
	'and'      => 8743,
	'ang'      => 8736,
	'Aring'    => 197,
	'aring'    => 229,
	'asymp'    => 8776,
	'Atilde'   => 195,
	'atilde'   => 227,
	'Auml'     => 196,
	'auml'     => 228,
	'bdquo'    => 8222,
	'Beta'     => 914,
	'beta'     => 946,
	'brvbar'   => 166,
	'bull'     => 8226,
	'cap'      => 8745,
	'Ccedil'   => 199,
	'ccedil'   => 231,
	'cedil'    => 184,
	'cent'     => 162,
	'Chi'      => 935,
	'chi'      => 967,
	'circ'     => 710,
	'clubs'    => 9827,
	'cong'     => 8773,
	'copy'     => 169,
	'crarr'    => 8629,
	'cup'      => 8746,
	'curren'   => 164,
	'dagger'   => 8224,
	'Dagger'   => 8225,
	'darr'     => 8595,
	'dArr'     => 8659,
	'deg'      => 176,
	'Delta'    => 916,
	'delta'    => 948,
	'diams'    => 9830,
	'divide'   => 247,
	'Eacute'   => 201,
	'eacute'   => 233,
	'Ecirc'    => 202,
	'ecirc'    => 234,
	'Egrave'   => 200,
	'egrave'   => 232,
	'empty'    => 8709,
	'emsp'     => 8195,
	'ensp'     => 8194,
	'Epsilon'  => 917,
	'epsilon'  => 949,
	'equiv'    => 8801,
	'Eta'      => 919,
	'eta'      => 951,
	'ETH'      => 208,
	'eth'      => 240,
	'Euml'     => 203,
	'euml'     => 235,
	'euro'     => 8364,
	'exist'    => 8707,
	'fnof'     => 402,
	'forall'   => 8704,
	'frac12'   => 189,
	'frac14'   => 188,
	'frac34'   => 190,
	'frasl'    => 8260,
	'Gamma'    => 915,
	'gamma'    => 947,
	'ge'       => 8805,
	'gt'       => 62,
	'harr'     => 8596,
	'hArr'     => 8660,
	'hearts'   => 9829,
	'hellip'   => 8230,
	'Iacute'   => 205,
	'iacute'   => 237,
	'Icirc'    => 206,
	'icirc'    => 238,
	'iexcl'    => 161,
	'Igrave'   => 204,
	'igrave'   => 236,
	'image'    => 8465,
	'infin'    => 8734,
	'int'      => 8747,
	'Iota'     => 921,
	'iota'     => 953,
	'iquest'   => 191,
	'isin'     => 8712,
	'Iuml'     => 207,
	'iuml'     => 239,
	'Kappa'    => 922,
	'kappa'    => 954,
	'Lambda'   => 923,
	'lambda'   => 955,
	'lang'     => 9001,
	'laquo'    => 171,
	'larr'     => 8592,
	'lArr'     => 8656,
	'lceil'    => 8968,
	'ldquo'    => 8220,
	'le'       => 8804,
	'lfloor'   => 8970,
	'lowast'   => 8727,
	'loz'      => 9674,
	'lrm'      => 8206,
	'lsaquo'   => 8249,
	'lsquo'    => 8216,
	'lt'       => 60,
	'macr'     => 175,
	'mdash'    => 8212,
	'micro'    => 181,
	'middot'   => 183,
	'minus'    => 8722,
	'Mu'       => 924,
	'mu'       => 956,
	'nabla'    => 8711,
	'nbsp'     => 160,
	'ndash'    => 8211,
	'ne'       => 8800,
	'ni'       => 8715,
	'not'      => 172,
	'notin'    => 8713,
	'nsub'     => 8836,
	'Ntilde'   => 209,
	'ntilde'   => 241,
	'Nu'       => 925,
	'nu'       => 957,
	'Oacute'   => 211,
	'oacute'   => 243,
	'Ocirc'    => 212,
	'ocirc'    => 244,
	'OElig'    => 338,
	'oelig'    => 339,
	'Ograve'   => 210,
	'ograve'   => 242,
	'oline'    => 8254,
	'Omega'    => 937,
	'omega'    => 969,
	'Omicron'  => 927,
	'omicron'  => 959,
	'oplus'    => 8853,
	'or'       => 8744,
	'ordf'     => 170,
	'ordm'     => 186,
	'Oslash'   => 216,
	'oslash'   => 248,
	'Otilde'   => 213,
	'otilde'   => 245,
	'otimes'   => 8855,
	'Ouml'     => 214,
	'ouml'     => 246,
	'para'     => 182,
	'part'     => 8706,
	'permil'   => 8240,
	'perp'     => 8869,
	'Phi'      => 934,
	'phi'      => 966,
	'Pi'       => 928,
	'pi'       => 960,
	'piv'      => 982,
	'plusmn'   => 177,
	'pound'    => 163,
	'prime'    => 8242,
	'Prime'    => 8243,
	'prod'     => 8719,
	'prop'     => 8733,
	'Psi'      => 936,
	'psi'      => 968,
	'quot'     => 34,
	'radic'    => 8730,
	'rang'     => 9002,
	'raquo'    => 187,
	'rarr'     => 8594,
	'rArr'     => 8658,
	'rceil'    => 8969,
	'rdquo'    => 8221,
	'real'     => 8476,
	'reg'      => 174,
	'rfloor'   => 8971,
	'Rho'      => 929,
	'rho'      => 961,
	'rlm'      => 8207,
	'rsaquo'   => 8250,
	'rsquo'    => 8217,
	'sbquo'    => 8218,
	'Scaron'   => 352,
	'scaron'   => 353,
	'sdot'     => 8901,
	'sect'     => 167,
	'shy'      => 173,
	'Sigma'    => 931,
	'sigma'    => 963,
	'sigmaf'   => 962,
	'sim'      => 8764,
	'spades'   => 9824,
	'sub'      => 8834,
	'sube'     => 8838,
	'sum'      => 8721,
	'sup'      => 8835,
	'sup1'     => 185,
	'sup2'     => 178,
	'sup3'     => 179,
	'supe'     => 8839,
	'szlig'    => 223,
	'Tau'      => 932,
	'tau'      => 964,
	'there4'   => 8756,
	'Theta'    => 920,
	'theta'    => 952,
	'thetasym' => 977,
	'thinsp'   => 8201,
	'THORN'    => 222,
	'thorn'    => 254,
	'tilde'    => 732,
	'times'    => 215,
	'trade'    => 8482,
	'Uacute'   => 218,
	'uacute'   => 250,
	'uarr'     => 8593,
	'uArr'     => 8657,
	'Ucirc'    => 219,
	'ucirc'    => 251,
	'Ugrave'   => 217,
	'ugrave'   => 249,
	'uml'      => 168,
	'upsih'    => 978,
	'Upsilon'  => 933,
	'upsilon'  => 965,
	'Uuml'     => 220,
	'uuml'     => 252,
	'weierp'   => 8472,
	'Xi'       => 926,
	'xi'       => 958,
	'Yacute'   => 221,
	'yacute'   => 253,
	'yen'      => 165,
	'Yuml'     => 376,
	'yuml'     => 255,
	'Zeta'     => 918,
	'zeta'     => 950,
	'zwj'      => 8205,
	'zwnj'     => 8204 );

/**
 * Character entity aliases accepted by MediaWiki
 */
global $wgHtmlEntityAliases;
$wgHtmlEntityAliases = array(
	'רלמ' => 'rlm',
	'رلم' => 'rlm',
);


/**
 * XHTML sanitizer for MediaWiki
 * @ingroup Parser
 */
class Sanitizer {
	const NONE = 0;
	const INITIAL_NONLETTER = 1;

	/**
	 * Cleans up HTML, removes dangerous tags and attributes, and
	 * removes HTML comments
	 * @private
	 * @param string $text
	 * @param callback $processCallback to do any variable or parameter replacements in HTML attribute values
	 * @param array $args for the processing callback
	 * @return string
	 */
	static function removeHTMLtags( $text, $processCallback = null, $args = array(), $extratags = array() ) {
		global $wgUseTidy;

		static $htmlpairs, $htmlsingle, $htmlsingleonly, $htmlnest, $tabletags,
			$htmllist, $listtags, $htmlsingleallowed, $htmlelements, $staticInitialised;

		wfProfileIn( __METHOD__ );

		if ( !$staticInitialised ) {

			$htmlpairs = array_merge( $extratags, array( # Tags that must be closed
				'b', 'del', 'i', 'ins', 'u', 'font', 'big', 'small', 'sub', 'sup', 'h1',
				'h2', 'h3', 'h4', 'h5', 'h6', 'cite', 'code', 'em', 's',
				'strike', 'strong', 'tt', 'var', 'div', 'center',
				'blockquote', 'ol', 'ul', 'dl', 'table', 'caption', 'pre',
				'ruby', 'rt' , 'rb' , 'rp', 'p', 'span', 'u'
			) );
			$htmlsingle = array(
				'br', 'hr', 'li', 'dt', 'dd'
			);
			$htmlsingleonly = array( # Elements that cannot have close tags
				'br', 'hr'
			);
			$htmlnest = array( # Tags that can be nested--??
				'table', 'tr', 'td', 'th', 'div', 'blockquote', 'ol', 'ul',
				'dl', 'font', 'big', 'small', 'sub', 'sup', 'span'
			);
			$tabletags = array( # Can only appear inside table, we will close them
				'td', 'th', 'tr',
			);
			$htmllist = array( # Tags used by list
				'ul','ol',
			);
			$listtags = array( # Tags that can appear in a list
				'li',
			);

			$htmlsingleallowed = array_merge( $htmlsingle, $tabletags );
			$htmlelements = array_merge( $htmlsingle, $htmlpairs, $htmlnest );

			# Convert them all to hashtables for faster lookup
			$vars = array( 'htmlpairs', 'htmlsingle', 'htmlsingleonly', 'htmlnest', 'tabletags',
				'htmllist', 'listtags', 'htmlsingleallowed', 'htmlelements' );
			foreach ( $vars as $var ) {
				$$var = array_flip( $$var );
			}
			$staticInitialised = true;
		}

		# Remove HTML comments
		$text = Sanitizer::removeHTMLcomments( $text );
		$bits = explode( '<', $text );
		$text = str_replace( '>', '&gt;', array_shift( $bits ) );
		if(!$wgUseTidy) {
			$tagstack = $tablestack = array();
			foreach ( $bits as $x ) {
				$regs = array();
				if( preg_match( '!^(/?)(\\w+)([^>]*?)(/{0,1}>)([^<]*)$!', $x, $regs ) ) {
					list( /* $qbar */, $slash, $t, $params, $brace, $rest ) = $regs;
				} else {
					$slash = $t = $params = $brace = $rest = null;
				}

				$badtag = 0 ;
				if ( isset( $htmlelements[$t = strtolower( $t )] ) ) {
					# Check our stack
					if ( $slash ) {
						# Closing a tag...
						if( isset( $htmlsingleonly[$t] ) ) {
							$badtag = 1;
						} elseif ( ( $ot = @array_pop( $tagstack ) ) != $t ) {
							if ( isset( $htmlsingleallowed[$ot] ) ) {
								# Pop all elements with an optional close tag
								# and see if we find a match below them
								$optstack = array();
								array_push ($optstack, $ot);
								while ( ( ( $ot = @array_pop( $tagstack ) ) != $t ) &&
										isset( $htmlsingleallowed[$ot] ) )
								{
									array_push ($optstack, $ot);
								}
								if ( $t != $ot ) {
									# No match. Push the optinal elements back again
									$badtag = 1;
									while ( $ot = @array_pop( $optstack ) ) {
										array_push( $tagstack, $ot );
									}
								}
							} else {
								@array_push( $tagstack, $ot );
								# <li> can be nested in <ul> or <ol>, skip those cases:
								if(!(isset( $htmllist[$ot] ) && isset( $listtags[$t] ) )) {
									$badtag = 1;
								}
							}
						} else {
							if ( $t == 'table' ) {
								$tagstack = array_pop( $tablestack );
							}
						}
						$newparams = '';
					} else {
						# Keep track for later
						if ( isset( $tabletags[$t] ) &&
						! in_array( 'table', $tagstack ) ) {
							$badtag = 1;
						} else if ( in_array( $t, $tagstack ) &&
						! isset( $htmlnest [$t ] ) ) {
							$badtag = 1 ;
						# Is it a self closed htmlpair ? (bug 5487)
						} else if( $brace == '/>' &&
						isset( $htmlpairs[$t] ) ) {
							$badtag = 1;
						} elseif( isset( $htmlsingleonly[$t] ) ) {
							# Hack to force empty tag for uncloseable elements
							$brace = '/>';
						} else if( isset( $htmlsingle[$t] ) ) {
							# Hack to not close $htmlsingle tags
							$brace = NULL;
						} else if( isset( $tabletags[$t] )
						&&  in_array($t ,$tagstack) ) {
							// New table tag but forgot to close the previous one
							$text .= "</$t>";
						} else {
							if ( $t == 'table' ) {
								array_push( $tablestack, $tagstack );
								$tagstack = array();
							}
							array_push( $tagstack, $t );
						}

						# Replace any variables or template parameters with
						# plaintext results.
						if( is_callable( $processCallback ) ) {
							call_user_func_array( $processCallback, array( &$params, $args ) );
						}

						# Strip non-approved attributes from the tag
						$newparams = Sanitizer::fixTagAttributes( $params, $t );
					}
					if ( ! $badtag ) {
						$rest = str_replace( '>', '&gt;', $rest );
						$close = ( $brace == '/>' && !$slash ) ? ' /' : '';
						$text .= "<$slash$t$newparams$close>$rest";
						continue;
					}
				}
				$text .= '&lt;' . str_replace( '>', '&gt;', $x);
			}
			# Close off any remaining tags
			while ( is_array( $tagstack ) && ($t = array_pop( $tagstack )) ) {
				$text .= "</$t>\n";
				if ( $t == 'table' ) { $tagstack = array_pop( $tablestack ); }
			}
		} else {
			# this might be possible using tidy itself
			foreach ( $bits as $x ) {
				preg_match( '/^(\\/?)(\\w+)([^>]*?)(\\/{0,1}>)([^<]*)$/',
				$x, $regs );
				@list( /* $qbar */, $slash, $t, $params, $brace, $rest ) = $regs;
				if ( isset( $htmlelements[$t = strtolower( $t )] ) ) {
					if( is_callable( $processCallback ) ) {
						call_user_func_array( $processCallback, array( &$params, $args ) );
					}
					$newparams = Sanitizer::fixTagAttributes( $params, $t );
					$rest = str_replace( '>', '&gt;', $rest );
					$text .= "<$slash$t$newparams$brace$rest";
				} else {
					$text .= '&lt;' . str_replace( '>', '&gt;', $x);
				}
			}
		}
		wfProfileOut( __METHOD__ );
		return $text;
	}

	/**
	 * Remove '<!--', '-->', and everything between.
	 * To avoid leaving blank lines, when a comment is both preceded
	 * and followed by a newline (ignoring spaces), trim leading and
	 * trailing spaces and one of the newlines.
	 *
	 * @private
	 * @param string $text
	 * @return string
	 */
	static function removeHTMLcomments( $text ) {
		wfProfileIn( __METHOD__ );
		while (($start = strpos($text, '<!--')) !== false) {
			$end = strpos($text, '-->', $start + 4);
			if ($end === false) {
				# Unterminated comment; bail out
				break;
			}

			$end += 3;

			# Trim space and newline if the comment is both
			# preceded and followed by a newline
			$spaceStart = max($start - 1, 0);
			$spaceLen = $end - $spaceStart;
			while (substr($text, $spaceStart, 1) === ' ' && $spaceStart > 0) {
				$spaceStart--;
				$spaceLen++;
			}
			while (substr($text, $spaceStart + $spaceLen, 1) === ' ')
				$spaceLen++;
			if (substr($text, $spaceStart, 1) === "\n" and substr($text, $spaceStart + $spaceLen, 1) === "\n") {
				# Remove the comment, leading and trailing
				# spaces, and leave only one newline.
				$text = substr_replace($text, "\n", $spaceStart, $spaceLen + 1);
			}
			else {
				# Remove just the comment.
				$text = substr_replace($text, '', $start, $end - $start);
			}
		}
		wfProfileOut( __METHOD__ );
		return $text;
	}

	/**
	 * Take an array of attribute names and values and normalize or discard
	 * illegal values for the given element type.
	 *
	 * - Discards attributes not on a whitelist for the given element
	 * - Unsafe style attributes are discarded
	 * - Invalid id attributes are reencoded
	 *
	 * @param array $attribs
	 * @param string $element
	 * @return array
	 *
	 * @todo Check for legal values where the DTD limits things.
	 * @todo Check for unique id attribute :P
	 */
	static function validateTagAttributes( $attribs, $element ) {
		return Sanitizer::validateAttributes( $attribs,
			Sanitizer::attributeWhitelist( $element ) );
	}

	/**
	 * Take an array of attribute names and values and normalize or discard
	 * illegal values for the given whitelist.
	 *
	 * - Discards attributes not the given whitelist
	 * - Unsafe style attributes are discarded
	 * - Invalid id attributes are reencoded
	 *
	 * @param array $attribs
	 * @param array $whitelist list of allowed attribute names
	 * @return array
	 *
	 * @todo Check for legal values where the DTD limits things.
	 * @todo Check for unique id attribute :P
	 */
	static function validateAttributes( $attribs, $whitelist ) {
		$whitelist = array_flip( $whitelist );
		$out = array();
		foreach( $attribs as $attribute => $value ) {
			if( !isset( $whitelist[$attribute] ) ) {
				continue;
			}
			# Strip javascript "expression" from stylesheets.
			# http://msdn.microsoft.com/workshop/author/dhtml/overview/recalc.asp
			if( $attribute == 'style' ) {
				$value = Sanitizer::checkCss( $value );
				if( $value === false ) {
					# haxx0r
					continue;
				}
			}

			if ( $attribute === 'id' )
				$value = Sanitizer::escapeId( $value );

			// If this attribute was previously set, override it.
			// Output should only have one attribute of each name.
			$out[$attribute] = $value;
		}
		return $out;
	}

	/**
	 * Merge two sets of HTML attributes.
	 * Conflicting items in the second set will override those
	 * in the first, except for 'class' attributes which will be
	 * combined.
	 *
	 * @todo implement merging for other attributes such as style
	 * @param array $a
	 * @param array $b
	 * @return array
	 */
	static function mergeAttributes( $a, $b ) {
		$out = array_merge( $a, $b );
		if( isset( $a['class'] )
			&& isset( $b['class'] )
			&& $a['class'] !== $b['class'] ) {

			$out['class'] = implode( ' ',
				array_unique(
					preg_split( '/\s+/',
						$a['class'] . ' ' . $b['class'],
						-1,
						PREG_SPLIT_NO_EMPTY ) ) );
		}
		return $out;
	}

	/**
	 * Pick apart some CSS and check it for forbidden or unsafe structures.
	 * Returns a sanitized string, or false if it was just too evil.
	 *
	 * Currently URL references, 'expression', 'tps' are forbidden.
	 *
	 * @param string $value
	 * @return mixed
	 */
	static function checkCss( $value ) {
		$stripped = Sanitizer::decodeCharReferences( $value );

		// Remove any comments; IE gets token splitting wrong
		$stripped = StringUtils::delimiterReplace( '/*', '*/', ' ', $stripped );

		$value = $stripped;

		// ... and continue checks
		$stripped = preg_replace( '!\\\\([0-9A-Fa-f]{1,6})[ \\n\\r\\t\\f]?!e',
			'codepointToUtf8(hexdec("$1"))', $stripped );
		$stripped = str_replace( '\\', '', $stripped );
		if( preg_match( '/(?:expression|tps*:\/\/|url\\s*\().*/is',
				$stripped ) ) {
			# haxx0r
			return false;
		}

		return $value;
	}

	/**
	 * Take a tag soup fragment listing an HTML element's attributes
	 * and normalize it to well-formed XML, discarding unwanted attributes.
	 * Output is safe for further wikitext processing, with escaping of
	 * values that could trigger problems.
	 *
	 * - Normalizes attribute names to lowercase
	 * - Discards attributes not on a whitelist for the given element
	 * - Turns broken or invalid entities into plaintext
	 * - Double-quotes all attribute values
	 * - Attributes without values are given the name as attribute
	 * - Double attributes are discarded
	 * - Unsafe style attributes are discarded
	 * - Prepends space if there are attributes.
	 *
	 * @param string $text
	 * @param string $element
	 * @return string
	 */
	static function fixTagAttributes( $text, $element ) {
		if( trim( $text ) == '' ) {
			return '';
		}

		$stripped = Sanitizer::validateTagAttributes(
			Sanitizer::decodeTagAttributes( $text ), $element );

		$attribs = array();
		foreach( $stripped as $attribute => $value ) {
			$encAttribute = htmlspecialchars( $attribute );
			$encValue = Sanitizer::safeEncodeAttribute( $value );

			$attribs[] = "$encAttribute=\"$encValue\"";
		}
		return count( $attribs ) ? ' ' . implode( ' ', $attribs ) : '';
	}

	/**
	 * Encode an attribute value for HTML output.
	 * @param $text
	 * @return HTML-encoded text fragment
	 */
	static function encodeAttribute( $text ) {
		$encValue = htmlspecialchars( $text, ENT_QUOTES );

		// Whitespace is normalized during attribute decoding,
		// so if we've been passed non-spaces we must encode them
		// ahead of time or they won't be preserved.
		$encValue = strtr( $encValue, array(
			"\n" => '&#10;',
			"\r" => '&#13;',
			"\t" => '&#9;',
		) );

		return $encValue;
	}

	/**
	 * Encode an attribute value for HTML tags, with extra armoring
	 * against further wiki processing.
	 * @param $text
	 * @return HTML-encoded text fragment
	 */
	static function safeEncodeAttribute( $text ) {
		$encValue = Sanitizer::encodeAttribute( $text );

		# Templates and links may be expanded in later parsing,
		# creating invalid or dangerous output. Suppress this.
		$encValue = strtr( $encValue, array(
			'<'    => '&lt;',   // This should never happen,
			'>'    => '&gt;',   // we've received invalid input
			'"'    => '&quot;', // which should have been escaped.
			'{'    => '&#123;',
			'['    => '&#91;',
			"''"   => '&#39;&#39;',
			'ISBN' => '&#73;SBN',
			'RFC'  => '&#82;FC',
			'PMID' => '&#80;MID',
			'|'    => '&#124;',
			'__'   => '&#95;_',
		) );

		# Stupid hack
		$encValue = preg_replace_callback(
			'/(' . wfUrlProtocols() . ')/',
			array( 'Sanitizer', 'armorLinksCallback' ),
			$encValue );
		return $encValue;
	}

	/**
	 * Given a value escape it so that it can be used in an id attribute and
	 * return it, this does not validate the value however (see first link)
	 *
	 * @see http://www.w3.org/TR/html401/types.html#type-name Valid characters
	 *                                                          in the id and
	 *                                                          name attributes
	 * @see http://www.w3.org/TR/html401/struct/links.html#h-12.2.3 Anchors with the id attribute
	 *
	 * @param string $id    Id to validate
	 * @param int    $flags Currently only two values: Sanitizer::INITIAL_NONLETTER
	 *                      (default) permits initial non-letter characters,
	 *                      such as if you're adding a prefix to them.
	 *                      Sanitizer::NONE will prepend an 'x' if the id
	 *                      would otherwise start with a nonletter.
	 * @return string
	 */
	static function escapeId( $id, $flags = Sanitizer::INITIAL_NONLETTER ) {
		static $replace = array(
			'%3A' => ':',
			'%' => '.'
		);

		$id = urlencode( Sanitizer::decodeCharReferences( strtr( $id, ' ', '_' ) ) );
		$id = str_replace( array_keys( $replace ), array_values( $replace ), $id );

		if( ~$flags & Sanitizer::INITIAL_NONLETTER
		&& !preg_match( '/[a-zA-Z]/', $id[0] ) ) {
			// Initial character must be a letter!
			$id = "x$id";
		}
		return $id;
	}

	/**
	 * Given a value, escape it so that it can be used as a CSS class and
	 * return it.
	 *
	 * @todo For extra validity, input should be validated UTF-8.
	 *
	 * @see http://www.w3.org/TR/CSS21/syndata.html Valid characters/format
	 *
	 * @param string $class
	 * @return string
	 */
	static function escapeClass( $class ) {
		// Convert ugly stuff to underscores and kill underscores in ugly places
		return rtrim(preg_replace(
			array('/(^[0-9\\-])|[\\x00-\\x20!"#$%&\'()*+,.\\/:;<=>?@[\\]^`{|}~]|\\xC2\\xA0/','/_+/'),
			'_',
			$class ), '_');
	}

	/**
	 * Regex replace callback for armoring links against further processing.
	 * @param array $matches
	 * @return string
	 * @private
	 */
	private static function armorLinksCallback( $matches ) {
		return str_replace( ':', '&#58;', $matches[1] );
	}

	/**
	 * Return an associative array of attribute names and values from
	 * a partial tag string. Attribute names are forces to lowercase,
	 * character references are decoded to UTF-8 text.
	 *
	 * @param string
	 * @return array
	 */
	static function decodeTagAttributes( $text ) {
		$attribs = array();

		if( trim( $text ) == '' ) {
			return $attribs;
		}

		$pairs = array();
		if( !preg_match_all(
			MW_ATTRIBS_REGEX,
			$text,
			$pairs,
			PREG_SET_ORDER ) ) {
			return $attribs;
		}

		foreach( $pairs as $set ) {
			$attribute = strtolower( $set[1] );
			$value = Sanitizer::getTagAttributeCallback( $set );

			// Normalize whitespace
			$value = preg_replace( '/[\t\r\n ]+/', ' ', $value );
			$value = trim( $value );

			// Decode character references
			$attribs[$attribute] = Sanitizer::decodeCharReferences( $value );
		}
		return $attribs;
	}

	/**
	 * Pick the appropriate attribute value from a match set from the
	 * MW_ATTRIBS_REGEX matches.
	 *
	 * @param array $set
	 * @return string
	 * @private
	 */
	private static function getTagAttributeCallback( $set ) {
		if( isset( $set[6] ) ) {
			# Illegal #XXXXXX color with no quotes.
			return $set[6];
		} elseif( isset( $set[5] ) ) {
			# No quotes.
			return $set[5];
		} elseif( isset( $set[4] ) ) {
			# Single-quoted
			return $set[4];
		} elseif( isset( $set[3] ) ) {
			# Double-quoted
			return $set[3];
		} elseif( !isset( $set[2] ) ) {
			# In XHTML, attributes must have a value.
			# For 'reduced' form, return explicitly the attribute name here.
			return $set[1];
		} else {
			throw new MWException( "Tag conditions not met. This should never happen and is a bug." );
		}
	}

	/**
	 * Normalize whitespace and character references in an XML source-
	 * encoded text for an attribute value.
	 *
	 * See http://www.w3.org/TR/REC-xml/#AVNormalize for background,
	 * but note that we're not returning the value, but are returning
	 * XML source fragments that will be slapped into output.
	 *
	 * @param string $text
	 * @return string
	 * @private
	 */
	private static function normalizeAttributeValue( $text ) {
		return str_replace( '"', '&quot;',
			self::normalizeWhitespace(
				Sanitizer::normalizeCharReferences( $text ) ) );
	}

	private static function normalizeWhitespace( $text ) {
		return preg_replace(
			'/\r\n|[\x20\x0d\x0a\x09]/',
			' ',
			$text );
	}

	/**
	 * Ensure that any entities and character references are legal
	 * for XML and XHTML specifically. Any stray bits will be
	 * &amp;-escaped to result in a valid text fragment.
	 *
	 * a. any named char refs must be known in XHTML
	 * b. any numeric char refs must be legal chars, not invalid or forbidden
	 * c. use &#x, not &#X
	 * d. fix or reject non-valid attributes
	 *
	 * @param string $text
	 * @return string
	 * @private
	 */
	static function normalizeCharReferences( $text ) {
		return preg_replace_callback(
			MW_CHAR_REFS_REGEX,
			array( 'Sanitizer', 'normalizeCharReferencesCallback' ),
			$text );
	}
	/**
	 * @param string $matches
	 * @return string
	 */
	static function normalizeCharReferencesCallback( $matches ) {
		$ret = null;
		if( $matches[1] != '' ) {
			$ret = Sanitizer::normalizeEntity( $matches[1] );
		} elseif( $matches[2] != '' ) {
			$ret = Sanitizer::decCharReference( $matches[2] );
		} elseif( $matches[3] != ''  ) {
			$ret = Sanitizer::hexCharReference( $matches[3] );
		} elseif( $matches[4] != '' ) {
			$ret = Sanitizer::hexCharReference( $matches[4] );
		}
		if( is_null( $ret ) ) {
			return htmlspecialchars( $matches[0] );
		} else {
			return $ret;
		}
	}

	/**
	 * If the named entity is defined in the HTML 4.0/XHTML 1.0 DTD,
	 * return the named entity reference as is. If the entity is a
	 * MediaWiki-specific alias, returns the HTML equivalent. Otherwise,
	 * returns HTML-escaped text of pseudo-entity source (eg &amp;foo;)
	 *
	 * @param string $name
	 * @return string
	 * @static
	 */
	static function normalizeEntity( $name ) {
		global $wgHtmlEntities, $wgHtmlEntityAliases;
		if ( isset( $wgHtmlEntityAliases[$name] ) ) {
			return "&{$wgHtmlEntityAliases[$name]};";
		} elseif( isset( $wgHtmlEntities[$name] ) ) {
			return "&$name;";
		} else {
			return "&amp;$name;";
		}
	}

	static function decCharReference( $codepoint ) {
		$point = intval( $codepoint );
		if( Sanitizer::validateCodepoint( $point ) ) {
			return sprintf( '&#%d;', $point );
		} else {
			return null;
		}
	}

	static function hexCharReference( $codepoint ) {
		$point = hexdec( $codepoint );
		if( Sanitizer::validateCodepoint( $point ) ) {
			return sprintf( '&#x%x;', $point );
		} else {
			return null;
		}
	}

	/**
	 * Returns true if a given Unicode codepoint is a valid character in XML.
	 * @param int $codepoint
	 * @return bool
	 */
	private static function validateCodepoint( $codepoint ) {
		return ($codepoint ==    0x09)
			|| ($codepoint ==    0x0a)
			|| ($codepoint ==    0x0d)
			|| ($codepoint >=    0x20 && $codepoint <=   0xd7ff)
			|| ($codepoint >=  0xe000 && $codepoint <=   0xfffd)
			|| ($codepoint >= 0x10000 && $codepoint <= 0x10ffff);
	}

	/**
	 * Decode any character references, numeric or named entities,
	 * in the text and return a UTF-8 string.
	 *
	 * @param string $text
	 * @return string
	 * @public
	 * @static
	 */
	public static function decodeCharReferences( $text ) {
		return preg_replace_callback(
			MW_CHAR_REFS_REGEX,
			array( 'Sanitizer', 'decodeCharReferencesCallback' ),
			$text );
	}

	/**
	 * @param string $matches
	 * @return string
	 */
	static function decodeCharReferencesCallback( $matches ) {
		if( $matches[1] != '' ) {
			return Sanitizer::decodeEntity( $matches[1] );
		} elseif( $matches[2] != '' ) {
			return  Sanitizer::decodeChar( intval( $matches[2] ) );
		} elseif( $matches[3] != ''  ) {
			return  Sanitizer::decodeChar( hexdec( $matches[3] ) );
		} elseif( $matches[4] != '' ) {
			return  Sanitizer::decodeChar( hexdec( $matches[4] ) );
		}
		# Last case should be an ampersand by itself
		return $matches[0];
	}

	/**
	 * Return UTF-8 string for a codepoint if that is a valid
	 * character reference, otherwise U+FFFD REPLACEMENT CHARACTER.
	 * @param int $codepoint
	 * @return string
	 * @private
	 */
	static function decodeChar( $codepoint ) {
		if( Sanitizer::validateCodepoint( $codepoint ) ) {
			return codepointToUtf8( $codepoint );
		} else {
			return UTF8_REPLACEMENT;
		}
	}

	/**
	 * If the named entity is defined in the HTML 4.0/XHTML 1.0 DTD,
	 * return the UTF-8 encoding of that character. Otherwise, returns
	 * pseudo-entity source (eg &foo;)
	 *
	 * @param string $name
	 * @return string
	 */
	static function decodeEntity( $name ) {
		global $wgHtmlEntities, $wgHtmlEntityAliases;
		if ( isset( $wgHtmlEntityAliases[$name] ) ) {
			$name = $wgHtmlEntityAliases[$name];
		}
		if( isset( $wgHtmlEntities[$name] ) ) {
			return codepointToUtf8( $wgHtmlEntities[$name] );
		} else {
			return "&$name;";
		}
	}

	/**
	 * Fetch the whitelist of acceptable attributes for a given
	 * element name.
	 *
	 * @param string $element
	 * @return array
	 */
	static function attributeWhitelist( $element ) {
		static $list;
		if( !isset( $list ) ) {
			$list = Sanitizer::setupAttributeWhitelist();
		}
		return isset( $list[$element] )
			? $list[$element]
			: array();
	}

	/**
	 * @todo Document it a bit
	 * @return array
	 */
	static function setupAttributeWhitelist() {
		$common = array( 'id', 'class', 'lang', 'dir', 'title', 'style' );
		$block = array_merge( $common, array( 'align' ) );
		$tablealign = array( 'align', 'char', 'charoff', 'valign' );
		$tablecell = array( 'abbr',
		                    'axis',
		                    'headers',
		                    'scope',
		                    'rowspan',
		                    'colspan',
		                    'nowrap', # deprecated
		                    'width',  # deprecated
		                    'height', # deprecated
		                    'bgcolor' # deprecated
		                    );

		# Numbers refer to sections in HTML 4.01 standard describing the element.
		# See: http://www.w3.org/TR/html4/
		$whitelist = array (
			# 7.5.4
			'div'        => $block,
			'center'     => $common, # deprecated
			'span'       => $block, # ??

			# 7.5.5
			'h1'         => $block,
			'h2'         => $block,
			'h3'         => $block,
			'h4'         => $block,
			'h5'         => $block,
			'h6'         => $block,

			# 7.5.6
			# address

			# 8.2.4
			# bdo

			# 9.2.1
			'em'         => $common,
			'strong'     => $common,
			'cite'       => $common,
			# dfn
			'code'       => $common,
			# samp
			# kbd
			'var'        => $common,
			# abbr
			# acronym

			# 9.2.2
			'blockquote' => array_merge( $common, array( 'cite' ) ),
			# q

			# 9.2.3
			'sub'        => $common,
			'sup'        => $common,

			# 9.3.1
			'p'          => $block,

			# 9.3.2
			'br'         => array( 'id', 'class', 'title', 'style', 'clear' ),

			# 9.3.4
			'pre'        => array_merge( $common, array( 'width' ) ),

			# 9.4
			'ins'        => array_merge( $common, array( 'cite', 'datetime' ) ),
			'del'        => array_merge( $common, array( 'cite', 'datetime' ) ),

			# 10.2
			'ul'         => array_merge( $common, array( 'type' ) ),
			'ol'         => array_merge( $common, array( 'type', 'start' ) ),
			'li'         => array_merge( $common, array( 'type', 'value' ) ),

			# 10.3
			'dl'         => $common,
			'dd'         => $common,
			'dt'         => $common,

			# 11.2.1
			'table'      => array_merge( $common,
								array( 'summary', 'width', 'border', 'frame',
										'rules', 'cellspacing', 'cellpadding',
										'align', 'bgcolor',
								) ),

			# 11.2.2
			'caption'    => array_merge( $common, array( 'align' ) ),

			# 11.2.3
			'thead'      => array_merge( $common, $tablealign ),
			'tfoot'      => array_merge( $common, $tablealign ),
			'tbody'      => array_merge( $common, $tablealign ),

			# 11.2.4
			'colgroup'   => array_merge( $common, array( 'span', 'width' ), $tablealign ),
			'col'        => array_merge( $common, array( 'span', 'width' ), $tablealign ),

			# 11.2.5
			'tr'         => array_merge( $common, array( 'bgcolor' ), $tablealign ),

			# 11.2.6
			'td'         => array_merge( $common, $tablecell, $tablealign ),
			'th'         => array_merge( $common, $tablecell, $tablealign ),

			# 13.2
			# Not usually allowed, but may be used for extension-style hooks
			# such as <math> when it is rasterized
			'img'        => array_merge( $common, array( 'alt' ) ),

			# 15.2.1
			'tt'         => $common,
			'b'          => $common,
			'i'          => $common,
			'big'        => $common,
			'small'      => $common,
			'strike'     => $common,
			's'          => $common,
			'u'          => $common,

			# 15.2.2
			'font'       => array_merge( $common, array( 'size', 'color', 'face' ) ),
			# basefont

			# 15.3
			'hr'         => array_merge( $common, array( 'noshade', 'size', 'width' ) ),

			# XHTML Ruby annotation text module, simple ruby only.
			# http://www.w3c.org/TR/ruby/
			'ruby'       => $common,
			# rbc
			# rtc
			'rb'         => $common,
			'rt'         => $common, #array_merge( $common, array( 'rbspan' ) ),
			'rp'         => $common,

			# MathML root element, where used for extensions
			# 'title' may not be 100% valid here; it's XHTML
			# http://www.w3.org/TR/REC-MathML/
			'math'       => array( 'class', 'style', 'id', 'title' ),
			);
		return $whitelist;
	}

	/**
	 * Take a fragment of (potentially invalid) HTML and return
	 * a version with any tags removed, encoded as plain text.
	 *
	 * Warning: this return value must be further escaped for literal
	 * inclusion in HTML output as of 1.10!
	 *
	 * @param string $text HTML fragment
	 * @return string
	 */
	static function stripAllTags( $text ) {
		# Actual <tags>
		$text = StringUtils::delimiterReplace( '<', '>', '', $text );

		# Normalize &entities and whitespace
		$text = self::decodeCharReferences( $text );
		$text = self::normalizeWhitespace( $text );

		return $text;
	}

	/**
	 * Hack up a private DOCTYPE with HTML's standard entity declarations.
	 * PHP 4 seemed to know these if you gave it an HTML doctype, but
	 * PHP 5.1 doesn't.
	 *
	 * Use for passing XHTML fragments to PHP's XML parsing functions
	 *
	 * @return string
	 * @static
	 */
	static function hackDocType() {
		global $wgHtmlEntities;
		$out = "<!DOCTYPE html [\n";
		foreach( $wgHtmlEntities as $entity => $codepoint ) {
			$out .= "<!ENTITY $entity \"&#$codepoint;\">";
		}
		$out .= "]>\n";
		return $out;
	}

	static function cleanUrl( $url, $hostname=true ) {
		# Normalize any HTML entities in input. They will be
		# re-escaped by makeExternalLink().
		$url = Sanitizer::decodeCharReferences( $url );

		# Escape any control characters introduced by the above step
		$url = preg_replace( '/[\][<>"\\x00-\\x20\\x7F]/e', "urlencode('\\0')", $url );

		# Validate hostname portion
		$matches = array();
		if( preg_match( '!^([^:]+:)(//[^/]+)?(.*)$!iD', $url, $matches ) ) {
			list( /* $whole */, $protocol, $host, $rest ) = $matches;

			// Characters that will be ignored in IDNs.
			// http://tools.ietf.org/html/3454#section-3.1
			// Strip them before further processing so blacklists and such work.
			$strip = "/
				\\s|          # general whitespace
				\xc2\xad|     # 00ad SOFT HYPHEN
				\xe1\xa0\x86| # 1806 MONGOLIAN TODO SOFT HYPHEN
				\xe2\x80\x8b| # 200b ZERO WIDTH SPACE
				\xe2\x81\xa0| # 2060 WORD JOINER
				\xef\xbb\xbf| # feff ZERO WIDTH NO-BREAK SPACE
				\xcd\x8f|     # 034f COMBINING GRAPHEME JOINER
				\xe1\xa0\x8b| # 180b MONGOLIAN FREE VARIATION SELECTOR ONE
				\xe1\xa0\x8c| # 180c MONGOLIAN FREE VARIATION SELECTOR TWO
				\xe1\xa0\x8d| # 180d MONGOLIAN FREE VARIATION SELECTOR THREE
				\xe2\x80\x8c| # 200c ZERO WIDTH NON-JOINER
				\xe2\x80\x8d| # 200d ZERO WIDTH JOINER
				[\xef\xb8\x80-\xef\xb8\x8f] # fe00-fe00f VARIATION SELECTOR-1-16
				/xuD";

			$host = preg_replace( $strip, '', $host );

			// @fixme: validate hostnames here

			return $protocol . $host . $rest;
		} else {
			return $url;
		}
	}

}
