<?php

require_once( dirname(__FILE__).'/../LanguageConverter.php' );
require_once( dirname(__FILE__).'/LanguageSr_ec.php' );
require_once( dirname(__FILE__).'/LanguageSr_el.php' );

/**
 * There are two levels of conversion for Serbian: the script level
 * (Cyrillics <-> Latin), and the variant level (ekavian
 * <->iyekavian). The two are orthogonal. So we really only need two
 * dictionaries: one for Cyrillics and Latin, and one for ekavian and
 * iyekavian.
 *
 * @ingroup Language
 */
class SrConverter extends LanguageConverter {
	var $mToLatin = array(
		'а' => 'a', 'б' => 'b',  'в' => 'v', 'г' => 'g',  'д' => 'd',
		'ђ' => 'đ', 'е' => 'e',  'ж' => 'ž', 'з' => 'z',  'и' => 'i',
		'ј' => 'j', 'к' => 'k',  'л' => 'l', 'љ' => 'lj', 'м' => 'm',
		'н' => 'n', 'њ' => 'nj', 'о' => 'o', 'п' => 'p',  'р' => 'r',
		'с' => 's', 'т' => 't',  'ћ' => 'ć', 'у' => 'u',  'ф' => 'f',
		'х' => 'h', 'ц' => 'c',  'ч' => 'č', 'џ' => 'dž', 'ш' => 'š',

		'А' => 'A', 'Б' => 'B',  'В' => 'V', 'Г' => 'G',  'Д' => 'D',
		'Ђ' => 'Đ', 'Е' => 'E',  'Ж' => 'Ž', 'З' => 'Z',  'И' => 'I',
		'Ј' => 'J', 'К' => 'K',  'Л' => 'L', 'Љ' => 'Lj', 'М' => 'M',
		'Н' => 'N', 'Њ' => 'Nj', 'О' => 'O', 'П' => 'P',  'Р' => 'R',
		'С' => 'S', 'Т' => 'T',  'Ћ' => 'Ć', 'У' => 'U',  'Ф' => 'F',
		'Х' => 'H', 'Ц' => 'C',  'Ч' => 'Č', 'Џ' => 'Dž', 'Ш' => 'Š',
	);

	var $mToCyrillics = array(
		'a' => 'а', 'b'  => 'б', 'c' => 'ц', 'č' => 'ч', 'ć'  => 'ћ',
		'd' => 'д', 'dž' => 'џ', 'đ' => 'ђ', 'e' => 'е', 'f'  => 'ф',
		'g' => 'г', 'h'  => 'х', 'i' => 'и', 'j' => 'ј', 'k'  => 'к',
		'l' => 'л', 'lj' => 'љ', 'm' => 'м', 'n' => 'н', 'nj' => 'њ',
		'o' => 'о', 'p'  => 'п', 'r' => 'р', 's' => 'с', 'š'  => 'ш',
		't' => 'т', 'u'  => 'у', 'v' => 'в', 'z' => 'з', 'ž'  => 'ж',

		'A' => 'А', 'B'  => 'Б', 'C' => 'Ц', 'Č' => 'Ч', 'Ć'  => 'Ћ',
		'D' => 'Д', 'Dž' => 'Џ', 'Đ' => 'Ђ', 'E' => 'Е', 'F'  => 'Ф',
		'G' => 'Г', 'H'  => 'Х', 'I' => 'И', 'J' => 'Ј', 'K'  => 'К',
		'L' => 'Л', 'LJ' => 'Љ', 'M' => 'М', 'N' => 'Н', 'NJ' => 'Њ',
		'O' => 'О', 'P'  => 'П', 'R' => 'Р', 'S' => 'С', 'Š'  => 'Ш',
		'T' => 'Т', 'U'  => 'У', 'V' => 'В', 'Z' => 'З', 'Ž'  => 'Ж',

		'DŽ' => 'Џ', 'd!ž' => 'дж', 'D!ž'=> 'Дж', 'D!Ž'=> 'ДЖ',
		'Lj' => 'Љ', 'l!j' => 'лј', 'L!j'=> 'Лј', 'L!J'=> 'ЛЈ',
		'Nj' => 'Њ', 'n!j' => 'нј', 'N!j'=> 'Нј', 'N!J'=> 'НЈ'
	);

	function loadDefaultTables() {
		$this->mTables = array(
			'sr-ec' => new ReplacementArray( $this->mToCyrillics ),
			'sr-el' => new ReplacementArray( $this->mToLatin),
			'sr'    => new ReplacementArray()
		);
	}

