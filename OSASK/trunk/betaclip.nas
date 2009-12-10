[INSTRSET "i386p"]
[FORMAT "WCOFF"]
[FILE "betaclip.nas"]
[bits 32]

extern fill.to16
extern fill.to8
extern clipping

;[absolute -4*11]
betaclip:	;equ -4*11
.ebufend:	equ -4*11
.output:	equ -4*11	;resd 1
.fill:		equ -4*10	;resd 1
.destskip:	equ -4*9	;resd 1
.rightmargin:	equ -4*8	;resd 1
.transstartx:	equ -4*7	;resd 1
.transwidth:	equ -4*6	;resd 1
.downmargin:	equ -4*5	;resd 1
.upmargin:	equ -4*4	;resd 1
.transstarty:	equ -4*3	;resd 1
.transheight:	equ -4*2	;resd 1
.srcwidthbyte:	equ -4		;resd 1
.savedregs:	equ 0		;resd 4
.reteip:	equ 4*4		;resd 1
.src:		equ 4*5		;resd 1
.srcw:		equ 4*6		;resd 1
.srch:		equ 4*7		;resd 1
.dest:		equ 4*8		;resd 1
.destw:		equ 4*9		;resd 1
.desth:		equ 4*10	;resd 1
.mode:		equ 4*11	;resd 1
.backcolor:	equ 4*12	;resd 1

[section .text]
global _betaclip
_betaclip:
	push edi
	push esi
	push ebp
	push ebx
	mov ebp,esp
	mov ecx, [ebp+betaclip.srcw]
	xor eax,eax
	shl ecx, 2
	mov ebx, [ebp+betaclip.desth]
	push ecx
	mov ecx, [ebp+betaclip.srch]
	mov edx, ecx
	sub edx,ebx
	sar edx, 1
	call clipping
	test ecx,ecx
	jz near end
	push ecx
	push eax
	sub eax, edx
	add ecx, eax
	push eax
	sub ebx, ecx
	push ebx
	xor eax,eax
	mov ecx, [ebp+betaclip.srcw]
	mov ebx, [ebp+betaclip.destw]
	mov edx, ecx
	sub edx, ebx
	inc edx
	sar edx, 1
	call clipping
	test ecx,ecx
	jz near end
	push ecx
	push eax
	sub eax, edx
	pop edx
	push edx
	add edx, ecx
	add ecx, eax
	sub ebx, ecx
	shl ecx, 2
	push ebx
	add ebx, eax
	push ebx
	mov ebx, edx

	mov esi, [ebp+betaclip.src]
	mov edi, [ebp+betaclip.dest]
	mov ecx, eax
	mov eax, [ebp+betaclip.destw]
	mul dword[ebp+betaclip.upmargin]
	add ecx, eax

	mov eax, [ebp+betaclip.srcwidthbyte]
	mul dword[ebp+betaclip.transstarty]
	add esi, eax
	mov eax, [ebp+betaclip.backcolor]
	cmp dword[ebp+betaclip.mode], byte 2
	ja near bpp32

	lea esi, [esi+ebx*4]
	mov eax, [ebp+betaclip.backcolor]
	xor ebx, ebx
	mov edx, [ebp+betaclip.transwidth]
	sub ebx, edx
	cmp dword[ebp+betaclip.mode],byte 1

	jna ..@6.ifnot

	  push dword fill.to16
	  push dword output.bpp16
	 jmp short ..@6.ifend


	..@6.ifnot:

	  push dword fill.to8
	jne ..@9.ifnot
	    push dword output.bpp8
	 jmp short ..@9.ifend


	..@9.ifnot:

	    push dword output.bpp4
	..@9.ifend:
	  mov ah, al
	..@6.ifend:
	mov edx, eax
	shl eax, 16
	or  eax, edx
	mov [ebp+betaclip.backcolor], eax

	call [ebp+betaclip.fill]

	..@14.do:
	  push byte 0
	  dec edx
		jnz ..@14.do
	jmp short .enter

	..@17.do:
	  add esi, [ebp+betaclip.srcwidthbyte]
	  sub ebx, [ebp+betaclip.transwidth]
	  mov ecx, [ebp+betaclip.destskip]
	  mov eax, [ebp+betaclip.backcolor]
	  call [ebp+betaclip.fill]
	..@19.do:
.enter:
	    mov ecx, 0x00ff0000
	     mov eax, [ebp+betaclip.ebufend+ebx*4]
	    and ecx, eax
	    add edx, ecx
	     mov ecx, 0x00ff0000
	    and ecx, edx
	     add dl, al
	    add dh, ah
	     mov eax,[esi+ebx*4]
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
	    or  eax, ecx

	    call [ebp+betaclip.output]
	    inc   ebx
		jnz ..@19.do
	  dec dword[ebp+betaclip.transheight]
		jnz ..@17.do
	mov eax, [ebp++betaclip.destw]
	mov ecx, [ebp+betaclip.rightmargin]
	mul dword[ebp+betaclip.downmargin]
	add ecx, eax
	mov eax, [ebp+betaclip.backcolor]
	call [ebp+betaclip.fill]

end:	mov esp, ebp
	pop ebx
	pop ebp
	pop esi
	pop edi
	ret

output:
.bpp4:

	mov edx, eax
	 push eax
	and eax, 0x00c0c0c0
	jz ..@23.ifnot
	  add eax, 0x00404040
	  and eax, 0x01010100

	  mov ah, 0xc0
	jz ..@25.ifnot
	    xor eax, 0x00004001
	..@25.ifnot:
	  add dl, ah
	  adc al, al
	   add dh, ah
	  adc al, al
	  shr edx, 16
	  add dl, ah
	  adc al, al
	..@23.ifnot:
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

	jmp short .ret_error

.bpp8:

	mov edx, 0x0000c0c0
	and edx, eax
	shr edx, 2
	 mov ecx, eax
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
	or  ecx, eax
	mov [ebp+betaclip.ebufend+ebx*4], ecx
	 mov edx, ecx
	ret

.bpp16:
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

bpp32:
	mov ebx, [ebp+betaclip.transstartx]
	mov edx, [ebp+betaclip.transheight]
	rep stosd
	mov ecx, [ebp+betaclip.transwidth]
	lea esi, [esi+ebx*4]
	mov ebx, [ebp+betaclip.srcw]
	sub ebx, ecx
	shl ebx, 2
	jmp short .enter
	..@29.do:
	  add esi, ebx
	  mov ecx, [ebp+betaclip.destskip]
	  rep stosd
	  mov ecx, [ebp+betaclip.transwidth]
.enter:	  rep movsd
	  dec edx
		jnz ..@29.do
	push eax
	mov eax, [ebp+betaclip.destw]
	mov ecx, [ebp+betaclip.rightmargin]
	mul dword[ebp+betaclip.downmargin]
	add ecx, eax
	pop eax
	rep stosd
	jmp near end
