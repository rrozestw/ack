/ $Header$
.text
.globl lar~
.globl save1~

lar~:
	mov	(sp)+,save1~
	sub	(r0),r1
	mov	04(r0),r0
	mul	r0,r1
	add	(sp)+,r1
	add	r0,r1
	asr	r0
	beq	1f
2:	mov	-(r1),-(sp)
	sob	r0,2b
	jmp	*save1~
1:	clr	r0
	bisb	-(r1),r0
	mov	r0,-(sp)
	jmp	*save1~
