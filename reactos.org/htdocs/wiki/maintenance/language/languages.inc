<?php
/**
 * Handle messages in the language files.
 *
 * @file
 * @ingroup MaintenanceLanguage
 */

/**
 * @ingroup MaintenanceLanguage
 */
class languages {
	protected $mLanguages; # List of languages
	protected $mRawMessages; # Raw list of the messages in each language
	protected $mMessages; # Messages in each language (except for English), divided to groups
	protected $mGeneralMessages; # General messages in English, divided to groups
	protected $mIgnoredMessages; # All the messages which should be exist only in the English file
	protected $mOptionalMessages; # All the messages which may be translated or not, depending on the language

	/**
	 * Load the list of languages: all the Messages*.php
	 * files in the languages directory.
	 *
	 * @param $exif Treat the EXIF messages?
	 */
	function __construct( $exif = true ) {
		require( dirname(__FILE__) . '/messageTypes.inc' );
		$this->mIgnoredMessages = $wgIgnoredMessages;
		if ( $exif ) {
			$this->mOptionalMessages = array_merge( $wgOptionalMessages );
		} else {
			$this->mOptionalMessages = array_merge( $wgOptionalMessages, $wgEXIFMessages );
		}

		$this->mLanguages = array_keys( Language::getLanguageNames( true ) );
		sort( $this->mLanguages );
	}

	/**
	 * Get the language list.
	 *
	 * @return The language list.
	 */
	public function getLanguages() {
		return $this->mLanguages;
	}

	/**
	 * Get the ignored messages list.
	 *
	 * @return The ignored messages list.
	 */
	public function getIgnoredMessages() {
		return $this->mIgnoredMessages;
	}

	/**
	 * Get the optional messages list.
	 *
	 * @return The optional messages list.
	 */
	public function getOptionalMessages() {
		return $this->mOptionalMessages;
	}

	/**
	 * Load the raw messages for a specific language from the messages file.
	 *
	 * @param $code The language code.
	 */
	protected function loadRawMessages( $code ) {
		if ( isset( $this->mRawMessages[$code] ) ) {
			return;
		}
		$filename = Language::getMessagesFileName( $code );
		if ( file_exists( $filename ) ) {
			require( $filename );
			if ( isset( $messages ) ) {
				$this->mRawMessages[$code] = $messages;
			} else {
				$this->mRawMessages[$code] = array();
			}
		} else {
			$this->mRawMessages[$code] = array();
		}
	}

	/**
	 * Load the messages for a specific language (which is not English) and divide them to groups:
	 * all - all the messages.
	 * required - messages which should be translated in order to get a complete translation.
	 * optional - messages which can be translated, the fallback translation is used if not translated.
	 * obsolete - messages which should not be translated, either because they are not exist, or they are ignored messages.
	 * translated - messages which are either required or optional, but translated from English and needed.
	 *
	 * @param $code The language code.
	 */
	private function loadMessages( $code ) {
		if ( isset( $this->mMessages[$code] ) ) {
			return;
		}
		$this->loadRawMessages( $code );
		$this->loadGeneralMessages();
		$this->mMessages[$code]['all'] = $this->mRawMessages[$code];
		$this->mMessages[$code]['required'] = array();
		$this->mMessages[$code]['optional'] = array();
		$this->mMessages[$code]['obsolete'] = array();
		$this->mMessages[$code]['translated'] = array();
		foreach ( $this->mMessages[$code]['all'] as $key => $value ) {
			if ( isset( $this->mGeneralMessages['required'][$key] ) ) {
				$this->mMessages[$code]['required'][$key] = $value;
				$this->mMessages[$code]['translated'][$key] = $value;
			} else if ( isset( $this->mGeneralMessages['optional'][$key] ) ) {
				$this->mMessages[$code]['optional'][$key] = $value;
				$this->mMessages[$code]['translated'][$key] = $value;
			} else {
				$this->mMessages[$code]['obsolete'][$key] = $value;
			}
		}
	}

	/**
	 * Load the messages for English and divide them to groups:
	 * all - all the messages.
	 * required - messages which should be translated to other languages in order to get a complete translation.
	 * optional - messages which can be translated to other languages, but it's not required for a complete translation.
	 * ignored - messages which should not be translated to other languages.
	 * translatable - messages which are either required or optional, but can be translated from English.
	 */
	private function loadGeneralMessages() {
		if ( isset( $this->mGeneralMessages ) ) {
			return;
		}
		$this->loadRawMessages( 'en' );
		$this->mGeneralMessages['all'] = $this->mRawMessages['en'];
		$this->mGeneralMessages['required'] = array();
		$this->mGeneralMessages['optional'] = array();
		$this->mGeneralMessages['ignored'] = array();
		$this->mGeneralMessages['translatable'] = array();
		foreach ( $this->mGeneralMessages['all'] as $key => $value ) {
			if ( in_array( $key, $this->mIgnoredMessages ) ) {
				$this->mGeneralMessages['ignored'][$key] = $value;
			} else if ( in_array( $key, $this->mOptionalMessages ) ) {
				$this->mGeneralMessages['optional'][$key] = $value;
				$this->mGeneralMessages['translatable'][$key] = $value;
			} else {
				$this->mGeneralMessages['required'][$key] = $value;
				$this->mGeneralMessages['translatable'][$key] = $value;
			}
		}
	}

