<?php
/**
 * @todo document
 * @file
 */

/**
 * @todo document
 */
class Revision {
	const DELETED_TEXT = 1;
	const DELETED_COMMENT = 2;
	const DELETED_USER = 4;
	const DELETED_RESTRICTED = 8;

	/**
	 * Load a page revision from a given revision ID number.
	 * Returns null if no such revision can be found.
	 *
	 * @param int $id
	 * @access public
	 * @static
	 */
	public static function newFromId( $id ) {
		return Revision::newFromConds(
			array( 'page_id=rev_page',
			       'rev_id' => intval( $id ) ) );
	}

	/**
	 * Load either the current, or a specified, revision
	 * that's attached to a given title. If not attached
	 * to that title, will return null.
	 *
	 * @param Title $title
	 * @param int $id
	 * @return Revision
	 */
	public static function newFromTitle( $title, $id = 0 ) {
		if( $id ) {
			$matchId = intval( $id );
		} else {
			$matchId = 'page_latest';
		}
		return Revision::newFromConds(
			array( "rev_id=$matchId",
			       'page_id=rev_page',
			       'page_namespace' => $title->getNamespace(),
			       'page_title'     => $title->getDBkey() ) );
	}

	/**
	 * Load a page revision from a given revision ID number.
	 * Returns null if no such revision can be found.
	 *
	 * @param Database $db
	 * @param int $id
	 * @access public
	 * @static
	 */
	public static function loadFromId( $db, $id ) {
		return Revision::loadFromConds( $db,
			array( 'page_id=rev_page',
			       'rev_id' => intval( $id ) ) );
	}

	/**
	 * Load either the current, or a specified, revision
	 * that's attached to a given page. If not attached
	 * to that page, will return null.
	 *
	 * @param Database $db
	 * @param int $pageid
	 * @param int $id
	 * @return Revision
	 * @access public
	 * @static
	 */
	public static function loadFromPageId( $db, $pageid, $id = 0 ) {
		$conds=array('page_id=rev_page','rev_page'=>intval( $pageid ), 'page_id'=>intval( $pageid ));
		if( $id ) {
			$conds['rev_id']=intval($id);
		} else {
			$conds[]='rev_id=page_latest';
		}
		return Revision::loadFromConds( $db, $conds );
	}

	/**
	 * Load either the current, or a specified, revision
	 * that's attached to a given page. If not attached
	 * to that page, will return null.
	 *
	 * @param Database $db
	 * @param Title $title
	 * @param int $id
	 * @return Revision
	 * @access public
	 * @static
	 */
	public static function loadFromTitle( $db, $title, $id = 0 ) {
		if( $id ) {
			$matchId = intval( $id );
		} else {
			$matchId = 'page_latest';
		}
		return Revision::loadFromConds(
			$db,
			array( "rev_id=$matchId",
			       'page_id=rev_page',
			       'page_namespace' => $title->getNamespace(),
			       'page_title'     => $title->getDBkey() ) );
	}

	/**
	 * Load the revision for the given title with the given timestamp.
	 * WARNING: Timestamps may in some circumstances not be unique,
	 * so this isn't the best key to use.
	 *
	 * @param Database $db
	 * @param Title $title
	 * @param string $timestamp
	 * @return Revision
	 * @access public
	 * @static
	 */
	public static function loadFromTimestamp( $db, $title, $timestamp ) {
		return Revision::loadFromConds(
			$db,
			array( 'rev_timestamp'  => $db->timestamp( $timestamp ),
			       'page_id=rev_page',
			       'page_namespace' => $title->getNamespace(),
			       'page_title'     => $title->getDBkey() ) );
	}

	/**
	 * Given a set of conditions, fetch a revision.
	 *
	 * @param array $conditions
	 * @return Revision
	 * @access private
	 * @static
	 */
	private static function newFromConds( $conditions ) {
		$db = wfGetDB( DB_SLAVE );
		$row = Revision::loadFromConds( $db, $conditions );
		if( is_null( $row ) ) {
			$dbw = wfGetDB( DB_MASTER );
			$row = Revision::loadFromConds( $dbw, $conditions );
		}
		return $row;
	}

