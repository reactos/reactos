<?php

/**
 * Get translated magic words, if available
 *
 * @param string $lang Language code
 * @return array
 */
function efParserFunctionsWords( $lang ) {
	$words = array();

	/**
	 * English
	 */
	$words['en'] = array(
		'expr'       => array( 0, 'expr' ),
		'if'         => array( 0, 'if' ),
		'ifeq'       => array( 0, 'ifeq' ),
		'ifexpr'     => array( 0, 'ifexpr' ),
		'iferror'    => array( 0, 'iferror' ),
		'switch'     => array( 0, 'switch' ),
		'default'    => array( 0, '#default' ),
		'ifexist'    => array( 0, 'ifexist' ),
		'time'       => array( 0, 'time' ),
		'timel'      => array( 0, 'timel' ),
		'rel2abs'    => array( 0, 'rel2abs' ),
		'titleparts' => array( 0, 'titleparts' ),
	);

	/**
	 * Farsi-Persian
	 */
	$words['fa'] = array(
		'expr' 		 => array( 0, 'حساب',         'expr' ),
		'if' 		 => array( 0, 'اگر',          'if' ),
		'ifeq' 		 => array( 0, 'اگرمساوی',     'ifeq' ),
		'ifexpr' 	 => array( 0, 'اگرحساب',      'ifexpr' ),
		'iferror'    => array( 0, 'اگرخطا',       'iferror' ),
		'switch' 	 => array( 0, 'گزینه',        'switch' ),
		'default' 	 => array( 0, '#پیش‌فرض',      '#default' ),
		'ifexist' 	 => array( 0, 'اگرموجود',     'ifexist' ),
		'time' 		 => array( 0, 'زمان',         'time' ),
		'timel'      => array( 0, 'زمان‌بلند',     'timel' ),
		'rel2abs' 	 => array( 0, 'نسبی‌به‌مطلق',   'rel2abs' ),
		'titleparts' => array( 0, 'پاره‌عنوان',    'titleparts' ),
	);

	/**
	 * Hebrew
	 */
	$words['he'] = array(
		'expr'       => array( 0, 'חשב',         'expr' ),
		'if'         => array( 0, 'תנאי',        'if' ),
		'ifeq'       => array( 0, 'שווה',        'ifeq' ),
		'ifexpr'     => array( 0, 'חשב תנאי',    'ifexpr' ),
		'iferror'    => array( 0, 'תנאי שגיאה',  'iferror' ),
		'switch'     => array( 0, 'בחר',         'switch' ),
		'default'    => array( 0, '#ברירת מחדל', '#default' ),
		'ifexist'    => array( 0, 'קיים',        'ifexist' ),
		'time'       => array( 0, 'זמן',         'time' ),
		'timel'      => array( 0, 'זמןמ',        'timel' ),
		'rel2abs'    => array( 0, 'יחסי למוחלט', 'rel2abs' ),
		'titleparts' => array( 0, 'חלק בכותרת',  'titleparts' ),
	);

	/**
	 * Indonesian
	 */
	$words['id'] = array(
		'expr'       => array( 0, 'hitung',       'expr' ),
		'if'         => array( 0, 'jika',         'if' ),
		'ifeq'       => array( 0, 'jikasama',     'ifeq' ),
		'ifexpr'     => array( 0, 'jikahitung',   'ifexpr' ),
		'iferror'    => array( 0, 'jikasalah',   'iferror' ),
		'switch'     => array( 0, 'pilih',        'switch' ),
		'default'    => array( 0, '#baku',        '#default' ),
		'ifexist'    => array( 0, 'jikaada',      'ifexist' ),
		'time'       => array( 0, 'waktu',        'time' ),
		'timel'      => array( 0, 'waktu1',       'timel' ),
		'rel2abs'    => array( 0, 'rel2abs' ),
		'titleparts' => array( 0, 'bagianjudul',  'titleparts' ),
	);

	# English is used as a fallback, and the English synonyms are
	# used if a translation has not been provided for a given word
	return ( $lang == 'en' || !isset( $words[$lang] ) )
		? $words['en']
		: array_merge( $words['en'], $words[$lang] );
}
