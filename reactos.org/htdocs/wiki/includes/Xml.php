<?php

/**
 * Module of static functions for generating XML
 */

class Xml {
	/**
	 * Format an XML element with given attributes and, optionally, text content.
	 * Element and attribute names are assumed to be ready for literal inclusion.
	 * Strings are assumed to not contain XML-illegal characters; special
	 * characters (<, >, &) are escaped but illegals are not touched.
	 *
	 * @param $element String: element name
	 * @param $attribs Array: Name=>value pairs. Values will be escaped.
	 * @param $contents String: NULL to make an open tag only; '' for a contentless closed tag (default)
	 * @param $allowShortTag Bool: whether '' in $contents will result in a contentless closed tag
	 * @return string
	 */
	public static function element( $element, $attribs = null, $contents = '', $allowShortTag = true ) {
		$out = '<' . $element;
		if( !is_null( $attribs ) ) {
			$out .=  self::expandAttributes( $attribs );
		}
		if( is_null( $contents ) ) {
			$out .= '>';
		} else {
			if( $allowShortTag && $contents === '' ) {
				$out .= ' />';
			} else {
				$out .= '>' . htmlspecialchars( $contents ) . "</$element>";
			}
		}
		return $out;
	}

	/**
	 * Given an array of ('attributename' => 'value'), it generates the code
	 * to set the XML attributes : attributename="value".
	 * The values are passed to Sanitizer::encodeAttribute.
	 * Return null if no attributes given.
	 * @param $attribs Array of attributes for an XML element
	 */
	public static function expandAttributes( $attribs ) {
		$out = '';
		if( is_null( $attribs ) ) {
			return null;
		} elseif( is_array( $attribs ) ) {
			foreach( $attribs as $name => $val )
				$out .= " {$name}=\"" . Sanitizer::encodeAttribute( $val ) . '"';
			return $out;
		} else {
			throw new MWException( 'Expected attribute array, got something else in ' . __METHOD__ );
		}
	}

	/**
	 * Format an XML element as with self::element(), but run text through the
	 * UtfNormal::cleanUp() validator first to ensure that no invalid UTF-8
	 * is passed.
	 *
	 * @param $element String:
	 * @param $attribs Array: Name=>value pairs. Values will be escaped.
	 * @param $contents String: NULL to make an open tag only; '' for a contentless closed tag (default)
	 * @return string
	 */
	public static function elementClean( $element, $attribs = array(), $contents = '') {
		if( $attribs ) {
			$attribs = array_map( array( 'UtfNormal', 'cleanUp' ), $attribs );
		}
		if( $contents ) {
			wfProfileIn( __METHOD__ . '-norm' );
			$contents = UtfNormal::cleanUp( $contents );
			wfProfileOut( __METHOD__ . '-norm' );
		}
		return self::element( $element, $attribs, $contents );
	}

	/**
	 * This opens an XML element
	 *
	 * @param $element name of the element
	 * @param $attribs array of attributes, see Xml::expandAttributes()
	 * @return string
	 */
	public static function openElement( $element, $attribs = null ) {
		return '<' . $element . self::expandAttributes( $attribs ) . '>';
	}

	/**
	 * Shortcut to close an XML element
	 * @param $element element name
	 * @return string
	 */
	public static function closeElement( $element ) { return "</$element>"; }

	/**
	 * Same as Xml::element(), but does not escape contents. Handy when the
	 * content you have is already valid xml.
	 *
	 * @param $element element name
	 * @param $attribs array of attributes
	 * @param $contents content of the element
	 * @return string
	 */
	public static function tags( $element, $attribs = null, $contents ) {
		return self::openElement( $element, $attribs ) . $contents . "</$element>";
	}

	/**
	 * Build a drop-down box for selecting a namespace
	 *
	 * @param $selected Mixed: Namespace which should be pre-selected
	 * @param $all Mixed: Value of an item denoting all namespaces, or null to omit
	 * @param $hidden Mixed: Include hidden namespaces? [WTF? --RC]
	 * @param $element_name String: value of the "name" attribute of the select tag
	 * @return string
	 */
	public static function namespaceSelector( $selected = '', $all = null, $hidden = false, $element_name = 'namespace' ) {
		global $wgContLang;
		$namespaces = $wgContLang->getFormattedNamespaces();
		$options = array();

		// Godawful hack... we'll be frequently passed selected namespaces
		// as strings since PHP is such a shithole.
		// But we also don't want blanks and nulls and "all"s matching 0,
		// so let's convert *just* string ints to clean ints.
		if( preg_match( '/^\d+$/', $selected ) ) {
			$selected = intval( $selected );
		}

		if( !is_null( $all ) )
			$namespaces = array( $all => wfMsg( 'namespacesall' ) ) + $namespaces;
		foreach( $namespaces as $index => $name ) {
			if( $index < NS_MAIN )
				continue;
			if( $index === 0 )
				$name = wfMsg( 'blanknamespace' );
			$options[] = self::option( $name, $index, $index === $selected );
		}

		return Xml::openElement( 'select', array( 'id' => 'namespace', 'name' => $element_name,
			'class' => 'namespaceselector' ) )
			. "\n"
			. implode( "\n", $options )
			. "\n"
			. Xml::closeElement( 'select' );
	}

