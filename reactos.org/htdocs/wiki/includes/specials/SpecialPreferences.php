<?php
/**
 * Hold things related to displaying and saving user preferences.
 * @file
 * @ingroup SpecialPage
 */

/**
 * Entry point that create the "Preferences" object
 */
function wfSpecialPreferences() {
	global $wgRequest;

	$form = new PreferencesForm( $wgRequest );
	$form->execute();
}

/**
 * Preferences form handling
 * This object will show the preferences form and can save it as well.
 * @ingroup SpecialPage
 */
class PreferencesForm {
	var $mQuickbar, $mOldpass, $mNewpass, $mRetypePass, $mStubs;
	var $mRows, $mCols, $mSkin, $mMath, $mDate, $mUserEmail, $mEmailFlag, $mNick;
	var $mUserLanguage, $mUserVariant;
	var $mSearch, $mRecent, $mRecentDays, $mHourDiff, $mSearchLines, $mSearchChars, $mAction;
	var $mReset, $mPosted, $mToggles, $mUseAjaxSearch, $mSearchNs, $mRealName, $mImageSize;
	var $mUnderline, $mWatchlistEdits;

	/**
	 * Constructor
	 * Load some values
	 */
	function PreferencesForm( &$request ) {
		global $wgContLang, $wgUser, $wgAllowRealName;

		$this->mQuickbar = $request->getVal( 'wpQuickbar' );
		$this->mOldpass = $request->getVal( 'wpOldpass' );
		$this->mNewpass = $request->getVal( 'wpNewpass' );
		$this->mRetypePass =$request->getVal( 'wpRetypePass' );
		$this->mStubs = $request->getVal( 'wpStubs' );
		$this->mRows = $request->getVal( 'wpRows' );
		$this->mCols = $request->getVal( 'wpCols' );
		$this->mSkin = $request->getVal( 'wpSkin' );
		$this->mMath = $request->getVal( 'wpMath' );
		$this->mDate = $request->getVal( 'wpDate' );
		$this->mUserEmail = $request->getVal( 'wpUserEmail' );
		$this->mRealName = $wgAllowRealName ? $request->getVal( 'wpRealName' ) : '';
		$this->mEmailFlag = $request->getCheck( 'wpEmailFlag' ) ? 0 : 1;
		$this->mNick = $request->getVal( 'wpNick' );
		$this->mUserLanguage = $request->getVal( 'wpUserLanguage' );
		$this->mUserVariant = $request->getVal( 'wpUserVariant' );
		$this->mSearch = $request->getVal( 'wpSearch' );
		$this->mRecent = $request->getVal( 'wpRecent' );
		$this->mRecentDays = $request->getVal( 'wpRecentDays' );
		$this->mHourDiff = $request->getVal( 'wpHourDiff' );
		$this->mSearchLines = $request->getVal( 'wpSearchLines' );
		$this->mSearchChars = $request->getVal( 'wpSearchChars' );
		$this->mImageSize = $request->getVal( 'wpImageSize' );
		$this->mThumbSize = $request->getInt( 'wpThumbSize' );
		$this->mUnderline = $request->getInt( 'wpOpunderline' );
		$this->mAction = $request->getVal( 'action' );
		$this->mReset = $request->getCheck( 'wpReset' );
		$this->mPosted = $request->wasPosted();
		$this->mSuccess = $request->getCheck( 'success' );
		$this->mWatchlistDays = $request->getVal( 'wpWatchlistDays' );
		$this->mWatchlistEdits = $request->getVal( 'wpWatchlistEdits' );
		$this->mUseAjaxSearch = $request->getCheck( 'wpUseAjaxSearch' );
		$this->mDisableMWSuggest = $request->getCheck( 'wpDisableMWSuggest' );

		$this->mSaveprefs = $request->getCheck( 'wpSaveprefs' ) &&
			$this->mPosted &&
			$wgUser->matchEditToken( $request->getVal( 'wpEditToken' ) );

		# User toggles  (the big ugly unsorted list of checkboxes)
		$this->mToggles = array();
		if ( $this->mPosted ) {
			$togs = User::getToggles();
			foreach ( $togs as $tname ) {
				$this->mToggles[$tname] = $request->getCheck( "wpOp$tname" ) ? 1 : 0;
			}
		}

		$this->mUsedToggles = array();

		# Search namespace options
		# Note: namespaces don't necessarily have consecutive keys
		$this->mSearchNs = array();
		if ( $this->mPosted ) {
			$namespaces = $wgContLang->getNamespaces();
			foreach ( $namespaces as $i => $namespace ) {
				if ( $i >= 0 ) {
					$this->mSearchNs[$i] = $request->getCheck( "wpNs$i" ) ? 1 : 0;
				}
			}
		}

		# Validate language
		if ( !preg_match( '/^[a-z\-]*$/', $this->mUserLanguage ) ) {
			$this->mUserLanguage = 'nolanguage';
		}

		wfRunHooks( 'InitPreferencesForm', array( $this, $request ) );
	}

