<?php
/**
 * Special page to allow managing user group membership
 *
 * @file
 * @ingroup SpecialPage
 */

/**
 * A class to manage user levels rights.
 * @ingroup SpecialPage
 */
class UserrightsPage extends SpecialPage {
	# The target of the local right-adjuster's interest.  Can be gotten from
	# either a GET parameter or a subpage-style parameter, so have a member
	# variable for it.
	protected $mTarget;
	protected $isself = false;

	public function __construct() {
		parent::__construct( 'Userrights' );
	}

	public function isRestricted() {
		return true;
	}

	public function userCanExecute( $user ) {
		$available = $this->changeableGroups();
		return !empty( $available['add'] )
			or !empty( $available['remove'] )
			or ($this->isself and
				(!empty( $available['add-self'] )
				 or !empty( $available['remove-self'] )));
	}

	/**
	 * Manage forms to be shown according to posted data.
	 * Depending on the submit button used, call a form or a save function.
	 *
	 * @param $par Mixed: string if any subpage provided, else null
	 */
	function execute( $par ) {
		// If the visitor doesn't have permissions to assign or remove
		// any groups, it's a bit silly to give them the user search prompt.
		global $wgUser, $wgRequest;

		if( $par ) {
			$this->mTarget = $par;
		} else {
			$this->mTarget = $wgRequest->getVal( 'user' );
		}

		if (!$this->mTarget) {
			/*
			 * If the user specified no target, and they can only
			 * edit their own groups, automatically set them as the
			 * target.
			 */
			$available = $this->changeableGroups();
			if (empty($available['add']) && empty($available['remove']))
				$this->mTarget = $wgUser->getName();
		}

		if ($this->mTarget == $wgUser->getName())
			$this->isself = true;

		if( !$this->userCanExecute( $wgUser ) ) {
			// fixme... there may be intermediate groups we can mention.
			global $wgOut;
			$wgOut->showPermissionsErrorPage( array(
				$wgUser->isAnon()
					? 'userrights-nologin'
					: 'userrights-notallowed' ) );
			return;
		}

		if ( wfReadOnly() ) {
			global $wgOut;
			$wgOut->readOnlyPage();
			return;
		}

		$this->outputHeader();

		$this->setHeaders();

		// show the general form
		$this->switchForm();

		if( $wgRequest->wasPosted() ) {
			// save settings
			if( $wgRequest->getCheck( 'saveusergroups' ) ) {
				$reason = $wgRequest->getVal( 'user-reason' );
				if( $wgUser->matchEditToken( $wgRequest->getVal( 'wpEditToken' ), $this->mTarget ) ) {
					$this->saveUserGroups(
						$this->mTarget,
						$reason
					);
				}
			}
		}

		// show some more forms
		if( $this->mTarget ) {
			$this->editUserGroupsForm( $this->mTarget );
		}
	}

	/**
	 * Save user groups changes in the database.
	 * Data comes from the editUserGroupsForm() form function
	 *
	 * @param $username String: username to apply changes to.
	 * @param $reason String: reason for group change
	 * @return null
	 */
	function saveUserGroups( $username, $reason = '') {
		global $wgRequest, $wgUser, $wgGroupsAddToSelf, $wgGroupsRemoveFromSelf;

		$user = $this->fetchUser( $username );
		if( !$user ) {
			return;
		}

		$allgroups = $this->getAllGroups();
		$addgroup = array();
		$removegroup = array();

		// This could possibly create a highly unlikely race condition if permissions are changed between
		//  when the form is loaded and when the form is saved. Ignoring it for the moment.
		foreach ($allgroups as $group) {
			// We'll tell it to remove all unchecked groups, and add all checked groups.
			// Later on, this gets filtered for what can actually be removed
			if ($wgRequest->getCheck( "wpGroup-$group" )) {
				$addgroup[] = $group;
			} else {
				$removegroup[] = $group;
			}
		}

		// Validate input set...
		$changeable = $this->changeableGroups();
		if ($wgUser->getId() != 0 && $wgUser->getId() == $user->getId()) {
			$addable = array_merge($changeable['add'], $wgGroupsAddToSelf);
			$removable = array_merge($changeable['remove'], $wgGroupsRemoveFromSelf);
		} else {
			$addable = $changeable['add'];
			$removable = $changeable['remove'];
		}

		$removegroup = array_unique(
			array_intersect( (array)$removegroup, $removable ) );
		$addgroup = array_unique(
			array_intersect( (array)$addgroup, $addable ) );

		$oldGroups = $user->getGroups();
		$newGroups = $oldGroups;
		// remove then add groups
		if( $removegroup ) {
			$newGroups = array_diff($newGroups, $removegroup);
			foreach( $removegroup as $group ) {
				$user->removeGroup( $group );
			}
		}
		if( $addgroup ) {
			$newGroups = array_merge($newGroups, $addgroup);
			foreach( $addgroup as $group ) {
				$user->addGroup( $group );
			}
		}
		$newGroups = array_unique( $newGroups );

		// Ensure that caches are cleared
		$user->invalidateCache();

		wfDebug( 'oldGroups: ' . print_r( $oldGroups, true ) );
		wfDebug( 'newGroups: ' . print_r( $newGroups, true ) );
		if( $user instanceof User ) {
			// hmmm
			wfRunHooks( 'UserRights', array( &$user, $addgroup, $removegroup ) );
		}

		if( $newGroups != $oldGroups ) {
			$this->addLogEntry( $user, $oldGroups, $newGroups );
		}
	}
	
