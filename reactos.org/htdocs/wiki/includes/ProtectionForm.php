<?php
/**
 * Copyright (C) 2005 Brion Vibber <brion@pobox.com>
 * http://www.mediawiki.org/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

/**
 * @todo document, briefly.
 */
class ProtectionForm {
	var $mRestrictions = array();
	var $mReason = '';
	var $mCascade = false;
	var $mExpiry = null;
	var $mPermErrors = array();
	var $mApplicableTypes = array();

	function __construct( &$article ) {
		global $wgRequest, $wgUser;
		global $wgRestrictionTypes, $wgRestrictionLevels;
		$this->mArticle =& $article;
		$this->mTitle =& $article->mTitle;
		$this->mApplicableTypes = $this->mTitle->exists() ? $wgRestrictionTypes : array('create');

		if( $this->mTitle ) {
			$this->mTitle->loadRestrictions();

			foreach( $this->mApplicableTypes as $action ) {
				// Fixme: this form currently requires individual selections,
				// but the db allows multiples separated by commas.
				$this->mRestrictions[$action] = implode( '', $this->mTitle->getRestrictions( $action ) );
			}

			$this->mCascade = $this->mTitle->areRestrictionsCascading();

			if ( $this->mTitle->mRestrictionsExpiry == 'infinity' ) {
				$this->mExpiry = 'infinite';
			} else if ( strlen($this->mTitle->mRestrictionsExpiry) == 0 ) {
				$this->mExpiry = '';
			} else {
				// FIXME: this format is not user friendly
				$this->mExpiry = wfTimestamp( TS_ISO_8601, $this->mTitle->mRestrictionsExpiry );
			}
		}

		// The form will be available in read-only to show levels.
		$this->disabled = wfReadOnly() || ($this->mPermErrors = $this->mTitle->getUserPermissionsErrors('protect',$wgUser)) != array();
		$this->disabledAttrib = $this->disabled
			? array( 'disabled' => 'disabled' )
			: array();

		if( $wgRequest->wasPosted() ) {
			$this->mReason = $wgRequest->getText( 'mwProtect-reason' );
			$this->mCascade = $wgRequest->getBool( 'mwProtect-cascade' );
			$this->mExpiry = $wgRequest->getText( 'mwProtect-expiry' );

			foreach( $this->mApplicableTypes as $action ) {
				$val = $wgRequest->getVal( "mwProtect-level-$action" );
				if( isset( $val ) && in_array( $val, $wgRestrictionLevels ) ) {
					//prevent users from setting levels that they cannot later unset
					if( $val == 'sysop' ) {
						//special case, rewrite sysop to either protect and editprotected
						if( !$wgUser->isAllowed('protect') && !$wgUser->isAllowed('editprotected') )
							continue;
					} else {
						if( !$wgUser->isAllowed($val) )
							continue;
					}
					$this->mRestrictions[$action] = $val;
				}
			}
		}
	}

	function execute() {
		global $wgRequest, $wgOut;
		if( $wgRequest->wasPosted() ) {
			if( $this->save() ) {
				$article = new Article( $this->mTitle );
				$q = $article->isRedirect() ? 'redirect=no' : '';
				$wgOut->redirect( $this->mTitle->getFullUrl( $q ) );
			}
		} else {
			$this->show();
		}
	}

	function show( $err = null ) {
		global $wgOut, $wgUser;

		$wgOut->setRobotpolicy( 'noindex,nofollow' );

		if( is_null( $this->mTitle ) ||
			$this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
			$wgOut->showFatalError( wfMsg( 'badarticleerror' ) );
			return;
		}

		list( $cascadeSources, /* $restrictions */ ) = $this->mTitle->getCascadeProtectionSources();

		if ( "" != $err ) {
			$wgOut->setSubtitle( wfMsgHtml( 'formerror' ) );
			$wgOut->addHTML( "<p class='error'>{$err}</p>\n" );
		}

		if ( $cascadeSources && count($cascadeSources) > 0 ) {
			$titles = '';

			foreach ( $cascadeSources as $title ) {
				$titles .= '* [[:' . $title->getPrefixedText() . "]]\n";
			}

			$wgOut->wrapWikiMsg( "$1\n$titles", array( 'protect-cascadeon', count($cascadeSources) ) );
		}

		$sk = $wgUser->getSkin();
		$titleLink = $sk->makeLinkObj( $this->mTitle );
		$wgOut->setPageTitle( wfMsg( 'protect-title', $this->mTitle->getPrefixedText() ) );
		$wgOut->setSubtitle( wfMsg( 'protect-backlink', $titleLink ) );

		# Show an appropriate message if the user isn't allowed or able to change
		# the protection settings at this time
		if( $this->disabled ) {
			if( wfReadOnly() ) {
				$wgOut->readOnlyPage();
			} elseif( $this->mPermErrors ) {
				$wgOut->addWikiText( $wgOut->formatPermissionsErrorMessage( $this->mPermErrors ) );
			}
		} else {
			$wgOut->addWikiMsg( 'protect-text', $this->mTitle->getPrefixedText() );
		}

		$wgOut->addHTML( $this->buildForm() );

		$this->showLogExtract( $wgOut );
	}