	function execute() {
		global $wgUser, $wgOut;

		if ( $wgUser->isAnon() ) {
			$wgOut->showErrorPage( 'prefsnologin', 'prefsnologintext' );
			return;
		}
		if ( wfReadOnly() ) {
			$wgOut->readOnlyPage();
			return;
		}
		if ( $this->mReset ) {
			$this->resetPrefs();
			$this->mainPrefsForm( 'reset', wfMsg( 'prefsreset' ) );
		} else if ( $this->mSaveprefs ) {
			$this->savePreferences();
		} else {
			$this->resetPrefs();
			$this->mainPrefsForm( '' );
		}
	}
	/**
	 * @access private
	 */
	function validateInt( &$val, $min=0, $max=0x7fffffff ) {
		$val = intval($val);
		$val = min($val, $max);
		$val = max($val, $min);
		return $val;
	}

	/**
	 * @access private
	 */
	function validateFloat( &$val, $min, $max=0x7fffffff ) {
		$val = floatval( $val );
		$val = min( $val, $max );
		$val = max( $val, $min );
		return( $val );
	}

	/**
	 * @access private
	 */
	function validateIntOrNull( &$val, $min=0, $max=0x7fffffff ) {
		$val = trim($val);
		if($val === '') {
			return null;
		} else {
			return $this->validateInt( $val, $min, $max );
		}
	}

	/**
	 * @access private
	 */
	function validateDate( $val ) {
		global $wgLang, $wgContLang;
		if ( $val !== false && (
			in_array( $val, (array)$wgLang->getDatePreferences() ) ||
			in_array( $val, (array)$wgContLang->getDatePreferences() ) ) )
		{
			return $val;
		} else {
			return $wgLang->getDefaultDateFormat();
		}
	}

	/**
	 * Used to validate the user inputed timezone before saving it as
	 * 'timecorrection', will return '00:00' if fed bogus data.
	 * Note: It's not a 100% correct implementation timezone-wise, it will
	 * accept stuff like '14:30',
	 * @access private
	 * @param string $s the user input
	 * @return string
	 */
	function validateTimeZone( $s ) {
		if ( $s !== '' ) {
			if ( strpos( $s, ':' ) ) {
				# HH:MM
				$array = explode( ':' , $s );
				$hour = intval( $array[0] );
				$minute = intval( $array[1] );
			} else {
				$minute = intval( $s * 60 );
				$hour = intval( $minute / 60 );
				$minute = abs( $minute ) % 60;
			}
			# Max is +14:00 and min is -12:00, see:
			# http://en.wikipedia.org/wiki/Timezone
			$hour = min( $hour, 14 );
			$hour = max( $hour, -12 );
			$minute = min( $minute, 59 );
			$minute = max( $minute, 0 );
			$s = sprintf( "%02d:%02d", $hour, $minute );
		}
		return $s;
	}

	/**
	 * @access private
	 */
	function savePreferences() {
		global $wgUser, $wgOut, $wgParser;
		global $wgEnableUserEmail, $wgEnableEmail;
		global $wgEmailAuthentication, $wgRCMaxAge;
		global $wgAuth, $wgEmailConfirmToEdit;

		$wgUser->setOption( 'language', $this->mUserLanguage );
		$wgUser->setOption( 'variant', $this->mUserVariant );
		$wgUser->setOption( 'quickbar', $this->mQuickbar );
		global $wgUseTeX;
		if( $wgUseTeX ) {
			$wgUser->setOption( 'math', $this->mMath );
		}
		$wgUser->setOption( 'date', $this->validateDate( $this->mDate ) );
		$wgUser->setOption( 'searchlimit', $this->validateIntOrNull( $this->mSearch ) );
		$wgUser->setOption( 'contextlines', $this->validateIntOrNull( $this->mSearchLines ) );
		$wgUser->setOption( 'contextchars', $this->validateIntOrNull( $this->mSearchChars ) );
		$wgUser->setOption( 'rclimit', $this->validateIntOrNull( $this->mRecent ) );
		$wgUser->setOption( 'rcdays', $this->validateInt($this->mRecentDays, 1, ceil($wgRCMaxAge / (3600*24))));
		$wgUser->setOption( 'wllimit', $this->validateIntOrNull( $this->mWatchlistEdits, 0, 1000 ) );
		$wgUser->setOption( 'rows', $this->validateInt( $this->mRows, 4, 1000 ) );
		$wgUser->setOption( 'cols', $this->validateInt( $this->mCols, 4, 1000 ) );
		$wgUser->setOption( 'stubthreshold', $this->validateIntOrNull( $this->mStubs ) );
		$wgUser->setOption( 'timecorrection', $this->validateTimeZone( $this->mHourDiff, -12, 14 ) );
		$wgUser->setOption( 'imagesize', $this->mImageSize );
		$wgUser->setOption( 'thumbsize', $this->mThumbSize );
		$wgUser->setOption( 'underline', $this->validateInt($this->mUnderline, 0, 2) );
		$wgUser->setOption( 'watchlistdays', $this->validateFloat( $this->mWatchlistDays, 0, 7 ) );
		$wgUser->setOption( 'ajaxsearch', $this->mUseAjaxSearch );
		$wgUser->setOption( 'disablesuggest', $this->mDisableMWSuggest );

		# Set search namespace options
		foreach( $this->mSearchNs as $i => $value ) {
			$wgUser->setOption( "searchNs{$i}", $value );
		}

		if( $wgEnableEmail && $wgEnableUserEmail ) {
			$wgUser->setOption( 'disablemail', $this->mEmailFlag );
		}

		# Set user toggles
		foreach ( $this->mToggles as $tname => $tvalue ) {
			$wgUser->setOption( $tname, $tvalue );
		}

		$error = false;

		if( !$wgAuth->updateExternalDB( $wgUser ) ){
			$this->mainPrefsForm( 'error', wfMsg( 'externaldberror' ) );
			return;
		}

		$msg = '';
		if ( !wfRunHooks( 'SavePreferences', array( $this, $wgUser, &$msg, $oldOptions ) ) ) {
			$this->mainPrefsForm( 'error', $msg );
			return;
		}

		$wgUser->setCookies();
		$wgUser->saveSettings();

		if( $needRedirect && $error === false ) {
			$title = SpecialPage::getTitleFor( 'Preferences' );
			$wgOut->redirect( $title->getFullURL( 'success' ) );
			return;
		}

		$wgOut->parserOptions( ParserOptions::newFromUser( $wgUser ) );
		$this->mainPrefsForm( $error === false ? 'success' : 'error', $error);
	}

