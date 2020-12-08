.data
nums: .word 1, 5, 6, 19, 33, 37, 49, 53, 67, 69, 81, 87, 192
ncnt: .word 13
wntd: .word 200
#ncnt:   .word   10
#nums:   .word   1, 3, 4, 6, 7, 8, 10, 11, 14, 15
#wntd:   .word   3
#nums:   .word   2, 5, 8, 12, 16, 23, 38, 56, 72, 91
#wntd:   .word   91

# s0 = Count of the numbers
# s1 = Numbers array address
# s2 = Wanted number
# s3 = 2 for divisions
# s4 = Move number
# s5 = Current numbers index
# s6 = Current number address
# s7 = Current number
# t0 = 4 for multiplications
# t1 = 1 for BEQ
# t2 = Old move

.text
main:
    li		$t0, 4		# $t0 = 4
    li		$t1, 1		# $t1 = 1
    la		$s0, ncnt
    lw		$s0, 0($s0)
    la		$s1, nums
    la		$s2, wntd
    lw		$s2, 0($s2)
    li		$s3, 2		# $s3 = 2

    addi	$s4, $s0, -1	# $s4 = $s0 + -1
    div		$s4, $s3	# $s4 / 2
    mflo	$s4			# $s4 = floor($s0 / 2)
    move 	$s5, $s4	# $s5 = $s4
    loop:
        bge		$s5, $s0, high_idx	# if $s5 >= $s0 then high_idx
        blt		$s5, $zero, small_idx	# if $s5 < $zero then small_idx
        move 	$t2, $s4	# $t2 = $s4
        jal		calculate_addr	# jump to calculate_addr and save position to $ra
        lw		$s7, 0($s6) # calculate the current number
        beq		$s7, $s2, exit	# if $s7 == $s2 then exit
        bge		$t2, $t1, cont_secure	# if $t2 >= $t1 then cont_secure
        ble		$s4, $t1, not_found	# if $s4 <= $t1 then not_found
        cont_secure:
            bgt		$s7, $s2, decrease	# if $s7 > $s2 then decrease
            blt		$s7, $s2, increase	# if $s7 < $s2 then increase
    decrease:
        sub		$s5, $s5, $s4	# $s5 = $s5 - $s4
        addi	$s5, $s5, -1	# $s5 = $s5 + -1
        j		loop			# jump to loop
    increase:
        add		$s5, $s5, $s4	# $s5 = $s5 + $s4
        addi	$s5, $s5, 1	    # $s5 = $s5 + 1
        j		loop			# jump to loop
    small_idx:
        move 	$s5, $zero		# $s5 = $zero
        j		loop			# jump to loop
    high_idx:
        addi	$s5, $s0, -1	# $s5 = $s0 + -1
        j		loop			# jump to loop

calculate_addr:
    div		$s4, $s3	# $s4 / 2
    mflo	$s4			# $s4 = floor($s0 / 2)
    blt		$s4, $zero, small_move	# if $s4 < $zero then small_move
    cont_cal:
        mult	$s5, $t0	# $s5 * 4 = Hi and Lo registers
        mflo	$s6			# copy $s5 * 4 to $s6
        add		$s6, $s6, $s1   # $s6 = $s6 + $s1
        jr		$ra			# jump to $ra
    small_move:
        li		$s4, 1		# $s4 = 1
        j		cont_cal	# jump to cont_cal

not_found:
    li		$s5, -1		# $s5 = -1
    jal		exit		# jump to exit and save position to $ra

exit:
    move 	$a0, $s5	# $a0 = $s5
    li		$v0, 1		# $v0 = 1
    syscall
    li		$a0, 10		# $a0 = 10
    li		$v0, 11     # $v0 = 11
    syscall
    li		$v0, 21		# EXIT
    syscall