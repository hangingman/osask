;	"boot.asm" ver.1.9
;	OSASK用のブートプログラム
;	Copyright(C) 2001 H.Kawai (川合秀実)

.486p
			include ./inc.asm

BootMdl			segment para use16

BootBgn			equ	$

			assume	cs:BootMdl,ds:BootMdl,es:BootMdl
			assume	ss:BootMdl,fs:BootMdl,gs:BootMdl

Entry			proc	near

			org	0000h
			jmp	short Boot_entry2 ; 2バイト

			org	0004h
SysWorkSeg		dw	SysWorkMdl

			org	0008h
			jmp	V86TaskEntry

Boot_entry2:

;	パラメーター解析 & VGA 832x630x4bitにする

		;	DB	8CH,0C8H
		;	DB	8EH,0D8H

ifdef abcdefgh
;	PIT1のチェック
			in	 al, 0041h ; dummy read
			nop
			in	 al, 0041h

			mov	 al,11100100b ; read back command
			out	0043h, al
			nop
			in	 al, 0041h
			mov	 ah, al
			test	 al,01h
			jnz	Boot_pit1_error ; BCD count
			and	 ax,3006h
			cmp	 al,04h
			jne	short Boot_pit1_set ; モード2じゃないので勝手にやる
			cmp	 ah,20h			
			je	Boot_pit1_error ; high 8bit mode
			test	 ah, ah
			je	Boot_pit1_error ; unknown mode

			jmp	short Boot_pit1_skip
Boot_pit1_set:
			mov	 al,54h
			out	 0043h, al
			mov	 al,0ffh
			out	 0041h, al
endif
Boot_pit1_skip:
			DB	8CH,0D8H
			mov	 ds, word ptr cs:[SysWorkSeg]
			DB	8CH,0CEH
			add	 ax, 16
			cmp	 ax, si
			je	Boot_normal
		;	jne	Boot_boot_from_IPL

Boot_boot_from_IPL:
			mov	 byte ptr ds:[DiskCacheReady],3
Boot_normal:
		;	mov	 ax, word ptr ds:[VGA_mode]
		;;	cmp	 ax,0012h
		;;	je	Boot_normal2 ; !!!!
		;;	cmp0	 ah
		;;	jne	Boot_VESA
		;	int	10h
;mov dx,03d4h
;mov ax,3213h
;out dx,ax

ifdef abcdefgh

			jmp	Boot_normal2
Boot_VESA:
			mov	eax,dword ptr ds:[VESA_busdevfnc]
			and	eax,0ffffff00h
			je	Boot_normal5

;	デバイスの確認

			cli
			mov	 dx,0cf8h
			mov	ecx,eax
			out	 dx,eax
			or	 dx,4
			in	eax,dx
			and	 dx,not 4
			cmp	eax,dword ptr ds:[VESAPNP_00]
			jne	Boot_normal4

			lea	eax,[ecx+08h]
			out	 dx,eax
			or	 dx,4
			in	eax,dx
			and	 dx,not 4
			cmp	eax,dword ptr ds:[VESAPNP_08]
			jne	Boot_normal4

			lea	eax,[ecx+2ch]
			out	 dx,eax
			or	 dx,4
			in	eax,dx
			and	 dx,not 4
			cmp	eax,dword ptr ds:[VESAPNP_2c]
			jne	Boot_normal4

			mov	eax,dword ptr ds:[VESA_busdevfnc]
			and	eax,07h
			lea	eax,[ecx+eax*4+10h]
			out	 dx,eax
			or	 dx,4
			in	eax,dx
			and	 dx,not 4
			and	eax,0fffff000h
			test	eax,1
			jnz	Boot_normal4
			mov	dword ptr ds:[VGA_PCI_base],eax
			clr	eax
			out	 dx,eax

			mov	 al, byte ptr ds:[VESA_busdevfnc]
			mov	 cl, al
			and	eax,00000080h
			shr	 cl,3
			jz	VESA_noofs
			or	eax,00000100h
			and	 cl,0fh
			add	 cl,6
			shl	eax, cl
			add	dword ptr ds:[VGA_PCI_base],eax
VESA_noofs:
			mov	eax,dword ptr ds:[vesadrv_sizadr][0]
			mov	ecx,dword ptr ds:[vesadrv_sizadr][4]
			mov	dword ptr ds:[vgadrv_sizadr][0],eax
			mov	dword ptr ds:[vgadrv_sizadr][4],ecx
Boot_normal5:
			mov	 bx, word ptr ds:[VGA_mode]
			mov	 ax,4f02h
			int	10h
			cmp	 ax,004fh
			jne	Boot_normal3
			jmp	Boot_normal2
Boot_normal4:
			clr	eax
			out	 dx,eax
Boot_normal3:
			mov	 word ptr ds:[VGA_mode],0012h
		;	mov	 word ptr ds:[GUIGUI_mouse_limit_x],640
		;	mov	 word ptr ds:[GUIGUI_mouse_limit_y],480
			clr	eax
			mov	dword ptr ds:[VGA_PCI_base],eax

endif