	/**
	 * Get all the messages for a specific language (not English), without the
	 * fallback language messages, divided to groups:
	 * all - all the messages.
	 * required - messages which should be translated in order to get a complete translation.
	 * optional - messages which can be translated, the fallback translation is used if not translated.
	 * obsolete - messages which should not be translated, either because they are not exist, or they are ignored messages.
	 * translated - messages which are either required or optional, but translated from English and needed.
	 *
	 * @param $code The language code.
	 *
	 * @return The messages in this language.
	 */
	public function getMessages( $code ) {
		$this->loadMessages( $code );
		return $this->mMessages[$code];
	}

	/**
	 * Get all the general English messages, divided to groups:
	 * all - all the messages.
	 * required - messages which should be translated to other languages in order to get a complete translation.
	 * optional - messages which can be translated to other languages, but it's not required for a complete translation.
	 * ignored - messages which should not be translated to other languages.
	 * translatable - messages which are either required or optional, but can be translated from English.
	 *
	 * @return The general English messages.
	 */
	public function getGeneralMessages() {
		$this->loadGeneralMessages();
		return $this->mGeneralMessages;
	}

	/**
	 * Get the untranslated messages for a specific language.
	 *
	 * @param $code The language code.
	 *
	 * @return The untranslated messages for this language.
	 */
	public function getUntranslatedMessages( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$requiredGeneralMessages = array_keys( $this->mGeneralMessages['required'] );
		$requiredMessages = array_keys( $this->mMessages[$code]['required'] );
		$untranslatedMessages = array();
		foreach ( array_diff( $requiredGeneralMessages, $requiredMessages ) as $key ) {
			$untranslatedMessages[$key] = $this->mGeneralMessages['required'][$key];
		}
		return $untranslatedMessages;
	}

	/**
	 * Get the duplicate messages for a specific language.
	 *
	 * @param $code The language code.
	 *
	 * @return The duplicate messages for this language.
	 */
	public function getDuplicateMessages( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$duplicateMessages = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {
			if ( $this->mGeneralMessages['translatable'][$key] == $value ) {
				$duplicateMessages[$key] = $value;
			}
		}
		return $duplicateMessages;
	}

	public function getObsoleteMessages( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		return $this->mMessages[$code]['obsolete'];
	}

	/**
	 * Get the messages which do not use some variables.
	 *
	 * @param $code The language code.
	 *
	 * @return The messages which do not use some variables in this language.
	 */
	public function getMessagesWithoutVariables( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$variables = array( '\$1', '\$2', '\$3', '\$4', '\$5', '\$6', '\$7', '\$8', '\$9' );
		$messagesWithoutVariables = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {
			$missing = false;
			foreach ( $variables as $var ) {
				if ( preg_match( "/$var/sU", $this->mGeneralMessages['translatable'][$key] ) &&
					!preg_match( "/$var/sU", $value ) ) {
					$missing = true;
				}
			}
			if ( $missing ) {
				$messagesWithoutVariables[$key] = $value;
			}
		}
		return $messagesWithoutVariables;
	}

	/**
	 * Get the messages which do not use plural.
	 *
	 * @param $code The language code.
	 *
	 * @return The messages which do not use plural in this language.
	 */
	public function getMessagesWithoutPlural( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$messagesWithoutPlural = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {
			if ( stripos( $this->mGeneralMessages['translatable'][$key], '{{plural:' ) !== false && stripos( $value, '{{plural:' ) === false ) {
				$messagesWithoutPlural[$key] = $value;
			}
		}
		return $messagesWithoutPlural;
	}

	/**
	 * Get the empty messages.
	 *
	 * @param $code The language code.
	 *
	 * @return The empty messages for this language.
	 */
	public function getEmptyMessages( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$emptyMessages = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {
			if ( $value === '' || $value === '-' ) {
				$emptyMessages[$key] = $value;
			}
		}
		return $emptyMessages;
	}

