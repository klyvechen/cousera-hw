## Program to check the character in the given string
	
	.data
user_input: 	.space 50
cmp_str: 	.space 50
success_msg:	.asciiz "Success! Location: "
fail_msg:	.asciiz "Fail!"

        .text
        .globl  main
main:

init:					# init the program, read the string from user input
	li 	$v0, 8
	la	$a0, user_input
	li 	$a1, 40
	syscall
	la	$a1, cmp_str		# save the user input to cmp_str
	jal	strcpy
	sll	$0, $0, 0

read_chr:
	li 	$v0, 8			# start to read the finding string
	la	$a0, user_input
	li 	$a1, 40
	syscall
	lb	$t0, 0x0($a0)
	move	$a0, $t0
	beq  	$a0, 0x3F, program_end	# exit the program if the character is ?
	la	$t1, cmp_str		# assign t1 as the cmp_str
	li	$t4, 0

do_cmp:
	lb	$t2, 0($t1)
	beqz	$t2, find_fail
	addiu	$t4, $t4, 1
	beq	$t0, $t2 find_success
	addiu	$t1, $t1, 1
	j	do_cmp
	sll	$0, $0, 0

find_success:
	la	$a0, success_msg
	move	$a1, $t4
	jal	print_result
	sll	$0, $0, 0
	j	read_chr
	
find_fail:
	la	$a0, fail_msg
	li	$a1, 0
	jal	print_result
	sll	$0, $0, 0
	j	read_chr

###
#  $a0, the address of string format (type address)
#  $a1, the result param1 (type number)
###
print_result: 
	li	$v0, 0x4		# print the program result
	syscall
	beqz	$a1, print_end		# skip to print the param if $a1 is zero
	move	$a0, $a1
	li	$v0, 1			# print the integer
	syscall
	sll	$0, $0, 0
print_end:
	li	$a0, 0xA
	li	$v0, 11			# print line end charactor
	syscall
	jr 	$ra
	sll	$0, $0, 0
			
####
# copy string start
# copy head of string from $a0 to $a1
# $a0 the address of the source string
# $a1 the address of the destination string
####						
strcpy:
	addiu	$sp, $sp, -8		# store the stack status
	sw	$a0, 0($sp)
	sw	$a1, 4($sp)
	move	$t1, $a0
	
strcpy_bng:
	lb	$t0, 0($t1)		# move the charater to the assigned location
	beqz	$t0, strcpy_end
	sb	$t0, 0($a1)
	addiu	$t1, $t1, 1
	addiu	$a1, $a1, 1
	j	strcpy_bng
	sll	$0, $0, 0
	
strcpy_end:
	lw	$a0, 0($sp)		# process the end of the string copy program
	lw	$a1, 4($sp)
	addiu	$sp, $sp, 8
	jr	$ra
	sll	$0, $0, 0
####
# copy string end
####

program_end:

## End of file
