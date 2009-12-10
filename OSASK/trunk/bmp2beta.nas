[INSTRSET "i386p"]
[FORMAT "WCOFF"]
[FILE "bmp2beta.nas"]

;[absolute 0]
BMP:
.fType:		equ 0	;resw 1
.fSize:		equ 2	;resd 1
			;resd 1
.fOffBits:	equ 10	;resd 1
.iSize:		equ 14	;resd 1
.iWidth:	equ 18	;resd 1
.iHeight:	equ 22	;resd 1
.iPlanes:	equ 26	;resw 1
.iBitCount:	equ 28	;resw 1
.iCompression:	equ 30	;resd 1
.iSizeImage:	equ 34	;resd 1
.iXPPM:		equ 38	;resd 1
.iYPPM:		equ 42	;resd 1
.iClrUsed:	equ 46	;resd 1
.iClrImportant:	equ 50	;resd 1
BMP_size:	equ 54

;[absolute 0]
BMPOS2:
.fType:		equ 0	;resw 1
.fSize:		equ 2	;resd 1
			;resd 1
.fOffBits:	equ 10	;resd 1
.iSize:		equ 14	;resd 1
.iWidth:	equ 18	;resw 1
.iHeight:	equ 20	;resw 1
.iPlanes:	equ 22	;resw 1
.iBitCount:	equ 24	;resw 1
BMPOS2_size:	equ 26

;[absolute 0]
CQuad:
.b:	equ 0	;resb 1
.g:	equ 1	;resb 1
.r:	equ 2	;resb 1
		;resb 1
CQuad_size: equ 4

;[absolute -4*21]
bmp2beta:	;equ -4*21
.ebufend:	equ -4*21
.ebufindex:	equ -4*21	;resd 1
.error:		equ -4*20	;resd 1
.transwidthbyte	equ -4*19	;resd 1
.remainder2:	equ -4*18	;resd 1
.destskip:	equ -4*17	;resd 1
.remainder:	equ -4*16	;resd 1
.output:	equ -4*15	;resd 1
.fillspace:	equ -4*14	;resd 1
.rightmargin:	equ -4*13	;resd 1
.leftmargin:	equ -4*12	;resd 1
.transwidth:	equ -4*11	;resd 1
.transendy_:	equ -4*10	;resd 1
.downmargin:	equ -4*9	;resd 1
.upmargin:	equ -4*8	;resd 1
.transheight:	equ -4*7	;resd 1
.bmpheight:	equ -4*6	;resd 1
.bmpwidthbyte:	equ -4*5	;resd 1
.bpp:		equ -4*4	;resd 1
.bmpwidth:	equ -4*3	;resd 1
.palette:	equ -4*2	;resd 1
.palettesize:	equ -4		;resd 1
.errn:		equ 0		;resd 1
.savedreg:	equ 4		;resd 5
.reteip:	equ 4*6		;resd 1
.mtop:		equ 4*7		;resd 1
.msize:		equ 4*8		;resd 1
.buf:		equ 4*9		;resd 1
.mode:		equ 4*10	;resd 1
.bufw:		equ 4*11	;resd 1
.bufh:		equ 4*12	;resd 1
.backcolor:	equ 4*13	;resd 1
.x:		equ 4*14	;resd 1
.y:		equ 4*15	;resd 1
.end:		equ 4*16

