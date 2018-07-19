##################################################################
# TITLE: Boot Up Code
# AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
# DATE CREATED: 1/12/02
# EDITED: Leandro Poloni Dantas (leandro,poloni@gmail.com)
# DATE EDITED: 24/8/17
# FILENAME: boot.asm
# PROJECT: Plasma CPU core
# COPYRIGHT: Software placed into the public domain by the author.
#    Software 'as is' without warranty.  Author liable for nothing.
# DESCRIPTION:
#    Initializes the stack pointer and jumps to main().
# MODIFICAÇÕES:
# 24/08/17 - Alterie o retorno da rotina de tratamento de interrupção
#				 para trazer o valor do stack pointer ($sp) em $v0.
#				 Assim é possível realizar a troca de trarefa através de 
#				 escalonador por software.
##################################################################
   #Reserve 512 bytes for stack
   .comm InitStack, 512

   .text
   .align 2
   .global entry
   .ent	entry
entry:
   .set noreorder

   #These four instructions should be the first instructions.
   #convert.exe previously initialized $gp, .sbss_start, .bss_end, $sp
   la    $gp, _gp             #initialize global pointer
   la    $5, __bss_start      #$5 = .sbss_start
   la    $4, _end             #$2 = .bss_end
   la    $sp, InitStack+488   #initialize stack pointer

$BSS_CLEAR:
   sw    $0, 0($5)
   slt   $3, $5, $4
   bnez  $3, $BSS_CLEAR
   addiu $5, $5, 4

   jal   main
   nop
$L1:
   j $L1

   .end entry


###################################################
   #address 0x3c
   .global interrupt_service_routine
   .ent interrupt_service_routine
interrupt_service_routine:
   .set noreorder
   .set noat
	
#Debug - Tempo para realisar a ISR
#Seta um ou mais pinos de saída em Assembly (GPIO0_SET = 0x20000030)
li		$t5,0x8001		#bit 15 e 0
li		$t8,0x2002		#bit 13 e 1
lui	$t9,0x2000
ori	$t6,$t9,0x30	#t6	seta
ori	$t7,$t9,0x40	#t7	reseta
sw		$t5,0($t6)		#seta bit 15
sw		$t5,0($t7)		#reseta bit 15
#sw		$t5,0($t6)		#seta bit 15
#sw		$t5,0($t7)		#reseta bit 15
sw		$t5,0($t6)		#seta bit 15

   #Registers $26 and $27 are reserved for the OS
   #Save all temporary registers
   #Slots 0($29) through 12($29) reserved for saving a0-a3
#   addi  $29, $29, -104  #adjust sp
   addi  $29, $29, -136  #adjust sp	(considerando s0-s7)
   sw    $1,  16($29)    #at
   sw    $2,  20($29)    #v0
   sw    $3,  24($29)    #v1
   sw    $4,  28($29)    #a0
   sw    $5,  32($29)    #a1
   sw    $6,  36($29)    #a2
   sw    $7,  40($29)    #a3
   sw    $8,  44($29)    #t0
   sw    $9,  48($29)    #t1
   sw    $10, 52($29)    #t2
   sw    $11, 56($29)    #t3
   sw    $12, 60($29)    #t4
   sw    $13, 64($29)    #t5
   sw    $14, 68($29)    #t6
   sw    $15, 72($29)    #t7
   sw    $24, 76($29)    #t8
   sw    $25, 80($29)    #t9
   sw    $31, 84($29)    #lr
   mfc0  $26, $14        #C0_EPC=14 (Exception PC)
   addi  $26, $26, -4    #Backup one opcode
   sw    $26, 88($29)    #pc
   mfhi  $27
   sw    $27, 92($29)    #hi
   mflo  $27
   sw    $27, 96($29)    #lo
	#Incluidos no backup 26/08/2017
	sw    $16, 100($29)	 #s0
	sw    $17, 104($29)	 #s1
	sw    $18, 108($29)	 #s2
	sw    $19, 112($29)	 #s3
	sw    $20, 116($29)	 #s4
	sw    $21, 120($29)	 #s5
	sw    $22, 124($29)	 #s6
	sw    $23, 128($29)	 #s7

   lui   $6,  0x2000    
   lw    $4,  0x20($6)   #IRQ_STATUS
   lw    $6,  0x10($6)   #IRQ_MASK
   and   $4,  $4, $6		 #Envia em $a0 produto de IRQ_STATUS and IRQ_MASK

#Debug
#sw		$t8,0($t6)		#seta bit 13
#sw		$t5,0($t7)		#reseta bit 15

#   jal   OS_InterruptServiceRoutine		#Chama função para tratamanto de interrupções (no_os_rt.c)
#   addi  $5,  $29, 0		 #Envia em $a1 o valor do $sp

addi  $5,  $29, 0		 #Envia em $a1 o valor do $sp
jal   OS_InterruptServiceRoutine		#Chama função para tratamanto de interrupções (no_os_rt.c)
sw		$t5,0($t7)		#reseta bit 15

