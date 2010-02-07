		.file	"gasfunc.s"
		.text

.globl start_app
start_app:
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

.globl asm_api
asm_api:
		pushal
		call	c_api
		popal
		ret

.globl asm_end
asm_end:
		xorl	%edi,%edi
		call	asm_api
		.byte	0x40
