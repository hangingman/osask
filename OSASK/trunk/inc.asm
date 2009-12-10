cmp0			macro	reg
			test	reg,reg
			endm

clr			macro	reg
			xor	reg,reg
			endm

fcall			macro	sel,ofs
			db	09ah
			dd	offset ofs
			dw	sel
			endm

jmp_system_count0	macro
			jmp	fword ptr ss:[0ffffffe8h]
;			call	fword ptr ss:[0ffffffe8h]
			endm
