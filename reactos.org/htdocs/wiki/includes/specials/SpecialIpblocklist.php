<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * @todo document
 */
function wfSpecialIpblocklist() {
	global $wgUser, $wgOut, $wgRequest;

	$ip = $wgRequest->getVal( 'wpUnblockAddress', $wgRequest->getVal( 'ip' ) );
	$id = $wgRequest->getVal( 'id' );
	$reason = $wgRequest->getText( 'wpUnblockReason' );
	$action = $wgRequest->getText( 'action' );
	$successip = $wgRequest->getVal( 'successip' );

	$ipu = new IPUnblockForm( $ip, $id, $reason );

	if( $action == 'unblock' ) {
		# Check permissions
		if( !$wgUser->isAllowed( 'block' ) ) {
			$wgOut->permissionRequired( 'block' );
			return;
		}
		# Check for database lock
		if( wfReadOnly() ) {
			$wgOut->readOnlyPage();
			return;
		}
		# Show unblock form
		$ipu->showForm( '' );
	} elseif( $action == 'submit' && $wgRequest->wasPosted()
		&& $wgUser->matchEditToken( $wgRequest->getVal( 'wpEditToken' ) ) ) {
		# Check permissions
		if( !$wgUser->isAllowed( 'block' ) ) {
			$wgOut->permissionRequired( 'block' );
			return;
		}
		# Check for database lock
		if( wfReadOnly() ) {
			$wgOut->readOnlyPage();
			return;
		}
		# Remove blocks and redirect user to success page
		$ipu->doSubmit();
	} elseif( $action == 'success' ) {
		# Inform the user of a successful unblock
		# (No need to check permissions or locks here,
		# if something was done, then it's too late!)
		if ( substr( $successip, 0, 1) == '#' ) {
			// A block ID was unblocked
			$ipu->showList( $wgOut->parse( wfMsg( 'unblocked-id', $successip ) ) );
		} else {
			// A username/IP was unblocked
			$ipu->showList( $wgOut->parse( wfMsg( 'unblocked', $successip ) ) );
		}
	} else {
		# Just show the block list
		$ipu->showList( '' );
	}

}

/**
 * implements Special:ipblocklist GUI
 * @ingroup SpecialPage
 */
class IPUnblockForm {
	var $ip, $reason, $id;

	function IPUnblockForm( $ip, $id, $reason ) {
		$this->ip = strtr( $ip, '_', ' ' );
		$this->id = $id;
		$this->reason = $reason;
	}

