<?php

/**
 * File reversion user interface
 *
 * @ingroup Media
 * @author Rob Church <robchur@gmail.com>
 */
class FileRevertForm {

	protected $title = null;
	protected $file = null;
	protected $archiveName = '';
	protected $timestamp = false;
	protected $oldFile;

	/**
	 * Constructor
	 *
	 * @param File $file File we're reverting
	 */
	public function __construct( $file ) {
		$this->title = $file->getTitle();
		$this->file = $file;
	}

	/**
	 * Fulfil the request; shows the form or reverts the file,
	 * pending authentication, confirmation, etc.
	 */
	public function execute() {
		global $wgOut, $wgRequest, $wgUser, $wgLang;
		$this->setHeaders();

		if( wfReadOnly() ) {
			$wgOut->readOnlyPage();
			return;
		} elseif( !$wgUser->isLoggedIn() ) {
			$wgOut->showErrorPage( 'uploadnologin', 'uploadnologintext' );
			return;
		} elseif( !$this->title->userCan( 'edit' ) || !$this->title->userCan( 'upload' ) ) {
			// The standard read-only thing doesn't make a whole lot of sense
			// here; surely it should show the image or something? -- RC
			$article = new Article( $this->title );
			$wgOut->readOnlyPage( $article->getContent(), true );
			return;
		} elseif( $wgUser->isBlocked() ) {
			$wgOut->blockedPage();
			return;
		}

		$this->archiveName = $wgRequest->getText( 'oldimage' );
		$token = $wgRequest->getText( 'wpEditToken' );
		if( !$this->isValidOldSpec() ) {
			$wgOut->showUnexpectedValueError( 'oldimage', htmlspecialchars( $this->archiveName ) );
			return;
		}

		if( !$this->haveOldVersion() ) {
			$wgOut->addHtml( wfMsgExt( 'filerevert-badversion', 'parse' ) );
			$wgOut->returnToMain( false, $this->title );
			return;
		}

		// Perform the reversion if appropriate
		if( $wgRequest->wasPosted() && $wgUser->matchEditToken( $token, $this->archiveName ) ) {
			$source = $this->file->getArchiveVirtualUrl( $this->archiveName );
			$comment = $wgRequest->getText( 'wpComment' );
			// TODO: Preserve file properties from database instead of reloading from file
			$status = $this->file->upload( $source, $comment, $comment );
			if( $status->isGood() ) {
				$wgOut->addHtml( wfMsgExt( 'filerevert-success', 'parse', $this->title->getText(),
					$wgLang->date( $this->getTimestamp(), true ),
					$wgLang->time( $this->getTimestamp(), true ),
					wfExpandUrl( $this->file->getArchiveUrl( $this->archiveName ) ) ) );
				$wgOut->returnToMain( false, $this->title );
			} else {
				$wgOut->addWikiText( $status->getWikiText() );
			}
			return;
		}

		// Show the form
		$this->showForm();
	}

	/**
	 * Show the confirmation form
	 */
	protected function showForm() {
		global $wgOut, $wgUser, $wgRequest, $wgLang, $wgContLang;
		$timestamp = $this->getTimestamp();

		$form  = Xml::openElement( 'form', array( 'method' => 'post', 'action' => $this->getAction() ) );
		$form .= Xml::hidden( 'wpEditToken', $wgUser->editToken( $this->archiveName ) );
		$form .= '<fieldset><legend>' . wfMsgHtml( 'filerevert-legend' ) . '</legend>';
		$form .= wfMsgExt( 'filerevert-intro', 'parse', $this->title->getText(),
			$wgLang->date( $timestamp, true ), $wgLang->time( $timestamp, true ),
			wfExpandUrl( $this->file->getArchiveUrl( $this->archiveName ) ) );
		$form .= '<p>' . Xml::inputLabel( wfMsg( 'filerevert-comment' ), 'wpComment', 'wpComment',
			60, wfMsgForContent( 'filerevert-defaultcomment',
			$wgContLang->date( $timestamp, false, false ), $wgContLang->time( $timestamp, false, false ) ) ) . '</p>';
		$form .= '<p>' . Xml::submitButton( wfMsg( 'filerevert-submit' ) ) . '</p>';
		$form .= '</fieldset>';
		$form .= '</form>';

		$wgOut->addHtml( $form );
	}

	/**
	 * Set headers, titles and other bits
	 */
	protected function setHeaders() {
		global $wgOut, $wgUser;
		$wgOut->setPageTitle( wfMsg( 'filerevert', $this->title->getText() ) );
		$wgOut->setRobotPolicy( 'noindex,nofollow' );
		$wgOut->setSubtitle( wfMsg( 'filerevert-backlink', $wgUser->getSkin()->makeKnownLinkObj( $this->title ) ) );
	}

	/**
	 * Is the provided `oldimage` value valid?
	 *
	 * @return bool
	 */
	protected function isValidOldSpec() {
		return strlen( $this->archiveName ) >= 16
			&& strpos( $this->archiveName, '/' ) === false
			&& strpos( $this->archiveName, '\\' ) === false;
	}

	/**
	 * Does the provided `oldimage` value correspond
	 * to an existing, local, old version of this file?
	 *
	 * @return bool
	 */
	protected function haveOldVersion() {
		return $this->getOldFile()->exists();
	}

	/**
	 * Prepare the form action
	 *
	 * @return string
	 */
	protected function getAction() {
		$q = array();
		$q[] = 'action=revert';
		$q[] = 'oldimage=' . urlencode( $this->archiveName );
		return $this->title->getLocalUrl( implode( '&', $q ) );
	}

	/**
	 * Extract the timestamp of the old version
	 *
	 * @return string
	 */
	protected function getTimestamp() {
		if( $this->timestamp === false ) {
			$this->timestamp = $this->getOldFile()->getTimestamp();
		}
		return $this->timestamp;
	}

	protected function getOldFile() {
		if ( !isset( $this->oldFile ) ) {
			$this->oldFile = RepoGroup::singleton()->getLocalRepo()->newFromArchiveName( $this->title, $this->archiveName );
		}
		return $this->oldFile;
	}
}
