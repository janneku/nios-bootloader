/*
 * void write_irq_mask(uint32_t mask);
 */
.global write_irq_mask
write_irq_mask:
	wrctl	ienable, r4
	ret

/*
 * void enable_irqs();
 */
.global enable_irqs
enable_irqs:
	movui	r8, 1
	wrctl	status, r8
	ret

/*
 * void disable_irqs();
 */
.global disable_irqs
disable_irqs:
	wrctl	status, zero
	ret

/*
 * void flush_icache(uint32_t addr);
 */
.global flush_icache
flush_icache:
	flushi	r4
	ret

/*
 * void flush_dcache(uint32_t addr);
 */
.global flush_dcache
flush_dcache:
	flushd	0(r4)
	ret

/*
 * Exception wrapper.
 * Tallentaa rekisterit ja hypp‰‰ C-kieliseen k‰sittelij‰‰n
 */
.global excp_wrapper
.set noat
excp_wrapper:
	/* Kaikkia rekistereit‰ ei tarvitse tallentaa, C-k‰‰nt‰j‰
	 * huolehtii muiden tallennuksesta jos niit‰ k‰ytet‰‰n.
         */
	addi	sp, sp, -64
	stw	ra, 0(sp)
	stw	r1, 4(sp)
	stw	r2, 8(sp)
	stw	r3, 12(sp)
	stw	r4, 16(sp)
	stw	r5, 20(sp)
	stw	r6, 24(sp)
	stw	r7, 28(sp)
	stw	r8, 32(sp)
	stw	r9, 36(sp)
	stw	r10, 40(sp)
	stw	r11, 44(sp)
	stw	r12, 48(sp)
	stw	r13, 52(sp)
	stw	r14, 56(sp)
	stw	r15, 60(sp)
	
	rdctl	r4, ipending
	call	exception_handler
	
	/* palautetaan rekisterit */
	ldw	ra, 0(sp)
	ldw	r1, 4(sp)
	ldw	r2, 8(sp)
	ldw	r3, 12(sp)
	ldw	r4, 16(sp)
	ldw	r5, 20(sp)
	ldw	r6, 24(sp)
	ldw	r7, 28(sp)
	ldw	r8, 32(sp)
	ldw	r9, 36(sp)
	ldw	r10, 40(sp)
	ldw	r11, 44(sp)
	ldw	r12, 48(sp)
	ldw	r13, 52(sp)
	ldw	r14, 56(sp)
	ldw	r15, 60(sp)
	addi	sp, sp, 64
	
	addi	ea, ea, -4
	eret

.end
