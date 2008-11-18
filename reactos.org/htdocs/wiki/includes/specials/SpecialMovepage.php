<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/**
 * Constructor
 */
function wfSpecialMovepage( $par = null ) {
	global $wgUser, $wgOut, $wgRequest, $action;

	# Check for database lock
	if ( wfReadOnly() ) {
		$wgOut->readOnlyPage();
		return;
	}

	$target = isset( $par ) ? $par : $wgRequest->getVal( 'target' );
	$oldTitleText = $wgRequest->getText( 'wpOldTitle', $target );
	$newTitleText = $wgRequest->getText( 'wpNewTitle' );

	$oldTitle = Title::newFromText( $oldTitleText );
	$newTitle = Title::newFromText( $newTitleText );

	if( is_null( $oldTitle ) ) {
		$wgOut->showErrorPage( 'notargettitle', 'notargettext' );
		return;
	}
	if( !$oldTitle->exists() ) {
		$wgOut->showErrorPage( 'nopagetitle', 'nopagetext' );
		return;
	}

	# Check rights
	$permErrors = $oldTitle->getUserPermissionsErrors( 'move', $wgUser );
	if( !empty( $permErrors ) ) {
		$wgOut->showPermissionsErrorPage( $permErrors );
		return;
	}

	$form = new MovePageForm( $oldTitle, $newTitle );

	if ( 'submit' == $action && $wgRequest->wasPosted()
		&& $wgUser->matchEditToken( $wgRequest->getVal( 'wpEditToken' ) ) ) {
		$form->doSubmit();
	} else {
		$form->showForm( '' );
	}
}

/**
 * HTML form for Special:Movepage
 * @ingroup SpecialPage
 */
class MovePageForm {
	var $oldTitle, $newTitle, $reason; # Text input
	var $moveTalk, $deleteAndMove, $moveSubpages, $fixRedirects;

	private $watch = false;

	function MovePageForm( $oldTitle, $newTitle ) {
		global $wgRequest;
		$target = isset($par) ? $par : $wgRequest->getVal( 'target' );
		$this->oldTitle = $oldTitle;
		$this->newTitle = $newTitle;
		$this->reason = $wgRequest->getText( 'wpReason' );
		if ( $wgRequest->wasPosted() ) {
			$this->moveTalk = $wgRequest->getBool( 'wpMovetalk', false );
			$this->fixRedirects = $wgRequest->getBool( 'wpFixRedirects', false );
		} else {
			$this->moveTalk = $wgRequest->getBool( 'wpMovetalk', true );
			$this->fixRedirects = $wgRequest->getBool( 'wpFixRedirects', true );
		}
		$this->moveSubpages = $wgRequest->getBool( 'wpMovesubpages', false );
		$this->deleteAndMove = $wgRequest->getBool( 'wpDeleteAndMove' ) && $wgRequest->getBool( 'wpConfirm' );
		$this->watch = $wgRequest->getCheck( 'wpWatch' );
	}