	/**
	 * Get the messages with trailing whitespace.
	 *
	 * @param $code The language code.
	 *
	 * @return The messages with trailing whitespace in this language.
	 */
	public function getMessagesWithWhitespace( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$messagesWithWhitespace = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {
			if ( $this->mGeneralMessages['translatable'][$key] !== '' && $value !== rtrim( $value ) ) {
				$messagesWithWhitespace[$key] = $value;
			}
		}
		return $messagesWithWhitespace;
	}

	/**
	 * Get the non-XHTML messages.
	 *
	 * @param $code The language code.
	 *
	 * @return The non-XHTML messages for this language.
	 */
	public function getNonXHTMLMessages( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$wrongPhrases = array(
			'<hr *\\?>',
			'<br *\\?>',
			'<hr/>',
			'<br/>',
			'<hr>',
			'<br>',
		);
		$wrongPhrases = '~(' . implode( '|', $wrongPhrases ) . ')~sDu';
		$nonXHTMLMessages = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {
			if ( preg_match( $wrongPhrases, $value ) ) {
				$nonXHTMLMessages[$key] = $value;
			}
		}
		return $nonXHTMLMessages;
	}

	/**
	 * Get the messages which include wrong characters.
	 *
	 * @param $code The language code.
	 *
	 * @return The messages which include wrong characters in this language.
	 */
	public function getMessagesWithWrongChars( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$wrongChars = array(
			'[LRM]' => "\xE2\x80\x8E",
			'[RLM]' => "\xE2\x80\x8F",
			'[LRE]' => "\xE2\x80\xAA",
			'[RLE]' => "\xE2\x80\xAB",
			'[POP]' => "\xE2\x80\xAC",
			'[LRO]' => "\xE2\x80\xAD",
			'[RLO]' => "\xE2\x80\xAB",
			'[ZWSP]'=> "\xE2\x80\x8B",
			'[NBSP]'=> "\xC2\xA0",
			'[WJ]'  => "\xE2\x81\xA0",
			'[BOM]' => "\xEF\xBB\xBF",
			'[FFFD]'=> "\xEF\xBF\xBD",
		);
		$wrongRegExp = '/(' . implode( '|', array_values( $wrongChars ) ) . ')/sDu';
		$wrongCharsMessages = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {
			if ( preg_match( $wrongRegExp, $value ) ) {
				foreach ( $wrongChars as $viewableChar => $hiddenChar ) {
					$value = str_replace( $hiddenChar, $viewableChar, $value );
				}
				$wrongCharsMessages[$key] = $value;
			}
		}
		return $wrongCharsMessages;
	}

	public function getMessagesWithDubiousLinks( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$tc = Title::legalChars() . '#%{}';
		$messages = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {
			$matches = array();
			preg_match_all( "/\[\[([{$tc}]+)(?:\\|(.+?))?]]/sDu", $value, $matches);
			for ($i = 0; $i < count($matches[0]); $i++ ) {
				if ( preg_match( "/.*project.*/isDu",  $matches[1][$i]) ) {
					$messages[$key][] = $matches[0][$i];
				}
			}


			if ( isset( $messages[$key] ) ) {
				$messages[$key] = implode( $messages[$key],", " );
			}
		}
		return $messages;
	}

	public function getMessagesWithUnbalanced( $code ) {
		$this->loadGeneralMessages();
		$this->loadMessages( $code );
		$messages = array();
		foreach ( $this->mMessages[$code]['translated'] as $key => $value ) {

			$a = $b = $c = $d = 0;
			foreach ( preg_split('//', $value) as $char ) {
				switch ($char) {
					case '[': $a++; break;
					case ']': $b++; break;
					case '{': $c++; break;
					case '}': $d++; break;
				}
			}

			if ( $a !== $b || $c !== $d ) {
				$messages[$key] = "$a, $b, $c, $d";
			}
			
		}
		return $messages;
	}

}

class extensionLanguages extends languages {
	private $mMessageGroup; # The message group

	/**
	 * Load the messages group.
	 * @param $group The messages group.
	 */
	function __construct( MessageGroup $group ) {
		$this->mMessageGroup = $group;

		$bools = $this->mMessageGroup->getBools();
		$this->mIgnoredMessages = $bools['ignored'];
		$this->mOptionalMessages = $bools['optional'];
	}

	/**
	 * Get the extension name.
	 *
	 * @return The extension name.
	 */
	public function name() {
		return $this->mMessageGroup->getLabel();
	}

	/**
	 * Load the raw messages for a specific language.
	 *
	 * @param $code The language code.
	 */
	protected function loadRawMessages( $code ) {
		if( !isset( $this->mRawMessages[$code] ) ) {
			$this->mRawMessages[$code] = $this->mMessageGroup->load( $code );
			if( empty( $this->mRawMessages[$code] ) ) {
				$this->mRawMessages[$code] = array();
			}
		}
	}
}
