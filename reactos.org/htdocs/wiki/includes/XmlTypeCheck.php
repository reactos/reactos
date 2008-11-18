<?php

class XmlTypeCheck {
	/**
	 * Will be set to true or false to indicate whether the file is
	 * well-formed XML. Note that this doesn't check schema validity.
	 */
	public $wellFormed = false;

	/**
	 * Name of the document's root element, including any namespace
	 * as an expanded URL.
	 */
	public $rootElement = '';

	private $softNamespaces;
	private $namespaces = array();

	/**
	 * @param $file string filename
	 * @param $softNamespaces bool
	 *        If set to true, use of undeclared XML namespaces will be ignored.
	 *        This matches the behavior of rsvg, but more compliant consumers
	 *        such as Firefox will reject such files.
	 *        Leave off for the default, stricter checks.
	 */
	function __construct( $file, $softNamespaces=false ) {
		$this->softNamespaces = $softNamespaces;
		$this->run( $file );
	}

	private function run( $fname ) {
		if( $this->softNamespaces ) {
			$parser = xml_parser_create( 'UTF-8' );
		} else {
			$parser = xml_parser_create_ns( 'UTF-8' );
		}

		// case folding violates XML standard, turn it off
		xml_parser_set_option( $parser, XML_OPTION_CASE_FOLDING, false );

		xml_set_element_handler( $parser, array( $this, 'elementOpen' ), false );

		$file = fopen( $fname, "rb" );
		do {
			$chunk = fread( $file, 32768 );
			$ret = xml_parse( $parser, $chunk, feof( $file ) );
			if( $ret == 0 ) {
				// XML isn't well-formed!
				fclose( $file );
				xml_parser_free( $parser );
				return;
			}
		} while( !feof( $file ) );

		$this->wellFormed = true;

		fclose( $file );
		xml_parser_free( $parser );
	}

	private function elementOpen( $parser, $name, $attribs ) {
		if( $this->softNamespaces ) {
			// Check namespaces manually, so expat doesn't throw
			// errors on use of undeclared namespaces.
			foreach( $attribs as $attrib => $val ) {
				if( $attrib == 'xmlns' ) {
					$this->namespaces[''] = $val;
				} elseif( substr( $attrib, 0, strlen( 'xmlns:' ) ) == 'xmlns:' ) {
					$this->namespaces[substr( $attrib, strlen( 'xmlns:' ) )] = $val;
				}
			}

			if( strpos( $name, ':' ) === false ) {
				$ns = '';
				$subname = $name;
			} else {
				list( $ns, $subname ) = explode( ':', $name, 2 );
			}

			if( isset( $this->namespaces[$ns] ) ) {
				$name = $this->namespaces[$ns] . ':' . $subname;
			} else {
				// Technically this is invalid for XML with Namespaces.
				// But..... we'll just let it slide in soft mode.
			}
		}

		// We only need the first open element
		$this->rootElement = $name;
		xml_set_element_handler( $parser, false, false );
	}
}
