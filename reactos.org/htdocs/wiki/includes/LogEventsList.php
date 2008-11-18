<?php
# Copyright (C) 2004 Brion Vibber <brion@pobox.com>, 2008 Aaron Schulz
# http://www.mediawiki.org/
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
# http://www.gnu.org/copyleft/gpl.html

class LogEventsList {
	const NO_ACTION_LINK = 1;

	private $skin;
	private $out;
	public $flags;

	function __construct( $skin, $out, $flags = 0 ) {
		$this->skin = $skin;
		$this->out = $out;
		$this->flags = $flags;
		$this->preCacheMessages();
	}

	/**
	 * As we use the same small set of messages in various methods and that
	 * they are called often, we call them once and save them in $this->message
	 */
	private function preCacheMessages() {
		// Precache various messages
		if( !isset( $this->message ) ) {
			$messages = 'revertmerge protect_change unblocklink revertmove undeletelink revdel-restore rev-delundel';
			foreach( explode(' ', $messages ) as $msg ) {
				$this->message[$msg] = wfMsgExt( $msg, array( 'escape') );
			}
		}
	}

	/**
	 * Set page title and show header for this log type
	 * @param string $type
	 */
	public function showHeader( $type ) {
		if( LogPage::isLogType( $type ) ) {
			$this->out->setPageTitle( LogPage::logName( $type ) );
			$this->out->addHtml( LogPage::logHeader( $type ) );
		}
	}

	/**
	 * Show options for the log list
	 * @param string $type,
	 * @param string $user,
	 * @param string $page,
	 * @param string $pattern
	 * @param int $year
	 * @parm int $month
	 */
	public function showOptions( $type='', $user='', $page='', $pattern='', $year='', $month='' ) {
		global $wgScript, $wgMiserMode;
		$action = htmlspecialchars( $wgScript );
		$title = SpecialPage::getTitleFor( 'Log' );
		$special = htmlspecialchars( $title->getPrefixedDBkey() );

		$this->out->addHTML( "<form action=\"$action\" method=\"get\"><fieldset>" .
			Xml::element( 'legend', array(), wfMsg( 'log' ) ) .
			Xml::hidden( 'title', $special ) . "\n" .
			$this->getTypeMenu( $type ) . "\n" .
			$this->getUserInput( $user ) . "\n" .
			$this->getTitleInput( $page ) . "\n" .
			( !$wgMiserMode ? ($this->getTitlePattern( $pattern )."\n") : "" ) .
			"<p>" . $this->getDateMenu( $year, $month ) . "\n" .
			Xml::submitButton( wfMsg( 'allpagessubmit' ) ) . "</p>\n" .
			"</fieldset></form>" );
	}

	/**
	 * @return string Formatted HTML
	 * @param string $queryType
	 */
	private function getTypeMenu( $queryType ) {
		global $wgLogRestrictions, $wgUser;

		$html = "<select name='type'>\n";

		$validTypes = LogPage::validTypes();
		$m = array(); // Temporary array

		// First pass to load the log names
		foreach( $validTypes as $type ) {
			$text = LogPage::logName( $type );
			$m[$text] = $type;
		}

		// Second pass to sort by name
		ksort($m);

		// Third pass generates sorted XHTML content
		foreach( $m as $text => $type ) {
			$selected = ($type == $queryType);
			// Restricted types
			if ( isset($wgLogRestrictions[$type]) ) {
				if ( $wgUser->isAllowed( $wgLogRestrictions[$type] ) ) {
					$html .= Xml::option( $text, $type, $selected ) . "\n";
				}
			} else {
				$html .= Xml::option( $text, $type, $selected ) . "\n";
			}
		}

		$html .= '</select>';
		return $html;
	}

	/**
	 * @return string Formatted HTML
	 * @param string $user
	 */
	private function getUserInput( $user ) {
		return Xml::inputLabel( wfMsg( 'specialloguserlabel' ), 'user', 'user', 15, $user );
	}

	/**
	 * @return string Formatted HTML
	 * @param string $title
	 */
	private function getTitleInput( $title ) {
		return Xml::inputLabel( wfMsg( 'speciallogtitlelabel' ), 'page', 'page', 20, $title );
	}

