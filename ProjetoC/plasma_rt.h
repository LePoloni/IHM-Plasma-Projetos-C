/*--------------------------------------------------------------------
 * TITLE: Plasma Real-Time Hardware Defines
 * AUTHOR: Leandro Poloni Dantas (leandro.poloni@gmail.com) 
 * BASED ON: plasma.h of Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 05/08/17
 * FILENAME: plasma_rt.h
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma Real-Time Hardware Defines
 * MODIFICAÇÕES:
 * 05/08/17 - Incluidos os registradores para real-time
 *--------------------------------------------------------------------*/
#ifndef __PLASMA_H__
#define __PLASMA_H__

/*********** Hardware addresses ***********/
#define RAM_INTERNAL_BASE 0x00000000 //8KB
#define RAM_EXTERNAL_BASE 0x10000000 //1MB or 64MB
#define RAM_EXTERNAL_SIZE 0x00100000
#define ETHERNET_RECEIVE  0x13ff0000
#define ETHERNET_TRANSMIT 0x13fe0000
#define MISC_BASE         0x20000000
#define UART_WRITE        0x20000000
#define UART_READ         0x20000000
#define IRQ_MASK          0x20000010
#define IRQ_STATUS        0x20000020
#define GPIO0_SET         0x20000030
#define GPIO0_CLEAR       0x20000040
#define GPIOA_IN          0x20000050
#define COUNTER_REG       0x20000060
#define ETHERNET_REG      0x20000070

#define SCHEDULER_EN      0x20000080  //Habilita o escalonador
#define TIME_SLICE        0x20000090  //Tempo do tick ou time slice em pulsos de clock
#define TASK_NUMBER       0x200000A0  //Tarefa ativa (era Quantidade de tarefas 23/09/17)
#define TASK_VECTOR       0x200000B0  //Vetor de tarefas (existe/não existe, máx. 32)
#define PRI1_VECTOR       0x200000C0  //Vetor de prioridade das tarefas 7~0 (4 bits por tarefa)
#define PRI2_VECTOR       0x200000D0  //Vetor de prioridade das tarefas 15~8
#define PRI3_VECTOR       0x200000E0  //Vetor de prioridade das tarefas 23~16
#define PRI4_VECTOR       0x200000F0  //Vetor de prioridade das tarefas 31~24
#define SLEEP_TIME        0x20000100  //Tempo de sleep ou yield da tarefa atual

#define FLASH_BASE        0x30000000

/*********** GPIO out bits ***************/
#define ETHERNET_MDIO     0x00200000
#define ETHERNET_MDIO_WE  0x00400000
#define ETHERNET_MDC      0x00800000
#define ETHERNET_ENABLE   0x01000000

/*********** Interrupt bits **************/
#define IRQ_UART_READ_AVAILABLE  0x01
#define IRQ_UART_WRITE_AVAILABLE 0x02
#define IRQ_COUNTER18_NOT        0x04
#define IRQ_COUNTER18            0x08
#define IRQ_ETHERNET_RECEIVE     0x10
#define IRQ_ETHERNET_TRANSMIT    0x20
#define IRQ_GPIO31_NOT           0x40
#define IRQ_GPIO31               0x80

/*********** Protótipos de Funções *******/
unsigned int trataInt(unsigned int, unsigned int);

#endif //__PLASMA_H__

/*********** Códigos para debug **********/
/*
//Cria uma linha de código Assembly que pode ser facilmente localizada
//no arquivo test.lst ($26 = $k0)
asm volatile("addi $26,$0,1212;");

//Seta um ou mais pinos de saída em Assembly (GPIO0_SET = 0x20000030)
li		$v0,<valor>
lui	$v1,0x2000
ori	$v1,$v1,0x30
sw		$v0,0($v1)

//Reseta um ou mais pinos de saída em Assembly (GPIO0_CLEAR = 0x20000040)
li		$v0,<valor>
lui	$v1,0x2000
ori	$v1,$v1,0x40
sw		$v0,0($v1)
*/