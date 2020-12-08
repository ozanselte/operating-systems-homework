.data
fnum:   .word   0
enum:   .word   0
xnum:   .word   0

.text
.globl main
main:
    jal		get_numbers		# jump to get_numbers and save position to $ra
    jal		iterate_numbers	# jump to iterate_numvers and save position to $ra
    jal		exit				# jump to exit and save position to $ra

get_numbers:
    li		$v0, 5		# $v0 = 5
    syscall
    move 	$s0, $v0	# $s0 = $v0
    li		$v0, 5		# $v0 = 5
    syscall
    move 	$s1, $v0	# $s1 = $v0
    li		$v0, 5		# $v0 = 5
    syscall
    move 	$s2, $v0	# $s2 = $v0
    sw		$s0, fnum		# fnum = $s0
    sw		$s1, enum		# enum = $s1
    sw		$s2, xnum		# xnum = $s2
    jr		$ra				# jump to $ra

iterate_numbers:
    addi	$s3, $s0, -1			# $s3 = $s0 - 1
    begin_in:
        addi	$s3, $s3, 1			# $s3 = $s3 + 1
        bgt		$s3, $s1, close_in	# if $s3 > $s1 then close_in
        div		$s3, $s2			# $s3 / $s2
        mfhi	$t0					# $t0 = $s3 mod $s2 
        bne		$t0, $zero, begin_in	# if $t0 != $zero then begin_in
        move 	$a0, $s3	# $a0 = $s3
        li		$v0, 1		# $v0 = 1
        syscall
        li		$a0, 10		# $a0 = 10
        li		$v0, 11     # $v0 = 11
        syscall
        j		begin_in				# jump to begin_in
    close_in:
        jr		$ra				# jump to $ra

exit:
    li		$v0, 10		# $v0 = 10
    syscall