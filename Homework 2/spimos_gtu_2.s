.data
P1_PATH:    .asciiz "BinarySearch.asm"
P2_PATH:    .asciiz "LinearSearch.asm"
P3_PATH:    .asciiz "Collatz.asm"

.text
.globl main
main:
    li		$s2, 0		# $s2 = 0
    li		$s3, 10		# $s3 = 10
    li		$a0, 3
    li		$v0, 22		# RANDOM
    syscall
    move 	$s1, $v0	# $s1 = $v0

    forks:
    beq		$s2, $s3, waits	# if $s2 == 10 then waits
    li		$v0, 18		# FORK
    syscall
    beq		$v0, $zero, child	# if $v0 == $zero then child
    addi	$s2, $s2, 1			# $s2 = $s2 + 1
    j		forks				# jump to forks

waits:
    li		$a0, -1		# $sa0, -1
    li		$v0, 19		# WAITPID
    syscall

exit:
    li		$v0, 10		# EXIT
    syscall

child:
    li		$t0, 0		# $t0 = 0
    li		$t1, 1		# $t1 = 1
    beq		$s1, $t0, child_0	# if $s1 == $t0 then child_0
    beq		$s1, $t1, child_1	# if $s1 == $t1 then child_1
    j		child_2				# jump to child_2


child_0:
    la		$a0, P1_PATH
    li		$v0, 20		# EXECVE
    syscall

child_1:
    la		$a0, P2_PATH
    li		$v0, 20		# EXECVE
    syscall

child_2:
    la		$a0, P3_PATH
    li		$v0, 20		# EXECVE
    syscall