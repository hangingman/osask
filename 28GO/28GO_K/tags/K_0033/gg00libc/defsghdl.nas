[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 32]
[FILE 'defsghdl.nas']
GLOBAL _lib_definesignalhandler
GLOBAL _handler
GLOBAL _user_handler
GLOBAL _softint_ret

[SECTION .data]

			ALIGNB	4
_user_handler		dd	0
_softint_ret		dd	0018h, 0080h, 0, 1, 0000h

[SECTION .text]

_lib_definesignalhandler:

			push	ebx
			mov	ebx,dword ss:[esp + 8] ; void (*lib_signalhandler)(int *)
			push	0
			mov	[DS:_user_handler],ebx
			push	 cs
			push	_handler
			push	0010h
			mov	ebx,esp
			call	0x00c7:0
			add	esp,16
			pop	ebx
			ret

_handler:

			mov	eax,000fh
			mov	 es, ax
			mov	 ds, ax
			mov	 fs, ax
			mov	eax,0027h
			mov	 gs, ax
			push	esp
			call	_user_handler
			mov	ebx,_softint_ret
			pop	eax
			call	0x00c7:0
