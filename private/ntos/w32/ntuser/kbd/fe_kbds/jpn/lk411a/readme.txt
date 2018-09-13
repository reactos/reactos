
[[ DEC LK411 keyboard layout driver ]]


1. 概要

   LKキーボードは、DEC製のOpenVMSやUNIXシステムおよびVT端末などで広く用いられ
   ているDECの標準キーボードです。
   LK411はLK400シリーズの中でPC用のインターフェース(PS/2仕様)を持つキーボード
   で、現在 以下の２種類があります。

    LK411-AJ : ANSI配列＋カナ(JIS類似配列)、日本語変換キー付き、111キー
    LK411-JJ : JIS 配列＋カナ(JIS配列)、    日本語変換キー付き、112キー


2. キーのレジェンド

   LK411は OpenVMSやUNIX、VT端末用のレイアウトを持つため、キーのレジェンド
   (キートップに書かれている文字)と実際の動作が、以下のキーで異なります。
   たとえば、Windows NTにログオンするときには Ctrl+Alt+Del キーの代わりに、
   Ctrl(またはComp) + Alt + Remove(または KP.) を押します。

   [ LK411-AJ ]

	レジェンド	  実際の動作
	======================================
	` ~(ESC)	: ESC
	< >		: ` ~
 	Comp		: 右Ctrl
	Find		: Home
	Select		: End
	Insert Here 	: Ins
	Remove		: Del
	Prev		: Page Up
	Next		: Page Down
	PF1		: Num Lock
	PF2		: KP /
	PF3		: KP *
	PF4		: KP -
	KP -		: KP +
	KP ,		: KP +
	F18		: Print Screen
	F19		: Scroll Lock
	F20		: Pause

   [ LK411-JJ ]

	レジェンド	  実際の動作
	======================================
 	Comp		: 右Ctrl
	Find		: Home
	Select		: End
	Insert Here 	: Ins
	Remove		: Del
	Prev		: Page Up
	Next		: Page Down
	PF1		: Num Lock
	PF2		: KP /
	PF3		: KP *
	PF4		: KP -
	KP -		: KP +
	KP ,		: KP +
	F18		: Print Screen
	F19		: Scroll Lock
	F20		: Pause


3. IMEでのキー操作

    以下は LK411キーボードで、Windows NT標準のIMEを使用する場合のキー操作です。

        IMEの機能                       LK411のキー操作
    =================================================================
    漢字（IMEの起動/停止）		Alt + 変換
					Alt + <> (LK411-AJのみ)
    変換				変換（次候補）
					スペース
    無変換				無変換
    カナ				カナ
					Shift + Ctrl + ひらがな
    英数入力				Shift + Lock
    ひらがな入力			ひらがな
    カタカナ入力			Shift + ひらがな
    半角／全角				Ctrl + ひらがな


4. ＬＥＤ

   LKキーボードには、Num Lock用のLEDはありません(ただし、PF1 キーは
   Num Lockとして動作します)。 代わりに、LK411には カナ入力モード用のLEDが
   あります。
   カナ入力モードのときは、LEDが点灯します。
   以下が、LK411でのキー操作と対応するLEDです。

      LK411のキー操作       LED
    ==================================
    Hold		: Scroll Lock
    Lock		: Caps Lock
    カナ		: カナ

以上
