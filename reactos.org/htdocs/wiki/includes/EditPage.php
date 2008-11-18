<?php
/**
 * Contains the EditPage class
 * @file
 */

/**
 * The edit page/HTML interface (split from Article)
 * The actual database and text munging is still in Article,
 * but it should get easier to call those from alternate
 * interfaces.
 *
 * EditPage cares about two distinct titles:
 * $wgTitle is the page that forms submit to, links point to,
 * redirects go to, etc. $this->mTitle (as well as $mArticle) is the
 * page in the database that is actually being edited. These are
 * usually the same, but they are now allowed to be different.
 */
class EditPage {
	const AS_SUCCESS_UPDATE			= 200;
	const AS_SUCCESS_NEW_ARTICLE		= 201;
	const AS_HOOK_ERROR			= 210;
	const AS_FILTERING			= 211;
	const AS_HOOK_ERROR_EXPECTED		= 212;
	const AS_BLOCKED_PAGE_FOR_USER		= 215;
	const AS_CONTENT_TOO_BIG		= 216;
	const AS_USER_CANNOT_EDIT		= 217;
	const AS_READ_ONLY_PAGE_ANON		= 218;
	const AS_READ_ONLY_PAGE_LOGGED		= 219;
	const AS_READ_ONLY_PAGE			= 220;
	const AS_RATE_LIMITED			= 221;
	const AS_ARTICLE_WAS_DELETED		= 222;
	const AS_NO_CREATE_PERMISSION		= 223;
	const AS_BLANK_ARTICLE			= 224;
	const AS_CONFLICT_DETECTED		= 225;
	const AS_SUMMARY_NEEDED			= 226;
	const AS_TEXTBOX_EMPTY			= 228;
	const AS_MAX_ARTICLE_SIZE_EXCEEDED	= 229;
	const AS_OK				= 230;
	const AS_END				= 231;
	const AS_SPAM_ERROR			= 232;
	const AS_IMAGE_REDIRECT_ANON		= 233;
	const AS_IMAGE_REDIRECT_LOGGED		= 234;

	var $mArticle;
	var $mTitle;
	var $mMetaData = '';
	var $isConflict = false;
	var $isCssJsSubpage = false;
	var $deletedSinceEdit = false;
	var $formtype;
	var $firsttime;
	var $lastDelete;
	var $mTokenOk = false;
	var $mTokenOkExceptSuffix = false;
	var $mTriedSave = false;
	var $tooBig = false;
	var $kblength = false;
	var $missingComment = false;
	var $missingSummary = false;
	var $allowBlankSummary = false;
	var $autoSumm = '';
	var $hookError = '';
	var $mPreviewTemplates;
	var $mBaseRevision = false;

	# Form values
	var $save = false, $preview = false, $diff = false;
	var $minoredit = false, $watchthis = false, $recreate = false;
	var $textbox1 = '', $textbox2 = '', $summary = '';
	var $edittime = '', $section = '', $starttime = '';
	var $oldid = 0, $editintro = '', $scrolltop = null;

	# Placeholders for text injection by hooks (must be HTML)
	# extensions should take care to _append_ to the present value
	public $editFormPageTop; // Before even the preview
	public $editFormTextTop;
	public $editFormTextBeforeContent;
	public $editFormTextAfterWarn;
	public $editFormTextAfterTools;
	public $editFormTextBottom;

	/* $didSave should be set to true whenever an article was succesfully altered. */
	public $didSave = false;

	public $suppressIntro = false;

	/**
	 * @todo document
	 * @param $article
	 */
	function EditPage( $article ) {
		$this->mArticle =& $article;
		$this->mTitle = $article->getTitle();

		# Placeholders for text injection by hooks (empty per default)
		$this->editFormPageTop =
		$this->editFormTextTop =
		$this->editFormTextBeforeContent =
		$this->editFormTextAfterWarn =
		$this->editFormTextAfterTools =
		$this->editFormTextBottom = "";
	}

	/**
	 * Fetch initial editing page content.
	 * @private
	 */
	function getContent( $def_text = '' ) {
		global $wgOut, $wgRequest, $wgParser, $wgMessageCache;

		# Get variables from query string :P
		$section = $wgRequest->getVal( 'section' );
		$preload = $wgRequest->getVal( 'preload' );
		$undoafter = $wgRequest->getVal( 'undoafter' );
		$undo = $wgRequest->getVal( 'undo' );

		wfProfileIn( __METHOD__ );

		$text = '';
		if( !$this->mTitle->exists() ) {
			if ( $this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
				$wgMessageCache->loadAllMessages();
				# If this is a system message, get the default text.
				$text = wfMsgWeirdKey ( $this->mTitle->getText() ) ;
			} else {
				# If requested, preload some text.
				$text = $this->getPreloadedText( $preload );
			}
			# We used to put MediaWiki:Newarticletext here if
			# $text was empty at this point.
			# This is now shown above the edit box instead.
		} else {
			// FIXME: may be better to use Revision class directly
			// But don't mess with it just yet. Article knows how to
			// fetch the page record from the high-priority server,
			// which is needed to guarantee we don't pick up lagged
			// information.

			$text = $this->mArticle->getContent();

			if ($undo > 0 && $undoafter > 0 && $undo < $undoafter) {
				# If they got undoafter and undo round the wrong way, switch them
				list( $undo, $undoafter ) = array( $undoafter, $undo );
			}

			if ( $undo > 0 && $undo > $undoafter ) {
				# Undoing a specific edit overrides section editing; section-editing
				# doesn't work with undoing.
				if ( $undoafter ) {
					$undorev = Revision::newFromId($undo);
					$oldrev = Revision::newFromId($undoafter);
				} else {
					$undorev = Revision::newFromId($undo);
					$oldrev = $undorev ? $undorev->getPrevious() : null;
				}

				# Sanity check, make sure it's the right page,
				# the revisions exist and they were not deleted.
				# Otherwise, $text will be left as-is.
				if( !is_null( $undorev ) && !is_null( $oldrev ) &&
					$undorev->getPage() == $oldrev->getPage() &&
					$undorev->getPage() == $this->mArticle->getID() &&
					!$undorev->isDeleted( Revision::DELETED_TEXT ) &&
					!$oldrev->isDeleted( Revision::DELETED_TEXT ) ) {
					$undorev_text = $undorev->getText();
					$oldrev_text = $oldrev->getText();
					$currev_text = $text;

					if ( $currev_text != $undorev_text ) {
						$result = wfMerge( $undorev_text, $oldrev_text, $currev_text, $text );
					} else {
						# No use doing a merge if it's just a straight revert.
						$text = $oldrev_text;
						$result = true;
					}
					if( $result ) {
						# Inform the user of our success and set an automatic edit summary
						$this->editFormPageTop .= $wgOut->parse( wfMsgNoTrans( 'undo-success' ) );
						$firstrev = $oldrev->getNext();
						# If we just undid one rev, use an autosummary
						if( $firstrev->mId == $undo ) {
							$this->summary = wfMsgForContent('undo-summary', $undo, $undorev->getUserText());
						}
						$this->formtype = 'diff';
					} else {
						# Warn the user that something went wrong
						$this->editFormPageTop .= $wgOut->parse( wfMsgNoTrans( 'undo-failure' ) );
					}
				} else {
					// Failed basic sanity checks.
					// Older revisions may have been removed since the link
					// was created, or we may simply have got bogus input.
					$this->editFormPageTop .= $wgOut->parse( wfMsgNoTrans( 'undo-norev' ) );
				}
			} else if( $section != '' ) {
				if( $section == 'new' ) {
					$text = $this->getPreloadedText( $preload );
				} else {
					$text = $wgParser->getSection( $text, $section, $def_text );
				}
			}
		}

		wfProfileOut( __METHOD__ );
		return $text;
	}

	/**
	 * Get the contents of a page from its title and remove includeonly tags
	 *
	 * @param $preload String: the title of the page.
	 * @return string The contents of the page.
	 */
	protected function getPreloadedText($preload) {
		if ( $preload === '' )
			return '';
		else {
			$preloadTitle = Title::newFromText( $preload );
			if ( isset( $preloadTitle ) && $preloadTitle->userCanRead() ) {
				$rev=Revision::newFromTitle($preloadTitle);
				if ( is_object( $rev ) ) {
					$text = $rev->getText();
					// TODO FIXME: AAAAAAAAAAA, this shouldn't be implementing
					// its own mini-parser! -ævar
					$text = preg_replace( '~</?includeonly>~', '', $text );
					return $text;
				} else
					return '';
			}
		}
	}

	/**
	 * This is the function that extracts metadata from the article body on the first view.
	 * To turn the feature on, set $wgUseMetadataEdit = true ; in LocalSettings
	 *  and set $wgMetadataWhitelist to the *full* title of the template whitelist
	 */
	function extractMetaDataFromArticle () {
		global $wgUseMetadataEdit , $wgMetadataWhitelist , $wgLang ;
		$this->mMetaData = '' ;
		if ( !$wgUseMetadataEdit ) return ;
		if ( $wgMetadataWhitelist == '' ) return ;
		$s = '' ;
		$t = $this->getContent();

		# MISSING : <nowiki> filtering

		# Categories and language links
		$t = explode ( "\n" , $t ) ;
		$catlow = strtolower ( $wgLang->getNsText ( NS_CATEGORY ) ) ;
		$cat = $ll = array() ;
		foreach ( $t AS $key => $x )
		{
			$y = trim ( strtolower ( $x ) ) ;
			while ( substr ( $y , 0 , 2 ) == '[[' )
			{
				$y = explode ( ']]' , trim ( $x ) ) ;
				$first = array_shift ( $y ) ;
				$first = explode ( ':' , $first ) ;
				$ns = array_shift ( $first ) ;
				$ns = trim ( str_replace ( '[' , '' , $ns ) ) ;
				if ( strlen ( $ns ) == 2 OR strtolower ( $ns ) == $catlow )
				{
					$add = '[[' . $ns . ':' . implode ( ':' , $first ) . ']]' ;
					if ( strtolower ( $ns ) == $catlow ) $cat[] = $add ;
					else $ll[] = $add ;
					$x = implode ( ']]' , $y ) ;
					$t[$key] = $x ;
					$y = trim ( strtolower ( $x ) ) ;
				}
			}
		}
		if ( count ( $cat ) ) $s .= implode ( ' ' , $cat ) . "\n" ;
		if ( count ( $ll ) ) $s .= implode ( ' ' , $ll ) . "\n" ;
		$t = implode ( "\n" , $t ) ;

		# Load whitelist
		$sat = array () ; # stand-alone-templates; must be lowercase
		$wl_title = Title::newFromText ( $wgMetadataWhitelist ) ;
		$wl_article = new Article ( $wl_title ) ;
		$wl = explode ( "\n" , $wl_article->getContent() ) ;
		foreach ( $wl AS $x )
		{
			$isentry = false ;
			$x = trim ( $x ) ;
			while ( substr ( $x , 0 , 1 ) == '*' )
			{
				$isentry = true ;
				$x = trim ( substr ( $x , 1 ) ) ;
			}
			if ( $isentry )
			{
				$sat[] = strtolower ( $x ) ;
			}

		}

		# Templates, but only some
		$t = explode ( '{{' , $t ) ;
		$tl = array () ;
		foreach ( $t AS $key => $x )
		{
			$y = explode ( '}}' , $x , 2 ) ;
			if ( count ( $y ) == 2 )
			{
				$z = $y[0] ;
				$z = explode ( '|' , $z ) ;
				$tn = array_shift ( $z ) ;
				if ( in_array ( strtolower ( $tn ) , $sat ) )
				{
					$tl[] = '{{' . $y[0] . '}}' ;
					$t[$key] = $y[1] ;
					$y = explode ( '}}' , $y[1] , 2 ) ;
				}
				else $t[$key] = '{{' . $x ;
			}
			else if ( $key != 0 ) $t[$key] = '{{' . $x ;
			else $t[$key] = $x ;
		}
		if ( count ( $tl ) ) $s .= implode ( ' ' , $tl ) ;
		$t = implode ( '' , $t ) ;

		$t = str_replace ( "\n\n\n" , "\n" , $t ) ;
		$this->mArticle->mContent = $t ;
		$this->mMetaData = $s ;
	}

