/ $Header$
.text
.globl ret~,lfr~,retar
.globl save1~
.globl unknown~

/ Size in r0
ret~:
	mov	r0,r1
	beq	1f
	asr	r1
	add	$retar,r0
	cmp	r0,$retend
	bhi	9f
3:	mov	(sp)+,-(r0)
	sob	r1,3b
1:	mov	r5,sp
	mov	(sp)+,r5
	rts	pc
9:	jmp	unknown~
lfr~:
	mov	(sp)+,save1~
	asr	r0
	beq	4f
	mov	$retar,r1
5:	mov	(r1)+,-(sp)
	sob	r0,5b
4:	jmp	*save1~

.data
retar:	.=.+16.
retend:
