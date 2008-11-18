<?php
/** Kazakh (Қазақша)
 *
 * @ingroup Language
 */

class LanguageKk_cyrl extends Language {

	# Convert from the nominative form of a noun to some other case
	# Invoked with {{GRAMMAR:case|word}}
	/**
	 * Cases: genitive, dative, accusative, locative, ablative, comitative + possessive forms
	 */

	function convertGrammarKk_cyrl( $word, $case ) {
		global $wgGrammarForms;
		if ( isset( $wgGrammarForms['kk-kz'][$case][$word] ) ) {
			return $wgGrammarForms['kk-kz'][$case][$word];
		}
		if ( isset( $wgGrammarForms['kk-cyrl'][$case][$word] ) ) {
			return $wgGrammarForms['kk-cyrl'][$case][$word];
		}
		// Set up some constants...
		// Vowels in last syllable
		$frontVowels = array( "е", "ө", "ү", "і", "ә", "э", "я", "ё", "и" );
		$backVowels = array( "а", "о", "ұ", "ы" );
		$allVowels = array( "е", "ө", "ү", "і", "ә", "э", "а", "о", "ұ", "ы", "я", "ё", "и" );
		// Preceding letters
		$Nasals = array( "м", "н", "ң" );
		$Sonants = array( "и", "й", "л", "р", "у", "ю");
		$Consonants = array( "п", "ф", "к", "қ", "т", "ш", "с", "х", "ц", "ч", "щ", "б", "в", "г", "д" );
		$Sibilants = array( "ж", "з" );
		$Sonorants = array( "и", "й", "л", "р", "у", "ю", "м", "н", "ң", "ж", "з");

		// Possessives
		$firstPerson = array( "м", "ң" ); // 1st singular, 2nd unformal
		$secondPerson = array( "з" ); // 1st plural, 2nd formal
		$thirdPerson = array( "ы", "і" ); // 3rd

		$lastLetter = self::lastLetter( $word, $allVowels );
		$wordEnding =& $lastLetter[0];
		$wordLastVowel =& $lastLetter[1];

		// Now convert the word
		switch ( $case ) {
			case "dc1":
			case "genitive": #ilik
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "тің";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "тың";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "нің";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ның";
					}
				} elseif ( in_array( $wordEnding, $Sonants ) || in_array( $wordEnding, $Sibilants )) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "дің";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "дың";
					}
				}
				break;
			case "dc2":
			case "dative": #barıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ке";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "қа";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Sonorants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ге";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ға";
					}
				}
				break;
			case "dc21":
			case "possessive dative": #täweldık + barıs
				if ( in_array( $wordEnding, $firstPerson ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "е";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "а";
					}
				} elseif ( in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ге";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ға";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
				  if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "не";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "на";
					}
				}
				break;
			case "dc3":
			case "accusative": #tabıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ті";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ты";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) ) {
					if ( in_array($wordLastVowel, $frontVowels ) ) {
						$word = $word . "ні";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ны";
					}
				} elseif ( in_array( $wordEnding, $Sonorants) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "ді";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ды";
					}
				}
				break;
			case "dc31":
			case "possessive accusative": #täweldık + tabıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ді";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ды";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
						$word = $word . "н";
				}
				break;
			case "dc4":
			case "locative": #jatıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "те";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "та";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Sonorants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "де";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "да";
					}
				}
				break;
			case "dc41":
			case "possessive locative": #täweldık + jatıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "де";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "да";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "нде";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "нда";
					}
				}
				break;
			case "dc5":
			case "ablative": #şığıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "тен";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "тан";
					}
				} elseif ( in_array($wordEnding, $allVowels ) || in_array($wordEnding, $Sonants ) || in_array($wordEnding, $Sibilants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ден";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "дан";
					}
				}  elseif ( in_array($wordEnding, $Nasals ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "нен";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "нан";
					}
				}
				break;
			case "dc51":
			case "possessive ablative": #täweldık + şığıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $thirdPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "нен";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "нан";
					}
				} elseif ( in_array($wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ден";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "дан";
					}
				}
				break;
			case "dc6":
			case "comitative": #kömektes
				if ( in_array( $wordEnding, $Consonants ) ) {
						$word = $word . "пен";
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) || in_array( $wordEnding, $Sonants ) ) {
						$word = $word . "мен";
				} elseif ( in_array( $wordEnding, $Sibilants ) ) {
						$word = $word . "бен";
				}
				break;
			case "dc61":
			case "possessive comitative": #täweldık + kömektes
				if ( in_array( $wordEnding, $Consonants ) ) {
						$word = $word . "пенен";
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) || in_array( $wordEnding, $Sonants ) ) {
						$word = $word . "менен";
				} elseif ( in_array( $wordEnding, $Sibilants ) ) {
						$word = $word . "бенен";
				}
				break;
			default: #dc0 #nominative #ataw
		}
		return $word;
	}

	function convertGrammarKk_latn( $word, $case ) {
		global $wgGrammarForms;
		if ( isset( $wgGrammarForms['kk-tr'][$case][$word] ) ) {
			return $wgGrammarForms['kk-tr'][$case][$word];
		}
		if ( isset( $wgGrammarForms['kk-latn'][$case][$word] ) ) {
			return $wgGrammarForms['kk-latn'][$case][$word];
		}
		// Set up some constants...
		// Vowels in last syllable
		$frontVowels = array( "e", "ö", "ü", "i", "ä", "é" );
		$backVowels = array( "a", "o", "u", "ı" );
		$allVowels = array( "e", "ö", "ü", "i", "ä", "é", "a", "o", "u", "ı" );
		// Preceding letters
		$Nasals = array( "m", "n", "ñ" );
		$Sonants = array( "ï", "y", "ý", "l", "r", "w");
		$Consonants = array( "p", "f", "k", "q", "t", "ş", "s", "x", "c", "ç", "b", "v", "g", "d" );
		$Sibilants = array( "j", "z" );
		$Sonorants = array( "ï", "y", "ý", "l", "r", "w", "m", "n", "ñ", "j", "z");

		// Possessives
		$firstPerson = array( "m", "ñ" ); // 1st singular, 2nd unformal
		$secondPerson = array( "z" ); // 1st plural, 2nd formal
		$thirdPerson = array( "ı", "i" ); // 3rd

		$lastLetter = self::lastLetter( $word, $allVowels );
		$wordEnding =& $lastLetter[0];
		$wordLastVowel =& $lastLetter[1];

		// Now convert the word
		switch ( $case ) {
			case "dc1":
			case "genitive": #ilik
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "tiñ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "tıñ";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "niñ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "nıñ";
					}
				} elseif ( in_array( $wordEnding, $Sonants ) || in_array( $wordEnding, $Sibilants )) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "diñ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "dıñ";
					}
				}
				break;
			case "dc2":
			case "dative": #barıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ke";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "qa";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Sonorants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ge";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ğa";
					}
				}
				break;
			case "dc21":
			case "possessive dative": #täweldık + barıs
				if ( in_array( $wordEnding, $firstPerson ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "e";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "a";
					}
				} elseif ( in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ge";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ğa";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
				  if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ne";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "na";
					}
				}
				break;
			case "dc3":
			case "accusative": #tabıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ti";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "tı";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) ) {
					if ( in_array($wordLastVowel, $frontVowels ) ) {
						$word = $word . "ni";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "nı";
					}
				} elseif ( in_array( $wordEnding, $Sonorants) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "di";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "dı";
					}
				}
				break;
			case "dc31":
			case "possessive accusative": #täweldık + tabıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "di";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "dı";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
						$word = $word . "n";
				}
				break;
			case "dc4":
			case "locative": #jatıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "te";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ta";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Sonorants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "de";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "da";
					}
				}
				break;
			case "dc41":
			case "possessive locative": #täweldık + jatıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "de";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "da";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "nde";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "nda";
					}
				}
				break;
			case "dc5":
			case "ablative": #şığıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ten";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "tan";
					}
				} elseif ( in_array($wordEnding, $allVowels ) || in_array($wordEnding, $Sonants ) || in_array($wordEnding, $Sibilants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "den";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "dan";
					}
				}  elseif ( in_array($wordEnding, $Nasals ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "nen";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "nan";
					}
				}
				break;
			case "dc51":
			case "possessive ablative": #täweldık + şığıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $thirdPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "nen";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "nan";
					}
				} elseif ( in_array($wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "den";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "dan";
					}
				}
				break;
			case "dc6":
			case "comitative": #kömektes
				if ( in_array( $wordEnding, $Consonants ) ) {
						$word = $word . "pen";
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) || in_array( $wordEnding, $Sonants ) ) {
						$word = $word . "men";
				} elseif ( in_array( $wordEnding, $Sibilants ) ) {
						$word = $word . "ben";
				}
				break;
			case "dc61":
			case "possessive comitative": #täweldık + kömektes
				if ( in_array( $wordEnding, $Consonants ) ) {
						$word = $word . "penen";
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) || in_array( $wordEnding, $Sonants ) ) {
						$word = $word . "menen";
				} elseif ( in_array( $wordEnding, $Sibilants ) ) {
						$word = $word . "benen";
				}
				break;
			default: #dc0 #nominative #ataw
		}
		return $word;
	}

	function convertGrammarKk_arab( $word, $case ) {
		global $wgGrammarForms;
		if ( isset( $wgGrammarForms['kk-cn'][$case][$word] ) ) {
			return $wgGrammarForms['kk-cn'][$case][$word];
		}
		if ( isset( $wgGrammarForms['kk-arab'][$case][$word] ) ) {
			return $wgGrammarForms['kk-arab'][$case][$word];
		}
		// Set up some constants...
		// Vowels in last syllable
		$frontVowels = array( "ە", "ٶ", "ٷ", "ٸ", "ٵ", "ە" );
		$backVowels = array( "ا", "و", "ۇ", "ى"  );
		$allVowels = array( "ە", "ٶ", "ٷ", "ٸ", "ٵ", "ە", "ا", "و", "ۇ", "ى" );
		// Preceding letters
		$Nasals = array( "م", "ن", "ڭ" );
		$Sonants = array( "ي", "ي", "ل", "ر", "ۋ");
		$Consonants = array( "پ", "ف", "ك", "ق", "ت", "ش", "س", "ح", "تس", "چ", "ب", "ۆ", "گ", "د" );
		$Sibilants = array( "ج", "ز" );
		$Sonorants = array( "ي", "ي", "ل", "ر", "ۋ", "م", "ن", "ڭ", "ج", "ز");

		// Possessives
		$firstPerson = array( "م", "ڭ" ); // 1st singular, 2nd unformal
		$secondPerson = array( "ز" ); // 1st plural, 2nd formal
		$thirdPerson = array( "ى", "ٸ" ); // 3rd

		$lastLetter = self::lastLetter( $word, $allVowels );
		$wordEnding =& $lastLetter[0];
		$wordLastVowel =& $lastLetter[1];

		// Now convert the word
		switch ( $case ) {
			case "dc1":
			case "genitive": #ilik
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "تٸڭ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "تىڭ";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "نٸڭ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "نىڭ";
					}
				} elseif ( in_array( $wordEnding, $Sonants ) || in_array( $wordEnding, $Sibilants )) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "دٸڭ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "دىڭ";
					}
				}
				break;
			case "dc2":
			case "dative": #barıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "كە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "قا";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Sonorants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "گە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "عا";
					}
				}
				break;
			case "dc21":
			case "possessive dative": #täweldık + barıs
				if ( in_array( $wordEnding, $firstPerson ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "ە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ا";
					}
				} elseif ( in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "گە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "عا";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
				  if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "نە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "نا";
					}
				}
				break;
			case "dc3":
			case "accusative": #tabıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "تٸ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "تى";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) ) {
					if ( in_array($wordLastVowel, $frontVowels ) ) {
						$word = $word . "نٸ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "نى";
					}
				} elseif ( in_array( $wordEnding, $Sonorants) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "دٸ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "دى";
					}
				}
				break;
			case "dc31":
			case "possessive accusative": #täweldık + tabıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "دٸ";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "دى";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
						$word = $word . "ن";
				}
				break;
			case "dc4":
			case "locative": #jatıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "تە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "تا";
					}
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Sonorants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "دە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "دا";
					}
				}
				break;
			case "dc41":
			case "possessive locative": #täweldık + jatıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "دە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "دا";
					}
				} elseif ( in_array( $wordEnding, $thirdPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels) ) {
						$word = $word . "ندە";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "ندا";
					}
				}
				break;
			case "dc5":
			case "ablative": #şığıs
				if ( in_array( $wordEnding, $Consonants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "تەن";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "تان";
					}
				} elseif ( in_array($wordEnding, $allVowels ) || in_array($wordEnding, $Sonants ) || in_array($wordEnding, $Sibilants ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "دەن";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "دان";
					}
				}  elseif ( in_array($wordEnding, $Nasals ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "نەن";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "نان";
					}
				}
				break;
			case "dc51":
			case "possessive ablative": #täweldık + şığıs
				if ( in_array( $wordEnding, $firstPerson ) || in_array( $wordEnding, $thirdPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "نەن";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "نان";
					}
				} elseif ( in_array($wordEnding, $secondPerson  ) ) {
					if ( in_array( $wordLastVowel, $frontVowels ) ) {
						$word = $word . "دەن";
					} elseif ( in_array( $wordLastVowel, $backVowels ) ) {
						$word = $word . "دان";
					}
				}
				break;
			case "dc6":
			case "comitative": #kömektes
				if ( in_array( $wordEnding, $Consonants ) ) {
						$word = $word . "پەن";
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) || in_array( $wordEnding, $Sonants ) ) {
						$word = $word . "مەن";
				} elseif ( in_array( $wordEnding, $Sibilants ) ) {
						$word = $word . "بەن";
				}
				break;
			case "dc61":
			case "possessive comitative": #täweldık + kömektes
				if ( in_array( $wordEnding, $Consonants ) ) {
						$word = $word . "پەنەن";
				} elseif ( in_array( $wordEnding, $allVowels ) || in_array( $wordEnding, $Nasals ) || in_array( $wordEnding, $Sonants ) ) {
						$word = $word . "مەنەن";
				} elseif ( in_array( $wordEnding, $Sibilants ) ) {
						$word = $word . "بەنەن";
				}
				break;
			default: #dc0 #nominative #ataw
		}
		return $word;
	}

	function lastLetter( $word, $allVowels ) {
		$lastLetter = array();
		$ar = array();

		// Put the word in a form we can play with since we're using UTF-8
		$ar = preg_split('//u', parent::lc($word), -1, PREG_SPLIT_NO_EMPTY);

		// Here's an array with the order of the letters in the word reversed
		// so we can find a match quicker *shrug*
		$wordReversed = array_reverse( $ar );

		// Here's the last letter in the word
		$lastLetter[0] = $ar[count( $ar ) - 1];

		// Find the last vowel in the word
		$lastLetter[1] = NULL;
		foreach ( $wordReversed as $xvalue ) {
			foreach ( $allVowels as $yvalue ) {
				if ( strcmp( $xvalue, $yvalue ) == 0 ) {
					$lastLetter[1] = $xvalue;
					break;
				} else {
					continue;
				}
			}
			if ( $lastLetter[1] !== NULL ) {
				break;
			} else {
				continue;
			}
		}

		return $lastLetter;
	}

	/**
	 * Avoid grouping whole numbers between 0 to 9999
	 */
	function commafy( $_ ) {
		if ( !preg_match( '/^\d{1,4}$/', $_ ) ) {
			return strrev( (string)preg_replace( '/(\d{3})(?=\d)(?!\d*\.)/', '$1,', strrev($_) ) );
		} else {
			return $_;
		}
	}
}