	/**
	 * @return string Formatted HTML
	 * @param int $year
	 * @param int $month
	 */
	private function getDateMenu( $year, $month ) {
		# Offset overrides year/month selection
		if( $month && $month !== -1 ) {
			$encMonth = intval( $month );
		} else {
			$encMonth = '';
		}
		if ( $year ) {
			$encYear = intval( $year );
		} else if( $encMonth ) {
			$thisMonth = intval( gmdate( 'n' ) );
			$thisYear = intval( gmdate( 'Y' ) );
			if( intval($encMonth) > $thisMonth ) {
				$thisYear--;
			}
			$encYear = $thisYear;
		} else {
			$encYear = '';
		}
		return Xml::label( wfMsg( 'year' ), 'year' ) . ' '.
			Xml::input( 'year', 4, $encYear, array('id' => 'year', 'maxlength' => 4) ) .
			' '.
			Xml::label( wfMsg( 'month' ), 'month' ) . ' '.
			Xml::monthSelector( $encMonth, -1 );
	}

	/**
	 * @return boolean Checkbox
	 */
	private function getTitlePattern( $pattern ) {
		return '<span style="white-space: nowrap">' .
			Xml::checkLabel( wfMsg( 'log-title-wildcard' ), 'pattern', 'pattern', $pattern ) .
			'</span>';
	}

	public function beginLogEventsList() {
		return "<ul>\n";
	}

	public function endLogEventsList() {
		return "</ul>\n";
	}

