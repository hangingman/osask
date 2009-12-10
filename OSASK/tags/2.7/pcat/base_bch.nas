;	"boot.nas" ver.2.1
;	OSASK/AT用のブートプログラム
;	Copyright(C) 2002 H.Kawai (川合秀実)

;	TAB = 4

[BITS 16]
[OPTIMIZE 1]
[OPTION 1]
[INSTRSET "i486p"]
[FORMAT "BIN"]

BootMdl		EQU		0
SysWorkMdl	EQU		(BootSiz + 0x0f) / 16 + BootMdl
StackMdl	EQU		0 ; (osalink0が調節してくれるので気にしない)

; <header> 30bytes.  0x0000 - 0x001d

header:

	DB	"MZ"
	DW	0, 0 ; 最終ページサイズ、ファイルページ数 (osalink0が調節してくれるので気にしない)
	DW	17 ; relocation entries
	DW	0x20 ; header-size / 16
	DW	StackSiz / 16 ; BSS-size / 16 (== stack-size / 16)
	DW	0xffff ; MAXALLOC
	DW	0 ; DOS_SS0 (osalink0が調節してくれるので気にしない)
	DW	StackSiz ; DOS_SP0
	DW	0 ; check-sum (ignore)
	DW	0 ; DOS_IP0 (entry-offset)
	DW	0 ; DOS_CS0 (entry-seg, relative)
	DW	0x001e ; relocation table offset
	DW	0 ; not use overlay
	DW	1 ; I don't know.

; <relocation table image> 0x001e - 0x01ff
; org 0x001e

; DW ofs, seg
	DW	BootMdl*16+4, 0 ; for SysWorkMdl
	DW	SysWorkMdl*16+modulelist+ 0*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 1*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 2*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 3*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 4*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 5*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 6*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 7*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 8*16+12, 0
	DW	SysWorkMdl*16+modulelist+ 9*16+12, 0
	DW	SysWorkMdl*16+modulelist+10*16+12, 0
	DW	SysWorkMdl*16+modulelist+11*16+12, 0
	DW	SysWorkMdl*16+modulelist+12*16+12, 0
	DW	SysWorkMdl*16+modulelist+13*16+12, 0
	DW	SysWorkMdl*16+modulelist+14*16+12, 0
	DW	SysWorkMdl*16+modulelist+15*16+12, 0

	RESB	header+512-$

	ORG		0x0000 ; NASKは何回でもORGできる

BootBgn:
Entry:
	JMP		.entry2 ; 2バイト

	RESB	4-$ ; MASMORG(0x0004)

.SysWorkSeg:
	DW		SysWorkMdl

	RESB	8-$ ; MASMORG(0x0008)

	JMP		V86TaskEntry

.entry2:
.pit1_skip:
	MOV		 AX, DS
	MOV		 DS, WORD [CS:.SysWorkSeg]
	MOV		 SI, CS
	ADD		 AX,16
	CMP		 AX, SI
	JE		.fromdos
	MOV		 BYTE [DiskCacheReady],3

.fromdos:
	mov	 ax, word ds:[VGA_mode]
	int	10h
mov dx,03d4h
mov ax,3213h
out dx,ax

	MOV		 AH, 0x02
	INT		0x16 ; keyboard BIOS
	SHR		 AL,4
	AND		 AL,0x07
	MOV		BYTE [boot_keylock],AL

    LSS		ESP,DWORD [stackseg+8]	; espの上位wordをクリア

	MOV		 AX, CS
	SHL		EAX,4
	AND		EAX,0xff000	; 4KB単位にする
	MOV		DWORD [bootmalloc_fre0],EAX
	MOV		 AX, SS
	SHL		EAX,4
	ADD		EAX,ESP
	ADD		EAX,0xfff
	AND		EAX,0xff000	; 4KB単位にする
	MOV		DWORD [bootmalloc_adr1],EAX
	MOV		ECX,640*1024
	SUB		ECX,EAX
	MOV		DWORD [bootmalloc_fre1],ECX

;	AC = 1となるので、SPをdwordアラインしておくこと

	PUSHFD
	POP		EAX
	OR		EAX,0x00240000	; bit18,21
	AND		EAX,0xfffc88ff	; bit8,9,10,12,13,14,16,17
	PUSH	EAX
	POPFD	
	PUSHFD
	POP		DWORD [eflags]

