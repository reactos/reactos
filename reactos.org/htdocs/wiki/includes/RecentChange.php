<?php

/**
 * Utility class for creating new RC entries
 * mAttribs:
 * 	rc_id           id of the row in the recentchanges table
 * 	rc_timestamp    time the entry was made
 * 	rc_cur_time     timestamp on the cur row
 * 	rc_namespace    namespace #
 * 	rc_title        non-prefixed db key
 * 	rc_type         is new entry, used to determine whether updating is necessary
 * 	rc_minor        is minor
 * 	rc_cur_id       page_id of associated page entry
 * 	rc_user	        user id who made the entry
 * 	rc_user_text    user name who made the entry
 * 	rc_comment      edit summary
 * 	rc_this_oldid   rev_id associated with this entry (or zero)
 * 	rc_last_oldid   rev_id associated with the entry before this one (or zero)
 * 	rc_bot          is bot, hidden
 * 	rc_ip           IP address of the user in dotted quad notation
 * 	rc_new          obsolete, use rc_type==RC_NEW
 * 	rc_patrolled    boolean whether or not someone has marked this edit as patrolled
 * 	rc_old_len	integer byte length of the text before the edit
 * 	rc_new_len	the same after the edit
  *	rc_deleted		partial deletion
 *	rc_logid		the log_id value for this log entry (or zero)
 *  rc_log_type		the log type (or null)
 *	rc_log_action	the log action (or null)
 *  rc_params		log params
 *
 * mExtra:
 * 	prefixedDBkey   prefixed db key, used by external app via msg queue
 * 	lastTimestamp   timestamp of previous entry, used in WHERE clause during update
 * 	lang            the interwiki prefix, automatically set in save()
 *  oldSize         text size before the change
 *  newSize         text size after the change
 *
 * temporary:		not stored in the database
 *      notificationtimestamp
 *      numberofWatchingusers
 *
 * @todo document functions and variables
 */
class RecentChange
{
	var $mAttribs = array(), $mExtra = array();
	var $mTitle = false, $mMovedToTitle = false;
	var $numberofWatchingusers = 0 ; # Dummy to prevent error message in SpecialRecentchangeslinked

	# Factory methods

	public static function newFromRow( $row )
	{
		$rc = new RecentChange;
		$rc->loadFromRow( $row );
		return $rc;
	}

	public static function newFromCurRow( $row )
	{
		$rc = new RecentChange;
		$rc->loadFromCurRow( $row );
		$rc->notificationtimestamp = false;
		$rc->numberofWatchingusers = false;
		return $rc;
	}

	/**
	 * Obtain the recent change with a given rc_id value
	 *
	 * @param $rcid rc_id value to retrieve
	 * @return RecentChange
	 */
	public static function newFromId( $rcid ) {
		$dbr = wfGetDB( DB_SLAVE );
		$res = $dbr->select( 'recentchanges', '*', array( 'rc_id' => $rcid ), __METHOD__ );
		if( $res && $dbr->numRows( $res ) > 0 ) {
			$row = $dbr->fetchObject( $res );
			$dbr->freeResult( $res );
			return self::newFromRow( $row );
		} else {
			return NULL;
		}
	}

	/**
	 * Find the first recent change matching some specific conditions
	 *
	 * @param array $conds Array of conditions
	 * @param mixed $fname Override the method name in profiling/logs
	 * @return RecentChange
	 */
	public static function newFromConds( $conds, $fname = false ) {
		if( $fname === false )
			$fname = __METHOD__;
		$dbr = wfGetDB( DB_SLAVE );
		$res = $dbr->select(
			'recentchanges',
			'*',
			$conds,
			$fname
		);
		if( $res instanceof ResultWrapper && $res->numRows() > 0 ) {
			$row = $res->fetchObject();
			$res->free();
			return self::newFromRow( $row );
		}
		return null;
	}

	# Accessors

	function setAttribs( $attribs )
	{
		$this->mAttribs = $attribs;
	}

	function setExtra( $extra )
	{
		$this->mExtra = $extra;
	}

	function &getTitle()
	{
		if ( $this->mTitle === false ) {
			$this->mTitle = Title::makeTitle( $this->mAttribs['rc_namespace'], $this->mAttribs['rc_title'] );
		}
		return $this->mTitle;
	}

