.data
P1_PATH:    .asciiz "BinarySearch.asm"
P2_PATH:    .asciiz "LinearSearch.asm"
P3_PATH:    .asciiz "Collatz.asm"

.text
.globl main
main:
    li		$v0, 18		# FORK
    syscall
    move 	$s0, $v0	# $s0 = Child0 PID
    beq		$s0, $zero, child_0	# if $s0 == $zero then child_0
    li		$v0, 18		# FORK
    syscall
    move 	$s1, $v0	# $s1 = Child1 PID
    beq		$s1, $zero, child_1	# if $s1 == $zero then child_1
    li		$v0, 18		# FORK
    syscall
    move 	$s2, $v0	# $s2 = Child2 PID
    beq		$s2, $zero, child_2	# if $s2 == $zero then child_2

    li		$a0, -1		# $a0 = -1
    li		$v0, 19		# WAITPID
    syscall

exit:
    li		$v0, 10		# EXIT
    syscall

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