Boot_normal2:
			sti
			mov	 ss, word ptr ds:[stackseg][4]
			mov	esp,dword ptr ds:[stackseg][0]	; espの上位wordをクリア

			DB	8CH,0C8H
			shl	eax,4
			and	eax,0ff000h	; 4KB単位にする
			mov	dword ptr ds:[bootmalloc_fre0],eax
			DB	8CH,0D0H
			shl	eax,4
			add	eax,esp
			add	eax,0fffh
			and	eax,0ff000h	; 4KB単位にする
			mov	dword ptr ds:[bootmalloc_adr1],eax
			mov	ecx,640*1024
			sub	ecx,eax
			mov	dword ptr ds:[bootmalloc_fre1],ecx

;	AC = 1となるので、SPをdwordアラインしておくこと

			pushfd
			pop	eax
			or	eax,000240000h	; bit18,21
			and	eax,0fffc8effh	; bit8,12,13,14,16,17
			push	eax
			popfd	
			pushfd
			pop	dword ptr ds:[eflags]

			cli	; IDTが設定されるまで、割り込みを禁止する

			mov	ecx,dword ptr ds:[alloclist][1*16][08]
			call	bootmalloc
			mov	dword ptr ds:[alloclist][1*16][12],eax	; idtgdt

			mov	ecx,dword ptr ds:[alloclist][0*16][08]
			call	bootmalloc
			mov	ebx,eax
			mov	dword ptr ds:[alloclist][0*16][12],eax	; pde,pte
			shr	eax,4
			clr	 di
			DB	8EH,0C0H
			mov	ecx,dword ptr ds:[alloclist][0*16][08]
			shr	ecx,1	; ECX /= 2;
			cld
			clr	 ax
			rep stosw

			add	ebx,4096
			DB	8CH,0C1H
			mov	eax,ebx
			add	 cx,0100h ; skip link-page
			DB	8EH,0C1H

			test	 byte ptr ds:[eflags][2],004h	; bit18(AC)
			jz	short Boot_skip386_1
			or	 al,010h	; PDE,PTEはキャッシュしない(PCD=1) 
Boot_skip386_1:
			mov	cr3,eax
			add	eax,4096+07h	; present, R/W, user
			mov	dword ptr es:[0000h],eax
			add	eax,4096	; 32bit-VRAM-page
			mov	dword ptr es:[0e00h],eax
			add	eax,4096
			mov	dword ptr es:[0004h],eax
			add	eax,4096
			mov	dword ptr es:[0008h],eax
			add	eax,4096
			mov	dword ptr es:[000ch],eax
			add	eax,4096
			mov	dword ptr es:[0010h],eax

			mov	 di,4096
			mov	 cx,640/4
			mov	eax,7	; present, R/W, user
Boot_fillpte:
			stosd
			add	eax,4096
			loop	Boot_fillpte

			mov	eax,dword ptr ds:[alloclist][16*1][12]	; gdt
			shr	eax,4
			DB	8EH,0E8H

;	to protect mode

			mov	eax,cr0

			or	eax,10000000000000000000000000001101b ; PG,TS,PE
			and	eax,10011111111110101111111111111011b ; CD,NW,AM,WP,EM

			test	 byte ptr ds:[eflags][2],004h	; bit18(AC)
			jz	short Boot_skip386_2

			or	eax,10000000000000000000000000100001b ; PG,NE,PE
Boot_skip386_2:
			mov	cr0,eax	; モード移行

			jmp	short Boot_Flash	; for pipeline-flash
Boot_Flash:
			mov	esi,offset modulelist
			mov	ecx,offset loaded_modules
Boot_expandaddr:
			mov	eax,dword ptr ds:[esi][12]
			shl	eax,4
			and	eax,0ffff0h
			mov	dword ptr ds:[esi][12],eax
			add	esi,16
			loop	Boot_expandaddr

;	GDTにDTを作る

			xor	esi,esi
			mov	ecx,4096/4
			clr	eax
Boot_clrgdt:
			mov	dword ptr gs:[esi],eax
			add	esi,4
			loop	Boot_clrgdt

;	int init_sel    == 2 * 8, init_ent == 1 * 16;
Boot_init_sel		equ	2 * 8
Boot_init_ent		equ	1 * 16

			mov	eax,dword ptr ds:[modulelist][Boot_init_ent][12] ; init-sel
			mov	 word ptr gs:[384][Boot_init_sel][2], ax
			shr	eax,16
			mov	 byte ptr gs:[384][Boot_init_sel][4], al
			mov	 byte ptr gs:[384][Boot_init_sel][7], ah
			mov	eax,dword ptr ds:[modulelist][Boot_init_ent][08]
			dec	eax
			mov	 word ptr gs:[384][Boot_init_sel][0], ax
			mov	 byte ptr gs:[384][Boot_init_sel][5],10011010b	; ER
			mov	 byte ptr gs:[384][Boot_init_sel][6],01000000b	; use32

			push	dword ptr ds:[alloclist][16*1][12]	; idtgdt
		;	push	383*10000h
			db	66h,68h
			dd	383*10000h
			lidt	fword ptr ss:[esp][2]
			add	dword ptr ss:[esp][4],384
			mov	 word ptr ss:[esp][2],(4096 - 384) - 1
			lgdt	fword ptr ss:[esp][2]
			add	esp,8

			db	66h	; opsiz-prefix
			db	11101010b ; far-jmp to 32bit-code
		;	dd	offset Init
			dd	0
			dw	Boot_init_sel	; init-sel

