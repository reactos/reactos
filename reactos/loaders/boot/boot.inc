;	To save space, functions that are just called once are 
;	implemented as macros instead. Four bytes are saved by
;	avoiding the call / ret instructions.


;	FINDFILE: Searches for the file in the root directory.
;
;	Returns:
;
;	If file not found:      CF set
;	
;	If file found:          CF clear
;	                        AX = first cluster of file


%macro FINDFILE 0      
		; First, read the whole root directory
		; into the temporary buffer.

                mov     ax, word  root_dir_start
                mov     dx, word  root_dir_start_hi
		mov     di, nRootDir
		xor     bx, bx
		mov     es, tempbuf
		call    readDisk
		jc      ffDone

		xor     di, di

next_entry:     mov     cx, 11
                mov     si, filename+7c00h
		push    di
		repe    cmpsb
		pop     di
                mov     ax, [es:di+1ah]    ; get cluster number from directory entry
		clc
		je      ffDone

		add     di, 20h             ; go to next directory entry
                cmp     byte  [es:di], 0     ; if the first byte of the name is 0,
		jnz     next_entry              ; there is no more files in the directory

		stc
ffDone:
%endmacro             

;	GETDRIVEPARMS:  Calculate start of some disk areas.

%macro GETDRIVEPARMS 0
                mov     si, word  nHidden
                mov     di, word  nHidden_hi
                add     si, word  resSectors
		adc     di, 0                   ; DI:SI = first FAT sector

                mov     word  fat_start, si
                mov     word  fat_start_hi, di

		mov     al, nFats
		xor     ah, ah
                mul     word  sectPerFat     ; DX:AX = total number of FAT sectors

		add     si, ax
		adc     di, dx                  ; DI:SI = first root directory sector
                mov     word  root_dir_start, si
                mov     word  root_dir_start_hi, di

		; Calculate how many sectors the root directory occupies.
		mov     bx, bytesPerSector
		mov     cl, 5                   ; divide BX by 32
		shr     bx, cl                  ; BX = directory entries per sector

		mov     ax, nRootDir
		xor     dx, dx
		div     bx                      

		mov     nRootDir, ax            ; AX = sectors per root directory

		add     si, ax                  
		adc     di, 0                   ; DI:SI = first data sector

		mov     data_start, si
                mov     data_start_hi, di
%endmacro

;	GETFATCHAIN:    
;
;	Reads the FAT chain and stores it in a temporary buffer in the first
;	64 kb.  The FAT chain is stored an array of 16-bit cluster numbers,
;	ending with 0.
;	
;	The file must fit in conventional memory, so it can't be larger than
;	640 kb. The sector size must be at least 512 bytes, so the FAT chain
;	can't be larger than around 3 kb.
;
;	Call with:      AX = first cluster in chain
;
;	Returns:        CF clear on success, set on error

%macro GETFATCHAIN    0 
		push    ax                      ; store first cluster number

		; Load the complete FAT into memory. The FAT can't be larger
		; than 128 kb, so it should fit in the temporary buffer.

		mov     es, tempbuf
		xor     bx, bx
		mov     di, sectPerFat
                mov     ax, word  fat_start
                mov     dx, word  fat_start_hi
		call    readDisk
		pop     ax                      ; restore first cluster number
		jc      boot_error

		; Set ES:DI to the temporary storage for the FAT chain.
		push    ds
		push    es
		pop     ds
		pop     es
		mov     di, FATBUF

next_clust:     stosw                           ; store cluster number
		mov     si, ax                  ; SI = cluster number
                cmp     byte  extBoot, 29h
		jne     fat_12
                cmp     byte  [bp+filesys+4], '6'  ; check for FAT-16 system
		je      fat_16

		; This is a FAT-12 disk.

fat_12:         add     si, si          ; multiply cluster number by 3...
		add     si, ax
		shr     si, 1           ; ...and divide by 2
		lodsw

		; If the cluster number was even, the cluster value is now in
		; bits 0-11 of AX. If the cluster number was odd, the cluster
		; value is in bits 4-15, and must be shifted right 4 bits. If
		; the number was odd, CF was set in the last shift instruction.

		jnc     fat_even
		mov     cl, 4      
		shr     ax, cl          ; shift the cluster number 
		
fat_even:       and     ah, 0fh         ; mask off the highest 4 bits
		cmp     ax, 0fffh       ; check for EOF
		jmp     short next_test

		; This is a FAT-16 disk. The maximal size of a 16-bit FAT
		; is 128 kb, so it may not fit within a single 64 kb segment.
 
fat_16:         mov     dx, tempbuf
		add     si, si          ; multiply cluster number by two
		jnc     first_half      ; if overflow...
		add     dh, 10h         ; ...add 64 kb to segment value

first_half:     mov     ds, dx          ; DS:SI = pointer to next cluster
		lodsw                   ; AX = next cluster

		cmp     ax, 0fff8h      ; >= FFF8 = 16-bit EOF
next_test:      jb      next_clust      ; continue if not EOF

finished:       ; Mark end of FAT chain with 0, so we have a single
		; EOF marker for both FAT-12 and FAT-16 systems.

                xor      ax, ax
		stosw
fatError:
%endmacro


;	loadFile: Loads the file into memory, one cluster at a time.

%macro LOADFILE  0
		mov     es, tempbuf     ; set ES:BX to load address
		xor     bx, bx

		mov     si, FATBUF      ; set DS:SI to the FAT chain
		push    cs      
		pop     ds              

next_cluster:   lodsw                           ; AX = next cluster to read
		or      ax, ax                  ; if EOF...
		je      boot_success            ; ...boot was successful

		dec     ax                      ; cluster numbers start with 2
		dec     ax

                mov     di, word  sectPerCluster
		and     di, 0ffh                ; DI = sectors per cluster
		mul     di                       
		add     ax, data_start
                adc     dx, data_start_hi        ; DX:AX = first sector to read
		call    readDisk
		jnc     next_cluster            

%endmacro                
