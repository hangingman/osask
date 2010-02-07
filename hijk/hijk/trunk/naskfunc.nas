[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[BITS 32]
[FILE "naskfunc.nas"]

	GLOBAL	_start_app, _asm_api, _asm_end
	EXTERN	_c_api

[SECTION .text]

;	void start_app(esi);

_start_app:
			MOV		ESI,[ESP+ 4]
			MOV		EAX,[ESI+20]
			LEA		ESP,[EAX- 4]
			MOV		EAX,[ESI+ 4]
			MOV		ECX,[ESI+ 8]
			MOV		EDX,[ESI+12]
			MOV		EBX,[ESI+16]
			MOV		EBP,[ESI+24]
			MOV		EDI,[ESI+32]
			RET

_asm_api:
		;	PUSH	[ESP]
			PUSHAD
			CALL	_c_api
		;	MOV		EAX,[ESP+32]
		;	MOV		[ESP+36],EAX
			POPAD
		;	ADD		ESP,4
			RET

_asm_end:
			XOR		EDI,EDI
			CALL	_asm_api
			DB		0x40
		;	RESB	32
