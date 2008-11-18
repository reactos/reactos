<?php

/*
 * Created on June 1, 2008
 * API for MediaWiki 1.8+
 *
 * Copyright (C) 2008 Bryan Tong Minh <Bryan.TongMinh@Gmail.com>
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
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * http://www.gnu.org/copyleft/gpl.html
 */

if (!defined('MEDIAWIKI')) {
	// Eclipse helper - will be ignored in production
	require_once ("ApiBase.php");
}


/**
 * @ingroup API
 */
class ApiEmailUser extends ApiBase {

	public function __construct($main, $action) {
		parent :: __construct($main, $action);
	}

	public function execute() {
		global $wgUser;
		$this->getMain()->requestWriteMode();
		$params = $this->extractRequestParams();
		
		// Check required parameters
		if ( !isset( $params['target'] ) )
			$this->dieUsageMsg( array( 'missingparam', 'target' ) );
		if ( !isset( $params['text'] ) )
			$this->dieUsageMsg( array( 'missingparam', 'text' ) );
		if ( !isset( $params['token'] ) )
			$this->dieUsageMsg( array( 'missingparam', 'token' ) );	
		
		// Validate target 
		$targetUser = EmailUserForm::validateEmailTarget( $params['target'] );
		if ( !( $targetUser instanceof User ) )
			$this->dieUsageMsg( array( $targetUser[0] ) );
		
		// Check permissions
		$error = EmailUserForm::getPermissionsError( $wgUser, $params['token'] );
		if ( $error )
			$this->dieUsageMsg( array( $error[0] ) );
		
			
		$form = new EmailUserForm( $targetUser, $params['text'], $params['subject'], $params['ccme'] );
		$retval = $form->doSubmit();
		if ( is_null( $retval ) )
			$result = array( 'result' => 'Success' );
		else
			$result = array( 'result' => 'Failure',
				 'message' => $retval->getMessage() );
		
		$this->getResult()->addValue( null, $this->getModuleName(), $result );
	}
	
	public function mustBePosted() { return true; }

	public function getAllowedParams() {
		return array (
			'target' => null,
			'subject' => null,
			'text' => null,
			'token' => null,
			'ccme' => false,
		);
	}

	public function getParamDescription() {
		return array (
			'target' => 'User to send email to',
			'subject' => 'Subject header',
			'text' => 'Mail body',
			// FIXME: How to properly get a token?
			'token' => 'A token previously acquired via prop=info',
			'ccme' => 'Send a copy of this mail to me',
		);
	}

	public function getDescription() {
		return array(
			'Emails a user.'
		);
	}

	protected function getExamples() {
		return array (
			'api.php?action=emailuser&target=WikiSysop&text=Content'
		);
	}

	public function getVersion() {
		return __CLASS__ . ': $Id: $';
	}
}	
	