	/* rules should be defined as -{ekavian | iyekavian-} -or-
		-{code:text | code:text | ...}-
		update: delete all rule parsing because it's not used
		        currently, and just produces a couple of bugs
	*/
	function parseManualRule($rule, $flags=array()) {
		if(in_array('T',$flags)){
			return parent::parseManualRule($rule, $flags);
		}

		// otherwise ignore all formatting
		foreach($this->mVariants as $v) {
			$carray[$v] = $rule;
		}

		return $carray;
	}

	// Do not convert content on talk pages
	function parserConvert( $text, &$parser ){
		if(is_object($parser->getTitle() ) && $parser->getTitle()->isTalkPage())
			$this->mDoContentConvert=false;
		else
			$this->mDoContentConvert=true;

		return parent::parserConvert($text, $parser );
	}

	/*
	 * A function wrapper:
	 *   - if there is no selected variant, leave the link
	 *     names as they were
	 *   - do not try to find variants for usernames
	 */
	function findVariantLink( &$link, &$nt ) {
		// check for user namespace
		if(is_object($nt)){
			$ns = $nt->getNamespace();
			if($ns==NS_USER || $ns==NS_USER_TALK)
				return;
		}

		$oldlink=$link;
		parent::findVariantLink($link,$nt);
		if($this->getPreferredVariant()==$this->mMainLanguageCode)
			$link=$oldlink;
	}

	/*
	 * We want our external link captions to be converted in variants,
	 * so we return the original text instead -{$text}-, except for URLs
	 */
	function markNoConversion($text, $noParse=false) {
		if($noParse || preg_match("/^https?:\/\/|ftp:\/\/|irc:\/\//",$text))
			return parent::markNoConversion($text);
		return $text;
	}

	/*
	 * An ugly function wrapper for parsing Image titles
	 * (to prevent image name conversion)
	 */
	function autoConvert($text, $toVariant=false) {
		global $wgTitle;
		if(is_object($wgTitle) && $wgTitle->getNameSpace()==NS_IMAGE){
			$imagename = $wgTitle->getNsText();
			if(preg_match("/^$imagename:/",$text)) return $text;
		}
		return parent::autoConvert($text,$toVariant);
	}

	/**
	 *  It translates text into variant, specials:
	 *    - ommiting roman numbers
	 */
	function translate($text, $toVariant){
		$breaks = '[^\w\x80-\xff]';

		// regexp for roman numbers
		$roman = 'M{0,4}(CM|CD|D?C{0,3})(XC|XL|L?X{0,3})(IX|IV|V?I{0,3})';

		$reg = '/^'.$roman.'$|^'.$roman.$breaks.'|'.$breaks.$roman.'$|'.$breaks.$roman.$breaks.'/';

		$matches = preg_split($reg, $text, -1, PREG_SPLIT_OFFSET_CAPTURE);

		$m = array_shift($matches);
		if( !isset( $this->mTables[$toVariant] ) ) {
			throw new MWException( "Broken variant table: " . implode( ',', array_keys( $this->mTables ) ) );
		}
		$ret = $this->mTables[$toVariant]->replace( $m[0] );
		$mstart = $m[1]+strlen($m[0]);
		foreach($matches as $m) {
			$ret .= substr($text, $mstart, $m[1]-$mstart);
			$ret .= parent::translate($m[0], $toVariant);
			$mstart = $m[1] + strlen($m[0]);
		}

		return $ret;
	}
}

/**
 * @ingroup Language
 */
class LanguageSr extends LanguageSr_ec {
	function __construct() {
		global $wgHooks;

		parent::__construct();

		$variants = array('sr', 'sr-ec', 'sr-el');
		$variantfallbacks = array(
			'sr'    => 'sr-ec',
			'sr-ec' => 'sr',
			'sr-el' => 'sr',
		);

		$marker = array();//don't mess with these, leave them as they are
		$flags = array(
			'S' => 'S', 'писмо' => 'S', 'pismo' => 'S',
			'W' => 'W', 'реч'   => 'W', 'reč'   => 'W', 'ријеч' => 'W', 'riječ' => 'W'
		);
		$this->mConverter = new SrConverter($this, 'sr', $variants, $variantfallbacks, $marker, $flags);
		$wgHooks['ArticleSaveComplete'][] = $this->mConverter;
	}
}
