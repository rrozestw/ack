.define _fork
.sect .text
.sect .rom
.sect .data
.sect .bss
.sect .text
.extern _fork
_fork:		trap #0
.data2 0x2
		bra 1f
		bcs 2f
		rts
2:
		jmp cerror
1:
		bcs	2b
		clr.l	d0
		rts
