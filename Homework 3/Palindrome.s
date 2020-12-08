.data
key0:	.asciiz	"wistful"
key1:	.asciiz	"sort"
key2:	.asciiz	"tranquil"
key3:	.asciiz	"exchange"
key4:	.asciiz	"disagreeable"
key5:	.asciiz	"elite"
key6:	.asciiz	"colorful"
key7:	.asciiz	"grouchy"
key8:	.asciiz	"cynical"
key9:	.asciiz	"employ"
key10:	.asciiz	"pack"
key11:	.asciiz	"hollow"
key12:	.asciiz	"nosy"
key13:	.asciiz	"barbarous"
key14:	.asciiz	"massive"
key15:	.asciiz	"swift"
key16:	.asciiz	"remarkable"
key17:	.asciiz	"aquatic"
key18:	.asciiz	"wide"
key19:	.asciiz	"examine"
key20:	.asciiz	"giants"
key21:	.asciiz	"fact"
key22:	.asciiz	"determined"
key23:	.asciiz	"title"
key24:	.asciiz	"heat"
key25:	.asciiz	"mitten"
key26:	.asciiz	"well-to-do"
key27:	.asciiz	"tour"
key28:	.asciiz	"stitch"
key29:	.asciiz	"waiting"
key30:	.asciiz	"vacation"
key31:	.asciiz	"lamentable"
key32:	.asciiz	"tree"
key33:	.asciiz	"root"
key34:	.asciiz	"ritzy"
key35:	.asciiz	"view"
key36:	.asciiz	"reviver"
key37:	.asciiz	"charge"
key38:	.asciiz	"plane"
key39:	.asciiz	"poison"
key40:	.asciiz	"hallah"
key41:	.asciiz	"aibohphobia"
key42:	.asciiz	"putup"
key43:	.asciiz	"digestion"
key44:	.asciiz	"sweet"
key45:	.asciiz	"sense"
key46:	.asciiz	"mourn"
key47:	.asciiz	"color"
key48:	.asciiz	"annoyed"
key49:	.asciiz	"marram"
key50:	.asciiz	"wish"
key51:	.asciiz	"windy"
key52:	.asciiz	"comb"
key53:	.asciiz	"rose"
key54:	.asciiz	"puzzled"
key55:	.asciiz	"argue"
key56:	.asciiz	"alula"
key57:	.asciiz	"black"
key58:	.asciiz	"truthful"
key59:	.asciiz	"magic"
key60:	.asciiz	"late"
key61:	.asciiz	"straight"
key62:	.asciiz	"men"
key63:	.asciiz	"yummy"
key64:	.asciiz	"brief"
key65:	.asciiz	"sparkling"
key66:	.asciiz	"Nauruan"
key67:	.asciiz	"anger"
key68:	.asciiz	"rough"
key69:	.asciiz	"scent"
key70:	.asciiz	"girl"
key71:	.asciiz	"lush"
key72:	.asciiz	"denned"
key73:	.asciiz	"handy"
key74:	.asciiz	"use"
key75:	.asciiz	"settle"
key76:	.asciiz	"distance"
key77:	.asciiz	"tart"
key78:	.asciiz	"future"
key79:	.asciiz	"escape"
key80:	.asciiz	"fork"
key81:	.asciiz	"cast"
key82:	.asciiz	"sock"
key83:	.asciiz	"ice"
key84:	.asciiz	"obsequious"
key85:	.asciiz	"direction"
key86:	.asciiz	"cloudy"
key87:	.asciiz	"Sinis"
key88:	.asciiz	"cluttered"
key89:	.asciiz	"tired"
key90:	.asciiz	"stats"
key91:	.asciiz	"manage"
key92:	.asciiz	"allow"
key93:	.asciiz	"satisfying"
key94:	.asciiz	"tangy"
key95:	.asciiz	"tested"
key96:	.asciiz	"aboard"
key97:	.asciiz	"six"
key98:	.asciiz	"friend"
key99:	.asciiz	"gold"
keys:	.word	key0, key1, key2, key3, key4, key5, key6, key7, key8, key9, key10, key11, key12, key13, key14, key15, key16, key17, key18, key19, key20, key21, key22, key23, key24, key25, key26, key27, key28, key29, key30, key31, key32, key33, key34, key35, key36, key37, key38, key39, key40, key41, key42, key43, key44, key45, key46, key47, key48, key49, key50, key51, key52, key53, key54, key55, key56, key57, key58, key59, key60, key61, key62, key63, key64, key65, key66, key67, key68, key69, key70, key71, key72, key73, key74, key75, key76, key77, key78, key79, key80, key81, key82, key83, key84, key85, key86, key87, key88, key89, key90, key91, key92, key93, key94, key95, key96, key97, key98, key99
pal:	.asciiz	": Palindrome"
no_pal:	.asciiz	": Not Palindrome"
ask_q:	.asciiz	"Do you want to continue (y/n)?\n"
ask_p:	.asciiz	"\nPlease enter the last word:\n"
g_bye:	.asciiz	"Goodbye..."
n_word:	.space	256

