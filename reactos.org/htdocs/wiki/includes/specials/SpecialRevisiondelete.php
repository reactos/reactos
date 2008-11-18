<?php
/**
 * Special page allowing users with the appropriate permissions to view
 * and hide revisions. Log items can also be hidden.
 *
 * @file
 * @ingroup SpecialPage
 */

function wfSpecialRevisiondelete( $par = null ) {
	global $wgOut, $wgRequest, $wgUser;
	# Handle our many different possible input types
	$target = $wgRequest->getText( 'target' );
	$oldid = $wgRequest->getArray( 'oldid' );
	$artimestamp = $wgRequest->getArray( 'artimestamp' );
	$logid = $wgRequest->getArray( 'logid' );
	$img = $wgRequest->getArray( 'oldimage' );
	$fileid = $wgRequest->getArray( 'fileid' );
	# For reviewing deleted files...
	$file = $wgRequest->getVal( 'file' );
	# If this is a revision, then we need a target page
	$page = Title::newFromUrl( $target );
	if( is_null($page) ) {
		$wgOut->addWikiMsg( 'undelete-header' );
		return;
	}
	# Only one target set at a time please!
	$i = (bool)$file + (bool)$oldid + (bool)$logid + (bool)$artimestamp + (bool)$fileid + (bool)$img;
	if( $i !== 1 ) {
		$wgOut->showErrorPage( 'revdelete-nooldid-title', 'revdelete-nooldid-text' );
		return;
	}
	# Logs must have a type given
	if( $logid && !strpos($page->getDBKey(),'/') ) {
		$wgOut->showErrorPage( 'revdelete-nooldid-title', 'revdelete-nooldid-text' );
		return;
	}
	# Either submit or create our form
	$form = new RevisionDeleteForm( $page, $oldid, $logid, $artimestamp, $fileid, $img, $file );
	if( $wgRequest->wasPosted() ) {
		$form->submit( $wgRequest );
	} else if( $oldid || $artimestamp ) {
		$form->showRevs();
	} else if( $fileid || $img ) {
		$form->showImages();
	} else if( $logid ) {
		$form->showLogItems();
	}
	# Show relevant lines from the deletion log. This will show even if said ID
	# does not exist...might be helpful
	$wgOut->addHTML( "<h2>" . htmlspecialchars( LogPage::logName( 'delete' ) ) . "</h2>\n" );
	LogEventsList::showLogExtract( $wgOut, 'delete', $page->getPrefixedText() );
	if( $wgUser->isAllowed( 'suppressionlog' ) ){
		$wgOut->addHTML( "<h2>" . htmlspecialchars( LogPage::logName( 'suppress' ) ) . "</h2>\n" );
		LogEventsList::showLogExtract( $wgOut, 'suppress', $page->getPrefixedText() );
	}
}

/**
 * Implements the GUI for Revision Deletion.
 * @ingroup SpecialPage
 */
class RevisionDeleteForm {
	/**
	 * @param Title $page
	 * @param array $oldids
	 * @param array $logids
	 * @param array $artimestamps
	 * @param array $fileids
	 * @param array $img
	 * @param string $file
	 */
	function __construct( $page, $oldids, $logids, $artimestamps, $fileids, $img, $file ) {
		global $wgUser, $wgOut;

		$this->page = $page;
		# For reviewing deleted files...
		if( $file ) {
			$oimage = RepoGroup::singleton()->getLocalRepo()->newFromArchiveName( $page, $file );
			$oimage->load();
			// Check if user is allowed to see this file
			if( !$oimage->userCan(File::DELETED_FILE) ) {
				$wgOut->permissionRequired( 'suppressrevision' );
			} else {
				$this->showFile( $file );
			}
			return;
		}
		$this->skin = $wgUser->getSkin();
		# Give a link to the log for this page
		if( !is_null($this->page) && $this->page->getNamespace() > -1 ) {
			$links = array();

			$logtitle = SpecialPage::getTitleFor( 'Log' );
			$links[] = $this->skin->makeKnownLinkObj( $logtitle, wfMsgHtml( 'viewpagelogs' ),
				wfArrayToCGI( array( 'page' => $this->page->getPrefixedUrl() ) ) );
			# Give a link to the page history
			$links[] = $this->skin->makeKnownLinkObj( $this->page, wfMsgHtml( 'pagehist' ),
				wfArrayToCGI( array( 'action' => 'history' ) ) );
			# Link to deleted edits
			if( $wgUser->isAllowed('undelete') ) {
				$undelete = SpecialPage::getTitleFor( 'Undelete' );
				$links[] = $this->skin->makeKnownLinkObj( $undelete, wfMsgHtml( 'deletedhist' ),
					wfArrayToCGI( array( 'target' => $this->page->getPrefixedUrl() ) ) );
			}
			# Logs themselves don't have histories or archived revisions
			$wgOut->setSubtitle( '<p>'.implode($links,' / ').'</p>' );
		}
		// At this point, we should only have one of these
		if( $oldids ) {
			$this->revisions = $oldids;
			$hide_content_name = array( 'revdelete-hide-text', 'wpHideText', Revision::DELETED_TEXT );
			$this->deleteKey='oldid';
		} else if( $artimestamps ) {
			$this->archrevs = $artimestamps;
			$hide_content_name = array( 'revdelete-hide-text', 'wpHideText', Revision::DELETED_TEXT );
			$this->deleteKey='artimestamp';
		} else if( $img ) {
			$this->ofiles = $img;
			$hide_content_name = array( 'revdelete-hide-image', 'wpHideImage', File::DELETED_FILE );
			$this->deleteKey='oldimage';
		} else if( $fileids ) {
			$this->afiles = $fileids;
			$hide_content_name = array( 'revdelete-hide-image', 'wpHideImage', File::DELETED_FILE );
			$this->deleteKey='fileid';
		} else if( $logids ) {
			$this->events = $logids;
			$hide_content_name = array( 'revdelete-hide-name', 'wpHideName', LogPage::DELETED_ACTION );
			$this->deleteKey='logid';
		}
		// Our checkbox messages depends one what we are doing,
		// e.g. we don't hide "text" for logs or images
		$this->checks = array(
			$hide_content_name,
			array( 'revdelete-hide-comment', 'wpHideComment', Revision::DELETED_COMMENT ),
			array( 'revdelete-hide-user', 'wpHideUser', Revision::DELETED_USER ) );
		if( $wgUser->isAllowed('suppressrevision') ) {
			$this->checks[] = array( 'revdelete-hide-restricted', 'wpHideRestricted', Revision::DELETED_RESTRICTED );
		}
	}

	/**
	 * Show a deleted file version requested by the visitor.
	 */
	private function showFile( $key ) {
		global $wgOut, $wgRequest;
		$wgOut->disable();

		# We mustn't allow the output to be Squid cached, otherwise
		# if an admin previews a deleted image, and it's cached, then
		# a user without appropriate permissions can toddle off and
		# nab the image, and Squid will serve it
		$wgRequest->response()->header( 'Expires: ' . gmdate( 'D, d M Y H:i:s', 0 ) . ' GMT' );
		$wgRequest->response()->header( 'Cache-Control: no-cache, no-store, max-age=0, must-revalidate' );
		$wgRequest->response()->header( 'Pragma: no-cache' );

		$store = FileStore::get( 'deleted' );
		$store->stream( $key );
	}

