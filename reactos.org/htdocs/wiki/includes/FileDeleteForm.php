<?php

/**
 * File deletion user interface
 *
 * @ingroup Media
 * @author Rob Church <robchur@gmail.com>
 */
class FileDeleteForm {

	private $title = null;
	private $file = null;

	private $oldfile = null;
	private $oldimage = '';

	/**
	 * Constructor
	 *
	 * @param File $file File we're deleting
	 */
	public function __construct( $file ) {
		$this->title = $file->getTitle();
		$this->file = $file;
	}

	/**
	 * Fulfil the request; shows the form or deletes the file,
	 * pending authentication, confirmation, etc.
	 */
	public function execute() {
		global $wgOut, $wgRequest, $wgUser;
		$this->setHeaders();

		if( wfReadOnly() ) {
			$wgOut->readOnlyPage();
			return;
		}
		$permission_errors = $this->title->getUserPermissionsErrors('delete', $wgUser);
		if (count($permission_errors)>0) {
			$wgOut->showPermissionsErrorPage( $permission_errors );
			return;
		}

		$this->oldimage = $wgRequest->getText( 'oldimage', false );
		$token = $wgRequest->getText( 'wpEditToken' );
		# Flag to hide all contents of the archived revisions
		$suppress = $wgRequest->getVal( 'wpSuppress' ) && $wgUser->isAllowed('suppressrevision');

		if( $this->oldimage && !self::isValidOldSpec($this->oldimage) ) {
			$wgOut->showUnexpectedValueError( 'oldimage', htmlspecialchars( $this->oldimage ) );
			return;
		}
		if( $this->oldimage )
			$this->oldfile = RepoGroup::singleton()->getLocalRepo()->newFromArchiveName( $this->title, $this->oldimage );

		if( !self::haveDeletableFile($this->file, $this->oldfile, $this->oldimage) ) {
			$wgOut->addHtml( $this->prepareMessage( 'filedelete-nofile' ) );
			$wgOut->addReturnTo( $this->title );
			return;
		}

		// Perform the deletion if appropriate
		if( $wgRequest->wasPosted() && $wgUser->matchEditToken( $token, $this->oldimage ) ) {
			$this->DeleteReasonList = $wgRequest->getText( 'wpDeleteReasonList' );
			$this->DeleteReason = $wgRequest->getText( 'wpReason' );
			$reason = $this->DeleteReasonList;
			if ( $reason != 'other' && $this->DeleteReason != '') {
				// Entry from drop down menu + additional comment
				$reason .= ': ' . $this->DeleteReason;
			} elseif ( $reason == 'other' ) {
				$reason = $this->DeleteReason;
			}

			$status = self::doDelete( $this->title, $this->file, $this->oldimage, $reason, $suppress );

			if( !$status->isGood() )
				$wgOut->addWikiText( $status->getWikiText( 'filedeleteerror-short', 'filedeleteerror-long' ) );
			if( $status->ok ) {
				$wgOut->setPagetitle( wfMsg( 'actioncomplete' ) );
				$wgOut->addHtml( $this->prepareMessage( 'filedelete-success' ) );
				// Return to the main page if we just deleted all versions of the
				// file, otherwise go back to the description page
				$wgOut->addReturnTo( $this->oldimage ? $this->title : Title::newMainPage() );
			}
			return;
		}

		$this->showForm();
		$this->showLogEntries();
	}

	public static function doDelete( &$title, &$file, &$oldimage, $reason, $suppress ) {
		$article = null;
		if( $oldimage ) {
			$status = $file->deleteOld( $oldimage, $reason, $suppress );
			if( $status->ok ) {
				// Need to do a log item
				$log = new LogPage( 'delete' );
				$logComment = wfMsgForContent( 'deletedrevision', $oldimage );
				if( trim( $reason ) != '' )
					$logComment .= ": {$reason}";
					$log->addEntry( 'delete', $title, $logComment );
			}
		} else {
			$status = $file->delete( $reason, $suppress );
			if( $status->ok ) {
				// Need to delete the associated article
				$article = new Article( $title );
				if( wfRunHooks('ArticleDelete', array(&$article, &$wgUser, &$reason)) ) {
					if( $article->doDeleteArticle( $reason, $suppress ) )
						wfRunHooks('ArticleDeleteComplete', array(&$article, &$wgUser, $reason));
				}
			}
		}
		if( $status->isGood() ) wfRunHooks('FileDeleteComplete', array(
			&$file, &$oldimage, &$article, &$wgUser, &$reason));

		return $status;
	}

