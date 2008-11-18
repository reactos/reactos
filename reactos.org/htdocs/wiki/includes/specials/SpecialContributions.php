<?php
/**
 * Special:Contributions, show user contributions in a paged list
 * @file
 * @ingroup SpecialPage
 */

/**
 * Pager for Special:Contributions
 * @ingroup SpecialPage Pager
 */
class ContribsPager extends ReverseChronologicalPager {
	public $mDefaultDirection = true;
	var $messages, $target;
	var $namespace = '', $year = '', $month = '', $mDb;

	function __construct( $target, $namespace = false, $year = false, $month = false ) {
		parent::__construct();
		foreach( explode( ' ', 'uctop diff newarticle rollbacklink diff hist newpageletter minoreditletter' ) as $msg ) {
			$this->messages[$msg] = wfMsgExt( $msg, array( 'escape') );
		}
		$this->target = $target;
		$this->namespace = $namespace;

		$year = intval($year);
		$month = intval($month);

		$this->year = $year > 0 ? $year : false;
		$this->month = ($month > 0 && $month < 13) ? $month : false;
		$this->getDateCond();

		$this->mDb = wfGetDB( DB_SLAVE, 'contributions' );
	}

	function getDefaultQuery() {
		$query = parent::getDefaultQuery();
		$query['target'] = $this->target;
		$query['month'] = $this->month;
		$query['year'] = $this->year;
		return $query;
	}

	function getQueryInfo() {
		list( $index, $userCond ) = $this->getUserCond();
		$conds = array_merge( array('page_id=rev_page'), $userCond, $this->getNamespaceCond() );
		$queryInfo = array(
			'tables' => array( 'page', 'revision' ),
			'fields' => array(
				'page_namespace', 'page_title', 'page_is_new', 'page_latest', 'rev_id', 'rev_page',
				'rev_text_id', 'rev_timestamp', 'rev_comment', 'rev_minor_edit', 'rev_user',
				'rev_user_text', 'rev_parent_id', 'rev_deleted'
			),
			'conds' => $conds,
			'options' => array( 'USE INDEX' => array('revision' => $index) )
		);
		wfRunHooks( 'ContribsPager::getQueryInfo', array( &$this, &$queryInfo ) );
		return $queryInfo;
	}

	function getUserCond() {
		$condition = array();

		if ( $this->target == 'newbies' ) {
			$max = $this->mDb->selectField( 'user', 'max(user_id)', false, __METHOD__ );
			$condition[] = 'rev_user >' . (int)($max - $max / 100);
			$index = 'user_timestamp';
		} else {
			$condition['rev_user_text'] = $this->target;
			$index = 'usertext_timestamp';
		}
		return array( $index, $condition );
	}

	function getNamespaceCond() {
		if ( $this->namespace !== '' ) {
			return array( 'page_namespace' => (int)$this->namespace );
		} else {
			return array();
		}
	}

	function getDateCond() {
		// Given an optional year and month, we need to generate a timestamp
		// to use as "WHERE rev_timestamp <= result"
		// Examples: year = 2006 equals < 20070101 (+000000)
		// year=2005, month=1    equals < 20050201
		// year=2005, month=12   equals < 20060101

		if (!$this->year && !$this->month)
			return;

		if ( $this->year ) {
			$year = $this->year;
		}
		else {
			// If no year given, assume the current one
			$year = gmdate( 'Y' );
			// If this month hasn't happened yet this year, go back to last year's month
			if( $this->month > gmdate( 'n' ) ) {
				$year--;
			}
		}

		if ( $this->month ) {
			$month = $this->month + 1;
			// For December, we want January 1 of the next year
			if ($month > 12) {
				$month = 1;
				$year++;
			}
		}
		else {
			// No month implies we want up to the end of the year in question
			$month = 1;
			$year++;
		}

		if ($year > 2032)
			$year = 2032;
		$ymd = (int)sprintf( "%04d%02d01", $year, $month );

		// Y2K38 bug
		if ($ymd > 20320101)
			$ymd = 20320101;

		$this->mOffset = $this->mDb->timestamp( "${ymd}000000" );
	}

	function getIndexField() {
		return 'rev_timestamp';
	}

	function getStartBody() {
		return "<ul>\n";
	}

	function getEndBody() {
		return "</ul>\n";
	}