	/**
	 * Create a date selector
	 *
	 * @param $selected Mixed: the month which should be selected, default ''
	 * @param $allmonths String: value of a special item denoting all month. Null to not include (default)
	 * @param $id String: Element identifier
	 * @return String: Html string containing the month selector
	 */
	public static function monthSelector( $selected = '', $allmonths = null, $id = 'month' ) {
		global $wgLang;
		$options = array();
	    if( is_null( $selected ) )
			$selected = '';
	    if( !is_null( $allmonths ) )
			$options[] = self::option( wfMsg( 'monthsall' ), $allmonths, $selected === $allmonths );
		for( $i = 1; $i < 13; $i++ )
				$options[] = self::option( $wgLang->getMonthName( $i ), $i, $selected === $i );
		return self::openElement( 'select', array( 'id' => $id, 'name' => 'month', 'class' => 'mw-month-selector' ) )
			. implode( "\n", $options )
			. self::closeElement( 'select' );
	}

	/**
	 *
	 * @param $selected The language code of the selected language
	 * @param $customisedOnly If true only languages which have some content are listed
	 * @return array of label and select
	 */
	public static function languageSelector( $selected, $customisedOnly = true ) {
		global $wgContLanguageCode;
		/**
		 * Make sure the site language is in the list; a custom language code
		 * might not have a defined name...
		 */
		$languages = Language::getLanguageNames( $customisedOnly );
		if( !array_key_exists( $wgContLanguageCode, $languages ) ) {
			$languages[$wgContLanguageCode] = $wgContLanguageCode;
		}
		ksort( $languages );

		/**
		 * If a bogus value is set, default to the content language.
		 * Otherwise, no default is selected and the user ends up
		 * with an Afrikaans interface since it's first in the list.
		 */
		$selected = isset( $languages[$selected] ) ? $selected : $wgContLanguageCode;
		$options = "\n";
		foreach( $languages as $code => $name ) {
			$options .= Xml::option( "$code - $name", $code, ($code == $selected) ) . "\n";
		}

		return array(
			Xml::label( wfMsg('yourlanguage'), 'wpUserLanguage' ),
			Xml::tags( 'select',
				array( 'id' => 'wpUserLanguage', 'name' => 'wpUserLanguage' ),
				$options
			)
		);

	}

	/**
	 * Shortcut to make a span element
	 * @param $text content of the element, will be escaped
	 * @param $class class name of the span element
	 * @param $attribs other attributes
	 * @return string 
	 */
	public static function span( $text, $class, $attribs=array() ) {
		return self::element( 'span', array( 'class' => $class ) + $attribs, $text );
	}

	/**
	 * Shortcut to make a specific element with a class attribute
	 * @param $text content of the element, will be escaped
	 * @param $class class name of the span element
	 * @param $tag element name
	 * @param $attribs other attributes
	 * @return string 
	 */
	public static function wrapClass( $text, $class, $tag='span', $attribs=array() ) {
		return self::tags( $tag, array( 'class' => $class ) + $attribs, $text );
	}

	/**
	 * Convenience function to build an HTML text input field
	 * @param $name value of the name attribute
	 * @param $size value of the size attribute
	 * @param $value value of the value attribute
	 * @param $attribs other attributes
	 * @return string HTML
	 */
	public static function input( $name, $size=false, $value=false, $attribs=array() ) {
		return self::element( 'input', array(
			'name' => $name,
			'size' => $size,
			'value' => $value ) + $attribs );
	}

	/**
	 * Convenience function to build an HTML password input field
	 * @param $name value of the name attribute
	 * @param $size value of the size attribute
	 * @param $value value of the value attribute
	 * @param $attribs other attributes
	 * @return string HTML
	 */
	public static function password( $name, $size=false, $value=false, $attribs=array() ) {
		return self::input( $name, $size, $value, array_merge($attribs, array('type' => 'password')));
	}

