<?php
/**#@+
 * Give information about the version of MediaWiki, PHP, the DB and extensions
 *
 * @file
 * @ingroup SpecialPage
 *
 * @author Ævar Arnfjörð Bjarmason <avarab@gmail.com>
 * @copyright Copyright © 2005, Ævar Arnfjörð Bjarmason
 * @license http://www.gnu.org/copyleft/gpl.html GNU General Public License 2.0 or later
 */

/**
 * constructor
 */
function wfSpecialVersion() {
	$version = new SpecialVersion;
	$version->execute();
}

/**
 * @ingroup SpecialPage
 */
class SpecialVersion {
	private $firstExtOpened = true;

	/**
	 * main()
	 */
	function execute() {
		global $wgOut, $wgMessageCache, $wgSpecialVersionShowHooks;
		$wgMessageCache->loadAllMessages();

		$wgOut->addHTML( '<div dir="ltr">' );
		$text = 
			$this->MediaWikiCredits() .
			$this->softwareInformation() .
			$this->extensionCredits();
		if  ( $wgSpecialVersionShowHooks ) {
			$text .= $this->wgHooks();
		}
		$wgOut->addWikiText( $text );
		$wgOut->addHTML( $this->IPInfo() );
		$wgOut->addHTML( '</div>' );
	}

	/**#@+
	 * @private
	 */

