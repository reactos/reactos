<?php
/**
 * Cache for article titles (prefixed DB keys) and ids linked from one source
 *
 * @ingroup Cache
 */
class LinkCache {
	// Increment $mClassVer whenever old serialized versions of this class
	// becomes incompatible with the new version.
	/* private */ var $mClassVer = 4;

	/* private */ var $mPageLinks;
	/* private */ var $mGoodLinks, $mBadLinks;
	/* private */ var $mForUpdate;

	/**
	 * Get an instance of this class
	 */
	static function &singleton() {
		static $instance;
		if ( !isset( $instance ) ) {
			$instance = new LinkCache;
		}
		return $instance;
	}

	function __construct() {
		$this->mForUpdate = false;
		$this->mPageLinks = array();
		$this->mGoodLinks = array();
		$this->mGoodLinkFields = array();
		$this->mBadLinks = array();
	}

	/**
	 * General accessor to get/set whether SELECT FOR UPDATE should be used
	 */
	public function forUpdate( $update = NULL ) {
		return wfSetVar( $this->mForUpdate, $update );
	}

	public function getGoodLinkID( $title ) {
		if ( array_key_exists( $title, $this->mGoodLinks ) ) {
			return $this->mGoodLinks[$title];
		} else {
			return 0;
		}
	}

	/**
	 * Get a field of a title object from cache.
	 * If this link is not good, it will return NULL.
	 * @param Title $title
	 * @param string $field ('length','redirect')
	 * @return mixed
	 */
	public function getGoodLinkFieldObj( $title, $field ) {
		$dbkey = $title->getPrefixedDbKey();
		if ( array_key_exists( $dbkey, $this->mGoodLinkFields ) ) {
			return $this->mGoodLinkFields[$dbkey][$field];
		} else {
			return NULL;
		}
	}

	public function isBadLink( $title ) {
		return array_key_exists( $title, $this->mBadLinks );
	}

	/**
	 * Add a link for the title to the link cache
	 * @param int $id
	 * @param Title $title
	 * @param int $len
	 * @param int $redir
	 */
	public function addGoodLinkObj( $id, $title, $len = -1, $redir = NULL ) {
		$dbkey = $title->getPrefixedDbKey();
		$this->mGoodLinks[$dbkey] = $id;
		$this->mGoodLinkFields[$dbkey] = array( 'length' => $len, 'redirect' => $redir );
		$this->mPageLinks[$dbkey] = $title;
	}

	public function addBadLinkObj( $title ) {
		$dbkey = $title->getPrefixedDbKey();
		if ( ! $this->isBadLink( $dbkey ) ) {
			$this->mBadLinks[$dbkey] = 1;
			$this->mPageLinks[$dbkey] = $title;
		}
	}

	public function clearBadLink( $title ) {
		unset( $this->mBadLinks[$title] );
	}

	/* obsolete, for old $wgLinkCacheMemcached stuff */
	public function clearLink( $title ) {}

	public function getPageLinks() { return $this->mPageLinks; }
	public function getGoodLinks() { return $this->mGoodLinks; }
	public function getBadLinks() { return array_keys( $this->mBadLinks ); }

	/**
	 * Add a title to the link cache, return the page_id or zero if non-existent
	 * @param $title String: title to add
	 * @param $len int, page size
	 * @param $redir bool, is redirect?
	 * @return integer
	 */
	public function addLink( $title, $len = -1, $redir = NULL ) {
		$nt = Title::newFromDBkey( $title );
		if( $nt ) {
			return $this->addLinkObj( $nt, $len, $redir );
		} else {
			return 0;
		}
	}

	/**
	 * Add a title to the link cache, return the page_id or zero if non-existent
	 * @param $nt Title to add.
	 * @param $len int, page size
	 * @param $redir bool, is redirect?
	 * @return integer
	 */
	public function addLinkObj( &$nt, $len = -1, $redirect = NULL ) {
		global $wgAntiLockFlags, $wgProfiler;

		$title = $nt->getPrefixedDBkey();
		if ( $this->isBadLink( $title ) ) { return 0; }
		$id = $this->getGoodLinkID( $title );
		if ( 0 != $id ) { return $id; }

		$fname = 'LinkCache::addLinkObj';
		if ( isset( $wgProfiler ) ) {
			$fname .= ' (' . $wgProfiler->getCurrentSection() . ')';
		}

		wfProfileIn( $fname );

		$ns = $nt->getNamespace();
		$t = $nt->getDBkey();

		if ( '' == $title ) {
			wfProfileOut( $fname );
			return 0;
		}

		# Some fields heavily used for linking...
		if ( $this->mForUpdate ) {
			$db = wfGetDB( DB_MASTER );
			if ( !( $wgAntiLockFlags & ALF_NO_LINK_LOCK ) ) {
				$options = array( 'FOR UPDATE' );
			} else {
				$options = array();
			}
		} else {
			$db = wfGetDB( DB_SLAVE );
			$options = array();
		}

		$s = $db->selectRow( 'page', 
			array( 'page_id', 'page_len', 'page_is_redirect' ),
			array( 'page_namespace' => $ns, 'page_title' => $t ),
			$fname, $options );
		# Set fields...
		$id = $s ? $s->page_id : 0;
		$len = $s ? $s->page_len : -1;
		$redirect = $s ? $s->page_is_redirect : 0;

		if( 0 == $id ) {
			$this->addBadLinkObj( $nt );
		} else {
			$this->addGoodLinkObj( $id, $nt, $len, $redirect );
		}
		wfProfileOut( $fname );
		return $id;
	}

	/**
	 * Clears cache
	 */
	public function clear() {
		$this->mPageLinks = array();
		$this->mGoodLinks = array();
		$this->mGoodLinkFields = array();
		$this->mBadLinks = array();
	}
}
