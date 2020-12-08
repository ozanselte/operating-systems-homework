.data
P1_PATH:    .asciiz "BinarySearch.asm"
P2_PATH:    .asciiz "LinearSearch.asm"
P3_PATH:    .asciiz "Collatz.asm"

.text
.globl main
main:
    li		$s1, 1		# $s1 = 1 
    li		$s2, 0		# $s2 = 0
    li		$s3, 3  	# $s3 = 3
    li		$a0, 3
    li		$v0, 22		# RANDOM
    syscall
    move 	$s0, $v0	# $s0 = $v0
    beq		$s0, $zero, r_1	# if $s0 == $zero then r_1
    beq		$s0, $s1, r_2	# if $s0 == $s1 then r_2
    la		$t0, P1_PATH
    la		$t1, P2_PATH
    j		forks			# jump to forks
    r_1:
    la		$t0, P2_PATH
    la		$t1, P3_PATH
    j		forks			# jump to forks
    r_2:
    la		$t0, P1_PATH
    la		$t1, P3_PATH
    j		forks			# jump to forks

    forks:
    beq		$s2, $s3, waits	# if $s2 == 10 then waits
    li		$v0, 18		# FORK
    syscall
    beq		$v0, $zero, child_0	# if $s0 == $zero then child_0
    li		$v0, 18		# FORK
    syscall
    beq		$v0, $zero, child_1	# if $s0 == $zero then child_1
    addi	$s2, $s2, 1			# $s2 = $s2 + 1
    j		forks				# jump to forks

waits:
    li		$a0, -1		# $sa0, -1
    li		$v0, 19		# WAITPID
    syscall

exit:
    li		$v0, 10		# EXIT
    syscall

child_0:
    move 	$a0, $t0	# $a0 = $t0
    li		$v0, 20		# EXECVE
    syscall

child_1:
    move 	$a0, $t1	# $a0 = $t1
    li		$v0, 20		# EXECVE
    syscall