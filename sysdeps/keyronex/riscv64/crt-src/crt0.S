
.section .text
.global _start
_start:
	.weak __global_pointer$
	.hidden __global_pointer$
	.option push
	.option norelax
	lla gp, __global_pointer$
	.option pop

	la a0, main
	call __mlibc_entry

.section .note.GNU-stack,"",%progbits
