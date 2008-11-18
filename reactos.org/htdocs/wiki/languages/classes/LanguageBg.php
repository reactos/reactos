<?php

/** Bulgarian (Български)
 *
 * @ingroup Language
 */
class LanguageBg extends Language {
	/**
	 * ISO number formatting: 123 456 789,99.
	 * Avoid tripple grouping by numbers with whole part up to 4 digits.
	 */
	function commafy($_) {
		if (!preg_match('/^\d{1,4}$/',$_)) {
			return strrev((string)preg_replace('/(\d{3})(?=\d)(?!\d*\.)/','$1,',strrev($_)));
		} else {
			return $_;
		}
	}
}