	/**
	 * Internal function for use in checkboxes and radio buttons and such.
	 * @return array
	 */
	public static function attrib( $name, $present = true ) {
		return $present ? array( $name => $name ) : array();
	}

	/**
	 * Convenience function to build an HTML checkbox
	 * @param $name value of the name attribute
	 * @param $checked Whether the checkbox is checked or not
	 * @param $attribs other attributes
	 * @return string HTML
	 */
	public static function check( $name, $checked=false, $attribs=array() ) {
		return self::element( 'input', array_merge(
			array(
				'name' => $name,
				'type' => 'checkbox',
				'value' => 1 ),
			self::attrib( 'checked', $checked ),
			$attribs ) );
	}

	/**
	 * Convenience function to build an HTML radio button
	 * @param $name value of the name attribute
	 * @param $value value of the value attribute
	 * @param $checked Whether the checkbox is checked or not
	 * @param $attribs other attributes
	 * @return string HTML
	 */
	public static function radio( $name, $value, $checked=false, $attribs=array() ) {
		return self::element( 'input', array(
			'name' => $name,
			'type' => 'radio',
			'value' => $value ) + self::attrib( 'checked', $checked ) + $attribs );
	}

	/**
	 * Convenience function to build an HTML form label
	 * @param $label text of the label
	 * @param $id 
	 * @return string HTML
	 */
	public static function label( $label, $id ) {
		return self::element( 'label', array( 'for' => $id ), $label );
	}

	/**
	 * Convenience function to build an HTML text input field with a label
	 * @param $label text of the label
	 * @param $name value of the name attribute
	 * @param $id id of the input
	 * @param $size value of the size attribute
	 * @param $value value of the value attribute
	 * @param $attribs other attributes
	 * @return string HTML
	 */
	public static function inputLabel( $label, $name, $id, $size=false, $value=false, $attribs=array() ) {
		list( $label, $input ) = self::inputLabelSep( $label, $name, $id, $size, $value, $attribs );
		return $label . '&nbsp;' . $input;
	}

	/**
	 * Same as Xml::inputLabel() but return input and label in an array
	 */
	public static function inputLabelSep( $label, $name, $id, $size=false, $value=false, $attribs=array() ) {
		return array(
			Xml::label( $label, $id ),
			self::input( $name, $size, $value, array( 'id' => $id ) + $attribs )
		);
	}

	/**
	 * Convenience function to build an HTML checkbox with a label
	 * @return string HTML
	 */
	public static function checkLabel( $label, $name, $id, $checked=false, $attribs=array() ) {
		return self::check( $name, $checked, array( 'id' => $id ) + $attribs ) .
			'&nbsp;' .
			self::label( $label, $id );
	}

	/**
	 * Convenience function to build an HTML radio button with a label
	 * @return string HTML
	 */
	public static function radioLabel( $label, $name, $value, $id, $checked=false, $attribs=array() ) {
		return self::radio( $name, $value, $checked, array( 'id' => $id ) + $attribs ) .
			'&nbsp;' .
			self::label( $label, $id );
	}

	/**
	 * Convenience function to build an HTML submit button
	 * @param $value String: label text for the button
	 * @param $attribs Array: optional custom attributes
	 * @return string HTML
	 */
	public static function submitButton( $value, $attribs=array() ) {
		return self::element( 'input', array( 'type' => 'submit', 'value' => $value ) + $attribs );
	}

	/**
	 * Convenience function to build an HTML hidden form field.
	 * @param $name String: name attribute for the field
	 * @param $value String: value for the hidden field
	 * @param $attribs Array: optional custom attributes
	 * @return string HTML
	 */
	public static function hidden( $name, $value, $attribs=array() ) {
		return self::element( 'input', array(
			'name' => $name,
			'type' => 'hidden',
			'value' => $value ) + $attribs );
	}

	/**
	 * Convenience function to build an HTML drop-down list item.
	 * @param $text String: text for this item
	 * @param $value String: form submission value; if empty, use text
	 * @param $selected boolean: if true, will be the default selected item
	 * @param $attribs array: optional additional HTML attributes
	 * @return string HTML
	 */
	public static function option( $text, $value=null, $selected=false,
			$attribs=array() ) {
		if( !is_null( $value ) ) {
			$attribs['value'] = $value;
		}
		if( $selected ) {
			$attribs['selected'] = 'selected';
		}
		return self::element( 'option', $attribs, $text );
	}

