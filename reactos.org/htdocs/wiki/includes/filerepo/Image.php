<?php

/**
 * Backwards compatibility class
 * @deprecated
 * @ingroup FileRepo
 */
class Image extends LocalFile {
	function __construct( $title ) {
		wfDeprecated( __METHOD__ );
		$repo = RepoGroup::singleton()->getLocalRepo();
		parent::__construct( $title, $repo );
	}

	/**
	 * Wrapper for wfFindFile(), for backwards-compatibility only
	 * Do not use in core code.
	 * @deprecated
	 */
	static function newFromTitle( $title, $time = false ) {
		wfDeprecated( __METHOD__ );
		$img = wfFindFile( $title, $time );
		if ( !$img ) {
			$img = wfLocalFile( $title );
		}
		return $img;
	}

	/**
	 * Wrapper for wfFindFile(), for backwards-compatibility only.
	 * Do not use in core code.
	 *
	 * @param string $name name of the image, used to create a title object using Title::makeTitleSafe
	 * @return image object or null if invalid title
	 * @deprecated
	 */
	static function newFromName( $name ) {
		wfDeprecated( __METHOD__ );
		$title = Title::makeTitleSafe( NS_IMAGE, $name );
		if ( is_object( $title ) ) {
			$img = wfFindFile( $title );
			if ( !$img ) {
				$img = wfLocalFile( $title );
			}
			return $img;
		} else {
			return NULL;
		}
	}

	/**
	 * Return the URL of an image, provided its name.
	 *
	 * Backwards-compatibility for extensions.
	 * Note that fromSharedDirectory will only use the shared path for files
	 * that actually exist there now, and will return local paths otherwise.
	 *
	 * @param string $name	Name of the image, without the leading "Image:"
	 * @param boolean $fromSharedDirectory	Should this be in $wgSharedUploadPath?
	 * @return string URL of $name image
	 * @deprecated
	 */
	static function imageUrl( $name, $fromSharedDirectory = false ) {
		wfDeprecated( __METHOD__ );
		$image = null;
		if( $fromSharedDirectory ) {
			$image = wfFindFile( $name );
		}
		if( !$image ) {
			$image = wfLocalFile( $name );
		}
		return $image->getUrl();
	}
}