	/**
	 * This lets a user set restrictions for live and archived revisions
	 */
	function showRevs() {
		global $wgOut, $wgUser, $action;

		$UserAllowed = true;

		$count = ($this->deleteKey=='oldid') ?
			count($this->revisions) : count($this->archrevs);
		$wgOut->addWikiMsg( 'revdelete-selected', $this->page->getPrefixedText(), $count );

		$bitfields = 0;
		$wgOut->addHtml( "<ul>" );

		$where = $revObjs = array();
		$dbr = wfGetDB( DB_SLAVE );
		
		$revisions = 0;
		// Live revisions...
		if( $this->deleteKey=='oldid' ) {
			// Run through and pull all our data in one query
			foreach( $this->revisions as $revid ) {
				$where[] = intval($revid);
			}
			$whereClause = 'rev_id IN(' . implode(',',$where) . ')';
			$result = $dbr->select( array('revision','page'), '*',
				array( 'rev_page' => $this->page->getArticleID(),
					$whereClause, 'rev_page = page_id' ),
				__METHOD__ );
			while( $row = $dbr->fetchObject( $result ) ) {
				$revObjs[$row->rev_id] = new Revision( $row );
			}
			foreach( $this->revisions as $revid ) {
				// Hiding top revisison is bad
				if( !isset($revObjs[$revid]) || $revObjs[$revid]->isCurrent() ) {
					continue;
				} else if( !$revObjs[$revid]->userCan(Revision::DELETED_RESTRICTED) ) {
				// If a rev is hidden from sysops
					if( $action != 'submit') {
						$wgOut->permissionRequired( 'suppressrevision' );
						return;
					}
					$UserAllowed = false;
				}
				$revisions++;
				$wgOut->addHtml( $this->historyLine( $revObjs[$revid] ) );
				$bitfields |= $revObjs[$revid]->mDeleted;
			}
		// The archives...
		} else {
			// Run through and pull all our data in one query
			foreach( $this->archrevs as $timestamp ) {
				$where[] = $dbr->addQuotes( $timestamp );
			}
			$whereClause = 'ar_timestamp IN(' . implode(',',$where) . ')';
			$result = $dbr->select( 'archive', '*',
				array( 'ar_namespace' => $this->page->getNamespace(),
					'ar_title' => $this->page->getDBKey(),
						$whereClause ),
				__METHOD__ );
			while( $row = $dbr->fetchObject( $result ) ) {
				$revObjs[$row->ar_timestamp] = new Revision( array(
				'page'       => $this->page->getArticleId(),
				'id'         => $row->ar_rev_id,
				'text'       => $row->ar_text_id,
				'comment'    => $row->ar_comment,
				'user'       => $row->ar_user,
				'user_text'  => $row->ar_user_text,
				'timestamp'  => $row->ar_timestamp,
				'minor_edit' => $row->ar_minor_edit,
				'text_id'    => $row->ar_text_id,
				'deleted'    => $row->ar_deleted,
				'len'        => $row->ar_len) );
			}
			foreach( $this->archrevs as $timestamp ) {
				if( !isset($revObjs[$timestamp]) ) {
					continue;
				} else if( !$revObjs[$timestamp]->userCan(Revision::DELETED_RESTRICTED) ) {
				// If a rev is hidden from sysops
					if( $action != 'submit') {
						$wgOut->permissionRequired( 'suppressrevision' );
						return;
					}
					$UserAllowed = false;
				}
				$revisions++;
				$wgOut->addHtml( $this->historyLine( $revObjs[$timestamp] ) );
				$bitfields |= $revObjs[$timestamp]->mDeleted;
			}
		}
		if( !$revisions ) {
			$wgOut->showErrorPage( 'revdelete-nooldid-title', 'revdelete-nooldid-text' );
			return;
		}
		
		$wgOut->addHtml( "</ul>" );

		$wgOut->addWikiMsg( 'revdelete-text' );

		// Normal sysops can always see what they did, but can't always change it
		if( !$UserAllowed ) return;

		$items = array(
			Xml::inputLabel( wfMsg( 'revdelete-log' ), 'wpReason', 'wpReason', 60 ),
			Xml::submitButton( wfMsg( 'revdelete-submit' ) )
		);
		$hidden = array(
			Xml::hidden( 'wpEditToken', $wgUser->editToken() ),
			Xml::hidden( 'target', $this->page->getPrefixedText() ),
			Xml::hidden( 'type', $this->deleteKey )
		);
		if( $this->deleteKey=='oldid' ) {
			foreach( $revObjs as $rev )
				$hidden[] = Xml::hidden( 'oldid[]', $rev->getId() );
		} else {
			foreach( $revObjs as $rev )
				$hidden[] = Xml::hidden( 'artimestamp[]', $rev->getTimestamp() );
		}
		$special = SpecialPage::getTitleFor( 'Revisiondelete' );
		$wgOut->addHtml(
			Xml::openElement( 'form', array( 'method' => 'post', 'action' => $special->getLocalUrl( 'action=submit' ), 
				'id' => 'mw-revdel-form-revisions' ) ) .
			Xml::openElement( 'fieldset' ) .
			xml::element( 'legend', null,  wfMsg( 'revdelete-legend' ) )
		);
		// FIXME: all items checked for just one rev are checked, even if not set for the others
		foreach( $this->checks as $item ) {
			list( $message, $name, $field ) = $item;
			$wgOut->addHtml( Xml::tags( 'div', null, Xml::checkLabel( wfMsg( $message ), $name, $name, $bitfields & $field ) ) );
		}
		foreach( $items as $item ) {
			$wgOut->addHtml( Xml::tags( 'p', null, $item ) );
		}
		foreach( $hidden as $item ) {
			$wgOut->addHtml( $item );
		}
		$wgOut->addHtml(
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'form' ) . "\n"
		);

	}

	/**
	 * This lets a user set restrictions for archived images
	 */
	function showImages() {
		// What is $action doing here???
		global $wgOut, $wgUser, $action, $wgLang;

		$UserAllowed = true;

		$count = ($this->deleteKey=='oldimage') ? count($this->ofiles) : count($this->afiles);
		$wgOut->addWikiMsg( 'revdelete-selected',
			$this->page->getPrefixedText(),
			$wgLang->formatNum($count) );

		$bitfields = 0;
		$wgOut->addHtml( "<ul>" );

		$where = $filesObjs = array();
		$dbr = wfGetDB( DB_SLAVE );
		// Live old revisions...
		$revisions = 0;
		if( $this->deleteKey=='oldimage' ) {
			// Run through and pull all our data in one query
			foreach( $this->ofiles as $timestamp ) {
				$where[] = $dbr->addQuotes( $timestamp.'!'.$this->page->getDbKey() );
			}
			$whereClause = 'oi_archive_name IN(' . implode(',',$where) . ')';
			$result = $dbr->select( 'oldimage', '*',
				array( 'oi_name' => $this->page->getDbKey(),
					$whereClause ),
				__METHOD__ );
			while( $row = $dbr->fetchObject( $result ) ) {
				$filesObjs[$row->oi_archive_name] = RepoGroup::singleton()->getLocalRepo()->newFileFromRow( $row );
				$filesObjs[$row->oi_archive_name]->user = $row->oi_user;
				$filesObjs[$row->oi_archive_name]->user_text = $row->oi_user_text;
			}
			// Check through our images
			foreach( $this->ofiles as $timestamp ) {
				$archivename = $timestamp.'!'.$this->page->getDbKey();
				if( !isset($filesObjs[$archivename]) ) {
					continue;
				} else if( !$filesObjs[$archivename]->userCan(File::DELETED_RESTRICTED) ) {
					// If a rev is hidden from sysops
					if( $action != 'submit' ) {
						$wgOut->permissionRequired( 'suppressrevision' );
						return;
					}
					$UserAllowed = false;
				}
				$revisions++;
				// Inject history info
				$wgOut->addHtml( $this->fileLine( $filesObjs[$archivename] ) );
				$bitfields |= $filesObjs[$archivename]->deleted;
			}
		// Archived files...
		} else {
			// Run through and pull all our data in one query
			foreach( $this->afiles as $id ) {
				$where[] = intval($id);
			}
			$whereClause = 'fa_id IN(' . implode(',',$where) . ')';
			$result = $dbr->select( 'filearchive', '*',
				array( 'fa_name' => $this->page->getDbKey(),
					$whereClause ),
				__METHOD__ );
			while( $row = $dbr->fetchObject( $result ) ) {
				$filesObjs[$row->fa_id] = ArchivedFile::newFromRow( $row );
			}

			foreach( $this->afiles as $fileid ) {
				if( !isset($filesObjs[$fileid]) ) {
					continue;
				} else if( !$filesObjs[$fileid]->userCan(File::DELETED_RESTRICTED) ) {
					// If a rev is hidden from sysops
					if( $action != 'submit' ) {
						$wgOut->permissionRequired( 'suppressrevision' );
						return;
					}
					$UserAllowed = false;
				}
				$revisions++;
				// Inject history info
				$wgOut->addHtml( $this->archivedfileLine( $filesObjs[$fileid] ) );
				$bitfields |= $filesObjs[$fileid]->deleted;
			}
		}
		if( !$revisions ) {
			$wgOut->showErrorPage( 'revdelete-nooldid-title', 'revdelete-nooldid-text' );
			return;
		}
		
		$wgOut->addHtml( "</ul>" );

		$wgOut->addWikiMsg('revdelete-text' );
		//Normal sysops can always see what they did, but can't always change it
		if( !$UserAllowed ) return;

		$items = array(
			Xml::inputLabel( wfMsg( 'revdelete-log' ), 'wpReason', 'wpReason', 60 ),
			Xml::submitButton( wfMsg( 'revdelete-submit' ) )
		);
		$hidden = array(
			Xml::hidden( 'wpEditToken', $wgUser->editToken() ),
			Xml::hidden( 'target', $this->page->getPrefixedText() ),
			Xml::hidden( 'type', $this->deleteKey )
		);
		if( $this->deleteKey=='oldimage' ) {
			foreach( $this->ofiles as $filename )
				$hidden[] = Xml::hidden( 'oldimage[]', $filename );
		} else {
			foreach( $this->afiles as $fileid )
				$hidden[] = Xml::hidden( 'fileid[]', $fileid );
		}
		$special = SpecialPage::getTitleFor( 'Revisiondelete' );
		$wgOut->addHtml(
			Xml::openElement( 'form', array( 'method' => 'post', 'action' => $special->getLocalUrl( 'action=submit' ), 
				'id' => 'mw-revdel-form-filerevisions' ) ) .
			Xml::fieldset( wfMsg( 'revdelete-legend' ) )
		);
		// FIXME: all items checked for just one file are checked, even if not set for the others
		foreach( $this->checks as $item ) {
			list( $message, $name, $field ) = $item;
			$wgOut->addHtml( Xml::tags( 'div', null, Xml::checkLabel( wfMsg( $message ), $name, $name, $bitfields & $field ) ) );
		}
		foreach( $items as $item ) {
			$wgOut->addHtml( "<p>$item</p>" );
		}
		foreach( $hidden as $item ) {
			$wgOut->addHtml( $item );
		}

		$wgOut->addHtml(
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'form' ) . "\n"
		);
	}

	/**
	 * This lets a user set restrictions for log items
	 */
	function showLogItems() {
		global $wgOut, $wgUser, $action, $wgMessageCache, $wgLang;

		$UserAllowed = true;
		$wgOut->addWikiMsg( 'logdelete-selected', $wgLang->formatNum( count($this->events) ) );

		$bitfields = 0;
		$wgOut->addHtml( "<ul>" );

		$where = $logRows = array();
		$dbr = wfGetDB( DB_SLAVE );
		// Run through and pull all our data in one query
		$logItems = 0;
		foreach( $this->events as $logid ) {
			$where[] = intval($logid);
		}
		list($log,$logtype) = explode( '/',$this->page->getDBKey(), 2 );
		$whereClause = "log_type = '$logtype' AND log_id IN(" . implode(',',$where) . ")";
		$result = $dbr->select( 'logging', '*',
			array( $whereClause ),
			__METHOD__ );
		while( $row = $dbr->fetchObject( $result ) ) {
			$logRows[$row->log_id] = $row;
		}
		$wgMessageCache->loadAllMessages();
		foreach( $this->events as $logid ) {
			// Don't hide from oversight log!!!
			if( !isset( $logRows[$logid] ) || $logRows[$logid]->log_type=='suppress' ) {
				continue;
			} else if( !LogEventsList::userCan( $logRows[$logid],Revision::DELETED_RESTRICTED) ) {
			// If an event is hidden from sysops
				if( $action != 'submit') {
					$wgOut->permissionRequired( 'suppressrevision' );
					return;
				}
				$UserAllowed = false;
			}
			$logItems++;
			$wgOut->addHtml( $this->logLine( $logRows[$logid] ) );
			$bitfields |= $logRows[$logid]->log_deleted;
		}
		if( !$logItems ) {
			$wgOut->showErrorPage( 'revdelete-nooldid-title', 'revdelete-nooldid-text' );
			return;
		}
		
		$wgOut->addHtml( "</ul>" );

		$wgOut->addWikiMsg( 'revdelete-text' );
		// Normal sysops can always see what they did, but can't always change it
		if( !$UserAllowed ) return;

		$items = array(
			Xml::inputLabel( wfMsg( 'revdelete-log' ), 'wpReason', 'wpReason', 60 ),
			Xml::submitButton( wfMsg( 'revdelete-submit' ) ) );
		$hidden = array(
			Xml::hidden( 'wpEditToken', $wgUser->editToken() ),
			Xml::hidden( 'target', $this->page->getPrefixedText() ),
			Xml::hidden( 'type', $this->deleteKey ) );
		foreach( $this->events as $logid ) {
			$hidden[] = Xml::hidden( 'logid[]', $logid );
		}

		$special = SpecialPage::getTitleFor( 'Revisiondelete' );
		$wgOut->addHtml(
			Xml::openElement( 'form', array( 'method' => 'post', 'action' => $special->getLocalUrl( 'action=submit' ), 
				'id' => 'mw-revdel-form-logs' ) ) .
			Xml::fieldset( wfMsg( 'revdelete-legend' ) )
		);
		// FIXME: all items checked for just on event are checked, even if not set for the others
		foreach( $this->checks as $item ) {
			list( $message, $name, $field ) = $item;
			$wgOut->addHtml( Xml::tags( 'div', null, Xml::checkLabel( wfMsg( $message ), $name, $name, $bitfields & $field ) ) );
		}
		foreach( $items as $item ) {
			$wgOut->addHtml( "<p>$item</p>" );
		}
		foreach( $hidden as $item ) {
			$wgOut->addHtml( $item );
		}

		$wgOut->addHtml(
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'form' ) . "\n"
		);
	}

	/**
	 * @param Revision $rev
	 * @returns string
	 */
	private function historyLine( $rev ) {
		global $wgLang;

		$date = $wgLang->timeanddate( $rev->getTimestamp() );
		$difflink = $del = '';
		// Live revisions
		if( $this->deleteKey=='oldid' ) {
			$revlink = $this->skin->makeLinkObj( $this->page, $date, 'oldid=' . $rev->getId() );
			$difflink = '(' . $this->skin->makeKnownLinkObj( $this->page, wfMsgHtml('diff'),
				'diff=' . $rev->getId() . '&oldid=prev' ) . ')';
		// Archived revisions
		} else {
			$undelete = SpecialPage::getTitleFor( 'Undelete' );
			$target = $this->page->getPrefixedText();
			$revlink = $this->skin->makeLinkObj( $undelete, $date,
				"target=$target&timestamp=" . $rev->getTimestamp() );
			$difflink = '(' . $this->skin->makeKnownLinkObj( $undelete, wfMsgHtml('diff'),
				"target=$target&diff=prev&timestamp=" . $rev->getTimestamp() ) . ')';
		}

		if( $rev->isDeleted(Revision::DELETED_TEXT) ) {
			$revlink = '<span class="history-deleted">'.$revlink.'</span>';
			$del = ' <tt>' . wfMsgHtml( 'deletedrev' ) . '</tt>';
			if( !$rev->userCan(Revision::DELETED_TEXT) ) {
				$revlink = '<span class="history-deleted">'.$date.'</span>';
				$difflink = '(' . wfMsgHtml('diff') . ')';
			}
		}

		return "<li> $difflink $revlink ".$this->skin->revUserLink( $rev )." ".$this->skin->revComment( $rev )."$del</li>";
	}

	/**
	 * @param File $file
	 * @returns string
	 */
	private function fileLine( $file ) {
		global $wgLang, $wgTitle;

		$target = $this->page->getPrefixedText();
		$date = $wgLang->timeanddate( $file->getTimestamp(), true  );

		$del = '';
		# Hidden files...
		if( $file->isDeleted(File::DELETED_FILE) ) {
			$del = ' <tt>' . wfMsgHtml( 'deletedrev' ) . '</tt>';
			if( !$file->userCan(File::DELETED_FILE) ) {
				$pageLink = $date;
			} else {
				$pageLink = $this->skin->makeKnownLinkObj( $wgTitle, $date,
					"target=$target&file=$file->sha1.".$file->getExtension() );
			}
			$pageLink = '<span class="history-deleted">' . $pageLink . '</span>';
		# Regular files...
		} else {
			$url = $file->getUrlRel();
			$pageLink = "<a href=\"{$url}\">{$date}</a>";
		}

		$data = wfMsg( 'widthheight',
					$wgLang->formatNum( $file->getWidth() ),
					$wgLang->formatNum( $file->getHeight() ) ) .
			' (' . wfMsgExt( 'nbytes', 'parsemag', $wgLang->formatNum( $file->getSize() ) ) . ')';
		$data = htmlspecialchars( $data );

		return "<li>$pageLink ".$this->fileUserTools( $file )." $data ".$this->fileComment( $file )."$del</li>";
	}

	/**
	 * @param ArchivedFile $file
	 * @returns string
	 */
	private function archivedfileLine( $file ) {
		global $wgLang, $wgTitle;

		$target = $this->page->getPrefixedText();
		$date = $wgLang->timeanddate( $file->getTimestamp(), true  );

		$undelete = SpecialPage::getTitleFor( 'Undelete' );
		$pageLink = $this->skin->makeKnownLinkObj( $undelete, $date, "target=$target&file={$file->getKey()}" );

		$del = '';
		if( $file->isDeleted(File::DELETED_FILE) ) {
			$del = ' <tt>' . wfMsgHtml( 'deletedrev' ) . '</tt>';
		}

		$data = wfMsg( 'widthheight',
					$wgLang->formatNum( $file->getWidth() ),
					$wgLang->formatNum( $file->getHeight() ) ) .
			' (' . wfMsgExt( 'nbytes', 'parsemag', $wgLang->formatNum( $file->getSize() ) ) . ')';
		$data = htmlspecialchars( $data );

		return "<li> $pageLink ".$this->fileUserTools( $file )." $data ".$this->fileComment( $file )."$del</li>";
	}

	/**
	 * @param Array $row row
	 * @returns string
	 */
	private function logLine( $row ) {
		global $wgLang;

		$date = $wgLang->timeanddate( $row->log_timestamp );
		$paramArray = LogPage::extractParams( $row->log_params );
		$title = Title::makeTitle( $row->log_namespace, $row->log_title );

		$logtitle = SpecialPage::getTitleFor( 'Log' );
		$loglink = $this->skin->makeKnownLinkObj( $logtitle, wfMsgHtml( 'log' ),
			wfArrayToCGI( array( 'page' => $title->getPrefixedUrl() ) ) );
		// Action text
		if( !LogEventsList::userCan($row,LogPage::DELETED_ACTION) ) {
			$action = '<span class="history-deleted">' . wfMsgHtml('rev-deleted-event') . '</span>';
		} else {
			$action = LogPage::actionText( $row->log_type, $row->log_action, $title,
				$this->skin, $paramArray, true, true );
			if( $row->log_deleted & LogPage::DELETED_ACTION )
				$action = '<span class="history-deleted">' . $action . '</span>';
		}
		// User links
		$userLink = $this->skin->userLink( $row->log_user, User::WhoIs($row->log_user) );
		if( LogEventsList::isDeleted($row,LogPage::DELETED_USER) ) {
			$userLink = '<span class="history-deleted">' . $userLink . '</span>';
		}
		// Comment
		$comment = $wgLang->getDirMark() . $this->skin->commentBlock( $row->log_comment );
		if( LogEventsList::isDeleted($row,LogPage::DELETED_COMMENT) ) {
			$comment = '<span class="history-deleted">' . $comment . '</span>';
		}
		return "<li>($loglink) $date $userLink $action $comment</li>";
	}

	/**
	 * Generate a user tool link cluster if the current user is allowed to view it
	 * @param ArchivedFile $file
	 * @return string HTML
	 */
	private function fileUserTools( $file ) {
		if( $file->userCan( Revision::DELETED_USER ) ) {
			$link = $this->skin->userLink( $file->user, $file->user_text ) .
				$this->skin->userToolLinks( $file->user, $file->user_text );
		} else {
			$link = wfMsgHtml( 'rev-deleted-user' );
		}
		if( $file->isDeleted( Revision::DELETED_USER ) ) {
			return '<span class="history-deleted">' . $link . '</span>';
		}
		return $link;
	}

	/**
	 * Wrap and format the given file's comment block, if the current
	 * user is allowed to view it.
	 *
	 * @param ArchivedFile $file
	 * @return string HTML
	 */
	private function fileComment( $file ) {
		if( $file->userCan( File::DELETED_COMMENT ) ) {
			$block = $this->skin->commentBlock( $file->description );
		} else {
			$block = ' ' . wfMsgHtml( 'rev-deleted-comment' );
		}
		if( $file->isDeleted( File::DELETED_COMMENT ) ) {
			return "<span class=\"history-deleted\">$block</span>";
		}
		return $block;
	}

	/**
	 * @param WebRequest $request
	 */
	function submit( $request ) {
		global $wgUser, $wgOut;

		$bitfield = $this->extractBitfield( $request );
		$comment = $request->getText( 'wpReason' );
		# Can the user set this field?
		if( $bitfield & Revision::DELETED_RESTRICTED && !$wgUser->isAllowed('suppressrevision') ) {
			$wgOut->permissionRequired( 'suppressrevision' );
			return false;
		}
		# If the save went through, go to success message. Otherwise
		# bounce back to form...
		if( $this->save( $bitfield, $comment, $this->page ) ) {
			$this->success();
		} else if( $request->getCheck( 'oldid' ) || $request->getCheck( 'artimestamp' ) ) {
			return $this->showRevs();
		} else if( $request->getCheck( 'logid' ) ) {
			return $this->showLogs();
		} else if( $request->getCheck( 'oldimage' ) || $request->getCheck( 'fileid' ) ) {
			return $this->showImages();
		}
	}

	private function success() {
		global $wgOut;

		$wgOut->setPagetitle( wfMsg( 'actioncomplete' ) );

		$wrap = '<span class="success">$1</span>';

		if( $this->deleteKey=='logid' ) {
			$wgOut->wrapWikiMsg( $wrap, 'logdelete-success' );
			$this->showLogItems();
		} else if( $this->deleteKey=='oldid' || $this->deleteKey=='artimestamp' ) {
				$wgOut->wrapWikiMsg( $wrap, 'revdelete-success' );
		  	$this->showRevs();
		} else if( $this->deleteKey=='fileid' ) {
			$wgOut->wrapWikiMsg( $wrap, 'revdelete-success' );
		  	$this->showImages();
		} else if( $this->deleteKey=='oldimage' ) {
			$wgOut->wrapWikiMsg( $wrap, 'revdelete-success' );
			$this->showImages();
		}
	}

	/**
	 * Put together a rev_deleted bitfield from the submitted checkboxes
	 * @param WebRequest $request
	 * @return int
	 */
	private function extractBitfield( $request ) {
		$bitfield = 0;
		foreach( $this->checks as $item ) {
			list( /* message */ , $name, $field ) = $item;
			if( $request->getCheck( $name ) ) {
				$bitfield |= $field;
			}
		}
		return $bitfield;
	}

	private function save( $bitfield, $reason, $title ) {
		$dbw = wfGetDB( DB_MASTER );
		// Don't allow simply locking the interface for no reason
		if( $bitfield == Revision::DELETED_RESTRICTED ) {
			$bitfield = 0;
		}
		$deleter = new RevisionDeleter( $dbw );
		// By this point, only one of the below should be set
		if( isset($this->revisions) ) {
			return $deleter->setRevVisibility( $title, $this->revisions, $bitfield, $reason );
		} else if( isset($this->archrevs) ) {
			return $deleter->setArchiveVisibility( $title, $this->archrevs, $bitfield, $reason );
		} else if( isset($this->ofiles) ) {
			return $deleter->setOldImgVisibility( $title, $this->ofiles, $bitfield, $reason );
		} else if( isset($this->afiles) ) {
			return $deleter->setArchFileVisibility( $title, $this->afiles, $bitfield, $reason );
		} else if( isset($this->events) ) {
			return $deleter->setEventVisibility( $title, $this->events, $bitfield, $reason );
		}
	}
}

