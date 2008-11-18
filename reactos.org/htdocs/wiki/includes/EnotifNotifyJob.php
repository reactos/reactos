<?php

/**
 * Job for email notification mails
 *
 * @ingroup JobQueue
 */
class EnotifNotifyJob extends Job {

	function __construct( $title, $params, $id = 0 ) {
		parent::__construct( 'enotifNotify', $title, $params, $id );
	}

	function run() {
		$enotif = new EmailNotification();
		// Get the user from ID (rename safe). Anons are 0, so defer to name.
		if( isset($this->params['editorID']) && $this->params['editorID'] ) {
			$editor = User::newFromId( $this->params['editorID'] );
		// B/C, only the name might be given.
		} else {
			$editor = User::newFromName( $this->params['editor'], false );
		}
		$enotif->actuallyNotifyOnPageChange(
			$editor,
			$this->title,
			$this->params['timestamp'],
			$this->params['summary'],
			$this->params['minorEdit'],
			$this->params['oldid']
		);
		return true;
	}

}
