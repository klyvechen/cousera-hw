## Program to show the character mapping
        .text
        .globl  main
main:

read:		
	li 	$v0, 8		# store the user input to address of user_input
	la	$a0, user_input
	li 	$a1, 40
	syscall
	lb	$t0, 0x0($a0)	# load the first charater of address input to $a0
	move	$a0, $t0
	
deal_chr:			
	beq  	$a0, 0x3F, program_end		# load input into $a0
	ble 	$a0, 0x2F, handle_not_in_rgn
	ble 	$a0, 0x39, handle_int_input
	ble 	$a0, 0x40, handle_not_in_rgn
	ble 	$a0, 0x5A, handle_big_char
	ble 	$a0, 0x60, handle_not_in_rgn
	ble 	$a0, 0x7A, handle_sml_char	
	j	handle_not_in_rgn		# else goto handle not in region
	sll	$0, $0, 0

handle_int_input:
	subi 	$a0, $a0, 0x30	# handle the number input character
	sll	$a0, $a0, 2
	la	$a1, num_array
	j	handle_input
	
handle_big_char:
	subi 	$t0, $a0, 0x41	# handle the uppercase input character
	sll	$t0, $t0, 2	# calculate the index of the target string
	la	$a1, str_array	# get the start address of string array
	add	$t0, $t0, $a1	# get the target address of the string
	lw	$a0, 0x0($t0)
	li	$a1, 0
	jal	capi_str	# Capitalized the $a1-th index of the string
	sll	$0, $0, 0
	move	$a0, $v0
	jal 	print_str_ln
	sll	$0, $0, 0
	j 	read

handle_sml_char:
	subi 	$a0, $a0, 0x61	# handle the lowercase input character
	sll	$a0, $a0, 2
	la	$a1, str_array
	j	handle_input
		
handle_not_in_rgn:		
	la	$a0, not_in	# handle the character which is not alphabat or number
	jal 	print_str_ln
	sll	$0, $0, 0
	j 	read

handle_input:		
	add	$a0, $a0, $a1	# get the target address of the string
	lw	$t0, 0x0($a0)		
	move	$a0, $t0
	jal 	print_str_ln
	sll	$0, $0, 0
	j 	read

####
# Capitalize the string
# $a0 address of the string to modify
# $a1 index of the charactor which is want to be capitalized
# $v0 the return value, the address of the modified string
####

# store register to stack
capi_str:
	addiu	$sp, $sp, -40
	sw	$a0, 0($sp)
	sw	$a1, 4($sp)
	move	$s0, $a0
	move	$s1, $a1
	sw	$ra, 8($sp)
	sw	$s0, 12($sp)
	sw	$s1, 16($sp)
	sw	$s2, 20($sp)
	sw	$s3, 24($sp)
	
# body of cap_str
	la	$a1, buffer
	jal	strcpy
	sll	$0, $0, 0
	move 	$t0, $s0
	move 	$t1, $s1
	addu	$t0, $t0, $t1
	lb	$t2, 0x0($t0)
	addiu 	$t2, $t2, -0x20
	addu 	$t3, $a1, $t1
	sb	$t2, 0x0($t3)
	move	$v0, $a1

# load back register from stack
	lw	$ra, 8($sp)
	lw	$s0, 12($sp)
	lw	$s1, 16($sp)
	lw	$s2, 20($sp)
	lw	$s3, 24($sp)			
	lw	$a0, 0($sp)
	lw	$a1, 4($sp)
	addiu	$sp, $sp, 40
	jr	$ra
	sll	$0, $0, 0

####
# copy string start
# copy head of string from $a0 to $a1
# $a0 the address of the source string
# $a1 the address of the destination string
####	

# init the string copy					
strcpy:
	addiu	$sp, $sp, -8		# store the stack status
	sw	$a0, 0($sp)
	sw	$a1, 4($sp)
	move	$t1, $a0
	
# copy the character to the second address looply
strcpy_bng:
	lb	$t0, 0($t1)		# move the charater to the assigned location
	beqz	$t0, strcpy_end
	sb	$t0, 0($a1)
	addiu	$t1, $t1, 1
	addiu	$a1, $a1, 1
	j	strcpy_bng
	sll	$0, $0, 0

# end of the string copy
strcpy_end:
	lw	$a0, 0($sp)		# process the end of the string copy program
	lw	$a1, 4($sp)
	addiu	$sp, $sp, 8
	jr	$ra
	sll	$0, $0, 0
####
# copy string end
####
																				
print_str_ln: 
	li	$v0, 0x4	# print string with line end
	syscall
	li	$a0, 0xA
	li	$v0, 11		# print line end charactor
	syscall
	jr 	$ra
	sll	$0, $0, 0

###
# End of the program
###
program_end:
			
	.data
astr:  .asciiz "alpha"		# string mapping for a b c d
bstr:  .asciiz "bravo"	
cstr:  .asciiz "china"
dstr:  .asciiz "delta"
estr:  .asciiz "echo"
fstr:  .asciiz "foxtrot"
gstr:  .asciiz "golf"
hstr:  .asciiz "hotel"
istr:  .asciiz "india"
jstr:  .asciiz "juliet"
kstr:  .asciiz "kilo"
lstr:  .asciiz "lima"
mstr:  .asciiz "mary"
nstr:  .asciiz "november"
ostr:  .asciiz "oscar"
pstr:  .asciiz "paper"
qstr:  .asciiz "quebec"
rstr:  .asciiz "research"
sstr:  .asciiz "sierra"
tstr:  .asciiz "tango"
ustr:  .asciiz "uniform"
vstr:  .asciiz "victor"
wstr:  .asciiz "whisky"
xstr:  .asciiz "x-ray"
ystr:  .asciiz "yankee"
zstr:  .asciiz "zulu"
not_in:  .asciiz "*"

one: 	.asciiz "First"		# string for one two three
two:  	.asciiz "Second"	
three:  .asciiz "Third"
four:  	.asciiz "Fourth"
five:   .asciiz "Fifth"
six:  	.asciiz "Sixth"
seven:  .asciiz "Seventh"
eight:  .asciiz "Eighth"
nine:  	.asciiz "Ninth"
zero:  	.asciiz "zero"

str_array: .word astr, bstr, cstr, dstr, estr, fstr, gstr, hstr, istr, jstr, kstr, lstr, mstr,
	       	 nstr, ostr, pstr, qstr, rstr, sstr, tstr, ustr, vstr, wstr, xstr, ystr, zstr
	       	 
num_array: .word zero, one, two, three, four, five, six, seven, eight, nine

buffer:   .space 100
user_input: .space 50
## End of file
