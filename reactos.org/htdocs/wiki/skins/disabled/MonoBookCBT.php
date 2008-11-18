<?php

if ( !defined( 'MEDIAWIKI' ) ) {
	die( "This file is part of MediaWiki, it is not a valid entry point\n" );
}

require_once( 'includes/cbt/CBTProcessor.php' );
require_once( 'includes/cbt/CBTCompiler.php' );
require_once( 'includes/SkinTemplate.php' );

/**
 * MonoBook clone using the new dependency-tracking template processor.
 * EXPERIMENTAL - use only for testing and profiling at this stage.
 *
 * See includes/cbt/README for an explanation.
 *
 * The main thing that's missing is cache invalidation, on change of:
 *   * messages
 *   * user preferences
 *   * source template
 *   * source code and configuration files
 *
 * The other thing is that lots of dependencies that are declared in the callbacks
 * are not intelligently handled. There's some room for improvement there.
 *
 * The class is derived from SkinTemplate, but that's only temporary. Eventually 
 * it'll be derived from Skin, and I've avoided using SkinTemplate functions as 
 * much as possible. In fact, the only SkinTemplate dependencies I know of at the 
 * moment are the functions to generate the gen=css and gen=js files. 
 * 
 */
class SkinMonoBookCBT extends SkinTemplate {
	var $mOut, $mTitle;
	var $mStyleName = 'monobook';
	var $mCompiling = false;
	var $mFunctionCache = array();

	/******************************************************
	 * General functions                                  *
	 ******************************************************/
	
	/** Execute the template and write out the result */
	function outputPage( &$out ) {
		echo $this->execute( $out );
	}

	function execute( &$out ) {
		global $wgTitle, $wgStyleDirectory, $wgParserCacheType;
		$fname = 'SkinMonoBookCBT::execute';
		wfProfileIn( $fname );
		wfProfileIn( "$fname-setup" );
		Skin::initPage( $out );
		
		$this->mOut =&	$out;
		$this->mTitle =& $wgTitle;

		$sourceFile = "$wgStyleDirectory/MonoBook.tpl";

		wfProfileOut( "$fname-setup" );
	
		if ( $wgParserCacheType == CACHE_NONE ) {
			$template = file_get_contents( $sourceFile );
			$text = $this->executeTemplate( $template );
		} else {
			$compiled = $this->getCompiledTemplate( $sourceFile );
			
			wfProfileIn( "$fname-eval" );
			$text = eval( $compiled );
			wfProfileOut( "$fname-eval" );
		}
		wfProfileOut( $fname );
		return $text;
	}

	function getCompiledTemplate( $sourceFile ) {
		global $wgDBname, $wgMemc, $wgRequest, $wgUser, $parserMemc;
		$fname = 'SkinMonoBookCBT::getCompiledTemplate';

		$expiry = 3600;
		
		// Sandbox template execution
		if ( $this->mCompiling ) {
			return;
		}
		
		wfProfileIn( $fname );

		// Is the request an ordinary page view?
		if ( $wgRequest->wasPosted() || 
				count( array_diff( array_keys( $_GET ), array( 'title', 'useskin', 'recompile' ) ) ) != 0 ) 
		{
			$type = 'nonview';
		} else {
			$type = 'view';
		}

		// Per-user compiled template
		// Put all logged-out users on the same cache key
		$cacheKey = "$wgDBname:monobookcbt:$type:" . $wgUser->getId();

		$recompile = $wgRequest->getVal( 'recompile' );
		if ( $recompile == 'user' ) {
			$recompileUser = true;
			$recompileGeneric = false;
		} elseif ( $recompile ) {
			$recompileUser = true;
			$recompileGeneric = true;
		} else {
			$recompileUser = false;
			$recompileGeneric = false;
		}
		
		if ( !$recompileUser ) { 
			$php = $parserMemc->get( $cacheKey );
		}
		if ( $recompileUser || !$php ) {
			if ( $wgUser->isLoggedIn() ) {
				// Perform staged compilation
				// First compile a generic template for all logged-in users
				$genericKey = "$wgDBname:monobookcbt:$type:loggedin";
				if ( !$recompileGeneric ) {
					$template = $parserMemc->get( $genericKey );
				}
				if ( $recompileGeneric || !$template ) {
					$template = file_get_contents( $sourceFile );
					$ignore = array( 'loggedin', '!loggedin dynamic' );
					if ( $type == 'view' ) {
						$ignore[] = 'nonview dynamic';
					}
					$template = $this->compileTemplate( $template, $ignore );
					$parserMemc->set( $genericKey, $template, $expiry );
				}
			} else {
				$template = file_get_contents( $sourceFile );
			}

			$ignore = array( 'lang', 'loggedin', 'user' );
			if ( $wgUser->isLoggedIn() ) {
				$ignore[] = '!loggedin dynamic';
			} else {
				$ignore[] = 'loggedin dynamic';
			}
			if ( $type == 'view' ) {
				$ignore[] = 'nonview dynamic';
			}
			$compiled = $this->compileTemplate( $template, $ignore );

			// Reduce whitespace
			// This is done here instead of in CBTProcessor because we can be 
			// more sure it is safe here.
			$compiled = preg_replace( '/^[ \t]+/m', '', $compiled );
			$compiled = preg_replace( '/[\r\n]+/', "\n", $compiled );

			// Compile to PHP
			$compiler = new CBTCompiler( $compiled );
			$ret = $compiler->compile();
			if ( $ret !== true ) {
				echo $ret;
				wfErrorExit();
			}
			$php = $compiler->generatePHP( '$this' );

			$parserMemc->set( $cacheKey, $php, $expiry );
		}
		wfProfileOut( $fname );
		return $php;
	}

	function compileTemplate( $template, $ignore ) {
		$tp = new CBTProcessor( $template, $this, $ignore );
		$tp->mFunctionCache = $this->mFunctionCache;

		$this->mCompiling = true;
		$compiled = $tp->compile();
		$this->mCompiling = false;

		if ( $tp->getLastError() ) {
			// If there was a compile error, don't save the template
			// Instead just print the error and exit
			echo $compiled;
			wfErrorExit();
		}
		$this->mFunctionCache = $tp->mFunctionCache;
		return $compiled;
	}

	function executeTemplate( $template ) {
		$fname = 'SkinMonoBookCBT::executeTemplate';
		wfProfileIn( $fname );
		$tp = new CBTProcessor( $template, $this );
		$tp->mFunctionCache = $this->mFunctionCache;
		
		$this->mCompiling = true;
		$text = $tp->execute();
		$this->mCompiling = false;

		$this->mFunctionCache = $tp->mFunctionCache;
		wfProfileOut( $fname );
		return $text;
	}
	
	/******************************************************
	 * Callbacks                                          *
	 ******************************************************/

	function lang() { return $GLOBALS['wgContLanguageCode']; }

	function dir() {
		global $wgContLang;
		return $wgContLang->isRTL() ? 'rtl' : 'ltr';
	}
	
	function mimetype() { return $GLOBALS['wgMimeType']; }
	function charset() { return $GLOBALS['wgOutputEncoding']; }
	function headlinks() { 
		return cbt_value( $this->mOut->getHeadLinks(), 'dynamic' );
	}
	function headscripts() { 
		return cbt_value( $this->mOut->getScript(), 'dynamic' );
	}
	
	function pagetitle() { 
		return cbt_value( $this->mOut->getHTMLTitle(), array( 'title', 'lang' ) ); 
	}
	
	function stylepath() { return $GLOBALS['wgStylePath']; }
	function stylename() { return $this->mStyleName; }
	
	function notprintable() {
		global $wgRequest;
		return cbt_value( !$wgRequest->getBool( 'printable' ), 'nonview dynamic' );
	}
	
	function jsmimetype() { return $GLOBALS['wgJsMimeType']; }
	
	function jsvarurl() {
		global $wgUseSiteJs, $wgUser;
		if ( !$wgUseSiteJs ) return '';
		
		if ( $wgUser->isLoggedIn() ) {
			$url = self::makeUrl( '-','action=raw&smaxage=0&gen=js' );
		} else {
			$url = self::makeUrl( '-','action=raw&gen=js' );
		}
		return cbt_value( $url, 'loggedin' );
	}
	
	function pagecss() {
		global $wgHooks;
		
		$out = false;
		wfRunHooks( 'SkinTemplateSetupPageCss', array( &$out ) );

		// Unknown dependencies
		return cbt_value( $out, 'dynamic' );
	}
	
	function usercss() {
		if ( $this->isCssPreview() ) {
			global $wgRequest;
			$usercss = $this->makeStylesheetCdata( $wgRequest->getText('wpTextbox1') );
		} else {
			$usercss = $this->makeStylesheetLink( self::makeUrl($this->getUserPageText() . 
				'/'.$this->mStyleName.'.css', 'action=raw&ctype=text/css' ) );
		}

		// Dynamic when not an ordinary page view, also depends on the username
		return cbt_value( $usercss, array( 'nonview dynamic', 'user' ) );
	}
	
	function sitecss() {
		global $wgUseSiteCss;
		if ( !$wgUseSiteCss ) {
			return '';
		}

		global $wgSquidMaxage, $wgContLang, $wgStylePath;
			
		$query = "action=raw&ctype=text/css&smaxage=$wgSquidMaxage";
		
		$sitecss = '';
		if ( $wgContLang->isRTL() ) {
			$sitecss .= $this->makeStylesheetLink( $wgStylePath . '/' . $this->mStyleName . '/rtl.css' ) . "\n";
		}

		$sitecss .= $this->makeStylesheetLink( self::makeNSUrl( 'Common.css', $query, NS_MEDIAWIKI ) ) . "\n";
		$sitecss .= $this->makeStylesheetLink( self::makeNSUrl( ucfirst( $this->mStyleName ) . '.css', $query, NS_MEDIAWIKI ) ) . "\n";

		// No deps
		return $sitecss;
	}

	function gencss() {
		global $wgUseSiteCss;
		if ( !$wgUseSiteCss ) return '';
		
		global $wgSquidMaxage, $wgUser, $wgAllowUserCss;
		if ( $this->isCssPreview() ) {
			$siteargs = '&smaxage=0&maxage=0';
		} else {
			$siteargs = '&maxage=' . $wgSquidMaxage;
		}
		if ( $wgAllowUserCss && $wgUser->isLoggedIn() ) {
			$siteargs .= '&ts={user_touched}';
			$isTemplate = true;
		} else {
			$isTemplate = false;
		}
		
		$link = $this->makeStylesheetLink( self::makeUrl('-','action=raw&gen=css' . $siteargs) ) . "\n";

		if ( $wgAllowUserCss ) {
			$deps = 'loggedin';
		} else { 
			$deps = array();
		}
		return cbt_value( $link, $deps, $isTemplate );
	}
	
	function user_touched() {
		global $wgUser;
		return cbt_value( $wgUser->mTouched, 'dynamic' );
	}
		
	function userjs() {
		global $wgAllowUserJs, $wgJsMimeType;
		if ( !$wgAllowUserJs ) return '';
		
		if ( $this->isJsPreview() ) {
			$url = '';
		} else {
			$url = self::makeUrl($this->getUserPageText().'/'.$this->mStyleName.'.js', 'action=raw&ctype='.$wgJsMimeType.'&dontcountme=s');
		}
		return cbt_value( $url, array( 'nonview dynamic', 'user' ) );
	}
	
	function userjsprev() {
		global $wgAllowUserJs, $wgRequest;
		if ( !$wgAllowUserJs ) return '';
		if ( $this->isJsPreview() ) {
			$js = '/*<![CDATA[*/ ' . $wgRequest->getText('wpTextbox1') . ' /*]]>*/';
		} else {
			$js = '';
		}
		return cbt_value( $js, array( 'nonview dynamic' ) );
	}
	