	function showForm( $err, $hookErr = '' ) {
		global $wgOut, $wgUser;

		$skin = $wgUser->getSkin();

		$oldTitleLink = $skin->makeLinkObj( $this->oldTitle );
		$oldTitle = $this->oldTitle->getPrefixedText();

		$wgOut->setPagetitle( wfMsg( 'move-page', $oldTitle ) );
		$wgOut->setSubtitle( wfMsg( 'move-page-backlink', $oldTitleLink ) );

		if( $this->newTitle == '' ) {
			# Show the current title as a default
			# when the form is first opened.
			$newTitle = $oldTitle;
		} else {
			if( $err == '' ) {
				$nt = Title::newFromURL( $this->newTitle );
				if( $nt ) {
					# If a title was supplied, probably from the move log revert
					# link, check for validity. We can then show some diagnostic
					# information and save a click.
					$newerr = $this->oldTitle->isValidMoveOperation( $nt );
					if( is_string( $newerr ) ) {
						$err = $newerr;
					}
				}
			}
			$newTitle = $this->newTitle;
		}

		if ( $err == 'articleexists' && $wgUser->isAllowed( 'delete' ) ) {
			$wgOut->addWikiMsg( 'delete_and_move_text', $newTitle );
			$movepagebtn = wfMsg( 'delete_and_move' );
			$submitVar = 'wpDeleteAndMove';
			$confirm = "
				<tr>
					<td></td>
					<td class='mw-input'>" .
						Xml::checkLabel( wfMsg( 'delete_and_move_confirm' ), 'wpConfirm', 'wpConfirm' ) .
					"</td>
				</tr>";
			$err = '';
		} else {
			$wgOut->addWikiMsg( 'movepagetext' );
			$movepagebtn = wfMsg( 'movepagebtn' );
			$submitVar = 'wpMove';
			$confirm = false;
		}

		$oldTalk = $this->oldTitle->getTalkPage();
		$considerTalk = ( !$this->oldTitle->isTalkPage() && $oldTalk->exists() );

		$dbr = wfGetDB( DB_SLAVE );
		$hasRedirects = $dbr->selectField( 'redirect', '1', 
			array( 
				'rd_namespace' => $this->oldTitle->getNamespace(),
				'rd_title' => $this->oldTitle->getDBkey(),
			) , __METHOD__ );
		
		if ( $considerTalk ) {
			$wgOut->addWikiMsg( 'movepagetalktext' );
		}

		$titleObj = SpecialPage::getTitleFor( 'Movepage' );
		$token = htmlspecialchars( $wgUser->editToken() );

		if ( $err != '' ) {
			$wgOut->setSubtitle( wfMsg( 'formerror' ) );
			if( $err == 'hookaborted' ) {
				$errMsg = "<p><strong class=\"error\">$hookErr</strong></p>\n";
				$wgOut->addHTML( $errMsg );
			} else {
				$wgOut->wrapWikiMsg( '<p><strong class="error">$1</strong></p>', $err );
			}
		}

		$wgOut->addHTML(
			 Xml::openElement( 'form', array( 'method' => 'post', 'action' => $titleObj->getLocalURL( 'action=submit' ), 'id' => 'movepage' ) ) .
			 Xml::openElement( 'fieldset' ) .
			 Xml::element( 'legend', null, wfMsg( 'move-page-legend' ) ) .
			 Xml::openElement( 'table', array( 'border' => '0', 'id' => 'mw-movepage-table' ) ) .
			 "<tr>
			 	<td class='mw-label'>" .
					wfMsgHtml( 'movearticle' ) .
				"</td>
				<td class='mw-input'>
					<strong>{$oldTitleLink}</strong>
				</td>
			</tr>
			<tr>
				<td class='mw-label'>" .
					Xml::label( wfMsg( 'newtitle' ), 'wpNewTitle' ) .
				"</td>
				<td class='mw-input'>" .
					Xml::input( 'wpNewTitle', 40, $newTitle, array( 'type' => 'text', 'id' => 'wpNewTitle' ) ) .
					Xml::hidden( 'wpOldTitle', $oldTitle ) .
				"</td>
			</tr>
			<tr>
				<td class='mw-label'>" .
					Xml::label( wfMsg( 'movereason' ), 'wpReason' ) .
				"</td>
				<td class='mw-input'>" .
					Xml::tags( 'textarea', array( 'name' => 'wpReason', 'id' => 'wpReason', 'cols' => 60, 'rows' => 2 ), htmlspecialchars( $this->reason ) ) .
				"</td>
			</tr>"
		);

		if( $considerTalk ) {
			$wgOut->addHTML( "
				<tr>
					<td></td>
					<td class='mw-input'>" .
						Xml::checkLabel( wfMsg( 'movetalk' ), 'wpMovetalk', 'wpMovetalk', $this->moveTalk ) .
					"</td>
				</tr>"
			);
		}

		if ( $hasRedirects ) {
			$wgOut->addHTML( "
				<tr>
					<td></td>
					<td class='mw-input' >" .
						Xml::checkLabel( wfMsg( 'fix-double-redirects' ), 'wpFixRedirects', 
							'wpFixRedirects', $this->fixRedirects ) .
					"</td>
				</td>"
			);
		}

		if( ($this->oldTitle->hasSubpages() || $this->oldTitle->getTalkPage()->hasSubpages())
		&& $this->oldTitle->userCan( 'move-subpages' ) ) {
			$wgOut->addHTML( "
				<tr>
					<td></td>
					<td class=\"mw-input\">" .
				Xml::checkLabel( wfMsgHtml(
						$this->oldTitle->hasSubpages()
						? 'move-subpages'
						: 'move-talk-subpages'
					),
					'wpMovesubpages', 'wpMovesubpages',
					# Don't check the box if we only have talk subpages to
					# move and we aren't moving the talk page.
					$this->moveSubpages && ($this->oldTitle->hasSubpages() || $this->moveTalk)
				) .
					"</td>
				</tr>"
			);
		}

		$watchChecked = $this->watch || $wgUser->getBoolOption( 'watchmoves' ) 
			|| $this->oldTitle->userIsWatching();
		$wgOut->addHTML( "
			<tr>
				<td></td>
				<td class='mw-input'>" .
					Xml::checkLabel( wfMsg( 'move-watch' ), 'wpWatch', 'watch', $watchChecked ) .
				"</td>
			</tr>
				{$confirm}
			<tr>
				<td>&nbsp;</td>
				<td class='mw-submit'>" .
					Xml::submitButton( $movepagebtn, array( 'name' => $submitVar ) ) .
				"</td>
			</tr>" .
			Xml::closeElement( 'table' ) .
			Xml::hidden( 'wpEditToken', $token ) .
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'form' ) .
			"\n"
		);

