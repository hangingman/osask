.386p

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

			include ../inc.asm
