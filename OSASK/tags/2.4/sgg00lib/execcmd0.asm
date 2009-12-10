.486p
.model	flat
.code

;void sgg_execcmd0(const int cmd, ...)

_sgg_execcmd0		proc	near

			push	ebx
			lea	ebx,[esp+8]	; cmd
			db	09ah
			dd	0
			dw	000cfh
			pop	ebx
			ret

_sgg_execcmd0		endp

			end