	function save() {
		global $wgRequest, $wgUser, $wgOut;

		if( $this->disabled ) {
			$this->show();
			return false;
		}

		$token = $wgRequest->getVal( 'wpEditToken' );
		if( !$wgUser->matchEditToken( $token ) ) {
			$this->show( wfMsg( 'sessionfailure' ) );
			return false;
		}

		if ( strlen( $this->mExpiry ) == 0 ) {
			$this->mExpiry = 'infinite';
		}

		if ( $this->mExpiry == 'infinite' || $this->mExpiry == 'indefinite' ) {
			$expiry = Block::infinity();
		} else {
			# Convert GNU-style date, on error returns -1 for PHP <5.1 and false for PHP >=5.1
			$expiry = strtotime( $this->mExpiry );

			if ( $expiry < 0 || $expiry === false ) {
				$this->show( wfMsg( 'protect_expiry_invalid' ) );
				return false;
			}

			// Fixme: non-qualified absolute times are not in users specified timezone
			// and there isn't notice about it in the ui
			$expiry = wfTimestamp( TS_MW, $expiry );

			if ( $expiry < wfTimestampNow() ) {
				$this->show( wfMsg( 'protect_expiry_old' ) );
				return false;
			}

		}

		# They shouldn't be able to do this anyway, but just to make sure, ensure that cascading restrictions aren't being applied
		#  to a semi-protected page.
		global $wgGroupPermissions;

		$edit_restriction = $this->mRestrictions['edit'];

		if ($this->mCascade && ($edit_restriction != 'protect') &&
			!(isset($wgGroupPermissions[$edit_restriction]['protect']) && $wgGroupPermissions[$edit_restriction]['protect'] ) )
			$this->mCascade = false;

		if ($this->mTitle->exists()) {
			$ok = $this->mArticle->updateRestrictions( $this->mRestrictions, $this->mReason, $this->mCascade, $expiry );
		} else {
			$ok = $this->mTitle->updateTitleProtection( $this->mRestrictions['create'], $this->mReason, $expiry );
		}

		if( !$ok ) {
			throw new FatalError( "Unknown error at restriction save time." );
		}

		if( $wgRequest->getCheck( 'mwProtectWatch' ) ) {
			$this->mArticle->doWatch();
		} elseif( $this->mTitle->userIsWatching() ) {
			$this->mArticle->doUnwatch();
		}

		return $ok;
	}