	function trackbackhtml() {
		global $wgUseTrackbacks;
		if ( !$wgUseTrackbacks ) return '';

		if ( $this->mOut->isArticleRelated() ) {
			$tb = $this->mTitle->trackbackRDF();
		} else {
			$tb = '';
		}
		return cbt_value( $tb, 'dynamic' );
	}
	
	function body_ondblclick() {
		global $wgUser;
		if( $this->isEditable() && $wgUser->getOption("editondblclick") ) {
			$js = 'document.location = "' . $this->getEditUrl() .'";';
		} else {
			$js = '';
		}

		if ( User::getDefaultOption('editondblclick') ) {
			return cbt_value( $js, 'user', 'title' );
		} else {
			// Optimise away for logged-out users
			return cbt_value( $js, 'loggedin dynamic' );
		}
	}
	
	function body_onload() {
		global $wgUser;
		if ( $this->isEditable() && $wgUser->getOption( 'editsectiononrightclick' ) ) {
			$js = 'setupRightClickEdit()';
		} else {
			$js = '';
		}
		return cbt_value( $js, 'loggedin dynamic' );
	}
	
	function nsclass() {
		return cbt_value( 'ns-' . $this->mTitle->getNamespace(), 'title' );
	}
	
	function sitenotice() {
		// Perhaps this could be given special dependencies using our knowledge of what 
		// wfGetSiteNotice() depends on.
		return cbt_value( wfGetSiteNotice(), 'dynamic' );
	}
	
	function title() {
		return cbt_value( $this->mOut->getPageTitle(), array( 'title', 'lang' ) );
	}

	function title_urlform() {
		return cbt_value( $this->getThisTitleUrlForm(), 'title' );
	}

	function title_userurl() {
		return cbt_value( urlencode( $this->mTitle->getDBkey() ), 'title' );
	}

	function subtitle() {
		$subpagestr = $this->subPageSubtitle();
		if ( !empty( $subpagestr ) ) {
			$s = '<span class="subpages">'.$subpagestr.'</span>'.$this->mOut->getSubtitle();
		} else {
			$s = $this->mOut->getSubtitle();
		}
		return cbt_value( $s, array( 'title', 'nonview dynamic' ) );
	}
	
	function undelete() {
		return cbt_value( $this->getUndeleteLink(), array( 'title', 'lang' ) );
	}
	
	function newtalk() {
		global $wgUser, $wgDBname;
		$newtalks = $wgUser->getNewMessageLinks();

		if (count($newtalks) == 1 && $newtalks[0]["wiki"] === $wgDBname) {
			$usertitle = $this->getUserPageTitle();
			$usertalktitle = $usertitle->getTalkPage();
			if( !$usertalktitle->equals( $this->mTitle ) ) {
				$ntl = wfMsg( 'youhavenewmessages',
					$this->makeKnownLinkObj(
						$usertalktitle,
						wfMsgHtml( 'newmessageslink' ),
						'redirect=no'
					),
					$this->makeKnownLinkObj(
						$usertalktitle,
						wfMsgHtml( 'newmessagesdifflink' ),
						'diff=cur'
					)
				);
				# Disable Cache
				$this->mOut->setSquidMaxage(0);
			}
		} else if (count($newtalks)) {
			$sep = str_replace("_", " ", wfMsgHtml("newtalkseperator"));
			$msgs = array();
			foreach ($newtalks as $newtalk) {
				$msgs[] = wfElement("a", 
					array('href' => $newtalk["link"]), $newtalk["wiki"]);
			}
			$parts = implode($sep, $msgs);
			$ntl = wfMsgHtml('youhavenewmessagesmulti', $parts);
			$this->mOut->setSquidMaxage(0);
		} else {
			$ntl = '';
		}
		return cbt_value( $ntl, 'dynamic' );
	}
	
	function showjumplinks() {
		global $wgUser;
		return cbt_value( $wgUser->getOption( 'showjumplinks' ) ? 'true' : '', 'user' );
	}
	
	function bodytext() {
		return cbt_value( $this->mOut->getHTML(), 'dynamic' );
	}
	
	function catlinks() {
		if ( !isset( $this->mCatlinks ) ) {
			$this->mCatlinks = $this->getCategories();
		}
		return cbt_value( $this->mCatlinks, 'dynamic' );
	}
	
	function extratabs( $itemTemplate ) {
		global $wgContLang, $wgDisableLangConversion;
		
		$etpl = cbt_escape( $itemTemplate );

		/* show links to different language variants */
		$variants = $wgContLang->getVariants();
		$s = '';
		if ( !$wgDisableLangConversion && count( $wgContLang->getVariants() ) > 1 ) {
			$vcount=0;
			foreach ( $variants as $code ) {
				$name = $wgContLang->getVariantname( $code );
				if ( $name == 'disable' ) {
					continue;
				}
				$code = cbt_escape( $code );
				$name = cbt_escape( $name );
				$s .= "{ca_variant {{$code}} {{$name}} {{$vcount}} {{$etpl}}}\n";
				$vcount ++;
			}
		}
		return cbt_value( $s, array(), true );
	}

	function is_special() { return cbt_value( $this->mTitle->getNamespace() == NS_SPECIAL, 'title' ); }
	function can_edit() { return cbt_value( (string)($this->mTitle->userCan( 'edit' )), 'dynamic' ); }
	function can_move() { return cbt_value( (string)($this->mTitle->userCan( 'move' )), 'dynamic' ); }
	function is_talk() { return cbt_value( (string)($this->mTitle->isTalkPage()), 'title' ); }
	function is_protected() { return cbt_value( (string)$this->mTitle->isProtected(), 'dynamic' ); }
	function nskey() { return cbt_value( $this->mTitle->getNamespaceKey(), 'title' ); }

