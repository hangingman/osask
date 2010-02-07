[FORMAT "WCOFF"]
[INSTRSET "i486p"]
[OPTIMIZE 1]
[OPTION 1]
[BITS 32]

		GLOBAL	startup

[SECTION .text]
startup:
		CALL	[ESI]
		DB		0x38, 0x65, 0x00, 0x01, "y", 0x41, "year"
		DB		0x01, "m", 0x85, "month"
		DB		0x48, 0x60, 0x85, 0x86, 0x10, 0x13
		MOV		BL,12
		TEST	EAX,EAX
		JZ		.skp6
		DEC		ECX
		CMP		ECX,12
		JAE		.err
		MOV		EBX,ECX
.skp6:
		MOV		CH,BL
		MOV		EAX,EBP
		CMP		EAX,100
		JB		.err
		CMP		EAX,9999
		JNA		.skp5
.err:
		CALL	[ESI]
		DB		0x86, 0xbf
.skp5:
		CDQ
		MOV		EBX,400
		DIV		EBX
		TEST	EDX,EDX
		JZ		.skp2
		MOV		EAX,EBP
		TEST	AL,0x03
		JNZ		.skp3
		MOV		BL,100
		DIV		BL
		TEST	AH,AH
		JZ		.skp3
.skp2:
		ADD		BYTE [Table+1],0x10
.skp3:

; m==CL, m1==CH, y==EBP

.lop1:
		MOVZX	EAX,CL
		LEA		EAX,[EAX*3+MonthName]
		CALL	[ESI]
		DB		0x35, 0x18, 0x0a, "         ", 0x00
		DB		0x52, 0x30, 0x50, 0x13, " "
		MOV		EAX,EBP
		MOV		BL,100
		DIV		BL
		MOV		DH,'0'
		CALL	putdec2
		MOV		DH,0
		MOV		AL,AH
		CALL	putdec2
		CALL	[ESI]
		DB		0x51, 0x0a, 0x0a, "Sun Mon Tue Wed Thu Fri Sat", 0x0a, 0x00
		MOV		EAX,EBP
		CMP		CL,2
		SBB		EAX,0
		MOV		EDX,EAX
		SHR		EAX,2
		ADD		EDX,EAX
		MOV		BL,25
		DIV		BL
		CBW
		SUB		EDX,EAX
		SHR		EAX,2
		ADD		EDX,EAX
		MOVZX	EAX,CL
		MOV		BL,BYTE [EAX+Table]
		MOV		AL,BL
		SHR		BL,4
		AND		EAX,0x0f
		ADD		EAX,EDX
		CDQ
		PUSH	7
		POP		EDI
		DIV		EDI
		XOR		EDI,EDI
		ADD		BL,28
		TEST	DL,DL
		JZ		.skp4
		PUSH	EDX
.lop2:
		CALL	[ESI]
		DB		0x51, "    ", 0x00
		DEC		EDX
		JNZ		.lop2
		POP		EDX
.skp4:

; m==CL, m1==CH, w==DL, table1[m]==BL

		MOV		AL,1
.lop0:
		CALL	[ESI]
		DB		0x51, " ", 0x00
		MOV		DH,'0'
		CALL	putdec2
		MOV		AH,' '
		INC		EDX
		CMP		DL,7
		JNE		.skp0
		MOV		AH,0x0a
		MOV		DL,0
.skp0:
		CALL	[ESI]
		DB		0x55, 0x16, 0xbc
		INC		EAX
		CMP		AL,BL
		JBE		.lop0
		TEST	DL,DL
		JE		.skp1
		CALL	[ESI]
		DB		0x51, 0x0a, 0x00
.skp1:
		INC		ECX
		CMP		CL,CH
		JB		.lop1
		CALL	[ESI]
		DB		0x51, 0x0a, 0x00
		RET

putdec2:
		PUSHAD
		CBW
		MOV		DL,10
		DIV		DL
		ADD		AX,0x3030
		CMP		AL,DH
		JNE		.skp
		MOV		AL,' '
.skp:
		CALL	[ESI]
		DB		0x55, 0x26, 0xb8, 0x6b, 0xc0
		POPAD
		RET

[SECTION .data align=4]
Table:
		DB		0x31, 0x04, 0x33, 0x26, 0x31, 0x24, 0x36, 0x32, 0x25, 0x30, 0x23, 0x35
MonthName:
		DB		"JanFebMarAprMayJunJulAugSepOctNovDec"