	/**
	 * Given a set of conditions, fetch a revision from
	 * the given database connection.
	 *
	 * @param Database $db
	 * @param array $conditions
	 * @return Revision
	 * @access private
	 * @static
	 */
	private static function loadFromConds( $db, $conditions ) {
		$res = Revision::fetchFromConds( $db, $conditions );
		if( $res ) {
			$row = $res->fetchObject();
			$res->free();
			if( $row ) {
				$ret = new Revision( $row );
				return $ret;
			}
		}
		$ret = null;
		return $ret;
	}

	/**
	 * Return a wrapper for a series of database rows to
	 * fetch all of a given page's revisions in turn.
	 * Each row can be fed to the constructor to get objects.
	 *
	 * @param Title $title
	 * @return ResultWrapper
	 * @access public
	 * @static
	 */
	public static function fetchAllRevisions( $title ) {
		return Revision::fetchFromConds(
			wfGetDB( DB_SLAVE ),
			array( 'page_namespace' => $title->getNamespace(),
			       'page_title'     => $title->getDBkey(),
			       'page_id=rev_page' ) );
	}

	/**
	 * Return a wrapper for a series of database rows to
	 * fetch all of a given page's revisions in turn.
	 * Each row can be fed to the constructor to get objects.
	 *
	 * @param Title $title
	 * @return ResultWrapper
	 * @access public
	 * @static
	 */
	public static function fetchRevision( $title ) {
		return Revision::fetchFromConds(
			wfGetDB( DB_SLAVE ),
			array( 'rev_id=page_latest',
			       'page_namespace' => $title->getNamespace(),
			       'page_title'     => $title->getDBkey(),
			       'page_id=rev_page' ) );
	}

	/**
	 * Given a set of conditions, return a ResultWrapper
	 * which will return matching database rows with the
	 * fields necessary to build Revision objects.
	 *
	 * @param Database $db
	 * @param array $conditions
	 * @return ResultWrapper
	 * @access private
	 * @static
	 */
	private static function fetchFromConds( $db, $conditions ) {
		$fields = self::selectFields();
		$fields[] = 'page_namespace';
		$fields[] = 'page_title';
		$fields[] = 'page_latest';
		$res = $db->select(
			array( 'page', 'revision' ),
			$fields,
			$conditions,
			'Revision::fetchRow',
			array( 'LIMIT' => 1 ) );
		$ret = $db->resultObject( $res );
		return $ret;
	}

	/**
	 * Return the list of revision fields that should be selected to create
	 * a new revision.
	 */
	static function selectFields() {
		return array(
			'rev_id',
			'rev_page',
			'rev_text_id',
			'rev_timestamp',
			'rev_comment',
			'rev_user_text,'.
			'rev_user',
			'rev_minor_edit',
			'rev_deleted',
			'rev_len',
			'rev_parent_id'
		);
	}
	
	/**
	 * Return the list of text fields that should be selected to read the 
	 * revision text
	 */
	static function selectTextFields() {
		return array(
			'old_text',
			'old_flags'
		);
	}
	/**
	 * Return the list of page fields that should be selected from page table
	 */
	static function selectPageFields() {
		return array(
			'page_namespace',
			'page_title',
			'page_latest'
		);
	}

