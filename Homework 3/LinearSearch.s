.data
ncnt:   .word   10
nums:   .word   10, 20, 80, 30, 60, 50, 110, 100, 130, 170
wntd:   .word   110

# s0 = Count of the numbers
# s1 = Numbers array address
# s2 = Wanted number
# s3 = Current number index
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
    li		$s3, 0, -1		# $s3 = -1
    addi	$s4, $s1, -4	# $s4 = $s1 + -4
    loop:
        addi	$s4, $s4, 4	# $s4 = $s4 + 4
        addi	$s3, $s3, 1	# $s3 = $s3 + 1
        bge		$s3, $s0, not_found	# if $s3 >= $s0 then not_found
        lw		$s5, 0($s4)     # load the current number
        beq		$s5, $s2, exit	# if $s5 == $s2 then exit
        j		loop			# jump to loop
    not_found:
        li		$s3, -1		# $s3 = -1
        jal		exit		# jump to exit and save position to $ra

exit:
    move 	$a0, $s3	# $a0 = $s3
    li		$v0, 1		# $v0 = 1
    syscall
    li		$a0, 10		# $a0 = 10
    li		$v0, 11     # $v0 = 11
    syscall
    li		$v0, 21		# EXIT
    syscall