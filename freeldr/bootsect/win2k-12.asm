00008000  660FB64610        movzx eax,byte [bp+NumberOfFats]
00008005  668B4E24          mov ecx,[bp+SectorsPerFatBig]
00008009  66F7E1            mul ecx
0000800C  6603461C          add eax,[bp+HiddenSectors]
00008010  660FB7560E        movzx edx,word [bp+ReservedSectors]
00008015  6603C2            add eax,edx
00008018  668946FC          mov [bp-0x4],eax
0000801C  66C746F4FFFFFFFF  mov dword [bp-0xc],0xffffffff
00008024  668B462C          mov eax,[bp+RootDirStartCluster]
00008028  6683F802          cmp eax,byte +0x2
0000802C  0F82A6FC          jc near 0x7cd6
00008030  663DF8FFFF0F      cmp eax,0xffffff8
00008036  0F839CFC          jnc near 0x7cd6
0000803A  6650              push eax
0000803C  6683E802          sub eax,byte +0x2
00008040  660FB65E0D        movzx ebx,byte [bp+SectsPerCluster]
00008045  8BF3              mov si,bx
00008047  66F7E3            mul ebx
0000804A  660346FC          add eax,[bp-0x4]
0000804E  BB0082            mov bx,0x8200
00008051  8BFB              mov di,bx
00008053  B90100            mov cx,0x1
00008056  E887FC            call 0x7ce0
00008059  382D              cmp [di],ch
0000805B  741E              jz 0x807b
0000805D  B10B              mov cl,0xb
0000805F  56                push si
00008060  BE707D            mov si,0x7d70
00008063  F3A6              repe cmpsb
00008065  5E                pop si
00008066  741B              jz 0x8083
00008068  03F9              add di,cx
0000806A  83C715            add di,byte +0x15
0000806D  3BFB              cmp di,bx
0000806F  72E8              jc 0x8059
00008071  4E                dec si
00008072  75DA              jnz 0x804e
00008074  6658              pop eax
00008076  E86500            call 0x80de
00008079  72BF              jc 0x803a
0000807B  83C404            add sp,byte +0x4
0000807E  E955FC            jmp 0x7cd6
00008081  0020              add [bx+si],ah
00008083  83C404            add sp,byte +0x4
00008086  8B7509            mov si,[di+0x9]
00008089  8B7D0F            mov di,[di+0xf]
0000808C  8BC6              mov ax,si
0000808E  66C1E010          shl eax,0x10
00008092  8BC7              mov ax,di
00008094  6683F802          cmp eax,byte +0x2
00008098  0F823AFC          jc near 0x7cd6
0000809C  663DF8FFFF0F      cmp eax,0xffffff8
000080A2  0F8330FC          jnc near 0x7cd6
000080A6  6650              push eax
000080A8  6683E802          sub eax,byte +0x2
000080AC  660FB64E0D        movzx ecx,byte [bp+0xd]
000080B1  66F7E1            mul ecx
000080B4  660346FC          add eax,[bp-0x4]
000080B8  BB0000            mov bx,0x0
000080BB  06                push es
000080BC  8E068180          mov es,[0x8081]
000080C0  E81DFC            call 0x7ce0
000080C3  07                pop es
000080C4  6658              pop eax
000080C6  C1EB04            shr bx,0x4
000080C9  011E8180          add [0x8081],bx
000080CD  E80E00            call 0x80de
000080D0  0F830200          jnc near 0x80d6
000080D4  72D0              jc 0x80a6
000080D6  8A5640            mov dl,[bp+0x40]
000080D9  EA00000020        jmp 0x2000:0x0
000080DE  66C1E002          shl eax,0x2
000080E2  E81100            call 0x80f6
000080E5  26668B01          mov eax,[es:bx+di]
000080E9  6625FFFFFF0F      and eax,0xfffffff
000080EF  663DF8FFFF0F      cmp eax,0xffffff8
000080F5  C3                ret
000080F6  BF007E            mov di,0x7e00
000080F9  660FB74E0B        movzx ecx,word [bp+0xb]
000080FE  6633D2            xor edx,edx
00008101  66F7F1            div ecx
00008104  663B46F4          cmp eax,[bp-0xc]
00008108  743A              jz 0x8144
0000810A  668946F4          mov [bp-0xc],eax
0000810E  6603461C          add eax,[bp+0x1c]
00008112  660FB74E0E        movzx ecx,word [bp+0xe]
00008117  6603C1            add eax,ecx
0000811A  660FB75E28        movzx ebx,word [bp+0x28]
0000811F  83E30F            and bx,byte +0xf
00008122  7416              jz 0x813a
00008124  3A5E10            cmp bl,[bp+0x10]
00008127  0F83ABFB          jnc near 0x7cd6
0000812B  52                push dx
0000812C  668BC8            mov ecx,eax
0000812F  668B4624          mov eax,[bp+0x24]
00008133  66F7E3            mul ebx
00008136  6603C1            add eax,ecx
00008139  5A                pop dx
0000813A  52                push dx
0000813B  8BDF              mov bx,di
0000813D  B90100            mov cx,0x1
00008140  E89DFB            call 0x7ce0
00008143  5A                pop dx
00008144  8BDA              mov bx,dx
00008146  C3                ret
00008147  0000              add [bx+si],al
00008149  0000              add [bx+si],al
0000814B  0000              add [bx+si],al
0000814D  0000              add [bx+si],al
0000814F  0000              add [bx+si],al
00008151  0000              add [bx+si],al
00008153  0000              add [bx+si],al
00008155  0000              add [bx+si],al
00008157  0000              add [bx+si],al
00008159  0000              add [bx+si],al
0000815B  0000              add [bx+si],al
0000815D  0000              add [bx+si],al
0000815F  0000              add [bx+si],al
00008161  0000              add [bx+si],al
00008163  0000              add [bx+si],al
00008165  0000              add [bx+si],al
00008167  0000              add [bx+si],al
00008169  0000              add [bx+si],al
0000816B  0000              add [bx+si],al
0000816D  0000              add [bx+si],al
0000816F  0000              add [bx+si],al
00008171  0000              add [bx+si],al
00008173  0000              add [bx+si],al
00008175  0000              add [bx+si],al
00008177  0000              add [bx+si],al
00008179  0000              add [bx+si],al
0000817B  0000              add [bx+si],al
0000817D  0000              add [bx+si],al
0000817F  0000              add [bx+si],al
00008181  0000              add [bx+si],al
00008183  0000              add [bx+si],al
00008185  0000              add [bx+si],al
00008187  0000              add [bx+si],al
00008189  0000              add [bx+si],al
0000818B  0000              add [bx+si],al
0000818D  0000              add [bx+si],al
0000818F  0000              add [bx+si],al
00008191  0000              add [bx+si],al
00008193  0000              add [bx+si],al
00008195  0000              add [bx+si],al
00008197  0000              add [bx+si],al
00008199  0000              add [bx+si],al
0000819B  0000              add [bx+si],al
0000819D  0000              add [bx+si],al
0000819F  0000              add [bx+si],al
000081A1  0000              add [bx+si],al
000081A3  0000              add [bx+si],al
000081A5  0000              add [bx+si],al
000081A7  0000              add [bx+si],al
000081A9  0000              add [bx+si],al
000081AB  0000              add [bx+si],al
000081AD  0000              add [bx+si],al
000081AF  0000              add [bx+si],al
000081B1  0000              add [bx+si],al
000081B3  0000              add [bx+si],al
000081B5  0000              add [bx+si],al
000081B7  0000              add [bx+si],al
000081B9  0000              add [bx+si],al
000081BB  0000              add [bx+si],al
000081BD  0000              add [bx+si],al
000081BF  0000              add [bx+si],al
000081C1  0000              add [bx+si],al
000081C3  0000              add [bx+si],al
000081C5  0000              add [bx+si],al
000081C7  0000              add [bx+si],al
000081C9  0000              add [bx+si],al
000081CB  0000              add [bx+si],al
000081CD  0000              add [bx+si],al
000081CF  0000              add [bx+si],al
000081D1  0000              add [bx+si],al
000081D3  0000              add [bx+si],al
000081D5  0000              add [bx+si],al
000081D7  0000              add [bx+si],al
000081D9  0000              add [bx+si],al
000081DB  0000              add [bx+si],al
000081DD  0000              add [bx+si],al
000081DF  0000              add [bx+si],al
000081E1  0000              add [bx+si],al
000081E3  0000              add [bx+si],al
000081E5  0000              add [bx+si],al
000081E7  0000              add [bx+si],al
000081E9  0000              add [bx+si],al
000081EB  0000              add [bx+si],al
000081ED  0000              add [bx+si],al
000081EF  0000              add [bx+si],al
000081F1  0000              add [bx+si],al
000081F3  0000              add [bx+si],al
000081F5  0000              add [bx+si],al
000081F7  0000              add [bx+si],al
000081F9  0000              add [bx+si],al
000081FB  0000              add [bx+si],al
000081FD  0055AA            add [di-0x56],dl