	/**
	 * @access private
	 */
	function resetPrefs() {
		global $wgUser, $wgLang, $wgContLang, $wgContLanguageCode, $wgAllowRealName;

		$this->mOldpass = $this->mNewpass = $this->mRetypePass = '';
		$this->mUserEmail = $wgUser->getEmail();
		$this->mUserEmailAuthenticationtimestamp = $wgUser->getEmailAuthenticationtimestamp();
		$this->mRealName = ($wgAllowRealName) ? $wgUser->getRealName() : '';

		# language value might be blank, default to content language
		$this->mUserLanguage = $wgUser->getOption( 'language', $wgContLanguageCode );

		$this->mUserVariant = $wgUser->getOption( 'variant');
		$this->mEmailFlag = $wgUser->getOption( 'disablemail' ) == 1 ? 1 : 0;
		$this->mNick = $wgUser->getOption( 'nickname' );

		$this->mQuickbar = $wgUser->getOption( 'quickbar' );
		$this->mSkin = Skin::normalizeKey( $wgUser->getOption( 'skin' ) );
		$this->mMath = $wgUser->getOption( 'math' );
		$this->mDate = $wgUser->getDatePreference();
		$this->mRows = $wgUser->getOption( 'rows' );
		$this->mCols = $wgUser->getOption( 'cols' );
		$this->mStubs = $wgUser->getOption( 'stubthreshold' );
		$this->mHourDiff = $wgUser->getOption( 'timecorrection' );
		$this->mSearch = $wgUser->getOption( 'searchlimit' );
		$this->mSearchLines = $wgUser->getOption( 'contextlines' );
		$this->mSearchChars = $wgUser->getOption( 'contextchars' );
		$this->mImageSize = $wgUser->getOption( 'imagesize' );
		$this->mThumbSize = $wgUser->getOption( 'thumbsize' );
		$this->mRecent = $wgUser->getOption( 'rclimit' );
		$this->mRecentDays = $wgUser->getOption( 'rcdays' );
		$this->mWatchlistEdits = $wgUser->getOption( 'wllimit' );
		$this->mUnderline = $wgUser->getOption( 'underline' );
		$this->mWatchlistDays = $wgUser->getOption( 'watchlistdays' );
		$this->mUseAjaxSearch = $wgUser->getBoolOption( 'ajaxsearch' );
		$this->mDisableMWSuggest = $wgUser->getBoolOption( 'disablesuggest' );

		$togs = User::getToggles();
		foreach ( $togs as $tname ) {
			$this->mToggles[$tname] = $wgUser->getOption( $tname );
		}

		$namespaces = $wgContLang->getNamespaces();
		foreach ( $namespaces as $i => $namespace ) {
			if ( $i >= NS_MAIN ) {
				$this->mSearchNs[$i] = $wgUser->getOption( 'searchNs'.$i );
			}
		}

		wfRunHooks( 'ResetPreferences', array( $this, $wgUser ) );
	}

	/**
	 * @access private
	 */
	function namespacesCheckboxes() {
		global $wgContLang;

		# Determine namespace checkboxes
		$namespaces = $wgContLang->getNamespaces();
		$r1 = null;

		foreach ( $namespaces as $i => $name ) {
			if ($i < 0)
				continue;
			$checked = $this->mSearchNs[$i] ? "checked='checked'" : '';
			$name = str_replace( '_', ' ', $namespaces[$i] );

			if ( empty($name) )
				$name = wfMsg( 'blanknamespace' );

			$r1 .= "<input type='checkbox' value='1' name='wpNs$i' id='wpNs$i' {$checked}/> <label for='wpNs$i'>{$name}</label><br />\n";
		}
		return $r1;
	}


	function getToggle( $tname, $trailer = false, $disabled = false ) {
		global $wgUser, $wgLang;

		$this->mUsedToggles[$tname] = true;
		$ttext = $wgLang->getUserToggle( $tname );

		$checked = $wgUser->getOption( $tname ) == 1 ? ' checked="checked"' : '';
		$disabled = $disabled ? ' disabled="disabled"' : '';
		$trailer = $trailer ? $trailer : '';
		return "<div class='toggle'><input type='checkbox' value='1' id=\"$tname\" name=\"wpOp$tname\"$checked$disabled />" .
			" <span class='toggletext'><label for=\"$tname\">$ttext</label>$trailer</span></div>\n";
	}