	/**
	 * Generates the unblock form
	 * @param $err string: error message
	 * @return $out string: HTML form
	 */
	function showForm( $err ) {
		global $wgOut, $wgUser, $wgSysopUserBans;

		$wgOut->setPagetitle( wfMsg( 'unblockip' ) );
		$wgOut->addWikiMsg( 'unblockiptext' );

		$titleObj = SpecialPage::getTitleFor( "Ipblocklist" );
		$action = $titleObj->getLocalURL( "action=submit" );

		if ( "" != $err ) {
			$wgOut->setSubtitle( wfMsg( "formerror" ) );
			$wgOut->addWikiText( Xml::tags( 'span', array( 'class' => 'error' ), $err ) . "\n" );
		}

		$addressPart = false;
		if ( $this->id ) {
			$block = Block::newFromID( $this->id );
			if ( $block ) {
				$encName = htmlspecialchars( $block->getRedactedName() );
				$encId = $this->id;
				$addressPart = $encName . Xml::hidden( 'id', $encId );
				$ipa = wfMsgHtml( $wgSysopUserBans ? 'ipadressorusername' : 'ipaddress' );
			}
		}
		if ( !$addressPart ) {
			$addressPart = Xml::input( 'wpUnblockAddress', 40, $this->ip, array( 'type' => 'text', 'tabindex' => '1' ) );
			$ipa = Xml::label( wfMsg( $wgSysopUserBans ? 'ipadressorusername' : 'ipaddress' ), 'wpUnblockAddress' );
		}

		$wgOut->addHTML(
			Xml::openElement( 'form', array( 'method' => 'post', 'action' => $action, 'id' => 'unblockip' ) ) .
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'ipb-unblock' ) ) .
			Xml::openElement( 'table', array( 'id' => 'mw-unblock-table' ) ).
			"<tr>
				<td class='mw-label'>
					{$ipa}
				</td>
				<td class='mw-input'>
					{$addressPart}
				</td>
			</tr>
			<tr>
				<td class='mw-label'>" .
					Xml::label( wfMsg( 'ipbreason' ), 'wpUnblockReason' ) .
				"</td>
				<td class='mw-input'>" .
					Xml::input( 'wpUnblockReason', 40, $this->reason, array( 'type' => 'text', 'tabindex' => '2' ) ) .
				"</td>
			</tr>
			<tr>
				<td>&nbsp;</td>
				<td class='mw-submit'>" .
					Xml::submitButton( wfMsg( 'ipusubmit' ), array( 'name' => 'wpBlock', 'tabindex' => '3' ) ) .
				"</td>
			</tr>" .
			Xml::closeElement( 'table' ) .
			Xml::closeElement( 'fieldset' ) .
			Xml::hidden( 'wpEditToken', $wgUser->editToken() ) .
			Xml::closeElement( 'form' ) . "\n"
		);

	}

	const UNBLOCK_SUCCESS = 0; // Success
	const UNBLOCK_NO_SUCH_ID = 1; // No such block ID
	const UNBLOCK_USER_NOT_BLOCKED = 2; // IP wasn't blocked
	const UNBLOCK_BLOCKED_AS_RANGE = 3; // IP is part of a range block
	const UNBLOCK_UNKNOWNERR = 4; // Unknown error

	/**
	 * Backend code for unblocking. doSubmit() wraps around this.
	 * $range is only used when UNBLOCK_BLOCKED_AS_RANGE is returned, in which
	 * case it contains the range $ip is part of.
	 * @return array array(message key, parameters) on failure, empty array on success
	 */

	static function doUnblock(&$id, &$ip, &$reason, &$range = null)
	{
		if ( $id ) {
			$block = Block::newFromID( $id );
			if ( !$block ) {
				return array('ipb_cant_unblock', htmlspecialchars($id));
			}
			$ip = $block->getRedactedName();
		} else {
			$block = new Block();
			$ip = trim( $ip );
			if ( substr( $ip, 0, 1 ) == "#" ) {
				$id = substr( $ip, 1 );
				$block = Block::newFromID( $id );
				if( !$block ) {
					return array('ipb_cant_unblock', htmlspecialchars($id));
				}
				$ip = $block->getRedactedName();
			} else {
				$block = Block::newFromDB( $ip );
				if ( !$block ) {
					return array('ipb_cant_unblock', htmlspecialchars($id));
				}
				if( $block->mRangeStart != $block->mRangeEnd
						&& !strstr( $ip, "/" ) ) {
					/* If the specified IP is a single address, and the block is
					 * a range block, don't unblock the range. */
					 $range = $block->mAddress;
					 return array('ipb_blocked_as_range', $ip, $range);
				}
			}
		}
		// Yes, this is really necessary
		$id = $block->mId;

		# Delete block
		if ( !$block->delete() ) {
			return array('ipb_cant_unblock', htmlspecialchars($id));
		}

		# Make log entry
		$log = new LogPage( 'block' );
		$log->addEntry( 'unblock', Title::makeTitle( NS_USER, $ip ), $reason );
		return array();
	}

	function doSubmit() {
		global $wgOut;
		$retval = self::doUnblock($this->id, $this->ip, $this->reason, $range);
		if(!empty($retval))
		{
			$key = array_shift($retval);
			$this->showForm(wfMsgReal($key, $retval));
			return;
		}
		# Report to the user
		$titleObj = SpecialPage::getTitleFor( "Ipblocklist" );
		$success = $titleObj->getFullURL( "action=success&successip=" . urlencode( $this->ip ) );
		$wgOut->redirect( $success );
	}

	function showList( $msg ) {
		global $wgOut, $wgUser;

		$wgOut->setPagetitle( wfMsg( "ipblocklist" ) );
		if ( "" != $msg ) {
			$wgOut->setSubtitle( $msg );
		}

		// Purge expired entries on one in every 10 queries
		if ( !mt_rand( 0, 10 ) ) {
			Block::purgeExpired();
		}

		$conds = array();
		$matches = array();
		// Is user allowed to see all the blocks?
		if ( !$wgUser->isAllowed( 'suppress' ) )
			$conds['ipb_deleted'] = 0;
		if ( $this->ip == '' ) {
			// No extra conditions
		} elseif ( substr( $this->ip, 0, 1 ) == '#' ) {
			$conds['ipb_id'] = substr( $this->ip, 1 );
		} elseif ( IP::toUnsigned( $this->ip ) !== false ) {
			$conds['ipb_address'] = $this->ip;
			$conds['ipb_auto'] = 0;
		} elseif( preg_match( '/^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})\\/(\\d{1,2})$/', $this->ip, $matches ) ) {
			$conds['ipb_address'] = Block::normaliseRange( $this->ip );
			$conds['ipb_auto'] = 0;
		} else {
			$user = User::newFromName( $this->ip );
			if ( $user && ( $id = $user->getId() ) != 0 ) {
				$conds['ipb_user'] = $id;
			} else {
				// Uh...?
				$conds['ipb_address'] = $this->ip;
				$conds['ipb_auto'] = 0;
			}
		}

		$pager = new IPBlocklistPager( $this, $conds );
		if ( $pager->getNumRows() ) {
			$wgOut->addHTML(
				$this->searchForm() .
				$pager->getNavigationBar() .
				Xml::tags( 'ul', null, $pager->getBody() ) .
				$pager->getNavigationBar()
			);
		} elseif ( $this->ip != '') {
			$wgOut->addHTML( $this->searchForm() );
			$wgOut->addWikiMsg( 'ipblocklist-no-results' );
		} else {
			$wgOut->addWikiMsg( 'ipblocklist-empty' );
		}
	}

	function searchForm() {
		global $wgTitle, $wgScript, $wgRequest;
		return
			Xml::tags( 'form', array( 'action' => $wgScript ),
				Xml::hidden( 'title', $wgTitle->getPrefixedDbKey() ) .
				Xml::openElement( 'fieldset' ) .
				Xml::element( 'legend', null, wfMsg( 'ipblocklist-legend' ) ) .
				Xml::inputLabel( wfMsg( 'ipblocklist-username' ), 'ip', 'ip', /* size */ false, $this->ip ) .
				'&nbsp;' .
				Xml::submitButton( wfMsg( 'ipblocklist-submit' ) ) .
				Xml::closeElement( 'fieldset' )
			);
	}

	/**
	 * Callback function to output a block
	 */
	function formatRow( $block ) {
		global $wgUser, $wgLang;

		wfProfileIn( __METHOD__ );

		static $sk=null, $msg=null;

		if( is_null( $sk ) )
			$sk = $wgUser->getSkin();
		if( is_null( $msg ) ) {
			$msg = array();
			$keys = array( 'infiniteblock', 'expiringblock', 'unblocklink',
				'anononlyblock', 'createaccountblock', 'noautoblockblock', 'emailblock' );
			foreach( $keys as $key ) {
				$msg[$key] = wfMsgHtml( $key );
			}
			$msg['blocklistline'] = wfMsg( 'blocklistline' );
		}

		# Prepare links to the blocker's user and talk pages
		$blocker_id = $block->getBy();
		$blocker_name = $block->getByName();
		$blocker = $sk->userLink( $blocker_id, $blocker_name );
		$blocker .= $sk->userToolLinks( $blocker_id, $blocker_name );

		# Prepare links to the block target's user and contribs. pages (as applicable, don't do it for autoblocks)
		if( $block->mAuto ) {
			$target = $block->getRedactedName(); # Hide the IP addresses of auto-blocks; privacy
		} else {
			$target = $sk->userLink( $block->mUser, $block->mAddress )
				. $sk->userToolLinks( $block->mUser, $block->mAddress, false, Linker::TOOL_LINKS_NOBLOCK );
		}

		$formattedTime = $wgLang->timeanddate( $block->mTimestamp, true );

		$properties = array();
		$properties[] = Block::formatExpiry( $block->mExpiry );
		if ( $block->mAnonOnly ) {
			$properties[] = $msg['anononlyblock'];
		}
		if ( $block->mCreateAccount ) {
			$properties[] = $msg['createaccountblock'];
		}
		if (!$block->mEnableAutoblock && $block->mUser ) {
			$properties[] = $msg['noautoblockblock'];
		}

		if ( $block->mBlockEmail && $block->mUser ) {
			$properties[] = $msg['emailblock'];
		}

		$properties = implode( ', ', $properties );

		$line = wfMsgReplaceArgs( $msg['blocklistline'], array( $formattedTime, $blocker, $target, $properties ) );

		$unblocklink = '';
		if ( $wgUser->isAllowed('block') ) {
			$titleObj = SpecialPage::getTitleFor( "Ipblocklist" );
			$unblocklink = ' (' . $sk->makeKnownLinkObj($titleObj, $msg['unblocklink'], 'action=unblock&id=' . urlencode( $block->mId ) ) . ')';
		}

		$comment = $sk->commentBlock( $block->mReason );

		$s = "{$line} $comment";
		if ( $block->mHideName )
			$s = '<span class="history-deleted">' . $s . '</span>';

		wfProfileOut( __METHOD__ );
		return "<li>$s $unblocklink</li>\n";
	}
}

