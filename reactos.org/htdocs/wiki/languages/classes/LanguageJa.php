<?php

/**
 * Japanese (日本語)
 *
 * @ingroup Language
 */
class LanguageJa extends Language {
	function stripForSearch( $string ) {
		# MySQL fulltext index doesn't grok utf-8, so we
		# need to fold cases and convert to hex
		$s = $string;

		# Strip known punctuation ?
		#$s = preg_replace( '/\xe3\x80[\x80-\xbf]/', '', $s ); # U3000-303f

		# Space strings of like hiragana/katakana/kanji
		$hiragana = '(?:\xe3(?:\x81[\x80-\xbf]|\x82[\x80-\x9f]))'; # U3040-309f
		$katakana = '(?:\xe3(?:\x82[\xa0-\xbf]|\x83[\x80-\xbf]))'; # U30a0-30ff
		$kanji = '(?:\xe3[\x88-\xbf][\x80-\xbf]'
			. '|[\xe4-\xe8][\x80-\xbf]{2}'
			. '|\xe9[\x80-\xa5][\x80-\xbf]'
			. '|\xe9\xa6[\x80-\x99])';
			# U3200-9999 = \xe3\x88\x80-\xe9\xa6\x99
		$s = preg_replace( "/({$hiragana}+|{$katakana}+|{$kanji}+)/", ' $1 ', $s );

		# Double-width roman characters: ff00-ff5f ~= 0020-007f
		$s = preg_replace( '/\xef\xbc([\x80-\xbf])/e', 'chr((ord("$1") & 0x3f) + 0x20)', $s );
		$s = preg_replace( '/\xef\xbd([\x80-\x99])/e', 'chr((ord("$1") & 0x3f) + 0x60)', $s );

		# Do general case folding and UTF-8 armoring
		return parent::stripForSearch( $s );
	}

	# Italic is not appropriate for Japanese script
	# Unfortunately most browsers do not recognise this, and render <em> as italic
	function emphasize( $text ) {
		return $text;
	}
}