		/**
	 * @param Row $row a single row from the result set
	 * @return string Formatted HTML list item
	 * @private
	 */
	public function logLine( $row ) {
		global $wgLang, $wgUser, $wgContLang;

		$title = Title::makeTitle( $row->log_namespace, $row->log_title );
		$time = $wgLang->timeanddate( wfTimestamp(TS_MW, $row->log_timestamp), true );
		// User links
		if( self::isDeleted($row,LogPage::DELETED_USER) ) {
			$userLink = '<span class="history-deleted">' . wfMsgHtml( 'rev-deleted-user' ) . '</span>';
		} else {
			$userLink = $this->skin->userLink( $row->log_user, $row->user_name ) .
				$this->skin->userToolLinks( $row->log_user, $row->user_name, true, 0, $row->user_editcount );
		}
		// Comment
		if( self::isDeleted($row,LogPage::DELETED_COMMENT) ) {
			$comment = '<span class="history-deleted">' . wfMsgHtml('rev-deleted-comment') . '</span>';
		} else {
			$comment = $wgContLang->getDirMark() . $this->skin->commentBlock( $row->log_comment );
		}
		// Extract extra parameters
		$paramArray = LogPage::extractParams( $row->log_params );
		$revert = $del = '';
		// Some user can hide log items and have review links
		if( $wgUser->isAllowed( 'deleterevision' ) ) {
			$del = $this->showhideLinks( $row ) . ' ';
		}
		// Add review links and such...
		if( !($this->flags & self::NO_ACTION_LINK) && !($row->log_deleted & LogPage::DELETED_ACTION) ) {
			if( self::typeAction($row,'move','move') && isset( $paramArray[0] ) && $wgUser->isAllowed( 'move' ) ) {
				$destTitle = Title::newFromText( $paramArray[0] );
				if( $destTitle ) {
					$revert = '(' . $this->skin->makeKnownLinkObj( SpecialPage::getTitleFor( 'Movepage' ),
						$this->message['revertmove'],
						'wpOldTitle=' . urlencode( $destTitle->getPrefixedDBkey() ) .
						'&wpNewTitle=' . urlencode( $title->getPrefixedDBkey() ) .
						'&wpReason=' . urlencode( wfMsgForContent( 'revertmove' ) ) .
						'&wpMovetalk=0' ) . ')';
				}
			// Show undelete link
			} else if( self::typeAction($row,array('delete','suppress'),'delete') && $wgUser->isAllowed( 'delete' ) ) {
				$revert = '(' . $this->skin->makeKnownLinkObj( SpecialPage::getTitleFor( 'Undelete' ),
					$this->message['undeletelink'], 'target='. urlencode( $title->getPrefixedDBkey() ) ) . ')';
			// Show unblock link
			} else if( self::typeAction($row,array('block','suppress'),'block') && $wgUser->isAllowed( 'block' ) ) {
				$revert = '(' .  $this->skin->makeKnownLinkObj( SpecialPage::getTitleFor( 'Ipblocklist' ),
					$this->message['unblocklink'],
					'action=unblock&ip=' . urlencode( $row->log_title ) ) . ')';
			// Show change protection link
			} else if( self::typeAction($row,'protect','modify') && $wgUser->isAllowed( 'protect' ) ) {
				$revert = '(' .  $this->skin->makeKnownLinkObj( $title, $this->message['protect_change'], 'action=unprotect' ) . ')';
			// Show unmerge link
			} else if ( self::typeAction($row,'merge','merge') ) {
				$merge = SpecialPage::getTitleFor( 'Mergehistory' );
				$revert = '(' .  $this->skin->makeKnownLinkObj( $merge, $this->message['revertmerge'],
					wfArrayToCGI( array('target' => $paramArray[0], 'dest' => $title->getPrefixedDBkey(), 
						'mergepoint' => $paramArray[1] ) ) ) . ')';
			// If an edit was hidden from a page give a review link to the history
			} else if( self::typeAction($row,array('delete','suppress'),'revision') && $wgUser->isAllowed( 'deleterevision' ) ) {
				if( count($paramArray) == 2 ) {
					$revdel = SpecialPage::getTitleFor( 'Revisiondelete' );
					// Different revision types use different URL params...
					$key = $paramArray[0];
					// Link to each hidden object ID, $paramArray[1] is the url param
					$Ids = explode( ',', $paramArray[1] );
					$revParams = '';
					foreach( $Ids as $n => $id ) {
						$revParams .= '&' . urlencode($key) . '[]=' . urlencode($id);
					}
					$revert = '(' . $this->skin->makeKnownLinkObj( $revdel, $this->message['revdel-restore'], 
						'target=' . $title->getPrefixedUrl() . $revParams ) . ')';
				}
			// Hidden log items, give review link
			} else if( self::typeAction($row,array('delete','suppress'),'event') && $wgUser->isAllowed( 'deleterevision' ) ) {
				if( count($paramArray) == 1 ) {
					$revdel = SpecialPage::getTitleFor( 'Revisiondelete' );
					$Ids = explode( ',', $paramArray[0] );
					// Link to each hidden object ID, $paramArray[1] is the url param
					$logParams = '';
					foreach( $Ids as $n => $id ) {
						$logParams .= '&logid[]=' . intval($id);
					}
					$revert = '(' . $this->skin->makeKnownLinkObj( $revdel, $this->message['revdel-restore'], 
						'target=' . $title->getPrefixedUrl() . $logParams ) . ')';
				}
			} else {
				wfRunHooks( 'LogLine', array( $row->log_type, $row->log_action, $title, $paramArray,
					&$comment, &$revert, $row->log_timestamp ) );
				// wfDebug( "Invoked LogLine hook for " $row->log_type . ", " . $row->log_action . "\n" );
				// Do nothing. The implementation is handled by the hook modifiying the passed-by-ref parameters.
			}
		}
		// Event description
		if( self::isDeleted($row,LogPage::DELETED_ACTION) ) {
			$action = '<span class="history-deleted">' . wfMsgHtml('rev-deleted-event') . '</span>';
		} else {
			$action = LogPage::actionText( $row->log_type, $row->log_action, $title, $this->skin, $paramArray, true );
		}

		return "<li>$del$time $userLink $action $comment $revert</li>\n";
	}

	/**
	 * @param Row $row
	 * @return string
	 */
	private function showhideLinks( $row ) {
		$revdel = SpecialPage::getTitleFor( 'Revisiondelete' );
		// If event was hidden from sysops
		if( !self::userCan( $row, LogPage::DELETED_RESTRICTED ) ) {
			$del = $this->message['rev-delundel'];
		} else if( $row->log_type == 'suppress' ) {
			// No one should be hiding from the oversight log
			$del = $this->message['rev-delundel'];
		} else {
			$target = SpecialPage::getTitleFor( 'Log', $row->log_type );
			$del = $this->skin->makeKnownLinkObj( $revdel, $this->message['rev-delundel'],
				'target=' . $target->getPrefixedUrl() . '&logid='.$row->log_id );
			// Bolden oversighted content
			if( self::isDeleted( $row, LogPage::DELETED_RESTRICTED ) )
				$del = "<strong>$del</strong>";
		}
		return "<tt>(<small>$del</small>)</tt>";
	}

	/**
	 * @param Row $row
	 * @param mixed $type (string/array)
	 * @param string $action
	 * @return bool
	 */
	public static function typeAction( $row, $type, $action ) {
		if( is_array($type) ) {
			return ( in_array($row->log_type,$type) && $row->log_action == $action );
		} else {
			return ( $row->log_type == $type && $row->log_action == $action );
		}
	}

	/**
	 * Determine if the current user is allowed to view a particular
	 * field of this log row, if it's marked as deleted.
	 * @param Row $row
	 * @param int $field
	 * @return bool
	 */
	public static function userCan( $row, $field ) {
		if( ( $row->log_deleted & $field ) == $field ) {
			global $wgUser;
			$permission = ( $row->log_deleted & LogPage::DELETED_RESTRICTED ) == LogPage::DELETED_RESTRICTED
				? 'suppressrevision'
				: 'deleterevision';
			wfDebug( "Checking for $permission due to $field match on $row->log_deleted\n" );
			return $wgUser->isAllowed( $permission );
		} else {
			return true;
		}
	}

	/**
	 * @param Row $row
	 * @param int $field one of DELETED_* bitfield constants
	 * @return bool
	 */
	public static function isDeleted( $row, $field ) {
		return ($row->log_deleted & $field) == $field;
	}

	/**
	 * Quick function to show a short log extract
	 * @param OutputPage $out
	 * @param string $type
	 * @param string $page
	 * @param string $user
	 */
	 public static function showLogExtract( $out, $type='', $page='', $user='' ) {
		global $wgUser;
		# Insert list of top 50 or so items
		$loglist = new LogEventsList( $wgUser->getSkin(), $out, 0 );
		$pager = new LogPager( $loglist, $type, $user, $page, '' );
		$logBody = $pager->getBody();
		if( $logBody ) {
			$out->addHTML(
				$loglist->beginLogEventsList() .
				$logBody .
				$loglist->endLogEventsList()
			);
		} else {
			$out->addWikiMsg( 'logempty' );
		}
	 }

	 /**
	 * SQL clause to skip forbidden log types for this user
	 * @param Database $db
	 * @returns mixed (string or false)
	 */
	public static function getExcludeClause( $db ) {
		global $wgLogRestrictions, $wgUser;
		// Reset the array, clears extra "where" clauses when $par is used
		$hiddenLogs = array();
		// Don't show private logs to unprivileged users
		foreach( $wgLogRestrictions as $logtype => $right ) {
			if( !$wgUser->isAllowed($right) ) {
				$safetype = $db->strencode( $logtype );
				$hiddenLogs[] = $safetype;
			}
		}
		if( count($hiddenLogs) == 1 ) {
			return 'log_type != ' . $db->addQuotes( $hiddenLogs[0] );
		} elseif( !empty( $hiddenLogs ) ) {
			return 'log_type NOT IN (' . $db->makeList($hiddenLogs) . ')';
		}
		return false;
	}
}

/**
 * @ingroup Pager
 */
class LogPager extends ReverseChronologicalPager {
	private $type = '', $user = '', $title = '', $pattern = '', $year = '', $month = '';
	public $mLogEventsList;
	/**
	* constructor
	* @param LogEventsList $loglist,
	* @param string $type,
	* @param string $user,
	* @param string $page,
	* @param string $pattern
	* @param array $conds
	*/
	function __construct( $list, $type='', $user='', $title='', $pattern='', $conds=array(), $y=false, $m=false ) {
		parent::__construct();
		$this->mConds = $conds;

		$this->mLogEventsList = $list;

		$this->limitType( $type );
		$this->limitUser( $user );
		$this->limitTitle( $title, $pattern );
		$this->limitDate( $y, $m );
	}

	function getDefaultQuery() {
		$query = parent::getDefaultQuery();
		$query['type'] = $this->type;
		$query['month'] = $this->month;
		$query['year'] = $this->year;
		return $query;
	}

	/**
	 * Set the log reader to return only entries of the given type.
	 * Type restrictions enforced here
	 * @param string $type A log type ('upload', 'delete', etc)
	 * @private
	 */
	private function limitType( $type ) {
		global $wgLogRestrictions, $wgUser;
		// Don't even show header for private logs; don't recognize it...
		if( isset($wgLogRestrictions[$type]) && !$wgUser->isAllowed($wgLogRestrictions[$type]) ) {
			$type = '';
		}
		// Don't show private logs to unpriviledged users
		$hideLogs = LogEventsList::getExcludeClause( $this->mDb );
		if( $hideLogs !== false ) {
			$this->mConds[] = $hideLogs;
		}
		if( empty($type) ) {
			return false;
		}
		$this->type = $type;
		$this->mConds['log_type'] = $type;
	}