	/**
	 * Generates each row in the contributions list.
	 *
	 * Contributions which are marked "top" are currently on top of the history.
	 * For these contributions, a [rollback] link is shown for users with roll-
	 * back privileges. The rollback link restores the most recent version that
	 * was not written by the target user.
	 *
	 * @todo This would probably look a lot nicer in a table.
	 */
	function formatRow( $row ) {
		wfProfileIn( __METHOD__ );

		global $wgLang, $wgUser, $wgContLang;

		$sk = $this->getSkin();
		$rev = new Revision( $row );

		$page = Title::makeTitle( $row->page_namespace, $row->page_title );
		$link = $sk->makeKnownLinkObj( $page );
		$difftext = $topmarktext = '';
		if( $row->rev_id == $row->page_latest ) {
			$topmarktext .= '<strong>' . $this->messages['uctop'] . '</strong>';
			if( !$row->page_is_new ) {
				$difftext .= '(' . $sk->makeKnownLinkObj( $page, $this->messages['diff'], 'diff=0' ) . ')';
			} else {
				$difftext .= $this->messages['newarticle'];
			}

			if( !$page->getUserPermissionsErrors( 'rollback', $wgUser )
			&&  !$page->getUserPermissionsErrors( 'edit', $wgUser ) ) {
				$topmarktext .= ' '.$sk->generateRollback( $rev );
			}

		}
		# Is there a visible previous revision?
		if( $rev->userCan(Revision::DELETED_TEXT) ) {
			$difftext = '(' . $sk->makeKnownLinkObj( $page, $this->messages['diff'], 'diff=prev&oldid='.$row->rev_id ) . ')';
		} else {
			$difftext = '(' . $this->messages['diff'] . ')';
		}
		$histlink='('.$sk->makeKnownLinkObj( $page, $this->messages['hist'], 'action=history' ) . ')';

		$comment = $wgContLang->getDirMark() . $sk->revComment( $rev, false, true );
		$d = $wgLang->timeanddate( wfTimestamp( TS_MW, $row->rev_timestamp ), true );

		if( $this->target == 'newbies' ) {
			$userlink = ' . . ' . $sk->userLink( $row->rev_user, $row->rev_user_text );
			$userlink .= ' (' . $sk->userTalkLink( $row->rev_user, $row->rev_user_text ) . ') ';
		} else {
			$userlink = '';
		}

		if( $rev->isDeleted( Revision::DELETED_TEXT ) ) {
			$d = '<span class="history-deleted">' . $d . '</span>';
		}

		if( $rev->getParentId() === 0 ) {
			$nflag = '<span class="newpage">' . $this->messages['newpageletter'] . '</span>';
		} else {
			$nflag = '';
		}

		if( $row->rev_minor_edit ) {
			$mflag = '<span class="minor">' . $this->messages['minoreditletter'] . '</span> ';
		} else {
			$mflag = '';
		}

		$ret = "{$d} {$histlink} {$difftext} {$nflag}{$mflag} {$link}{$userlink}{$comment} {$topmarktext}";
		if( $rev->isDeleted( Revision::DELETED_TEXT ) ) {
			$ret .= ' ' . wfMsgHtml( 'deletedrev' );
		}
		// Let extensions add data
		wfRunHooks( 'ContributionsLineEnding', array( &$this, &$ret, $row ) );
		
		$ret = "<li>$ret</li>\n";
		wfProfileOut( __METHOD__ );
		return $ret;
	}

	/**
	 * Get the Database object in use
	 *
	 * @return Database
	 */
	public function getDatabase() {
		return $this->mDb;
	}

}

/**
 * Special page "user contributions".
 * Shows a list of the contributions of a user.
 *
 * @return	none
 * @param	$par	String: (optional) user name of the user for which to show the contributions
 */
