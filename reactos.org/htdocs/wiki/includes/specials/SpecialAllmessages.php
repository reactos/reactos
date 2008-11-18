<?php
/**
 * Use this special page to get a list of the MediaWiki system messages.
 * @file
 * @ingroup SpecialPage
 */

/**
 * Constructor.
 */
function wfSpecialAllmessages() {
	global $wgOut, $wgRequest, $wgMessageCache, $wgTitle;
	global $wgUseDatabaseMessages;

	# The page isn't much use if the MediaWiki namespace is not being used
	if( !$wgUseDatabaseMessages ) {
		$wgOut->addWikiMsg( 'allmessagesnotsupportedDB' );
		return;
	}

	wfProfileIn( __METHOD__ );

	wfProfileIn( __METHOD__ . '-setup' );
	$ot = $wgRequest->getText( 'ot' );

	$navText = wfMsg( 'allmessagestext' );

	# Make sure all extension messages are available

	$wgMessageCache->loadAllMessages();

	$sortedArray = array_merge( Language::getMessagesFor( 'en' ), $wgMessageCache->getExtensionMessagesFor( 'en' ) );
	ksort( $sortedArray );
	$messages = array();

	foreach ( $sortedArray as $key => $value ) {
		$messages[$key]['enmsg'] = $value;
		$messages[$key]['statmsg'] = wfMsgReal( $key, array(), false, false, false ); // wfMsgNoDbNoTrans doesn't exist
		$messages[$key]['msg'] = wfMsgNoTrans( $key );
	}

	wfProfileOut( __METHOD__ . '-setup' );

	wfProfileIn( __METHOD__ . '-output' );
	$wgOut->addScriptFile( 'allmessages.js' );
	if ( $ot == 'php' ) {
		$navText .= wfAllMessagesMakePhp( $messages );
		$wgOut->addHTML( 'PHP | <a href="' . $wgTitle->escapeLocalUrl( 'ot=html' ) . '">HTML</a> | ' .
			'<a href="' . $wgTitle->escapeLocalUrl( 'ot=xml' ) . '">XML</a>' .
			'<pre>' . htmlspecialchars( $navText ) . '</pre>' );
	} else if ( $ot == 'xml' ) {
		$wgOut->disable();
		header( 'Content-type: text/xml' );
		echo wfAllMessagesMakeXml( $messages );
	} else {
		$wgOut->addHTML( '<a href="' . $wgTitle->escapeLocalUrl( 'ot=php' ) . '">PHP</a> | ' .
			'HTML |  <a href="' . $wgTitle->escapeLocalUrl( 'ot=xml' ) . '">XML</a>' );
		$wgOut->addWikiText( $navText );
		$wgOut->addHTML( wfAllMessagesMakeHTMLText( $messages ) );
	}
	wfProfileOut( __METHOD__ . '-output' );

	wfProfileOut( __METHOD__ );
}

function wfAllMessagesMakeXml( $messages ) {
	global $wgLang;
	$lang = $wgLang->getCode();
	$txt = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n";
	$txt .= "<messages lang=\"$lang\">\n";
	foreach( $messages as $key => $m ) {
		$txt .= "\t" . Xml::element( 'message', array( 'name' => $key ), $m['msg'] ) . "\n";
	}
	$txt .= "</messages>";
	return $txt;
}

/**
 * Create the messages array, formatted in PHP to copy to language files.
 * @param $messages Messages array.
 * @return The PHP messages array.
 * @todo Make suitable for language files.
 */
function wfAllMessagesMakePhp( $messages ) {
	global $wgLang;
	$txt = "\n\n\$messages = array(\n";
	foreach( $messages as $key => $m ) {
		if( $wgLang->getCode() != 'en' && $m['msg'] == $m['enmsg'] ) {
			continue;
		} else if ( wfEmptyMsg( $key, $m['msg'] ) ) {
			$m['msg'] = '';
			$comment = ' #empty';
		} else {
			$comment = '';
		}
		$txt .= "'$key' => '" . preg_replace( '/(?<!\\\\)\'/', "\'", $m['msg']) . "',$comment\n";
	}
	$txt .= ');';
	return $txt;
}

/**
 * Create a list of messages, formatted in HTML as a list of messages and values and showing differences between the default language file message and the message in MediaWiki: namespace.
 * @param $messages Messages array.
 * @return The HTML list of messages.
 */