	/**
	 * Add a rights log entry for an action.
	 */
	function addLogEntry( $user, $oldGroups, $newGroups ) {
		global $wgRequest;
		$log = new LogPage( 'rights' );

		$log->addEntry( 'rights',
			$user->getUserPage(),
			$wgRequest->getText( 'user-reason' ),
			array(
				$this->makeGroupNameListForLog( $oldGroups ),
				$this->makeGroupNameListForLog( $newGroups )
			)
		);
	}

	/**
	 * Edit user groups membership
	 * @param $username String: name of the user.
	 */
	function editUserGroupsForm( $username ) {
		global $wgOut;

		$user = $this->fetchUser( $username );
		if( !$user ) {
			return;
		}

		$groups = $user->getGroups();

		$this->showEditUserGroupsForm( $user, $groups );

		// This isn't really ideal logging behavior, but let's not hide the
		// interwiki logs if we're using them as is.
		$this->showLogFragment( $user, $wgOut );
	}

	/**
	 * Normalize the input username, which may be local or remote, and
	 * return a user (or proxy) object for manipulating it.
	 *
	 * Side effects: error output for invalid access
	 * @return mixed User, UserRightsProxy, or null
	 */
	function fetchUser( $username ) {
		global $wgOut, $wgUser;

		$parts = explode( '@', $username );
		if( count( $parts ) < 2 ) {
			$name = trim( $username );
			$database = '';
		} else {
			list( $name, $database ) = array_map( 'trim', $parts );

			if( !$wgUser->isAllowed( 'userrights-interwiki' ) ) {
				$wgOut->addWikiMsg( 'userrights-no-interwiki' );
				return null;
			}
			if( !UserRightsProxy::validDatabase( $database ) ) {
				$wgOut->addWikiMsg( 'userrights-nodatabase', $database );
				return null;
			}
		}

		if( $name == '' ) {
			$wgOut->addWikiMsg( 'nouserspecified' );
			return false;
		}

		if( $name{0} == '#' ) {
			// Numeric ID can be specified...
			// We'll do a lookup for the name internally.
			$id = intval( substr( $name, 1 ) );

			if( $database == '' ) {
				$name = User::whoIs( $id );
			} else {
				$name = UserRightsProxy::whoIs( $database, $id );
			}

			if( !$name ) {
				$wgOut->addWikiMsg( 'noname' );
				return null;
			}
		}

		if( $database == '' ) {
			$user = User::newFromName( $name );
		} else {
			$user = UserRightsProxy::newFromName( $database, $name );
		}

		if( !$user || $user->isAnon() ) {
			$wgOut->addWikiMsg( 'nosuchusershort', $username );
			return null;
		}

		return $user;
	}

	function makeGroupNameList( $ids ) {
		if( empty( $ids ) ) {
			return wfMsg( 'rightsnone' );
		} else {
			return implode( ', ', $ids );
		}
	}

	function makeGroupNameListForLog( $ids ) {
		if( empty( $ids ) ) {
			return '';
		} else {
			return $this->makeGroupNameList( $ids );
		}
	}