	/**
	 * @param object $row
	 * @access private
	 */
	function Revision( $row ) {
		if( is_object( $row ) ) {
			$this->mId        = intval( $row->rev_id );
			$this->mPage      = intval( $row->rev_page );
			$this->mTextId    = intval( $row->rev_text_id );
			$this->mComment   =         $row->rev_comment;
			$this->mUserText  =         $row->rev_user_text;
			$this->mUser      = intval( $row->rev_user );
			$this->mMinorEdit = intval( $row->rev_minor_edit );
			$this->mTimestamp =         $row->rev_timestamp;
			$this->mDeleted   = intval( $row->rev_deleted );

			if( !isset( $row->rev_parent_id ) )
				$this->mParentId = is_null($row->rev_parent_id) ? null : 0;
			else
				$this->mParentId  = intval( $row->rev_parent_id );

			if( !isset( $row->rev_len ) || is_null( $row->rev_len ) )
				$this->mSize = null;
			else
				$this->mSize = intval( $row->rev_len );

			if( isset( $row->page_latest ) ) {
				$this->mCurrent   = ( $row->rev_id == $row->page_latest );
				$this->mTitle     = Title::makeTitle( $row->page_namespace,
				                                      $row->page_title );
			} else {
				$this->mCurrent = false;
				$this->mTitle = null;
			}

			// Lazy extraction...
			$this->mText      = null;
			if( isset( $row->old_text ) ) {
				$this->mTextRow = $row;
			} else {
				// 'text' table row entry will be lazy-loaded
				$this->mTextRow = null;
			}
		} elseif( is_array( $row ) ) {
			// Build a new revision to be saved...
			global $wgUser;

			$this->mId        = isset( $row['id']         ) ? intval( $row['id']         ) : null;
			$this->mPage      = isset( $row['page']       ) ? intval( $row['page']       ) : null;
			$this->mTextId    = isset( $row['text_id']    ) ? intval( $row['text_id']    ) : null;
			$this->mUserText  = isset( $row['user_text']  ) ? strval( $row['user_text']  ) : $wgUser->getName();
			$this->mUser      = isset( $row['user']       ) ? intval( $row['user']       ) : $wgUser->getId();
			$this->mMinorEdit = isset( $row['minor_edit'] ) ? intval( $row['minor_edit'] ) : 0;
			$this->mTimestamp = isset( $row['timestamp']  ) ? strval( $row['timestamp']  ) : wfTimestamp( TS_MW );
			$this->mDeleted   = isset( $row['deleted']    ) ? intval( $row['deleted']    ) : 0;
			$this->mSize      = isset( $row['len']        ) ? intval( $row['len']        ) : null;
			$this->mParentId  = isset( $row['parent_id']  ) ? intval( $row['parent_id']  ) : null;

			// Enforce spacing trimming on supplied text
			$this->mComment   = isset( $row['comment']    ) ?  trim( strval( $row['comment'] ) ) : null;
			$this->mText      = isset( $row['text']       ) ? rtrim( strval( $row['text']    ) ) : null;
			$this->mTextRow   = null;

			$this->mTitle     = null; # Load on demand if needed
			$this->mCurrent   = false;
			# If we still have no len_size, see it we have the text to figure it out
			if ( !$this->mSize )
				$this->mSize      = is_null($this->mText) ? null : strlen($this->mText);
		} else {
			throw new MWException( 'Revision constructor passed invalid row format.' );
		}
	}

	/**#@+
	 * @access public
	 */

	/**
	 * Get revision ID
	 * @return int
	 */
	public function getId() {
		return $this->mId;
	}

	/**
	 * Get text row ID
	 * @return int
	 */
	public function getTextId() {
		return $this->mTextId;
	}

	/**
	 * Get parent revision ID (the original previous page revision)
	 * @return int
	 */
	public function getParentId() {
		return $this->mParentId;
	}

	/**
	 * Returns the length of the text in this revision, or null if unknown.
	 * @return int
	 */
	public function getSize() {
		return $this->mSize;
	}

	/**
	 * Returns the title of the page associated with this entry.
	 * @return Title
	 */
	public function getTitle() {
		if( isset( $this->mTitle ) ) {
			return $this->mTitle;
		}
		$dbr = wfGetDB( DB_SLAVE );
		$row = $dbr->selectRow(
			array( 'page', 'revision' ),
			array( 'page_namespace', 'page_title' ),
			array( 'page_id=rev_page',
			       'rev_id' => $this->mId ),
			'Revision::getTitle' );
		if( $row ) {
			$this->mTitle = Title::makeTitle( $row->page_namespace,
			                                  $row->page_title );
		}
		return $this->mTitle;
	}

