<?php

/**
 * @ingroup Language
 */
class LanguageZh_hans extends Language {
	function stripForSearch( $string ) {
		# MySQL fulltext index doesn't grok utf-8, so we
		# need to fold cases and convert to hex
		# we also separate characters as "words"
		if( function_exists( 'mb_strtolower' ) ) {
			return preg_replace(
				"/([\\xc0-\\xff][\\x80-\\xbf]*)/e",
				"' U8' . bin2hex( \"$1\" )",
				mb_strtolower( $string ) );
		} else {
			list( , $wikiLowerChars ) = Language::getCaseMaps();
			return preg_replace(
				"/([\\xc0-\\xff][\\x80-\\xbf]*)/e",
				"' U8' . bin2hex( strtr( \"\$1\", \$wikiLowerChars ) )",
				$string );
		}
	}
}
