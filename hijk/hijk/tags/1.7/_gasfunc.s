		.file	"_gasfunc.s"
		.text

.globl _start_app
_start_app:
		movl	4(%esp),%esi
		movl	20(%esi),%eax
		leal	-4(%eax),%esp
		movl	4(%esi),%eax
		movl	8(%esi),%ecx
		movl	12(%esi),%edx
		movl	16(%esi),%ebx
		movl	24(%esi),%ebp
		movl	32(%esi),%edi
		ret

.globl _asm_api
_asm_api:
		pushal
		call	_c_api
		popal
		ret

.globl _asm_end
_asm_end:
		xorl	%edi,%edi
		call	_asm_api
		.byte	0x40