	/**
	 * Build a drop-down box from a textual list.
	 *
	 * @param $name Mixed: Name and id for the drop-down
	 * @param $class Mixed: CSS classes for the drop-down
	 * @param $other Mixed: Text for the "Other reasons" option
	 * @param $list Mixed: Correctly formatted text to be used to generate the options
	 * @param $selected Mixed: Option which should be pre-selected
	 * @param $tabindex Mixed: Value of the tabindex attribute
	 * @return string
	 */
	public static function listDropDown( $name= '', $list = '', $other = '', $selected = '', $class = '', $tabindex = Null ) {
		$options = '';
		$optgroup = false;

		$options = self::option( $other, 'other', $selected === 'other' );

		foreach ( explode( "\n", $list ) as $option) {
				$value = trim( $option );
				if ( $value == '' ) {
					continue;
				} elseif ( substr( $value, 0, 1) == '*' && substr( $value, 1, 1) != '*' ) {
					// A new group is starting ...
					$value = trim( substr( $value, 1 ) );
					if( $optgroup ) $options .= self::closeElement('optgroup');
					$options .= self::openElement( 'optgroup', array( 'label' => $value ) );
					$optgroup = true;
				} elseif ( substr( $value, 0, 2) == '**' ) {
					// groupmember
					$value = trim( substr( $value, 2 ) );
					$options .= self::option( $value, $value, $selected === $value );
				} else {
					// groupless reason list
					if( $optgroup ) $options .= self::closeElement('optgroup');
					$options .= self::option( $value, $value, $selected === $value );
					$optgroup = false;
				}
			}
			if( $optgroup ) $options .= self::closeElement('optgroup');

		$attribs = array();
		if( $name ) {
			$attribs['id'] = $name;
			$attribs['name'] = $name;
		}
		if( $class ) {
			$attribs['class'] = $class;
		}
		if( $tabindex ) {
			$attribs['tabindex'] = $tabindex;
		}
		return Xml::openElement( 'select', $attribs )
			. "\n"
			. $options
			. "\n"
			. Xml::closeElement( 'select' );
	}

	/**
	 * Shortcut for creating fieldsets.
	 *
	 * @param $legend Legend of the fieldset. If evaluates to false, legend is not added.
	 * @param $content Pre-escaped content for the fieldset. If false, only open fieldset is returned.
	 * @param $attribs Any attributes to fieldset-element.
	 */
	public static function fieldset( $legend = false, $content = false, $attribs = array() ) {
		$s = Xml::openElement( 'fieldset', $attribs ) . "\n";
		if ( $legend ) {
			$s .= Xml::element( 'legend', null, $legend ) . "\n";
		}
		if ( $content !== false ) {
			$s .= $content . "\n";
			$s .= Xml::closeElement( 'fieldset' ) . "\n";
		}

		return $s;
	}
	
	/**
	 * Shortcut for creating textareas.
	 *
	 * @param $name The 'name' for the textarea
	 * @param $content Content for the textarea
	 * @param $cols The number of columns for the textarea
	 * @param $rows The number of rows for the textarea
	 * @param $attribs Any other attributes for the textarea
	 */
	public static function textarea( $name, $content, $cols = 40, $rows = 5, $attribs = array() ) {
		return self::element( 'textarea',
					array(	'name' => $name,
						'id' => $name,
						'cols' => $cols,
						'rows' => $rows
					) + $attribs,
					$content, false );
	}

	/**
	 * Returns an escaped string suitable for inclusion in a string literal
	 * for JavaScript source code.
	 * Illegal control characters are assumed not to be present.
	 *
	 * @param $string String to escape
	 * @return String
	 */
	public static function escapeJsString( $string ) {
		// See ECMA 262 section 7.8.4 for string literal format
		$pairs = array(
			"\\" => "\\\\",
			"\"" => "\\\"",
			'\'' => '\\\'',
			"\n" => "\\n",
			"\r" => "\\r",

			# To avoid closing the element or CDATA section
			"<" => "\\x3c",
			">" => "\\x3e",

			# To avoid any complaints about bad entity refs
			"&" => "\\x26",

			# Work around https://bugzilla.mozilla.org/show_bug.cgi?id=274152
			# Encode certain Unicode formatting chars so affected
			# versions of Gecko don't misinterpret our strings;
			# this is a common problem with Farsi text.
			"\xe2\x80\x8c" => "\\u200c", // ZERO WIDTH NON-JOINER
			"\xe2\x80\x8d" => "\\u200d", // ZERO WIDTH JOINER
		);
		return strtr( $string, $pairs );
	}