		$this->showLogFragment( $this->oldTitle, $wgOut );

	}

	function doSubmit() {
		global $wgOut, $wgUser, $wgRequest, $wgMaximumMovedPages, $wgLang;

		if ( $wgUser->pingLimiter( 'move' ) ) {
			$wgOut->rateLimited();
			return;
		}

		$ot = $this->oldTitle;
		$nt = $this->newTitle;

		# Delete to make way if requested
		if ( $wgUser->isAllowed( 'delete' ) && $this->deleteAndMove ) {
			$article = new Article( $nt );

			# Disallow deletions of big articles
			$bigHistory = $article->isBigDeletion();
			if( $bigHistory && !$nt->userCan( 'bigdelete' ) ) {
				global $wgLang, $wgDeleteRevisionsLimit;
				$this->showForm( array('delete-toobig', $wgLang->formatNum( $wgDeleteRevisionsLimit ) ) );
				return;
			}

			// This may output an error message and exit
			$article->doDelete( wfMsgForContent( 'delete_and_move_reason' ) );
		}

		# don't allow moving to pages with # in
		if ( !$nt || $nt->getFragment() != '' ) {
			$this->showForm( 'badtitletext' );
			return;
		}

		$error = $ot->moveTo( $nt, true, $this->reason );
		if ( $error !== true ) {
			# FIXME: showForm() should handle multiple errors
			call_user_func_array(array($this, 'showForm'), $error[0]);
			return;
		}

		if ( $this->fixRedirects ) {
			DoubleRedirectJob::fixRedirects( 'move', $ot, $nt );
		}

		wfRunHooks( 'SpecialMovepageAfterMove', array( &$this , &$ot , &$nt ) )	;

		$wgOut->setPagetitle( wfMsg( 'pagemovedsub' ) );

		$oldUrl = $ot->getFullUrl( 'redirect=no' );
		$newUrl = $nt->getFullUrl();
		$oldText = $ot->getPrefixedText();
		$newText = $nt->getPrefixedText();
		$oldLink = "<span class='plainlinks'>[$oldUrl $oldText]</span>";
		$newLink = "<span class='plainlinks'>[$newUrl $newText]</span>";

		$wgOut->addWikiMsg( 'movepage-moved', $oldLink, $newLink, $oldText, $newText );

		# Now we move extra pages we've been asked to move: subpages and talk
		# pages.  First, if the old page or the new page is a talk page, we
		# can't move any talk pages: cancel that.
		if( $ot->isTalkPage() || $nt->isTalkPage() ) {
			$this->moveTalk = false;
		}

		if( !$ot->userCan( 'move-subpages' ) ) {
			$this->moveSubpages = false;
		}

		# Next make a list of id's.  This might be marginally less efficient
		# than a more direct method, but this is not a highly performance-cri-
		# tical code path and readable code is more important here.
		#
		# Note: this query works nicely on MySQL 5, but the optimizer in MySQL
		# 4 might get confused.  If so, consider rewriting as a UNION.
		#
		# If the target namespace doesn't allow subpages, moving with subpages
		# would mean that you couldn't move them back in one operation, which
		# is bad.  FIXME: A specific error message should be given in this
		# case.
		$dbr = wfGetDB( DB_MASTER );
		if( $this->moveSubpages && (
			MWNamespace::hasSubpages( $nt->getNamespace() ) || (
				$this->moveTalk &&
				MWNamespace::hasSubpages( $nt->getTalkPage()->getNamespace() )
			)
		) ) {
			$conds = array(
				'page_title LIKE '.$dbr->addQuotes( $dbr->escapeLike( $ot->getDBkey() ) . '/%' )
					.' OR page_title = ' . $dbr->addQuotes( $ot->getDBkey() )
			);
			$conds['page_namespace'] = array();
			if( MWNamespace::hasSubpages( $nt->getNamespace() ) ) {
				$conds['page_namespace'] []= $ot->getNamespace();
			}
			if( $this->moveTalk && MWNamespace::hasSubpages( $nt->getTalkPage()->getNamespace() ) ) {
				$conds['page_namespace'] []= $ot->getTalkPage()->getNamespace();
			}
		} elseif( $this->moveTalk ) {
			$conds = array(
				'page_namespace' => $ot->getTalkPage()->getNamespace(),
				'page_title' => $ot->getDBKey()
			);
		} else {
			# Skip the query
			$conds = null;
		}

		$extrapages = array();
		if( !is_null( $conds ) ) {
			$extrapages = $dbr->select( 'page',
				array( 'page_id', 'page_namespace', 'page_title' ),
				$conds,
				__METHOD__
			);
		}

		$extraOutput = array();
		$skin = $wgUser->getSkin();
		$count = 1;
		foreach( $extrapages as $row ) {
			if( $row->page_id == $ot->getArticleId() ) {
				# Already did this one.
				continue;
			}

			$oldSubpage = Title::newFromRow( $row );
			$newPageName = preg_replace(
				'#^'.preg_quote( $ot->getDBKey(), '#' ).'#',
				$nt->getDBKey(),
				$oldSubpage->getDBKey()
			);
			if( $oldSubpage->isTalkPage() ) {
				$newNs = $nt->getTalkPage()->getNamespace();
			} else {
				$newNs = $nt->getSubjectPage()->getNamespace();
			}
			# Bug 14385: we need makeTitleSafe because the new page names may
			# be longer than 255 characters.
			$newSubpage = Title::makeTitleSafe( $newNs, $newPageName );
			if( !$newSubpage ) {
				$oldLink = $skin->makeKnownLinkObj( $oldSubpage );
				$extraOutput []= wfMsgHtml( 'movepage-page-unmoved', $oldLink,
					htmlspecialchars(Title::makeName( $newNs, $newPageName )));
				continue;
			}

			# This was copy-pasted from Renameuser, bleh.
			if ( $newSubpage->exists() && !$oldSubpage->isValidMoveTarget( $newSubpage ) ) {
				$link = $skin->makeKnownLinkObj( $newSubpage );
				$extraOutput []= wfMsgHtml( 'movepage-page-exists', $link );
			} else {
				$success = $oldSubpage->moveTo( $newSubpage, true, $this->reason );
				if( $success === true ) {
					if ( $this->fixRedirects ) {
						DoubleRedirectJob::fixRedirects( 'move', $oldSubpage, $newSubpage );
					}
					$oldLink = $skin->makeKnownLinkObj( $oldSubpage, '', 'redirect=no' );
					$newLink = $skin->makeKnownLinkObj( $newSubpage );
					$extraOutput []= wfMsgHtml( 'movepage-page-moved', $oldLink, $newLink );
				} else {
					$oldLink = $skin->makeKnownLinkObj( $oldSubpage );
					$newLink = $skin->makeLinkObj( $newSubpage );
					$extraOutput []= wfMsgHtml( 'movepage-page-unmoved', $oldLink, $newLink );
				}
			}

			++$count;
			if( $count >= $wgMaximumMovedPages ) {
				$extraOutput []= wfMsgExt( 'movepage-max-pages', array( 'parsemag', 'escape' ), $wgLang->formatNum( $wgMaximumMovedPages ) );
				break;
			}
		}

		if( $extraOutput !== array() ) {
			$wgOut->addHTML( "<ul>\n<li>" . implode( "</li>\n<li>", $extraOutput ) . "</li>\n</ul>" );
		}

		# Deal with watches (we don't watch subpages)
		if( $this->watch ) {
			$wgUser->addWatch( $ot );
			$wgUser->addWatch( $nt );
		} else {
			$wgUser->removeWatch( $ot );
			$wgUser->removeWatch( $nt );
		}
	}

	function showLogFragment( $title, &$out ) {
		$out->addHTML( Xml::element( 'h2', NULL, LogPage::logName( 'move' ) ) );
		LogEventsList::showLogExtract( $out, 'move', $title->getPrefixedText() );
	}

}
