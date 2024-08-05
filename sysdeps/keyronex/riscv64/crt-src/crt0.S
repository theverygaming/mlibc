
.section .text
.global _start
_start:
	la a0, main
	call __mlibc_entry

.section .note.GNU-stack,"",%progbits