	function getMovedToTitle()
	{
		if ( $this->mMovedToTitle === false ) {
			$this->mMovedToTitle = Title::makeTitle( $this->mAttribs['rc_moved_to_ns'],
				$this->mAttribs['rc_moved_to_title'] );
		}
		return $this->mMovedToTitle;
	}

	# Writes the data in this object to the database
	function save()
	{
		global $wgLocalInterwiki, $wgPutIPinRC, $wgRC2UDPAddress, 
		$wgRC2UDPPort, $wgRC2UDPPrefix, $wgRC2UDPOmitBots;
		$fname = 'RecentChange::save';

		$dbw = wfGetDB( DB_MASTER );
		if ( !is_array($this->mExtra) ) {
			$this->mExtra = array();
		}
		$this->mExtra['lang'] = $wgLocalInterwiki;

		if ( !$wgPutIPinRC ) {
			$this->mAttribs['rc_ip'] = '';
		}

		## If our database is strict about IP addresses, use NULL instead of an empty string
		if ( $dbw->strictIPs() and $this->mAttribs['rc_ip'] == '' ) {
			unset( $this->mAttribs['rc_ip'] );
		}

		# Fixup database timestamps
		$this->mAttribs['rc_timestamp'] = $dbw->timestamp($this->mAttribs['rc_timestamp']);
		$this->mAttribs['rc_cur_time'] = $dbw->timestamp($this->mAttribs['rc_cur_time']);
		$this->mAttribs['rc_id'] = $dbw->nextSequenceValue( 'rc_rc_id_seq' );

		## If we are using foreign keys, an entry of 0 for the page_id will fail, so use NULL
		if ( $dbw->cascadingDeletes() and $this->mAttribs['rc_cur_id']==0 ) {
			unset ( $this->mAttribs['rc_cur_id'] );
		}

		# Insert new row
		$dbw->insert( 'recentchanges', $this->mAttribs, $fname );

		# Set the ID
		$this->mAttribs['rc_id'] = $dbw->insertId();

		# Update old rows, if necessary
		if ( $this->mAttribs['rc_type'] == RC_EDIT ) {
			$lastTime = $this->mExtra['lastTimestamp'];
			#$now = $this->mAttribs['rc_timestamp'];
			#$curId = $this->mAttribs['rc_cur_id'];

			# Don't bother looking for entries that have probably
			# been purged, it just locks up the indexes needlessly.
			global $wgRCMaxAge;
			$age = time() - wfTimestamp( TS_UNIX, $lastTime );
			if( $age < $wgRCMaxAge ) {
				# live hack, will commit once tested - kate
				# Update rc_this_oldid for the entries which were current
				#
				#$oldid = $this->mAttribs['rc_last_oldid'];
				#$ns = $this->mAttribs['rc_namespace'];
				#$title = $this->mAttribs['rc_title'];
				#
				#$dbw->update( 'recentchanges',
				#	array( /* SET */
				#		'rc_this_oldid' => $oldid
				#	), array( /* WHERE */
				#		'rc_namespace' => $ns,
				#		'rc_title' => $title,
				#		'rc_timestamp' => $dbw->timestamp( $lastTime )
				#	), $fname
				#);
			}

			# Update rc_cur_time
			#$dbw->update( 'recentchanges', array( 'rc_cur_time' => $now ),
			#	array( 'rc_cur_id' => $curId ), $fname );
		}

		# Notify external application via UDP
		if ( $wgRC2UDPAddress && ( !$this->mAttribs['rc_bot'] || !$wgRC2UDPOmitBots ) ) {
			$conn = socket_create( AF_INET, SOCK_DGRAM, SOL_UDP );
			if ( $conn ) {
				$line = $wgRC2UDPPrefix . $this->getIRCLine();
				socket_sendto( $conn, $line, strlen($line), 0, $wgRC2UDPAddress, $wgRC2UDPPort );
				socket_close( $conn );
			}
		}

		# E-mail notifications
		global $wgUseEnotif, $wgShowUpdatedMarker, $wgUser;
		if( $wgUseEnotif || $wgShowUpdatedMarker ) {
			// Users
			if( $this->mAttribs['rc_user'] ) {
				$editor = ($wgUser->getId() == $this->mAttribs['rc_user']) ? 
					$wgUser : User::newFromID( $this->mAttribs['rc_user'] );
			// Anons
			} else {
				$editor = ($wgUser->getName() == $this->mAttribs['rc_user_text']) ? 
					$wgUser : User::newFromName( $this->mAttribs['rc_user_text'], false );
			}
			# FIXME: this would be better as an extension hook
			$enotif = new EmailNotification();
			$title = Title::makeTitle( $this->mAttribs['rc_namespace'], $this->mAttribs['rc_title'] );
			$enotif->notifyOnPageChange( $editor, $title,
				$this->mAttribs['rc_timestamp'],
				$this->mAttribs['rc_comment'],
				$this->mAttribs['rc_minor'],
				$this->mAttribs['rc_last_oldid'] );
		}

		# Notify extensions
		wfRunHooks( 'RecentChange_save', array( &$this ) );
	}

