/ $Header$
.globl xor~
.globl save~,retu~

xor~:
	jsr	pc,save~
	mov	sp,r1
	add	r0,r1
	asr	r0
1:
	mov	(sp)+,r2
	xor	r2,(r1)+
	sob	r0,1b
	jmp	retu~