;	CLI		; IDTが設定されるまで、割り込みを禁止する

	MOV		ECX,DWORD [alloclist+1*16+ 8]
	CALL	bootmalloc
	MOV		DWORD [alloclist+1*16+12],EAX	; idtgdt

	MOV		ECX,DWORD [alloclist+0*16+ 8]
	CALL	bootmalloc
	MOV		EBX,EAX
	MOV		DWORD [alloclist+0*16+12],EAX	; pde,pte
	SHR		EAX,4
	XOR		 DI, DI
	MOV		 ES, AX
	MOV		ECX,DWORD [alloclist+0*16+ 8]
	SHR		ECX,1	; ECX /= 2;
;	CLD
	XOR		 AX, AX
	REP STOSW

	ADD		EBX,4096
	MOV		 CX, ES
	MOV		EAX,EBX
	ADD		 CX,0x0100 ; skip link-page
	MOV		 ES, CX

	TEST	 BYTE [eflags+2],0x04	; bit18(AC)
	JZ		.skip386_1
	OR		 AL,0x10	; PDE,PTEはキャッシュしない(PCD=1) 
.skip386_1:
	MOV		CR3,EAX
	XOR		 DI, DI
	ADD		EAX,4096+0x07	; present, R/W, user
	STOSD				; ES:0x0000
	ADD		EAX,4096	; 32bit-VRAM-page
	MOV		DWORD [ES:0x0e00],EAX
	ADD		EAX,4096
	STOSD				; ES:0x0004
	ADD		EAX,4096
	STOSD				; ES:0x0008
	ADD		EAX,4096
	STOSD				; ES:0x000C
	ADD		EAX,4096
	STOSD				; ES:0x0010

	MOV		 DI,4096
	MOV		 CX,640/4
	MOV		EAX,7	; present, R/W, user
.fillpte:
	STOSD
	ADD		EAX,4096
	LOOP	.fillpte

	MOV		EAX,DWORD [alloclist+16*1+12]	; gdt
	SHR		EAX,4
	MOV		 GS, AX

;	to protect mode

	MOV		EAX,CR0

	OR		EAX,10000000_00000000_00000000_00001101b ; PG,TS,PE
	AND		EAX,10011111_11111010_11111111_11111011b ; CD,NW,AM,WP,EM

	TEST	 BYTE [eflags+2],0x04	; bit18(AC)
	JZ		.skip386_2
	OR		 AL,00100001b ; NE,PE
.skip386_2:
	MOV		CR0,EAX	; モード移行
	JMP		..$	; for pipeline-flash
	MOV		ESI,modulelist
	MOV		ECX,loaded_modules
.expandaddr:
	MOV		EAX,DWORD [ESI+12]
	SHL		EAX,4
	AND		EAX,0xffff0
	MOV		DWORD [ESI+12],eax
	ADD		ESI,16
	LOOP	.expandaddr

;	GDTにDTを作る

	XOR		 SI, SI
	MOV		 CX,4096/4
	XOR		EAX,EAX
.clrgdt:
	MOV		DWORD [GS:SI],EAX
	ADD		 SI,4
	LOOP	.clrgdt

;	int init_sel    == 2 * 8, init_ent == 1 * 16;
.init_sel	EQU		2 * 8
.init_ent	EQU		1 * 16

	MOV		EAX,DWORD [modulelist+.init_ent+12] ; init-sel
	MOV		 WORD [GS:384+.init_sel+2], AX
	SHR		EAX,16
	MOV		 BYTE [GS:384+.init_sel+4], AL
	MOV		 BYTE [GS:384+.init_sel+7], AH
	MOV		EAX,DWORD [modulelist+.init_ent+08]
	DEC		EAX
	MOV		 WORD [GS:384+.init_sel+0], AX
	MOV		 BYTE [GS:384+.init_sel+5],10011010b	; ER
	MOV		 BYTE [GS:384+.init_sel+6],01000000b	; use32

	PUSH	DWORD [alloclist+16*1+12]	; idtgdt
	PUSH	DWORD 383*10000h
	LIDT	[ESP+2]
	ADD		DWORD [ESP+4],384
	MOV		 WORD [ESP+2],(4096 - 384) - 1
	LGDT	[ESP+2]
	ADD		 SP,8

	JMP		FAR DWORD .init_sel:0

