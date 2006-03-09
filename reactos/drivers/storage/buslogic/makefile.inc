clean:
	del BT958dt.h BT958.bmf

BT958dt.h: BT958.bmf
	wmimofck -hBT958dt.h BT958.bmf