	/**
	 * Set the title of the revision
	 * @param Title $title
	 */
	public function setTitle( $title ) {
		$this->mTitle = $title;
	}

	/**
	 * Get the page ID
	 * @return int
	 */
	public function getPage() {
		return $this->mPage;
	}

	/**
	 * Fetch revision's user id if it's available to all users
	 * @return int
	 */
	public function getUser() {
		if( $this->isDeleted( self::DELETED_USER ) ) {
			return 0;
		} else {
			return $this->mUser;
		}
	}

	/**
	 * Fetch revision's user id without regard for the current user's permissions
	 * @return string
	 */
	public function getRawUser() {
		return $this->mUser;
	}

	/**
	 * Fetch revision's username if it's available to all users
	 * @return string
	 */
	public function getUserText() {
		if( $this->isDeleted( self::DELETED_USER ) ) {
			return "";
		} else {
			return $this->mUserText;
		}
	}

	/**
	 * Fetch revision's username without regard for view restrictions
	 * @return string
	 */
	public function getRawUserText() {
		return $this->mUserText;
	}

	/**
	 * Fetch revision comment if it's available to all users
	 * @return string
	 */
	function getComment() {
		if( $this->isDeleted( self::DELETED_COMMENT ) ) {
			return "";
		} else {
			return $this->mComment;
		}
	}

	/**
	 * Fetch revision comment without regard for the current user's permissions
	 * @return string
	 */
	public function getRawComment() {
		return $this->mComment;
	}

	/**
	 * @return bool
	 */
	public function isMinor() {
		return (bool)$this->mMinorEdit;
	}

	/**
	 * int $field one of DELETED_* bitfield constants
	 * @return bool
	 */
	public function isDeleted( $field ) {
		return ($this->mDeleted & $field) == $field;
	}

	/**
	 * Fetch revision text if it's available to all users
	 * @return string
	 */
	public function getText() {
		if( $this->isDeleted( self::DELETED_TEXT ) ) {
			return "";
		} else {
			return $this->getRawText();
		}
	}

	/**
	 * Fetch revision text without regard for view restrictions
	 * @return string
	 */
	public function getRawText() {
		if( is_null( $this->mText ) ) {
			// Revision text is immutable. Load on demand:
			$this->mText = $this->loadText();
		}
		return $this->mText;
	}

	/**
	 * Fetch revision text if it's available to THIS user
	 * @return string
	 */
	public function revText() {
		if( !$this->userCan( self::DELETED_TEXT ) ) {
			return "";
		} else {
			return $this->getRawText();
		}
	}

	/**
	 * @return string
	 */
	public function getTimestamp() {
		return wfTimestamp(TS_MW, $this->mTimestamp);
	}

	/**
	 * @return bool
	 */
	public function isCurrent() {
		return $this->mCurrent;
	}

	/**
	 * Get previous revision for this title
	 * @return Revision
	 */
	public function getPrevious() {
		if( $this->getTitle() ) {
			$prev = $this->getTitle()->getPreviousRevisionID( $this->getId() );
			if( $prev ) {
				return Revision::newFromTitle( $this->getTitle(), $prev );
			}
		}
		return null;
	}

	/**
	 * @return Revision
	 */
	public function getNext() {
		if( $this->getTitle() ) {
			$next = $this->getTitle()->getNextRevisionID( $this->getId() );
			if ( $next ) {
				return Revision::newFromTitle( $this->getTitle(), $next );
			}
		}
		return null;
	}

