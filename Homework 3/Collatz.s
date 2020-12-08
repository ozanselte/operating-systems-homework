.data
UPLMT:   .word   25

# s0 = Upper limit
# s1 = Current number
# s2 = 2
# s3 = 3
# s4 = Temp number
# s5 = Temp / 2
# s6 = Temp % 2
# s7 = 1

.text
.globl main
main:
    la		$s0, UPLMT
    lw		$s0, 0($s0)
    li		$s1, 1		# $s1 = 1
    li		$s2, 2		# $s2 = 2
    li		$s3, 3		# $s3 = 3
    li		$s7, 1		# $s7 = 1
    main_loop:
        addi	$s1, $s1, 1		    # $s1 = $s1 + 1
        bgt		$s1, $s0, exit	    # if $s1 >= $s0 then exit
        jal		print_start			# jump to print_start and save position to $ra
        move 	$s4, $s1	        # $s4 = $s1
    inner_loop:
        div		$s4, $s2			# $s4 / $s2
        mflo	$s5					# $s5 = floor($s4 / $s2) 
        mfhi	$s6					# $s6 = $s4 mod $s2 
        bne		$s6, $zero, odd_num	# if $s6 != $zero then odd_num
        move 	$s4, $s5    		# $s4 = $s5
        inner_continue:
            jal		print_num			# jump to print_num and save position to $ra
            beq		$s4, $s7, main_loop	# if $s4 == $s7 then main_loop
            j		inner_loop		    # jump to inner_loop
        odd_num:
            mult	$s4, $s3			# $s4 * $s3 = Hi and Lo registers
            mflo	$s4					# copy Lo to $s4
            addi	$s4, $s4, 1			# $s4 = $s4 + 1
            j		inner_continue		# jump to inner_continue

print_start:
    li		$a0, 10		# $a0 = 10
    li		$v0, 11     # $v0 = 11
    syscall
    move 	$a0, $s1	# $a0 = $s1
    li		$v0, 1		# $v0 = 1
    syscall
    li		$a0, 58		# $a0 = 58
    li		$v0, 11     # $v0 = 11
    syscall
    li		$a0, 32		# $a0 = 32
    li		$v0, 11     # $v0 = 11
    syscall
    jr		$ra			# jump to $ra

print_num:
    move 	$a0, $s4	# $a0 = $s4
    li		$v0, 1		# $v0 = 1
    syscall
    li		$a0, 32		# $a0 = 32
    li		$v0, 11     # $v0 = 11
    syscall
    jr		$ra			# jump to $ra

exit:
    li		$a0, 10		# $a0 = 10
    li		$v0, 11     # $v0 = 11
    syscall
    li		$v0, 21		# EXIT
    syscall