	/**
	 * Mark a given change as patrolled
	 *
	 * @param mixed $change RecentChange or corresponding rc_id
	 * @returns integer number of affected rows
	 */
	public static function markPatrolled( $change ) {
		$rcid = $change instanceof RecentChange
			? $change->mAttribs['rc_id']
			: $change;
		$dbw = wfGetDB( DB_MASTER );
		$dbw->update(
			'recentchanges',
			array(
				'rc_patrolled' => 1
			),
			array(
				'rc_id' => $rcid
			),
			__METHOD__
		);
		return $dbw->affectedRows();
	}

	# Makes an entry in the database corresponding to an edit
	public static function notifyEdit( $timestamp, &$title, $minor, &$user, $comment,
		$oldId, $lastTimestamp, $bot, $ip = '', $oldSize = 0, $newSize = 0,
		$newId = 0)
	{
		if ( !$ip ) {
			$ip = wfGetIP();
			if ( !$ip ) {
				$ip = '';
			}
		}

		$rc = new RecentChange;
		$rc->mAttribs = array(
			'rc_timestamp'	=> $timestamp,
			'rc_cur_time'	=> $timestamp,
			'rc_namespace'	=> $title->getNamespace(),
			'rc_title'	=> $title->getDBkey(),
			'rc_type'	=> RC_EDIT,
			'rc_minor'	=> $minor ? 1 : 0,
			'rc_cur_id'	=> $title->getArticleID(),
			'rc_user'	=> $user->getId(),
			'rc_user_text'	=> $user->getName(),
			'rc_comment'	=> $comment,
			'rc_this_oldid'	=> $newId,
			'rc_last_oldid'	=> $oldId,
			'rc_bot'	=> $bot ? 1 : 0,
			'rc_moved_to_ns'	=> 0,
			'rc_moved_to_title'	=> '',
			'rc_ip'		=> $ip,
			'rc_patrolled'	=> 0,
			'rc_new'	=> 0,  # obsolete
			'rc_old_len'	=> $oldSize,
			'rc_new_len'	=> $newSize,
			'rc_deleted'	=> 0,
			'rc_logid'		=> 0,
			'rc_log_type'	=> null,
			'rc_log_action'	=> '',
			'rc_params'		=> ''
		);

		$rc->mExtra =  array(
			'prefixedDBkey'	=> $title->getPrefixedDBkey(),
			'lastTimestamp' => $lastTimestamp,
			'oldSize'       => $oldSize,
			'newSize'       => $newSize,
		);
		$rc->save();
		return( $rc->mAttribs['rc_id'] );
	}