	/**
	 * Get previous revision Id for this page_id
	 * This is used to populate rev_parent_id on save
	 * @param Database $db
	 * @return int
	 */
	private function getPreviousRevisionId( $db ) {
		if( is_null($this->mPage) ) {
			return 0;
		}
		# Use page_latest if ID is not given
		if( !$this->mId ) {
			$prevId = $db->selectField( 'page', 'page_latest',
				array( 'page_id' => $this->mPage ),
				__METHOD__ );
		} else {
			$prevId = $db->selectField( 'revision', 'rev_id',
				array( 'rev_page' => $this->mPage, 'rev_id < ' . $this->mId ),
				__METHOD__,
				array( 'ORDER BY' => 'rev_id DESC' ) );
		}
		return intval($prevId);
	}

	/**
	  * Get revision text associated with an old or archive row
	  * $row is usually an object from wfFetchRow(), both the flags and the text
	  * field must be included
	  *
	  * @param integer $row Id of a row
	  * @param string $prefix table prefix (default 'old_')
	  * @return string $text|false the text requested
	  */
	public static function getRevisionText( $row, $prefix = 'old_' ) {
		wfProfileIn( __METHOD__ );

		# Get data
		$textField = $prefix . 'text';
		$flagsField = $prefix . 'flags';

		if( isset( $row->$flagsField ) ) {
			$flags = explode( ',', $row->$flagsField );
		} else {
			$flags = array();
		}

		if( isset( $row->$textField ) ) {
			$text = $row->$textField;
		} else {
			wfProfileOut( __METHOD__ );
			return false;
		}

		# Use external methods for external objects, text in table is URL-only then
		if ( in_array( 'external', $flags ) ) {
			$url=$text;
			@list(/* $proto */,$path)=explode('://',$url,2);
			if ($path=="") {
				wfProfileOut( __METHOD__ );
				return false;
			}
			$text=ExternalStore::fetchFromURL($url);
		}

		// If the text was fetched without an error, convert it
		if ( $text !== false ) {
			if( in_array( 'gzip', $flags ) ) {
				# Deal with optional compression of archived pages.
				# This can be done periodically via maintenance/compressOld.php, and
				# as pages are saved if $wgCompressRevisions is set.
				$text = gzinflate( $text );
			}

			if( in_array( 'object', $flags ) ) {
				# Generic compressed storage
				$obj = unserialize( $text );
				if ( !is_object( $obj ) ) {
					// Invalid object
					wfProfileOut( __METHOD__ );
					return false;
				}
				$text = $obj->getText();
			}

			global $wgLegacyEncoding;
			if( $wgLegacyEncoding && !in_array( 'utf-8', $flags ) ) {
				# Old revisions kept around in a legacy encoding?
				# Upconvert on demand.
				global $wgInputEncoding, $wgContLang;
				$text = $wgContLang->iconv( $wgLegacyEncoding, $wgInputEncoding, $text );
			}
		}
		wfProfileOut( __METHOD__ );
		return $text;
	}

	/**
	 * If $wgCompressRevisions is enabled, we will compress data.
	 * The input string is modified in place.
	 * Return value is the flags field: contains 'gzip' if the
	 * data is compressed, and 'utf-8' if we're saving in UTF-8
	 * mode.
	 *
	 * @param mixed $text reference to a text
	 * @return string
	 */
	public static function compressRevisionText( &$text ) {
		global $wgCompressRevisions;
		$flags = array();

		# Revisions not marked this way will be converted
		# on load if $wgLegacyCharset is set in the future.
		$flags[] = 'utf-8';

		if( $wgCompressRevisions ) {
			if( function_exists( 'gzdeflate' ) ) {
				$text = gzdeflate( $text );
				$flags[] = 'gzip';
			} else {
				wfDebug( "Revision::compressRevisionText() -- no zlib support, not compressing\n" );
			}
		}
		return implode( ',', $flags );
	}