# s0 = Current word's index
# s1 = Count of the words
# s2 = Current word's address
# s3 = Word beginning
# s4 = Word length
# s5 = Word end
# s6 = 90 Z
# s7 = 65 A
# t8 = 10 \n

.text
.globl main
main:
	li		$s0, 0		# $s0 = 0
	li		$s1, 100	# $s1 = 100
	la		$s2, keys
	li		$s6, 90		# $s6 = 90
	li		$s7, 65		# $s7 = 65
	li		$t8, 10		# $t8 = 10
	cont:
		lw		$s3, 0($s2)
		jal		print_word			# jump to print_word and save position to $ra
		jal		process_word		# jump to process_word and save position to $ra
		jal		print_pal			# jump to print_pal and save position to $ra
		jal		print_nl			# jump to print_nl and save position to $ra
		addi	$s0, $s0, 1			# $s0 = $s0 + 1
		addi	$s2, $s2, 4			# $s2 = $s2 + 4
		blt		$s0, $s1, cont		# if $s0 < $s1 then cont
	jal		ask_user				# jump to ask_user and save position to $ra

process_word:
	li		$s4, 0			# $s4 = 0
	move 	$s5, $s3		# $s5 = $s3
	pw_cond:
		lb		$t0, 0($s5)
		blt		$t0, $s7, lowercase	# if $t0 < A then lowercase
		bgt		$t0, $s6, lowercase	# if $t0 > Z then lowercase
		addi	$t0, $t0, 32		# $t0 = $t0 + 32
		sb		$t0, 0($s5)
		lowercase:
			addi	$s4, $s4, 1			# $s4 = $s4 + 1
			addi	$s5, $s5, 1			# $s5 = $s5 + 1
			bne		$t0, $zero, pw_cond	# if $t0 != $zero then pw_cond
	addi	$s5, $s5, -2			# $s5 = $s5 - 2
	srl		$s4, $s4, 1
	pal_loop:
		move 	$t2, $zero			# $t2 = $zero
		beq		$s4, $zero, pal_end	# if $s4 == $zero then target
		lb		$t0, 0($s3)
		lb		$t1, 0($s5)
		addi	$s3, $s3, 1			# $s3 = $s3 + 1
		addi	$s5, $s5, -1		# $s5 = $s5 - 1
		addi	$s4, $s4, -1		# $s4 = $s4 - 1
		li		$t2, 1				# $t2 = 1
		beq		$t0, $t1, pal_loop	# if $t0 == $t1 then pal_loop
	pal_end:
		jr		$ra				# jump to $ra

ask_user:
	la		$a0, ask_q
	li		$v0, 4		# $v0 = 4
    syscall
	li		$v0, 12		# $v0 = 12
    syscall
	li		$v1, 121	# $v1 = 121
	bne		$v0, $v1, ask_exit	# if $v0 != $v1 then ask_exit
	la		$a0, ask_p
	li		$v0, 4		# $v0 = 4
    syscall
	la		$a0, n_word
	li		$v0, 8		# $v0 = 8
    syscall
	ask_loop:
		lb		$t0, 0($a0)
		bne		$t0, $t8, no_lf		# if $t0 != $t8 then no_lf
		li		$t0, 0		# $t0 = 0
		sb		$t0, 0($a0)
		
	no_lf:
		addi	$a0, $a0, 1				# $a0 = $a0 + 1
		bne		$t0, $zero, ask_loop	# if $t0 != $zero then ask_loop
	la		$s3, n_word
	jal		print_word			# jump to print_word and save position to $ra
	jal		process_word		# jump to process_word and save position to $ra
	jal		print_pal			# jump to print_pal and save position to $ra
	jal		print_nl			# jump to print_nl and save position to $ra
	ask_exit:
		jal		exit					# jump to exit and save position to $ra

print_pal:
	la		$a0, pal
	beq		$t2, $zero, plnd	# if $t2 == $zero then plnd
	la		$a0, no_pal
	plnd:
	li		$v0, 4		# $v0 = 4
    syscall
	jr		$ra			# jump to $ra

print_word:
	addi	$a0, $s0, 1			# $a0 = $s0 + 1
	li		$v0, 1		# $v0 = 1
	syscall
	li		$a0, 58		# $a0 = 58
    li		$v0, 11     # $v0 = 11
    syscall
	li		$a0, 32		# $a0 = 32
    li		$v0, 11     # $v0 = 11
    syscall
    move 	$a0, $s3	# $a0 = $s3
    li		$v0, 4		# $v0 = 4
    syscall
	jr		$ra			# jump to $ra

print_nl:
	li		$a0, 10		# $a0 = 10
    li		$v0, 11     # $v0 = 11
    syscall
    jr		$ra			# jump to $ra

exit:
    li		$v0, 10		# EXIT
    syscall