	/**
	 * Encode a variable of unknown type to JavaScript.
	 * Arrays are converted to JS arrays, objects are converted to JS associative
	 * arrays (objects). So cast your PHP associative arrays to objects before
	 * passing them to here.
	 */
	public static function encodeJsVar( $value ) {
		if ( is_bool( $value ) ) {
			$s = $value ? 'true' : 'false';
		} elseif ( is_null( $value ) ) {
			$s = 'null';
		} elseif ( is_int( $value ) ) {
			$s = $value;
		} elseif ( is_array( $value ) ) {
			$s = '[';
			foreach ( $value as $elt ) {
				if ( $s != '[' ) {
					$s .= ', ';
				}
				$s .= self::encodeJsVar( $elt );
			}
			$s .= ']';
		} elseif ( is_object( $value ) ) {
			$s = '{';
			foreach ( (array)$value as $name => $elt ) {
				if ( $s != '{' ) {
					$s .= ', ';
				}
				$s .= '"' . self::escapeJsString( $name ) . '": ' .
					self::encodeJsVar( $elt );
			}
			$s .= '}';
		} else {
			$s = '"' . self::escapeJsString( $value ) . '"';
		}
		return $s;
	}


	/**
	 * Check if a string is well-formed XML.
	 * Must include the surrounding tag.
	 *
	 * @param $text String: string to test.
	 * @return bool
	 *
	 * @todo Error position reporting return
	 */
	public static function isWellFormed( $text ) {
		$parser = xml_parser_create( "UTF-8" );

		# case folding violates XML standard, turn it off
		xml_parser_set_option( $parser, XML_OPTION_CASE_FOLDING, false );

		if( !xml_parse( $parser, $text, true ) ) {
			//$err = xml_error_string( xml_get_error_code( $parser ) );
			//$position = xml_get_current_byte_index( $parser );
			//$fragment = $this->extractFragment( $html, $position );
			//$this->mXmlError = "$err at byte $position:\n$fragment";
			xml_parser_free( $parser );
			return false;
		}
		xml_parser_free( $parser );
		return true;
	}

	/**
	 * Check if a string is a well-formed XML fragment.
	 * Wraps fragment in an \<html\> bit and doctype, so it can be a fragment
	 * and can use HTML named entities.
	 *
	 * @param $text String:
	 * @return bool
	 */
	public static function isWellFormedXmlFragment( $text ) {
		$html =
			Sanitizer::hackDocType() .
			'<html>' .
			$text .
			'</html>';
		return Xml::isWellFormed( $html );
	}

	/**
	 * Replace " > and < with their respective HTML entities ( &quot;,
	 * &gt;, &lt;)
	 *
	 * @param $in String: text that might contain HTML tags.
	 * @return string Escaped string
	 */
	public static function escapeTagsOnly( $in ) {
		return str_replace(
			array( '"', '>', '<' ),
			array( '&quot;', '&gt;', '&lt;' ),
			$in );
	}
	
	/**
	* Generate a form (without the opening form element).
	* Output optionally includes a submit button.
	* @param $fields Associative array, key is message corresponding to a description for the field (colon is in the message), value is appropriate input.
	* @param $submitLabel A message containing a label for the submit button.
	* @return string HTML form.
	*/
	public static function buildForm( $fields, $submitLabel = null ) {
		$form = '';
		$form .= "<table><tbody>";
	
		foreach( $fields as $labelmsg => $input ) {
			$id = "mw-$labelmsg";
			
			$form .= Xml::openElement( 'tr', array( 'id' => $id ) );
			$form .= Xml::tags( 'td', array('class' => 'mw-label'), wfMsgExt( $labelmsg, array('parseinline') ) );
			$form .= Xml::openElement( 'td' ) . $input . Xml::closeElement( 'td' );
			$form .= Xml::closeElement( 'tr' );
		}
	
		$form .= "</tbody></table>";
		
		if ($submitLabel) {	
			$form .= Xml::submitButton( wfMsg($submitLabel) );
		}
	
		return $form;
	}
}

class XmlSelect {
	protected $options = array();
	protected $default = false;
	protected $attributes = array();

	public function __construct( $name = false, $id = false, $default = false ) {
		if ( $name ) $this->setAttribute( 'name', $name );
		if ( $id ) $this->setAttribute( 'id', $id );
		if ( $default ) $this->default = $default;
	}

	public function setDefault( $default ) {
		$this->default = $default;
	}

	public function setAttribute( $name, $value ) {
		$this->attributes[$name] = $value;
	}

	public function addOption( $name, $value = false ) {
		$value = $value ? $value : $name;
		$this->options[] = Xml::option( $name, $value, $value === $this->default );
	}

	public function getHTML() {
		return Xml::tags( 'select', $this->attributes, implode( "\n", $this->options ) );
	}

}