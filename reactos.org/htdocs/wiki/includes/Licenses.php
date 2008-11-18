<?php
/**
 * A License class for use on Special:Upload
 *
 * @ingroup SpecialPage
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

class Licenses {
	/**#@+
	 * @private
	 */
	/**
	 * @var string
	 */
	var $msg;

	/**
	 * @var array
	 */
	var $licenses = array();

	/**
	 * @var string
	 */
	var $html;
	/**#@-*/

	/**
	 * Constructor
	 *
	 * @param $str String: the string to build the licenses member from, will use
	 *                    wfMsgForContent( 'licenses' ) if null (default: null)
	 */
	function __construct( $str = null ) {
		// PHP sucks, this should be possible in the constructor
		$this->msg = is_null( $str ) ? wfMsgForContent( 'licenses' ) : $str;
		$this->html = '';

		$this->makeLicenses();
		$tmp = $this->getLicenses();
		$this->makeHtml( $tmp );
	}

	/**#@+
	 * @private
	 */
	function makeLicenses() {
		$levels = array();
		$lines = explode( "\n", $this->msg );

		foreach ( $lines as $line ) {
			if ( strpos( $line, '*' ) !== 0 )
				continue;
			else {
				list( $level, $line ) = $this->trimStars( $line );

				if ( strpos( $line, '|' ) !== false ) {
					$obj = new License( $line );
					$this->stackItem( $this->licenses, $levels, $obj );
				} else {
					if ( $level < count( $levels ) ) {
						$levels = array_slice( $levels, 0, $level );
					}
					if ( $level == count( $levels ) ) {
						$levels[$level - 1] = $line;
					} else if ( $level > count( $levels ) ) {
						$levels[] = $line;
					}
				}
			}
		}
	}

	function trimStars( $str ) {
		$i = $count = 0;

		wfSuppressWarnings();
		while ($str[$i++] == '*')
			++$count;
		wfRestoreWarnings();

		return array( $count, ltrim( $str, '* ' ) );
	}

	function stackItem( &$list, $path, $item ) {
		$position =& $list;
		if ( $path )
			foreach( $path as $key )
				$position =& $position[$key];
		$position[] = $item;
	}

	function makeHtml( &$tagset, $depth = 0 ) {
		foreach ( $tagset as $key => $val )
			if ( is_array( $val ) ) {
				$this->html .= $this->outputOption(
					$this->msg( $key ),
					array(
						'value' => '',
						'disabled' => 'disabled',
						'style' => 'color: GrayText', // for MSIE
					),
					$depth
				);
				$this->makeHtml( $val, $depth + 1 );
			} else {
				$this->html .= $this->outputOption(
					$this->msg( $val->text ),
					array(
						'value' => $val->template,
						'title' => '{{' . $val->template . '}}'
					),
					$depth
				);
			}
	}

	function outputOption( $val, $attribs = null, $depth ) {
		$val = str_repeat( /* &nbsp */ "\xc2\xa0", $depth * 2 ) . $val;
		return str_repeat( "\t", $depth ) . wfElement( 'option', $attribs, $val ) . "\n";
	}

	function msg( $str ) {
		$out = wfMsg( $str );
		return wfEmptyMsg( $str, $out ) ? $str : $out;
	}

	/**#@-*/

	/**
	 *  Accessor for $this->licenses
	 *
	 * @return array
	 */
	function getLicenses() { return $this->licenses; }

	/**
	 * Accessor for $this->html
	 *
	 * @return string
	 */
	function getHtml() { return $this->html; }
}

/**
 * A License class for use on Special:Upload (represents a single type of license).
 */
class License {
	/**
	 * @var string
	 */
	var $template;

	/**
	 * @var string
	 */
	var $text;

	/**
	 * Constructor
	 *
	 * @param $str String: license name??
	 */
	function License( $str ) {
		list( $text, $template ) = explode( '|', strrev( $str ), 2 );

		$this->template = strrev( $template );
		$this->text = strrev( $text );
	}
}