/**
 * Implements the actions for Revision Deletion.
 * @ingroup SpecialPage
 */
class RevisionDeleter {
	function __construct( $db ) {
		$this->dbw = $db;
	}

	/**
	 * @param $title, the page these events apply to
	 * @param array $items list of revision ID numbers
	 * @param int $bitfield new rev_deleted value
	 * @param string $comment Comment for log records
	 */
	function setRevVisibility( $title, $items, $bitfield, $comment ) {
		global $wgOut;

		$userAllowedAll = $success = true;
		$revIDs = array();
		$revCount = 0;
		// Run through and pull all our data in one query
		foreach( $items as $revid ) {
			$where[] = intval($revid);
		}
		$whereClause = 'rev_id IN(' . implode(',',$where) . ')';
		$result = $this->dbw->select( 'revision', '*',
			array( 'rev_page' => $title->getArticleID(),
				$whereClause ),
			__METHOD__ );
		while( $row = $this->dbw->fetchObject( $result ) ) {
			$revObjs[$row->rev_id] = new Revision( $row );
		}
		// To work!
		foreach( $items as $revid ) {
			if( !isset($revObjs[$revid]) || $revObjs[$revid]->isCurrent() ) {
				$success = false;
				continue; // Must exist
			} else if( !$revObjs[$revid]->userCan(Revision::DELETED_RESTRICTED) ) {
    			$userAllowedAll=false;
				continue;
			}
			// For logging, maintain a count of revisions
			if( $revObjs[$revid]->mDeleted != $bitfield ) {
				$revCount++;
				$revIDs[]=$revid;

			   	$this->updateRevision( $revObjs[$revid], $bitfield );
				$this->updateRecentChangesEdits( $revObjs[$revid], $bitfield, false );
			}
		}
		// Clear caches...
		// Don't log or touch if nothing changed
		if( $revCount > 0 ) {
			$this->updateLog( $title, $revCount, $bitfield, $revObjs[$revid]->mDeleted,
				$comment, $title, 'oldid', $revIDs );
			$this->updatePage( $title );
		}
		// Where all revs allowed to be set?
		if( !$userAllowedAll ) {
			//FIXME: still might be confusing???
			$wgOut->permissionRequired( 'suppressrevision' );
			return false;
		}

		return $success;
	}