	/**
	 * Makes an entry in the database corresponding to page creation
	 * Note: the title object must be loaded with the new id using resetArticleID()
	 * @todo Document parameters and return
	 */
	public static function notifyNew( $timestamp, &$title, $minor, &$user, $comment, $bot,
	  $ip='', $size = 0, $newId = 0 )
	{
		if ( !$ip ) {
			$ip = wfGetIP();
			if ( !$ip ) {
				$ip = '';
			}
		}

		$rc = new RecentChange;
		$rc->mAttribs = array(
			'rc_timestamp'      => $timestamp,
			'rc_cur_time'       => $timestamp,
			'rc_namespace'      => $title->getNamespace(),
			'rc_title'          => $title->getDBkey(),
			'rc_type'           => RC_NEW,
			'rc_minor'          => $minor ? 1 : 0,
			'rc_cur_id'         => $title->getArticleID(),
			'rc_user'           => $user->getId(),
			'rc_user_text'      => $user->getName(),
			'rc_comment'        => $comment,
			'rc_this_oldid'     => $newId,
			'rc_last_oldid'     => 0,
			'rc_bot'            => $bot ? 1 : 0,
			'rc_moved_to_ns'    => 0,
			'rc_moved_to_title' => '',
			'rc_ip'             => $ip,
			'rc_patrolled'      => 0,
			'rc_new'	    	=> 1, # obsolete
			'rc_old_len'        => 0,
			'rc_new_len'	    => $size,
			'rc_deleted'		=> 0,
			'rc_logid'			=> 0,
			'rc_log_type'		=> null,
			'rc_log_action'		=> '',
			'rc_params'			=> ''
		);

		$rc->mExtra =  array(
			'prefixedDBkey'	=> $title->getPrefixedDBkey(),
			'lastTimestamp' => 0,
			'oldSize' => 0,
			'newSize' => $size
		);
		$rc->save();
		return( $rc->mAttribs['rc_id'] );
	}

	# Makes an entry in the database corresponding to a rename
	public static function notifyMove( $timestamp, &$oldTitle, &$newTitle, &$user, $comment, $ip='', $overRedir = false )
	{
		global $wgRequest;

		if ( !$ip ) {
			$ip = wfGetIP();
			if ( !$ip ) {
				$ip = '';
			}
		}

		$rc = new RecentChange;
		$rc->mAttribs = array(
			'rc_timestamp'	=> $timestamp,
			'rc_cur_time'	=> $timestamp,
			'rc_namespace'	=> $oldTitle->getNamespace(),
			'rc_title'	=> $oldTitle->getDBkey(),
			'rc_type'	=> $overRedir ? RC_MOVE_OVER_REDIRECT : RC_MOVE,
			'rc_minor'	=> 0,
			'rc_cur_id'	=> $oldTitle->getArticleID(),
			'rc_user'	=> $user->getId(),
			'rc_user_text'	=> $user->getName(),
			'rc_comment'	=> $comment,
			'rc_this_oldid'	=> 0,
			'rc_last_oldid'	=> 0,
			'rc_bot'	=> $user->isAllowed( 'bot' ) ? $wgRequest->getBool( 'bot' , true ) : 0,
			'rc_moved_to_ns'	=> $newTitle->getNamespace(),
			'rc_moved_to_title'	=> $newTitle->getDBkey(),
			'rc_ip'		=> $ip,
			'rc_new'	=> 0, # obsolete
			'rc_patrolled'	=> 1,
			'rc_old_len'	=> NULL,
			'rc_new_len'	=> NULL,
			'rc_deleted'	=> 0,
			'rc_logid'		=> 0, # notifyMove not used anymore
			'rc_log_type'	=> null,
			'rc_log_action'	=> '',
			'rc_params'		=> ''
		);

		$rc->mExtra = array(
			'prefixedDBkey'	=> $oldTitle->getPrefixedDBkey(),
			'lastTimestamp' => 0,
			'prefixedMoveTo'	=> $newTitle->getPrefixedDBkey()
		);
		$rc->save();
	}

	public static function notifyMoveToNew( $timestamp, &$oldTitle, &$newTitle, &$user, $comment, $ip='' ) {
		RecentChange::notifyMove( $timestamp, $oldTitle, $newTitle, $user, $comment, $ip, false );
	}

	public static function notifyMoveOverRedirect( $timestamp, &$oldTitle, &$newTitle, &$user, $comment, $ip='' ) {
		RecentChange::notifyMove( $timestamp, $oldTitle, $newTitle, $user, $comment, $ip, true );
	}