	/**
	 * Output a form to allow searching for a user
	 */
	function switchForm() {
		global $wgOut, $wgScript;
		$wgOut->addHTML(
			Xml::openElement( 'form', array( 'method' => 'get', 'action' => $wgScript, 'name' => 'uluser', 'id' => 'mw-userrights-form1' ) ) .
			Xml::hidden( 'title',  $this->getTitle()->getPrefixedText() ) .
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', array(), wfMsg( 'userrights-lookup-user' ) ) .
			Xml::inputLabel( wfMsg( 'userrights-user-editname' ), 'user', 'username', 30, $this->mTarget ) . ' ' .
			Xml::submitButton( wfMsg( 'editusergroup' ) ) .
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'form' ) . "\n"
		);
	}

	/**
	 * Go through used and available groups and return the ones that this
	 * form will be able to manipulate based on the current user's system
	 * permissions.
	 *
	 * @param $groups Array: list of groups the given user is in
	 * @return Array:  Tuple of addable, then removable groups
	 */
	protected function splitGroups( $groups ) {
		global $wgGroupsAddToSelf, $wgGroupsRemoveFromSelf;
		list($addable, $removable) = array_values( $this->changeableGroups() );

		$removable = array_intersect(
				array_merge($this->isself ? $wgGroupsRemoveFromSelf : array(), $removable),
				$groups ); // Can't remove groups the user doesn't have
		$addable   = array_diff(
				array_merge($this->isself ? $wgGroupsAddToSelf : array(), $addable),
				$groups ); // Can't add groups the user does have

		return array( $addable, $removable );
	}

	/**
	 * Show the form to edit group memberships.
	 *
	 * @param $user      User or UserRightsProxy you're editing
	 * @param $groups    Array:  Array of groups the user is in
	 */
	protected function showEditUserGroupsForm( $user, $groups ) {
		global $wgOut, $wgUser, $wgLang;

		list( $addable, $removable ) = $this->splitGroups( $groups );

		$list = array();
		foreach( $user->getGroups() as $group )
			$list[] = self::buildGroupLink( $group );

		$grouplist = '';
		if( count( $list ) > 0 ) {
			$grouplist = wfMsgHtml( 'userrights-groupsmember' );
			$grouplist = '<p>' . $grouplist  . ' ' . $wgLang->listToText( $list ) . '</p>';
		}
		$wgOut->addHTML(
			Xml::openElement( 'form', array( 'method' => 'post', 'action' => $this->getTitle()->getLocalURL(), 'name' => 'editGroup', 'id' => 'mw-userrights-form2' ) ) .
			Xml::hidden( 'user', $this->mTarget ) .
			Xml::hidden( 'wpEditToken', $wgUser->editToken( $this->mTarget ) ) .
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', array(), wfMsg( 'userrights-editusergroup' ) ) .
			wfMsgExt( 'editinguser', array( 'parse' ), wfEscapeWikiText( $user->getName() ) ) .
			wfMsgExt( 'userrights-groups-help', array( 'parse' ) ) .
			$grouplist .
			Xml::tags( 'p', null, $this->groupCheckboxes( $groups ) ) .
			Xml::openElement( 'table', array( 'border' => '0', 'id' => 'mw-userrights-table-outer' ) ) .
				"<tr>
					<td class='mw-label'>" .
						Xml::label( wfMsg( 'userrights-reason' ), 'wpReason' ) .
					"</td>
					<td class='mw-input'>" .
						Xml::input( 'user-reason', 60, false, array( 'id' => 'wpReason', 'maxlength' => 255 ) ) .
					"</td>
				</tr>
				<tr>
					<td></td>
					<td class='mw-submit'>" .
						Xml::submitButton( wfMsg( 'saveusergroups' ), array( 'name' => 'saveusergroups' ) ) .
					"</td>
				</tr>" .
			Xml::closeElement( 'table' ) . "\n" .
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'form' ) . "\n"
		);
	}

	/**
	 * Format a link to a group description page
	 *
	 * @param $group string
	 * @return string
	 */
	private static function buildGroupLink( $group ) {
		static $cache = array();
		if( !isset( $cache[$group] ) )
			$cache[$group] = User::makeGroupLinkHtml( $group, User::getGroupName( $group ) );
		return $cache[$group];
	}
	
	/**
	 * Returns an array of all groups that may be edited
	 * @return array Array of groups that may be edited.
	 */
	 protected static function getAllGroups() {
	 	return User::getAllGroups();
	 }

	/**
	 * Adds a table with checkboxes where you can select what groups to add/remove
	 *
	 * @param $usergroups Array: groups the user belongs to
	 * @return string XHTML table element with checkboxes
	 */
	private function groupCheckboxes( $usergroups ) {
		$allgroups = $this->getAllGroups();
		$ret = '';

		$column = 1;
		$settable_col = '';
		$unsettable_col = '';

		foreach ($allgroups as $group) {
			$set = in_array( $group, $usergroups );
			# Should the checkbox be disabled?
			$disabled = !(
				( $set && $this->canRemove( $group ) ) ||
				( !$set && $this->canAdd( $group ) ) );
			# Do we need to point out that this action is irreversible?
			$irreversible = !$disabled && (
				($set && !$this->canAdd( $group )) ||
				(!$set && !$this->canRemove( $group ) ) );

			$attr = $disabled ? array( 'disabled' => 'disabled' ) : array();
			$text = $irreversible
				? wfMsgHtml( 'userrights-irreversible-marker', User::getGroupMember( $group ) )
				: User::getGroupMember( $group );
			$checkbox = Xml::checkLabel( $text, "wpGroup-$group",
				"wpGroup-$group", $set, $attr );
			$checkbox = $disabled ? Xml::tags( 'span', array( 'class' => 'mw-userrights-disabled' ), $checkbox ) : $checkbox;

			if ($disabled) {
				$unsettable_col .= "$checkbox<br />\n";
			} else {
				$settable_col .= "$checkbox<br />\n";
			}
		}

		if ($column) {
			$ret .=	Xml::openElement( 'table', array( 'border' => '0', 'class' => 'mw-userrights-groups' ) ) .
				"<tr>
";
			if( $settable_col !== '' ) {
				$ret .= xml::element( 'th', null, wfMsg( 'userrights-changeable-col' ) );
			}
			if( $unsettable_col !== '' ) {
				$ret .= xml::element( 'th', null, wfMsg( 'userrights-unchangeable-col' ) );
			}
			$ret.= "</tr>
				<tr>
";
			if( $settable_col !== '' ) {
				$ret .=
"					<td style='vertical-align:top;'>
						$settable_col
					</td>
";
			}
			if( $unsettable_col !== '' ) {
				$ret .=
"					<td style='vertical-align:top;'>
						$unsettable_col
					</td>
";
			}
			$ret .= Xml::closeElement( 'tr' ) . Xml::closeElement( 'table' );
		}

		return $ret;
	}

	/**
	 * @param  $group String: the name of the group to check
	 * @return bool Can we remove the group?
	 */
	private function canRemove( $group ) {
		// $this->changeableGroups()['remove'] doesn't work, of course. Thanks,
		// PHP.
		$groups = $this->changeableGroups();
		return in_array( $group, $groups['remove'] ) || ($this->isself && in_array( $group, $groups['remove-self'] ));
	}

	/**
	 * @param $group string: the name of the group to check
	 * @return bool Can we add the group?
	 */
	private function canAdd( $group ) {
		$groups = $this->changeableGroups();
		return in_array( $group, $groups['add'] ) || ($this->isself && in_array( $group, $groups['add-self'] ));
	}

	/**
	 * Returns an array of the groups that the user can add/remove.
	 *
	 * @return Array array( 'add' => array( addablegroups ), 'remove' => array( removablegroups ) )
	 */
	function changeableGroups() {
		global $wgUser, $wgGroupsAddToSelf, $wgGroupsRemoveFromSelf;

		if( $wgUser->isAllowed( 'userrights' ) ) {
			// This group gives the right to modify everything (reverse-
			// compatibility with old "userrights lets you change
			// everything")
			// Using array_merge to make the groups reindexed
			$all = array_merge( User::getAllGroups() );
			return array(
				'add' => $all,
				'remove' => $all,
				'add-self' => array(),
				'remove-self' => array()
			);
		}

		// Okay, it's not so simple, we will have to go through the arrays
		$groups = array(
				'add' => array(),
				'remove' => array(),
				'add-self' => $wgGroupsAddToSelf,
				'remove-self' => $wgGroupsRemoveFromSelf);
		$addergroups = $wgUser->getEffectiveGroups();

		foreach ($addergroups as $addergroup) {
			$groups = array_merge_recursive(
				$groups, $this->changeableByGroup($addergroup)
			);
			$groups['add']    = array_unique( $groups['add'] );
			$groups['remove'] = array_unique( $groups['remove'] );
		}
		return $groups;
	}

	/**
	 * Returns an array of the groups that a particular group can add/remove.
	 *
	 * @param $group String: the group to check for whether it can add/remove
	 * @return Array array( 'add' => array( addablegroups ), 'remove' => array( removablegroups ) )
	 */
	private function changeableByGroup( $group ) {
		global $wgAddGroups, $wgRemoveGroups;

		$groups = array( 'add' => array(), 'remove' => array() );
		if( empty($wgAddGroups[$group]) ) {
			// Don't add anything to $groups
		} elseif( $wgAddGroups[$group] === true ) {
			// You get everything
			$groups['add'] = User::getAllGroups();
		} elseif( is_array($wgAddGroups[$group]) ) {
			$groups['add'] = $wgAddGroups[$group];
		}

		// Same thing for remove
		if( empty($wgRemoveGroups[$group]) ) {
		} elseif($wgRemoveGroups[$group] === true ) {
			$groups['remove'] = User::getAllGroups();
		} elseif( is_array($wgRemoveGroups[$group]) ) {
			$groups['remove'] = $wgRemoveGroups[$group];
		}
		return $groups;
	}

	/**
	 * Show a rights log fragment for the specified user
	 *
	 * @param $user User to show log for
	 * @param $output OutputPage to use
	 */
	protected function showLogFragment( $user, $output ) {
		$output->addHtml( Xml::element( 'h2', null, LogPage::logName( 'rights' ) . "\n" ) );
		LogEventsList::showLogExtract( $output, 'rights', $user->getUserPage()->getPrefixedText() );
	}
}