	 /**
	 * @param $title, the page these events apply to
	 * @param array $items list of revision ID numbers
	 * @param int $bitfield new rev_deleted value
	 * @param string $comment Comment for log records
	 */
	function setArchiveVisibility( $title, $items, $bitfield, $comment ) {
		global $wgOut;

		$userAllowedAll = $success = true;
		$count = 0;
		$Id_set = array();
		// Run through and pull all our data in one query
		foreach( $items as $timestamp ) {
			$where[] = $this->dbw->addQuotes( $timestamp );
		}
		$whereClause = 'ar_timestamp IN(' . implode(',',$where) . ')';
		$result = $this->dbw->select( 'archive', '*',
			array( 'ar_namespace' => $title->getNamespace(),
				'ar_title' => $title->getDBKey(),
					$whereClause ),
			__METHOD__ );
		while( $row = $this->dbw->fetchObject( $result ) ) {
			$revObjs[$row->ar_timestamp] = new Revision( array(
			'page'       => $title->getArticleId(),
			'id'         => $row->ar_rev_id,
			'text'       => $row->ar_text_id,
			'comment'    => $row->ar_comment,
			'user'       => $row->ar_user,
			'user_text'  => $row->ar_user_text,
			'timestamp'  => $row->ar_timestamp,
			'minor_edit' => $row->ar_minor_edit,
			'text_id'    => $row->ar_text_id,
			'deleted'    => $row->ar_deleted,
			'len'        => $row->ar_len) );
		}
		// To work!
		foreach( $items as $timestamp ) {
			// This will only select the first revision with this timestamp.
			// Since they are all selected/deleted at once, we can just check the
			// permissions of one. UPDATE is done via timestamp, so all revs are set.
			if( !is_object($revObjs[$timestamp]) ) {
				$success = false;
				continue; // Must exist
			} else if( !$revObjs[$timestamp]->userCan(Revision::DELETED_RESTRICTED) ) {
    			$userAllowedAll=false;
				continue;
			}
			// Which revisions did we change anything about?
			if( $revObjs[$timestamp]->mDeleted != $bitfield ) {
			   $Id_set[]=$timestamp;
			   $count++;

			   $this->updateArchive( $revObjs[$timestamp], $title, $bitfield );
			}
		}
		// For logging, maintain a count of revisions
		if( $count > 0 ) {
			$this->updateLog( $title, $count, $bitfield, $revObjs[$timestamp]->mDeleted,
				$comment, $title, 'artimestamp', $Id_set );
		}
		// Where all revs allowed to be set?
		if( !$userAllowedAll ) {
			$wgOut->permissionRequired( 'suppressrevision' );
			return false;
		}

		return $success;
	}