	# A log entry is different to an edit in that previous revisions are not kept
	public static function notifyLog( $timestamp, &$title, &$user, $actionComment, $ip='',
	   $type, $action, $target, $logComment, $params, $newId=0 )
	{
		global $wgRequest;

		if ( !$ip ) {
			$ip = wfGetIP();
			if ( !$ip ) {
				$ip = '';
			}
		}

		$rc = new RecentChange;
		$rc->mAttribs = array(
			'rc_timestamp'	=> $timestamp,
			'rc_cur_time'	=> $timestamp,
			'rc_namespace'	=> $target->getNamespace(),
			'rc_title'	=> $target->getDBkey(),
			'rc_type'	=> RC_LOG,
			'rc_minor'	=> 0,
			'rc_cur_id'	=> $target->getArticleID(),
			'rc_user'	=> $user->getId(),
			'rc_user_text'	=> $user->getName(),
			'rc_comment'	=> $logComment,
			'rc_this_oldid'	=> 0,
			'rc_last_oldid'	=> 0,
			'rc_bot'	=> $user->isAllowed( 'bot' ) ? $wgRequest->getBool( 'bot' , true ) : 0,
			'rc_moved_to_ns'	=> 0,
			'rc_moved_to_title'	=> '',
			'rc_ip'	=> $ip,
			'rc_patrolled' => 1,
			'rc_new'	=> 0, # obsolete
			'rc_old_len'	=> NULL,
			'rc_new_len'	=> NULL,
			'rc_deleted'	=> 0,
			'rc_logid'		=> $newId,
			'rc_log_type'	=> $type,
			'rc_log_action'	=> $action,
			'rc_params'		=> $params
		);
		$rc->mExtra =  array(
			'prefixedDBkey'	=> $title->getPrefixedDBkey(),
			'lastTimestamp' => 0,
			'actionComment' => $actionComment, // the comment appended to the action, passed from LogPage
		);
		$rc->save();
	}

	# Initialises the members of this object from a mysql row object
	function loadFromRow( $row )
	{
		$this->mAttribs = get_object_vars( $row );
		$this->mAttribs["rc_timestamp"] = wfTimestamp(TS_MW, $this->mAttribs["rc_timestamp"]);
		$this->mExtra = array();
	}

	# Makes a pseudo-RC entry from a cur row
	function loadFromCurRow( $row )
	{
		$this->mAttribs = array(
			'rc_timestamp' => wfTimestamp(TS_MW, $row->rev_timestamp),
			'rc_cur_time' => $row->rev_timestamp,
			'rc_user' => $row->rev_user,
			'rc_user_text' => $row->rev_user_text,
			'rc_namespace' => $row->page_namespace,
			'rc_title' => $row->page_title,
			'rc_comment' => $row->rev_comment,
			'rc_minor' => $row->rev_minor_edit ? 1 : 0,
			'rc_type' => $row->page_is_new ? RC_NEW : RC_EDIT,
			'rc_cur_id' => $row->page_id,
			'rc_this_oldid'	=> $row->rev_id,
			'rc_last_oldid'	=> isset($row->rc_last_oldid) ? $row->rc_last_oldid : 0,
			'rc_bot'	=> 0,
			'rc_moved_to_ns'	=> 0,
			'rc_moved_to_title'	=> '',
			'rc_ip' => '',
			'rc_id' => $row->rc_id,
			'rc_patrolled' => $row->rc_patrolled,
			'rc_new' => $row->page_is_new, # obsolete
			'rc_old_len' => $row->rc_old_len,
			'rc_new_len' => $row->rc_new_len,
			'rc_params' => isset($row->rc_params) ? $row->rc_params : '',
			'rc_log_type' => isset($row->rc_log_type) ? $row->rc_log_type : null,
			'rc_log_action' => isset($row->rc_log_action) ? $row->rc_log_action : null,
			'rc_log_id' => isset($row->rc_log_id) ? $row->rc_log_id: 0,
			// this one REALLY should be set...
			'rc_deleted' => isset($row->rc_deleted) ? $row->rc_deleted: 0,
		);

		$this->mExtra = array();
	}

	/**
	 * Get an attribute value
	 *
	 * @param $name Attribute name
	 * @return mixed
	 */
	public function getAttribute( $name ) {
		return isset( $this->mAttribs[$name] ) ? $this->mAttribs[$name] : NULL;
	}

	/**
	 * Gets the end part of the diff URL associated with this object
	 * Blank if no diff link should be displayed
	 */
	function diffLinkTrail( $forceCur )
	{
		if ( $this->mAttribs['rc_type'] == RC_EDIT ) {
			$trail = "curid=" . (int)($this->mAttribs['rc_cur_id']) .
				"&oldid=" . (int)($this->mAttribs['rc_last_oldid']);
			if ( $forceCur ) {
				$trail .= '&diff=0' ;
			} else {
				$trail .= '&diff=' . (int)($this->mAttribs['rc_this_oldid']);
			}
		} else {
			$trail = '';
		}
		return $trail;
	}