	function getToggles( $items ) {
		$out = "";
		foreach( $items as $item ) {
			if( $item === false )
				continue;
			if( is_array( $item ) ) {
				list( $key, $trailer ) = $item;
			} else {
				$key = $item;
				$trailer = false;
			}
			$out .= $this->getToggle( $key, $trailer );
		}
		return $out;
	}

	function addRow($td1, $td2) {
		return "<tr><td class='mw-label'>$td1</td><td class='mw-input'>$td2</td></tr>";
	}

	/**
	 * Helper function for user information panel
	 * @param $td1 label for an item
	 * @param $td2 item or null
	 * @param $td3 optional help or null
	 * @return xhtml block
	 */
	function tableRow( $td1, $td2 = null, $td3 = null ) {

		if ( is_null( $td3 ) ) {
			$td3 = '';
		} else {
			$td3 = Xml::tags( 'tr', null,
				Xml::tags( 'td', array( 'class' => 'pref-label', 'colspan' => '2' ), $td3 )
			);
		}

		if ( is_null( $td2 ) ) {
			$td1 = Xml::tags( 'td', array( 'class' => 'pref-label', 'colspan' => '2' ), $td1 );
			$td2 = '';
		} else {
			$td1 = Xml::tags( 'td', array( 'class' => 'pref-label' ), $td1 );
			$td2 = Xml::tags( 'td', array( 'class' => 'pref-input' ), $td2 );
		}

		return Xml::tags( 'tr', null, $td1 . $td2 ). $td3 . "\n";

	}