	 /**
	 * @param $title, the page these events apply to
	 * @param array $items list of revision ID numbers
	 * @param int $bitfield new rev_deleted value
	 * @param string $comment Comment for log records
	 */
	function setOldImgVisibility( $title, $items, $bitfield, $comment ) {
		global $wgOut;

		$userAllowedAll = $success = true;
		$count = 0;
		$set = array();
		// Run through and pull all our data in one query
		foreach( $items as $timestamp ) {
			$where[] = $this->dbw->addQuotes( $timestamp.'!'.$title->getDbKey() );
		}
		$whereClause = 'oi_archive_name IN(' . implode(',',$where) . ')';
		$result = $this->dbw->select( 'oldimage', '*',
			array( 'oi_name' => $title->getDbKey(),
				$whereClause ),
			__METHOD__ );
		while( $row = $this->dbw->fetchObject( $result ) ) {
			$filesObjs[$row->oi_archive_name] = RepoGroup::singleton()->getLocalRepo()->newFileFromRow( $row );
			$filesObjs[$row->oi_archive_name]->user = $row->oi_user;
			$filesObjs[$row->oi_archive_name]->user_text = $row->oi_user_text;
		}
		// To work!
		foreach( $items as $timestamp ) {
			$archivename = $timestamp.'!'.$title->getDbKey();
			if( !isset($filesObjs[$archivename]) ) {
				$success = false;
				continue; // Must exist
			} else if( !$filesObjs[$archivename]->userCan(File::DELETED_RESTRICTED) ) {
    			$userAllowedAll=false;
				continue;
			}

			$transaction = true;
			// Which revisions did we change anything about?
			if( $filesObjs[$archivename]->deleted != $bitfield ) {
				$count++;

				$this->dbw->begin();
				$this->updateOldFiles( $filesObjs[$archivename], $bitfield );
				// If this image is currently hidden...
				if( $filesObjs[$archivename]->deleted & File::DELETED_FILE ) {
					if( $bitfield & File::DELETED_FILE ) {
						# Leave it alone if we are not changing this...
						$set[]=$archivename;
						$transaction = true;
					} else {
						# We are moving this out
						$transaction = $this->makeOldImagePublic( $filesObjs[$archivename] );
						$set[]=$transaction;
					}
				// Is it just now becoming hidden?
				} else if( $bitfield & File::DELETED_FILE ) {
					$transaction = $this->makeOldImagePrivate( $filesObjs[$archivename] );
					$set[]=$transaction;
				} else {
					$set[]=$timestamp;
				}
				// If our file operations fail, then revert back the db
				if( $transaction==false ) {
					$this->dbw->rollback();
					return false;
				}
				$this->dbw->commit();
			}
		}

		// Log if something was changed
		if( $count > 0 ) {
			$this->updateLog( $title, $count, $bitfield, $filesObjs[$archivename]->deleted,
				$comment, $title, 'oldimage', $set );
			# Purge page/history
			$file = wfLocalFile( $title );
			$file->purgeCache();
			$file->purgeHistory();
			# Invalidate cache for all pages using this file
			$update = new HTMLCacheUpdate( $title, 'imagelinks' );
			$update->doUpdate();
		}
		// Where all revs allowed to be set?
		if( !$userAllowedAll ) {
			$wgOut->permissionRequired( 'suppressrevision' );
			return false;
		}

		return $success;
	}

