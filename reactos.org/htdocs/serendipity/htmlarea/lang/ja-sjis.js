// I18N constants -- Japanese SHIFT JIS

// LANG: "ja-sjis", ENCODING: SHIFT_JIS
// Author: Mihai Bazon, http://dynarch.com/mishoo
// Translator:  
//   Manabu Onoue <tmocsys@tmocsys.com>, 2004.
//   Tadashi Jokagi <elf2000@users.sourceforge.net>, 2005.

HTMLArea.I18N = {

	// the following should be the filename without .js extension
	// it will be used for automatically load plugin language.
	lang: "ja-sjis",

	tooltips: {
		bold:           "太字",
		italic:         "斜体",
		underline:      "下線",
		strikethrough:  "打ち消し線",
		subscript:      "下付き添え字",
		superscript:    "上付き添え字",
		justifyleft:    "左寄せ",
		justifycenter:  "中央寄せ",
		justifyright:   "右寄せ",
		justifyfull:    "均等割付",
		orderedlist:    "番号付き箇条書き",
		unorderedlist:  "記号付き箇条書き",
		outdent:        "インデント解除",
		indent:         "インデント設定",
		forecolor:      "文字色",
		hilitecolor:      "背景色",
		horizontalrule: "水平線",
		createlink:     "リンク作成",
		insertimage:    "画像挿入",
		inserttable:    "テーブル挿入",
		htmlmode:       "HTML表示切替",
		popupeditor:    "エディタ拡大",
		about:          "バージョン情報",
		showhelp:       "Help using editor",
		textindicator:  "現在のスタイル",
		undo:           "最後の操作を取り消し",
		redo:           "最後の動作をやり直し",
		cut:            "選択を切り取り",
		copy:           "選択をコピー",
		paste:          "クリップボードから貼り付け",
		lefttoright:    "左から右の方向",
		righttoleft:    "右から左の方向",
		removeformat:   "書式を取り除く",
		print:          "ドキュメントを印刷",
		killword:       "MSOffice タグを取り除く"
	},

	buttons: {
		"ok":           "OK",
		"cancel":       "取り消し"
	},

	msg: {
		"Path":         "パス",
		"TEXT_MODE":    "テキストモードです。[<>] ボタンを使って WYSIWYG に戻ります。",

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
		"Cancel"                                            : "取り消し",

		"Alignment:"                                        : "位置合わせ:",
		"Not set"                                           : "設定しない",
		"Left"                                              : "左",
		"Right"                                             : "右",
		"Texttop"                                           : "Texttop",
		"Absmiddle"                                         : "Absmiddle",
		"Baseline"                                          : "ベースライン",
		"Absbottom"                                         : "Absbottom",
		"Bottom"                                            : "下",
		"Middle"                                            : "中央",
		"Top"                                               : "上",

		"Layout"                                            : "レイアウト",
		"Spacing"                                           : "間隔",
		"Horizontal:"                                       : "水平:",
		"Horizontal padding"                                : "水平の隙間",
		"Vertical:"                                         : "垂直:",
		"Vertical padding"                                  : "垂直の隙間",
		"Border thickness:"                                 : "境界線の太さ:",
		"Leave empty for no border"                         : "境界線をなくすには空にします",

		// Insert Link
		"Insert/Modify Link"                                : "Insert/Modify Link",
		"None (use implicit)"                               : "なし (use implicit)",
		"New window (_blank)"                               : "新規ウィンドウ (_blank)",
		"Same frame (_self)"                                : "同じフレーム (_self)",
		"Top frame (_top)"                                  : "上のフレーム (_top)",
		"Other"                                             : "その他",
		"Target:"                                           : "対象:",
		"Title (tooltip):"                                  : "題名 (ツールチップ):",
		"URL:"                                              : "URL:",
		"You must enter the URL where this link points to"  : "You must enter the URL where this link points to",
		// Insert Table
		"Insert Table"                                      : "テーブルの挿入",
		"Rows:"                                             : "行:",
		"Number of rows"                                    : "行数",
		"Cols:"                                             : "列:",
		"Number of columns"                                 : "列数",
		"Width:"                                            : "幅:",
		"Width of the table"                                : "テーブルの幅",
		"Percent"                                           : "パーセント",
		"Pixels"                                            : "ピクセル",
		"Em"                                                : "Em",
		"Width unit"                                        : "幅の単位",
		"Positioning of this table"                         : "このテーブルの位置",
		"Cell spacing:"                                     : "セルの間隔:",
		"Space between adjacent cells"                      : "隣接したセルの間隔",
		"Cell padding:"                                     : "セルの隙間:",
		"Space between content and border in cell"          : "セルの境界線と内容の間隔",
		// Insert Image
		"Insert Image"                                      : "画像の挿入",
		"Image URL:"                                        : "画像 URL:",
		"Enter the image URL here"                          : "ここに画像の URL を入力",
		"Preview"                                           : "プレビュー",
		"Preview the image in a new window"                 : "新規ウィンドウで画像をプレビュー",
		"Alternate text:"                                   : "代用テキスト:",
		"For browsers that don't support images"            : "画像をサポートしないブラウザーのために",
		"Positioning of this image"                         : "この画像の位置",
		"Image Preview:"                                    : "画像のプレビュー:"
	}
};
