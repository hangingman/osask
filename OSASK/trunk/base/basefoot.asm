InitMdl		segment	PARA USE32 ER 
InitSiz:
InitMdl		ends	

StackMdl		segment	para use32 stack

StackBgn		equ	$

			dd	64 dup (?)	; 256bytes

StackSiz		equ	$-StackBgn

StackMdl		ends

			end	Entry