	/**
	 * Set the log reader to return only entries by the given user.
	 * @param string $name (In)valid user name
	 * @private
	 */
	function limitUser( $name ) {
		if( $name == '' ) {
			return false;
		}
		$usertitle = Title::makeTitleSafe( NS_USER, $name );
		if( is_null($usertitle) ) {
			return false;
		}
		/* Fetch userid at first, if known, provides awesome query plan afterwards */
		$userid = User::idFromName( $name );
		if( !$userid ) {
			/* It should be nicer to abort query at all,
			   but for now it won't pass anywhere behind the optimizer */
			$this->mConds[] = "NULL";
		} else {
			$this->mConds['log_user'] = $userid;
			$this->user = $usertitle->getText();
		}
	}

	/**
	 * Set the log reader to return only entries affecting the given page.
	 * (For the block and rights logs, this is a user page.)
	 * @param string $page Title name as text
	 * @private
	 */
	function limitTitle( $page, $pattern ) {
		global $wgMiserMode;

		$title = Title::newFromText( $page );
		if( strlen($page) == 0 || !$title instanceof Title )
			return false;

		$this->title = $title->getPrefixedText();
		$ns = $title->getNamespace();
		# Using the (log_namespace, log_title, log_timestamp) index with a
		# range scan (LIKE) on the first two parts, instead of simple equality,
		# makes it unusable for sorting.  Sorted retrieval using another index
		# would be possible, but then we might have to scan arbitrarily many
		# nodes of that index. Therefore, we need to avoid this if $wgMiserMode
		# is on.
		#
		# This is not a problem with simple title matches, because then we can
		# use the page_time index.  That should have no more than a few hundred
		# log entries for even the busiest pages, so it can be safely scanned
		# in full to satisfy an impossible condition on user or similar.
		if( $pattern && !$wgMiserMode ) {
			# use escapeLike to avoid expensive search patterns like 't%st%'
			$safetitle = $this->mDb->escapeLike( $title->getDBkey() );
			$this->mConds['log_namespace'] = $ns;
			$this->mConds[] = "log_title LIKE '$safetitle%'";
			$this->pattern = $pattern;
		} else {
			$this->mConds['log_namespace'] = $ns;
			$this->mConds['log_title'] = $title->getDBkey();
		}
	}

	/**
	 * Set the log reader to return only entries from given date.
	 * @param int $year
	 * @param int $month
	 * @private
	 */
	function limitDate( $year, $month ) {
		$year = intval($year);
		$month = intval($month);

		$this->year = ($year > 0 && $year < 10000) ? $year : '';
		$this->month = ($month > 0 && $month < 13) ? $month : '';

		if( $this->year || $this->month ) {
			// Assume this year if only a month is given
			if( $this->year ) {
				$year_start = $this->year;
			} else {
				$year_start = substr( wfTimestampNow(), 0, 4 );
				$thisMonth = gmdate( 'n' );
				if( $this->month > $thisMonth ) {
					// Future contributions aren't supposed to happen. :)
					$year_start--;
				}
			}

			if( $this->month ) {
				$month_end = str_pad($this->month + 1, 2, '0', STR_PAD_LEFT);
				$year_end = $year_start;
			} else {
				$month_end = 0;
				$year_end = $year_start + 1;
			}
			$ts_end = str_pad($year_end . $month_end, 14, '0' );

			$this->mOffset = $ts_end;
		}
	}

	function getQueryInfo() {
		$this->mConds[] = 'user_id = log_user';
		# Don't use the wrong logging index
		if( $this->title || $this->pattern || $this->user ) {
			$index = array( 'USE INDEX' => array( 'logging' => array('page_time','user_time') ) );
		} else if( $this->type ) {
			$index = array( 'USE INDEX' => array( 'logging' => 'type_time' ) );
		} else {
			$index = array( 'USE INDEX' => array( 'logging' => 'times' ) );
		}
		return array(
			'tables' => array( 'logging', 'user' ),
			'fields' => array( 'log_type', 'log_action', 'log_user', 'log_namespace', 'log_title', 'log_params',
				'log_comment', 'log_id', 'log_deleted', 'log_timestamp', 'user_name', 'user_editcount' ),
			'conds' => $this->mConds,
			'options' => $index
		);
	}