	/**
	 * Show the confirmation form
	 */
	private function showForm() {
		global $wgOut, $wgUser, $wgRequest, $wgContLang;
		$align = $wgContLang->isRtl() ? 'left' : 'right';

		if( $wgUser->isAllowed( 'suppressrevision' ) ) {
			$suppress = "<tr id=\"wpDeleteSuppressRow\" name=\"wpDeleteSuppressRow\"><td></td><td>";
			$suppress .= Xml::checkLabel( wfMsg( 'revdelete-suppress' ), 'wpSuppress', 'wpSuppress', false, array( 'tabindex' => '2' ) );
			$suppress .= "</td></tr>";
		} else {
			$suppress = '';
		}

		$form = Xml::openElement( 'form', array( 'method' => 'post', 'action' => $this->getAction() ) ) .
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'filedelete-legend' ) ) .
			Xml::hidden( 'wpEditToken', $wgUser->editToken( $this->oldimage ) ) .
			$this->prepareMessage( 'filedelete-intro' ) .
			Xml::openElement( 'table' ) .
			"<tr>
				<td align='$align'>" .
					Xml::label( wfMsg( 'filedelete-comment' ), 'wpDeleteReasonList' ) .
				"</td>
				<td>" .
					Xml::listDropDown( 'wpDeleteReasonList',
						wfMsgForContent( 'filedelete-reason-dropdown' ),
						wfMsgForContent( 'filedelete-reason-otherlist' ), '', 'wpReasonDropDown', 1 ) .
				"</td>
			</tr>
			<tr>
				<td align='$align'>" .
					Xml::label( wfMsg( 'filedelete-otherreason' ), 'wpReason' ) .
				"</td>
				<td>" .
					Xml::input( 'wpReason', 60, $wgRequest->getText( 'wpReason' ), array( 'type' => 'text', 'maxlength' => '255', 'tabindex' => '2', 'id' => 'wpReason' ) ) .
				"</td>
			</tr>
			{$suppress}
			<tr>
				<td></td>
				<td>" .
					Xml::submitButton( wfMsg( 'filedelete-submit' ), array( 'name' => 'mw-filedelete-submit', 'id' => 'mw-filedelete-submit', 'tabindex' => '3' ) ) .
				"</td>
			</tr>" .
			Xml::closeElement( 'table' ) .
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'form' );

			if ( $wgUser->isAllowed( 'editinterface' ) ) {
				$skin = $wgUser->getSkin();
				$link = $skin->makeLink ( 'MediaWiki:Filedelete-reason-dropdown', wfMsgHtml( 'filedelete-edit-reasonlist' ) );
				$form .= '<p class="mw-filedelete-editreasons">' . $link . '</p>';
			}

		$wgOut->addHtml( $form );
	}

	/**
	 * Show deletion log fragments pertaining to the current file
	 */
	private function showLogEntries() {
		global $wgOut;
		$wgOut->addHtml( '<h2>' . htmlspecialchars( LogPage::logName( 'delete' ) ) . "</h2>\n" );
		LogEventsList::showLogExtract( $wgOut, 'delete', $this->title->getPrefixedText() );
	}

	/**
	 * Prepare a message referring to the file being deleted,
	 * showing an appropriate message depending upon whether
	 * it's a current file or an old version
	 *
	 * @param string $message Message base
	 * @return string
	 */
	private function prepareMessage( $message ) {
		global $wgLang;
		if( $this->oldimage ) {
			$url = $this->file->getArchiveUrl( $this->oldimage );
			return wfMsgExt(
				"{$message}-old", # To ensure grep will find them: 'filedelete-intro-old', 'filedelete-nofile-old', 'filedelete-success-old'
				'parse',
				$this->title->getText(),
				$wgLang->date( $this->getTimestamp(), true ),
				$wgLang->time( $this->getTimestamp(), true ),
				wfExpandUrl( $this->file->getArchiveUrl( $this->oldimage ) ) );
		} else {
			return wfMsgExt(
				$message,
				'parse',
				$this->title->getText()
			);
		}
	}

	/**
	 * Set headers, titles and other bits
	 */
	private function setHeaders() {
		global $wgOut, $wgUser;
		$wgOut->setPageTitle( wfMsg( 'filedelete', $this->title->getText() ) );
		$wgOut->setRobotPolicy( 'noindex,nofollow' );
		$wgOut->setSubtitle( wfMsg( 'filedelete-backlink', $wgUser->getSkin()->makeKnownLinkObj( $this->title ) ) );
	}

	/**
	 * Is the provided `oldimage` value valid?
	 *
	 * @return bool
	 */
	public static function isValidOldSpec($oldimage) {
		return strlen( $oldimage ) >= 16
			&& strpos( $oldimage, '/' ) === false
			&& strpos( $oldimage, '\\' ) === false;
	}

	/**
	 * Could we delete the file specified? If an `oldimage`
	 * value was provided, does it correspond to an
	 * existing, local, old version of this file?
	 *
	 * @return bool
	 */
	public static function haveDeletableFile(&$file, &$oldfile, $oldimage) {
		return $oldimage
			? $oldfile && $oldfile->exists() && $oldfile->isLocal()
			: $file && $file->exists() && $file->isLocal();
	}

	/**
	 * Prepare the form action
	 *
	 * @return string
	 */
	private function getAction() {
		$q = array();
		$q[] = 'action=delete';
		if( $this->oldimage )
			$q[] = 'oldimage=' . urlencode( $this->oldimage );
		return $this->title->getLocalUrl( implode( '&', $q ) );
	}

	/**
	 * Extract the timestamp of the old version
	 *
	 * @return string
	 */
	private function getTimestamp() {
		return $this->oldfile->getTimestamp();
	}

}