	/**
	 * Build the input form
	 *
	 * @return $out string HTML form
	 */
	function buildForm() {
		global $wgUser;

		$out = '';
		if( !$this->disabled ) {
			$out .= $this->buildScript();
			// The submission needs to reenable the move permission selector
			// if it's in locked mode, or some browsers won't submit the data.
			$out .=	Xml::openElement( 'form', array( 'method' => 'post', 'action' => $this->mTitle->getLocalUrl( 'action=protect' ), 'id' => 'mw-Protect-Form', 'onsubmit' => 'protectEnable(true)' ) ) .
				Xml::hidden( 'wpEditToken',$wgUser->editToken() );
		}

		$out .= Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'protect-legend' ) ) .
			Xml::openElement( 'table', array( 'id' => 'mwProtectSet' ) ) .
			Xml::openElement( 'tbody' ) .
			"<tr>\n";

		foreach( $this->mRestrictions as $action => $required ) {
			/* Not all languages have V_x <-> N_x relation */
			$label = Xml::element( 'label',
					array( 'for' => "mwProtect-level-$action" ),
					wfMsg( 'restriction-' . $action ) );
			$out .= "<th>$label</th>";
		}
		$out .= "</tr>
			<tr>\n";
		foreach( $this->mRestrictions as $action => $selected ) {
			$out .= "<td>" .
					$this->buildSelector( $action, $selected ) .
				"</td>";
		}
		$out .= "</tr>\n";

		// JavaScript will add another row with a value-chaining checkbox

		$out .= Xml::closeElement( 'tbody' ) .
			Xml::closeElement( 'table' ) .
			Xml::openElement( 'table', array( 'id' => 'mw-protect-table2' ) ) .
			Xml::openElement( 'tbody' );

		if( $this->mTitle->exists() ) {
			$out .= '<tr>
					<td></td>
					<td class="mw-input">' .
						Xml::checkLabel( wfMsg( 'protect-cascade' ), 'mwProtect-cascade', 'mwProtect-cascade', $this->mCascade, $this->disabledAttrib ) .
					"</td>
				</tr>\n";
		}

		$attribs = array( 'id' => 'expires' ) + $this->disabledAttrib;
		$out .= "<tr>
				<td class='mw-label'>" .
					Xml::label( wfMsgExt( 'protectexpiry', array( 'parseinline' ) ), 'expires' ) .
				'</td>
				<td class="mw-input">' .
					Xml::input( 'mwProtect-expiry', 60, $this->mExpiry, $attribs ) .
				'</td>
			</tr>';

		if( !$this->disabled ) {
			$id = 'mwProtect-reason';
			$out .= "<tr>
					<td class='mw-label'>" .
						Xml::label( wfMsg( 'protectcomment' ), $id ) .
					'</td>
					<td class="mw-input">' .
						Xml::input( $id, 60, $this->mReason, array( 'type' => 'text', 'id' => $id, 'maxlength' => 255 ) ) .
					"</td>
				</tr>
				<tr>
					<td></td>
					<td class='mw-input'>" .
						Xml::checkLabel( wfMsg( 'watchthis' ),
							'mwProtectWatch', 'mwProtectWatch',
							$this->mTitle->userIsWatching() || $wgUser->getOption( 'watchdefault' ) ) .
					"</td>
				</tr>
				<tr>
					<td></td>
					<td class='mw-submit'>" .
						Xml::submitButton( wfMsg( 'confirm' ), array( 'id' => 'mw-Protect-submit' ) ) .
					"</td>
				</tr>\n";
		}

		$out .= Xml::closeElement( 'tbody' ) .
			Xml::closeElement( 'table' ) .
			Xml::closeElement( 'fieldset' );

		if ( !$this->disabled ) {
			$out .= Xml::closeElement( 'form' ) .
				$this->buildCleanupScript();
		}

		return $out;
	}

	function buildSelector( $action, $selected ) {
		global $wgRestrictionLevels, $wgUser;
		$id = 'mwProtect-level-' . $action;
		$attribs = array(
			'id' => $id,
			'name' => $id,
			'size' => count( $wgRestrictionLevels ),
			'onchange' => 'protectLevelsUpdate(this)',
			) + $this->disabledAttrib;

		$out = Xml::openElement( 'select', $attribs );
		foreach( $wgRestrictionLevels as $key ) {
			//don't let them choose levels above their own (aka so they can still unprotect and edit the page). but only when the form isn't disabled
			if( $key == 'sysop' ) {
				//special case, rewrite sysop to protect and editprotected
				if( !$wgUser->isAllowed('protect') && !$wgUser->isAllowed('editprotected') && !$this->disabled )
					continue;
			} else {
				if( !$wgUser->isAllowed($key) && !$this->disabled )
					continue;
			}
			$out .= Xml::option( $this->getOptionLabel( $key ), $key, $key == $selected );
		}
		$out .= Xml::closeElement( 'select' );
		return $out;
	}

	/**
	 * Prepare the label for a protection selector option
	 *
	 * @param string $permission Permission required
	 * @return string
	 */
	private function getOptionLabel( $permission ) {
		if( $permission == '' ) {
			return wfMsg( 'protect-default' );
		} else {
			$key = "protect-level-{$permission}";
			$msg = wfMsg( $key );
			if( wfEmptyMsg( $key, $msg ) )
				$msg = wfMsg( 'protect-fallback', $permission );
			return $msg;
		}
	}

	function buildScript() {
		global $wgStylePath, $wgStyleVersion;
		return Xml::tags( 'script', array(
			'type' => 'text/javascript',
			'src' => $wgStylePath . "/common/protect.js?$wgStyleVersion" ), '' );
	}

	function buildCleanupScript() {
		global $wgRestrictionLevels, $wgGroupPermissions;
		$script = 'var wgCascadeableLevels=';
		$CascadeableLevels = array();
		foreach( $wgRestrictionLevels as $key ) {
			if ( (isset($wgGroupPermissions[$key]['protect']) && $wgGroupPermissions[$key]['protect']) || $key == 'protect' ) {
				$CascadeableLevels[] = "'" . Xml::escapeJsString( $key ) . "'";
			}
		}
		$script .= "[" . implode(',',$CascadeableLevels) . "];\n";
		$script .= 'protectInitialize("mwProtectSet","' . Xml::escapeJsString( wfMsg( 'protect-unchain' ) ) . '","' . count($this->mApplicableTypes) . '")';
		return Xml::tags( 'script', array( 'type' => 'text/javascript' ), $script );
	}

	/**
	 * @param OutputPage $out
	 * @access private
	 */
	function showLogExtract( &$out ) {
		# Show relevant lines from the protection log:
		$out->addHTML( Xml::element( 'h2', null, LogPage::logName( 'protect' ) ) );
		LogEventsList::showLogExtract( $out, 'protect', $this->mTitle->getPrefixedText() );
	}
}