	 /**
	 * @param $title, the page these events apply to
	 * @param array $items list of revision ID numbers
	 * @param int $bitfield new rev_deleted value
	 * @param string $comment Comment for log records
	 */
	function setArchFileVisibility( $title, $items, $bitfield, $comment ) {
		global $wgOut;

		$userAllowedAll = $success = true;
		$count = 0;
		$Id_set = array();

		// Run through and pull all our data in one query
		foreach( $items as $id ) {
			$where[] = intval($id);
		}
		$whereClause = 'fa_id IN(' . implode(',',$where) . ')';
		$result = $this->dbw->select( 'filearchive', '*',
			array( 'fa_name' => $title->getDbKey(),
				$whereClause ),
			__METHOD__ );
		while( $row = $this->dbw->fetchObject( $result ) ) {
			$filesObjs[$row->fa_id] = ArchivedFile::newFromRow( $row );
		}
		// To work!
		foreach( $items as $fileid ) {
			if( !isset($filesObjs[$fileid]) ) {
				$success = false;
				continue; // Must exist
			} else if( !$filesObjs[$fileid]->userCan(File::DELETED_RESTRICTED) ) {
    			$userAllowedAll=false;
				continue;
			}
			// Which revisions did we change anything about?
			if( $filesObjs[$fileid]->deleted != $bitfield ) {
			   $Id_set[]=$fileid;
			   $count++;

			   $this->updateArchFiles( $filesObjs[$fileid], $bitfield );
			}
		}
		// Log if something was changed
		if( $count > 0 ) {
			$this->updateLog( $title, $count, $bitfield, $comment,
				$filesObjs[$fileid]->deleted, $title, 'fileid', $Id_set );
		}
		// Where all revs allowed to be set?
		if( !$userAllowedAll ) {
			$wgOut->permissionRequired( 'suppressrevision' );
			return false;
		}

		return $success;
	}

