<?php
/**
 * Provide things related to namespaces
 * @file
 */

/**
 * Definitions of the NS_ constants are in Defines.php
 * @private
 */
$wgCanonicalNamespaceNames = array(
	NS_MEDIA            => 'Media',
	NS_SPECIAL          => 'Special',
	NS_TALK             => 'Talk',
	NS_USER             => 'User',
	NS_USER_TALK        => 'User_talk',
	NS_PROJECT          => 'Project',
	NS_PROJECT_TALK     => 'Project_talk',
	NS_IMAGE            => 'Image',
	NS_IMAGE_TALK       => 'Image_talk',
	NS_MEDIAWIKI        => 'MediaWiki',
	NS_MEDIAWIKI_TALK   => 'MediaWiki_talk',
	NS_TEMPLATE         => 'Template',
	NS_TEMPLATE_TALK    => 'Template_talk',
	NS_HELP             => 'Help',
	NS_HELP_TALK        => 'Help_talk',
	NS_CATEGORY         => 'Category',
	NS_CATEGORY_TALK    => 'Category_talk',
);

if( is_array( $wgExtraNamespaces ) ) {
	$wgCanonicalNamespaceNames = $wgCanonicalNamespaceNames + $wgExtraNamespaces;
}

/**
 * This is a utility class with only static functions
 * for dealing with namespaces that encodes all the
 * "magic" behaviors of them based on index.  The textual
 * names of the namespaces are handled by Language.php.
 *
 * These are synonyms for the names given in the language file
 * Users and translators should not change them
 *
 */

class MWNamespace {

	/**
	 * Can pages in the given namespace be moved?
	 *
	 * @param $index Int: namespace index
	 * @return bool
	 */
	public static function isMovable( $index ) {
		global $wgAllowImageMoving;
		return !( $index < NS_MAIN || ($index == NS_IMAGE && !$wgAllowImageMoving)  || $index == NS_CATEGORY );
	}

	/**
	 * Is the given namespace is a subject (non-talk) namespace?
	 *
	 * @param $index Int: namespace index
	 * @return bool
	 */
	public static function isMain( $index ) {
		return !self::isTalk( $index );
	}

	/**
	 * Is the given namespace a talk namespace?
	 *
	 * @param $index Int: namespace index
	 * @return bool
	 */
	public static function isTalk( $index ) {
		return $index > NS_MAIN
			&& $index % 2;
	}

	/**
	 * Get the talk namespace index for a given namespace
	 *
	 * @param $index Int: namespace index
	 * @return int
	 */
	public static function getTalk( $index ) {
		return self::isTalk( $index )
			? $index
			: $index + 1;
	}

	/**
	 * Get the subject namespace index for a given namespace
	 *
	 * @param $index Int: Namespace index
	 * @return int
	 */
	public static function getSubject( $index ) {
		return self::isTalk( $index )
			? $index - 1
			: $index;
	}

	/**
	 * Returns the canonical (English Wikipedia) name for a given index
	 *
	 * @param $index Int: namespace index
	 * @return string
	 */
	public static function getCanonicalName( $index ) {
		global $wgCanonicalNamespaceNames;
		return $wgCanonicalNamespaceNames[$index];
	}

	/**
	 * Returns the index for a given canonical name, or NULL
	 * The input *must* be converted to lower case first
	 *
	 * @param $name String: namespace name
	 * @return int
	 */
	public static function getCanonicalIndex( $name ) {
		global $wgCanonicalNamespaceNames;
		static $xNamespaces = false;
		if ( $xNamespaces === false ) {
			$xNamespaces = array();
			foreach ( $wgCanonicalNamespaceNames as $i => $text ) {
				$xNamespaces[strtolower($text)] = $i;
			}
		}
		if ( array_key_exists( $name, $xNamespaces ) ) {
			return $xNamespaces[$name];
		} else {
			return NULL;
		}
	}

	/**
	 * Can this namespace ever have a talk namespace?
	 *
	 * @param $index Int: namespace index
	 * @return bool
	 */
	 public static function canTalk( $index ) {
	 	return $index >= NS_MAIN;
	 }

	/**
	 * Does this namespace contain content, for the purposes of calculating
	 * statistics, etc?
	 *
	 * @param $index Int: index to check
	 * @return bool
	 */
	public static function isContent( $index ) {
		global $wgContentNamespaces;
		return $index == NS_MAIN || in_array( $index, $wgContentNamespaces );
	}

	/**
	 * Can pages in a namespace be watched?
	 *
	 * @param $index Int
	 * @return bool
	 */
	public static function isWatchable( $index ) {
		return $index >= NS_MAIN;
	}

	/**
	 * Does the namespace allow subpages?
	 *
	 * @param $index int Index to check
	 * @return bool
	 */
	public static function hasSubpages( $index ) {
		global $wgNamespacesWithSubpages;
		return !empty( $wgNamespacesWithSubpages[$index] );
	}

}