	protected function wasDeletedSinceLastEdit() {
		/* Note that we rely on the logging table, which hasn't been always there,
		 * but that doesn't matter, because this only applies to brand new
		 * deletes.
		 */
		if ( $this->deletedSinceEdit )
			return true;
		if ( $this->mTitle->isDeleted() ) {
			$this->lastDelete = $this->getLastDelete();
			if ( !is_null($this->lastDelete) ) {
				$deletetime = $this->lastDelete->log_timestamp;
				if ( ($deletetime - $this->starttime) > 0 ) {
					$this->deletedSinceEdit = true;
				}
			}
		}
		return $this->deletedSinceEdit;
	}

	function submit() {
		$this->edit();
	}

	/**
	 * This is the function that gets called for "action=edit". It
	 * sets up various member variables, then passes execution to
	 * another function, usually showEditForm()
	 *
	 * The edit form is self-submitting, so that when things like
	 * preview and edit conflicts occur, we get the same form back
	 * with the extra stuff added.  Only when the final submission
	 * is made and all is well do we actually save and redirect to
	 * the newly-edited page.
	 */
	function edit() {
		global $wgOut, $wgUser, $wgRequest;

		if ( !wfRunHooks( 'AlternateEdit', array( &$this ) ) )
			return;

		wfProfileIn( __METHOD__ );
		wfDebug( __METHOD__.": enter\n" );

		// this is not an article
		$wgOut->setArticleFlag(false);

		$this->importFormData( $wgRequest );
		$this->firsttime = false;

		if( $this->live ) {
			$this->livePreview();
			wfProfileOut( __METHOD__ );
			return;
		}
		
		$wgOut->addScriptFile( 'edit.js' );

		if( wfReadOnly() ) {
			$this->readOnlyPage( $this->getContent() );
			wfProfileOut( __METHOD__ );
			return;
		}

		$permErrors = $this->mTitle->getUserPermissionsErrors('edit', $wgUser);
		
		if( !$this->mTitle->exists() ) {
			$permErrors = array_merge( $permErrors,
				wfArrayDiff2( $this->mTitle->getUserPermissionsErrors('create', $wgUser), $permErrors ) );
		}

		# Ignore some permissions errors.
		$remove = array();
		foreach( $permErrors as $error ) {
			if ( ( $this->preview || $this->diff ) &&
				($error[0] == 'blockedtext' || $error[0] == 'autoblockedtext'))
			{
				// Don't worry about blocks when previewing/diffing
				$remove[] = $error;
			}

			if ($error[0] == 'readonlytext')
			{
				if ($this->edit) {
					$this->formtype = 'preview';
				} elseif ($this->save || $this->preview || $this->diff) {
					$remove[] = $error;
				}
			}
		}
		$permErrors = wfArrayDiff2( $permErrors, $remove );
		
		if ( $permErrors ) {
			wfDebug( __METHOD__.": User can't edit\n" );
			$this->readOnlyPage( $this->getContent(), true, $permErrors, 'edit' );
			wfProfileOut( __METHOD__ );
			return;
		} else {
			if ( $this->save ) {
				$this->formtype = 'save';
			} else if ( $this->preview ) {
				$this->formtype = 'preview';
			} else if ( $this->diff ) {
				$this->formtype = 'diff';
			} else { # First time through
				$this->firsttime = true;
				if( $this->previewOnOpen() ) {
					$this->formtype = 'preview';
				} else {
					$this->extractMetaDataFromArticle () ;
					$this->formtype = 'initial';
				}
			}
		}

		wfProfileIn( __METHOD__."-business-end" );

		$this->isConflict = false;
		// css / js subpages of user pages get a special treatment
		$this->isCssJsSubpage      = $this->mTitle->isCssJsSubpage();
		$this->isValidCssJsSubpage = $this->mTitle->isValidCssJsSubpage();

		# Show applicable editing introductions
		if( $this->formtype == 'initial' || $this->firsttime )
			$this->showIntro();

		if( $this->mTitle->isTalkPage() ) {
			$wgOut->addWikiMsg( 'talkpagetext' );
		}

		# Attempt submission here.  This will check for edit conflicts,
		# and redundantly check for locked database, blocked IPs, etc.
		# that edit() already checked just in case someone tries to sneak
		# in the back door with a hand-edited submission URL.

		if ( 'save' == $this->formtype ) {
			if ( !$this->attemptSave() ) {
				wfProfileOut( __METHOD__."-business-end" );
				wfProfileOut( __METHOD__ );
				return;
			}
		}

		# First time through: get contents, set time for conflict
		# checking, etc.
		if ( 'initial' == $this->formtype || $this->firsttime ) {
			if ($this->initialiseForm() === false) {
				$this->noSuchSectionPage();
				wfProfileOut( __METHOD__."-business-end" );
				wfProfileOut( __METHOD__ );
				return;
			}
			if( !$this->mTitle->getArticleId() )
				wfRunHooks( 'EditFormPreloadText', array( &$this->textbox1, &$this->mTitle ) );
		}

		$this->showEditForm();
		wfProfileOut( __METHOD__."-business-end" );
		wfProfileOut( __METHOD__ );
	}

	/**
	 * Show a read-only error
	 * Parameters are the same as OutputPage:readOnlyPage()
	 * Redirect to the article page if redlink=1
	 */
	function readOnlyPage( $source = null, $protected = false, $reasons = array(), $action = null ) {
		global $wgRequest, $wgOut;
		if ( $wgRequest->getBool( 'redlink' ) ) {
			// The edit page was reached via a red link.
			// Redirect to the article page and let them click the edit tab if
			// they really want a permission error.
			$wgOut->redirect( $this->mTitle->getFullUrl() );
		} else {
			$wgOut->readOnlyPage( $source, $protected, $reasons, $action );
		}
	}

	/**
	 * Should we show a preview when the edit form is first shown?
	 *
	 * @return bool
	 */
	protected function previewOnOpen() {
		global $wgRequest, $wgUser;
		if( $wgRequest->getVal( 'preview' ) == 'yes' ) {
			// Explicit override from request
			return true;
		} elseif( $wgRequest->getVal( 'preview' ) == 'no' ) {
			// Explicit override from request
			return false;
		} elseif( $this->section == 'new' ) {
			// Nothing *to* preview for new sections
			return false;
		} elseif( ( $wgRequest->getVal( 'preload' ) !== '' || $this->mTitle->exists() ) && $wgUser->getOption( 'previewonfirst' ) ) {
			// Standard preference behaviour
			return true;
		} elseif( !$this->mTitle->exists() && $this->mTitle->getNamespace() == NS_CATEGORY ) {
			// Categories are special
			return true;
		} else {
			return false;
		}
	}

	/**
	 * @todo document
	 * @param $request
	 */
	function importFormData( &$request ) {
		global $wgLang, $wgUser;
		$fname = 'EditPage::importFormData';
		wfProfileIn( $fname );

		# Section edit can come from either the form or a link
		$this->section = $request->getVal( 'wpSection', $request->getVal( 'section' ) );

		if( $request->wasPosted() ) {
			# These fields need to be checked for encoding.
			# Also remove trailing whitespace, but don't remove _initial_
			# whitespace from the text boxes. This may be significant formatting.
			$this->textbox1 = $this->safeUnicodeInput( $request, 'wpTextbox1' );
			$this->textbox2 = $this->safeUnicodeInput( $request, 'wpTextbox2' );
			$this->mMetaData = rtrim( $request->getText( 'metadata' ) );
			# Truncate for whole multibyte characters. +5 bytes for ellipsis
			$this->summary = $wgLang->truncate( $request->getText( 'wpSummary'  ), 250 );

			# Remove extra headings from summaries and new sections.
			$this->summary = preg_replace('/^\s*=+\s*(.*?)\s*=+\s*$/', '$1', $this->summary);

			$this->edittime = $request->getVal( 'wpEdittime' );
			$this->starttime = $request->getVal( 'wpStarttime' );

			$this->scrolltop = $request->getIntOrNull( 'wpScrolltop' );

			if( is_null( $this->edittime ) ) {
				# If the form is incomplete, force to preview.
				wfDebug( "$fname: Form data appears to be incomplete\n" );
				wfDebug( "POST DATA: " . var_export( $_POST, true ) . "\n" );
				$this->preview  = true;
			} else {
				/* Fallback for live preview */
				$this->preview = $request->getCheck( 'wpPreview' ) || $request->getCheck( 'wpLivePreview' );
				$this->diff = $request->getCheck( 'wpDiff' );

				// Remember whether a save was requested, so we can indicate
				// if we forced preview due to session failure.
				$this->mTriedSave = !$this->preview;

				if ( $this->tokenOk( $request ) ) {
					# Some browsers will not report any submit button
					# if the user hits enter in the comment box.
					# The unmarked state will be assumed to be a save,
					# if the form seems otherwise complete.
					wfDebug( "$fname: Passed token check.\n" );
				} else if ( $this->diff ) {
					# Failed token check, but only requested "Show Changes".
					wfDebug( "$fname: Failed token check; Show Changes requested.\n" );
				} else {
					# Page might be a hack attempt posted from
					# an external site. Preview instead of saving.
					wfDebug( "$fname: Failed token check; forcing preview\n" );
					$this->preview = true;
				}
			}
			$this->save = !$this->preview && !$this->diff;
			if( !preg_match( '/^\d{14}$/', $this->edittime )) {
				$this->edittime = null;
			}

			if( !preg_match( '/^\d{14}$/', $this->starttime )) {
				$this->starttime = null;
			}

			$this->recreate  = $request->getCheck( 'wpRecreate' );

			$this->minoredit = $request->getCheck( 'wpMinoredit' );
			$this->watchthis = $request->getCheck( 'wpWatchthis' );

			# Don't force edit summaries when a user is editing their own user or talk page
			if( ( $this->mTitle->mNamespace == NS_USER || $this->mTitle->mNamespace == NS_USER_TALK ) && $this->mTitle->getText() == $wgUser->getName() ) {
				$this->allowBlankSummary = true;
			} else {
				$this->allowBlankSummary = $request->getBool( 'wpIgnoreBlankSummary' );
			}

			$this->autoSumm = $request->getText( 'wpAutoSummary' );
		} else {
			# Not a posted form? Start with nothing.
			wfDebug( "$fname: Not a posted form.\n" );
			$this->textbox1  = '';
			$this->textbox2  = '';
			$this->mMetaData = '';
			$this->summary   = '';
			$this->edittime  = '';
			$this->starttime = wfTimestampNow();
			$this->edit      = false;
			$this->preview   = false;
			$this->save      = false;
			$this->diff      = false;
			$this->minoredit = false;
			$this->watchthis = false;
			$this->recreate  = false;

			if ( $this->section == 'new' && $request->getVal( 'preloadtitle' ) ) {
				$this->summary = $request->getVal( 'preloadtitle' );
			}
		}

		$this->oldid = $request->getInt( 'oldid' );

		$this->live = $request->getCheck( 'live' );
		$this->editintro = $request->getText( 'editintro' );

		wfProfileOut( $fname );
	}