	/**
	 * @param $title, the log page these events apply to
	 * @param array $items list of log ID numbers
	 * @param int $bitfield new log_deleted value
	 * @param string $comment Comment for log records
	 */
	function setEventVisibility( $title, $items, $bitfield, $comment ) {
		global $wgOut;

		$userAllowedAll = $success = true;
		$count = 0;
		$log_Ids = array();

		// Run through and pull all our data in one query
		foreach( $items as $logid ) {
			$where[] = intval($logid);
		}
		list($log,$logtype) = explode( '/',$title->getDBKey(), 2 );
		$whereClause = "log_type ='$logtype' AND log_id IN(" . implode(',',$where) . ")";
		$result = $this->dbw->select( 'logging', '*',
			array( $whereClause ),
			__METHOD__ );
		while( $row = $this->dbw->fetchObject( $result ) ) {
			$logRows[$row->log_id] = $row;
		}
		// To work!
		foreach( $items as $logid ) {
			if( !isset($logRows[$logid]) ) {
				$success = false;
				continue; // Must exist
			} else if( !LogEventsList::userCan($logRows[$logid], LogPage::DELETED_RESTRICTED)
				 || $logRows[$logid]->log_type == 'suppress' ) {
			// Don't hide from oversight log!!!
    			$userAllowedAll=false;
    			continue;
			}
			// Which logs did we change anything about?
			if( $logRows[$logid]->log_deleted != $bitfield ) {
				$log_Ids[]=$logid;
				$count++;

			   	$this->updateLogs( $logRows[$logid], $bitfield );
				$this->updateRecentChangesLog( $logRows[$logid], $bitfield, true );
			}
		}
		// Don't log or touch if nothing changed
		if( $count > 0 ) {
			$this->updateLog( $title, $count, $bitfield, $logRows[$logid]->log_deleted,
				$comment, $title, 'logid', $log_Ids );
		}
		// Were all revs allowed to be set?
		if( !$userAllowedAll ) {
			$wgOut->permissionRequired( 'suppressrevision' );
			return false;
		}

		return $success;
	}

	/**
	 * Moves an image to a safe private location
	 * Caller is responsible for clearing caches
	 * @param File $oimage
	 * @returns mixed, timestamp string on success, false on failure
	 */
	function makeOldImagePrivate( $oimage ) {
		$transaction = new FSTransaction();
		if( !FileStore::lock() ) {
			wfDebug( __METHOD__.": failed to acquire file store lock, aborting\n" );
			return false;
		}
		$oldpath = $oimage->getArchivePath() . DIRECTORY_SEPARATOR . $oimage->archive_name;
		// Dupe the file into the file store
		if( file_exists( $oldpath ) ) {
			// Is our directory configured?
			if( $store = FileStore::get( 'deleted' ) ) {
				if( !$oimage->sha1 ) {
					$oimage->upgradeRow(); // sha1 may be missing
				}
				$key = $oimage->sha1 . '.' . $oimage->getExtension();
				$transaction->add( $store->insert( $key, $oldpath, FileStore::DELETE_ORIGINAL ) );
			} else {
				$group = null;
				$key = null;
				$transaction = false; // Return an error and do nothing
			}
		} else {
			wfDebug( __METHOD__." deleting already-missing '$oldpath'; moving on to database\n" );
			$group = null;
			$key = '';
			$transaction = new FSTransaction(); // empty
		}

		if( $transaction === false ) {
			// Fail to restore?
			wfDebug( __METHOD__.": import to file store failed, aborting\n" );
			throw new MWException( "Could not archive and delete file $oldpath" );
			return false;
		}

		wfDebug( __METHOD__.": set db items, applying file transactions\n" );
		$transaction->commit();
		FileStore::unlock();

		$m = explode('!',$oimage->archive_name,2);
		$timestamp = $m[0];

		return $timestamp;
	}

	/**
	 * Moves an image from a safe private location
	 * Caller is responsible for clearing caches
	 * @param File $oimage
	 * @returns mixed, string timestamp on success, false on failure
	 */
	function makeOldImagePublic( $oimage ) {
		$transaction = new FSTransaction();
		if( !FileStore::lock() ) {
			wfDebug( __METHOD__." could not acquire filestore lock\n" );
			return false;
		}

		$store = FileStore::get( 'deleted' );
		if( !$store ) {
			wfDebug( __METHOD__.": skipping row with no file.\n" );
			return false;
		}

		$key = $oimage->sha1.'.'.$oimage->getExtension();
		$destDir = $oimage->getArchivePath();
		if( !is_dir( $destDir ) ) {
			wfMkdirParents( $destDir );
		}
		$destPath = $destDir . DIRECTORY_SEPARATOR . $oimage->archive_name;
		// Check if any other stored revisions use this file;
		// if so, we shouldn't remove the file from the hidden
		// archives so they will still work. Check hidden files first.
		$useCount = $this->dbw->selectField( 'oldimage', '1',
			array( 'oi_sha1' => $oimage->sha1,
				'oi_deleted & '.File::DELETED_FILE => File::DELETED_FILE ),
			__METHOD__, array( 'FOR UPDATE' ) );
		// Check the rest of the deleted archives too.
		// (these are the ones that don't show in the image history)
		if( !$useCount ) {
			$useCount = $this->dbw->selectField( 'filearchive', '1',
				array( 'fa_storage_group' => 'deleted', 'fa_storage_key' => $key ),
				__METHOD__, array( 'FOR UPDATE' ) );
		}

		if( $useCount == 0 ) {
			wfDebug( __METHOD__.": nothing else using {$oimage->sha1}, will deleting after\n" );
			$flags = FileStore::DELETE_ORIGINAL;
		} else {
			$flags = 0;
		}
		$transaction->add( $store->export( $key, $destPath, $flags ) );

		wfDebug( __METHOD__.": set db items, applying file transactions\n" );
		$transaction->commit();
		FileStore::unlock();

		$m = explode('!',$oimage->archive_name,2);
		$timestamp = $m[0];

		return $timestamp;
	}

	/**
	 * Update the revision's rev_deleted field
	 * @param Revision $rev
	 * @param int $bitfield new rev_deleted bitfield value
	 */
	function updateRevision( $rev, $bitfield ) {
		$this->dbw->update( 'revision',
			array( 'rev_deleted' => $bitfield ),
			array( 'rev_id' => $rev->getId(),
				'rev_page' => $rev->getPage() ),
			__METHOD__ );
	}

	/**
	 * Update the revision's rev_deleted field
	 * @param Revision $rev
	 * @param Title $title
	 * @param int $bitfield new rev_deleted bitfield value
	 */
	function updateArchive( $rev, $title, $bitfield ) {
		$this->dbw->update( 'archive',
			array( 'ar_deleted' => $bitfield ),
			array( 'ar_namespace' => $title->getNamespace(),
				'ar_title'     => $title->getDBKey(),
				'ar_timestamp' => $this->dbw->timestamp( $rev->getTimestamp() ),
				'ar_rev_id' => $rev->getId() ),
			__METHOD__ );
	}

	/**
	 * Update the images's oi_deleted field
	 * @param File $file
	 * @param int $bitfield new rev_deleted bitfield value
	 */
	function updateOldFiles( $file, $bitfield ) {
		$this->dbw->update( 'oldimage',
			array( 'oi_deleted' => $bitfield ),
			array( 'oi_name' => $file->getName(),
				'oi_timestamp' => $this->dbw->timestamp( $file->getTimestamp() ) ),
			__METHOD__ );
	}