	function getIndexField() {
		return 'log_timestamp';
	}

	function getStartBody() {
		wfProfileIn( __METHOD__ );
		# Do a link batch query
		if( $this->getNumRows() > 0 ) {
			$lb = new LinkBatch;
			while( $row = $this->mResult->fetchObject() ) {
				$lb->add( $row->log_namespace, $row->log_title );
				$lb->addObj( Title::makeTitleSafe( NS_USER, $row->user_name ) );
				$lb->addObj( Title::makeTitleSafe( NS_USER_TALK, $row->user_name ) );
			}
			$lb->execute();
			$this->mResult->seek( 0 );
		}
		wfProfileOut( __METHOD__ );
		return '';
	}

	function formatRow( $row ) {
		return $this->mLogEventsList->logLine( $row );
	}

	public function getType() {
		return $this->type;
	}

	public function getUser() {
		return $this->user;
	}

	public function getPage() {
		return $this->title;
	}

	public function getPattern() {
		return $this->pattern;
	}

	public function getYear() {
		return $this->year;
	}

	public function getMonth() {
		return $this->month;
	}
}

/**
 * @deprecated
 * @ingroup SpecialPage
 */
class LogReader {
	var $pager;
	/**
	 * @param WebRequest $request For internal use use a FauxRequest object to pass arbitrary parameters.
	 */
	function __construct( $request ) {
		global $wgUser, $wgOut;
		# Get parameters
		$type = $request->getVal( 'type' );
		$user = $request->getText( 'user' );
		$title = $request->getText( 'page' );
		$pattern = $request->getBool( 'pattern' );
		$y = $request->getIntOrNull( 'year' );
		$m = $request->getIntOrNull( 'month' );
		# Don't let the user get stuck with a certain date
		$skip = $request->getText( 'offset' ) || $request->getText( 'dir' ) == 'prev';
		if( $skip ) {
			$y = '';
			$m = '';
		}
		# Use new list class to output results
		$loglist = new LogEventsList( $wgUser->getSkin(), $wgOut, 0 );
		$this->pager = new LogPager( $loglist, $type, $user, $title, $pattern, $y, $m );
	}

	/**
	* Is there at least one row?
	* @return bool
	*/
	public function hasRows() {
		return isset($this->pager) ? ($this->pager->getNumRows() > 0) : false;
	}
}

/**
 * @deprecated
 * @ingroup SpecialPage
 */
class LogViewer {
	const NO_ACTION_LINK = 1;
	/**
	 * @var LogReader $reader
	 */
	var $reader;
	/**
	 * @param LogReader &$reader where to get our data from
	 * @param integer $flags Bitwise combination of flags:
	 *     LogEventsList::NO_ACTION_LINK   Don't show restore/unblock/block links
	 */
	function __construct( &$reader, $flags = 0 ) {
		global $wgUser;
		$this->reader =& $reader;
		$this->reader->pager->mLogEventsList->flags = $flags;
		# Aliases for shorter code...
		$this->pager =& $this->reader->pager;
		$this->list =& $this->reader->pager->mLogEventsList;
	}

	/**
	 * Take over the whole output page in $wgOut with the log display.
	 */
	public function show() {
		# Set title and add header
		$this->list->showHeader( $pager->getType() );
		# Show form options
		$this->list->showOptions( $this->pager->getType(), $this->pager->getUser(), $this->pager->getPage(),
			$this->pager->getPattern(), $this->pager->getYear(), $this->pager->getMonth() );
		# Insert list
		$logBody = $this->pager->getBody();
		if( $logBody ) {
			$wgOut->addHTML(
				$this->pager->getNavigationBar() .
				$this->list->beginLogEventsList() .
				$logBody .
				$this->list->endLogEventsList() .
				$this->pager->getNavigationBar()
			);
		} else {
			$wgOut->addWikiMsg( 'logempty' );
		}
	}

	/**
	 * Output just the list of entries given by the linked LogReader,
	 * with extraneous UI elements. Use for displaying log fragments in
	 * another page (eg at Special:Undelete)
	 * @param OutputPage $out where to send output
	 */
	public function showList( &$out ) {
		$logBody = $this->pager->getBody();
		if( $logBody ) {
			$out->addHTML(
				$this->list->beginLogEventsList() .
				$logBody .
				$this->list->endLogEventsList()
			);
		} else {
			$out->addWikiMsg( 'logempty' );
		}
	}
}