function wfSpecialContributions( $par = null ) {
	global $wgUser, $wgOut, $wgLang, $wgRequest;

	$options = array();

	if ( isset( $par ) && $par == 'newbies' ) {
		$target = 'newbies';
		$options['contribs'] = 'newbie';
	} elseif ( isset( $par ) ) {
		$target = $par;
	} else {
		$target = $wgRequest->getVal( 'target' );
	}

	// check for radiobox
	if ( $wgRequest->getVal( 'contribs' ) == 'newbie' ) {
		$target = 'newbies';
		$options['contribs'] = 'newbie';
	}

	if ( !strlen( $target ) ) {
		$wgOut->addHTML( contributionsForm( '' ) );
		return;
	}

	$options['limit'] = $wgRequest->getInt( 'limit', 50 );
	$options['target'] = $target;

	$nt = Title::makeTitleSafe( NS_USER, $target );
	if ( !$nt ) {
		$wgOut->addHTML( contributionsForm( '' ) );
		return;
	}
	$id = User::idFromName( $nt->getText() );

	if ( $target != 'newbies' ) {
		$target = $nt->getText();
		$wgOut->setSubtitle( contributionsSub( $nt, $id ) );
	} else {
		$wgOut->setSubtitle( wfMsgHtml( 'sp-contributions-newbies-sub') );
	}

	if ( ( $ns = $wgRequest->getVal( 'namespace', null ) ) !== null && $ns !== '' ) {
		$options['namespace'] = intval( $ns );
	} else {
		$options['namespace'] = '';
	}
	if ( $wgUser->isAllowed( 'markbotedit' ) && $wgRequest->getBool( 'bot' ) ) {
		$options['bot'] = '1';
	}

	$skip = $wgRequest->getText( 'offset' ) || $wgRequest->getText( 'dir' ) == 'prev';
	# Offset overrides year/month selection
	if ( ( $month = $wgRequest->getIntOrNull( 'month' ) ) !== null && $month !== -1 ) {
		$options['month'] = intval( $month );
	} else {
		$options['month'] = '';
	}
	if ( ( $year = $wgRequest->getIntOrNull( 'year' ) ) !== null ) {
		$options['year'] = intval( $year );
	} else if( $options['month'] ) {
		$thisMonth = intval( gmdate( 'n' ) );
		$thisYear = intval( gmdate( 'Y' ) );
		if( intval( $options['month'] ) > $thisMonth ) {
			$thisYear--;
		}
		$options['year'] = $thisYear;
	} else {
		$options['year'] = '';
	}

	wfRunHooks( 'SpecialContributionsBeforeMainOutput', $id );

	if( $skip ) {
		$options['year'] = '';
		$options['month'] = '';
	}

	$wgOut->addHTML( contributionsForm( $options ) );

	$pager = new ContribsPager( $target, $options['namespace'], $options['year'], $options['month'] );
	if ( !$pager->getNumRows() ) {
		$wgOut->addWikiMsg( 'nocontribs' );
		return;
	}

	# Show a message about slave lag, if applicable
	if( ( $lag = $pager->getDatabase()->getLag() ) > 0 )
		$wgOut->showLagWarning( $lag );

	$wgOut->addHTML(
		'<p>' . $pager->getNavigationBar() . '</p>' .
		$pager->getBody() .
		'<p>' . $pager->getNavigationBar() . '</p>' );

	# If there were contributions, and it was a valid user or IP, show
	# the appropriate "footer" message - WHOIS tools, etc.
	if( $target != 'newbies' ) {
		$message = IP::isIPAddress( $target )
			? 'sp-contributions-footer-anon'
			: 'sp-contributions-footer';


		$text = wfMsgNoTrans( $message, $target );
		if( !wfEmptyMsg( $message, $text ) && $text != '-' ) {
			$wgOut->addHtml( '<div class="mw-contributions-footer">' );
			$wgOut->addWikiText( $text );
			$wgOut->addHtml( '</div>' );
		}
	}
}

/**
 * Generates the subheading with links
 * @param Title $nt Title object for the target
 * @param integer $id User ID for the target
 * @return String: appropriately-escaped HTML to be output literally
 */
