.set noreorder
.section .text

.global InitPAD
.type InitPAD, @function
InitPAD:
	addiu	$t2, $0 , 0xb0
	jr		$t2
	addiu	$t1, $0 , 0x12
	