[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 32]
[FILE 'readmdsz.nas']
GLOBAL _lib_readmodulesize

[SECTION .data]

buf	dd	0, 0, 0, 0
command	dd	00c8h, 0ffffff03h, 0, 16, buf, 000ch, 0000h

[SECTION .text]

; int lib_readmodulesize(int slot)

_lib_readmodulesize:

	push	ebx
	mov	ebx,command
	mov	eax,[esp+8]	; slot
	mov	dword ds:[ebx+8],eax
	db	09ah
	dd	0
	dw	000c7h
	pop	ebx
	mov	eax,dword ds:[buf+0]
	ret