[section .text]
global fill.to16
global fill.to8
global clipping
[bits 32]
global _bmp2beta
_bmp2beta:
	pushfd
	xor eax, eax
	push edi
	inc eax
	push esi
	push ebp
	push ebx
	push eax
	mov ebp, esp

	mov esi, [ebp+bmp2beta.mtop]
	cmp dword[ebp +bmp2beta.msize], byte BMP_size
	jbe near .end

	cmp word[esi],'BM'
	je ..@17.ifnot
	  sub esi, byte -128
	  cmp word[esi], 'BM'
	  jne near .end
	..@17.ifnot:

	inc dword [ebp +bmp2beta.errn]
	inc dword [ebp +bmp2beta.errn]
	mov eax, [esi +BMP.iSize]
	cmp eax,byte 12
	jne ..@21.ifnot
	  lea edi, [esi+eax+14]
	  movzx eax, word[esi +BMP.iWidth]
	  movzx ecx, word[esi +BMP.iHeight]
	  movzx edx, word[esi +BMP.iBitCount]
	  push byte 3
	  inc dword [ebp +bmp2beta.errn]
	 jmp short ..@21.ifend
	..@21.ifnot:
	  lea edi, [esi+eax+14]
	  sub eax,byte 40
	  jne near .end
	  inc dword [ebp +bmp2beta.errn]
	  cmp eax, [esi+BMP.iCompression]
	  jne near .end
	  push byte 4
	  mov   eax, [esi +BMP.iWidth]
	  mov   ecx, [esi +BMP.iHeight]
	  movzx edx, word[esi +BMP.iBitCount]
	..@21.ifend:
	push edi
	push eax
	inc dword [ebp +bmp2beta.errn]
	add esi, [esi +BMP.fOffBits]
	dec edx
	jne ..@25.ifnot
	  add eax, byte 31
	  push dword .bpp1
	  and eax, byte -32
	  shr eax, 3
	  jmp short .clipping_load
	..@25.ifnot:
	sub edx, byte 4-1
	jne ..@28.ifnot
	  add eax, byte 7
	  push dword .bpp4
	  and eax, byte -8
	  shr eax, 1
	  jmp short .clipping_load
	..@28.ifnot:
	sub edx, byte 8-4
	jne ..@31.ifnot
	  add eax, byte 3
	  push dword .bpp8
	  and eax, byte -4
	  jmp short .clipping_load
	..@31.ifnot:
	sub edx, byte 16-8
	jne ..@34.ifnot
	  lea eax, [eax*2+1]
	  push dword .bpp16
	  and eax, byte -4
	  jmp short .clipping_load
	..@34.ifnot:
	sub edx, byte 24-16
	jne ..@37.ifnot
	  lea eax, [eax*3+3]
	  push dword .bpp24
	  and eax, byte -4
	  jmp short .clipping_load
	..@37.ifnot:
	sub edx, byte 32-24
	jne ..@40.ifnot
	  shl eax, 2
	  push dword .bpp32
	  jmp short .clipping_load
	..@40.ifnot:
.end:
	mov esp, ebp
	pop eax
	pop ebx
	pop ebp
	pop esi
	pop edi
	popfd
	ret

.clipping_load:
	push eax
	inc dword [ebp +bmp2beta.errn]

	push ecx
	mov ebx, [ebp+bmp2beta.bufh]

	mov edx, ecx
	sub edx, ebx
	sar edx, 1

	xor eax, eax

	call clipping
	test ecx, ecx
	jz .end
	push ecx
	push eax
	sub eax, edx
	pop edx
	add ecx, eax
	neg edx
	push eax
	add edx, [ebp+bmp2beta.bmpheight]
	sub ebx, ecx
	dec edx
	push ebx
	push edx


	mov ebx, [ebp+bmp2beta.bufw]
	mov ecx, [ebp+bmp2beta.bmpwidth]

	mov edx, ecx
	sub edx, ebx
	inc edx
	sar edx, 1

	xor eax, eax
	call clipping
	test ecx, ecx
	jz .end
	push ecx
	push eax
	sub eax, edx
	pop edx
	push eax
	add edx, ecx
	add ecx, eax
	sub ebx, ecx
	push ebx
	mov ebx, edx


	mov eax, [ebp+bmp2beta.bufw]
	mov ecx, [ebp+bmp2beta.leftmargin]
	mul dword[ebp+bmp2beta.upmargin]
	add ecx, eax
	mov edi, [ebp+bmp2beta.buf]
	mov eax, [ebp+bmp2beta.backcolor]

	cmp dword[ebp+bmp2beta.mode], byte 2
	jna ..@43.ifnot
	  push dword fill.to32
	  push dword output.to32
	..@43.ifnot:
	jnbe ..@46.ifnot
	jne ..@48.ifnot
	  push dword fill.to16
	  push dword output.to16
	..@48.ifnot:
	jnb ..@51.ifnot
	cmp dword[ebp+bmp2beta.mode],byte 1

	jne ..@54.ifnot

	    push dword fill.to8
	    push dword output.to8
	..@54.ifnot:
	jnb ..@57.ifnot
	    push dword fill.to4
	    push dword output.to4
	..@57.ifnot:
	  mov ah, al
	..@51.ifnot:
	 mov edx, eax
	 shl eax, 16
	 or  eax, edx
	..@46.ifnot:

	mov [ebp+bmp2beta.backcolor], eax
	call [ebp+bmp2beta.fillspace]

	mov eax, [ebp+bmp2beta.bmpwidthbyte]
	mul dword[ebp+bmp2beta.transendy_]
	add esi, eax

	jmp [ebp+bmp2beta.bpp]