function wfAllMessagesMakeHTMLText( $messages ) {
	global $wgLang, $wgContLang, $wgUser;
	wfProfileIn( __METHOD__ );

	$sk = $wgUser->getSkin();
	$talk = wfMsg( 'talkpagelinktext' );

	$input = Xml::element( 'input', array(
		'type'    => 'text',
		'id'      => 'allmessagesinput',
		'onkeyup' => 'allmessagesfilter()'
	), '' );
	$checkbox = Xml::element( 'input', array(
		'type'    => 'button',
		'value'   => wfMsgHtml( 'allmessagesmodified' ),
		'id'      => 'allmessagescheckbox',
		'onclick' => 'allmessagesmodified()'
	), '' );

	$txt = '<span id="allmessagesfilter" style="display: none;">' . wfMsgHtml( 'allmessagesfilter' ) . " {$input}{$checkbox} " . '</span>';

	$txt .= '
<table border="1" cellspacing="0" width="100%" id="allmessagestable">
	<tr>
		<th rowspan="2">' . wfMsgHtml( 'allmessagesname' ) . '</th>
		<th>' . wfMsgHtml( 'allmessagesdefault' ) . '</th>
	</tr>
	<tr>
		<th>' . wfMsgHtml( 'allmessagescurrent' ) . '</th>
	</tr>';

	wfProfileIn( __METHOD__ . "-check" );

	# This is a nasty hack to avoid doing independent existence checks
	# without sending the links and table through the slow wiki parser.
	$pageExists = array(
		NS_MEDIAWIKI => array(),
		NS_MEDIAWIKI_TALK => array()
	);
	$dbr = wfGetDB( DB_SLAVE );
	$page = $dbr->tableName( 'page' );
	$sql = "SELECT page_namespace,page_title FROM $page WHERE page_namespace IN (" . NS_MEDIAWIKI . ", " . NS_MEDIAWIKI_TALK . ")";
	$res = $dbr->query( $sql );
	while( $s = $dbr->fetchObject( $res ) ) {
		$pageExists[$s->page_namespace][$s->page_title] = true;
	}
	$dbr->freeResult( $res );
	wfProfileOut( __METHOD__ . "-check" );

	wfProfileIn( __METHOD__ . "-output" );

	$i = 0;

	foreach( $messages as $key => $m ) {
		$title = $wgLang->ucfirst( $key );
		if( $wgLang->getCode() != $wgContLang->getCode() ) {
			$title .= '/' . $wgLang->getCode();
		}

		$titleObj =& Title::makeTitle( NS_MEDIAWIKI, $title );
		$talkPage =& Title::makeTitle( NS_MEDIAWIKI_TALK, $title );

		$changed = ( $m['statmsg'] != $m['msg'] );
		$message = htmlspecialchars( $m['statmsg'] );
		$mw = htmlspecialchars( $m['msg'] );

		if( isset( $pageExists[NS_MEDIAWIKI][$title] ) ) {
			$pageLink = $sk->makeKnownLinkObj( $titleObj, "<span id=\"sp-allmessages-i-$i\">" .  htmlspecialchars( $key ) . '</span>' );
		} else {
			$pageLink = $sk->makeBrokenLinkObj( $titleObj, "<span id=\"sp-allmessages-i-$i\">" .  htmlspecialchars( $key ) . '</span>' );
		}
		if( isset( $pageExists[NS_MEDIAWIKI_TALK][$title] ) ) {
			$talkLink = $sk->makeKnownLinkObj( $talkPage, htmlspecialchars( $talk ) );
		} else {
			$talkLink = $sk->makeBrokenLinkObj( $talkPage, htmlspecialchars( $talk ) );
		}

		$anchor = 'msg_' . htmlspecialchars( strtolower( $title ) );
		$anchor = "<a id=\"$anchor\" name=\"$anchor\"></a>";

		if( $changed ) {
			$txt .= "
	<tr class=\"orig\" id=\"sp-allmessages-r1-$i\">
		<td rowspan=\"2\">
			$anchor$pageLink<br />$talkLink
		</td><td>
$message
		</td>
	</tr><tr class=\"new\" id=\"sp-allmessages-r2-$i\">
		<td>
$mw
		</td>
	</tr>";
		} else {
			$txt .= "
	<tr class=\"def\" id=\"sp-allmessages-r1-$i\">
		<td>
			$anchor$pageLink<br />$talkLink
		</td><td>
$mw
		</td>
	</tr>";
		}
		$i++;
	}
	$txt .= '</table>';
	wfProfileOut( __METHOD__ . '-output' );

	wfProfileOut( __METHOD__ );
	return $txt;
}