Boot_param_err:
Boot_pit1_error:
			mov	 ax,04c01h
			int	21h

Entry			endp

V86TaskEntry		proc	near

			mov	esp,offset StackSiz
			int	10h
		;	push	 gs
		;	push	 fs
		;	push	 ds
		;	push	 es
		;	pushad
			int	03h	; これをトラップする

V86TaskEntry		endp

bootmalloc		proc	near

;	dsはsyswork,ecxにバイト数 -> eaxにヘッドアドレス

			sub	dword ptr ds:[bootmalloc_fre1],ecx
			jb	short Boot_check0
			mov	eax,dword ptr ds:[bootmalloc_adr1]
			add	eax,dword ptr ds:[bootmalloc_fre1]
			ret
Boot_check0:
;			add	dword ptr ds:[bootmalloc_fre1],ecx
;			sub	dword ptr ds:[bootmalloc_fre0],ecx
;			jb	short Boot_error
;			mov	eax,dword ptr ds:[bootmalloc_adr0]
;			add	eax,dword ptr ds:[bootmalloc_fre0]
;			ret
Boot_error:
			jmp	Boot_error

bootmalloc		endp

BootSiz			equ	$-BootBgn

BootMdl			ends

SysWorkMdl		segment	para use16

SysWorkBgn		equ	$

			align	4

VESA_busdevfnc		dd	0 ; 80 bus dev-func ofs-reg
VESAPNP_00		dd	0
VESAPNP_08		dd	0
VESAPNP_2c		dd	0

VGA_mode		dw	12h	; +0x10
			dw	0
to_winman0		dd	0 ; +0x14

VGA_PCI_base		dd	0
eflags			dd	0

			align	16

modulelist		db	"syswork "	;  0 * 16
			dd	offset SysWorkSiz,SysWorkMdl
			db	"init    "	;  1 * 16
			dd	0,BootMdl
			db	"vgadrv0 "	;  2 * 16
vgadrv_sizadr		dd	0,BootMdl
			db	"keymos0 "	;  3 * 16
			dd	0,BootMdl
			db	"timerdrv"	;  4 * 16
			dd	0,BootMdl
			db	"tapi0   "	;  5 * 16
			dd	0,BootMdl
			db	"decode0 "	;  6 * 16
			dd	0,BootMdl
			db	"fdcdrv0 "	;  7 * 16
			dd	0,BootMdl
			db	"bootseg "	;  8 * 16
			dd	BootSiz,BootMdl
			db	"stack000"	;  9 * 16
stackseg		dd	offset StackSiz,StackMdl
			db	"pioneer0"	; 10 * 16
			dd	0,BootMdl
			db	"winman0 "	; 11 * 16
			dd	0,BootMdl
			db	"pokon0  "	; 12 * 16
			dd	0,BootMdl
			db	"vesadrv0"	; 13 * 16
vesadrv_sizadr		dd	0,BootMdl
			db	"ankfont0"	; 14 * 16
			dd	0,BootMdl
			db	"papi0   "	; 15 * 16
			dd	0,BootMdl

loaded_modules		equ	($ - modulelist) / 16

alloclist		db	"pdepte  "	; 0
			dd	4096*8,-1
			db	"idtgdt  "
			dd	4096,-1	; 48+463entry
			db	"fontbuf "
			dd	4096,-1
			db	"stack   "
			dd	4096*4,-1
			db	"keydata "	; 4
			dd	4096,-1
			db	"gapidata"
			dd	4096*3,-1   ; 起動最低限しか用意しない
			db	"timerdat"
			dd	4096*2,-1
			db	"tapiwork"
			dd	0,0 ; for 31 tasks(init, idle, winman0, pokon0).
			db	"decodata"	; 8
			dd	4096*6,-1
			db	"fdcwork "
			dd	4096,-1
			db	"papiwork"
			dd	4096*8,-1

		;	dd	"_shell  ",offset ShellSiz,  ShellMdl

			dd	0

			align	4

bootmalloc_adr0		dd	0
bootmalloc_fre0		dd	0
bootmalloc_adr1		dd	?	; LastMdlを指す
bootmalloc_fre1		dd	?	; 640KB - bootmalloc_adr1

FD_motor_init		db	01ch ; motor on
FD_cache_init		db	1 ; must init
DiskCacheReady		db	0
	; bit0 : cache enable, bit1:boot from OSASK boot-sector
			db	?
FD_debug		dd	-1

SysWorkSiz		equ	$-SysWorkBgn

SysWorkMdl		ends

StackMdl		segment	para use32 stack

StackBgn		equ	$

			dd	64 dup (?)	; 256bytes

StackSiz		equ	$-StackBgn

StackMdl		ends

			end	Entry
END
