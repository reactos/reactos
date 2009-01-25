 /**
 * $Id: editor_plugin_src.js 42 2006-08-08 14:32:24Z spocke $
 *
 * @author Moxiecode - based on work by Andrew Tetlaw
 * @copyright Copyright © 2004-2008, Moxiecode Systems AB, All rights reserved.
 */

function init() {
	SXE.initElementDialog('acronym');
	if (SXE.currentAction == "update") {
		SXE.showRemoveButton();
	}
}

function insertAcronym() {
	SXE.insertElement('acronym');
	tinyMCEPopup.close();
}

function removeAcronym() {
	SXE.removeElement('acronym');
	tinyMCEPopup.close();
}

tinyMCEPopup.onInit.add(init);
