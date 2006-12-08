clean:
	del $(O)\BT958dt.h $(O)\BT958.bmf

$(O)\BT958.bmf: BT958.mof
	mofcomp -B:$(O)\BT958.bmf BT958.mof

$(O)\BT958dt.h: $(O)\BT958.bmf
	wmimofck -h$(O)\BT958dt.h $(O)\BT958.bmf