.bpp24:

	lea esi, [esi+ebx*2]
	add esi, ebx
	call module_size_check

	lea edx, [esi-1]
	xor edx, byte 3
	and edx, byte 3
	push edx
	mov ecx, [ebp+bmp2beta.transwidth]
	lea eax, [edx*3]
	sub edx, ecx
	sub esi, eax
	mov eax, [ebp+bmp2beta.leftmargin]
	add eax, [ebp+bmp2beta.rightmargin]
	lea edx, [edx*3]
	push eax
	lea eax, [esi+edx]
	and eax, byte 3
	jnz ..@62.ifnot
	  push dword .ltop
	 jmp short ..@62.ifend


	..@62.ifnot:
	  cmp eax,byte 2
	  jnb ..@65.ifnot

	  push dword .1
	 jmp short ..@65.ifend


	..@65.ifnot:
	  jne ..@68.ifnot

	  push dword .2
	 jmp short ..@68.ifend


	..@68.ifnot:

	  push dword .3
	..@68.ifend:

	..@65.ifend:
	..@62.ifend:

	lea eax, [eax*3]
	add edx, eax
	push edx
	call ebufalloc

	mov ecx, [ebp+bmp2beta.transheight]
	jmp short .enter24

	..@72.do:
	  push ecx
	  mov ecx, [ebp+bmp2beta.transwidth]
	  mov eax, [ebp+bmp2beta.backcolor]
	  mov [ebp+bmp2beta.ebufindex], ecx
	  mov ecx, [ebp+bmp2beta.destskip]
	  call [ebp+bmp2beta.fillspace]
	  pop ecx
	  sub esi, [ebp+bmp2beta.bmpwidthbyte]
.enter24:
	  mov ebx, [ebp+bmp2beta.transwidthbyte]
	  jmp dword[ebp+bmp2beta.remainder2]
.3:	  mov edx, [esi+edx-8]
	  mov eax, [esi+edx-12]
	  shrd eax, edx, 24
	  and eax, 0x00ffffff
	  call dword[ebp+bmp2beta.output]

.2:	  mov eax, [esi+ebx-8]
	  mov edx, [esi+ebx-4]
	  shrd eax, edx, 16
	  and eax, 0x00ffffff
	  call dword[ebp+bmp2beta.output]

.1:	  mov eax, [esi+ebx-4]
	  shr eax, 8
	  call dword[ebp+bmp2beta.output]

.ltop:	  

	test ebx,ebx
	jz ..@75.ifnot

	..@77.do:
	      mov edx, [esi+ebx]
	       mov eax, 0x00ffffff
	      and eax, edx
	      call dword[ebp+bmp2beta.output]
	      mov eax, [esi+ebx+4]
	      shrd edx, eax, 24
	      xchg eax, edx
	      and eax, 0x00ffffff
	      call dword[ebp+bmp2beta.output]
	      mov eax, [esi+ebx+8]
	      shrd edx, eax, 16
	      xchg eax, edx
	      shr edx, 8
	      and eax, 0x00ffffff
	      call dword[ebp+bmp2beta.output]
	      mov eax, edx
	      and eax, 0x00ffffff
	      call dword[ebp+bmp2beta.output]
	      add ebx, byte 12
		jnz ..@77.do
	..@75.ifnot:
	  add ebx, [ebp+bmp2beta.remainder]
	jz ..@81.ifnot
	    or edx, byte -1
	..@83.do:
	      mov eax, [esi+edx]
	      shr eax, 8
	      call [ebp+bmp2beta.output]
	      add edx, byte 3
	      dec ebx
		jnz ..@83.do
	..@81.ifnot:
	  mov [ebp+bmp2beta.error], ebx
	  mov [ebp+bmp2beta.ebufindex], ebx
	  dec ecx
		jnz near ..@72.do
