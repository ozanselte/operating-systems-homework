.data
ncnt:   .word   10
nums:   .word   23, 91, 16, 12, 72, 5, 8, 56, 2, 38
wntd:   .word   72

# s0 = Count of the numbers
# s1 = Numbers array address
# s2 = Wanted number
# s4 = Current number address
# s5 = Current number

.text
.globl main
main:
    la		$s0, ncnt
    lw		$s0, 0($s0)
    la		$s1, nums
    la		$s2, wntd
    lw		$s2, 0($s2)
    addi	$s4, $s1, -4	# $s4 = $s1 + -4
    loop:
        addi	$s4, $s4, 4	# $s4 = $s4 + 4
        lw		$s5, 0($s4)     # load the current number
        jal		print_num	    # jump to print_num and save position to $ra
        beq		$s5, $s2, exit	# if $s7 == $s2 then exit
        j		loop			# jump to loop

print_num:
    move 	$a0, $s5	# $a0 = $s5
    li		$v0, 1		# $v0 = 1
    syscall
    li		$a0, 10		# $a0 = 10
    li		$v0, 11     # $v0 = 11
    syscall
    jr		$ra			# jump to $ra

exit:
    li		$v0, 10		# $v0 = 10
    syscall