	function request_url() {
		global $wgRequest;
		return cbt_value( $wgRequest->getRequestURL(), 'dynamic' );
	}

	function subject_url() { 
		$title = $this->getSubjectPage();
		if ( $title->exists() ) {
			$url = $title->getLocalUrl();
		} else {
			$url = $title->getLocalUrl( 'action=edit' );
		}
		return cbt_value( $url, 'title' ); 
	}

	function talk_url() {
		$title = $this->getTalkPage();
		if ( $title->exists() ) {
			$url = $title->getLocalUrl();
		} else {
			$url = $title->getLocalUrl( 'action=edit' );
		}
		return cbt_value( $url, 'title' );
	}

	function edit_url() {
		return cbt_value( $this->getEditUrl(), array( 'title', 'nonview dynamic' ) );
	}

	function move_url() {
		return cbt_value( $this->makeSpecialParamUrl( 'Movepage' ), array(), true );
	}

	function localurl( $query ) {
		return cbt_value( $this->mTitle->getLocalURL( $query ), 'title' );
	}

	function selecttab( $tab, $extraclass = '' ) {
		if ( !isset( $this->mSelectedTab ) ) {
			$prevent_active_tabs = false ;
			wfRunHooks( 'SkinTemplatePreventOtherActiveTabs', array( &$this , &$preventActiveTabs ) );

			$actionTabs = array(
				'edit' => 'edit',
				'submit' => 'edit',
				'history' => 'history',
				'protect' => 'protect',
				'unprotect' => 'protect',
				'delete' => 'delete',
				'watch' => 'watch',
				'unwatch' => 'watch',
			);
			if ( $preventActiveTabs ) {
				$this->mSelectedTab = false;
			} else {
				$action = $this->getAction();
				$section = $this->getSection();
				
				if ( isset( $actionTabs[$action] ) ) {
					$this->mSelectedTab = $actionTabs[$action];

					if ( $this->mSelectedTab == 'edit' && $section == 'new' ) {
						$this->mSelectedTab = 'addsection';
					}
				} elseif ( $this->mTitle->isTalkPage() ) {
					$this->mSelectedTab = 'talk';
				} else {
					$this->mSelectedTab = 'subject';
				}
			}
		}
		if ( $extraclass ) {
			if ( $this->mSelectedTab == $tab ) {
				$s = 'class="selected ' . htmlspecialchars( $extraclass ) . '"';
			} else {
				$s = 'class="' . htmlspecialchars( $extraclass ) . '"';
			}			
		} else {
			if ( $this->mSelectedTab == $tab ) {
				$s = 'class="selected"';
			} else {
				$s = '';
			}
		}
		return cbt_value( $s, array( 'nonview dynamic', 'title' ) );
	}

	function subject_newclass() {
		$title = $this->getSubjectPage();
		$class = $title->exists() ? '' : 'new';
		return cbt_value( $class, 'dynamic' );
	}

	function talk_newclass() {
		$title = $this->getTalkPage();
		$class = $title->exists() ? '' : 'new';
		return cbt_value( $class, 'dynamic' );
	}	

	function ca_variant( $code, $name, $index, $template ) {
		global $wgContLang;
		$selected = ($code == $wgContLang->getPreferredVariant());
		$action = $this->getAction();
		$actstr = '';
		if( $action )
			$actstr = 'action=' . $action . '&';
		$s = strtr( $template, array( 
			'$id' => htmlspecialchars( 'varlang-' . $index ),
			'$class' => $selected ? 'class="selected"' : '',
			'$text' => $name,
			'$href' => htmlspecialchars( $this->mTitle->getLocalUrl( $actstr . 'variant=' . $code ) )
		));
		return cbt_value( $s, 'dynamic' );
	}

	function is_watching() {
		return cbt_value( (string)$this->mTitle->userIsWatching(), array( 'dynamic' ) );
	}

	
	function personal_urls( $itemTemplate ) {
		global $wgShowIPinHeader, $wgContLang;

		# Split this function up into many small functions, to obtain the
		# best specificity in the dependencies of each one. The template below 
		# has no dependencies, so its generation, and any static subfunctions,
		# can be optimised away.
		$etpl = cbt_escape( $itemTemplate );
		$s = "
			{userpage {{$etpl}}}
			{mytalk {{$etpl}}}
			{preferences {{$etpl}}}
			{watchlist {{$etpl}}}
			{mycontris {{$etpl}}}
			{logout {{$etpl}}}
		";

		if ( $wgShowIPinHeader ) {
			$s .= "
				{anonuserpage {{$etpl}}}
				{anontalk {{$etpl}}}
				{anonlogin {{$etpl}}}
			";
		} else {
			$s .= "{login {{$etpl}}}\n";
		}
		// No dependencies
		return cbt_value( $s, array(), true /*this is a template*/ );
	}

	function userpage( $itemTemplate ) {
		global $wgUser;
		if ( $this->isLoggedIn() ) {
			$userPage = $this->getUserPageTitle();
			$s = $this->makeTemplateLink( $itemTemplate, 'userpage', $userPage, $wgUser->getName() );
		} else {
			$s = '';
		}
		return cbt_value( $s, 'user' );
	}
	
	function mytalk( $itemTemplate ) {
		global $wgUser;
		if ( $this->isLoggedIn() ) {
			$userPage = $this->getUserPageTitle();
			$talkPage = $userPage->getTalkPage();
			$s = $this->makeTemplateLink( $itemTemplate, 'mytalk', $talkPage, wfMsg('mytalk') );
		} else {
			$s = '';
		}
		return cbt_value( $s, 'user' );
	}
	
	function preferences( $itemTemplate ) {
		if ( $this->isLoggedIn() ) {
			$s = $this->makeSpecialTemplateLink( $itemTemplate, 'preferences', 
				'Preferences', wfMsg( 'preferences' ) );
		} else {
			$s = '';
		}
		return cbt_value( $s, array( 'loggedin', 'lang' ) );
	}
	