.bpp32:
.bpp16:
	jmp .finish

.bpp8:

	inc edx
	add esi, ebx
	and edx, ebx
	call module_size_check

	mov ecx, [ebp+bmp2beta.transwidth]
	push edx
	mov ebx, ecx
	sub esi, edx
	sub ebx, edx
	mov eax, [ebp+bmp2beta.leftmargin]
	shr ebx, 1
	add eax, [ebp+bmp2beta.rightmargin]
	push eax
	push eax
	push ebx
	call ebufalloc

	xor ecx, ecx
	mov ebx, [ebp+bmp2beta.palette]
	mov eax, [ebp+bmp2beta.palettesize]
	mov cl, 255
	mul ecx
	add ebx, eax
	..@88.do:
	  mov eax, [ebx]
	  sub ebx, [ebp+bmp2beta.palettesize]
	  and eax, 0x00ffffff
	  dec ecx
	  push eax
		jns ..@88.do
	xor ebx, ebx
	mov ecx, [ebp+bmp2beta.transheight]
	jmp short .enter8

	..@91.do:
	  push ecx
	  mov ecx, [ebp+bmp2beta.transwidth]
	  mov eax, [ebp+bmp2beta.backcolor]
	  mov [ebp+bmp2beta.ebufindex], ecx
	  mov ecx, [ebp+bmp2beta.destskip]
	  call [ebp+bmp2beta.fillspace]
	  pop ecx
	  sub esi, [ebp+bmp2beta.bmpwidthbyte]
.enter8:
	  sub ebx, [ebp+bmp2beta.transwidthbyte]
	jz ..@93.ifnot
	..@95.do:
	      xor eax, eax
	      xor edx, edx
	      mov al, [esi +ebx*2]
	      mov dl, [esi +ebx*2+1]
	      mov eax, [esp+eax*4]
	      mov edx, [esp+edx*4]
	      call [ebp+bmp2beta.output]
	      mov eax, edx
	      mov edx, [ebp+bmp2beta.output]
	      call edx
	      inc ebx
		jnz ..@95.do
	..@93.ifnot:
	cmp ebx,[ebp+bmp2beta.remainder]

	je ..@100.ifnot

	    mov bl, [esi]
	    mov eax, [esp+ebx*4]
	    xor ebx, ebx
	    and eax, 0x00ffffff
	    call [ebp+bmp2beta.output]
	..@100.ifnot:
	  mov [ebp+bmp2beta.error], ebx
	  mov [ebp+bmp2beta.ebufindex], ebx
	  dec ecx
		jnz ..@91.do

	jmp near .finish

.bpp4:
	inc edx
	and edx, ebx
	inc ebx
	shr ebx, 1
	add esi, ebx
	call module_size_check


	mov ebx, [ebp+bmp2beta.transwidth]
	push edx
	mov ecx, ebx
	sub esi, edx
	sub ebx, edx
	mov eax, [ebp+bmp2beta.leftmargin]
	shr ebx, 1
	sbb edx, edx
	add eax, [ebp+bmp2beta.rightmargin]
	push eax
	push edx
	push ebx
	mov ebx, [ebp+bmp2beta.palette]
	call ebufalloc


	push byte 15
	pop ecx
	mov eax, [ebp+bmp2beta.palettesize]
	mul ecx
	add ebx, eax

	..@104.do:
	  mov eax, [ebx]
	  sub ebx, [ebp+bmp2beta.palettesize]
	  and eax, 0x00ffffff
	  dec ecx
	  push eax
		jns ..@104.do
	xor ebx, ebx
	xor edx, edx
	mov ecx, [ebp+bmp2beta.transheight]
	jmp short .enter4

	..@107.do:
	  push ecx
	  mov ecx, [ebp+bmp2beta.transwidth]
	  mov eax, [ebp+bmp2beta.backcolor]
	  mov [ebp+bmp2beta.ebufindex], ecx
	  mov ecx, [ebp+bmp2beta.destskip]
	  call [ebp+bmp2beta.fillspace]
	  pop ecx
	  sub esi, [ebp+bmp2beta.bmpwidthbyte]
