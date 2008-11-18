<?php

require_once( dirname(__FILE__).'/../LanguageConverter.php' );

/**
 * Converts Tajiki to latin orthography
 * @ingroup Language
 */
class TgConverter extends LanguageConverter {
	private $table = array(
		'а' => 'a',
		'б' => 'b',
		'в' => 'v',
		'г' => 'g',
		'д' => 'd',
		'е' => 'e',
		'ё' => 'jo',
		'ж' => 'ƶ',
		'з' => 'z',
		'ии ' => 'iji ',
		'и' => 'i',
		'й' => 'j',
		'к' => 'k',
		'л' => 'l',
		'м' => 'm',
		'н' => 'n',
		'о' => 'o',
		'п' => 'p',
		'р' => 'r',
		'с' => 's',
		'т' => 't',
		'у' => 'u',
		'ф' => 'f',
		'х' => 'x',
		'ч' => 'c',
		'ш' => 'ş',
		'ъ' => '\'',
		'э' => 'e',
		'ю' => 'ju',
		'я' => 'ja',
		'ғ' => 'ƣ',
		'ӣ' => 'ī',
		'қ' => 'q',
		'ӯ' => 'ū',
		'ҳ' => 'h',
		'ҷ' => 'ç',
		'ц' => 'ts',
		'А' => 'A',
		'Б' => 'B',
		'В' => 'V',
		'Г' => 'G',
		'Д' => 'D',
		'Е' => 'E',
		'Ё' => 'Jo',
		'Ж' => 'Ƶ',
		'З' => 'Z',
		'И' => 'I',
		'Й' => 'J',
		'К' => 'K',
		'Л' => 'L',
		'М' => 'M',
		'Н' => 'N',
		'О' => 'O',
		'П' => 'P',
		'Р' => 'R',
		'С' => 'S',
		'Т' => 'T',
		'У' => 'U',
		'Ф' => 'F',
		'Х' => 'X',
		'Ч' => 'C',
		'Ш' => 'Ş',
		'Ъ' => '\'',
		'Э' => 'E',
		'Ю' => 'Ju',
		'Я' => 'Ja',
		'Ғ' => 'Ƣ',
		'Ӣ' => 'Ī',
		'Қ' => 'Q',
		'Ӯ' => 'Ū',
		'Ҳ' => 'H',
		'Ҷ' => 'Ç',
		'Ц' => 'Ts',
	);

	function loadDefaultTables() {
		$this->mTables = array(
			'tg-latn' => new ReplacementArray( $this->table ),
			'tg'      => new ReplacementArray()
		);
	}

}

/**
 * @ingroup Language
 */
class LanguageTg extends Language {
	function __construct() {
		parent::__construct();
		$variants = array( 'tg', 'tg-latn' );
		$this->mConverter = new TgConverter( $this, 'tg', $variants );
	}
}