	/**
	 * Make sure the form isn't faking a user's credentials.
	 *
	 * @param $request WebRequest
	 * @return bool
	 * @private
	 */
	function tokenOk( &$request ) {
		global $wgUser;
		$token = $request->getVal( 'wpEditToken' );
		$this->mTokenOk = $wgUser->matchEditToken( $token );
		$this->mTokenOkExceptSuffix = $wgUser->matchEditTokenNoSuffix( $token );
		return $this->mTokenOk;
	}

	/**
	 * Show all applicable editing introductions
	 */
	protected function showIntro() {
		global $wgOut, $wgUser;
		if( $this->suppressIntro )
			return;

		# Show a warning message when someone creates/edits a user (talk) page but the user does not exists
		if( $this->mTitle->getNamespace() == NS_USER || $this->mTitle->getNamespace() == NS_USER_TALK ) {
			$parts = explode( '/', $this->mTitle->getText(), 2 );
			$username = $parts[0];
			$id = User::idFromName( $username );
			$ip = User::isIP( $username );

			if ( $id == 0 && !$ip ) {
				$wgOut->wrapWikiMsg( '<div class="mw-userpage-userdoesnotexist error">$1</div>',
					array( 'userpage-userdoesnotexist', $username ) );
			}
		}

		if( !$this->showCustomIntro() && !$this->mTitle->exists() ) {
			if( $wgUser->isLoggedIn() ) {
				$wgOut->wrapWikiMsg( '<div class="mw-newarticletext">$1</div>', 'newarticletext' );
			} else {
				$wgOut->wrapWikiMsg( '<div class="mw-newarticletextanon">$1</div>', 'newarticletextanon' );
			}
			$this->showDeletionLog( $wgOut );
		}
	}

	/**
	 * Attempt to show a custom editing introduction, if supplied
	 *
	 * @return bool
	 */
	protected function showCustomIntro() {
		if( $this->editintro ) {
			$title = Title::newFromText( $this->editintro );
			if( $title instanceof Title && $title->exists() && $title->userCanRead() ) {
				global $wgOut;
				$revision = Revision::newFromTitle( $title );
				$wgOut->addWikiTextTitleTidy( $revision->getText(), $this->mTitle );
				return true;
			} else {
				return false;
			}
		} else {
			return false;
		}
	}

	/**
	 * Attempt submission (no UI)
	 * @return one of the constants describing the result
	 */
	function internalAttemptSave( &$result, $bot = false ) {
		global $wgSpamRegex, $wgFilterCallback, $wgUser, $wgOut, $wgParser;
		global $wgMaxArticleSize;

		$fname = 'EditPage::attemptSave';
		wfProfileIn( $fname );
		wfProfileIn( "$fname-checks" );

		if( !wfRunHooks( 'EditPage::attemptSave', array( &$this ) ) )
		{
			wfDebug( "Hook 'EditPage::attemptSave' aborted article saving" );
			return self::AS_HOOK_ERROR;
		}

		# Check image redirect
		if ( $this->mTitle->getNamespace() == NS_IMAGE &&
			Title::newFromRedirect( $this->textbox1 ) instanceof Title &&
			!$wgUser->isAllowed( 'upload' ) ) {
				if( $wgUser->isAnon() ) {
					return self::AS_IMAGE_REDIRECT_ANON;
				} else {
					return self::AS_IMAGE_REDIRECT_LOGGED;
				}
		}

		# Reintegrate metadata
		if ( $this->mMetaData != '' ) $this->textbox1 .= "\n" . $this->mMetaData ;
		$this->mMetaData = '' ;

		# Check for spam
		$matches = array();
		if ( $wgSpamRegex && preg_match( $wgSpamRegex, $this->textbox1, $matches ) ) {
			$result['spam'] = $matches[0];
			$ip = wfGetIP();
			$pdbk = $this->mTitle->getPrefixedDBkey();
			$match = str_replace( "\n", '', $matches[0] );
			wfDebugLog( 'SpamRegex', "$ip spam regex hit [[$pdbk]]: \"$match\"" );
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_SPAM_ERROR;
		}
		if ( $wgFilterCallback && $wgFilterCallback( $this->mTitle, $this->textbox1, $this->section, $this->hookError, $this->summary ) ) {
			# Error messages or other handling should be performed by the filter function
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_FILTERING;
		}
		if ( !wfRunHooks( 'EditFilter', array( $this, $this->textbox1, $this->section, &$this->hookError, $this->summary ) ) ) {
			# Error messages etc. could be handled within the hook...
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_HOOK_ERROR;
		} elseif( $this->hookError != '' ) {
			# ...or the hook could be expecting us to produce an error
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_HOOK_ERROR_EXPECTED;
		}
		if ( $wgUser->isBlockedFrom( $this->mTitle, false ) ) {
			# Check block state against master, thus 'false'.
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_BLOCKED_PAGE_FOR_USER;
		}
		$this->kblength = (int)(strlen( $this->textbox1 ) / 1024);
		if ( $this->kblength > $wgMaxArticleSize ) {
			// Error will be displayed by showEditForm()
			$this->tooBig = true;
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_CONTENT_TOO_BIG;
		}

		if ( !$wgUser->isAllowed('edit') ) {
			if ( $wgUser->isAnon() ) {
				wfProfileOut( "$fname-checks" );
				wfProfileOut( $fname );
				return self::AS_READ_ONLY_PAGE_ANON;
			}
			else {
				wfProfileOut( "$fname-checks" );
				wfProfileOut( $fname );
				return self::AS_READ_ONLY_PAGE_LOGGED;
			}
		}

		if ( wfReadOnly() ) {
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_READ_ONLY_PAGE;
		}
		if ( $wgUser->pingLimiter() ) {
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_RATE_LIMITED;
		}

		# If the article has been deleted while editing, don't save it without
		# confirmation
		if ( $this->wasDeletedSinceLastEdit() && !$this->recreate ) {
			wfProfileOut( "$fname-checks" );
			wfProfileOut( $fname );
			return self::AS_ARTICLE_WAS_DELETED;
		}

		wfProfileOut( "$fname-checks" );

		# If article is new, insert it.
		$aid = $this->mTitle->getArticleID( GAID_FOR_UPDATE );
		if ( 0 == $aid ) {

			// Late check for create permission, just in case *PARANOIA*
			if ( !$this->mTitle->userCan( 'create' ) ) {
				wfDebug( "$fname: no create permission\n" );
				wfProfileOut( $fname );
				return self::AS_NO_CREATE_PERMISSION;
			}

			# Don't save a new article if it's blank.
			if ( '' == $this->textbox1 ) {
					wfProfileOut( $fname );
					return self::AS_BLANK_ARTICLE;
			}

			// Run post-section-merge edit filter
			if ( !wfRunHooks( 'EditFilterMerged', array( $this, $this->textbox1, &$this->hookError, $this->summary ) ) ) {
				# Error messages etc. could be handled within the hook...
				wfProfileOut( $fname );
				return self::AS_HOOK_ERROR;
			}

			$isComment = ( $this->section == 'new' );

			$this->mArticle->insertNewArticle( $this->textbox1, $this->summary,
				$this->minoredit, $this->watchthis, false, $isComment, $bot);

			wfProfileOut( $fname );
			return self::AS_SUCCESS_NEW_ARTICLE;
		}

		# Article exists. Check for edit conflict.

		$this->mArticle->clear(); # Force reload of dates, etc.
		$this->mArticle->forUpdate( true ); # Lock the article

		wfDebug("timestamp: {$this->mArticle->getTimestamp()}, edittime: {$this->edittime}\n");

		if( $this->mArticle->getTimestamp() != $this->edittime ) {
			$this->isConflict = true;
			if( $this->section == 'new' ) {
				if( $this->mArticle->getUserText() == $wgUser->getName() &&
					$this->mArticle->getComment() == $this->summary ) {
					// Probably a duplicate submission of a new comment.
					// This can happen when squid resends a request after
					// a timeout but the first one actually went through.
					wfDebug( "EditPage::editForm duplicate new section submission; trigger edit conflict!\n" );
				} else {
					// New comment; suppress conflict.
					$this->isConflict = false;
					wfDebug( "EditPage::editForm conflict suppressed; new section\n" );
				}
			}
		}
		$userid = $wgUser->getId();

		if ( $this->isConflict) {
			wfDebug( "EditPage::editForm conflict! getting section '$this->section' for time '$this->edittime' (article time '" .
				$this->mArticle->getTimestamp() . "')\n" );
			$text = $this->mArticle->replaceSection( $this->section, $this->textbox1, $this->summary, $this->edittime);
		}
		else {
			wfDebug( "EditPage::editForm getting section '$this->section'\n" );
			$text = $this->mArticle->replaceSection( $this->section, $this->textbox1, $this->summary);
		}
		if( is_null( $text ) ) {
			wfDebug( "EditPage::editForm activating conflict; section replace failed.\n" );
			$this->isConflict = true;
			$text = $this->textbox1;
		}

		# Suppress edit conflict with self, except for section edits where merging is required.
		if ( ( $this->section == '' ) && ( 0 != $userid ) && ( $this->mArticle->getUser() == $userid ) ) {
			wfDebug( "EditPage::editForm Suppressing edit conflict, same user.\n" );
			$this->isConflict = false;
		} else {
			# switch from section editing to normal editing in edit conflict
			if($this->isConflict) {
				# Attempt merge
				if( $this->mergeChangesInto( $text ) ){
					// Successful merge! Maybe we should tell the user the good news?
					$this->isConflict = false;
					wfDebug( "EditPage::editForm Suppressing edit conflict, successful merge.\n" );
				} else {
					$this->section = '';
					$this->textbox1 = $text;
					wfDebug( "EditPage::editForm Keeping edit conflict, failed merge.\n" );
				}
			}
		}

		if ( $this->isConflict ) {
			wfProfileOut( $fname );
			return self::AS_CONFLICT_DETECTED;
		}

		$oldtext = $this->mArticle->getContent();

		// Run post-section-merge edit filter
		if ( !wfRunHooks( 'EditFilterMerged', array( $this, $text, &$this->hookError, $this->summary ) ) ) {
			# Error messages etc. could be handled within the hook...
			wfProfileOut( $fname );
			return self::AS_HOOK_ERROR;
		}

		# Handle the user preference to force summaries here, but not for null edits
		if( $this->section != 'new' && !$this->allowBlankSummary &&  $wgUser->getOption( 'forceeditsummary') && 
			0 != strcmp($oldtext, $text) && 
			!is_object( Title::newFromRedirect( $text ) ) # check if it's not a redirect
		) {

			if( md5( $this->summary ) == $this->autoSumm ) {
				$this->missingSummary = true;
				wfProfileOut( $fname );
				return self::AS_SUMMARY_NEEDED;
			}
		}

		# And a similar thing for new sections
		if( $this->section == 'new' && !$this->allowBlankSummary && $wgUser->getOption( 'forceeditsummary' ) ) {
			if (trim($this->summary) == '') {
				$this->missingSummary = true;
				wfProfileOut( $fname );
				return self::AS_SUMMARY_NEEDED;
			}
		}

		# All's well
		wfProfileIn( "$fname-sectionanchor" );
		$sectionanchor = '';
		if( $this->section == 'new' ) {
			if ( $this->textbox1 == '' ) {
				$this->missingComment = true;
				return self::AS_TEXTBOX_EMPTY;
			}
			if( $this->summary != '' ) {
				$sectionanchor = $wgParser->guessSectionNameFromWikiText( $this->summary );
				# This is a new section, so create a link to the new section
				# in the revision summary.
				$cleanSummary = $wgParser->stripSectionName( $this->summary );
				$this->summary = wfMsgForContent( 'newsectionsummary', $cleanSummary );
			}
		} elseif( $this->section != '' ) {
			# Try to get a section anchor from the section source, redirect to edited section if header found
			# XXX: might be better to integrate this into Article::replaceSection
			# for duplicate heading checking and maybe parsing
			$hasmatch = preg_match( "/^ *([=]{1,6})(.*?)(\\1) *\\n/i", $this->textbox1, $matches );
			# we can't deal with anchors, includes, html etc in the header for now,
			# headline would need to be parsed to improve this
			if($hasmatch and strlen($matches[2]) > 0) {
				$sectionanchor = $wgParser->guessSectionNameFromWikiText( $matches[2] );
			}
		}
		wfProfileOut( "$fname-sectionanchor" );

		// Save errors may fall down to the edit form, but we've now
		// merged the section into full text. Clear the section field
		// so that later submission of conflict forms won't try to
		// replace that into a duplicated mess.
		$this->textbox1 = $text;
		$this->section = '';

		// Check for length errors again now that the section is merged in
		$this->kblength = (int)(strlen( $text ) / 1024);
		if ( $this->kblength > $wgMaxArticleSize ) {
			$this->tooBig = true;
			wfProfileOut( $fname );
			return self::AS_MAX_ARTICLE_SIZE_EXCEEDED;
		}

		# update the article here
		if( $this->mArticle->updateArticle( $text, $this->summary, $this->minoredit,
			$this->watchthis, $bot, $sectionanchor ) ) {
			wfProfileOut( $fname );
			return self::AS_SUCCESS_UPDATE;
		} else {
			$this->isConflict = true;
		}
		wfProfileOut( $fname );
		return self::AS_END;
	}