.enter4:
	  xor eax, eax
	  sub ebx, [ebp+bmp2beta.transwidthbyte]
	cmp eax,[ebp+bmp2beta.remainder2]

	je ..@110.ifnot

	    mov al, [esi+edx-1]
	    and eax, byte 0x0f
	    mov eax, [esp+eax*2]
	    call [ebp+bmp2beta.output]
	..@110.ifnot:
	test ebx,ebx

	jz ..@114.ifnot

	..@116.do:
	      xor eax, eax
	      mov dl, [esi+ebx]
	      mov al, dl
	      and edx, byte 0x0f
	      shr eax, 4
	      mov eax, [esp+eax*4]
	      mov edx, [esp+edx*4]
	      call [ebp+bmp2beta.output]
	      mov eax, edx
	      xor edx, edx
	      call [ebp+bmp2beta.output]
	      inc ebx
		jnz ..@116.do
	..@114.ifnot:
	cmp edx,[ebp+bmp2beta.remainder]

	je ..@121.ifnot

	    mov dl, [esi]
	    shr edx, 4
	    mov eax, [esp+edx*4]
	    xor edx, edx
	    call [ebp+bmp2beta.output]
	..@121.ifnot:
	  mov [ebp+bmp2beta.error], edx
	  dec ecx
		jnz ..@107.do

	jmp near .finish

.bpp1:

	mov dl, 7
	and edx, ebx
	shr ebx, 3
	add esi, ebx
	push esi
	test edx,edx

	jz ..@126.ifnot

	  inc esi
	..@126.ifnot:
	call module_size_check
	pop esi

	push edx
	mov eax, [ebp+bmp2beta.leftmargin]
	mov ebx, [ebp+bmp2beta.transwidth]
	add eax, [ebp+bmp2beta.rightmargin]
	mov ecx, ebx

	sub ebx, edx
	mov edx, ebx
	shr ebx, 3
	and edx, byte 7
	push eax
	push edx
	push ebx
	mov ebx, [ebp+bmp2beta.palette]
	call ebufalloc


	mov eax, [ebx]
	add ebx, [ebp+bmp2beta.palettesize]
	and eax, 0x00ffffff
	mov ebx, [ebx]
	and ebx, 0x00ffffff
	sub ebx, eax
	push ebx
	xor ebx, ebx
	xor edx, edx
	push eax
	jmp short .enter1

	..@129.do:
	  push ecx
	  mov ecx, [ebp+bmp2beta.transwidth]
	  mov eax, [ebp+bmp2beta.backcolor]
	  mov [ebp+bmp2beta.ebufindex], ecx
	  mov ecx, [ebp+bmp2beta.destskip]
	  call [ebp+bmp2beta.fillspace]
	  pop ecx
	  sub esi, [ebp+bmp2beta.bmpwidthbyte]