#Debug
#sw		$t8,0($t7)		#reseta bit 13
sw		$t5,0($t6)		#seta bit 15

	#Retornar em $v0 o novo valor de $sp
	addi	$29, $2, 0

   #Restore all temporary registers
   lw    $1,  16($29)    #at
   lw    $2,  20($29)    #v0
   lw    $3,  24($29)    #v1
   lw    $4,  28($29)    #a0
   lw    $5,  32($29)    #a1
   lw    $6,  36($29)    #a2
   lw    $7,  40($29)    #a3
   lw    $8,  44($29)    #t0
   lw    $9,  48($29)    #t1
   lw    $10, 52($29)    #t2
   lw    $11, 56($29)    #t3
   lw    $12, 60($29)    #t4
   lw    $13, 64($29)    #t5
   lw    $14, 68($29)    #t6
   lw    $15, 72($29)    #t7
   lw    $24, 76($29)    #t8
   lw    $25, 80($29)    #t9
   lw    $31, 84($29)    #lr
   lw    $26, 88($29)    #pc
   lw    $27, 92($29)    #hi
   mthi  $27
   lw    $27, 96($29)    #lo
   mtlo  $27
	#Incluidos no backup 26/08/2017
	lw    $16, 100($29)	 #s0
	lw    $17, 104($29)	 #s1
	lw    $18, 108($29)	 #s2
	lw    $19, 112($29)	 #s3
	lw    $20, 116($29)	 #s4
	lw    $21, 120($29)	 #s5
	lw    $22, 124($29)	 #s6
	lw    $23, 128($29)	 #s7
	
#   addi  $29, $29, 104   #adjust sp
   addi  $29, $29, 136   #adjust sp (considerando s0-s7)

isr_return:
   ori   $27, $0, 0x1    #re-enable interrupts

#Debug - Tempo para realisar a ISR
#Reseta um ou mais pinos de saída em Assembly (GPIO0_CLEAR = 0x20000040)
#li		$t5,0x8001		#bit 15 e 0
#lui	$t9,0x2000
#ori	$t6,$t9,0x30	#t6	seta
#ori	$t7,$t9,0x40	#t7	reseta	
#sw		$t5,0($t7)		#reseta bit 15

#   jr    $26
#   mtc0  $27, $12        #STATUS=1; enable interrupts

mtc0  $27, $12        #STATUS=1; enable interrupts
jr    $26
sw		$t5,0($t7)		#reseta bit 15

   .end interrupt_service_routine
   .set at


###################################################
   .global OS_AsmInterruptEnable
   .ent OS_AsmInterruptEnable
OS_AsmInterruptEnable:
   .set noreorder
   mfc0  $2, $12
   jr    $31
   mtc0  $4, $12            #STATUS=1; enable interrupts
   #nop
   .set reorder
   .end OS_AsmInterruptEnable


###################################################
   .global  OS_AsmInterruptInit
   .ent    OS_AsmInterruptInit
OS_AsmInterruptInit:
   .set noreorder
   #Patch interrupt vector to 0x1000003c
   la    $5, OS_AsmPatchValue
   lw    $6, 0($5)
   sw    $6, 0x3c($0)
   lw    $6, 4($5)
   sw    $6, 0x40($0)
   lw    $6, 8($5)
   sw    $6, 0x44($0)
   lw    $6, 12($5)
   jr    $31
   sw    $6, 0x48($0)

OS_AsmPatchValue:
   #Registers $26 and $27 are reserved for the OS
   #Code to place at address 0x3c
   lui   $26, 0x1000
   ori   $26, $26, 0x3c
   jr    $26
   nop

   .set reorder
   .end OS_AsmInterruptInit


###################################################
   .global   setjmp
   .ent     setjmp
setjmp:
   .set noreorder
   sw    $16, 0($4)   #s0
   sw    $17, 4($4)   #s1
   sw    $18, 8($4)   #s2
   sw    $19, 12($4)  #s3
   sw    $20, 16($4)  #s4
   sw    $21, 20($4)  #s5
   sw    $22, 24($4)  #s6
   sw    $23, 28($4)  #s7
   sw    $30, 32($4)  #s8
   sw    $28, 36($4)  #gp
   sw    $29, 40($4)  #sp
   sw    $31, 44($4)  #lr
   jr    $31
   ori   $2,  $0, 0

   .set reorder
   .end setjmp


###################################################
   .global   longjmp
   .ent     longjmp
longjmp:
   .set noreorder
   lw    $16, 0($4)   #s0
   lw    $17, 4($4)   #s1
   lw    $18, 8($4)   #s2
   lw    $19, 12($4)  #s3
   lw    $20, 16($4)  #s4
   lw    $21, 20($4)  #s5
   lw    $22, 24($4)  #s6
   lw    $23, 28($4)  #s7
   lw    $30, 32($4)  #s8
   lw    $28, 36($4)  #gp
   lw    $29, 40($4)  #sp
   lw    $31, 44($4)  #lr
   jr    $31
   ori   $2,  $5, 0

   .set reorder
   .end longjmp


###################################################
   .global   OS_AsmMult
   .ent     OS_AsmMult
OS_AsmMult:
   .set noreorder
   multu $4, $5
   mflo  $2
   mfhi  $4
   jr    $31
   sw    $4, 0($6)

   .set reorder
   .end OS_AsmMult


###################################################
   .global OS_Syscall
   .ent OS_Syscall
OS_Syscall:
   .set noreorder
   syscall 0
   jr    $31
   nop
   .set reorder
   .end OS_Syscall