	function watchlist( $itemTemplate ) {
		if ( $this->isLoggedIn() ) {
			$s = $this->makeSpecialTemplateLink( $itemTemplate, 'watchlist', 
				'Watchlist', wfMsg( 'watchlist' ) );
		} else {
			$s = '';
		}
		return cbt_value( $s, array( 'loggedin', 'lang' ) );
	}
	
	function mycontris( $itemTemplate ) {
		if ( $this->isLoggedIn() ) {
			global $wgUser;
			$s = $this->makeSpecialTemplateLink( $itemTemplate, 'mycontris', 
				"Contributions/" . $wgUser->getTitleKey(), wfMsg('mycontris') );
		} else {
			$s = '';
		}
		return cbt_value( $s, 'user' );
	}
	
	function logout( $itemTemplate ) {
		if ( $this->isLoggedIn() ) {
			$s = $this->makeSpecialTemplateLink( $itemTemplate, 'logout', 
				'Userlogout', wfMsg( 'userlogout' ), 
					$this->mTitle->getNamespace() === NS_SPECIAL && $this->mTitle->getText() === 'Preferences' 
					? '' : "returnto=" . $this->mTitle->getPrefixedURL() );
		} else {
			$s = '';
		}
		return cbt_value( $s, 'loggedin dynamic' );
	}
	
	function anonuserpage( $itemTemplate ) {
		if ( $this->isLoggedIn() ) {
			$s = '';
		} else {
			global $wgUser;
			$userPage = $this->getUserPageTitle();
			$s = $this->makeTemplateLink( $itemTemplate, 'userpage', $userPage, $wgUser->getName() );
		}
		return cbt_value( $s, '!loggedin dynamic' );
	}
	
	function anontalk( $itemTemplate ) {
		if ( $this->isLoggedIn() ) {
			$s = '';
		} else {
			$userPage = $this->getUserPageTitle();
			$talkPage = $userPage->getTalkPage();
			$s = $this->makeTemplateLink( $itemTemplate, 'mytalk', $talkPage, wfMsg('anontalk') );
		}
		return cbt_value( $s, '!loggedin dynamic' );
	}
	
	function anonlogin( $itemTemplate ) {
		if ( $this->isLoggedIn() ) {
			$s = '';
		} else {
			$s = $this->makeSpecialTemplateLink( $itemTemplate, 'anonlogin', 'Userlogin', 
				wfMsg( 'userlogin' ), 'returnto=' . urlencode( $this->getThisPDBK() ) );
		}
		return cbt_value( $s, '!loggedin dynamic' );
	}
	
	function login( $itemTemplate ) {
		if ( $this->isLoggedIn() ) {
			$s = '';
		} else {
			$s = $this->makeSpecialTemplateLink( $itemTemplate, 'login', 'Userlogin', 
				wfMsg( 'userlogin' ), 'returnto=' . urlencode( $this->getThisPDBK() ) );
		}
		return cbt_value( $s, '!loggedin dynamic' );
	}
	
	function logopath() { return $GLOBALS['wgLogo']; }
	function mainpage() { return self::makeMainPageUrl(); }
	
	function sidebar( $startSection, $endSection, $innerTpl ) {
		$s = '';
		$lines = explode( "\n", wfMsgForContent( 'sidebar' ) );
		$firstSection = true;
		foreach ($lines as $line) {
			if (strpos($line, '*') !== 0)
				continue;
			if (strpos($line, '**') !== 0) {
				$bar = trim($line, '* ');
				$name = wfMsg( $bar ); 
				if (wfEmptyMsg($bar, $name)) {
					$name = $bar;
				}
				if ( $firstSection ) {
					$firstSection = false;
				} else {
					$s .= $endSection;
				}
				$s .= strtr( $startSection,
					array( 
						'$bar' => htmlspecialchars( $bar ),
						'$barname' => $name
					) );
			} else {
				if (strpos($line, '|') !== false) { // sanity check
					$line = explode( '|' , trim($line, '* '), 2 );
					$link = wfMsgForContent( $line[0] );
					if ($link == '-')
						continue;
					if (wfEmptyMsg($line[1], $text = wfMsg($line[1])))
						$text = $line[1];
					if (wfEmptyMsg($line[0], $link))
						$link = $line[0];
					$href = self::makeInternalOrExternalUrl( $link );
					
					$s .= strtr( $innerTpl,
						array(
							'$text' => htmlspecialchars( $text ),
							'$href' => htmlspecialchars( $href ),
							'$id' => htmlspecialchars( 'n-' . strtr($line[1], ' ', '-') ),
							'$classactive' => ''
						) );
				} else { continue; }
			}
		}
		if ( !$firstSection ) {
			$s .= $endSection;
		}

		// Depends on user language only
		return cbt_value( $s, 'lang' );
	}
	
	function searchaction() {
		// Static link
		return $this->getSearchLink();
	}
	
	function search() {
		global $wgRequest;
		return cbt_value( trim( $this->getSearch() ), 'special dynamic' );
	}
	
	function notspecialpage() {
		return cbt_value( $this->mTitle->getNamespace() != NS_SPECIAL, 'special' );
	}
	
	function nav_whatlinkshere() {
		return cbt_value( $this->makeSpecialParamUrl('Whatlinkshere' ), array(), true );
	}

	function article_exists() {
		return cbt_value( (string)($this->mTitle->getArticleId() !== 0), 'title' );
	}
	
	function nav_recentchangeslinked() {
		return cbt_value( $this->makeSpecialParamUrl('Recentchangeslinked' ), array(), true );
	}
	
