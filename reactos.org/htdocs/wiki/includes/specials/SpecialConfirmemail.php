<?php

/**
 * Special page allows users to request email confirmation message, and handles
 * processing of the confirmation code when the link in the email is followed
 *
 * @ingroup SpecialPage
 * @author Brion Vibber
 * @author Rob Church <robchur@gmail.com>
 */
class EmailConfirmation extends UnlistedSpecialPage {

	/**
	 * Constructor
	 */
	public function __construct() {
		parent::__construct( 'Confirmemail' );
	}

	/**
	 * Main execution point
	 *
	 * @param $code Confirmation code passed to the page
	 */
	function execute( $code ) {
		global $wgUser, $wgOut;
		$this->setHeaders();
		if( empty( $code ) ) {
			if( $wgUser->isLoggedIn() ) {
				if( User::isValidEmailAddr( $wgUser->getEmail() ) ) {
					$this->showRequestForm();
				} else {
					$wgOut->addWikiMsg( 'confirmemail_noemail' );
				}
			} else {
				$title = SpecialPage::getTitleFor( 'Userlogin' );
				$self = SpecialPage::getTitleFor( 'Confirmemail' );
				$skin = $wgUser->getSkin();
				$llink = $skin->makeKnownLinkObj( $title, wfMsgHtml( 'loginreqlink' ), 'returnto=' . $self->getPrefixedUrl() );
				$wgOut->addHtml( wfMsgWikiHtml( 'confirmemail_needlogin', $llink ) );
			}
		} else {
			$this->attemptConfirm( $code );
		}
	}

	/**
	 * Show a nice form for the user to request a confirmation mail
	 */
	function showRequestForm() {
		global $wgOut, $wgUser, $wgLang, $wgRequest;
		if( $wgRequest->wasPosted() && $wgUser->matchEditToken( $wgRequest->getText( 'token' ) ) ) {
			$ok = $wgUser->sendConfirmationMail();
			if ( WikiError::isError( $ok ) ) {
				$wgOut->addWikiMsg( 'confirmemail_sendfailed', $ok->toString() );
			} else {
				$wgOut->addWikiMsg( 'confirmemail_sent' );
			}
		} else {
			if( $wgUser->isEmailConfirmed() ) {
				$time = $wgLang->timeAndDate( $wgUser->mEmailAuthenticated, true );
				$wgOut->addWikiMsg( 'emailauthenticated', $time );
			}
			if( $wgUser->isEmailConfirmationPending() ) {
				$wgOut->addWikiMsg( 'confirmemail_pending' );
			}
			$wgOut->addWikiMsg( 'confirmemail_text' );
			$self = SpecialPage::getTitleFor( 'Confirmemail' );
			$form  = wfOpenElement( 'form', array( 'method' => 'post', 'action' => $self->getLocalUrl() ) );
			$form .= wfHidden( 'token', $wgUser->editToken() );
			$form .= wfSubmitButton( wfMsgHtml( 'confirmemail_send' ) );
			$form .= wfCloseElement( 'form' );
			$wgOut->addHtml( $form );
		}
	}

	/**
	 * Attempt to confirm the user's email address and show success or failure
	 * as needed; if successful, take the user to log in
	 *
	 * @param $code Confirmation code
	 */
	function attemptConfirm( $code ) {
		global $wgUser, $wgOut;
		$user = User::newFromConfirmationCode( $code );
		if( is_object( $user ) ) {
			$user->confirmEmail();
			$user->saveSettings();
			$message = $wgUser->isLoggedIn() ? 'confirmemail_loggedin' : 'confirmemail_success';
			$wgOut->addWikiMsg( $message );
			if( !$wgUser->isLoggedIn() ) {
				$title = SpecialPage::getTitleFor( 'Userlogin' );
				$wgOut->returnToMain( true, $title );
			}
		} else {
			$wgOut->addWikiMsg( 'confirmemail_invalid' );
		}
	}

}

/**
 * Special page allows users to cancel an email confirmation using the e-mail
 * confirmation code
 *
 * @ingroup SpecialPage
 */
class EmailInvalidation extends UnlistedSpecialPage {

	public function __construct() {
		parent::__construct( 'Invalidateemail' );
	}

	function execute( $code ) {
		$this->setHeaders();
		$this->attemptInvalidate( $code );
	}

	/**
	 * Attempt to invalidate the user's email address and show success or failure
	 * as needed; if successful, link to main page
	 *
	 * @param $code Confirmation code
	 */
	function attemptInvalidate( $code ) {
		global $wgUser, $wgOut;
		$user = User::newFromConfirmationCode( $code );
		if( is_object( $user ) ) {
			$user->invalidateEmail();
			$user->saveSettings();
			$wgOut->addWikiMsg( 'confirmemail_invalidated' );
			if( !$wgUser->isLoggedIn() ) {
				$wgOut->returnToMain();
			}
		} else {
			$wgOut->addWikiMsg( 'confirmemail_invalid' );
		}
	}
}