	function cleanupForIRC( $text ) {
		return str_replace(array("\n", "\r"), array("", ""), $text);
	}

	function getIRCLine() {
		global $wgUseRCPatrol;

		// FIXME: Would be good to replace these 2 extract() calls with something more explicit
		// e.g. list ($rc_type, $rc_id) = array_values ($this->mAttribs); [or something like that]
		extract($this->mAttribs);
		extract($this->mExtra);

		if ( $rc_type == RC_LOG ) {
			$titleObj = Title::newFromText( "Log/$rc_log_type", NS_SPECIAL );
		} else {
			$titleObj =& $this->getTitle();
		}
		$title = $titleObj->getPrefixedText();
		$title = $this->cleanupForIRC( $title );

		// FIXME: *HACK* these should be getFullURL(), hacked for SSL madness --brion 2005-12-26
		if ( $rc_type == RC_LOG ) {
			$url = '';
		} elseif ( $rc_new && $wgUseRCPatrol ) {
			$url = $titleObj->getInternalURL("rcid=$rc_id");
		} else if ( $rc_new ) {
			$url = $titleObj->getInternalURL();
		} else if ( $wgUseRCPatrol ) {
			$url = $titleObj->getInternalURL("diff=$rc_this_oldid&oldid=$rc_last_oldid&rcid=$rc_id");
		} else {
			$url = $titleObj->getInternalURL("diff=$rc_this_oldid&oldid=$rc_last_oldid");
		}

		if ( isset( $oldSize ) && isset( $newSize ) ) {
			$szdiff = $newSize - $oldSize;
			if ($szdiff < -500) {
				$szdiff = "\002$szdiff\002";
			} elseif ($szdiff >= 0) {
				$szdiff = '+' . $szdiff ;
			}
			$szdiff = '(' . $szdiff . ')' ;
		} else {
			$szdiff = '';
		}

		$user = $this->cleanupForIRC( $rc_user_text );

		if ( $rc_type == RC_LOG ) {
			$logTargetText = $this->getTitle()->getPrefixedText();
			$comment = $this->cleanupForIRC( str_replace($logTargetText,"\00302$logTargetText\00310",$actionComment) );
			$flag = $rc_log_action;
		} else {
			$comment = $this->cleanupForIRC( $rc_comment );
			$flag = ($rc_new ? "N" : "") . ($rc_minor ? "M" : "") . ($rc_bot ? "B" : "");
		}
		# see http://www.irssi.org/documentation/formats for some colour codes. prefix is \003,
		# no colour (\003) switches back to the term default
		$fullString = "\00314[[\00307$title\00314]]\0034 $flag\00310 " .
		              "\00302$url\003 \0035*\003 \00303$user\003 \0035*\003 $szdiff \00310$comment\003\n";
		return $fullString;
	}

	/**
	 * Returns the change size (HTML).
	 * The lengths can be given optionally.
	 */
	function getCharacterDifference( $old = 0, $new = 0 ) {
		global $wgRCChangedSizeThreshold, $wgLang;

		if( $old === 0 ) {
			$old = $this->mAttribs['rc_old_len'];
		}
		if( $new === 0 ) {
			$new = $this->mAttribs['rc_new_len'];
		}

		if( $old === NULL || $new === NULL ) {
			return '';
		}

		$szdiff = $new - $old;
		$formatedSize = wfMsgExt( 'rc-change-size', array( 'parsemag', 'escape'),
			$wgLang->formatNum($szdiff) );

		if( $szdiff < $wgRCChangedSizeThreshold ) {
			return '<strong class=\'mw-plusminus-neg\'>(' . $formatedSize . ')</strong>';
		} elseif( $szdiff === 0 ) {
			return '<span class=\'mw-plusminus-null\'>(' . $formatedSize . ')</span>';
		} elseif( $szdiff > 0 ) {
			return '<span class=\'mw-plusminus-pos\'>(+' . $formatedSize . ')</span>';
		} else {
			return '<span class=\'mw-plusminus-neg\'>(' . $formatedSize . ')</span>';
		}
	}
}
