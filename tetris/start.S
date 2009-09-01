.global _start
_start:
	/* Asetetaan pino */
	movhi	sp, %hi(stack + (4*1024))
	ori	sp, sp, %lo(stack + (4*1024))
	
	call	main
	
	/* jos mainista palataan... */
afterlife:
	br	afterlife

.end