.enter1:
	  sub ebx, [ebp+bmp2beta.transwidthbyte]
	  mov ecx, [ebp+bmp2beta.remainder2]
	test ecx,ecx

	jz ..@132.ifnot

	    mov dl, [esi+ebx-1]
	    xor cl, 7
	    add dl, dl
	    shl dl, cl
	    xor cl, 7
	..@134.do:
	      add dl, dl
	      sbb eax, eax
	      and eax, [esp+4]
	      add eax, [esp]
	      call [ebp+bmp2beta.output]
	      dec ecx
		jnz ..@134.do
	..@132.ifnot:
	test ebx,ebx

	jz near ..@139.ifnot

	..@141.do:
	      mov dl,[esi+ebx]

	      add dl,dl

	      sbb eax, eax
	      add dl, dl
	      sbb ecx, ecx
	      and eax, [esp+4]
	      and ecx, [esp+4]
	      add eax, [esp]
	      add ecx, [esp]
	      call [ebp+bmp2beta.output]
	      mov eax, ecx
	      call [ebp+bmp2beta.output]
	      add dl,dl
	      sbb eax, eax
	      add dl, dl
	      sbb ecx, ecx
	      and eax, [esp+4]
	      and ecx, [esp+4]
	      add eax, [esp]
	      add ecx, [esp]
	      call [ebp+bmp2beta.output]
	      mov eax, ecx
	      call [ebp+bmp2beta.output]
	      add dl,dl
	      sbb eax, eax
	      add dl, dl
	      sbb ecx, ecx
	      and eax, [esp+4]
	      and ecx, [esp+4]
	      add eax, [esp]
	      add ecx, [esp]
	      call [ebp+bmp2beta.output]
	      mov eax, ecx
	      call [ebp+bmp2beta.output]
	      add dl,dl
	      sbb eax, eax
	      add dl, dl
	      sbb ecx, ecx
	      and eax, [esp+4]
	      and ecx, [esp+4]
	      add eax, [esp]
	      add ecx, [esp]
	      call [ebp+bmp2beta.output]
	      mov eax, ecx
	      call [ebp+bmp2beta.output]

	      inc ebx
		jnz near ..@141.do
	..@139.ifnot:
	  add ebx, [ebp+bmp2beta.remainder]
	jz ..@145.ifnot
	    mov dl, [esi]
	..@147.do:
	      add dl, dl
	      sbb eax, eax
	      and eax, [esp+4]
	      add eax, [esp]
	      call [ebp+bmp2beta.output]
	      dec ebx
		jnz ..@147.do
	..@145.ifnot:
	  mov [ebp+bmp2beta.error], ebx
	  dec dword[ebp+bmp2beta.transheight]
		jnz near ..@129.do

.finish:
	mov eax, [ebp+bmp2beta.bufw]
	mov ecx, [ebp+bmp2beta.rightmargin]
	mul dword[ebp+bmp2beta.downmargin]
	add ecx, eax
	mov eax, [ebp+bmp2beta.backcolor]
	call [ebp+bmp2beta.fillspace]

	mov [ebp], ecx
	jmp near .end


output:
.to32:
	mov [edi], eax
	add edi, byte 4
	ret
.to16:
	push ebx
	mov ebx, [ebp+bmp2beta.ebufindex]
	push edx
	push ecx
	call add_error

	mov edx, 0x00f800f8
	mov ecx, eax
	 and edx, eax
	shr edx, 3
	 and eax, 0x0000fc00
	shr eax, 5
	 inc edi
	or eax, edx
	 inc edi
	shr edx, 5
	or edx, eax
	 mov eax, ecx
	mov [edi-2], dx
	 mov edx, 0x00e000e0
	and edx, eax
	 and eax, 0x00e0c0e0
	add eax, edx
	 mov edx, 0x00f8fcf8
	shr eax, 6
	 and edx, ecx
	or eax, edx
	mov edx, 0x00ff0000
	and edx, eax
	jmp short .ret_error

.to8:
	push ebx
	mov ebx, [ebp+bmp2beta.ebufindex]
	push edx
	push ecx
	call add_error
	mov edx, 0x0000c0c0

	mov ecx, eax
	 and edx, eax
	shr edx, 2
	shr eax, 22
	or  eax, edx
	shr edx, 8+4-2
	 or al, 0xc0
	or  dl, al
	 mov eax, ecx
	mov [edi], dl
	 inc edi


	mov edx, eax
	 and eax, 0x00404040
	shr eax, 6
	 and edx, 0x00808080
	shr edx, 7
	 add eax, 0x007f7f7f
	add edx, 0x007f7f7f
	 xor eax, 0x007f7f7f
	xor edx, 0x007f7f7f
	 and eax, 0x00555555
 	and edx, 0x00aaaaaa
	or eax, edx
	mov edx, 0x00ff0000
	and edx, eax
.ret_error:
	sub ecx, edx
	sub ch, ah
	 sub cl, al
	mov eax, 0x00808080
	and eax, ecx
	shr ecx, 1
	and ecx, 0x007f7f7f
	or ecx, eax
	mov [ebp+bmp2beta.ebufend+ebx*4], ecx
	 inc ebx
	mov [ebp+bmp2beta.error], ecx
	 mov [ebp+bmp2beta.ebufindex], ebx
	pop ecx
	 pop edx
	pop ebx
	ret

