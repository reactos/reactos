<?php

function wfSpecialBlankpage() {
	global $wgOut;
	$wgOut->addHTML(wfMsg('intentionallyblankpage'));
}
