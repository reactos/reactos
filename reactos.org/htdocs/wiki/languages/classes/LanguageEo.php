<?php

/** Esperanto (Esperanto)
 *
 * @ingroup Language
 */
class LanguageEo extends Language {
	function iconv( $in, $out, $string ) {
		# For most languages, this is a wrapper for iconv
		# Por multaj lingvoj, ĉi tiu nur voku la sisteman funkcion iconv()
		# Ni ankaŭ konvertu X-sistemajn surogotajn
		if( strcasecmp( $in, 'x' ) == 0 and strcasecmp( $out, 'utf-8' ) == 0) {
			$xu = array (
				'xx' => 'x' , 'xX' => 'x' ,
				'Xx' => 'X' , 'XX' => 'X' ,
				"Cx" => "\xc4\x88" , "CX" => "\xc4\x88" ,
				"cx" => "\xc4\x89" , "cX" => "\xc4\x89" ,
				"Gx" => "\xc4\x9c" , "GX" => "\xc4\x9c" ,
				"gx" => "\xc4\x9d" , "gX" => "\xc4\x9d" ,
				"Hx" => "\xc4\xa4" , "HX" => "\xc4\xa4" ,
				"hx" => "\xc4\xa5" , "hX" => "\xc4\xa5" ,
				"Jx" => "\xc4\xb4" , "JX" => "\xc4\xb4" ,
				"jx" => "\xc4\xb5" , "jX" => "\xc4\xb5" ,
				"Sx" => "\xc5\x9c" , "SX" => "\xc5\x9c" ,
				"sx" => "\xc5\x9d" , "sX" => "\xc5\x9d" ,
				"Ux" => "\xc5\xac" , "UX" => "\xc5\xac" ,
				"ux" => "\xc5\xad" , "uX" => "\xc5\xad"
				) ;
			return preg_replace ( '/([cghjsu]x?)((?:xx)*)(?!x)/ei',
			  'strtr( "$1", $xu ) . strtr( "$2", $xu )', $string );
		} else if( strcasecmp( $in, 'UTF-8' ) == 0 and strcasecmp( $out, 'x' ) == 0 ) {
			$ux = array (
				'x' => 'xx' , 'X' => 'Xx' ,
				"\xc4\x88" => "Cx" , "\xc4\x89" => "cx" ,
				"\xc4\x9c" => "Gx" , "\xc4\x9d" => "gx" ,
				"\xc4\xa4" => "Hx" , "\xc4\xa5" => "hx" ,
				"\xc4\xb4" => "Jx" , "\xc4\xb5" => "jx" ,
				"\xc5\x9c" => "Sx" , "\xc5\x9d" => "sx" ,
				"\xc5\xac" => "Ux" , "\xc5\xad" => "ux"
			) ;
			# Double Xs only if they follow cxapelutaj literoj.
			return preg_replace( '/((?:[cghjsu]|\xc4[\x88\x89\x9c\x9d\xa4\xa5\xb4\xb5]'.
			  '|\xc5[\x9c\x9d\xac\xad])x*)/ei', 'strtr( "$1", $ux )', $string );
		}
		return iconv( $in, $out, $string );
	}

	function checkTitleEncoding( $s ) {
		# Check for X-system backwards-compatibility URLs
		$ishigh = preg_match( '/[\x80-\xff]/', $s);
		$isutf = preg_match( '/^([\x00-\x7f]|[\xc0-\xdf][\x80-\xbf]|' .
			'[\xe0-\xef][\x80-\xbf]{2}|[\xf0-\xf7][\x80-\xbf]{3})+$/', $s );

		if($ishigh and !$isutf) {
			# Assume Latin1
			$s = utf8_encode( $s );
		} else {
			if( preg_match( '/(\xc4[\x88\x89\x9c\x9d\xa4\xa5\xb4\xb5]'.
				'|\xc5[\x9c\x9d\xac\xad])/', $s ) )
			return $s;
		}

		//if( preg_match( '/[cghjsu]x/i', $s ) )
		//	return $this->iconv( 'x', 'utf-8', $s );
		return $s;
	}

	function initEncoding() {
		global $wgEditEncoding;
		$wgEditEncoding = 'x';
	}
}
