// I18N constants

// LANG: "en", ENCODING: UTF-8 | ISO-8859-1
// Author: Mihai Bazon, http://dynarch.com/mishoo

// FOR TRANSLATORS:
//
//   1. PLEASE PUT YOUR CONTACT INFO IN THE ABOVE LINE
//      (at least a valid email address)
//
//   2. PLEASE TRY TO USE UTF-8 FOR ENCODING;
//      (if this is not possible, please include a comment
//       that states what encoding is necessary.)

HTMLArea.I18N = {

	// the following should be the filename without .js extension
	// it will be used for automatically load plugin language.
	lang: "en",

	tooltips: {
		bold:           "Bold",
		italic:         "Italic",
		underline:      "Underline",
		strikethrough:  "Strikethrough",
		subscript:      "Subscript",
		superscript:    "Superscript",
		justifyleft:    "Justify Left",
		justifycenter:  "Justify Center",
		justifyright:   "Justify Right",
		justifyfull:    "Justify Full",
		orderedlist:    "Ordered List",
		unorderedlist:  "Bulleted List",
		outdent:        "Decrease Indent",
		indent:         "Increase Indent",
		forecolor:      "Font Color",
		hilitecolor:    "Background Color",
		horizontalrule: "Horizontal Rule",
		createlink:     "Insert Web Link",
		insertimage:    "Insert/Modify Image",
		inserttable:    "Insert Table",
		htmlmode:       "Toggle HTML Source",
		popupeditor:    "Enlarge Editor",
		about:          "About this editor",
		showhelp:       "Help using editor",
		textindicator:  "Current style",
		undo:           "Undoes your last action",
		redo:           "Redoes your last action",
		cut:            "Cut selection",
		copy:           "Copy selection",
		paste:          "Paste from clipboard",
		lefttoright:    "Direction left to right",
		righttoleft:    "Direction right to left",
		removeformat:   "Remove formatting",
		print:          "Print document",
		killword:       "Clear MSOffice tags"
	},

	buttons: {
		"ok":           "OK",
		"cancel":       "Cancel"
	},

	msg: {
		"Path":         "Path",
		"TEXT_MODE":    "You are in TEXT MODE.  Use the [<>] button to switch back to WYSIWYG.",

		"IE-sucks-full-screen" :
		// translate here
		"The full screen mode is known to cause problems with Internet Explorer, " +
		"due to browser bugs that we weren't able to workaround.  You might experience garbage " +
		"display, lack of editor functions and/or random browser crashes.  If your system is Windows 9x " +
		"it's very likely that you'll get a 'General Protection Fault' and need to reboot.\n\n" +
		"You have been warned.  Please press OK if you still want to try the full screen editor.",

		"Moz-Clipboard" :
		"Unprivileged scripts cannot access Cut/Copy/Paste programatically " +
		"for security reasons.  Click OK to see a technical note at mozilla.org " +
		"which shows you how to allow a script to access the clipboard."
	},

	dialogs: {
		// Common
		"OK"                                                : "OK",
		"Cancel"                                            : "Cancel",

		"Alignment:"                                        : "Alignment:",
		"Not set"                                           : "Not set",
		"Left"                                              : "Left",
		"Right"                                             : "Right",
		"Texttop"                                           : "Texttop",
		"Absmiddle"                                         : "Absmiddle",
		"Baseline"                                          : "Baseline",
		"Absbottom"                                         : "Absbottom",
		"Bottom"                                            : "Bottom",
		"Middle"                                            : "Middle",
		"Top"                                               : "Top",

		"Layout"                                            : "Layout",
		"Spacing"                                           : "Spacing",
		"Horizontal:"                                       : "Horizontal:",
		"Horizontal padding"                                : "Horizontal padding",
		"Vertical:"                                         : "Vertical:",
		"Vertical padding"                                  : "Vertical padding",
		"Border thickness:"                                 : "Border thickness:",
		"Leave empty for no border"                         : "Leave empty for no border",

		// Insert Link
		"Insert/Modify Link"                                : "Insert/Modify Link",
		"None (use implicit)"                               : "None (use implicit)",
		"New window (_blank)"                               : "New window (_blank)",
		"Same frame (_self)"                                : "Same frame (_self)",
		"Top frame (_top)"                                  : "Top frame (_top)",
		"Other"                                             : "Other",
		"Target:"                                           : "Target:",
		"Title (tooltip):"                                  : "Title (tooltip):",
		"URL:"                                              : "URL:",
		"You must enter the URL where this link points to"  : "You must enter the URL where this link points to",
		// Insert Table
		"Insert Table"                                      : "Insert Table",
		"Rows:"                                             : "Rows:",
		"Number of rows"                                    : "Number of rows",
		"Cols:"                                             : "Cols:",
		"Number of columns"                                 : "Number of columns",
		"Width:"                                            : "Width:",
		"Width of the table"                                : "Width of the table",
		"Percent"                                           : "Percent",
		"Pixels"                                            : "Pixels",
		"Em"                                                : "Em",
		"Width unit"                                        : "Width unit",
		"Positioning of this table"                         : "Positioning of this table",
		"Cell spacing:"                                     : "Cell spacing:",
		"Space between adjacent cells"                      : "Space between adjacent cells",
		"Cell padding:"                                     : "Cell padding:",
		"Space between content and border in cell"          : "Space between content and border in cell",
		// Insert Image
		"Insert Image"                                      : "Insert Image",
		"Image URL:"                                        : "Image URL:",
		"Enter the image URL here"                          : "Enter the image URL here",
		"Preview"                                           : "Preview",
		"Preview the image in a new window"                 : "Preview the image in a new window",
		"Alternate text:"                                   : "Alternate text:",
		"For browsers that don't support images"            : "For browsers that don't support images",
		"Positioning of this image"                         : "Positioning of this image",
		"Image Preview:"                                    : "Image Preview:"
	}
};
