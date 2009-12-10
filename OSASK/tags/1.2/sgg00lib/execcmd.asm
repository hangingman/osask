.486p
.model	flat
.code

_sgg_execcmd		proc	near

			push	ebx
			mov	ebx,[esp+8]
			db	09ah
			dd	0
			dw	000cfh
			pop	ebx
			ret

_sgg_execcmd		endp

			end