	/**
	 * Insert a new revision into the database, returning the new revision ID
	 * number on success and dies horribly on failure.
	 *
	 * @param Database $dbw
	 * @return int
	 */
	public function insertOn( $dbw ) {
		global $wgDefaultExternalStore;

		wfProfileIn( __METHOD__ );

		$data = $this->mText;
		$flags = Revision::compressRevisionText( $data );

		# Write to external storage if required
		if ( $wgDefaultExternalStore ) {
			if ( is_array( $wgDefaultExternalStore ) ) {
				// Distribute storage across multiple clusters
				$store = $wgDefaultExternalStore[mt_rand(0, count( $wgDefaultExternalStore ) - 1)];
			} else {
				$store = $wgDefaultExternalStore;
			}
			// Store and get the URL
			$data = ExternalStore::insert( $store, $data );
			if ( !$data ) {
				# This should only happen in the case of a configuration error, where the external store is not valid
				throw new MWException( "Unable to store text to external storage $store" );
			}
			if ( $flags ) {
				$flags .= ',';
			}
			$flags .= 'external';
		}

		# Record the text (or external storage URL) to the text table
		if( !isset( $this->mTextId ) ) {
			$old_id = $dbw->nextSequenceValue( 'text_old_id_val' );
			$dbw->insert( 'text',
				array(
					'old_id'    => $old_id,
					'old_text'  => $data,
					'old_flags' => $flags,
				), __METHOD__
			);
			$this->mTextId = $dbw->insertId();
		}

		# Record the edit in revisions
		$rev_id = isset( $this->mId )
			? $this->mId
			: $dbw->nextSequenceValue( 'rev_rev_id_val' );
		$dbw->insert( 'revision',
			array(
				'rev_id'         => $rev_id,
				'rev_page'       => $this->mPage,
				'rev_text_id'    => $this->mTextId,
				'rev_comment'    => $this->mComment,
				'rev_minor_edit' => $this->mMinorEdit ? 1 : 0,
				'rev_user'       => $this->mUser,
				'rev_user_text'  => $this->mUserText,
				'rev_timestamp'  => $dbw->timestamp( $this->mTimestamp ),
				'rev_deleted'    => $this->mDeleted,
				'rev_len'	     => $this->mSize,
				'rev_parent_id'  => $this->mParentId ? $this->mParentId : $this->getPreviousRevisionId( $dbw )
			), __METHOD__
		);

		$this->mId = !is_null($rev_id) ? $rev_id : $dbw->insertId();
		
		wfRunHooks( 'RevisionInsertComplete', array( &$this, $data, $flags ) );
		
		wfProfileOut( __METHOD__ );
		return $this->mId;
	}

	/**
	 * Lazy-load the revision's text.
	 * Currently hardcoded to the 'text' table storage engine.
	 *
	 * @return string
	 */
	private function loadText() {
		wfProfileIn( __METHOD__ );

		// Caching may be beneficial for massive use of external storage
		global $wgRevisionCacheExpiry, $wgMemc;
		$key = wfMemcKey( 'revisiontext', 'textid', $this->getTextId() );
		if( $wgRevisionCacheExpiry ) {
			$text = $wgMemc->get( $key );
			if( is_string( $text ) ) {
				wfProfileOut( __METHOD__ );
				return $text;
			}
		}

		// If we kept data for lazy extraction, use it now...
		if ( isset( $this->mTextRow ) ) {
			$row = $this->mTextRow;
			$this->mTextRow = null;
		} else {
			$row = null;
		}

		if( !$row ) {
			// Text data is immutable; check slaves first.
			$dbr = wfGetDB( DB_SLAVE );
			$row = $dbr->selectRow( 'text',
				array( 'old_text', 'old_flags' ),
				array( 'old_id' => $this->getTextId() ),
				__METHOD__ );
		}

		if( !$row ) {
			// Possible slave lag!
			$dbw = wfGetDB( DB_MASTER );
			$row = $dbw->selectRow( 'text',
				array( 'old_text', 'old_flags' ),
				array( 'old_id' => $this->getTextId() ),
				__METHOD__ );
		}

		$text = self::getRevisionText( $row );

		if( $wgRevisionCacheExpiry ) {
			$wgMemc->set( $key, $text, $wgRevisionCacheExpiry );
		}

		wfProfileOut( __METHOD__ );

		return $text;
	}

