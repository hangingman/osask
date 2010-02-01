[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 32]
[FILE 'initmdh1.nas']
GLOBAL _lib_initmodulehandle1

[SECTION .data]

			ALIGNB	4
subcmd	dd	0ffffff01h, 0, 0ffffff02h, 2, 07f000001h, 0, 0000h
command	dd	00a0h, 00e0h, 0, 28, subcmd, 000ch, 0000h

[SECTION .text]

; void lib_initmodulehandle1(const int slot, const int num, const int sig)

_lib_initmodulehandle1:

	push	ebx
	push	ecx
	mov	ebx,command
	push	eax
	mov	eax,[esp+16]	; slot
	mov	ecx,[esp+20]	; num
	mov	dword ds:[ebx+8],eax
	mov	eax,[esp+24]	; sig
	mov	dword ds:[ebx-28+4],ecx
	mov	dword ds:[ebx-28+20],eax
	db	09ah
	dd	0
	dw	000c7h
	pop	eax
	pop	ecx
	pop	ebx
	ret

