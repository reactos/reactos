// I18N constants

// LANG: "ch", ENCODING: UTF-8
// Samuel Stone, http://stonemicro.com/

HTMLArea.I18N = {

	// the following should be the filename without .js extension
	// it will be used for automatically load plugin language.
	lang: "ch",

	tooltips: {
		bold:           "粗體",
		italic:         "斜體",
		underline:      "底線",
		strikethrough:  "刪線",
		subscript:      "下標",
		superscript:    "上標",
		justifyleft:    "靠左",
		justifycenter:  "居中",
		justifyright:   "靠右",
		justifyfull:    "整齊",
		orderedlist:    "順序清單",
		unorderedlist:  "無序清單",
		outdent:        "伸排",
		indent:         "縮排",
		forecolor:      "文字顏色",
		backcolor:      "背景顏色",
		horizontalrule: "水平線",
		createlink:     "插入連結",
		insertimage:    "插入圖像",
		inserttable:    "插入表格",
		htmlmode:       "切換HTML原始碼",
		popupeditor:    "伸出編輯系統",
		about:          "關於 HTMLArea",
		help:           "說明",
		textindicator:  "字體例子",
		undo:           "回原",
		redo:           "重来",
		cut:            "剪制选项",
		copy:           "复制选项",
		paste:          "贴上",
		lefttoright:    "从左到右",
		righttoleft:    "从右到左"
	},

	buttons: {
		"ok":           "好",
		"cancel":       "取消"
	},

	msg: {
		"Path":         "途徑",
		"TEXT_MODE":    "你在用純字編輯方式.  用 [<>] 按鈕轉回 所見即所得 編輯方式.",

		"IE-sucks-full-screen" :
		// translate here
		"整頁式在Internet Explorer 上常出問題, " +
		"因為這是 Internet Explorer 的無名問題，我們無法解決。" +
		"你可能看見一些垃圾，或遇到其他問題。" +
		"我們已警告了你. 如果要轉到 正頁式 請按 好.",

		"Moz-Clipboard" :
		"Unprivileged scripts cannot access Cut/Copy/Paste programatically " +
		"for security reasons.  Click OK to see a technical note at mozilla.org " +
		"which shows you how to allow a script to access the clipboard."
	},

	dialogs: {
		"Cancel"                                            : "取消",
		"Insert/Modify Link"                                : "插入/改寫連結",
		"New window (_blank)"                               : "新窗户(_blank)",
		"None (use implicit)"                               : "無(use implicit)",
		"OK"                                                : "好",
		"Other"                                             : "其他",
		"Same frame (_self)"                                : "本匡 (_self)",
		"Target:"                                           : "目標匡:",
		"Title (tooltip):"                                  : "主題 (tooltip):",
		"Top frame (_top)"                                  : "上匡 (_top)",
		"URL:"                                              : "網址:",
		"You must enter the URL where this link points to"  : "你必須輸入你要连结的網址"
	}
};
