<?php

class DoubleRedirectJob extends Job {
	var $reason, $redirTitle, $destTitleText;
	static $user;

	/** 
	 * Insert jobs into the job queue to fix redirects to the given title
	 * @param string $type The reason for the fix, see message double-redirect-fixed-<reason>
	 * @param Title $redirTitle The title which has changed, redirects pointing to this title are fixed
	 */
	public static function fixRedirects( $reason, $redirTitle, $destTitle = false ) {
		# Need to use the master to get the redirect table updated in the same transaction
		$dbw = wfGetDB( DB_MASTER );
		$res = $dbw->select( 
			array( 'redirect', 'page' ), 
			array( 'page_namespace', 'page_title' ), 
			array( 
				'page_id = rd_from',
				'rd_namespace' => $redirTitle->getNamespace(),
				'rd_title' => $redirTitle->getDBkey()
			), __METHOD__ );
		if ( !$res->numRows() ) {
			return;
		}
		$jobs = array();
		foreach ( $res as $row ) {
			$title = Title::makeTitle( $row->page_namespace, $row->page_title );
			if ( !$title ) {
				continue;
			}

			$jobs[] = new self( $title, array( 
				'reason' => $reason,
				'redirTitle' => $redirTitle->getPrefixedDBkey() ) );
			# Avoid excessive memory usage
			if ( count( $jobs ) > 10000 ) {
				Job::batchInsert( $jobs );
				$jobs = array();
			}
		}
		Job::batchInsert( $jobs );
	}
	function __construct( $title, $params = false, $id = 0 ) {
		parent::__construct( 'fixDoubleRedirect', $title, $params, $id );
		$this->reason = $params['reason'];
		$this->redirTitle = Title::newFromText( $params['redirTitle'] );
		$this->destTitleText = !empty( $params['destTitle'] ) ? $params['destTitle'] : '';
	}

	function run() {
		if ( !$this->redirTitle ) {
			$this->setLastError( 'Invalid title' );
			return false;
		}

		$targetRev = Revision::newFromTitle( $this->title );
		if ( !$targetRev ) {
			wfDebug( __METHOD__.": target redirect already deleted, ignoring\n" );
			return true;
		}
		$text = $targetRev->getText();
		$currentDest = Title::newFromRedirect( $text );
		if ( !$currentDest || !$currentDest->equals( $this->redirTitle ) ) {
			wfDebug( __METHOD__.": Redirect has changed since the job was queued\n" );
			return true;
		}

		# Check for a suppression tag (used e.g. in periodically archived discussions)
		$mw = MagicWord::get( 'staticredirect' );
		if ( $mw->match( $text ) ) {
			wfDebug( __METHOD__.": skipping: suppressed with __STATICREDIRECT__\n" );
			return true;
		}

		# Find the current final destination
		$newTitle = self::getFinalDestination( $this->redirTitle );
		if ( !$newTitle ) {
			wfDebug( __METHOD__.": skipping: single redirect, circular redirect or invalid redirect destination\n" );
			return true;
		}
		if ( $newTitle->equals( $this->redirTitle ) ) {
			# The redirect is already right, no need to change it
			# This can happen if the page was moved back (say after vandalism)
			wfDebug( __METHOD__.": skipping, already good\n" );
		}

		# Preserve fragment (bug 14904)
		$newTitle = Title::makeTitle( $newTitle->getNamespace(), $newTitle->getDBkey(), 
			$currentDest->getFragment() );

		# Fix the text
		# Remember that redirect pages can have categories, templates, etc.,
		# so the regex has to be fairly general
		$newText = preg_replace( '/ \[ \[  [^\]]*  \] \] /x', 
			'[[' . $newTitle->getFullText() . ']]',
			$text, 1 );

		if ( $newText === $text ) {
			$this->setLastError( 'Text unchanged???' );
			return false;
		}

		# Save it
		global $wgUser;
		$oldUser = $wgUser;
		$wgUser = $this->getUser();
		$article = new Article( $this->title );
		$reason = wfMsgForContent( 'double-redirect-fixed-' . $this->reason, 
			$this->redirTitle->getPrefixedText(), $newTitle->getPrefixedText() );
		$article->doEdit( $newText, $reason, EDIT_UPDATE | EDIT_SUPPRESS_RC );
		$wgUser = $oldUser;

		return true;
	}

	/**
	 * Get the final destination of a redirect
	 * Returns false if the specified title is not a redirect, or if it is a circular redirect
	 */
	public static function getFinalDestination( $title ) {
		$dbw = wfGetDB( DB_MASTER );

		$seenTitles = array(); # Circular redirect check
		$dest = false;

		while ( true ) {
			$titleText = $title->getPrefixedDBkey();
			if ( isset( $seenTitles[$titleText] ) ) {
				wfDebug( __METHOD__, "Circular redirect detected, aborting\n" );
				return false;
			}
			$seenTitles[$titleText] = true;

			$row = $dbw->selectRow( 
				array( 'redirect', 'page' ),
				array( 'rd_namespace', 'rd_title' ),
				array( 
					'rd_from=page_id',
					'page_namespace' => $title->getNamespace(),
					'page_title' => $title->getDBkey()
				), __METHOD__ );
			if ( !$row ) {
				# No redirect from here, chain terminates
				break;
			} else {
				$dest = $title = Title::makeTitle( $row->rd_namespace, $row->rd_title );
			}
		}
		return $dest;
	}

	/**
	 * Get a user object for doing edits, from a request-lifetime cache
	 */
	function getUser() {
		if ( !self::$user ) {
			self::$user = User::newFromName( wfMsgForContent( 'double-redirect-fixer' ), false );
			if ( !self::$user->isLoggedIn() ) {
				self::$user->addToDatabase();
			}
		}
		return self::$user;
	}
}