.to4:
	push ebx
	mov ebx, [ebp+bmp2beta.ebufindex]
	push edx
	push ecx
	call add_error


	mov edx, eax
	 push eax
	and eax, 0x00c0c0c0
	jz ..@152.ifnot
	  add eax, 0x00404040
	  and eax, 0x01010100
	  mov ah, 0xc0
	jz ..@154.ifnot
	    xor eax, 0x00004001
	..@154.ifnot:
	  add dl, ah
	  adc al, al
	   add dh, ah
	  adc al, al
	  shr edx, 16
	  add dl, ah
	  adc al, al
	..@152.ifnot:
	mov [edi], al
	 inc edi

	shl al, 5
	sbb ecx, ecx
	 add al, al
	sbb dl, dl
	 add al, al
	sbb dh, dh
	 add al, al
	sbb eax, eax
	 or ecx, 0x00808080
	and eax, 0x00ff0000
	 and edx, ecx
	and eax, ecx
	 pop ecx
	xchg eax, edx

	jmp near .ret_error

add_error:
	push eax
	 mov edx, [ebp+bmp2beta.error]
	mov ecx, 0x00ff0000
	 mov eax, [ebp+bmp2beta.ebufend+ebx*4]
	and ecx, eax
	add edx, ecx
	 mov ecx, 0x00ff0000
	and ecx, edx
	 add dl, al
	add dh, ah
	 pop eax
	or eax, 0xff000000
	 add eax, ecx
	sbb ecx, ecx
	 add ah, dh
	sbb ch, ch
	 add al, dl
	sbb cl, cl
	 xor edx, byte -1
	and edx, 0x00808080
	 push eax
	shr edx, 7
	 mov eax, ecx
	add edx, 0x007f7f7f
	xor edx, 0x007f7f7f
	or eax, edx
	 and ecx, edx
	pop edx
	and eax, edx
	or eax, ecx
	ret


ebufalloc:
	neg ecx
	pop eax
	mov [ebp+bmp2beta.transwidth], ecx
	push byte 0
	push ecx
	..@158.do:
	  push byte 0
	  inc ecx
		jnz ..@158.do
	push eax
	ret

fill:
.to32:
	rep stosd
	ret
.to16:
	test edi,2

	jz ..@162.ifnot

	  mov [edi], ax
	  dec ecx
	  add edi, byte 2
	..@162.ifnot:
	shr ecx, 1
	rep stosd
	jnc ..@165.ifnot
	  mov [edi], ax
	  add edi, byte 2
	..@165.ifnot:
	ret
.to8:
.to4:	push edx
	push byte 3
	pop edx
	and edx, edi
	jz ..@168.ifnot
	  sub ecx, edx
	..@170.do:
	    mov [edi], al
	    inc edi
	    dec edx
		jnz ..@170.do
	..@168.ifnot:
	push byte 3
	pop edx
	and edx, ecx
	shr ecx, 2
	rep stosd
	test edx,edx

	jz ..@175.ifnot

	..@177.do:
	    mov [edi], al
	    inc edi
	    dec edx
		jnz ..@177.do
	..@175.ifnot:
	pop edx
	ret


module_size_check:
	mov ebx, esi
	sub ebx, [ebp+bmp2beta.mtop]
	cmp ebx,[ebp+bmp2beta.msize]

	jna ..@182.ifnot

	  mov dword[ebp+bmp2beta.errn], 2
	  jmp _bmp2beta.end
	..@182.ifnot:
	ret

clipping:
	push esi
	mov esi, edx
	sub esi, eax
	jnle ..@185.ifnot
	    add esi, ebx
	    jge .clipR
.notdraw:   xor ecx,ecx
	    pop esi
	    ret
	..@185.ifnot:
	cmp esi, ecx
	jge .notdraw
	sub ecx, esi
	mov esi, ebx
	mov eax, edx
.clipR:	

	cmp esi,ecx
	jnl ..@189.ifnot

	    mov ecx, esi
	..@189.ifnot:
	pop esi
	ret