V86TaskEntry:
	MOV		ESP,StackSiz
	INT		0x10
;	push	 gs
;	push	 fs
;	push	 ds
;	push	 es
;	pushad
	INT		3	; これをトラップする

bootmalloc:

;	dsはsyswork,ecxにバイト数 -> eaxにヘッドアドレス

	SUB		DWORD [bootmalloc_fre1],ECX
	JB		.check0
	MOV		EAX,DWORD [bootmalloc_adr1]
	ADD		EAX,DWORD [bootmalloc_fre1]
	RET

.check0:
;			add	dword ptr ds:[bootmalloc_fre1],ecx
;			sub	dword ptr ds:[bootmalloc_fre0],ecx
;			jb	short Boot_error
;			mov	eax,dword ptr ds:[bootmalloc_adr0]
;			add	eax,dword ptr ds:[bootmalloc_fre0]
;			ret
.error:
			JMP		.error

BootSiz		EQU		$ - BootBgn

			RESB	(16 - ($ % 16)) % 16

			ORG		0x0000

SysWorkBgn:

VESA_busdevfnc	DD	0 ; 80 bus dev-func ofs-reg
VESAPNP_00		DD	0
VESAPNP_08		DD	0
VESAPNP_2c		DD	0

VGA_mode		DW	12h	; +0x10
				DW	0
to_winman0		DD	1 ; +0x14

VGA_PCI_base	DD	0
eflags			DD	0

		;	align	16

modulelist:
	DD	"syswork ",SysWorkSiz,SysWorkMdl	;  0 * 16
	DD	"init    ",0,BootMdl	;  1 * 16
	DD	"vgadrv0 ",0,BootMdl	;  2 * 16
	DD	"keymos0 ",0,BootMdl	;  3 * 16
	DD	"timerdrv",0,BootMdl	;  4 * 16
	DD	"tapi0   ",0,BootMdl	;  5 * 16
	DD	"decode0 ",0,BootMdl	;  6 * 16
	DD	"fdcdrv0 ",0,BootMdl	;  7 * 16
	DD	"bootseg ",BootSiz,BootMdl	;  8 * 16
stackseg:
	DD	"stack000",StackSiz,StackMdl	;  9 * 16
	DD	"pioneer0",0,BootMdl	; 10 * 16
	DD	"winman0 ",0,BootMdl	; 11 * 16
	DD	"pokon0  ",0,BootMdl	; 12 * 16
	DD	"vesadrv0",0,BootMdl	; 13 * 16
	DD	"ankfont0",0,BootMdl	; 14 * 16
	DD	"papi0   ",0,BootMdl	; 15 * 16

loaded_modules	EQU		($ - modulelist) / 16

alloclist:
	DD	"pdepte  ",4096*8,-1	; 0
	DD	"idtgdt  ",4096,-1	; 48+463entry
	DD	"fontbuf ",4096,-1
	DD	"stack   ",4096*4,-1
	DD	"keydata ",4096,-1	; 4
	DD	"gapidata",4096*3,-1   ; 起動最低限しか用意しない
	DD	"timerdat",4096*2,-1
	DD	"tapiwork",0,0 ; for 31 tasks(init, idle, winman0, pokon0).
	DD	"decodata",4096*6,-1	; 8
	DD	"fdcwork ",4096,-1
	DD	"papiwork",4096*8,-1

	DD	0

bootmalloc_adr0		DD	0	; (+0x01d4)
bootmalloc_fre0		DD	0	; (+0x01d8)
bootmalloc_adr1		DD	0	; LastMdlを指す (+0x01dc)
bootmalloc_fre1		DD	0	; 640KB - bootmalloc_adr1 (+0x01e0)

FD_motor_init		DB	01ch ; motor on
FD_cache_init		DB	1 ; must init
DiskCacheReady		DB	0
	; bit0 : cache enable, bit1:boot from OSASK boot-sector
boot_keylock		DB	0
FD_debug			DD	-1

SysWorkSiz	EQU		$ - SysWorkBgn

			RESB	(16 - ($ % 16)) % 16

StackSiz	EQU		256

END
