<?php

/** Latvian (Latviešu)
 *
 * @ingroup Language
 *
 * @author Niklas Laxström
 *
 * @copyright Copyright © 2006, Niklas Laxström
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */
class LanguageLv extends Language {
	/**
	 * Plural form transformations. Using the first form for words with the last digit 1, but not for words with the last digits 11, and the second form for all the others.
	 *
	 * Example: {{plural:{{NUMBEROFARTICLES}}|article|articles}}
	 *
	 * @param integer $count
	 * @param string $wordform1
	 * @param string $wordform2
	 * @param string $wordform3 (not used)
	 * @return string
	 */
	function convertPlural( $count, $forms ) {
		if ( !count($forms) ) { return ''; }
		$forms = $this->preConvertPlural( $forms, 2 );

		return ( ( $count % 10 == 1 ) && ( $count % 100 != 11 ) ) ? $forms[0] : $forms[1];
	}

	# Convert from the nominative form of a noun to some other case
	# Invoked with {{GRAMMAR:case|word}}
	# ģenitīvs - kā, datīvs - kam, akuzatīvs - ko, lokatīvs - kur.
	/**
	 * Cases: ģenitīvs, datīvs, akuzatīvs, lokatīvs
	 */
	function convertGrammar( $word, $case ) {
		global $wgGrammarForms;

		$wgGrammarForms['lv']['ģenitīvs' ]['Vikipēdija']   = 'Vikipēdijas';
		$wgGrammarForms['lv']['ģenitīvs' ]['Vikivārdnīca'] = 'Vikivārdnīcas';
		$wgGrammarForms['lv']['datīvs'   ]['Vikipēdija']   = 'Vikipēdijai';
		$wgGrammarForms['lv']['datīvs'   ]['Vikivārdnīca'] = 'Vikivārdnīcai';
		$wgGrammarForms['lv']['akuzatīvs']['Vikipēdija']   = 'Vikipēdiju';
		$wgGrammarForms['lv']['akuzatīvs']['Vikivārdnīca'] = 'Vikivārdnīcu';
		$wgGrammarForms['lv']['lokatīvs' ]['Vikipēdija']   = 'Vikipēdijā';
		$wgGrammarForms['lv']['lokatīvs' ]['Vikivārdnīca'] = 'Vikivārdnīcā';

		if ( isset($wgGrammarForms['lv'][$case][$word]) ) {
			return $wgGrammarForms['lv'][$case][$word];
		}

		return $word;
	}
}