	/**
	 * @return wiki text showing the license information
	 */
	static function MediaWikiCredits() {
		$ret = Xml::element( 'h2', array( 'id' => 'mw-version-license' ), wfMsg( 'version-license' ) ) .
		"__NOTOC__
		This wiki is powered by '''[http://www.mediawiki.org/ MediaWiki]''',
		copyright (C) 2001-2008 Magnus Manske, Brion Vibber, Lee Daniel Crocker,
		Tim Starling, Erik Möller, Gabriel Wicke, Ævar Arnfjörð Bjarmason,
		Niklas Laxström, Domas Mituzas, Rob Church, Yuri Astrakhan, Aryeh Gregor,
		Aaron Schulz and others.

		MediaWiki is free software; you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation; either version 2 of the License, or
		(at your option) any later version.

		MediaWiki is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
		GNU General Public License for more details.

		You should have received [{{SERVER}}{{SCRIPTPATH}}/COPYING a copy of the GNU General Public License]
		along with this program; if not, write to the Free Software
		Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
		or [http://www.gnu.org/licenses/old-licenses/gpl-2.0.html read it online].
		";

		return str_replace( "\t\t", '', $ret ) . "\n";
	}

	/**
	 * @return wiki text showing the third party software versions (apache, php, mysql).
	 */
	static function softwareInformation() {
		$dbr = wfGetDB( DB_SLAVE );

		return Xml::element( 'h2', array( 'id' => 'mw-version-software' ), wfMsg( 'version-software' ) ) .
			Xml::openElement( 'table', array( 'id' => 'sv-software' ) ) .
				"<tr>
					<th>" . wfMsg( 'version-software-product' ) . "</th>
					<th>" . wfMsg( 'version-software-version' ) . "</th>
				</tr>\n
				<tr>
					<td>[http://www.mediawiki.org/ MediaWiki]</td>
					<td>" . self::getVersionLinked() . "</td>
				</tr>\n
				<tr>
					<td>[http://www.php.net/ PHP]</td>
					<td>" . phpversion() . " (" . php_sapi_name() . ")</td>
				</tr>\n
				<tr>
					<td>" . $dbr->getSoftwareLink() . "</td>
					<td>" . $dbr->getServerVersion() . "</td>
				</tr>\n" .
			Xml::closeElement( 'table' );
	}

	/**
	 * Return a string of the MediaWiki version with SVN revision if available
	 *
	 * @return mixed
	 */
	public static function getVersion() {
		global $wgVersion, $IP;
		wfProfileIn( __METHOD__ );
		$svn = self::getSvnRevision( $IP );
		$version = $svn ? "$wgVersion (r$svn)" : $wgVersion;
		wfProfileOut( __METHOD__ );
		return $version;
	}
	
	/**
	 * Return a string of the MediaWiki version with a link to SVN revision if
	 * available
	 *
	 * @return mixed
	 */
	public static function getVersionLinked() {
		global $wgVersion, $IP;
		wfProfileIn( __METHOD__ );
		$svn = self::getSvnRevision( $IP );
		$viewvc = 'http://svn.wikimedia.org/viewvc/mediawiki/trunk/phase3/?pathrev=';
		$version = $svn ? "$wgVersion ([{$viewvc}{$svn} r$svn])" : $wgVersion;
		wfProfileOut( __METHOD__ );
		return $version;
	}

	/** Generate wikitext showing extensions name, URL, author and description */
	function extensionCredits() {
		global $wgExtensionCredits, $wgExtensionFunctions, $wgParser, $wgSkinExtensionFunctions;

		if ( ! count( $wgExtensionCredits ) && ! count( $wgExtensionFunctions ) && ! count( $wgSkinExtensionFunctions ) )
			return '';

		$extensionTypes = array(
			'specialpage' => wfMsg( 'version-specialpages' ),
			'parserhook' => wfMsg( 'version-parserhooks' ),
			'variable' => wfMsg( 'version-variables' ),
			'media' => wfMsg( 'version-mediahandlers' ),
			'other' => wfMsg( 'version-other' ),
		);
		wfRunHooks( 'SpecialVersionExtensionTypes', array( &$this, &$extensionTypes ) );

		$out = Xml::element( 'h2', array( 'id' => 'mw-version-ext' ), wfMsg( 'version-extensions' ) ) .
			Xml::openElement( 'table', array( 'id' => 'sv-ext' ) );

		foreach ( $extensionTypes as $type => $text ) {
			if ( isset ( $wgExtensionCredits[$type] ) && count ( $wgExtensionCredits[$type] ) ) {
				$out .= $this->openExtType( $text );

				usort( $wgExtensionCredits[$type], array( $this, 'compare' ) );

				foreach ( $wgExtensionCredits[$type] as $extension ) {
					if ( isset( $extension['version'] ) ) {
						$version = $extension['version'];
					} elseif ( isset( $extension['svn-revision'] ) && 
						preg_match( '/\$(?:Rev|LastChangedRevision|Revision): *(\d+)/', 
							$extension['svn-revision'], $m ) ) 
					{
						$version = 'r' . $m[1];
					} else {
						$version = null;
					}

					$out .= $this->formatCredits(
						isset ( $extension['name'] )           ? $extension['name']        : '',
						$version,
						isset ( $extension['author'] )         ? $extension['author']      : '',
						isset ( $extension['url'] )            ? $extension['url']         : null,
						isset ( $extension['description'] )    ? $extension['description'] : '',
						isset ( $extension['descriptionmsg'] ) ? $extension['descriptionmsg'] : ''
					);
				}
			}
		}

		if ( count( $wgExtensionFunctions ) ) {
			$out .= $this->openExtType( wfMsg( 'version-extension-functions' ) );
			$out .= '<tr><td colspan="3">' . $this->listToText( $wgExtensionFunctions ) . "</td></tr>\n";
		}

		if ( $cnt = count( $tags = $wgParser->getTags() ) ) {
			for ( $i = 0; $i < $cnt; ++$i )
				$tags[$i] = "&lt;{$tags[$i]}&gt;";
			$out .= $this->openExtType( wfMsg( 'version-parser-extensiontags' ) );
			$out .= '<tr><td colspan="3">' . $this->listToText( $tags ). "</td></tr>\n";
		}

		if( $cnt = count( $fhooks = $wgParser->getFunctionHooks() ) ) {
			$out .= $this->openExtType( wfMsg( 'version-parser-function-hooks' ) );
			$out .= '<tr><td colspan="3">' . $this->listToText( $fhooks ) . "</td></tr>\n";
		}

		if ( count( $wgSkinExtensionFunctions ) ) {
			$out .= $this->openExtType( wfMsg( 'version-skin-extension-functions' ) );
			$out .= '<tr><td colspan="3">' . $this->listToText( $wgSkinExtensionFunctions ) . "</td></tr>\n";
		}
		$out .= Xml::closeElement( 'table' );
		return $out;
	}

	/** Callback to sort extensions by type */
	function compare( $a, $b ) {
		global $wgLang;
		if( $a['name'] === $b['name'] ) {
			return 0;
		} else {
			return $wgLang->lc( $a['name'] ) > $wgLang->lc( $b['name'] )
				? 1
				: -1;
		}
	}

	function formatCredits( $name, $version = null, $author = null, $url = null, $description = null, $descriptionMsg = null ) {
		$extension = isset( $url ) ? "[$url $name]" : $name;
		$version = isset( $version ) ? "(" . wfMsg( 'version-version' ) . " $version)" : '';

		# Look for a localized description
		if( isset( $descriptionMsg ) ) {
			$msg = wfMsg( $descriptionMsg );
			if ( !wfEmptyMsg( $descriptionMsg, $msg ) && $msg != '' ) {
				$description = $msg;
			}
		}

		return "<tr>
				<td><em>$extension $version</em></td>
				<td>$description</td>
				<td>" . $this->listToText( (array)$author ) . "</td>
			</tr>\n";
	}

	/**
	 * @return string
	 */
	function wgHooks() {
		global $wgHooks;

		if ( count( $wgHooks ) ) {
			$myWgHooks = $wgHooks;
			ksort( $myWgHooks );

			$ret = Xml::element( 'h2', array( 'id' => 'mw-version-hooks' ), wfMsg( 'version-hooks' ) ) .
				Xml::openElement( 'table', array( 'id' => 'sv-hooks' ) ) .
				"<tr>
					<th>" . wfMsg( 'version-hook-name' ) . "</th>
					<th>" . wfMsg( 'version-hook-subscribedby' ) . "</th>
				</tr>\n";

			foreach ( $myWgHooks as $hook => $hooks )
				$ret .= "<tr>
						<td>$hook</td>
						<td>" . $this->listToText( $hooks ) . "</td>
					</tr>\n";

			$ret .= Xml::closeElement( 'table' );
			return $ret;
		} else
			return '';
	}

	private function openExtType($text, $name = null) {
		$opt = array( 'colspan' => 3 );
		$out = '';

		if(!$this->firstExtOpened) {
			// Insert a spacing line
			$out .= '<tr class="sv-space">' . Xml::element( 'td', $opt ) . "</tr>\n";
		}
		$this->firstExtOpened = false;

		if($name) { $opt['id'] = "sv-$name"; }

		$out .= "<tr>" . Xml::element( 'th', $opt, $text) . "</tr>\n";
		return $out;
	}

	/**
	 * @static
	 *
	 * @return string
	 */
	function IPInfo() {
		$ip =  str_replace( '--', ' - ', htmlspecialchars( wfGetIP() ) );
		return "<!-- visited from $ip -->\n" .
			"<span style='display:none'>visited from $ip</span>";
	}

	/**
	 * @param array $list
	 * @return string
	 */
	function listToText( $list ) {
		$cnt = count( $list );

		if ( $cnt == 1 ) {
			// Enforce always returning a string
			return (string)$this->arrayToString( $list[0] );
		} elseif ( $cnt == 0 ) {
			return '';
		} else {
			sort( $list );
			$t = array_slice( $list, 0, $cnt - 1 );
			$one = array_map( array( &$this, 'arrayToString' ), $t );
			$two = $this->arrayToString( $list[$cnt - 1] );
			$and = wfMsg( 'and' );

			return implode( ', ', $one ) . " $and $two";
		}
	}

	/**
	 * @static
	 *
	 * @param mixed $list Will convert an array to string if given and return
	 *                    the paramater unaltered otherwise
	 * @return mixed
	 */
	function arrayToString( $list ) {
		if( is_object( $list ) ) {
			$class = get_class( $list );
			return "($class)";
		} elseif ( ! is_array( $list ) ) {
			return $list;
		} else {
			$class = get_class( $list[0] );
			return "($class, {$list[1]})";
		}
	}

	/**
	 * Retrieve the revision number of a Subversion working directory.
	 *
	 * @param string $dir
	 * @return mixed revision number as int, or false if not a SVN checkout
	 */
	public static function getSvnRevision( $dir ) {
		// http://svnbook.red-bean.com/nightly/en/svn.developer.insidewc.html
		$entries = $dir . '/.svn/entries';

		if( !file_exists( $entries ) ) {
			return false;
		}

		$content = file( $entries );

		// check if file is xml (subversion release <= 1.3) or not (subversion release = 1.4)
		if( preg_match( '/^<\?xml/', $content[0] ) ) {
			// subversion is release <= 1.3
			if( !function_exists( 'simplexml_load_file' ) ) {
				// We could fall back to expat... YUCK
				return false;
			}

			// SimpleXml whines about the xmlns...
			wfSuppressWarnings();
			$xml = simplexml_load_file( $entries );
			wfRestoreWarnings();

			if( $xml ) {
				foreach( $xml->entry as $entry ) {
					if( $xml->entry[0]['name'] == '' ) {
						// The directory entry should always have a revision marker.
						if( $entry['revision'] ) {
							return intval( $entry['revision'] );
						}
					}
				}
			}
			return false;
		} else {
			// subversion is release 1.4
			return intval( $content[3] );
		}
	}

	/**#@-*/
}

/**#@-*/
