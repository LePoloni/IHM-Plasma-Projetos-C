/*--------------------------------------------------------------------
 * TITLE: No OS Real-Time Softare Defines
 * AUTHOR: Leandro Poloni Dantas (leandro.poloni@gmail.com) 
 * BASED ON: no_os.c of Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 05/08/17
 * FILENAME: no_os_rt.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma Real-Time Software Defines
 * MODIFICAÇÕES:
 * 05/08/17 - Foram criadas funções para leitura e escrita de IOs
 * 24/08/17 - A função OS_InterruptServiceRoutine(,) deixou de ser void e
 *				  para a ser unsigned int para retornar o valor do stack pointer e
 *				  passou a ter mais um argumento, o valor do stack pointer ($sp).
 *				  Outra modificação nessa função foi a chamada da função trataInt(,)
 *				  que fica no arquivo plasma_rt.c, lá será feita o verdadeiro tratamento
 *				  das interrupções. 
 *--------------------------------------------------------------------*/

#include "plasma_rt.h"

#define MemoryRead(A) (*(volatile unsigned int*)(A))
#define MemoryWrite(A,V) *(volatile unsigned int*)(A)=(V)

int putchar(int value)
{
   while((MemoryRead(IRQ_STATUS) & IRQ_UART_WRITE_AVAILABLE) == 0)
      ;
   MemoryWrite(UART_WRITE, value);
   return 0;
}

int puts(const char *string)
//int puts(char *string)	//puts não estava funcionando, então tentei sem o "const" (26/12/17)
{
   while(*string)
   {
      if(*string == '\n')
         putchar('\r');
      putchar(*string++);
   }
   return 0;
}

void print_hex(unsigned long num)
{
   long i;
   unsigned long j;
   for(i = 28; i >= 0; i -= 4) 
   {
      j = (num >> i) & 0xf;
      if(j < 10) 
         putchar('0' + j);
      else 
         putchar('a' - 10 + j);
   }
}

unsigned int OS_InterruptServiceRoutine(unsigned int status, unsigned int stackp)
{
//Debug
//asm volatile("addi $26,$0,8888;");
	//Argumentos de funções são automaticamente $a0, $a1, $a2 e $a3
	//Colocar aqui as rotinas de teste e tratamento de interrupções
	(void)status;	//???
//Debug
//putchar('I');

//Debug
//puts("SP = "); puts(itoa10(stackp)); putchar('\r'); putchar('\n');
//puts("PC = "); puts(itoa10(MemoryRead(stackp+88))); putchar('\r'); putchar('\n');
	
	//Função para tratamento de interrupções
	//retorna o valor do no stack pointer (registrador $sp ou $29)
	stackp = trataInt(status, stackp);	//(plasma_rt.c)

//Debug
//asm volatile("addi $26,$0,7777;");

	//Retorna em $v0 o novo valor de $sp
	// asm volatile("move $v0,%0;"
				 // : "=r"(stackp)
				 // : "b" (stackp)
				// );
	return stackp;
}

int kbhit(void)
{
   return MemoryRead(IRQ_STATUS) & IRQ_UART_READ_AVAILABLE;
}

int getch(void)
{
   while(!kbhit()) ;
   return MemoryRead(UART_READ);
}

int clear_output_pin(unsigned int pin)
{
	unsigned int mask = 0x1<<pin;
	MemoryWrite(GPIO0_CLEAR, mask);
	return 0;
}

int set_output_pin(unsigned int pin)
{	
	unsigned int mask = 0x1<<pin;
//Debug
asm volatile("addi $26,$0,2121;");

	MemoryWrite(GPIO0_SET, mask);
	return 0;
}

unsigned int read_input_pin(unsigned int pin)
{
	return (MemoryRead(GPIOA_IN)>>pin) & 0x1;
}

unsigned int read_output_pin(unsigned int pin)
{
	return (MemoryRead(GPIO0_SET)>>pin) & 0x1;
}




