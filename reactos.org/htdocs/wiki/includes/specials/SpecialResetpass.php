<?php
/**
 * @file
 * @ingroup SpecialPage
 */

/** Constructor */
function wfSpecialResetpass( $par ) {
	$form = new PasswordResetForm();
	$form->execute( $par );
}

/**
 * Let users recover their password.
 * @ingroup SpecialPage
 */
class PasswordResetForm extends SpecialPage {
	function __construct( $name=null, $reset=null ) {
		if( $name !== null ) {
			$this->mName = $name;
			$this->mTemporaryPassword = $reset;
		} else {
			global $wgRequest;
			$this->mName = $wgRequest->getVal( 'wpName' );
			$this->mTemporaryPassword = $wgRequest->getVal( 'wpPassword' );
		}
	}

	/**
	 * Main execution point
	 */
	function execute( $par ) {
		global $wgUser, $wgAuth, $wgOut, $wgRequest;

		if( !$wgAuth->allowPasswordChange() ) {
			$this->error( wfMsg( 'resetpass_forbidden' ) );
			return;
		}

		if( $this->mName === null && !$wgRequest->wasPosted() ) {
			$this->error( wfMsg( 'resetpass_missing' ) );
			return;
		}

		if( $wgRequest->wasPosted() && $wgUser->matchEditToken( $wgRequest->getVal( 'token' ) ) ) {
			$newpass = $wgRequest->getVal( 'wpNewPassword' );
			$retype = $wgRequest->getVal( 'wpRetype' );
			try {
				$this->attemptReset( $newpass, $retype );
				$wgOut->addWikiMsg( 'resetpass_success' );

				$data = array(
					'action' => 'submitlogin',
					'wpName' => $this->mName,
					'wpPassword' => $newpass,
					'returnto' => $wgRequest->getVal( 'returnto' ),
				);
				if( $wgRequest->getCheck( 'wpRemember' ) ) {
					$data['wpRemember'] = 1;
				}
				$login = new LoginForm( new FauxRequest( $data, true ) );
				$login->execute();

				return;
			} catch( PasswordError $e ) {
				$this->error( $e->getMessage() );
			}
		}
		$this->showForm();
	}

	function error( $msg ) {
		global $wgOut;
		$wgOut->addHtml( '<div class="errorbox">' .
			htmlspecialchars( $msg ) .
			'</div>' );
	}

	function showForm() {
		global $wgOut, $wgUser, $wgRequest;

		$wgOut->disallowUserJs();

		$self = SpecialPage::getTitleFor( 'Resetpass' );
		$form  =
			'<div id="userloginForm">' .
			wfOpenElement( 'form',
				array(
					'method' => 'post',
					'action' => $self->getLocalUrl() ) ) .
			'<h2>' . wfMsgHtml( 'resetpass_header' ) . '</h2>' .
			'<div id="userloginprompt">' .
			wfMsgExt( 'resetpass_text', array( 'parse' ) ) .
			'</div>' .
			'<table>' .
			wfHidden( 'token', $wgUser->editToken() ) .
			wfHidden( 'wpName', $this->mName ) .
			wfHidden( 'wpPassword', $this->mTemporaryPassword ) .
			wfHidden( 'returnto', $wgRequest->getVal( 'returnto' ) ) .
			$this->pretty( array(
				array( 'wpName', 'username', 'text', $this->mName ),
				array( 'wpNewPassword', 'newpassword', 'password', '' ),
				array( 'wpRetype', 'yourpasswordagain', 'password', '' ),
			) ) .
			'<tr>' .
				'<td></td>' .
				'<td>' .
					Xml::checkLabel( wfMsg( 'remembermypassword' ),
						'wpRemember', 'wpRemember',
						$wgRequest->getCheck( 'wpRemember' ) ) .
				'</td>' .
			'</tr>' .
			'<tr>' .
				'<td></td>' .
				'<td>' .
					wfSubmitButton( wfMsgHtml( 'resetpass_submit' ) ) .
				'</td>' .
			'</tr>' .
			'</table>' .
			wfCloseElement( 'form' ) .
			'</div>';
		$wgOut->addHtml( $form );
	}

	function pretty( $fields ) {
		$out = '';
		foreach( $fields as $list ) {
			list( $name, $label, $type, $value ) = $list;
			if( $type == 'text' ) {
				$field = '<tt>' . htmlspecialchars( $value ) . '</tt>';
			} else {
				$field = Xml::input( $name, 20, $value,
					array( 'id' => $name, 'type' => $type ) );
			}
			$out .= '<tr>';
			$out .= '<td align="right">';
			$out .= Xml::label( wfMsg( $label ), $name );
			$out .= '</td>';
			$out .= '<td>';
			$out .= $field;
			$out .= '</td>';
			$out .= '</tr>';
		}
		return $out;
	}

	/**
	 * @throws PasswordError when cannot set the new password because requirements not met.
	 */
	function attemptReset( $newpass, $retype ) {
		$user = User::newFromName( $this->mName );
		if( $user->isAnon() ) {
			throw new PasswordError( 'no such user' );
		}

		if( !$user->checkTemporaryPassword( $this->mTemporaryPassword ) ) {
			throw new PasswordError( wfMsg( 'resetpass_bad_temporary' ) );
		}

		if( $newpass !== $retype ) {
			throw new PasswordError( wfMsg( 'badretype' ) );
		}

		$user->setPassword( $newpass );
		$user->saveSettings();
	}
}
