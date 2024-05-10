.global _start
_start:
	lea (_GLOBAL_OFFSET_TABLE_@GOTPC,%pc), %a0
	lea (_DYNAMIC,%pc), %a1

	# get ld base address
	movel	%a1, %a5
	subl	(%a0), %a5

	# push ld base address
	movel %a5, -(%sp)
	# push pointer to _DYNAMIC
	movel %a1, -(%sp)
	# relocate self
	jbsr relocateSelf68k@PLTPC
	# drop args
	addql #8, %sp

	move.l %sp, -(%sp)
	jbsr interpreterMain@PLTPC

	# finally _start
	jmp (%a0)

.section .note.GNU-stack,"",%progbits