	function feeds( $itemTemplate = '' ) {
		if ( !$this->mOut->isSyndicated() ) {
			$feeds = '';
		} elseif ( $itemTemplate == '' ) {
			// boolean only required
			$feeds = 'true';
		} else {
			$feeds = '';
			global $wgFeedClasses, $wgRequest;
			foreach( $wgFeedClasses as $format => $class ) {
				$feeds .= strtr( $itemTemplate,
					array( 
						'$key' => htmlspecialchars( $format ),
						'$text' => $format,
						'$href' => $wgRequest->appendQuery( "feed=$format" )
					) );
			}
		}
		return cbt_value( $feeds, 'special dynamic' );
	}

	function is_userpage() {
		list( $id, $ip ) = $this->getUserPageIdIp();
		return cbt_value( (string)($id || $ip), 'title' );
	}

	function is_ns_mediawiki() {
		return cbt_value( (string)$this->mTitle->getNamespace() == NS_MEDIAWIKI, 'title' );
	}

	function is_loggedin() {
		global $wgUser;
		return cbt_value( (string)($wgUser->isLoggedIn()), 'loggedin' );
	}

	function nav_contributions() {
		$url = $this->makeSpecialParamUrl( 'Contributions', '', '{title_userurl}' );
		return cbt_value( $url, array(), true );
	}

	function is_allowed( $right ) {
		global $wgUser;
		return cbt_value( (string)$wgUser->isAllowed( $right ), 'user' );
	}
	
	function nav_blockip() {
		$url = $this->makeSpecialParamUrl( 'Blockip', '', '{title_userurl}' );
		return cbt_value( $url, array(), true );
	}
	
	function nav_emailuser() {
		global $wgEnableEmail, $wgEnableUserEmail, $wgUser;
		if ( !$wgEnableEmail || !$wgEnableUserEmail ) return '';
		
		$url = $this->makeSpecialParamUrl( 'Emailuser', '', '{title_userurl}' );
		return cbt_value( $url, array(), true );
	}
	
	function nav_upload() {
		global $wgEnableUploads, $wgUploadNavigationUrl;
		if ( !$wgEnableUploads ) { 
			return '';
		} elseif ( $wgUploadNavigationUrl ) {
			return $wgUploadNavigationUrl;
		} else {
			return self::makeSpecialUrl('Upload');
		}
	}
	
	function nav_specialpages() {
		return self::makeSpecialUrl('Specialpages');
	}
	
	function nav_print() {
		global $wgRequest, $wgArticle;
		$action = $this->getAction();
		$url = '';
		if( $this->mTitle->getNamespace() !== NS_SPECIAL 
				&& ($action == '' || $action == 'view' || $action == 'purge' ) ) 
		{
			$revid = $wgArticle->getLatest();
			if ( $revid != 0 ) {
				$url = $wgRequest->appendQuery( 'printable=yes' );
			}
		}
		return cbt_value( $url, array( 'nonview dynamic', 'title' ) );
	}
	
	function nav_permalink() {
		$url = (string)$this->getPermalink();
		return cbt_value( $url, 'dynamic' );
	}

	function nav_trackbacklink() {
		global $wgUseTrackbacks;
		if ( !$wgUseTrackbacks ) return '';

		return cbt_value( $this->mTitle->trackbackURL(), 'title' );
	}
	
	function is_permalink() {
		return cbt_value( (string)($this->getPermalink() === false), 'nonview dynamic' );
	}
	
	function toolboxend() {
		// This is where the MonoBookTemplateToolboxEnd hook went in the old skin
		return '';
	}
	
	function language_urls( $outer, $inner ) {
		global $wgHideInterlanguageLinks, $wgOut, $wgContLang;
		if ( $wgHideInterlanguageLinks ) return '';

		$links = $wgOut->getLanguageLinks();
		$s = '';
		if ( count( $links ) ) {
			foreach( $links as $l ) {
				$tmp = explode( ':', $l, 2 );
				$nt = Title::newFromText( $l );
				$s .= strtr( $inner,
					array(
						'$class' => htmlspecialchars( 'interwiki-' . $tmp[0] ),
						'$href' => htmlspecialchars( $nt->getFullURL() ),
						'$text' => ($wgContLang->getLanguageName( $nt->getInterwiki() ) != ''?
							$wgContLang->getLanguageName( $nt->getInterwiki() ) : $l ),
					)
				);
			}
			$s = str_replace( '$body', $s, $outer );
		}
		return cbt_value( $s, 'dynamic' );
	}
	
	function poweredbyico() { return $this->getPoweredBy(); }
	function copyrightico() { return $this->getCopyrightIcon(); }

	function lastmod() { 
		global $wgMaxCredits;
		if ( $wgMaxCredits ) return '';

		if ( !isset( $this->mLastmod ) ) {
			if ( $this->isCurrentArticleView() ) {
				$this->mLastmod = $this->lastModified(); 
			} else {
				$this->mLastmod = '';
			}
		}
		return cbt_value( $this->mLastmod, 'dynamic' );
	}
	
	function viewcount() {
		global $wgDisableCounters;
		if ( $wgDisableCounters ) return '';
		
		global $wgLang, $wgArticle;
		if ( is_object( $wgArticle ) ) {
			$viewcount = $wgLang->formatNum( $wgArticle->getCount() );
			if ( $viewcount ) {
				$viewcount = wfMsg( "viewcount", $viewcount );
			} else {
				$viewcount = '';
			}
		} else {
			$viewcount = '';
		}
		return cbt_value( $viewcount, 'dynamic' );
   	}
	
	function numberofwatchingusers() {
		global $wgPageShowWatchingUsers;
		if ( !$wgPageShowWatchingUsers ) return '';

		$dbr = wfGetDB( DB_SLAVE );
		extract( $dbr->tableNames( 'watchlist' ) );
		$sql = "SELECT COUNT(*) AS n FROM $watchlist
			WHERE wl_title='" . $dbr->strencode($this->mTitle->getDBkey()) .
			"' AND  wl_namespace=" . $this->mTitle->getNamespace() ;
		$res = $dbr->query( $sql, 'SkinTemplate::outputPage');
		$row = $dbr->fetchObject( $res );
		$num = $row->n;
		if ($num > 0) {
			$s = wfMsg('number_of_watching_users_pageview', $num);
		} else {
			$s = '';
		}
		return cbt_value( $s, 'dynamic' );
	}
	
