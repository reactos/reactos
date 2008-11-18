<?php
/** Khmer (ភាសាខ្មែរ)
 *
 * @ingroup Language
 *
 * @author Niklas Laxström
 */
class LanguageKm extends Language {
	function commafy($_) {
		/* NO-op for Khmer. Cannot use
		 * $separatorTransformTable = array( ',' => '' )
		 * That would break when parsing and doing strstr '' => 'foo';
		 */
		return $_;
	}

}
