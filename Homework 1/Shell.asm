.data
cmd:    .space  256
hll:    .asciiz "OZAN-OS> "

# s0 = 'e'
# s1 = 'x'
# s2 = '\n'

.text
.globl  main
.ent    main
main:
    li		$s0, 101		# $s0 = 101
    li		$s1, 120		# $s1 = 120
    li		$s2, 10		# $s2 = 10
    jal		get_command
    jal		is_exit
    li		$v0, 18		# $v0 = 18
    syscall
    j		main

get_command:
    li		$v0, 4		# $v0 = 4
    la		$a0, hll
    syscall
    li		$v0, 8		# $v0 = 8
    la   	$a0, cmd	# $a0 = cmd
    syscall
    jr		$ra			# jump to $ra

is_exit:
    lb		$t0, 0($a0)
    lb		$t1, 1($a0)
    lb		$t2, 2($a0)
    bne		$t0, $s0, is_exit_return	# if $t0 != $s0 then is_exit_return
    bne		$t1, $s1, is_exit_return	# if $t1 != $s1 then is_exit_return
    bne		$t2, $s2, is_exit_return	# if $t2 != $s2 then is_exit_return
    jal     exit
    is_exit_return:
        jr		$ra					# jump to $ra

exit:
    li		$v0, 10		# $v0 = 10
    syscall