	/**
	 * Initialise form fields in the object
	 * Called on the first invocation, e.g. when a user clicks an edit link
	 */
	function initialiseForm() {
		$this->edittime = $this->mArticle->getTimestamp();
		$this->textbox1 = $this->getContent(false);
		if ($this->textbox1 === false) return false;

		if ( !$this->mArticle->exists() && $this->mTitle->getNamespace() == NS_MEDIAWIKI )
			$this->textbox1 = wfMsgWeirdKey( $this->mTitle->getText() );
		wfProxyCheck();
		return true;
	}

	/**
	 * Send the edit form and related headers to $wgOut
	 * @param $formCallback Optional callable that takes an OutputPage
	 *                      parameter; will be called during form output
	 *                      near the top, for captchas and the like.
	 */
	function showEditForm( $formCallback=null ) {
		global $wgOut, $wgUser, $wgLang, $wgContLang, $wgMaxArticleSize, $wgTitle;

		# If $wgTitle is null, that means we're in API mode.
		# Some hook probably called this function  without checking
		# for is_null($wgTitle) first. Bail out right here so we don't
		# do lots of work just to discard it right after.
		if(is_null($wgTitle))
			return;

		$fname = 'EditPage::showEditForm';
		wfProfileIn( $fname );

		$sk = $wgUser->getSkin();

		wfRunHooks( 'EditPage::showEditForm:initial', array( &$this ) ) ;

		$wgOut->setRobotpolicy( 'noindex,nofollow' );

		# Enabled article-related sidebar, toplinks, etc.
		$wgOut->setArticleRelated( true );

		if ( $this->formtype == 'preview' ) {
			$wgOut->setPageTitleActionText( wfMsg( 'preview' ) );
		}

		if ( $this->isConflict ) {
			$s = wfMsg( 'editconflict', $wgTitle->getPrefixedText() );
			$wgOut->setPageTitle( $s );
			$wgOut->addWikiMsg( 'explainconflict' );

			$this->textbox2 = $this->textbox1;
			$this->textbox1 = $this->getContent();
			$this->edittime = $this->mArticle->getTimestamp();
		} else {
			if( $this->section != '' ) {
				if( $this->section == 'new' ) {
					$s = wfMsg('editingcomment', $wgTitle->getPrefixedText() );
				} else {
					$s = wfMsg('editingsection', $wgTitle->getPrefixedText() );
					$matches = array();
					if( !$this->summary && !$this->preview && !$this->diff ) {
						preg_match( "/^(=+)(.+)\\1/mi",
							$this->textbox1,
							$matches );
						if( !empty( $matches[2] ) ) {
							global $wgParser;
							$this->summary = "/* " .
								$wgParser->stripSectionName(trim($matches[2])) .
								" */ ";
						}
					}
				}
			} else {
				$s = wfMsg( 'editing', $wgTitle->getPrefixedText() );
			}
			$wgOut->setPageTitle( $s );

			if ( $this->missingComment ) {
				$wgOut->wrapWikiMsg( '<div id="mw-missingcommenttext">$1</div>',  'missingcommenttext' );
			}

			if( $this->missingSummary && $this->section != 'new' ) {
				$wgOut->wrapWikiMsg( '<div id="mw-missingsummary">$1</div>', 'missingsummary' );
			}

			if( $this->missingSummary && $this->section == 'new' ) {
				$wgOut->wrapWikiMsg( '<div id="mw-missingcommentheader">$1</div>', 'missingcommentheader' );
			}

			if( $this->hookError !== '' ) {
				$wgOut->addWikiText( $this->hookError );
			}

			if ( !$this->checkUnicodeCompliantBrowser() ) {
				$wgOut->addWikiMsg( 'nonunicodebrowser' );
			}
			if ( isset( $this->mArticle ) && isset( $this->mArticle->mRevision ) ) {
			// Let sysop know that this will make private content public if saved

				if( !$this->mArticle->mRevision->userCan( Revision::DELETED_TEXT ) ) {
					$wgOut->addWikiMsg( 'rev-deleted-text-permission' );
				} else if( $this->mArticle->mRevision->isDeleted( Revision::DELETED_TEXT ) ) {
					$wgOut->addWikiMsg( 'rev-deleted-text-view' );
				}

				if( !$this->mArticle->mRevision->isCurrent() ) {
					$this->mArticle->setOldSubtitle( $this->mArticle->mRevision->getId() );
					$wgOut->addWikiMsg( 'editingold' );
				}
			}
		}

		if( wfReadOnly() ) {
			$wgOut->addHTML( '<div id="mw-read-only-warning">'.wfMsgWikiHTML( 'readonlywarning' ).'</div>' );
		} elseif( $wgUser->isAnon() && $this->formtype != 'preview' ) {
			$wgOut->addHTML( '<div id="mw-anon-edit-warning">'.wfMsgWikiHTML( 'anoneditwarning' ).'</div>' );
		} else {
			if( $this->isCssJsSubpage && $this->formtype != 'preview' ) {
				# Check the skin exists
				if( $this->isValidCssJsSubpage ) {
					$wgOut->addWikiMsg( 'usercssjsyoucanpreview' );
				} else {
					$wgOut->addWikiMsg( 'userinvalidcssjstitle', $wgTitle->getSkinFromCssJsSubpage() );
				}
			}
		}

		if( $this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
			# Show a warning if editing an interface message
			$wgOut->addWikiMsg( 'editinginterface' );
		} elseif( $this->mTitle->isProtected( 'edit' ) ) {
			# Is the title semi-protected?
			if( $this->mTitle->isSemiProtected() ) {
				$noticeMsg = 'semiprotectedpagewarning';
			} else {
				# Then it must be protected based on static groups (regular)
				$noticeMsg = 'protectedpagewarning';
			}
			$wgOut->addWikiMsg( $noticeMsg );
		}
		if ( $this->mTitle->isCascadeProtected() ) {
			# Is this page under cascading protection from some source pages?
			list($cascadeSources, /* $restrictions */) = $this->mTitle->getCascadeProtectionSources();
			$notice = "$1\n";
			if ( count($cascadeSources) > 0 ) {
				# Explain, and list the titles responsible
				foreach( $cascadeSources as $page ) {
					$notice .= '* [[:' . $page->getPrefixedText() . "]]\n";
				}
			}
			$wgOut->wrapWikiMsg( $notice, array( 'cascadeprotectedwarning', count($cascadeSources) ) );
		}
		if( !$this->mTitle->exists() && $this->mTitle->getRestrictions( 'create' ) != array() ){
			$wgOut->addWikiMsg( 'titleprotectedwarning' );
		}

		if ( $this->kblength === false ) {
			$this->kblength = (int)(strlen( $this->textbox1 ) / 1024);
		}
		if ( $this->tooBig || $this->kblength > $wgMaxArticleSize ) {
			$wgOut->addWikiMsg( 'longpageerror', $wgLang->formatNum( $this->kblength ), $wgMaxArticleSize );
		} elseif( $this->kblength > 29 ) {
			$wgOut->addWikiMsg( 'longpagewarning', $wgLang->formatNum( $this->kblength ) );
		}

		#need to parse the preview early so that we know which templates are used,
		#otherwise users with "show preview after edit box" will get a blank list
		if ( $this->formtype == 'preview' ) {
			$previewOutput = $this->getPreviewText();
		}

		$rows = $wgUser->getIntOption( 'rows' );
		$cols = $wgUser->getIntOption( 'cols' );

		$ew = $wgUser->getOption( 'editwidth' );
		if ( $ew ) $ew = " style=\"width:100%\"";
		else $ew = '';

		$q = 'action=submit';
		#if ( "no" == $redirect ) { $q .= "&redirect=no"; }
		$action = $wgTitle->escapeLocalURL( $q );

		$summary = wfMsg('summary');
		$subject = wfMsg('subject');

		$cancel = $sk->makeKnownLink( $wgTitle->getPrefixedText(),
				wfMsgExt('cancel', array('parseinline')) );
		$edithelpurl = Skin::makeInternalOrExternalUrl( wfMsgForContent( 'edithelppage' ));
		$edithelp = '<a target="helpwindow" href="'.$edithelpurl.'">'.
			htmlspecialchars( wfMsg( 'edithelp' ) ).'</a> '.
			htmlspecialchars( wfMsg( 'newwindow' ) );

		global $wgRightsText;
		if ( $wgRightsText ) {
			$copywarnMsg = array( 'copyrightwarning',
				'[[' . wfMsgForContent( 'copyrightpage' ) . ']]',
				$wgRightsText );
		} else {
			$copywarnMsg = array( 'copyrightwarning2',
				'[[' . wfMsgForContent( 'copyrightpage' ) . ']]' );
		}

		if( $wgUser->getOption('showtoolbar') and !$this->isCssJsSubpage ) {
			# prepare toolbar for edit buttons
			$toolbar = EditPage::getEditToolbar();
		} else {
			$toolbar = '';
		}

		// activate checkboxes if user wants them to be always active
		if( !$this->preview && !$this->diff ) {
			# Sort out the "watch" checkbox
			if( $wgUser->getOption( 'watchdefault' ) ) {
				# Watch all edits
				$this->watchthis = true;
			} elseif( $wgUser->getOption( 'watchcreations' ) && !$this->mTitle->exists() ) {
				# Watch creations
				$this->watchthis = true;
			} elseif( $this->mTitle->userIsWatching() ) {
				# Already watched
				$this->watchthis = true;
			}

			if( $wgUser->getOption( 'minordefault' ) ) $this->minoredit = true;
		}

		$wgOut->addHTML( $this->editFormPageTop );

		if ( $wgUser->getOption( 'previewontop' ) ) {

			if ( 'preview' == $this->formtype ) {
				$this->showPreview( $previewOutput );
			} else {
				$wgOut->addHTML( '<div id="wikiPreview"></div>' );
			}

			if ( 'diff' == $this->formtype ) {
				$this->showDiff();
			}
		}


		$wgOut->addHTML( $this->editFormTextTop );

		# if this is a comment, show a subject line at the top, which is also the edit summary.
		# Otherwise, show a summary field at the bottom
		$summarytext = htmlspecialchars( $wgContLang->recodeForEdit( $this->summary ) ); # FIXME

		# If a blank edit summary was previously provided, and the appropriate
		# user preference is active, pass a hidden tag as wpIgnoreBlankSummary. This will stop the
		# user being bounced back more than once in the event that a summary
		# is not required.
		#####
		# For a bit more sophisticated detection of blank summaries, hash the
		# automatic one and pass that in the hidden field wpAutoSummary.
		$summaryhiddens =  '';
		if( $this->missingSummary ) $summaryhiddens .= Xml::hidden( 'wpIgnoreBlankSummary', true );
		$autosumm = $this->autoSumm ? $this->autoSumm : md5( $this->summary );
		$summaryhiddens .= Xml::hidden( 'wpAutoSummary', $autosumm );
		if( $this->section == 'new' ) {
			$commentsubject="<span id='wpSummaryLabel'><label for='wpSummary'>{$subject}:</label></span>\n<input tabindex='1' type='text' value=\"$summarytext\" name='wpSummary' id='wpSummary' maxlength='200' size='60' />{$summaryhiddens}<br />";
			$editsummary = "<div class='editOptions'>\n";
			global $wgParser;
			$formattedSummary = wfMsgForContent( 'newsectionsummary', $wgParser->stripSectionName( $this->summary ) );
			$subjectpreview = $summarytext && $this->preview ? "<div class=\"mw-summary-preview\">".wfMsg('subject-preview').':'.$sk->commentBlock( $formattedSummary, $this->mTitle, true )."</div>\n" : '';
			$summarypreview = '';
		} else {
			$commentsubject = '';
			$editsummary="<div class='editOptions'>\n<span id='wpSummaryLabel'><label for='wpSummary'>{$summary}:</label></span>\n<input tabindex='2' type='text' value=\"$summarytext\" name='wpSummary' id='wpSummary' maxlength='200' size='60' />{$summaryhiddens}<br />";
			$summarypreview = $summarytext && $this->preview ? "<div class=\"mw-summary-preview\">".wfMsg('summary-preview').':'.$sk->commentBlock( $this->summary, $this->mTitle )."</div>\n" : '';
			$subjectpreview = '';
		}

		# Set focus to the edit box on load, except on preview or diff, where it would interfere with the display
		if( !$this->preview && !$this->diff ) {
			$wgOut->setOnloadHandler( 'document.editform.wpTextbox1.focus()' );
		}
		$templates = ($this->preview || $this->section != '') ? $this->mPreviewTemplates : $this->mArticle->getUsedTemplates();
		$formattedtemplates = $sk->formatTemplates( $templates, $this->preview, $this->section != '');

		$hiddencats = $this->mArticle->getHiddenCategories();
		$formattedhiddencats = $sk->formatHiddenCategories( $hiddencats );

		global $wgUseMetadataEdit ;
		if ( $wgUseMetadataEdit ) {
			$metadata = $this->mMetaData ;
			$metadata = htmlspecialchars( $wgContLang->recodeForEdit( $metadata ) ) ;
			$top = wfMsgWikiHtml( 'metadata_help' );
			$metadata = $top . "<textarea name='metadata' rows='3' cols='{$cols}'{$ew}>{$metadata}</textarea>" ;
		}
		else $metadata = "" ;

		$hidden = '';
		$recreate = '';
		if ($this->wasDeletedSinceLastEdit()) {
			if ( 'save' != $this->formtype ) {
				$wgOut->addWikiMsg('deletedwhileediting');
			} else {
				// Hide the toolbar and edit area, use can click preview to get it back
				// Add an confirmation checkbox and explanation.
				$toolbar = '';
				$hidden = 'type="hidden" style="display:none;"';
				$recreate = $wgOut->parse( wfMsg( 'confirmrecreate',  $this->lastDelete->user_name , $this->lastDelete->log_comment ));
				$recreate .=
					"<br /><input tabindex='1' type='checkbox' value='1' name='wpRecreate' id='wpRecreate' />".
					"<label for='wpRecreate' title='".wfMsg('tooltip-recreate')."'>". wfMsg('recreate')."</label>";
			}
		}

		$tabindex = 2;

		$checkboxes = self::getCheckboxes( $tabindex, $sk,
			array( 'minor' => $this->minoredit, 'watch' => $this->watchthis ) );

		$checkboxhtml = implode( $checkboxes, "\n" );

		$buttons = $this->getEditButtons( $tabindex );
		$buttonshtml = implode( $buttons, "\n" );

		$safemodehtml = $this->checkUnicodeCompliantBrowser()
			? '' : Xml::hidden( 'safemode', '1' );

		$wgOut->addHTML( <<<END
{$toolbar}
<form id="editform" name="editform" method="post" action="$action" enctype="multipart/form-data">
END
);

		if( is_callable( $formCallback ) ) {
			call_user_func_array( $formCallback, array( &$wgOut ) );
		}

		wfRunHooks( 'EditPage::showEditForm:fields', array( &$this, &$wgOut ) );

		// Put these up at the top to ensure they aren't lost on early form submission
		$wgOut->addHTML( "
<input type='hidden' value=\"" . htmlspecialchars( $this->section ) . "\" name=\"wpSection\" />
<input type='hidden' value=\"{$this->starttime}\" name=\"wpStarttime\" />\n
<input type='hidden' value=\"{$this->edittime}\" name=\"wpEdittime\" />\n
<input type='hidden' value=\"{$this->scrolltop}\" name=\"wpScrolltop\" id=\"wpScrolltop\" />\n" );

		$encodedtext = htmlspecialchars( $this->safeUnicodeOutput( $this->textbox1 ) );
		if( $encodedtext !== '' ) {
			// Ensure there's a newline at the end, otherwise adding lines
			// is awkward.
			// But don't add a newline if the ext is empty, or Firefox in XHTML
			// mode will show an extra newline. A bit annoying.
			$encodedtext .= "\n";
		}

		$wgOut->addHTML( <<<END
$recreate
{$commentsubject}
{$subjectpreview}
{$this->editFormTextBeforeContent}
<textarea tabindex='1' accesskey="," name="wpTextbox1" id="wpTextbox1" rows='{$rows}'
cols='{$cols}'{$ew} $hidden>{$encodedtext}</textarea>
END
);

		$wgOut->wrapWikiMsg( "<div id=\"editpage-copywarn\">\n$1\n</div>", $copywarnMsg );
		$wgOut->addHTML( $this->editFormTextAfterWarn );
		$wgOut->addHTML( "
{$metadata}
{$editsummary}
{$summarypreview}
{$checkboxhtml}
{$safemodehtml}
");

		$wgOut->addHTML(
"<div class='editButtons'>
{$buttonshtml}
	<span class='editHelp'>{$cancel} | {$edithelp}</span>
</div><!-- editButtons -->
</div><!-- editOptions -->");

		/**
		 * To make it harder for someone to slip a user a page
		 * which submits an edit form to the wiki without their
		 * knowledge, a random token is associated with the login
		 * session. If it's not passed back with the submission,
		 * we won't save the page, or render user JavaScript and
		 * CSS previews.
		 *
		 * For anon editors, who may not have a session, we just
		 * include the constant suffix to prevent editing from
		 * broken text-mangling proxies.
		 */
		$token = htmlspecialchars( $wgUser->editToken() );
		$wgOut->addHTML( "\n<input type='hidden' value=\"$token\" name=\"wpEditToken\" />\n" );

		$wgOut->addHtml( '<div class="mw-editTools">' );
		$wgOut->addWikiMsgArray( 'edittools', array(), array( 'content' ) );
		$wgOut->addHtml( '</div>' );

		$wgOut->addHTML( $this->editFormTextAfterTools );

		$wgOut->addHTML( "
<div class='templatesUsed'>
{$formattedtemplates}
</div>
<div class='hiddencats'>
{$formattedhiddencats}
</div>
");

		if ( $this->isConflict && wfRunHooks( 'EditPageBeforeConflictDiff', array( &$this, &$wgOut ) ) ) {
			$wgOut->wrapWikiMsg( '==$1==', "yourdiff" );

			$de = new DifferenceEngine( $this->mTitle );
			$de->setText( $this->textbox2, $this->textbox1 );
			$de->showDiff( wfMsg( "yourtext" ), wfMsg( "storedversion" ) );

			$wgOut->wrapWikiMsg( '==$1==', "yourtext" );
			$wgOut->addHTML( "<textarea tabindex='6' id='wpTextbox2' name=\"wpTextbox2\" rows='{$rows}' cols='{$cols}'>"
				. htmlspecialchars( $this->safeUnicodeOutput( $this->textbox2 ) ) . "\n</textarea>" );
		}
		$wgOut->addHTML( $this->editFormTextBottom );
		$wgOut->addHTML( "</form>\n" );
		if ( !$wgUser->getOption( 'previewontop' ) ) {

			if ( $this->formtype == 'preview') {
				$this->showPreview( $previewOutput );
			} else {
				$wgOut->addHTML( '<div id="wikiPreview"></div>' );
			}

			if ( $this->formtype == 'diff') {
				$this->showDiff();
			}

		}

		wfProfileOut( $fname );
	}

	/**
	 * Append preview output to $wgOut.
	 * Includes category rendering if this is a category page.
	 *
	 * @param string $text The HTML to be output for the preview.
	 */
	protected function showPreview( $text ) {
		global $wgOut;

		$wgOut->addHTML( '<div id="wikiPreview">' );
		if($this->mTitle->getNamespace() == NS_CATEGORY) {
			$this->mArticle->openShowCategory();
		}
		wfRunHooks( 'OutputPageBeforeHTML',array( &$wgOut, &$text ) );
		$wgOut->addHTML( $text );
		if($this->mTitle->getNamespace() == NS_CATEGORY) {
			$this->mArticle->closeShowCategory();
		}
		$wgOut->addHTML( '</div>' );
	}

	/**
	 * Live Preview lets us fetch rendered preview page content and
	 * add it to the page without refreshing the whole page.
	 * If not supported by the browser it will fall through to the normal form
	 * submission method.
	 *
	 * This function outputs a script tag to support live preview, and
	 * returns an onclick handler which should be added to the attributes
	 * of the preview button
	 */
	function doLivePreviewScript() {
		global $wgOut, $wgTitle;
		$wgOut->addScriptFile( 'preview.js' );
		$liveAction = $wgTitle->getLocalUrl( 'action=submit&wpPreview=true&live=true' );
		return "return !lpDoPreview(" .
			"editform.wpTextbox1.value," .
			'"' . $liveAction . '"' . ")";
	}

	function getLastDelete() {
		$dbr = wfGetDB( DB_SLAVE );
		$fname = 'EditPage::getLastDelete';
		$res = $dbr->select(
			array( 'logging', 'user' ),
			array( 'log_type',
			       'log_action',
			       'log_timestamp',
			       'log_user',
			       'log_namespace',
			       'log_title',
			       'log_comment',
			       'log_params',
			       'user_name', ),
			array( 'log_namespace' => $this->mTitle->getNamespace(),
			       'log_title' => $this->mTitle->getDBkey(),
			       'log_type' => 'delete',
			       'log_action' => 'delete',
			       'user_id=log_user' ),
			$fname,
			array( 'LIMIT' => 1, 'ORDER BY' => 'log_timestamp DESC' ) );

		if($dbr->numRows($res) == 1) {
			while ( $x = $dbr->fetchObject ( $res ) )
				$data = $x;
			$dbr->freeResult ( $res ) ;
		} else {
			$data = null;
		}
		return $data;
	}

	/**
	 * @todo document
	 */
	function getPreviewText() {
		global $wgOut, $wgUser, $wgTitle, $wgParser, $wgLang, $wgContLang;

		$fname = 'EditPage::getPreviewText';
		wfProfileIn( $fname );

		if ( $this->mTriedSave && !$this->mTokenOk ) {
			if ( $this->mTokenOkExceptSuffix ) {
				$note = wfMsg( 'token_suffix_mismatch' );
			} else {
				$note = wfMsg( 'session_fail_preview' );
			}
		} else {
			$note = wfMsg( 'previewnote' );
		}

		$parserOptions = ParserOptions::newFromUser( $wgUser );
		$parserOptions->setEditSection( false );

		global $wgRawHtml;
		if( $wgRawHtml && !$this->mTokenOk ) {
			// Could be an offsite preview attempt. This is very unsafe if
			// HTML is enabled, as it could be an attack.
			return $wgOut->parse( "<div class='previewnote'>" .
				wfMsg( 'session_fail_preview_html' ) . "</div>" );
		}

		# don't parse user css/js, show message about preview
		# XXX: stupid php bug won't let us use $wgTitle->isCssJsSubpage() here

		if ( $this->isCssJsSubpage ) {
			if(preg_match("/\\.css$/", $this->mTitle->getText() ) ) {
				$previewtext = wfMsg('usercsspreview');
			} else if(preg_match("/\\.js$/", $this->mTitle->getText() ) ) {
				$previewtext = wfMsg('userjspreview');
			}
			$parserOptions->setTidy(true);
			$parserOutput = $wgParser->parse( $previewtext , $this->mTitle, $parserOptions );
			$wgOut->addHTML( $parserOutput->mText );
			$previewHTML = '';
		} else {
			$toparse = $this->textbox1;

			# If we're adding a comment, we need to show the
			# summary as the headline
			if($this->section=="new" && $this->summary!="") {
				$toparse="== {$this->summary} ==\n\n".$toparse;
			}

			if ( $this->mMetaData != "" ) $toparse .= "\n" . $this->mMetaData;

			// Parse mediawiki messages with correct target language
			if ( $this->mTitle->getNamespace() == NS_MEDIAWIKI ) {
				$pos = strrpos( $this->mTitle->getText(), '/' );
				if ( $pos !== false ) {
					$code = substr( $this->mTitle->getText(), $pos+1 );
					switch ($code) {
						case $wgLang->getCode():
							$obj = $wgLang; break;
						case $wgContLang->getCode():
							$obj = $wgContLang; break;
						default:
							$obj = Language::factory( $code );
					}
					$parserOptions->setTargetLanguage( $obj );
				}
			}


			$parserOptions->setTidy(true);
			$parserOptions->enableLimitReport();
			$parserOutput = $wgParser->parse( $this->mArticle->preSaveTransform( $toparse ),
					$this->mTitle, $parserOptions );

			$previewHTML = $parserOutput->getText();
			$wgOut->addParserOutputNoText( $parserOutput );

			# ParserOutput might have altered the page title, so reset it
			# Also, use the title defined by DISPLAYTITLE magic word when present
			if( ( $dt = $parserOutput->getDisplayTitle() ) !== false ) {
				$wgOut->setPageTitle( wfMsg( 'editing', $dt ) );
			} else {
				$wgOut->setPageTitle( wfMsg( 'editing', $wgTitle->getPrefixedText() ) );
			}

			foreach ( $parserOutput->getTemplates() as $ns => $template)
				foreach ( array_keys( $template ) as $dbk)
					$this->mPreviewTemplates[] = Title::makeTitle($ns, $dbk);

			if ( count( $parserOutput->getWarnings() ) ) {
				$note .= "\n\n" . implode( "\n\n", $parserOutput->getWarnings() );
			}
		}

		$previewhead = '<h2>' . htmlspecialchars( wfMsg( 'preview' ) ) . "</h2>\n" .
			"<div class='previewnote'>" . $wgOut->parse( $note ) . "</div>\n";
		if ( $this->isConflict ) {
			$previewhead.='<h2>' . htmlspecialchars( wfMsg( 'previewconflict' ) ) . "</h2>\n";
		}

		if( $wgUser->getOption( 'previewontop' ) ) {
			// Spacer for the edit toolbar
			$previewfoot = '<p><br /></p>';
		} else {
			$previewfoot = '';
		}

		wfProfileOut( $fname );
		return $previewhead . $previewHTML . $previewfoot;
	}

	/**
	 * Call the stock "user is blocked" page
	 */
	function blockedPage() {
		global $wgOut, $wgUser;
		$wgOut->blockedPage( false ); # Standard block notice on the top, don't 'return'

		# If the user made changes, preserve them when showing the markup
		# (This happens when a user is blocked during edit, for instance)
		$first = $this->firsttime || ( !$this->save && $this->textbox1 == '' );
		if( $first ) {
			$source = $this->mTitle->exists() ? $this->getContent() : false;
		} else {
			$source = $this->textbox1;
		}

		# Spit out the source or the user's modified version
		if( $source !== false ) {
			$rows = $wgUser->getOption( 'rows' );
			$cols = $wgUser->getOption( 'cols' );
			$attribs = array( 'id' => 'wpTextbox1', 'name' => 'wpTextbox1', 'cols' => $cols, 'rows' => $rows, 'readonly' => 'readonly' );
			$wgOut->addHtml( '<hr />' );
			$wgOut->addWikiMsg( $first ? 'blockedoriginalsource' : 'blockededitsource', $this->mTitle->getPrefixedText() );
			# Why we don't use Xml::element here?
			# Is it because if $source is '', it returns <textarea />?
			$wgOut->addHtml( Xml::openElement( 'textarea', $attribs ) . htmlspecialchars( $source ) . Xml::closeElement( 'textarea' ) );
		}
	}

	/**
	 * Produce the stock "please login to edit pages" page
	 */
	function userNotLoggedInPage() {
		global $wgUser, $wgOut, $wgTitle;
		$skin = $wgUser->getSkin();

		$loginTitle = SpecialPage::getTitleFor( 'Userlogin' );
		$loginLink = $skin->makeKnownLinkObj( $loginTitle, wfMsgHtml( 'loginreqlink' ), 'returnto=' . $wgTitle->getPrefixedUrl() );

		$wgOut->setPageTitle( wfMsg( 'whitelistedittitle' ) );
		$wgOut->setRobotPolicy( 'noindex,nofollow' );
		$wgOut->setArticleRelated( false );

		$wgOut->addHtml( wfMsgWikiHtml( 'whitelistedittext', $loginLink ) );
		$wgOut->returnToMain( false, $wgTitle );
	}

	/**
	 * Creates a basic error page which informs the user that
	 * they have attempted to edit a nonexistant section.
	 */
	function noSuchSectionPage() {
		global $wgOut, $wgTitle;

		$wgOut->setPageTitle( wfMsg( 'nosuchsectiontitle' ) );
		$wgOut->setRobotPolicy( 'noindex,nofollow' );
		$wgOut->setArticleRelated( false );

		$wgOut->addWikiMsg( 'nosuchsectiontext', $this->section );
		$wgOut->returnToMain( false, $wgTitle );
	}

	/**
	 * Produce the stock "your edit contains spam" page
	 *
	 * @param $match Text which triggered one or more filters
	 */
	function spamPage( $match = false ) {
		global $wgOut, $wgTitle;

		$wgOut->setPageTitle( wfMsg( 'spamprotectiontitle' ) );
		$wgOut->setRobotPolicy( 'noindex,nofollow' );
		$wgOut->setArticleRelated( false );

		$wgOut->addHtml( '<div id="spamprotected">' );
		$wgOut->addWikiMsg( 'spamprotectiontext' );
		if ( $match )
			$wgOut->addWikiMsg( 'spamprotectionmatch', wfEscapeWikiText( $match ) );
		$wgOut->addHtml( '</div>' );

		$wgOut->returnToMain( false, $wgTitle );
	}

	/**
	 * @private
	 * @todo document
	 */
	function mergeChangesInto( &$editText ){
		$fname = 'EditPage::mergeChangesInto';
		wfProfileIn( $fname );

		$db = wfGetDB( DB_MASTER );

		// This is the revision the editor started from
		$baseRevision = $this->getBaseRevision();
		if( is_null( $baseRevision ) ) {
			wfProfileOut( $fname );
			return false;
		}
		$baseText = $baseRevision->getText();

		// The current state, we want to merge updates into it
		$currentRevision =  Revision::loadFromTitle(
			$db, $this->mTitle );
		if( is_null( $currentRevision ) ) {
			wfProfileOut( $fname );
			return false;
		}
		$currentText = $currentRevision->getText();

		$result = '';
		if( wfMerge( $baseText, $editText, $currentText, $result ) ){
			$editText = $result;
			wfProfileOut( $fname );
			return true;
		} else {
			wfProfileOut( $fname );
			return false;
		}
	}

	/**
	 * Check if the browser is on a blacklist of user-agents known to
	 * mangle UTF-8 data on form submission. Returns true if Unicode
	 * should make it through, false if it's known to be a problem.
	 * @return bool
	 * @private
	 */
	function checkUnicodeCompliantBrowser() {
		global $wgBrowserBlackList;
		if( empty( $_SERVER["HTTP_USER_AGENT"] ) ) {
			// No User-Agent header sent? Trust it by default...
			return true;
		}
		$currentbrowser = $_SERVER["HTTP_USER_AGENT"];
		foreach ( $wgBrowserBlackList as $browser ) {
			if ( preg_match($browser, $currentbrowser) ) {
				return false;
			}
		}
		return true;
	}

	/**
	 * @deprecated use $wgParser->stripSectionName()
	 */
	function pseudoParseSectionAnchor( $text ) {
		global $wgParser;
		return $wgParser->stripSectionName( $text );
	}

	/**
	 * Format an anchor fragment as it would appear for a given section name
	 * @param string $text
	 * @return string
	 * @private
	 */
	function sectionAnchor( $text ) {
		global $wgParser;
		return $wgParser->guessSectionNameFromWikiText( $text );
	}

	/**
	 * Shows a bulletin board style toolbar for common editing functions.
	 * It can be disabled in the user preferences.
	 * The necessary JavaScript code can be found in skins/common/edit.js.
	 * 
	 * @return string
	 */
	static function getEditToolbar() {
		global $wgStylePath, $wgContLang, $wgLang, $wgJsMimeType;

		/**
		 * toolarray an array of arrays which each include the filename of
		 * the button image (without path), the opening tag, the closing tag,
		 * and optionally a sample text that is inserted between the two when no
		 * selection is highlighted.
		 * The tip text is shown when the user moves the mouse over the button.
		 *
		 * Already here are accesskeys (key), which are not used yet until someone
		 * can figure out a way to make them work in IE. However, we should make
		 * sure these keys are not defined on the edit page.
		 */
		$toolarray = array(
			array(
				'image'  => $wgLang->getImageFile('button-bold'),
				'id'     => 'mw-editbutton-bold',
				'open'   => '\'\'\'',
				'close'  => '\'\'\'',
				'sample' => wfMsg('bold_sample'),
				'tip'    => wfMsg('bold_tip'),
				'key'    => 'B'
			),
			array(
				'image'  => $wgLang->getImageFile('button-italic'),
				'id'     => 'mw-editbutton-italic',
				'open'   => '\'\'',
				'close'  => '\'\'',
				'sample' => wfMsg('italic_sample'),
				'tip'    => wfMsg('italic_tip'),
				'key'    => 'I'
			),
			array(
				'image'  => $wgLang->getImageFile('button-link'),
				'id'     => 'mw-editbutton-link',
				'open'   => '[[',
				'close'  => ']]',
				'sample' => wfMsg('link_sample'),
				'tip'    => wfMsg('link_tip'),
				'key'    => 'L'
			),
			array(
				'image'  => $wgLang->getImageFile('button-extlink'),
				'id'     => 'mw-editbutton-extlink',
				'open'   => '[',
				'close'  => ']',
				'sample' => wfMsg('extlink_sample'),
				'tip'    => wfMsg('extlink_tip'),
				'key'    => 'X'
			),
			array(
				'image'  => $wgLang->getImageFile('button-headline'),
				'id'     => 'mw-editbutton-headline',
				'open'   => "\n== ",
				'close'  => " ==\n",
				'sample' => wfMsg('headline_sample'),
				'tip'    => wfMsg('headline_tip'),
				'key'    => 'H'
			),
			array(
				'image'  => $wgLang->getImageFile('button-image'),
				'id'     => 'mw-editbutton-image',
				'open'   => '[['.$wgContLang->getNsText(NS_IMAGE).':',
				'close'  => ']]',
				'sample' => wfMsg('image_sample'),
				'tip'    => wfMsg('image_tip'),
				'key'    => 'D'
			),
			array(
				'image'  => $wgLang->getImageFile('button-media'),
				'id'     => 'mw-editbutton-media',
				'open'   => '[['.$wgContLang->getNsText(NS_MEDIA).':',
				'close'  => ']]',
				'sample' => wfMsg('media_sample'),
				'tip'    => wfMsg('media_tip'),
				'key'    => 'M'
			),
			array(
				'image'  => $wgLang->getImageFile('button-math'),
				'id'     => 'mw-editbutton-math',
				'open'   => "<math>",
				'close'  => "</math>",
				'sample' => wfMsg('math_sample'),
				'tip'    => wfMsg('math_tip'),
				'key'    => 'C'
			),
			array(
				'image'  => $wgLang->getImageFile('button-nowiki'),
				'id'     => 'mw-editbutton-nowiki',
				'open'   => "<nowiki>",
				'close'  => "</nowiki>",
				'sample' => wfMsg('nowiki_sample'),
				'tip'    => wfMsg('nowiki_tip'),
				'key'    => 'N'
			),
			array(
				'image'  => $wgLang->getImageFile('button-sig'),
				'id'     => 'mw-editbutton-signature',
				'open'   => '--~~~~',
				'close'  => '',
				'sample' => '',
				'tip'    => wfMsg('sig_tip'),
				'key'    => 'Y'
			),
			array(
				'image'  => $wgLang->getImageFile('button-hr'),
				'id'     => 'mw-editbutton-hr',
				'open'   => "\n----\n",
				'close'  => '',
				'sample' => '',
				'tip'    => wfMsg('hr_tip'),
				'key'    => 'R'
			)
		);
		$toolbar = "<div id='toolbar'>\n";
		$toolbar.="<script type='$wgJsMimeType'>\n/*<![CDATA[*/\n";

		foreach($toolarray as $tool) {
			$params = array(
				$image = $wgStylePath.'/common/images/'.$tool['image'],
				// Note that we use the tip both for the ALT tag and the TITLE tag of the image.
				// Older browsers show a "speedtip" type message only for ALT.
				// Ideally these should be different, realistically they
				// probably don't need to be.
				$tip = $tool['tip'],
				$open = $tool['open'],
				$close = $tool['close'],
				$sample = $tool['sample'],
				$cssId = $tool['id'],
			);

			$paramList = implode( ',',
				array_map( array( 'Xml', 'encodeJsVar' ), $params ) );
			$toolbar.="addButton($paramList);\n";
		}

		$toolbar.="/*]]>*/\n</script>";
		$toolbar.="\n</div>";
		return $toolbar;
	}

	/**
	 * Returns an array of html code of the following checkboxes:
	 * minor and watch
	 *
	 * @param $tabindex Current tabindex
	 * @param $skin Skin object
	 * @param $checked Array of checkbox => bool, where bool indicates the checked
	 *                 status of the checkbox
	 *
	 * @return array
	 */
	public static function getCheckboxes( &$tabindex, $skin, $checked ) {
		global $wgUser;

		$checkboxes = array();

		$checkboxes['minor'] = '';
		$minorLabel = wfMsgExt('minoredit', array('parseinline'));
		if ( $wgUser->isAllowed('minoredit') ) {
			$attribs = array(
				'tabindex'  => ++$tabindex,
				'accesskey' => wfMsg( 'accesskey-minoredit' ),
				'id'        => 'wpMinoredit',
			);
			$checkboxes['minor'] =
				Xml::check( 'wpMinoredit', $checked['minor'], $attribs ) .
				"&nbsp;<label for='wpMinoredit'".$skin->tooltip('minoredit', 'withaccess').">{$minorLabel}</label>";
		}

		$watchLabel = wfMsgExt('watchthis', array('parseinline'));
		$checkboxes['watch'] = '';
		if ( $wgUser->isLoggedIn() ) {
			$attribs = array(
				'tabindex'  => ++$tabindex,
				'accesskey' => wfMsg( 'accesskey-watch' ),
				'id'        => 'wpWatchthis',
			);
			$checkboxes['watch'] =
				Xml::check( 'wpWatchthis', $checked['watch'], $attribs ) .
				"&nbsp;<label for='wpWatchthis'".$skin->tooltip('watch', 'withaccess').">{$watchLabel}</label>";
		}
		return $checkboxes;
	}

	/**
	 * Returns an array of html code of the following buttons:
	 * save, diff, preview and live
	 *
	 * @param $tabindex Current tabindex
	 *
	 * @return array
	 */
	public function getEditButtons(&$tabindex) {
		global $wgLivePreview, $wgUser;

		$buttons = array();

		$temp = array(
			'id'        => 'wpSave',
			'name'      => 'wpSave',
			'type'      => 'submit',
			'tabindex'  => ++$tabindex,
			'value'     => wfMsg('savearticle'),
			'accesskey' => wfMsg('accesskey-save'),
			'title'     => wfMsg( 'tooltip-save' ).' ['.wfMsg( 'accesskey-save' ).']',
		);
		$buttons['save'] = Xml::element('input', $temp, '');

		++$tabindex; // use the same for preview and live preview
		if ( $wgLivePreview && $wgUser->getOption( 'uselivepreview' ) ) {
			$temp = array(
				'id'        => 'wpPreview',
				'name'      => 'wpPreview',
				'type'      => 'submit',
				'tabindex'  => $tabindex,
				'value'     => wfMsg('showpreview'),
				'accesskey' => '',
				'title'     => wfMsg( 'tooltip-preview' ).' ['.wfMsg( 'accesskey-preview' ).']',
				'style'     => 'display: none;',
			);
			$buttons['preview'] = Xml::element('input', $temp, '');

			$temp = array(
				'id'        => 'wpLivePreview',
				'name'      => 'wpLivePreview',
				'type'      => 'submit',
				'tabindex'  => $tabindex,
				'value'     => wfMsg('showlivepreview'),
				'accesskey' => wfMsg('accesskey-preview'),
				'title'     => '',
				'onclick'   => $this->doLivePreviewScript(),
			);
			$buttons['live'] = Xml::element('input', $temp, '');
		} else {
			$temp = array(
				'id'        => 'wpPreview',
				'name'      => 'wpPreview',
				'type'      => 'submit',
				'tabindex'  => $tabindex,
				'value'     => wfMsg('showpreview'),
				'accesskey' => wfMsg('accesskey-preview'),
				'title'     => wfMsg( 'tooltip-preview' ).' ['.wfMsg( 'accesskey-preview' ).']',
			);
			$buttons['preview'] = Xml::element('input', $temp, '');
			$buttons['live'] = '';
		}

		$temp = array(
			'id'        => 'wpDiff',
			'name'      => 'wpDiff',
			'type'      => 'submit',
			'tabindex'  => ++$tabindex,
			'value'     => wfMsg('showdiff'),
			'accesskey' => wfMsg('accesskey-diff'),
			'title'     => wfMsg( 'tooltip-diff' ).' ['.wfMsg( 'accesskey-diff' ).']',
		);
		$buttons['diff'] = Xml::element('input', $temp, '');

		wfRunHooks( 'EditPageBeforeEditButtons', array( &$this, &$buttons ) );
		return $buttons;
	}

	/**
	 * Output preview text only. This can be sucked into the edit page
	 * via JavaScript, and saves the server time rendering the skin as
	 * well as theoretically being more robust on the client (doesn't
	 * disturb the edit box's undo history, won't eat your text on
	 * failure, etc).
	 *
	 * @todo This doesn't include category or interlanguage links.
	 *       Would need to enhance it a bit, <s>maybe wrap them in XML
	 *       or something...</s> that might also require more skin
	 *       initialization, so check whether that's a problem.
	 */
	function livePreview() {
		global $wgOut;
		$wgOut->disable();
		header( 'Content-type: text/xml; charset=utf-8' );
		header( 'Cache-control: no-cache' );

		$previewText = $this->getPreviewText();
		#$categories = $skin->getCategoryLinks();

		$s =
		'<?xml version="1.0" encoding="UTF-8" ?>' . "\n" .
		Xml::tags( 'livepreview', null,
			Xml::element( 'preview', null, $previewText )
			#.	Xml::element( 'category', null, $categories )
		);
		echo $s;
	}


	/**
	 * Get a diff between the current contents of the edit box and the
	 * version of the page we're editing from.
	 *
	 * If this is a section edit, we'll replace the section as for final
	 * save and then make a comparison.
	 */
	function showDiff() {
		$oldtext = $this->mArticle->fetchContent();
		$newtext = $this->mArticle->replaceSection(
			$this->section, $this->textbox1, $this->summary, $this->edittime );
		$newtext = $this->mArticle->preSaveTransform( $newtext );
		$oldtitle = wfMsgExt( 'currentrev', array('parseinline') );
		$newtitle = wfMsgExt( 'yourtext', array('parseinline') );
		if ( $oldtext !== false  || $newtext != '' ) {
			$de = new DifferenceEngine( $this->mTitle );
			$de->setText( $oldtext, $newtext );
			$difftext = $de->getDiff( $oldtitle, $newtitle );
			$de->showDiffStyle();
		} else {
			$difftext = '';
		}

		global $wgOut;
		$wgOut->addHtml( '<div id="wikiDiff">' . $difftext . '</div>' );
	}

	/**
	 * Filter an input field through a Unicode de-armoring process if it
	 * came from an old browser with known broken Unicode editing issues.
	 *
	 * @param WebRequest $request
	 * @param string $field
	 * @return string
	 * @private
	 */
	function safeUnicodeInput( $request, $field ) {
		$text = rtrim( $request->getText( $field ) );
		return $request->getBool( 'safemode' )
			? $this->unmakesafe( $text )
			: $text;
	}

	/**
	 * Filter an output field through a Unicode armoring process if it is
	 * going to an old browser with known broken Unicode editing issues.
	 *
	 * @param string $text
	 * @return string
	 * @private
	 */
	function safeUnicodeOutput( $text ) {
		global $wgContLang;
		$codedText = $wgContLang->recodeForEdit( $text );
		return $this->checkUnicodeCompliantBrowser()
			? $codedText
			: $this->makesafe( $codedText );
	}

	/**
	 * A number of web browsers are known to corrupt non-ASCII characters
	 * in a UTF-8 text editing environment. To protect against this,
	 * detected browsers will be served an armored version of the text,
	 * with non-ASCII chars converted to numeric HTML character references.
	 *
	 * Preexisting such character references will have a 0 added to them
	 * to ensure that round-trips do not alter the original data.
	 *
	 * @param string $invalue
	 * @return string
	 * @private
	 */
	function makesafe( $invalue ) {
		// Armor existing references for reversability.
		$invalue = strtr( $invalue, array( "&#x" => "&#x0" ) );

		$bytesleft = 0;
		$result = "";
		$working = 0;
		for( $i = 0; $i < strlen( $invalue ); $i++ ) {
			$bytevalue = ord( $invalue{$i} );
			if( $bytevalue <= 0x7F ) { //0xxx xxxx
				$result .= chr( $bytevalue );
				$bytesleft = 0;
			} elseif( $bytevalue <= 0xBF ) { //10xx xxxx
				$working = $working << 6;
				$working += ($bytevalue & 0x3F);
				$bytesleft--;
				if( $bytesleft <= 0 ) {
					$result .= "&#x" . strtoupper( dechex( $working ) ) . ";";
				}
			} elseif( $bytevalue <= 0xDF ) { //110x xxxx
				$working = $bytevalue & 0x1F;
				$bytesleft = 1;
			} elseif( $bytevalue <= 0xEF ) { //1110 xxxx
				$working = $bytevalue & 0x0F;
				$bytesleft = 2;
			} else { //1111 0xxx
				$working = $bytevalue & 0x07;
				$bytesleft = 3;
			}
		}
		return $result;
	}

	/**
	 * Reverse the previously applied transliteration of non-ASCII characters
	 * back to UTF-8. Used to protect data from corruption by broken web browsers
	 * as listed in $wgBrowserBlackList.
	 *
	 * @param string $invalue
	 * @return string
	 * @private
	 */
	function unmakesafe( $invalue ) {
		$result = "";
		for( $i = 0; $i < strlen( $invalue ); $i++ ) {
			if( ( substr( $invalue, $i, 3 ) == "&#x" ) && ( $invalue{$i+3} != '0' ) ) {
				$i += 3;
				$hexstring = "";
				do {
					$hexstring .= $invalue{$i};
					$i++;
				} while( ctype_xdigit( $invalue{$i} ) && ( $i < strlen( $invalue ) ) );

				// Do some sanity checks. These aren't needed for reversability,
				// but should help keep the breakage down if the editor
				// breaks one of the entities whilst editing.
				if ((substr($invalue,$i,1)==";") and (strlen($hexstring) <= 6)) {
					$codepoint = hexdec($hexstring);
					$result .= codepointToUtf8( $codepoint );
				} else {
					$result .= "&#x" . $hexstring . substr( $invalue, $i, 1 );
				}
			} else {
				$result .= substr( $invalue, $i, 1 );
			}
		}
		// reverse the transform that we made for reversability reasons.
		return strtr( $result, array( "&#x0" => "&#x" ) );
	}

	function noCreatePermission() {
		global $wgOut;
		$wgOut->setPageTitle( wfMsg( 'nocreatetitle' ) );
		$wgOut->addWikiMsg( 'nocreatetext' );
	}

	/**
	 * If there are rows in the deletion log for this page, show them,
	 * along with a nice little note for the user
	 *
	 * @param OutputPage $out
	 */
	protected function showDeletionLog( $out ) {
		global $wgUser;
		$loglist = new LogEventsList( $wgUser->getSkin(), $out );
		$pager = new LogPager( $loglist, 'delete', false, $this->mTitle->getPrefixedText() );
		if( $pager->getNumRows() > 0 ) {
			$out->addHtml( '<div id="mw-recreate-deleted-warn">' );
			$out->addWikiMsg( 'recreate-deleted-warn' );
			$out->addHTML(
				$loglist->beginLogEventsList() .
				$pager->getBody() .
				$loglist->endLogEventsList()
			);
			$out->addHtml( '</div>' );
		}
	}

	/**
	 * Attempt submission
	 * @return bool false if output is done, true if the rest of the form should be displayed
	 */
	function attemptSave() {
		global $wgUser, $wgOut, $wgTitle, $wgRequest;

		$resultDetails = false;
		$value = $this->internalAttemptSave( $resultDetails, $wgUser->isAllowed('bot') && $wgRequest->getBool('bot', true) );

		if( $value == self::AS_SUCCESS_UPDATE || $value == self::AS_SUCCESS_NEW_ARTICLE ) {
			$this->didSave = true;
		}

		switch ($value) {
			case self::AS_HOOK_ERROR_EXPECTED:
			case self::AS_CONTENT_TOO_BIG:
		 	case self::AS_ARTICLE_WAS_DELETED:
			case self::AS_CONFLICT_DETECTED:
			case self::AS_SUMMARY_NEEDED:
			case self::AS_TEXTBOX_EMPTY:
			case self::AS_MAX_ARTICLE_SIZE_EXCEEDED:
			case self::AS_END:
				return true;

			case self::AS_HOOK_ERROR:
			case self::AS_FILTERING:
			case self::AS_SUCCESS_NEW_ARTICLE:
			case self::AS_SUCCESS_UPDATE:
				return false;

			case self::AS_SPAM_ERROR:
				$this->spamPage ( $resultDetails['spam'] );
				return false;

			case self::AS_BLOCKED_PAGE_FOR_USER:
				$this->blockedPage();
				return false;

			case self::AS_IMAGE_REDIRECT_ANON:
				$wgOut->showErrorPage( 'uploadnologin', 'uploadnologintext' );
				return false;

			case self::AS_READ_ONLY_PAGE_ANON:
				$this->userNotLoggedInPage();
				return false;

		 	case self::AS_READ_ONLY_PAGE_LOGGED:
		 	case self::AS_READ_ONLY_PAGE:
		 		$wgOut->readOnlyPage();
		 		return false;

		 	case self::AS_RATE_LIMITED:
		 		$wgOut->rateLimited();
		 		return false;

		 	case self::AS_NO_CREATE_PERMISSION;
		 		$this->noCreatePermission();
		 		return;

			case self::AS_BLANK_ARTICLE:
		 		$wgOut->redirect( $wgTitle->getFullURL() );
		 		return false;

			case self::AS_IMAGE_REDIRECT_LOGGED:
				$wgOut->permissionRequired( 'upload' );
				return false;
		}
	}
	
	function getBaseRevision() {
		if ($this->mBaseRevision == false) {
			$db = wfGetDB( DB_MASTER );
			$baseRevision = Revision::loadFromTimestamp(
				$db, $this->mTitle, $this->edittime );
			return $this->mBaseRevision = $baseRevision;
		} else {
			return $this->mBaseRevision;
		}
	}
}