	function credits() {
		global $wgMaxCredits;
		if ( !$wgMaxCredits ) return '';
		
		if ( $this->isCurrentArticleView() ) {
			require_once("Credits.php");
			global $wgArticle, $wgShowCreditsIfMax;
			$credits = getCredits($wgArticle, $wgMaxCredits, $wgShowCreditsIfMax);
		} else {
			$credits = '';
		}
		return cbt_value( $credits, 'view dynamic' );
	}
	
	function normalcopyright() {
		return $this->getCopyright( 'normal' );
	}

	function historycopyright() {
		return $this->getCopyright( 'history' );
	}

	function is_currentview() {
		global $wgRequest;
		return cbt_value( (string)$this->isCurrentArticleView(), 'view' );
	}

	function usehistorycopyright() {
		global $wgRequest;
		if ( wfMsgForContent( 'history_copyright' ) == '-' ) return '';
		
		$oldid = $this->getOldId();
		$diff = $this->getDiff();
		$use = (string)(!is_null( $oldid ) && is_null( $diff ));
		return cbt_value( $use, 'nonview dynamic' );
	}
	
	function privacy() {
		return cbt_value( $this->privacyLink(), 'lang' );
	}
	function about() {
		return cbt_value( $this->aboutLink(), 'lang' );
	}
	function disclaimer() {
		return cbt_value( $this->disclaimerLink(), 'lang' );
	}
	function tagline() { 
		# A reference to this tag existed in the old MonoBook.php, but the
		# template data wasn't set anywhere
		return ''; 
	}
	function reporttime() {
		return cbt_value( $this->mOut->reportTime(), 'dynamic' );
	}
	
	function msg( $name ) {
		return cbt_value( wfMsg( $name ), 'lang' );
	}
	
	function fallbackmsg( $name, $fallback ) {
		$text = wfMsg( $name );
		if ( wfEmptyMsg( $name, $text ) ) {
			$text = $fallback;
		}
		return cbt_value( $text,  'lang' );
	}

	/******************************************************
	 * Utility functions                                  *
	 ******************************************************/

	/** Return true if this request is a valid, secure CSS preview */
	function isCssPreview() {
		if ( !isset( $this->mCssPreview ) ) {
			global $wgRequest, $wgAllowUserCss, $wgUser;
			$this->mCssPreview = 
				$wgAllowUserCss &&
				$wgUser->isLoggedIn() &&
				$this->mTitle->isCssSubpage() && 
				$this->userCanPreview( $this->getAction() );
		}
		return $this->mCssPreview;
	}

	/** Return true if this request is a valid, secure JS preview */
	function isJsPreview() {
		if ( !isset( $this->mJsPreview ) ) {
			global $wgRequest, $wgAllowUserJs, $wgUser;
			$this->mJsPreview = 
				$wgAllowUserJs &&
				$wgUser->isLoggedIn() &&
				$this->mTitle->isJsSubpage() && 
				$this->userCanPreview( $this->getAction() );
		}
		return $this->mJsPreview;
	}

	/** Get the title of the $wgUser's user page */
	function getUserPageTitle() {
		if ( !isset( $this->mUserPageTitle ) ) {
			global $wgUser;
			$this->mUserPageTitle = $wgUser->getUserPage();
		}
		return $this->mUserPageTitle;
	}

	/** Get the text of the user page title */
	function getUserPageText() {
		if ( !isset( $this->mUserPageText ) ) {
			$userPage = $this->getUserPageTitle();
			$this->mUserPageText = $userPage->getPrefixedText();
		}
		return $this->mUserPageText;
	}

	/** Make an HTML element for a stylesheet link */
	function makeStylesheetLink( $url ) {
		return '<link rel="stylesheet" type="text/css" href="' . htmlspecialchars( $url ) . "\"/>";
	}

	/** Make an XHTML element for inline CSS */
	function makeStylesheetCdata( $style ) {
		return "<style type=\"text/css\"> /*<![CDATA[*/ {$style} /*]]>*/ </style>";
	}

	/** Get the edit URL for this page */
	function getEditUrl() {
		if ( !isset( $this->mEditUrl ) ) {
			$this->mEditUrl = $this->mTitle->getLocalUrl( $this->editUrlOptions() );
		}
		return $this->mEditUrl;
	}

	/** Get the prefixed DB key for this page */
	function getThisPDBK() {
		if ( !isset( $this->mThisPDBK ) ) {
			$this->mThisPDBK = $this->mTitle->getPrefixedDbKey();
		}
		return $this->mThisPDBK;
	}

	function getThisTitleUrlForm() {
		if ( !isset( $this->mThisTitleUrlForm ) ) {
			$this->mThisTitleUrlForm = $this->mTitle->getPrefixedURL();
		}
		return $this->mThisTitleUrlForm;
	}

	/** 
	 * If the current page is a user page, get the user's ID and IP. Otherwise return array(0,false)
	 */
	function getUserPageIdIp() {
		if ( !isset( $this->mUserPageId ) ) {
			if( $this->mTitle->getNamespace() == NS_USER || $this->mTitle->getNamespace() == NS_USER_TALK ) {
				$this->mUserPageId = User::idFromName($this->mTitle->getText());
				$this->mUserPageIp = User::isIP($this->mTitle->getText());
			} else {
				$this->mUserPageId = 0;
				$this->mUserPageIp = false;
			}
		}
		return array( $this->mUserPageId, $this->mUserPageIp );
	}
	