	/**
	 * @access private
	 */
	function mainPrefsForm( $status , $message = '' ) {
		global $wgUser, $wgOut, $wgLang, $wgContLang;
		global $wgAllowRealName, $wgImageLimits, $wgThumbLimits;
		global $wgDisableLangConversion;
		global $wgEnotifWatchlist, $wgEnotifUserTalk,$wgEnotifMinorEdits;
		global $wgRCShowWatchingUsers, $wgEnotifRevealEditorAddress;
		global $wgEnableEmail, $wgEnableUserEmail, $wgEmailAuthentication;
		global $wgContLanguageCode, $wgDefaultSkin, $wgSkipSkins, $wgAuth;
		global $wgEmailConfirmToEdit, $wgAjaxSearch, $wgEnableMWSuggest;

		$wgOut->setPageTitle( wfMsg( 'preferences' ) );
		$wgOut->setArticleRelated( false );
		$wgOut->setRobotpolicy( 'noindex,nofollow' );
		$wgOut->addScriptFile( 'prefs.js' );

		$wgOut->disallowUserJs();  # Prevent hijacked user scripts from sniffing passwords etc.

		if ( $this->mSuccess || 'success' == $status ) {
			$wgOut->wrapWikiMsg( '<div class="successbox"><strong>$1</strong></div>', 'savedprefs' );
		} else	if ( 'error' == $status ) {
			$wgOut->addWikiText( '<div class="errorbox"><strong>' . $message  . '</strong></div>' );
		} else if ( '' != $status ) {
			$wgOut->addWikiText( $message . "\n----" );
		}

		$qbs = $wgLang->getQuickbarSettings();
		$skinNames = $wgLang->getSkinNames();
		$mathopts = $wgLang->getMathNames();
		$dateopts = $wgLang->getDatePreferences();
		$togs = User::getToggles();

		$titleObj = SpecialPage::getTitleFor( 'Preferences' );

		# Pre-expire some toggles so they won't show if disabled
		$this->mUsedToggles[ 'shownumberswatching' ] = true;
		$this->mUsedToggles[ 'showupdated' ] = true;
		$this->mUsedToggles[ 'enotifwatchlistpages' ] = true;
		$this->mUsedToggles[ 'enotifusertalkpages' ] = true;
		$this->mUsedToggles[ 'enotifminoredits' ] = true;
		$this->mUsedToggles[ 'enotifrevealaddr' ] = true;
		$this->mUsedToggles[ 'ccmeonemails' ] = true;
		$this->mUsedToggles[ 'uselivepreview' ] = true;


		if ( !$this->mEmailFlag ) { $emfc = 'checked="checked"'; }
		else { $emfc = ''; }


		if ($wgEmailAuthentication && ($this->mUserEmail != '') ) {
			if( $wgUser->getEmailAuthenticationTimestamp() ) {
				$emailauthenticated = wfMsg('emailauthenticated',$wgLang->timeanddate($wgUser->getEmailAuthenticationTimestamp(), true ) ).'<br />';
				$disableEmailPrefs = false;
			} else {
				$disableEmailPrefs = true;
				$skin = $wgUser->getSkin();
				$emailauthenticated = wfMsg('emailnotauthenticated').'<br />' .
					$skin->makeKnownLinkObj( SpecialPage::getTitleFor( 'Confirmemail' ),
						wfMsg( 'emailconfirmlink' ) ) . '<br />';
			}
		} else {
			$emailauthenticated = '';
			$disableEmailPrefs = false;
		}

		if ($this->mUserEmail == '') {
			$emailauthenticated = wfMsg( 'noemailprefs' ) . '<br />';
		}

		$ps = $this->namespacesCheckboxes();

		$enotifwatchlistpages = ($wgEnotifWatchlist) ? $this->getToggle( 'enotifwatchlistpages', false, $disableEmailPrefs ) : '';
		$enotifusertalkpages = ($wgEnotifUserTalk) ? $this->getToggle( 'enotifusertalkpages', false, $disableEmailPrefs ) : '';
		$enotifminoredits = ($wgEnotifWatchlist && $wgEnotifMinorEdits) ? $this->getToggle( 'enotifminoredits', false, $disableEmailPrefs ) : '';
		$enotifrevealaddr = (($wgEnotifWatchlist || $wgEnotifUserTalk) && $wgEnotifRevealEditorAddress) ? $this->getToggle( 'enotifrevealaddr', false, $disableEmailPrefs ) : '';

		# </FIXME>

		$wgOut->addHTML(
			Xml::openElement( 'form', array(
				'action' => $titleObj->getLocalUrl(),
				'method' => 'post',
				'id'     => 'mw-preferences-form',
			) ) .
			Xml::openElement( 'div', array( 'id' => 'preferences' ) )
		);

		# User data

		$wgOut->addHTML(
			Xml::fieldset( wfMsg('prefs-personal') ) .
			Xml::openElement( 'table' ) .
			$this->tableRow( Xml::element( 'h2', null, wfMsg( 'prefs-personal' ) ) )
		);

		# Get groups to which the user belongs
		$userEffectiveGroups = $wgUser->getEffectiveGroups();
		$userEffectiveGroupsArray = array();
		foreach( $userEffectiveGroups as $ueg ) {
			if( $ueg == '*' ) {
				// Skip the default * group, seems useless here
				continue;
			}
			$userEffectiveGroupsArray[] = User::makeGroupLinkHTML( $ueg );
		}
		asort( $userEffectiveGroupsArray );

		$sk = $wgUser->getSkin();
		$toolLinks = array();
		$toolLinks[] = $sk->makeKnownLinkObj( SpecialPage::getTitleFor( 'ListGroupRights' ), wfMsg( 'listgrouprights' ) );
		# At the moment one tool link only but be prepared for the future...
		# FIXME: Add a link to Special:Userrights for users who are allowed to use it. 
		# $wgUser->isAllowed( 'userrights' ) seems to strict in some cases

		$userInformationHtml =
			$this->tableRow( wfMsgHtml( 'username' ), htmlspecialchars( $wgUser->getName() ) ) .
			$this->tableRow( wfMsgHtml( 'uid' ), htmlspecialchars( $wgUser->getId() ) ) .

			$this->tableRow(
				wfMsgExt( 'prefs-memberingroups', array( 'parseinline' ), count( $userEffectiveGroupsArray ) ),
				implode( wfMsg( 'comma-separator' ), $userEffectiveGroupsArray ) . 
				'<br />(' . implode( ' | ', $toolLinks ) . ')'
			) .

			$this->tableRow(
				wfMsgHtml( 'prefs-edits' ),
				$wgLang->formatNum( User::edits( $wgUser->getId() ) )
			);

		if( wfRunHooks( 'PreferencesUserInformationPanel', array( $this, &$userInformationHtml ) ) ) {
			$wgOut->addHtml( $userInformationHtml );
		}

		global $wgParser, $wgMaxSigChars;
		if( mb_strlen( $this->mNick ) > $wgMaxSigChars ) {
			$invalidSig = $this->tableRow(
				'&nbsp;',
				Xml::element( 'span', array( 'class' => 'error' ),
					wfMsgExt( 'badsiglength', 'parsemag', $wgLang->formatNum( $wgMaxSigChars ) ) )
			);
		} elseif( !empty( $this->mToggles['fancysig'] ) &&
			false === $wgParser->validateSig( $this->mNick ) ) {
			$invalidSig = $this->tableRow(
				'&nbsp;',
				Xml::element( 'span', array( 'class' => 'error' ), wfMsg( 'badsig' ) )
			);
		} else {
			$invalidSig = '';
		}

		list( $lsLabel, $lsSelect) = Xml::languageSelector( $this->mUserLanguage );
		$wgOut->addHTML(
			$this->tableRow( $lsLabel, $lsSelect )
		);

		/* see if there are multiple language variants to choose from*/
		if(!$wgDisableLangConversion) {
			$variants = $wgContLang->getVariants();
			$variantArray = array();

			$languages = Language::getLanguageNames( true );
			foreach($variants as $v) {
				$v = str_replace( '_', '-', strtolower($v));
				if( array_key_exists( $v, $languages ) ) {
					// If it doesn't have a name, we'll pretend it doesn't exist
					$variantArray[$v] = $languages[$v];
				}
			}

			$options = "\n";
			foreach( $variantArray as $code => $name ) {
				$selected = ($code == $this->mUserVariant);
				$options .= Xml::option( "$code - $name", $code, $selected ) . "\n";
			}

			if(count($variantArray) > 1) {
				$wgOut->addHtml(
					$this->tableRow(
						Xml::label( wfMsg( 'yourvariant' ), 'wpUserVariant' ),
						Xml::tags( 'select',
							array( 'name' => 'wpUserVariant', 'id' => 'wpUserVariant' ),
							$options
						)
					)
				);
			}
		}

		# <FIXME>
		# Enotif
		if ( $wgEnableEmail ) {

			$moreEmail = '';
			if ($wgEnableUserEmail) {
				// fixme -- the "allowemail" pseudotoggle is a hacked-together
				// inversion for the "disableemail" preference.
				$emf = wfMsg( 'allowemail' );
				$disabled = $disableEmailPrefs ? ' disabled="disabled"' : '';
				$moreEmail =
					"<input type='checkbox' $emfc $disabled value='1' name='wpEmailFlag' id='wpEmailFlag' /> <label for='wpEmailFlag'>$emf</label>" .
					$this->getToggle( 'ccmeonemails', '', $disableEmailPrefs );
			}


			$wgOut->addHTML(
				$this->tableRow( Xml::element( 'h2', null, wfMsg( 'email' ) ) ) .
				$this->tableRow(
					$emailauthenticated.
					$enotifrevealaddr.
					$enotifwatchlistpages.
					$enotifusertalkpages.
					$enotifminoredits.
					$moreEmail
				)
			);
		}
		# </FIXME>

		$wgOut->addHTML(
			Xml::closeElement( 'table' ) .
			Xml::closeElement( 'fieldset' )
		);


		# Quickbar
		#
		if ($this->mSkin == 'cologneblue' || $this->mSkin == 'standard') {
			$wgOut->addHtml( "<fieldset>\n<legend>" . wfMsg( 'qbsettings' ) . "</legend>\n" );
			for ( $i = 0; $i < count( $qbs ); ++$i ) {
				if ( $i == $this->mQuickbar ) { $checked = ' checked="checked"'; }
				else { $checked = ""; }
				$wgOut->addHTML( "<div><label><input type='radio' name='wpQuickbar' value=\"$i\"$checked />{$qbs[$i]}</label></div>\n" );
			}
			$wgOut->addHtml( "</fieldset>\n\n" );
		} else {
			# Need to output a hidden option even if the relevant skin is not in use,
			# otherwise the preference will get reset to 0 on submit
			$wgOut->addHtml( wfHidden( 'wpQuickbar', $this->mQuickbar ) );
		}

		# Math
		#
		global $wgUseTeX;
		if( $wgUseTeX ) {
			$wgOut->addHTML( "<fieldset>\n<legend>" . wfMsg('math') . '</legend>' );
			foreach ( $mathopts as $k => $v ) {
				$checked = ($k == $this->mMath);
				$wgOut->addHTML(
					Xml::openElement( 'div' ) .
					Xml::radioLabel( wfMsg( $v ), 'wpMath', $k, "mw-sp-math-$k", $checked ) .
					Xml::closeElement( 'div' ) . "\n"
				);
			}
			$wgOut->addHTML( "</fieldset>\n\n" );
		}

		# Files
		#
		$wgOut->addHTML(
			"<fieldset>\n" . Xml::element( 'legend', null, wfMsg( 'files' ) ) . "\n"
		);

		$imageLimitOptions = null;
		foreach ( $wgImageLimits as $index => $limits ) {
			$selected = ($index == $this->mImageSize);
			$imageLimitOptions .= Xml::option( "{$limits[0]}×{$limits[1]}" .
				wfMsg('unit-pixel'), $index, $selected );
		}

		$imageSizeId = 'wpImageSize';
		$wgOut->addHTML(
			"<div>" . Xml::label( wfMsg('imagemaxsize'), $imageSizeId ) . " " .
			Xml::openElement( 'select', array( 'name' => $imageSizeId, 'id' => $imageSizeId ) ) .
				$imageLimitOptions .
			Xml::closeElement( 'select' ) . "</div>\n"
		);

		$imageThumbOptions = null;
		foreach ( $wgThumbLimits as $index => $size ) {
			$selected = ($index == $this->mThumbSize);
			$imageThumbOptions .= Xml::option($size . wfMsg('unit-pixel'), $index,
				$selected);
		}

		$thumbSizeId = 'wpThumbSize';
		$wgOut->addHTML(
			"<div>" . Xml::label( wfMsg('thumbsize'), $thumbSizeId ) . " " .
			Xml::openElement( 'select', array( 'name' => $thumbSizeId, 'id' => $thumbSizeId ) ) .
				$imageThumbOptions .
			Xml::closeElement( 'select' ) . "</div>\n"
		);

		$wgOut->addHTML( "</fieldset>\n\n" );

		# Date format
		#
		# Date/Time
		#

		$wgOut->addHTML(
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'datetime' ) ) . "\n"
		);

		if ($dateopts) {
			$wgOut->addHTML(
				Xml::openElement( 'fieldset' ) .
				Xml::element( 'legend', null, wfMsg( 'dateformat' ) ) . "\n"
			);
			$idCnt = 0;
			$epoch = '20010115161234'; # Wikipedia day
			foreach( $dateopts as $key ) {
				if( $key == 'default' ) {
					$formatted = wfMsg( 'datedefault' );
				} else {
					$formatted = $wgLang->timeanddate( $epoch, false, $key );
				}
				$wgOut->addHTML(
					Xml::tags( 'div', null,
						Xml::radioLabel( $formatted, 'wpDate', $key, "wpDate$idCnt", $key == $this->mDate )
					) . "\n"
				);
				$idCnt++;
			}
			$wgOut->addHTML( Xml::closeElement( 'fieldset' ) . "\n" );
		}

		$nowlocal = $wgLang->time( $now = wfTimestampNow(), true );
		$nowserver = $wgLang->time( $now, false );

		$wgOut->addHTML(
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'timezonelegend' ) ) .
			Xml::openElement( 'table' ) .
		 	$this->addRow( wfMsg( 'servertime' ), $nowserver ) .
			$this->addRow( wfMsg( 'localtime' ), $nowlocal ) .
			$this->addRow(
				Xml::label( wfMsg( 'timezoneoffset' ), 'wpHourDiff'  ),
				Xml::input( 'wpHourDiff', 6, $this->mHourDiff, array( 'id' => 'wpHourDiff' ) ) ) .
			"<tr>
				<td></td>
				<td class='mw-submit'>" .
					Xml::element( 'input',
						array( 'type' => 'button',
							'value' => wfMsg( 'guesstimezone' ),
							'onclick' => 'javascript:guessTimezone()',
							'id' => 'guesstimezonebutton',
							'style' => 'display:none;' ) ) .
				"</td>
			</tr>" .
			Xml::closeElement( 'table' ) .
			Xml::tags( 'div', array( 'class' => 'prefsectiontip' ), wfMsgExt( 'timezonetext', 'parseinline' ) ).
			Xml::closeElement( 'fieldset' ) .
			Xml::closeElement( 'fieldset' ) . "\n\n"
		);

		# Editing
		#
		global $wgLivePreview;
		$wgOut->addHTML( '<fieldset><legend>' . wfMsg( 'textboxsize' ) . '</legend>
			<div>' .
				wfInputLabel( wfMsg( 'rows' ), 'wpRows', 'wpRows', 3, $this->mRows ) .
				' ' .
				wfInputLabel( wfMsg( 'columns' ), 'wpCols', 'wpCols', 3, $this->mCols ) .
			"</div>" .
			$this->getToggles( array(
				'editsection',
				'editsectiononrightclick',
				'editondblclick',
				'editwidth',
				'showtoolbar',
				'previewonfirst',
				'previewontop',
				'minordefault',
				'externaleditor',
				'externaldiff',
				$wgLivePreview ? 'uselivepreview' : false,
				'forceeditsummary',
			) ) . '</fieldset>'
		);

		# Recent changes
		$wgOut->addHtml( '<fieldset><legend>' . wfMsgHtml( 'prefs-rc' ) . '</legend>' );

		$rc  = '<table><tr>';
		$rc .= '<td>' . Xml::label( wfMsg( 'recentchangesdays' ), 'wpRecentDays' ) . '</td>';
		$rc .= '<td>' . Xml::input( 'wpRecentDays', 3, $this->mRecentDays, array( 'id' => 'wpRecentDays' ) ) . '</td>';
		$rc .= '</tr><tr>';
		$rc .= '<td>' . Xml::label( wfMsg( 'recentchangescount' ), 'wpRecent' ) . '</td>';
		$rc .= '<td>' . Xml::input( 'wpRecent', 3, $this->mRecent, array( 'id' => 'wpRecent' ) ) . '</td>';
		$rc .= '</tr></table>';
		$wgOut->addHtml( $rc );

		$wgOut->addHtml( '<br />' );

		$toggles[] = 'hideminor';
		if( $wgRCShowWatchingUsers )
			$toggles[] = 'shownumberswatching';
		$toggles[] = 'usenewrc';
		$wgOut->addHtml( $this->getToggles( $toggles ) );

		$wgOut->addHtml( '</fieldset>' );

		# Watchlist
		$wgOut->addHtml( '<fieldset><legend>' . wfMsgHtml( 'prefs-watchlist' ) . '</legend>' );

		$wgOut->addHtml( wfInputLabel( wfMsg( 'prefs-watchlist-days' ), 'wpWatchlistDays', 'wpWatchlistDays', 3, $this->mWatchlistDays ) );
		$wgOut->addHtml( '<br /><br />' );

		$wgOut->addHtml( $this->getToggle( 'extendwatchlist' ) );
		$wgOut->addHtml( wfInputLabel( wfMsg( 'prefs-watchlist-edits' ), 'wpWatchlistEdits', 'wpWatchlistEdits', 3, $this->mWatchlistEdits ) );
		$wgOut->addHtml( '<br /><br />' );

		$wgOut->addHtml( $this->getToggles( array( 'watchlisthideown', 'watchlisthidebots', 'watchlisthideminor' ) ) );

		if( $wgUser->isAllowed( 'createpage' ) || $wgUser->isAllowed( 'createtalk' ) )
			$wgOut->addHtml( $this->getToggle( 'watchcreations' ) );
		foreach( array( 'edit' => 'watchdefault', 'move' => 'watchmoves', 'delete' => 'watchdeletion' ) as $action => $toggle ) {
			if( $wgUser->isAllowed( $action ) )
				$wgOut->addHtml( $this->getToggle( $toggle ) );
		}
		$this->mUsedToggles['watchcreations'] = true;
		$this->mUsedToggles['watchdefault'] = true;
		$this->mUsedToggles['watchmoves'] = true;
		$this->mUsedToggles['watchdeletion'] = true;

		$wgOut->addHtml( '</fieldset>' );

		# Search
		$ajaxsearch = $wgAjaxSearch ?
			$this->addRow(
				Xml::label( wfMsg( 'useajaxsearch' ), 'wpUseAjaxSearch' ),
				Xml::check( 'wpUseAjaxSearch', $this->mUseAjaxSearch, array( 'id' => 'wpUseAjaxSearch' ) )
			) : '';
		$mwsuggest = $wgEnableMWSuggest ?
			$this->addRow(
				Xml::label( wfMsg( 'mwsuggest-disable' ), 'wpDisableMWSuggest' ),
				Xml::check( 'wpDisableMWSuggest', $this->mDisableMWSuggest, array( 'id' => 'wpDisableMWSuggest' ) )
			) : '';
		$wgOut->addHTML(
			// Elements for the search tab itself
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'searchresultshead' ) ) .
			// Elements for the search options in the search tab
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'prefs-searchoptions' ) ) .
			Xml::openElement( 'table' ) .
			$ajaxsearch .
			$this->addRow(
				Xml::label( wfMsg( 'resultsperpage' ), 'wpSearch' ),
				Xml::input( 'wpSearch', 4, $this->mSearch, array( 'id' => 'wpSearch' ) )
			) .
			$this->addRow(
				Xml::label( wfMsg( 'contextlines' ), 'wpSearchLines' ),
				Xml::input( 'wpSearchLines', 4, $this->mSearchLines, array( 'id' => 'wpSearchLines' ) )
			) .
			$this->addRow(
				Xml::label( wfMsg( 'contextchars' ), 'wpSearchChars' ),
				Xml::input( 'wpSearchChars', 4, $this->mSearchChars, array( 'id' => 'wpSearchChars' ) )
			) .
			$mwsuggest .
			Xml::closeElement( 'table' ) .
			Xml::closeElement( 'fieldset' ) .
			// Elements for the namespace options in the search tab
			Xml::openElement( 'fieldset' ) .
			Xml::element( 'legend', null, wfMsg( 'prefs-namespaces' ) ) .
			wfMsgExt( 'defaultns', array( 'parse' ) ) .
			$ps .
			Xml::closeElement( 'fieldset' ) .
			// End of the search tab
			Xml::closeElement( 'fieldset' )
		);

		# Misc
		#
		$wgOut->addHTML('<fieldset><legend>' . wfMsg('prefs-misc') . '</legend>');
		$wgOut->addHtml( '<label for="wpStubs">' . wfMsg( 'stub-threshold' ) . '</label>&nbsp;' );
		$wgOut->addHtml( Xml::input( 'wpStubs', 6, $this->mStubs, array( 'id' => 'wpStubs' ) ) );
		$msgUnderline = htmlspecialchars( wfMsg ( 'tog-underline' ) );
		$msgUnderlinenever = htmlspecialchars( wfMsg ( 'underline-never' ) );
		$msgUnderlinealways = htmlspecialchars( wfMsg ( 'underline-always' ) );
		$msgUnderlinedefault = htmlspecialchars( wfMsg ( 'underline-default' ) );
		$uopt = $wgUser->getOption("underline");
		$s0 = $uopt == 0 ? ' selected="selected"' : '';
		$s1 = $uopt == 1 ? ' selected="selected"' : '';
		$s2 = $uopt == 2 ? ' selected="selected"' : '';
		$wgOut->addHTML("
<div class='toggle'><p><label for='wpOpunderline'>$msgUnderline</label>
<select name='wpOpunderline' id='wpOpunderline'>
<option value=\"0\"$s0>$msgUnderlinenever</option>
<option value=\"1\"$s1>$msgUnderlinealways</option>
<option value=\"2\"$s2>$msgUnderlinedefault</option>
</select></p></div>");

		foreach ( $togs as $tname ) {
			if( !array_key_exists( $tname, $this->mUsedToggles ) ) {
				$wgOut->addHTML( $this->getToggle( $tname ) );
			}
		}
		$wgOut->addHTML( '</fieldset>' );

		wfRunHooks( 'RenderPreferencesForm', array( $this, $wgOut ) );

		$token = htmlspecialchars( $wgUser->editToken() );
		$skin = $wgUser->getSkin();
		$wgOut->addHTML( "
	<div id='prefsubmit'>
	<div>
		<input type='submit' name='wpSaveprefs' class='btnSavePrefs' value=\"" . wfMsgHtml( 'saveprefs' ) . '"'.$skin->tooltipAndAccesskey('save')." />
		<input type='submit' name='wpReset' value=\"" . wfMsgHtml( 'resetprefs' ) . "\" />
	</div>

	</div>

	<input type='hidden' name='wpEditToken' value=\"{$token}\" />
	</div></form>\n" );

		$wgOut->addHtml( Xml::tags( 'div', array( 'class' => "prefcache" ),
			wfMsgExt( 'clearyourcache', 'parseinline' ) )
		);
	}
}