	/**
	 * Create a new null-revision for insertion into a page's
	 * history. This will not re-save the text, but simply refer
	 * to the text from the previous version.
	 *
	 * Such revisions can for instance identify page rename
	 * operations and other such meta-modifications.
	 *
	 * @param Database $dbw
	 * @param int      $pageId ID number of the page to read from
	 * @param string   $summary
	 * @param bool     $minor
	 * @return Revision
	 */
	public static function newNullRevision( $dbw, $pageId, $summary, $minor ) {
		wfProfileIn( __METHOD__ );

		$current = $dbw->selectRow(
			array( 'page', 'revision' ),
			array( 'page_latest', 'rev_text_id' ),
			array(
				'page_id' => $pageId,
				'page_latest=rev_id',
				),
			__METHOD__ );

		if( $current ) {
			$revision = new Revision( array(
				'page'       => $pageId,
				'comment'    => $summary,
				'minor_edit' => $minor,
				'text_id'    => $current->rev_text_id,
				'parent_id'  => $current->page_latest
				) );
		} else {
			$revision = null;
		}

		wfProfileOut( __METHOD__ );
		return $revision;
	}

	/**
	 * Determine if the current user is allowed to view a particular
	 * field of this revision, if it's marked as deleted.
	 * @param int $field one of self::DELETED_TEXT,
	 *                          self::DELETED_COMMENT,
	 *                          self::DELETED_USER
	 * @return bool
	 */
	public function userCan( $field ) {
		if( ( $this->mDeleted & $field ) == $field ) {
			global $wgUser;
			$permission = ( $this->mDeleted & self::DELETED_RESTRICTED ) == self::DELETED_RESTRICTED
				? 'suppressrevision'
				: 'deleterevision';
			wfDebug( "Checking for $permission due to $field match on $this->mDeleted\n" );
			return $wgUser->isAllowed( $permission );
		} else {
			return true;
		}
	}


	/**
	 * Get rev_timestamp from rev_id, without loading the rest of the row
	 * @param integer $id
	 * @param integer $pageid, optional
	 */
	static function getTimestampFromId( $id, $pageId = 0 ) {
		$dbr = wfGetDB( DB_SLAVE );
		$conds = array( 'rev_id' => $id );
		if( $pageId ) {
			$conds['rev_page'] = $pageId;
		}
		$timestamp = $dbr->selectField( 'revision', 'rev_timestamp', $conds, __METHOD__ );
		if ( $timestamp === false ) {
			# Not in slave, try master
			$dbw = wfGetDB( DB_MASTER );
			$timestamp = $dbw->selectField( 'revision', 'rev_timestamp', $conds, __METHOD__ );
		}
		return wfTimestamp( TS_MW, $timestamp );
	}

	/**
	 * Get count of revisions per page...not very efficient
	 * @param Database $db
	 * @param int $id, page id
	 */
	static function countByPageId( $db, $id ) {
		$row = $db->selectRow( 'revision', 'COUNT(*) AS revCount',
			array( 'rev_page' => $id ), __METHOD__ );
		if( $row ) {
			return $row->revCount;
		}
		return 0;
	}

	/**
	 * Get count of revisions per page...not very efficient
	 * @param Database $db
	 * @param Title $title
	 */
	static function countByTitle( $db, $title ) {
		$id = $title->getArticleId();
		if( $id ) {
			return Revision::countByPageId( $db, $id );
		}
		return 0;
	}
}

/**
 * Aliases for backwards compatibility with 1.6
 */
define( 'MW_REV_DELETED_TEXT', Revision::DELETED_TEXT );
define( 'MW_REV_DELETED_COMMENT', Revision::DELETED_COMMENT );
define( 'MW_REV_DELETED_USER', Revision::DELETED_USER );
define( 'MW_REV_DELETED_RESTRICTED', Revision::DELETED_RESTRICTED );