	/**
	 * Update the images's fa_deleted field
	 * @param ArchivedFile $file
	 * @param int $bitfield new rev_deleted bitfield value
	 */
	function updateArchFiles( $file, $bitfield ) {
		$this->dbw->update( 'filearchive',
			array( 'fa_deleted' => $bitfield ),
			array( 'fa_id' => $file->getId() ),
			__METHOD__ );
	}

	/**
	 * Update the logging log_deleted field
	 * @param Row $row
	 * @param int $bitfield new rev_deleted bitfield value
	 */
	function updateLogs( $row, $bitfield ) {
		$this->dbw->update( 'logging',
			array( 'log_deleted' => $bitfield ),
			array( 'log_id' => $row->log_id ),
			__METHOD__ );
	}

	/**
	 * Update the revision's recentchanges record if fields have been hidden
	 * @param Revision $rev
	 * @param int $bitfield new rev_deleted bitfield value
	 */
	function updateRecentChangesEdits( $rev, $bitfield ) {
		$this->dbw->update( 'recentchanges',
			array( 'rc_deleted' => $bitfield,
				   'rc_patrolled' => 1 ),
			array( 'rc_this_oldid' => $rev->getId(),
				'rc_timestamp' => $this->dbw->timestamp( $rev->getTimestamp() ) ),
			__METHOD__ );
	}

	/**
	 * Update the revision's recentchanges record if fields have been hidden
	 * @param Row $row
	 * @param int $bitfield new rev_deleted bitfield value
	 */
	function updateRecentChangesLog( $row, $bitfield ) {
		$this->dbw->update( 'recentchanges',
			array( 'rc_deleted' => $bitfield,
				   'rc_patrolled' => 1 ),
			array( 'rc_logid' => $row->log_id,
				'rc_timestamp' => $row->log_timestamp ),
			__METHOD__ );
	}

	/**
	 * Touch the page's cache invalidation timestamp; this forces cached
	 * history views to refresh, so any newly hidden or shown fields will
	 * update properly.
	 * @param Title $title
	 */
	function updatePage( $title ) {
		$title->invalidateCache();
		$title->purgeSquid();

		// Extensions that require referencing previous revisions may need this
		wfRunHooks( 'ArticleRevisionVisiblitySet', array( &$title ) );
	}

	/**
	 * Checks for a change in the bitfield for a certain option and updates the
	 * provided array accordingly.
	 *
	 * @param String $desc Description to add to the array if the option was
	 * enabled / disabled.
	 * @param int $field The bitmask describing the single option.
	 * @param int $diff The xor of the old and new bitfields.
	 * @param array $arr The array to update.
	 */
	function checkItem ( $desc, $field, $diff, $new, &$arr ) {
		if ( $diff & $field ) {
			$arr [ ( $new & $field ) ? 0 : 1 ][] = $desc;
		}
	}

	/**
	 * Gets an array describing the changes made to the visibilit of the revision.
	 * If the resulting array is $arr, then $arr[0] will contain an array of strings
	 * describing the items that were hidden, $arr[2] will contain an array of strings
	 * describing the items that were unhidden, and $arr[3] will contain an array with
	 * a single string, which can be one of "applied restrictions to sysops",
	 * "removed restrictions from sysops", or null.
	 *
	 * @param int $n The new bitfield.
	 * @param int $o The old bitfield.
	 * @return An array as described above.
	 */
	function getChanges ( $n, $o ) {
		$diff = $n ^ $o;
		$ret = array ( 0 => array(), 1 => array(), 2 => array() );

		$this->checkItem ( wfMsgForContent ( 'revdelete-content' ),
				Revision::DELETED_TEXT, $diff, $n, $ret );
		$this->checkItem ( wfMsgForContent ( 'revdelete-summary' ),
				Revision::DELETED_COMMENT, $diff, $n, $ret );
		$this->checkItem ( wfMsgForContent ( 'revdelete-uname' ),
				Revision::DELETED_USER, $diff, $n, $ret );

		// Restriction application to sysops
		if ( $diff & Revision::DELETED_RESTRICTED ) {
			if ( $n & Revision::DELETED_RESTRICTED )
				$ret[2][] = wfMsgForContent ( 'revdelete-restricted' );
			else
				$ret[2][] = wfMsgForContent ( 'revdelete-unrestricted' );
		}

		return $ret;
	}

	/**
	 * Gets a log message to describe the given revision visibility change. This
	 * message will be of the form "[hid {content, edit summary, username}];
	 * [unhid {...}][applied restrictions to sysops] for $count revisions: $comment".
	 *
	 * @param int $count The number of effected revisions.
	 * @param int $nbitfield The new bitfield for the revision.
	 * @param int $obitfield The old bitfield for the revision.
	 * @param string $comment The comment associated with the change.
	 * @param bool $isForLog
	 */
	function getLogMessage ( $count, $nbitfield, $obitfield, $comment, $isForLog = false ) {
		global $wgContLang;

		$s = '';
		$changes = $this->getChanges( $nbitfield, $obitfield );

		if ( count ( $changes[0] ) ) {
			$s .= wfMsgForContent ( 'revdelete-hid', implode ( ', ', $changes[0] ) );
		}

		if ( count ( $changes[1] ) ) {
			if ($s) $s .= '; ';

			$s .= wfMsgForContent ( 'revdelete-unhid', implode ( ', ', $changes[1] ) );
		}

		if ( count ( $changes[2] )) {
			if ($s)
				$s .= ' (' . $changes[2][0] . ')';
			else
				$s = $changes[2][0];
		}

		$msg = $isForLog ? 'logdelete-log-message' : 'revdelete-log-message';
		$ret = wfMsgExt ( $msg, array( 'parsemag', 'content' ),
			$s, $wgContLang->formatNum( $count ) );

		if ( $comment )
			$ret .= ": $comment";

		return $ret;

	}

	/**
	 * Record a log entry on the action
	 * @param Title $title, page where item was removed from
	 * @param int $count the number of revisions altered for this page
	 * @param int $nbitfield the new _deleted value
	 * @param int $obitfield the old _deleted value
	 * @param string $comment
	 * @param Title $target, the relevant page
	 * @param string $param, URL param
	 * @param Array $items
	 */
	function updateLog( $title, $count, $nbitfield, $obitfield, $comment, $target, $param, $items = array() ) {
		// Put things hidden from sysops in the oversight log
		$logtype = ( ($nbitfield | $obitfield) & Revision::DELETED_RESTRICTED ) ? 'suppress' : 'delete';
		$log = new LogPage( $logtype );

		$reason = $this->getLogMessage ( $count, $nbitfield, $obitfield, $comment, $param == 'logid' );

		if( $param == 'logid' ) {
			$params = array( implode( ',', $items) );
			$log->addEntry( 'event', $title, $reason, $params );
		} else {
			// Add params for effected page and ids
			$params = array( $param, implode( ',', $items) );
			$log->addEntry( 'revision', $title, $reason, $params );
		}
	}
}
