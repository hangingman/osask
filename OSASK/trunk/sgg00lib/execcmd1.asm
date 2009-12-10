.486p
.model	flat
.code

;const int sgg_execcmd1(const int ret, const int cmd, ...)
;	ret = n * 4 + 12

_sgg_execcmd1		proc	near

			push	ebx
			lea	ebx,[esp+12]	; cmd
			db	09ah
			dd	0
			dw	000cfh
			mov	eax,[esp+8]	; ret
			mov	eax,[esp+eax]
			pop	ebx
			ret

_sgg_execcmd1		endp

			end