	/**
	 * Returns a permalink URL, or false if the current page is already a 
	 * permalink, or blank if a permalink shouldn't be displayed
	 */
	function getPermalink() {
		if ( !isset( $this->mPermalink ) ) {
			global $wgRequest, $wgArticle;
			$action = $this->getAction();
			$oldid = $this->getOldId();
			$url = '';
			if( $this->mTitle->getNamespace() !== NS_SPECIAL 
					&& $this->mTitle->getArticleId() != 0
					&& ($action == '' || $action == 'view' || $action == 'purge' ) ) 
			{
				if ( !$oldid ) {
					$revid = $wgArticle->getLatest();			
					$url = $this->mTitle->getLocalURL( "oldid=$revid" );
				} else {
					$url = false;
				}
			} else {
				$url = '';
			}
		}
		return $url;
	}

	/**
	 * Returns true if the current page is an article, not a special page,
	 * and we are viewing a revision, not a diff
	 */
	function isArticleView() {
		global $wgOut, $wgArticle, $wgRequest;
		if ( !isset( $this->mIsArticleView ) ) {
			$oldid = $this->getOldId();
			$diff = $this->getDiff();
			$this->mIsArticleView = $wgOut->isArticle() and 
				(!is_null( $oldid ) or is_null( $diff )) and 0 != $wgArticle->getID();
		}
		return $this->mIsArticleView;
	}

	function isCurrentArticleView() {
		if ( !isset( $this->mIsCurrentArticleView ) ) {
			global $wgOut, $wgArticle, $wgRequest;
			$oldid = $this->getOldId();
			$this->mIsCurrentArticleView = $wgOut->isArticle() && is_null( $oldid ) && 0 != $wgArticle->getID();
		}
		return $this->mIsCurrentArticleView;
	}


	/**
	 * Return true if the current page is editable; if edit section on right 
	 * click should be enabled.
	 */
	function isEditable() {
		global $wgRequest;
		$action = $this->getAction();
		return ($this->mTitle->getNamespace() != NS_SPECIAL and !($action == 'edit' or $action == 'submit'));
	}

	/** Return true if the user is logged in */
	function isLoggedIn() {
		global $wgUser;
		return $wgUser->isLoggedIn();
	}

	/** Get the local URL of the current page */
	function getPageUrl() {
		if ( !isset( $this->mPageUrl ) ) {
			$this->mPageUrl = $this->mTitle->getLocalURL();
		} 
		return $this->mPageUrl;
	}

	/** Make a link to a title using a template */
	function makeTemplateLink( $template, $key, $title, $text ) {
		$url = $title->getLocalUrl();
		return strtr( $template, 
			array( 
				'$key' => $key,
				'$classactive' => ($url == $this->getPageUrl()) ? 'class="active"' : '',
				'$class' => $title->getArticleID() == 0 ? 'class="new"' : '', 
				'$href' => htmlspecialchars( $url ),
				'$text' => $text
			 ) );
	}

	/** Make a link to a URL using a template */
	function makeTemplateLinkUrl( $template, $key, $url, $text ) {
		return strtr( $template, 
			array( 
				'$key' => $key,
				'$classactive' => ($url == $this->getPageUrl()) ? 'class="active"' : '',
				'$class' => '', 
				'$href' => htmlspecialchars( $url ),
				'$text' => $text
			 ) );
	}

	/** Make a link to a special page using a template */
	function makeSpecialTemplateLink( $template, $key, $specialName, $text, $query = '' ) {
		$url = self::makeSpecialUrl( $specialName, $query );
		// Ignore the query when comparing
		$active = ($this->mTitle->getNamespace() == NS_SPECIAL && $this->mTitle->getDBkey() == $specialName);
		return strtr( $template, 
			array( 
				'$key' => $key,
				'$classactive' => $active ? 'class="active"' : '',
				'$class' => '', 
				'$href' => htmlspecialchars( $url ),
				'$text' => $text
			 ) );
	}

	function loadRequestValues() {
		global $wgRequest;
		$this->mAction = $wgRequest->getText( 'action' );
		$this->mOldId = $wgRequest->getVal( 'oldid' );
		$this->mDiff = $wgRequest->getVal( 'diff' );
		$this->mSection = $wgRequest->getVal( 'section' );
		$this->mSearch = $wgRequest->getVal( 'search' );
		$this->mRequestValuesLoaded = true;
	}
		


	/** Get the action parameter of the request */
	function getAction() {
		if ( !isset( $this->mRequestValuesLoaded ) ) {
			$this->loadRequestValues();
		}
		return $this->mAction;
	}

	/** Get the oldid parameter */
	function getOldId() {
		if ( !isset( $this->mRequestValuesLoaded ) ) {
			$this->loadRequestValues();
		}
		return $this->mOldId;
	}

	/** Get the diff parameter */
	function getDiff() {
		if ( !isset( $this->mRequestValuesLoaded ) ) {
			$this->loadRequestValues();
		}
		return $this->mDiff;
	}

	function getSection() {
		if ( !isset( $this->mRequestValuesLoaded ) ) {
			$this->loadRequestValues();
		}
		return $this->mSection;
	}

	function getSearch() {
		if ( !isset( $this->mRequestValuesLoaded ) ) {
			$this->loadRequestValues();
		}
		return $this->mSearch;
	}

	/** Make a special page URL of the form [[Special:Somepage/{title_urlform}]] */
	function makeSpecialParamUrl( $name, $query = '', $param = '{title_urlform}' ) {
		// Abuse makeTitle's lax validity checking to slip a control character into the URL
		$title = Title::makeTitle( NS_SPECIAL, "$name/\x1a" );
		$url = cbt_escape( $title->getLocalURL( $query ) );
		// Now replace it with the parameter
		return str_replace( '%1A', $param, $url );
	}

	function getSubjectPage() {
		if ( !isset( $this->mSubjectPage ) ) {
			$this->mSubjectPage = $this->mTitle->getSubjectPage();
		}
		return $this->mSubjectPage;
	}

	function getTalkPage() {
		if ( !isset( $this->mTalkPage ) ) {
			$this->mTalkPage = $this->mTitle->getTalkPage();
		}
		return $this->mTalkPage;
	}
}