/**
 * @todo document
 * @ingroup Pager
 */
class IPBlocklistPager extends ReverseChronologicalPager {
	public $mForm, $mConds;

	function __construct( $form, $conds = array() ) {
		$this->mForm = $form;
		$this->mConds = $conds;
		parent::__construct();
	}

	function getStartBody() {
		wfProfileIn( __METHOD__ );
		# Do a link batch query
		$this->mResult->seek( 0 );
		$lb = new LinkBatch;

		/*
		while ( $row = $this->mResult->fetchObject() ) {
			$lb->addObj( Title::makeTitleSafe( NS_USER, $row->user_name ) );
			$lb->addObj( Title::makeTitleSafe( NS_USER_TALK, $row->user_name ) );
			$lb->addObj( Title::makeTitleSafe( NS_USER, $row->ipb_address ) );
			$lb->addObj( Title::makeTitleSafe( NS_USER_TALK, $row->ipb_address ) );
		}*/
		# Faster way
		# Usernames and titles are in fact related by a simple substitution of space -> underscore
		# The last few lines of Title::secureAndSplit() tell the story.
		while ( $row = $this->mResult->fetchObject() ) {
			$name = str_replace( ' ', '_', $row->ipb_by_text );
			$lb->add( NS_USER, $name );
			$lb->add( NS_USER_TALK, $name );
			$name = str_replace( ' ', '_', $row->ipb_address );
			$lb->add( NS_USER, $name );
			$lb->add( NS_USER_TALK, $name );
		}
		$lb->execute();
		wfProfileOut( __METHOD__ );
		return '';
	}

	function formatRow( $row ) {
		$block = new Block;
		$block->initFromRow( $row );
		return $this->mForm->formatRow( $block );
	}

	function getQueryInfo() {
		$conds = $this->mConds;
		$conds[] = 'ipb_expiry>' . $this->mDb->addQuotes( $this->mDb->timestamp() );
		return array(
			'tables' => 'ipblocks',
			'fields' => '*',
			'conds' => $conds,
		);
	}

	function getIndexField() {
		return 'ipb_timestamp';
	}
}