function contributionsSub( $nt, $id ) {
	global $wgSysopUserBans, $wgLang, $wgUser;

	$sk = $wgUser->getSkin();

	if ( 0 == $id ) {
		$user = $nt->getText();
	} else {
		$user = $sk->makeLinkObj( $nt, htmlspecialchars( $nt->getText() ) );
	}
	$talk = $nt->getTalkPage();
	if( $talk ) {
		# Talk page link
		$tools[] = $sk->makeLinkObj( $talk, wfMsgHtml( 'talkpagelinktext' ) );
		if( ( $id != 0 && $wgSysopUserBans ) || ( $id == 0 && User::isIP( $nt->getText() ) ) ) {
			# Block link
			if( $wgUser->isAllowed( 'block' ) )
				$tools[] = $sk->makeKnownLinkObj( SpecialPage::getTitleFor( 'Blockip', $nt->getDBkey() ), wfMsgHtml( 'blocklink' ) );
			# Block log link
			$tools[] = $sk->makeKnownLinkObj( SpecialPage::getTitleFor( 'Log' ), wfMsgHtml( 'sp-contributions-blocklog' ), 'type=block&page=' . $nt->getPrefixedUrl() );
		}
		# Other logs link
		$tools[] = $sk->makeKnownLinkObj( SpecialPage::getTitleFor( 'Log' ), wfMsgHtml( 'log' ), 'user=' . $nt->getPartialUrl() );

		wfRunHooks( 'ContributionsToolLinks', array( $id, $nt, &$tools ) );

		$links = implode( ' | ', $tools );
	}

	// Old message 'contribsub' had one parameter, but that doesn't work for
	// languages that want to put the "for" bit right after $user but before
	// $links.  If 'contribsub' is around, use it for reverse compatibility,
	// otherwise use 'contribsub2'.
	if( wfEmptyMsg( 'contribsub', wfMsg( 'contribsub' ) ) ) {
		return wfMsgHtml( 'contribsub2', $user, $links );
	} else {
		return wfMsgHtml( 'contribsub', "$user ($links)" );
	}
}

/**
 * Generates the namespace selector form with hidden attributes.
 * @param $options Array: the options to be included.
 */
function contributionsForm( $options ) {
	global $wgScript, $wgTitle, $wgRequest;

	$options['title'] = $wgTitle->getPrefixedText();
	if ( !isset( $options['target'] ) ) {
		$options['target'] = '';
	} else {
		$options['target'] = str_replace( '_' , ' ' , $options['target'] );
	}

	if ( !isset( $options['namespace'] ) ) {
		$options['namespace'] = '';
	}

	if ( !isset( $options['contribs'] ) ) {
		$options['contribs'] = 'user';
	}

	if ( !isset( $options['year'] ) ) {
		$options['year'] = '';
	}

	if ( !isset( $options['month'] ) ) {
		$options['month'] = '';
	}

	if ( $options['contribs'] == 'newbie' ) {
		$options['target'] = '';
	}

	$f = Xml::openElement( 'form', array( 'method' => 'get', 'action' => $wgScript ) );

	foreach ( $options as $name => $value ) {
		if ( in_array( $name, array( 'namespace', 'target', 'contribs', 'year', 'month' ) ) ) {
			continue;
		}
		$f .= "\t" . Xml::hidden( $name, $value ) . "\n";
	}

	$f .= '<fieldset>' .
		Xml::element( 'legend', array(), wfMsg( 'sp-contributions-search' ) ) .
		Xml::radioLabel( wfMsgExt( 'sp-contributions-newbies', array( 'parseinline' ) ), 'contribs' , 'newbie' , 'newbie', $options['contribs'] == 'newbie' ? true : false ) . '<br />' .
		Xml::radioLabel( wfMsgExt( 'sp-contributions-username', array( 'parseinline' ) ), 'contribs' , 'user', 'user', $options['contribs'] == 'user' ? true : false ) . ' ' .
		Xml::input( 'target', 20, $options['target']) . ' '.
		'<span style="white-space: nowrap">' .
		Xml::label( wfMsg( 'namespace' ), 'namespace' ) . ' ' .
		Xml::namespaceSelector( $options['namespace'], '' ) .
		'</span>' .
		Xml::openElement( 'p' ) .
		'<span style="white-space: nowrap">' .
		Xml::label( wfMsg( 'year' ), 'year' ) . ' '.
		Xml::input( 'year', 4, $options['year'], array('id' => 'year', 'maxlength' => 4) ) .
		'</span>' .
		' '.
		'<span style="white-space: nowrap">' .
		Xml::label( wfMsg( 'month' ), 'month' ) . ' '.
		Xml::monthSelector( $options['month'], -1 ) . ' '.
		'</span>' .
		Xml::submitButton( wfMsg( 'sp-contributions-submit' ) ) .
		Xml::closeElement( 'p' );

	$explain = wfMsgExt( 'sp-contributions-explain', 'parseinline' );
	if( !wfEmptyMsg( 'sp-contributions-explain', $explain ) )
		$f .= "<p>{$explain}</p>";

	$f .= '</fieldset>' .
		Xml::closeElement( 'form' );
	return $f;
}
