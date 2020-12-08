.data
ncnt:   .word   10
nums:   .word   23, 91, 16, 12, 72, 5, 8, 56, 2, 38

# s0 = Count of the numbers
# s1 = All numbers beginning address
# s2 = Current beginning address
# s3 = Current address
# s5 = Current array count
# s6 = Minimum address
# s7 = Minimum number

.text
main:
    la		$s0, ncnt
    lw		$s0, 0($s0)
    la		$s1, nums
    addi	$s2, $s1, -4	# $s2 = $s1 + -4
    addi		$s5, $s0, -1		# $s5 = $s0 + -1
    main_loop:
        blt		$s5, $zero, exit	# if $s5 < $zero then exit
        addi	$s2, $s2, 4		# $s2 = $s2 + 4
        move 	$s3, $s2		# $s3 = $s2
        move 	$s6, $s2		# $s6 = $s2
        lw      $s7, 0($s6)
        jal		search_min		# jump to search_min and save position to $ra
        jal		print_num
        lw		$t0, 0($s2)
        sw		$s7, 0($s2)
        sw		$t0, 0($s6)
        jal     print_arr
        addi	$s5, $s5, -1	# $s5 = $s5 + -1
        j		main_loop		# jump to main_loop

search_min:
    li		$t1, 0		# $t1 = 0
    search_loop:
        bgt		$t1, $s5, exit_search	# if $t1 > $s5 then exit_search
        lw		$t2, 0($s3)
        bgt		$t2, $s7, search_cont	# if $t2 > $s7 then search_cont
        move 	$s6, $s3		# $s6 = $s3
        move 	$s7, $t2		# $s7 = $t2
        search_cont:
            addi	$s3, $s3, 4			# $s3 = $s3 + 4
            addi	$t1, $t1, 1			# $t1 = $t1 + 1
            j		search_loop				# jump to search_loop
    exit_search:
        jr		$ra					# jump to $ra

print_num:
    move 	$a0, $s7	# $a0 = $s7
    li		$v0, 1		# $v0 = 1
    syscall
    li		$a0, 10		# $a0 = 10
    li		$v0, 11     # $v0 = 11
    syscall
    jr		$ra			# jump to $ra

print_arr:
    addi 	$t0, $s1, -4	# $t0 = $s1 + -4
    li		$t2, 0		# $t2 = 0
    print_loop:
        addi	$t2, $t2, 1		# $t2 = $t2 + 1
        bgt		$t2, $s0, print_exit	# if $t2 >= $s0 then print_exit
        addi	$t0, $t0, 4		# $t0 = $t0 + 4
        lw		$t1, 0($t0)
        move 	$a0, $t1	# $a0 = $t1
        li		$v0, 1		# $v0 = 1
        syscall
        li		$a0, 32		# $a0 = 32
        li		$v0, 11     # $v0 = 11
        syscall
        j		print_loop				# jump to print_loop
    print_exit:
        li		$a0, 10		# $a0 = 10
        li		$v0, 11     # $v0 = 11
        syscall
        jr		$ra			# jump to $ra

exit:
    li		$v0, 10		# $v0 = 10
    syscall