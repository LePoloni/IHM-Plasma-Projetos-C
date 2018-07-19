/*--------------------------------------------------------------------
 * TITLE: Plasma Real-Time Hardware Defines
 * AUTHOR: Leandro Poloni Dantas (leandro.poloni@gmail.com) 
 * BASED ON: plasma.h of Steve Rhoads (rhoadss@yahoo.com)
 * DATE CREATED: 05/08/17
 * FILENAME: plasma_rt.c
 * PROJECT: Plasma CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Plasma Real-Time Hardware Defines
 * MODIFICAÇÕES:
 * 24/08/17 - A função trataInt(,) passou a receber dois argumentos
 *				  (satus e stack pointer) e a retornar o novo stack pointer.
 *				  Isso permiti que essa função faça a definição de quando
 *				  utilizar o escalonador por sotware com base no tipo de 
 *				  interrupção e no #define para controle de compilação.
 * 04/01/18 - Introduzida a possibilidade de utilizar difirentes
 *				  algoritmos de escalonamento por software.
 *--------------------------------------------------------------------*/
//#include <hw_microk.c>
#include "plasma_rt.h"

//Macros utilizadas
#define MemoryRead(A) (*(volatile unsigned int*)(A))
#define MemoryWrite(A,V) *(volatile unsigned int*)(A)=(V)

#define EnableScheduler() *(volatile unsigned int*)(SCHEDULER_EN)=(1)
#define DisableScheduler() *(volatile unsigned int*)(SCHEDULER_EN)=(0)

#define TimeSlice(V) *(volatile unsigned int*)(TIME_SLICE)=(V)

#define EnableCounter() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)|IRQ_COUNTER18)
#define DisableCounter() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)&~IRQ_COUNTER18)

#define EnableNotCounter() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)|IRQ_COUNTER18_NOT)
#define DisableNotCounter() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)&~IRQ_COUNTER18_NOT)

#define EnableTask(t) *(volatile unsigned int*)(TASK_VECTOR)=(MemoryRead(TASK_VECTOR)|(1<<t))
#define DisableTask(t) *(volatile unsigned int*)(TASK_VECTOR)=(MemoryRead(TASK_VECTOR)&~(1<<t))

#define EnableUartRX() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)|IRQ_UART_READ_AVAILABLE)
#define DisableUartRX() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)&~IRQ_UART_READ_AVAILABLE)

//Controle de compilação
//#define software_scheduler
#define software_scheduler_v2
//#define Controle_IR

//#define uart_rx
//#define space_invaders_control

//Defines número de tarefas
#define t0 0
#define tJ 2
#define tF 4
#define tR 6
#define tD 8

//Definição de constantes
#define fifo_size	10

//Variáveis globais
#ifdef space_invaders_control
	#define snave		'N'
	#define sinimigo1	'I'
	#define sinimigo2	'J'
	#define sinimigo3	'K'
	#define stiro		'T'
	#define slaser		'L'
	#define sexplosao	'E'
	#define T			5
	#define L			4
	#define H			3
	#define W			2
	#define Y			1
	#define X			0
	// #define naveW		57
	// #define naveH		35
	// #define inimigoW	48
	// #define inimigoH	35
	// #define tiroW		6
	// #define tiroH		30
	// #define laserW		14
	// #define laserH		31
	// #define explosaoW	57
	// #define explosaoH	40
	// #define telaW		504
	// #define telaH		310
	const unsigned int naveW=57;
	const unsigned int naveH=35;
	const unsigned int inimigoW=48;
	const unsigned int inimigoH=35;
	const unsigned int tiroW=6;
	const unsigned int tiroH=30;
	const unsigned int laserW=14;
	const unsigned int laserH=31;
	const unsigned int explosaoW=57;
	const unsigned int explosaoH=40;
	const unsigned int telaW=504;
	const unsigned int telaH=310;
	
	unsigned char nave[16];		//Ajustei a variável para ser múltipla de 32 bits
	unsigned char inimigos[18][6];
	unsigned char tiros[18][6];
	unsigned char lasers[18][6];
	unsigned char explosao[8];	//Ajustei a variável para ser múltipla de 32 bits
	char string_frame[256];		//Verificar como isso é transformado, seriam 56 inteiros?
#endif

struct mem
{
	char dados[fifo_size];
	unsigned int start;		//Posição da próxima escrica
	unsigned int end;			//Posição da próxima leitura
	unsigned int writes;		//Quantidade de escritas
	unsigned int reads;		//Quantidade de leituras
	
};
struct mem fifo;

//Protótipos de funções utilizadas de outros arquivos
int putchar(int value);
int puts(const char *string);
int kbhit(void);
int clear_output_pin(unsigned int pin);
int set_output_pin(unsigned int pin);
unsigned int read_input_pin(unsigned int pin);
unsigned int read_output_pin(unsigned int pin);
int sleep_yield_task(unsigned int period);
int create_task(int (*task_function_pointer)(), int number, int priority);
int create_task_a(int (*task_function_pointer)(), int number, int priority, int argumento);
void scheduler_init(void);
void sw_scheduler_init(void);
void sw_scheduler_init_v2(void);
unsigned int sw_scheduler(unsigned int stackp);
unsigned int sw_scheduler_v2(unsigned int stackp);
unsigned int sw_sleep_yield_task(unsigned int period);

char *name[] = {
   "", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
   "ten", "eleven", "twelve", "thirteen", "fourteen", "fifteen",
   "sixteen", "seventeen", "eighteen", "nineteen",
   "", "ten", "twenty", "thirty", "forty", "fifty", "sixty", "seventy",
   "eighty", "ninety"
};

char *xtoa(unsigned long num)
{
   static char buf[12];
   int i, digit;
   buf[8] = 0;
   for (i = 7; i >= 0; --i)
   {
      digit = num & 0xf;
      buf[i] = digit + (digit < 10 ? '0' : 'A' - 10);
      num >>= 4;
   }
   return buf;
}

char *itoa10(unsigned long num)
{
   static char buf[12];
   int i;
   buf[10] = 0;
   for (i = 9; i >= 0; --i)
   {
      buf[i] = (char)((num % 10) + '0');
      num /= 10;
   }
   return buf;
}

char *itoa2(unsigned long num)
{
   static char buf[3];
   int i;
   buf[2] = 0;
   for (i = 1; i >= 0; --i)
   {
      buf[i] = (char)((num % 10) + '0');
      num /= 10;
   }
   return buf;
}

void number_text(unsigned long number)
{
   int digit;
   puts(itoa10(number));
   puts(": ");
   if(number >= 1000000000)
   {
      digit = number / 1000000000;
      puts(name[digit]);
      puts(" billion ");
      number %= 1000000000;
   }
   if(number >= 100000000)
   {
      digit = number / 100000000;
      puts(name[digit]);
      puts(" hundred ");
      number %= 100000000;
      if(number < 1000000)
      {
         puts("million ");
      }
   }
   if(number >= 20000000)
   {
      digit = number / 10000000;
      puts(name[digit + 20]);
      putchar(' ');
      number %= 10000000;
      if(number < 1000000)
      {
         puts("million ");
      }
   }
   if(number >= 1000000)
   {
      digit = number / 1000000;
      puts(name[digit]);
      puts(" million ");
      number %= 1000000;
   }
   if(number >= 100000)
   {
      digit = number / 100000;
      puts(name[digit]);
      puts(" hundred ");
      number %= 100000;
      if(number < 1000)
      {
         puts("thousand ");
      }
   }
   if(number >= 20000)
   {
      digit = number / 10000;
      puts(name[digit + 20]);
      putchar(' ');
      number %= 10000;
      if(number < 1000)
      {
         puts("thousand ");
      }
   }
   if(number >= 1000)
   {
      digit = number / 1000;
      puts(name[digit]);
      puts(" thousand ");
      number %= 1000;
   }
   if(number >= 100)
   {
      digit = number / 100;
      puts(name[digit]);
      puts(" hundred ");
      number %= 100;
   }
   if(number >= 20)
   {
      digit = number / 10;
      puts(name[digit + 20]);
      putchar(' ');
      number %= 10;
   }
   puts(name[number]);
   putchar ('\r');
   putchar ('\n');
}

void fifo_reset()
{
	fifo.start = 0;
	fifo.end = 0;
	fifo.reads = 0;
	fifo.writes = 0;
}

unsigned int fifo_in(char v)
{
	//Se a quantidade de escritas menos leituras é menor que o tamanho da fifo, ela não está cheia
	if((fifo.writes - fifo.reads) < fifo_size)
	{
		//Escreve na fifo
		fifo.dados[fifo.start++] = v;
		//Incrementa as escritas
		fifo.writes++;
		//Reset o ponteiro da fifo (circular)
		if(fifo.start == fifo_size)
		{	
			fifo.start = 0;
		}
		//Sucesso na gravação
		return 1;		
	}
	//Ajusta o tamanho dos contadores de diferença, assim nunca a variável estoura
	fifo.writes =  fifo_size;
	fifo.reads = 0;
	//Fifo full
	return 0;
}

char fifo_out()
{
	char v;
	//Se a quantidade de escritas  maior a de leituras, ela não está vazia
	if(fifo.writes > fifo.reads)
	{
		//Escreve da fifo
		v = fifo.dados[fifo.end++];
		//Incrementa as leituras
		fifo.reads++;
		//Reset o ponteiro da fifo (circular)
		if(fifo.end == fifo_size)
		{	
			fifo.end = 0;
		}
		//Retorna o valor lido
		return v;		
	}
	//Ajusta o tamanho dos contadores de diferença, assim nunca a variável estoura
	fifo.writes =  0;
	fifo.reads = 0;
	//Fifo empty
	return 0;
}

unsigned int fifo_empty()
{
	if(fifo.writes == fifo.reads)
	{
		return 1;
	}
	return 0;
}

unsigned int fifo_full()
{
	if((fifo.writes - fifo.reads) == fifo_size)
	{
		return 1;
	}
	return 0;
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE BÁSICO
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/*
int main2()
{
   unsigned long number, i=0;
	//Teste asm
	//asm volatile("li	$t1,0xFFFF");	//Funcionou!!!!
	
   number = 3;
   for(i = 0;; ++i)
   {
      number_text(number);
      number *= 3;
      //++number;
		if(read_input_pin(0))	set_output_pin(1);
		else 							clear_output_pin(1);
   }
}
*/

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM 4 TAREFAS
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/* 
int task0(void)
{
	int i;

	putchar('\r'); putchar('\n');
	puts("Task0");
	putchar('\r'); putchar('\n');

//Debug	
//	i = MemoryRead(0x20000080);
//	putchar(i + 0x30);

	//Pisca led 7
	while(1)
	{
		for(i = 1000000;i>0;i--);		
		set_output_pin(7);
		for(i = 1000000;i>0;i--);		
		clear_output_pin(7);
		puts("T0 ");
	}	
}

int task1(void)
{
//	int i;
	putchar('\r'); putchar('\n');
	puts("Task1");
	putchar('\r'); putchar('\n');
	
//	i = MemoryRead(0x20000080);
//	putchar(i+0x30);
	
	while(1)
	{
		clear_output_pin(1);
		//Led0 = Sw0
		if(read_input_pin(0))	set_output_pin(0);
		else 							clear_output_pin(0);
		
		if(read_input_pin(1))	
		{
			//Led1= Sw1
			set_output_pin(1);
			puts(" sleep T1"); putchar('\r'); putchar('\n');
			sleep_yield_task(3000);
			puts(" weak-up T1"); putchar('\r'); putchar('\n');
		}
	}	
}

int task2(void)
{
	int i;
	int led = 2;
	
	putchar('\r'); putchar('\n');
	puts("Task2");
	putchar('\r'); putchar('\n');
	
//	i = MemoryRead(0x20000080);
//	putchar(i+0x30);
//Debug
asm volatile("addi $26,$0,1212;");	
	while(1)
	{
		//Pisca led 2
		for(i = 1000000;i>0;i--);		
		set_output_pin(led);
		for(i = 1000000;i>0;i--);		
		clear_output_pin(led);
		puts("T2 ");
		
		if(read_input_pin(2))	
		{
			//Led2 = Sw2
			set_output_pin(2);
			puts(" sleep T2"); putchar('\r'); putchar('\n');
			sleep_yield_task(3000);
			puts(" weak-up T2"); putchar('\r'); putchar('\n');
		}
//Debug
asm volatile("addi $26,$0,2323;");		
	}	
}

int task3(void)
{
	int i;
	int led = 3;
	
	putchar('\r'); putchar('\n');
	puts("Task3");
	putchar('\r'); putchar('\n');
	
//	i = MemoryRead(0x20000080);
//	putchar(i+0x30);
	
	while(1)
	{
		//Pisca led 3
		for(i = 1000000;i>0;i--);		
		set_output_pin(led);
		for(i = 1000000;i>0;i--);		
		clear_output_pin(led);
		puts("T3 ");
		
		if(read_input_pin(3))	
		{
			//Led3 = Sw3
			set_output_pin(3);
			puts(" sleep T3"); putchar('\r'); putchar('\n');
			sleep_yield_task(3000);
			puts(" weak-up T3"); putchar('\r'); putchar('\n');
		}
	}	
}

int main()
{
	int i,j,k;
	create_task(task0, 0, 1);
	create_task(task1, 1, 3);
	create_task(task2, 2, 2);
	create_task(task3, 3, 2);
	
//DEBUG
// puts("TASK_VECTOR (0x200000B0): ");
// j=0x80000000;
// k=MemoryRead(0x200000B0);
// for(i=31;i>=0;i--)
// {
	// if(j&k) putchar('1'); else putchar('0');
	// j = j>>1;
// }
// putchar('\r'); putchar('\n');
	
// puts("ENDERECO DA FUNCAO TAREFA: "); putchar('\r'); putchar('\n');
// puts("TASK0 End. 0x00020000: ");
// k=MemoryRead(0x00020000);
// putchar(((k>>8)&0x0000000F)+0x30);
// putchar(((k>>4)&0x0000000F)+0x30);
// putchar(((k>>0)&0x0000000F)+0x30);
// putchar('\r'); putchar('\n');
		
// puts("TASK1 End. 0x00020100: ");
// k=MemoryRead(0x00020100);
// putchar(((k>>8)&0x0000000F)+0x30);
// putchar(((k>>4)&0x0000000F)+0x30);
// putchar(((k>>0)&0x0000000F)+0x30);
// putchar('\r'); putchar('\n');
		
// puts("TASK2 End. 0x00020200: ");
// k=MemoryRead(0x00020200);
// putchar(((k>>8)&0x0000000F)+0x30);
// putchar(((k>>4)&0x0000000F)+0x30);
// putchar(((k>>0)&0x0000000F)+0x30);
// putchar('\r'); putchar('\n');
		
// puts("TASK3 End. 0x00020300: ");
// k=MemoryRead(0x00020300);
// putchar(((k>>8)&0x0000000F)+0x30);
// putchar(((k>>4)&0x0000000F)+0x30);
// putchar(((k>>0)&0x0000000F)+0x30);
// putchar('\r'); putchar('\n');
	
// puts("PRIORIDADE DA TAREFA (0x200000C0): ");putchar('\r'); putchar('\n');
// k=MemoryRead(0x200000C0);
// puts("TASK0: ");
// putchar(((k>>0)&0x0000000F)+0x30);
// //	putchar(((k)&0x0000000F)+0x30);
// putchar('\r'); putchar('\n');
		
// puts("TASK1: ");
// putchar(((k>>4)&0x0000000F)+0x30);
// putchar('\r'); putchar('\n');
		
// puts("TASK2: ");
// putchar(((k>>8)&0x0000000F)+0x30);
// putchar('\r'); putchar('\n');
		
// puts("TASK3: ");
// putchar(((k>>12)&0x0000000F)+0x30);
// putchar('\r'); putchar('\n');

//Debug		
// //Salva o valor de SP
// asm volatile("add $26,$26,$0;");	//Só para achar no test.lst
// asm volatile ("move %0, $29" : "=r" (i));	//Salva em registrador
// //asm volatile ("sw $29, %0" : "=m" (i));	//Salva na pilha 
// puts("SP antes = "); puts(itoa10(i)); putchar('\r'); putchar('\n');
// //i = 0x1ffc;
// asm volatile("li %0,0x1FFFC;" 
				 // "move $29,%0;"
				 // : "=r"(i)
				 // : "b" (i)
				// );
// asm volatile ("move %0, $29" : "=r" (k));	//Salva em registrador
// puts("SP depois = "); puts(itoa10(k)); putchar('\r'); putchar('\n');

	//TimeSlice(25000);		//1ms
	TimeSlice(32768);		//1,31ms	- 2^15 usado para interrupção do Contador
	//TimeSlice(262144);	//10,48ms	- 2^18 usado para interrupção do Contador
	
	
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 7 está em 1 roda o scheduler por hardware
	if(read_input_pin(7))
	{
		//Hardware------------------------------------------
		EnableScheduler();
		// *(volatile unsigned int*)(0x20000080)=(1);
		//MemoryWrite(0x20000080, 1);
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0();
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
//		OS_AsmInterruptEnable(0x01);
		//Prepara o SP com o da tarefa 0x0
//Debug
asm volatile("addi $26,$0,2222;");
		asm volatile("li %0,0x1FFFC;"			//Endereço do SP da tarefa 0
						 "move $29,%0;"
						 : "=r"(i)
						 : "b" (i)
						);
//Debug
asm volatile ("move %0, $29" : "=r" (k));	//Salva em registrador
puts("SP task0 = "); puts(itoa10(k)); putchar('\r'); putchar('\n');
		
		
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0();
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//FUNÇÃO DE TRATAMENTO DE INTERRUPÇÃO TIMER
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
unsigned int trataInt(unsigned int status, unsigned int stackp)
{
	static unsigned int conta = 0;
	
#ifdef uart_rx
	unsigned int temp;
	//Se é interrupção de recepção da UART
	if(status & IRQ_UART_READ_AVAILABLE)
	{
		fifo_in((char)MemoryRead(UART_READ));
		EnableTask(tR);		//Habilita taskRX		
	}
#endif
		
	//Contador de interrupções
	conta++;	
	//Se é interrupção de COUNTER
	if(status & IRQ_COUNTER18)
	{
//Debug
set_output_pin(9);
		//Inverte o tipo de interrupção
		DisableCounter();		EnableNotCounter();
//#if defined software_scheduler && ! defined software_scheduler_v2
#if defined software_scheduler
		//Executa o algoritmo de escalonamento e
		//retorna em $v0 o novo valor de $sp
		stackp = sw_scheduler(stackp);	//(hd_microk.c)
#endif
#ifdef software_scheduler_v2
		//Executa o algoritmo de escalonamento e
		//retorna em $v0 o novo valor de $sp
		stackp = sw_scheduler_v2(stackp);	//(hd_microk.c)
#endif			
	}
	//Se é interrupção de NOT_COUNTER
	else if(status & IRQ_COUNTER18_NOT)
	{
//Debug
clear_output_pin(9);
		//Inverte o tipo de interrupção
		DisableNotCounter();	EnableCounter();
//#if defined software_scheduler && ! defined software_scheduler_v2
#if defined software_scheduler
		//Executa o algoritmo de escalonamento e
		//retorna em $v0 o novo valor de $sp
		stackp = sw_scheduler(stackp);	//(hd_microk.c)
#endif
#ifdef software_scheduler
		//Executa o algoritmo de escalonamento e
		//retorna em $v0 o novo valor de $sp
		stackp = sw_scheduler_v2(stackp);	//(hd_microk.c)
#endif	
	}
	return stackp;
}

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE CAPACIDADE DE INCREMENTOS POR INTERRUPÇÃO
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/*
int main3(void)
{
	unsigned int temp,aux1,aux2;
	volatile unsigned int *conta;
	conta = 0x00020000;
	//Setup de interrupção
	EnableCounter();
	//MemoryWrite(IRQ_MASK,0x09);
	//Habilita interrupções (coprocessador reg 12 bit 0)
	OS_AsmInterruptEnable(0x01);
	aux1 = 0;
	aux2 = 0;
	isr = 0;
	*conta = 0;
	
	puts("***Teste Interrupcoes***"); putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Debug
		puts("Espera ISR"); putchar('\r'); putchar('\n');
		
		//Espera a primeira interrupção
		while(isr==0);
		//Zeras as variáveis
		aux1 = 0;
		aux2 = 0;
		*conta = 0;
		//Debug
		puts("Comecou"); putchar('\r'); putchar('\n');

		//Teste asm
		asm volatile("li	$t1,0x1234");	//Funcionou!!!!
		
		//Enquanto não completar 1000 interrupções
		while(*conta < 2500)
 	
			// temp = *conta;
			// //Aguarda uma interrupção (incremento de contador)
			// while(temp == *conta){aux1++;}
			// temp = *conta;
			// //Aguarda outra interrupção (incremento de contador)
			// while(temp ==  *conta){aux2++;}

			
			aux1++;
		}
		while(*conta < 5000)
		{
			aux2++;
		}
				
		//Desbalita interrupções de Counter e Not_Counter
		DisableCounter();
		DisableNotCounter();
		//Mostra os resultados
		puts("---1000 interrupcoes---"); putchar('\r'); putchar('\n');
		puts("aux1 = "); puts(itoa10(aux1)); putchar('\r'); putchar('\n');
		puts("aux2 = "); puts(itoa10(aux2)); putchar('\r'); putchar('\n');
		//Zera a flag de sinalização que interrupções estão ocorrendo
		isr = 0;	
		//Habilita interrupção de Counter
		EnableCounter();
	}	
	return 0;
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE CAPACIDADE DE INCREMENTOS POR HD SCHEDULER
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/*
int taskInc1(void)
{
	//A variável tem que ser volatile, caso contrário o compilador não cria o looping
	//que aparentemente não tem sentido
	volatile unsigned int inc1 = 0;	//inc1 salvo em $v0 (reg 2)
	
	
	//Teste asm
	asm volatile("li	$v0,0x0");

	while(1)
	{
		//inc1++;
		
		//Teste asm
		asm volatile("addiu	$v0,$v0,1");
		
		//Não usar essas linhas para não interferir na leitura
		//if(inc1%50==0)
		//{
		//	puts("inc1=");puts(itoa10(inc1)); putchar('\r'); putchar('\n');		
		//}
	}
	return 0;
}

int taskInc2(void)
{
	//A variável tem que ser volatile, caso contrário o compilador não cria o looping
	//que aparentemente não tem sentido
	volatile unsigned int inc2 = 1000;	//inc2 salvo em $v0 (reg 2)
	
	//Teste asm
	asm volatile("li	$v0,0x0");
	
	while(1)
	{
		//inc2++;
		
		//Teste asm
		asm volatile("addiu	$v0,$v0,1");
		
		//if(inc2%600==0)
		//{
		//	puts("inc2=");puts(itoa10(inc2)); putchar('\r'); putchar('\n');
		//}
	}
	return 0;
}

int taskVerifica(void)
{
	volatile unsigned int *p1, *p2, inc1, inc2, i;
	//Inicializa os ponteiros com o endereço das variáveis inc1 e inc2 ($v0)
	p1 = 0x00020100 + 8;	//inc1
	p2 = 0x00020200 + 8;	//inc2
	
	while(1)
	{
		//Tarefa entra em sleep por 1000 time slices
		sleep_yield_task(5000);
		
		//Delay impírico
		for(i=0;i<300;i++);
		
		//Desabilita o escalonador
		DisableScheduler();
		
		//Delay impírico
		for(i=0;i<300;i++);
				
		//Mostra os PCs das tarefas
		putchar('\r'); putchar('\n');
		inc1 = MemoryRead(0x00020100);
		puts("PC task1 = "); puts(itoa10(inc1)); putchar('\r'); putchar('\n');
		inc2 = MemoryRead(0x00020200);
		puts("PC task2 = "); puts(itoa10(inc2)); putchar('\r'); putchar('\n'); 
		putchar('\r'); putchar('\n');

		// puts("Todos registradores:"); putchar('\r'); putchar('\n'); 
		// for(i=0;i<32;i++)
		// {
			// //Mostra os PCs das tarefas
			// inc1 = MemoryRead(0x00020100 + (i*4));
			// puts(itoa10(inc1)); putchar('\r'); putchar('\n');
			// inc2 = MemoryRead(0x00020200 + (i*4));
			// puts(itoa10(inc2)); putchar('\r'); putchar('\n'); 
			// putchar('\r'); putchar('\n');			
		// }
		// puts("Fim Todos Registradores"); putchar('\r'); putchar('\n'); 

		//Lê aos valores de inc1 e inc2
		inc1 = *p1;
		inc2 = *p2;
		//Mostra os resultados
		puts("---1000 time slices---"); putchar('\r'); putchar('\n');
		puts("inc1 = "); puts(itoa10(inc1)); putchar('\r'); putchar('\n');
		puts("inc2 = "); puts(itoa10(inc2)); putchar('\r'); putchar('\n');
		//Limpa as variáveis
		// *p1 = 0;
		// *p2 = 0;
		//Habilita o escalonador
		//EnableScheduler();
		while(1);
	}
	return 0;
}

int main4()
{
	unsigned int x;
//	volatile unsigned int inc1 = 0;
	
// 	while(1)
//	{
//		inc1++;
//	}	
	
	create_task(taskVerifica, 3, 2);
	create_task(taskInc1, 1, 1);
	create_task(taskInc2, 2, 1);

	x = MemoryRead(0x00020300);
	puts(itoa10(x)); putchar('\r'); putchar('\n');
	x = MemoryRead(0x00020100);
	puts(itoa10(x)); putchar('\r'); putchar('\n');
	x = MemoryRead(0x00020200);
	puts(itoa10(x)); putchar('\r'); putchar('\n'); putchar('\r'); putchar('\n');	
	
	puts("TASK0 End. 0x00020000: ");
		x=MemoryRead(0x00020000);
		putchar(((x>>8)&0x0000000F)+0x30);
		putchar(((x>>4)&0x0000000F)+0x30);
		putchar(((x>>0)&0x0000000F)+0x30);
		putchar('\r'); putchar('\n');
		
		puts("TASK1 End. 0x00020100: ");
		x=MemoryRead(0x00020100);
		putchar(((x>>8)&0x0000000F)+0x30);
		putchar(((x>>4)&0x0000000F)+0x30);
		putchar(((x>>0)&0x0000000F)+0x30);
		putchar('\r'); putchar('\n');
		
		puts("TASK2 End. 0x00020200: ");
		x=MemoryRead(0x00020200);
		putchar(((x>>8)&0x0000000F)+0x30);
		putchar(((x>>4)&0x0000000F)+0x30);
		putchar(((x>>0)&0x0000000F)+0x30);
		putchar('\r'); putchar('\n');
	
	
	
	TimeSlice(32768);		//~1,31ms
	//TimeSlice(327680);		//~13,1ms
		
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	EnableScheduler();
	
	//Força o desvio para taskVerifica, tarefa idle.
	//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
	//o PC atual é salvo como PC da task da posição 0.
	//taskVerifica();
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM 2 TAREFAS (TESTE DE DESEMPEMNHO)
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/* 
int task0(void)
{
	int i;

	putchar('\r'); putchar('\n');
	puts("Task0");
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		putchar('.');
	}	
}

int task1(void)
{
	int i;
	
	putchar('\r'); putchar('\n');
	puts("Task1");
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Pisca led
		for(i = 1000000;i>0;i--);		
		set_output_pin(0);			//LED 0
		set_output_pin(15);			//GPIO_0 pino 20
		for(i = 1000000;i>0;i--);		
		clear_output_pin(0);			//LED 0
		clear_output_pin(15);		//GPIO_0 pino 20
		puts("T1 ");
	}	
}

int main()
{
	int i,j,k;
	create_task(task0, 0, 1);
	create_task(task1, 1, 1);

	 TimeSlice(25000);		//1ms
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	// TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	// TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	// TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 7 está em 1 roda o scheduler por hardware
	if(read_input_pin(7))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0();
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0();
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM 32 TAREFAS (TESTE DE DESEMPEMNHO)
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/* 
int task0(int print)
{
	int i;

	putchar('\r'); putchar('\n');
	puts("Task0");
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		putchar(print); putchar('-');
	}	
}

int main()
{
	int i,j,k;
	create_task_a(task0, 0, 1, '0');
	create_task_a(task0, 1, 1, '1');
	create_task_a(task0, 2, 1, '2');
	create_task_a(task0, 3, 1, '3');
	create_task_a(task0, 4, 1, '4');
	create_task_a(task0, 5, 1, '5');
	create_task_a(task0, 6, 1, '6');
	create_task_a(task0, 7, 1, '7');
	create_task_a(task0, 8, 1, '8');
	create_task_a(task0, 9, 1, '9');
	
	create_task_a(task0, 10, 1, 'A');
	create_task_a(task0, 11, 1, 'B');
	create_task_a(task0, 12, 1, 'C');
	create_task_a(task0, 13, 1, 'D');
	create_task_a(task0, 14, 1, 'E');
	create_task_a(task0, 15, 1, 'F');
	create_task_a(task0, 16, 1, 'G');
	create_task_a(task0, 17, 1, 'H');
	create_task_a(task0, 18, 1, 'I');
	create_task_a(task0, 19, 1, 'J');
	
	create_task_a(task0, 20, 1, 'K');
	create_task_a(task0, 21, 1, 'L');
	create_task_a(task0, 22, 1, 'M');
	create_task_a(task0, 23, 1, 'N');
	create_task_a(task0, 24, 1, 'O');
	create_task_a(task0, 25, 1, 'P');
	create_task_a(task0, 26, 1, 'Q');
	create_task_a(task0, 27, 1, 'R');
	create_task_a(task0, 28, 1, 'S');
	create_task_a(task0, 29, 1, 'T');
	
	create_task_a(task0, 30, 1, 'U');
	create_task_a(task0, 31, 1, 'V');

	

	// TimeSlice(25000);		//1ms
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	// TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	// TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 8 está em 1 roda o scheduler por hardware
	if(read_input_pin(8))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0('0');
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0(0);
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM JOGO SPACE INVADERS NO PC (TESTE DE DESEMPEMNHO)
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/* 
int task0(int print)
{
	int i = 0;

	putchar('\r'); putchar('\n');
	puts("Task0");
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		while(i++<1000000);
		puts(".-"); putchar('\r'); putchar('\n');
		i = 0;
	}	
}

int taskJoystick(int print)
{
	int tecla;

	putchar('\r'); putchar('\n');
	puts("Task Joystick");
	putchar('\r'); putchar('\n');

	//Verifica se há alguma tecla pressionada
	while(1)
	{
		tecla = MemoryRead(GPIOA_IN);
		//Tecla A Start(bit 10), B Direita(bit 11), C Fire(bit 12), D Esquerda(bit 13)
		if(!(tecla&0x0400))
		{
			puts("st"); putchar('\r'); putchar('\n');
			sleep_yield_task(400);
		}
		else if(!(tecla&0x0800))
		{
			puts("di"); putchar('\r'); putchar('\n');
		}
		else if(!(tecla&0x1000))
		{
			puts("fi"); putchar('\r'); putchar('\n');
		}
		else if(!(tecla&0x2000))
		{
			puts("es"); putchar('\r'); putchar('\n');
		}
		sleep_yield_task(100);
	}	
}

int taskFrame(int print)
{
	putchar('\r'); putchar('\n');
	puts("Task Frame");
	putchar('\r'); putchar('\n');

	//Faz pedido de atualização de frame
	while(1)
	{
		//Controla a velocidade dos frames
		puts("fr"); putchar('\r'); putchar('\n');
		sleep_yield_task(33);		
	}	
}

int taskRX(int print)
{
	char mensagem[2];
	
	putchar('\r'); putchar('\n');
	puts("Task RX");
	putchar('\r'); putchar('\n');

	//Recebe um caracter
	while(1)
	{
		//Debug
		// if(!fifo_empty())
		// {
			// mensagem[0] = fifo_out();
			// putchar(mensagem[0]);
		// }
		
		
		//Se a fifo não está vazia, chegou dado pela serial (vide função de trataInt)
		if(!fifo_empty())
		{
			mensagem[0] = fifo_out();
			//Se chegou uma letra 'o'
			if(mensagem[0] == 'o')
			{
				//Se a fifo esvaziou desabilita a tarefa, ela será habilitada pela função de trataInt
				if(fifo_empty())
				{
					//Desabilita a tarefa de recepção
					DisableTask(tR);		//Habilita a tarefaRX
					sleep_yield_task(0);	//Realiza o yield da tarefa;
				}
				mensagem[1] = fifo_out();
				//Off
				if(mensagem[1] == 'f')
				{
					DisableTask(tF);		//Desabilita a tarefaFrame
					puts("okf"); putchar('\r'); putchar('\n');
				}
				//On
				else if(mensagem[1] == 'n')
				{
					EnableTask(tF);		//Habilita a tarefaFrame
					puts("okn"); putchar('\r'); putchar('\n');
				}				
			}
		}
		else
		{
			//Desabilita a tarefa de recepção
			DisableTask(tR);		//Habilita a tarefaRX
			sleep_yield_task(0);	//Realiza o yield da tarefa;
		}
	}
}

int taskDebug(int print)
{
	int i, temp;

	putchar('\r'); putchar('\n');
	puts("TaskDebug");
	putchar('\r'); putchar('\n');

	while(1)
	{
		puts("Tasks:");
		temp = MemoryRead(TASK_VECTOR);
		i = 0x100;
		while(i>=1)
		{
			if(temp&i) 	putchar('1');
			else			putchar('0');
			i = i>>1;				
		}
		putchar('\r'); putchar('\n');
		
		puts("UartM:");
		temp = MemoryRead(IRQ_MASK);
		if(temp&1) 	putchar('1');
		else			putchar('0');
		putchar('\r'); putchar('\n');

		puts("UartS:");
		temp = MemoryRead(IRQ_STATUS);
		if(temp&1) 	putchar('1');
		else			putchar('0');
		putchar('\r'); putchar('\n');
		
		sleep_yield_task(2000);
	}	
}

int main()
{
#define uart_rx												//Mudar a forma de tratar interrupções
	
	//Cria as tarefas
	create_task_a(task0, 			t0, 1, '0');
	create_task_a(taskJoystick, 	tJ, 3, '1');
	create_task_a(taskFrame, 		tF, 3, '2');
	create_task_a(taskRX, 			tR, 4, '3');		//Prioridade 4
	create_task_a(taskDebug,		tD, 2, '4');
	
	//Reseta a fifo
	fifo_reset();
	
	//Habilita interrupção de RX
	EnableUartRX();
	//Habilita interrupções (coprocessador reg 12 bit 0)
	OS_AsmInterruptEnable(0x01);
	
	//Desabilita a taskFrame e taskRX (dependem de comando externo)
	//DisableTask(2);
	//DisableTask(3);
	
	//Define o time slice	
	TimeSlice(25000);		//1ms
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	// TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	// TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	// TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	puts("Integração Plasma_RT com jogo SPACE INVADERS"); putchar('\r'); putchar('\n');
	puts("Teclas do Joystick:"); putchar('\r'); putchar('\n');
	puts("A - Start    "); putchar('\r'); putchar('\n');
	puts("D - Direita  "); putchar('\r'); putchar('\n');
	puts("C - Fire     "); putchar('\r'); putchar('\n');
	puts("D - Esquerda "); putchar('\r'); putchar('\n');
		
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 7 está em 1 roda o scheduler por hardware
	if(read_input_pin(7))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0('0');
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0(0);
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM n TAREFAS E n TIME SLICES (TESTE DE DESEMPEMNHO)
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/* 
int task2(int v)
{
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		set_output_pin(1);			//LED 1
		clear_output_pin(1);			//LED 1
	}	
}

int task1(int v)
{
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		putchar(v + '0');
	}	
}

int task0(int v)
{
	int i;
	
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v)); puts("principal");
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Pisca led
		for(i = 100000;i>0;i--);		
		set_output_pin(0);			//LED 0
		set_output_pin(15);			//GPIO_0 pino 20
		for(i = 100000;i>0;i--);		
		clear_output_pin(0);			//LED 0
		clear_output_pin(15);		//GPIO_0 pino 20
		//puts("T1 ");
	}	
}

int main()
{
	int i,j,k;
	
//Debug
//asm volatile("addi $26,$0,1212;");
//set_output_pin(2);	
	
	
	MemoryWrite(TASK_VECTOR, 0);		//Reset das tarefas ativas
	
	k = MemoryRead(GPIOA_IN);
	
	create_task_a(task0, 0, 1, 0);	//Idle - Gerador de onda quadrada
	
	//Quantidade de tarefas
	switch(k&0x0F)
	{
		case 0: j = 2; break;
		case 1: j = 3; break;
		case 2: j = 4; break;
		case 3: j = 5; break;
		case 4: j = 10; break;
		case 5: j = 15; break;
		case 6: j = 20; break;
		case 7: j = 25; break;
		case 8: j = 26; break;
		case 9: j = 27; break;
		case 10: j = 28; break;
		case 11: j = 29; break;
		case 12: j = 30; break;
		case 13: j = 31; break;
		case 14: j = 32; break;
		default: j = 1; break;
	}	
	for(i=1;i<j;i++)
	{
		//create_task_a(task1, i, 1, i);	//Dummy
		create_task_a(task2, i, 1, i);	//Dummy
	}
	
	puts("---"); puts(itoa2(j)); puts(" tarefas criadas---"); putchar('\r'); putchar('\n'); 
	
	//Time slice
	switch((k&0xF0)>>4)
	{
		case 0: j = 128; break;
		case 1: j = 256; break;
		case 2: j = 512; break;
		case 3: j = 1024; break;
		case 4: j = 2048; break;
		case 5: j = 4096; break;
		case 6: j = 8192; break;
		case 7: j = 16384; break;
		case 8: j = 32768; break;
		case 9: j = 65536; break;
		case 10: j = 131072; break;
		case 11: j = 262144; break;
		default: j = 64; break;
	}
	TimeSlice(j);
	
	puts("---"); puts(itoa10(j)); puts(" clocks/time slice---"); putchar('\r'); putchar('\n'); 
	
	// TimeSlice(25000);		//1ms
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	// TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	// TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	// TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 8 está em 1 roda o scheduler por hardware
	if(read_input_pin(8))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0(0);
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
//DEBUG
puts("TASK_VECTOR (0x200000B0): ");
j=0x80000000;
k=MemoryRead(0x200000B0);
for(i=31;i>=0;i--)
{
	if(j&k) putchar('1'); else putchar('0');
	j = j>>1;
}
putchar('\r'); putchar('\n');
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0(0);
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM 3 TAREFAS COM MODO COLABORATIVO
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/* 
int task1(int v)
{
	int i;
	unsigned int tarefa_ativa, tarefas_habilitadas, j, k;
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		tarefa_ativa = MemoryRead(TASK_NUMBER);
		tarefas_habilitadas = MemoryRead(TASK_VECTOR);

		//Desabilita as demais tarefas para priorizar sua execução
		MemoryWrite(TASK_VECTOR, tarefa_ativa);

		puts("Tarefa ativa: ");
		//puts("TA:");
		putchar(v + '0');
		putchar('\r'); putchar('\n');

		//DEBUG
		puts("TASK_VECTOR: ");
		j=0x80000000;
		k=MemoryRead(TASK_VECTOR);
		for(i=31;i>=0;i--)
		{
			if(j&k) putchar('1'); else putchar('0');
			j = j>>1;
		}
		putchar('\r'); putchar('\n');
		
		//puts("TASK_NUMBER: ");
		j=0x80000000;
		k=MemoryRead(TASK_NUMBER);
		
		puts("TASK_NUMBER: ");
		for(i=31;i>=0;i--)
		{
			if(j&k) putchar('1'); else putchar('0');
			j = j>>1;
		}
		putchar('\r'); putchar('\n');
		
		//Habilita as demais tarefas para continuar a execução
		MemoryWrite(TASK_VECTOR, tarefas_habilitadas);		
		
		//Yield ao acabar
		sleep_yield_task(0);
	}	
}

int task0(int v)
{
	int i;
	
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v)); puts("principal");
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Pisca led
		for(i = 100000;i>0;i--);		
		set_output_pin(0);			//LED 0
		set_output_pin(15);			//GPIO_0 pino 20
		for(i = 100000;i>0;i--);		
		clear_output_pin(0);			//LED 0
		clear_output_pin(15);		//GPIO_0 pino 20
		//puts("T1 ");
	}	
}

int main()
{
	int i,j,k;
	
	MemoryWrite(TASK_VECTOR, 0);		//Reset das tarefas ativas
	
	k = MemoryRead(GPIOA_IN);
	
	create_task_a(task0, 0, 1, 0);	//Idle - Gerador de onda quadrada
	
	//Quantidade de tarefas
	switch(k&0x0F)
	{
		case 0: j = 2; break;
		case 1: j = 3; break;
		case 2: j = 4; break;
		case 3: j = 5; break;
		case 4: j = 10; break;
		case 5: j = 15; break;
		case 6: j = 20; break;
		case 7: j = 25; break;
		case 8: j = 26; break;
		case 9: j = 27; break;
		case 10: j = 28; break;
		case 11: j = 29; break;
		case 12: j = 30; break;
		case 13: j = 31; break;
		case 14: j = 32; break;
		default: j = 1; break;
	}	
	for(i=1;i<j;i++)
	{
		create_task_a(task1, i, 1, i);	//Dummy
	}
	
	puts("---"); puts(itoa2(j)); puts(" tarefas criadas---"); putchar('\r'); putchar('\n'); 
	
	//Time slice
	switch((k&0xF0)>>4)
	{
		case 0: j = 128; break;
		case 1: j = 256; break;
		case 2: j = 512; break;
		case 3: j = 1024; break;
		case 4: j = 2048; break;
		case 5: j = 4096; break;
		case 6: j = 8192; break;
		case 7: j = 16384; break;
		case 8: j = 32768; break;
		case 9: j = 65536; break;
		case 10: j = 131072; break;
		case 11: j = 262144; break;
		default: j = 64; break;
	}
	TimeSlice(j);
	
	puts("---"); puts(itoa10(j)); puts(" clocks/time slice---"); putchar('\r'); putchar('\n'); 
	
	// TimeSlice(25000);		//1ms
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	// TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	// TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	// TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 8 está em 1 roda o scheduler por hardware
	if(read_input_pin(8))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0(0);
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
//DEBUG
puts("TASK_VECTOR (0x200000B0): ");
j=0x80000000;
k=MemoryRead(0x200000B0);
for(i=31;i>=0;i--)
{
	if(j&k) putchar('1'); else putchar('0');
	j = j>>1;
}
putchar('\r'); putchar('\n');
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0(0);
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SENSOR INFRAVERMELHO
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/* 
int taskIR(int v)
{
	#define IDLE		0
	#define GUIDANCE	1
	#define DATAREAD	2
	#define IR_RX		31
	
	int state = IDLE, count, bitcount, bitRX, dado, comando, comandoB;

	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	while(1)
	{
//set_output_pin(1);	//LED 1
		//Máquina de estados com 3 etapas
		//1 - Recebe LEAD
		//Zera contador de tempo
		count = 0;
		while(state == IDLE)
		{
			if(read_input_pin(IR_RX) == 0)
			{
				count++;
//				if(count > 70)	//LEAD = 9 ms (70*81,92 us = 5,66 ms)
				if(count > 50)	//LEAD = 9 ms (50*81,92 us = 4,10 ms, medido 55~56)
				{
//set_output_pin(2);	//LED 2
					state = GUIDANCE;	//Troca de estado
					
					
// sleep_yield_task(0);
// while(read_input_pin(IR_RX) == 0)
// {
	// sleep_yield_task(0);
	// count++;
// }
// putchar((char)count);
					
					
					
				}
			}
			else
			{
				count = 0;
			}
			//Yield, libera processamento para outras tarefas
			sleep_yield_task(0);			
		}
		//2 - Recebe SPACE
		//Zera contador de tempo
		count = 0;
		while(state == GUIDANCE)
		{
			if(read_input_pin(IR_RX) == 1)
			{
				count++;
				//if(count > 35)	//SPACE = 4,5 ms (35*81,92 us = 2,87 ms)
				if(count > 20)	//SPACE = 4,5 ms (20*81,92 us = 1,63 ms, medido 26~27)
				{
//set_output_pin(3);	//LED 3					
					state = DATAREAD;	//Troca de estado
					
										
// sleep_yield_task(0);
// while(read_input_pin(IR_RX) == 1)
// {
	// sleep_yield_task(0);
	// count++;
// }
// putchar((char)count);
					
					
				}
				
			}
			else
			{
				count = 0;
			}
			//Yield, libera processamento para outras tarefas
			sleep_yield_task(0);			
		}
//set_output_pin(4);	//LED 4
		//Aguarda terminar o SPACE
		while(read_input_pin(IR_RX) == 1)
		{
			//Yield, libera processamento para outras tarefas
			sleep_yield_task(0);
		}
//set_output_pin(5);	//LED 5
		//Zera contador de tempo, contador de bits e o dado recebido, define a máscara de bit recebido como 1
		count = 0; bitcount = 0; dado = 0; bitRX = 1;
		//3 - Recebe o dado
		while((state == DATAREAD) && (bitcount != 32))
		{
			//Mede o tempo do bit em 1 (lógica invertida) para saber se é 0 ou 1
			if(read_input_pin(IR_RX) == 1)
			{
				count++;
			}
			else
			{
				if(count > 0)
				{
					//if(count > 10) //BIT 0 = 562,5 us, BIT 1 = 1,687 ms (10*81,92 = 819,2 us)
					if(count > 7) //BIT 0 = 562,5 us, BIT 1 = 1,687 ms (7*81,92 = 573,4 us, medido 9~10)
					{
						dado |= bitRX;	//Recebeu um bit 1
						//putchar('1');
//putchar((char)count+'0');
					}
					// else
					// {
					//		//Recebeu um bit 0
					// 	putchar('0');		
					// }
					bitRX = bitRX << 1;	//Desloca a máscara para o próximo bit
					bitcount++;				//Incrementa a contador de bits
					count = 0;				//Zera a medição do tempo do bit em 1
				}
			}
			//Yield, libera processamento para outras tarefas
			sleep_yield_task(0);			
		}
//set_output_pin(6);	//LED 6		
		state = IDLE;									//Troca de estado
		comando = (dado >> 16) & 0x000000FF;	//Separa comando
		comandoB = dado >> 24;						//Separa comando barrado
		//Se o comando foi recebido corretamente
		if(comando == ~comandoB)				
		{
//set_output_pin(7);	//LED 7
			//Coloca o comando na fifo para tratamento por outra tarefa
			fifo_in((char)comando);
//fifo_in((char)comandoB);
		}
		//Yield, libera processamento para outras tarefas
		sleep_yield_task(0);
//MemoryWrite(GPIO0_CLEAR,0x000000FE);		
	}	
}

int taskComando(int v)
{
	char comando, cH, cL;
	
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Se tem algum comando na fifo
		if(!fifo_empty())
		{
			comando = fifo_out();		//Lê o comando
			
			//putchar(comando+0x30);		//Envia pela serial
			
			cH = comando >> 4;
			cL = comando & 0xf;
			
			putchar(cH + (cH < 10 ? '0' : 'A' - 10));
			putchar(cL + (cL < 10 ? '0' : 'A' - 10));
						
			putchar('\r'); putchar('\n');
		}
		//Yield, libera processamento para outras tarefas
		//sleep_yield_task(12);	//Roda a cada 12*81,92 us = 983 us ~ 1 ms
		sleep_yield_task(120);	//Roda a cada 120*81,92 us = 9830 us ~ 10 ms
	}
}

int task0(int v)
{
	int i;
	
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v)); puts(" principal");
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Pisca led
		for(i = 100000;i>0;i--);		
		set_output_pin(0);			//LED 0
		set_output_pin(15);			//GPIO_0 pino 20
		for(i = 100000;i>0;i--);		
		clear_output_pin(0);			//LED 0
		clear_output_pin(15);		//GPIO_0 pino 20
		//puts("T1 ");
//fifo_in(9);
	}	
}

int main()
{
	MemoryWrite(TASK_VECTOR, 0);		//Reset das tarefas ativas
	
	create_task_a(task0,			0, 1, 0);	//Idle - Gerador de onda quadrada
	create_task_a(taskIR,		1, 6, 1);	//Receptor de Infravermelho
	create_task_a(taskComando,	2, 7, 2);	//Processa Comando do Infravermelho
	
	puts("---Tarefas criadas---"); putchar('\r'); putchar('\n'); 
	
	//Time slice
	TimeSlice(2048);	//81,92us - 2^11 clocks (talvez 100 us seja melhor [2500])
	
	puts("---"); puts(itoa10(2048)); puts(" clocks/time slice---"); putchar('\r'); putchar('\n'); 
	
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 8 está em 1 roda o scheduler por hardware
	if(read_input_pin(8))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0(0);
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0(0);
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM JOGO SPACE INVADERS NO PC (TESTE DE DESEMPENHO)
//TODA LÓGICA DE CONTROLE FICA NO PROGRAMA GRAVADO NO PLASMA
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/*
//----------------------------------------------------//
//Formato do protocolo de comunição							//
//1o bytes - n múltiplos de 3 bytes - último byte		//
//Tamanho  - Sprites/Sons/Status		- Tamanho			//
//----------------------------------------------------//

void inicializa_tela()	//OK
{
	unsigned int random, i;
	//Inicializa os objetos do tipo SpriteSpaceInvaders
	//Cada objeto é descrito por um inteiro sem sinal
	//Campos:	Tipo 		Live	Height 	Width 	CoordY 	CoordX (X e Y sempre divididos por 2 para caber em 8 bits)
	//Byte:		5			4		3			2			1			0
	//Bits:		7..0		7..0	7..0		7..0		7..0		7..0
	//Nave
	nave[T] = 0;
	nave[L] = 30;
	nave[H] = naveH;
	nave[W] = naveW;	
	nave[Y] = (telaH - naveH)>>1;	
	nave[X] = ((telaW>>1) - (naveW>>1))>>1;
	//Inimigos
	random = MemoryRead(COUNTER_REG)%3;	//sorteia o tipo inicial de inimigo
	for (i = 0; i < 18; i++)
	{
		inimigos[i][T]	= random;
		inimigos[i][L]	= 30;
		inimigos[i][H] = inimigoH;
		inimigos[i][W] = inimigoW;
		inimigos[i][Y] = ((inimigoH + 20) * (i / 6) + 30)>>1;
		//inimigos[i][Y] = ((inimigoH>>1) + 20) * (i / 6) + 15;
		inimigos[i][X] = ((telaW/6)*(i%6) + (inimigoW>>1))>>1;
		//inimigos[i][X] = ((telaW>>1)/6)*(i%6) + (inimigoW>>2);
		random = (random+1)%3;
	}
	//Tiros
	for (i = 0; i < 18; i++)
	{
		tiros[i][T]	= 0;
		tiros[i][L]	= 0;
		tiros[i][H] = tiroH;	
		tiros[i][W] = tiroW;	
		tiros[i][Y] = 0;	
		tiros[i][X] = 0;
	}
	//Lasers
	for (i = 0; i < 18; i++)
	{
		lasers[i][T]	= 0;
		lasers[i][L]	= 0;
		lasers[i][H] = laserH;	
		lasers[i][W] = laserW;	
		lasers[i][Y] = 0;	
		lasers[i][X] = 0;		
	}
}

void monta_frame()	//OK
{
	int i, f=1;
	
	//Nave
	//Se a nave está viva
	if(nave[L])
	{
		if(nave[T] == 0)
			string_frame[f++] = snave;			//TIPO 
		else
			string_frame[f++] = sexplosao;	//TIPO
	}
	string_frame[f++] = nave[Y]; string_frame[f++] = nave[X];	//Y,X
	//Inimigos
	for (i = 0; i < 18; i++)
	{
		//Se o inimigo está vivo
		if(inimigos[i][L])
		{
			switch (inimigos[i][T])
			{
				case 0: 	string_frame[f++] = sinimigo1; break;	//TIPO
				case 1:	string_frame[f++] = sinimigo2; break;
				case 2:	string_frame[f++] = sinimigo3; break;
				default: string_frame[f++] = sexplosao; break;
			}
			string_frame[f++] = inimigos[i][Y]; string_frame[f++] = inimigos[i][X];	//Y,X
		}
	}
	//Tiros
	for (i = 0; i < 18; i++)
	{
		//Se o tiro está 'vivo'
		if(tiros[i][L])
		{
			string_frame[f++] = stiro; string_frame[f++] = tiros[i][Y]; string_frame[f++] = tiros[i][X]; //TIPO,Y,X
		}
	}
	//Lasers
	for (i = 0; i < 18; i++)
	{
		//Se o laser está 'vivo'
		if(lasers[i][L])
		{
			string_frame[f++] = slaser; string_frame[f++] = lasers[i][Y]; string_frame[f++] = lasers[i][X]; //TIPO,Y,X
		}
	}
	//Tamanho do pacote a ser enviado
	//O 1o e o último bytes identificam o tamanho do pacote
	string_frame[f] = (char)(f+1);
	string_frame[0] = (char)(f+1);
	string_frame[f+1] = 0;	//Terminador!!!
}

void envia_frame()	//OK
{
	puts(string_frame);
}

void atualiaza_sprites(char tecla, int frame)	//OK
{
	int i;
	//Atualiza posição da nave
	if(!(tecla&0x02))
	{
		nave[X] += 5;	//Atualiza a coordenada X
	}
	else if(!(tecla&0x08))
	{
		nave[X] -= 5;	//Atualiza a coordenada X
	}
	//Atualiza posição dos inimigos
	if (frame % 60 == 0)
	{
		for (i = 0; i < 18; i++)
		{
			//Atualiza posição
			inimigos[i][Y] += 5;	//Metade do desejado (10 pixels)
			if(inimigos[i][Y] > (telaH>>1))
				inimigos[i][L] = 0;	//Live = 0		
		}
	}
	//Atualiza posição dos lasers
	for (i = 0; i < 18; i++)
	{
		//Atualiza posição
		lasers[i][Y] += 2;	//Metade do desejado (4 pixels)
		if(lasers[i][Y] > (telaH>>1))
			lasers[i][L] = 0;	//Live = 0		
	}
	//Atualiza posição dos tiros
	for (i = 0; i < 18; i++)
	{
		//Atualiza posição
		tiros[i][Y] -= 0x2;	//Metade do desejado (4 pixels)
		if(tiros[i][Y] < 4)
			tiros[i][L] = 0;	//Live = 0		
	}		
}

int verifica_fim_do_jogo()	//OK
{
	int count = 0, i;
	//Verifica quantos inimigos vivos existem
   for (i = 0; i < 18; i++)
   {
      if (inimigos[i][L])		//Verifica o bit Live
			count++;
		if ((inimigos[i][L] < 30) && (inimigos[i][L] > 0))	//Verifica se está morrendo
         inimigos[i][L]--;		
   }
	if((nave[L] < 30) && (nave[L]>0))	//Verifica se está morrendo
		nave[L]--;
	
	if ((count == 0) && nave[L])	//Se não existem mais inimigos e a nave está viva 
	{
		putchar((char)5);
		putchar('e'); putchar('W'); putchar('i');
		putchar((char)5);
		return 1;
	}
	else if ((count > 0) && !nave[L])	//Se existem mais inimigos e a nave não está viva 
	{
		putchar((char)5);
		putchar('e'); putchar('L'); putchar('o');
		putchar((char)5);
		return 2;
	}
	
	putchar((char)5);
	putchar('e'); putchar(' '); putchar(' ');
	putchar((char)5);
	return 0;
}

void cria_sprites(char tecla)	//OK
{
	int count = 0, i, j;
	//Cria lasers
   //Verifica quantos inimigos vivos existem
   for (i = 0; i < 18; i++)
   {
      if (inimigos[i][L] == 30)
			count++;
   }
	//Se existirem inimigos vivos pode soltar novo laser
	if (count > 0)
	{
		//Define a probabilidade de soltar um novo laser
		if ((MemoryRead(COUNTER_REG) & 0x0000000F) == 15)
		{
			//Sorteia um inimigo para soltar o laser
			do
			{
				j = MemoryRead(COUNTER_REG)%18;
			}
			while (inimigos[j][L] < 30);
			//Procura o próximo laser desativado na lista de objetos
			for (i = 0; i < 18; i++)
			{
				if (!lasers[i][L])
				{
					lasers[i][L] = 1;	//Live = 1
					//X = X do inimigo + metade da largura dele (como a ccord. x tem escala 1/2, devemos dividir W por 2)	
					lasers[i][X] = inimigos[j][X] + (inimigoW>>2);
					//Y = Y do inimigo + a altura dele
					lasers[i][Y] = inimigos[j][Y] + (inimigoH>>1);
					
				//Dispara o som de laser	OK!
				putchar((char)5);
				putchar('s');putchar('L');putchar(' ');
				putchar((char)5);
					
					break;
				}
			}
		}		
	}
	//Cria tiros
	if (!(tecla&0x04) && (nave[L] == 30))
	{
		//Procura tiro livre (live = 0)
		for (i = 0; i < 18; i++)
		{
			if (!tiros[i][L])
			{
				tiros[i][L] = 1;	//Live = 1
				//X = X do inimigo + metade da largura dele (como a ccord. x tem escala 1/2, devemos dividir W por 2)	
				tiros[i][X] = nave[X] + (naveW>>2);
				//Y = Y do inimigo + metade da altura dele
				tiros[i][Y] = nave[Y] - (naveH>>1);
				
				//Dispara o som de tiro	OK!
				putchar((char)5);
				putchar('s');putchar('T');putchar(' ');
				putchar((char)5);

				break;
			}
		}
	}	
}

int verifica_impacto(unsigned char superior[], unsigned char inferior[])	//OK
{
	// *
	// *          Top             .------> X
	// *        _______           |
	// *  Left |  sup  | Right    |
	// *       |     __|___       |
	// *       |    |  |   |      V
	// *       |____|__|   |      Y
	// *            | inf  |
	// *            |______| 
	// *             Botton
	// *
	unsigned int sL,sR,sT,sB,iL,iR,iT,iB;
	
	sL = superior[X];
	sR = sL + (superior[W]>>1);
	sT = superior[Y];
	sB = sT + (superior[H]>>1);
	
	iL = inferior[X];
	iR = iL + (inferior[W]>>1);
	iT = inferior[Y];
	iB = iT + (inferior[H]>>1);
	
	//Se ocorreu sobreposição de retângulos
	if ((sB >= iT) && (sT <= iB) && (sR >= iL) && (sL <= iR))
	{
//Debug
//		puts("Impacto: 1"); putchar('\r'); putchar('\n');
		return 1;
	}
	else
	{
//Debug
//		puts("Impacto: 0"); putchar('\r'); putchar('\n');
		return 0;
	}
}

void verifica_impactos()	//OK
{
	int i, j;
	//Laser x Nave
   if (nave[L])
   {
      for (i = 0; i < 18; i++)
      {
         if (lasers[i][L])
         {
            if (verifica_impacto(lasers[i], nave))
            {
               nave[L]--;				//Começa a morrer (Live = 29)
               lasers[i][L] = 0;		//Live = 0
               nave[T] = 3;			//Tipo = 3 (explosão)
               
					//Dispara o som de explosão
					putchar((char)5);
					putchar('s');putchar('E');putchar(' ');
					putchar((char)5);
               break;
            }
         }
      }
   }
	//Inimigo x Tiro
	for (j = 0; j < 18; j++)
	{
		//Se o tiro está 'vivo'
		if (tiros[j][L])
		{
			for (i = 0; i < 18; i++)
			{
				//Se o inimigo está vivo
				if (inimigos[i][L] == 30)
				{
					if (verifica_impacto(inimigos[i], tiros[j]))
					{
						inimigos[i][L]--;				//Começa a morrer (Live = 29)
						tiros[j][L] = 0;				//Live = 0
						inimigos[i][T] = 3;			//Tipo = 3 (explosão)
						
						//Dispara o som de explosão
						putchar((char)5);
						putchar('s');putchar('E');putchar(' ');
						putchar((char)5);
						
						break;
					}
				}
			}
		}
	}
	//Inimigo x Nave
	if (nave[L])
   {
      for (i = 0; i < 18; i++)
      {
         if (inimigos[i][L])
         {
            if (verifica_impacto(inimigos[i], nave))
            {
               nave[L]--;					//Começa a morrer (Live = 29)
               inimigos[i][L]--;			//Começa a morrer (Live = 29)
               nave[T] = 3;				//Tipo = 3 (explosão)
               inimigos[i][T] = 3;		//Tipo = 3 (explosão)
					
					//Dispara o som de explosão
					putchar((char)5);
					putchar('s');putchar('E');putchar(' ');
					putchar((char)5);

               break;
            }
         }
      }
   }
}

int task0(int print)
{
	//Dummy
	int i;
	
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(print)); puts(" principal");
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Pisca led
		for(i = 1000000;i>0;i--);		
		set_output_pin(0);			//LED 0
		set_output_pin(15);			//GPIO_0 pino 20
		for(i = 1000000;i>0;i--);		
		clear_output_pin(0);			//LED 0
		clear_output_pin(15);		//GPIO_0 pino 20
		//puts("T1 ");
	}		
}

int taskJoystick(int print)
{
	int tecla;

	putchar('\r'); putchar('\n');
	puts("Task Joystick");
	putchar('\r'); putchar('\n');

	//Verifica se há alguma tecla pressionada
	while(1)
	{
		//Lê a entrada do teclado
		tecla = MemoryRead(GPIOA_IN);
		//Coloca os 4 bits do joystick nas 4 1as posições e formata como char antes de colocar na fifo
		fifo_in((char)(tecla>>10));
		//ANTES:	Tecla A Start(bit 10), B Direita(bit 11), C Fire(bit 12), D Esquerda(bit 13)
		//DEPOIS:                   0                  1               2                   3
		//Roda 1 vez a cada ... time slices
		if(read_input_pin(7))
			sleep_yield_task(100);		//Hardware
		else
			sw_sleep_yield_task(100);	//Software
	}	
}

int taskFrame(int print)
{
	unsigned int frame = 0, tecla, game = 0;
	putchar('\r'); putchar('\n');
	puts("Task Frame");
	putchar('\r'); putchar('\n');

	//Faz pedido de atualização de frame
	while(1)
	{
		//Controla a velocidade dos frames
		//puts("fr"); putchar('\r'); putchar('\n');
		frame++;
		
		//Verifica tecla
		tecla = 0xFF;
		if(!fifo_empty())
			tecla = fifo_out();
		
		//Se apertar Start
		if(!(tecla&0x01))
		{
			//Se o jogo está ligado --> para
			if(game)
			{
				game = 0;
				inicializa_tela();
				monta_frame();
				envia_frame();
			}
			//Senão --> ativa
			else
			{
				game = 1;
				inicializa_tela();
				//Dispara o som de ufo	OK!
				putchar((char)5);
				putchar('s');putchar('U');putchar(' ');
				putchar((char)5);
			}
		}
		
		//Se  o jogo está ativo
		if(game)
		{
			//1
			atualiaza_sprites(tecla, frame);
			//2
			cria_sprites(tecla);
			//3
			monta_frame();
			envia_frame();
			//4
			verifica_impactos();
			//5
			//verifica_fim_do_jogo();
			if(verifica_fim_do_jogo())
			{
				game = 0;
				putchar((char)5);
				puts("000");
				putchar((char)5);				
			}
		}		
		//Roda 1 vez a cada ... time slices
		if(read_input_pin(7))
			sleep_yield_task(33);		//Hardware
		else
			sw_sleep_yield_task(33);	//Software
	}	
}

int taskRX(int print)
{
	char mensagem[2];
	
	putchar('\r'); putchar('\n');
	puts("Task RX");
	putchar('\r'); putchar('\n');

	//Recebe um caracter
	while(1)
	{
		//Se a fifo não está vazia, chegou dado pela serial (vide função de trataInt)
		if(!fifo_empty())
		{
			mensagem[0] = fifo_out();
			//Se chegou uma letra 'o'
			if(mensagem[0] == 'o')
			{
				//Se a fifo esvaziou desabilita a tarefa, ela será habilitada pela função de trataInt
				if(fifo_empty())
				{
					//Desabilita a tarefa de recepção
					DisableTask(tR);		//Habilita a tarefaRX
					sleep_yield_task(0);	//Realiza o yield da tarefa;
				}
				mensagem[1] = fifo_out();
				//Off
				if(mensagem[1] == 'f')
				{
					DisableTask(tF);		//Desabilita a tarefaFrame
					puts("okf"); putchar('\r'); putchar('\n');
				}
				//On
				else if(mensagem[1] == 'n')
				{
					EnableTask(tF);		//Habilita a tarefaFrame
					puts("okn"); putchar('\r'); putchar('\n');
				}				
			}
		}
		else
		{
			//Desabilita a tarefa de recepção
			DisableTask(tR);		//Habilita a tarefaRX
			sleep_yield_task(0);	//Realiza o yield da tarefa;
		}
	}
}

int taskDebug(int print)
{
	int i, temp;

	putchar('\r'); putchar('\n');
	puts("TaskDebug");
	putchar('\r'); putchar('\n');

	while(1)
	{
		puts("Tasks:");
		temp = MemoryRead(TASK_VECTOR);
		i = 0x100;
		while(i>=1)
		{
			if(temp&i) 	putchar('1');
			else			putchar('0');
			i = i>>1;				
		}
		putchar('\r'); putchar('\n');
		
		puts("UartM:");
		temp = MemoryRead(IRQ_MASK);
		if(temp&1) 	putchar('1');
		else			putchar('0');
		putchar('\r'); putchar('\n');

		puts("UartS:");
		temp = MemoryRead(IRQ_STATUS);
		if(temp&1) 	putchar('1');
		else			putchar('0');
		putchar('\r'); putchar('\n');
		
		sleep_yield_task(2000);
	}	
}

int main()
{
//#define uart_rx												//Mudar a forma de tratar interrupções

	puts("Space Inverdes - controlado pelo Plasma_RT"); putchar('\r'); putchar('\n');
	
	//Cria as tarefas
	create_task_a(task0, 			t0, 1, '0');
	create_task_a(taskJoystick, 	tJ, 4, '1');
	create_task_a(taskFrame, 		tF, 3, '2');
//	create_task_a(taskRX, 			tR, 4, '3');		//Prioridade 4
//	create_task_a(taskDebug,		tD, 2, '4');
	
	//Reseta a fifo
	fifo_reset();
	
	//Habilita interrupção de RX
//	EnableUartRX();
	//Habilita interrupções (coprocessador reg 12 bit 0)
//	OS_AsmInterruptEnable(0x01);
	
	//Desabilita a taskFrame e taskRX (dependem de comando externo)
	//DisableTask(2);
	//DisableTask(3);
	
	//Define o time slice	
	// TimeSlice(25000);		//1ms
	// TimeSlice(50000);		//2ms
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	// TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	// TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	puts("Integração Plasma_RT com jogo SPACE INVADERS"); putchar('\r'); putchar('\n');
	puts("Teclas do Joystick:"); putchar('\r'); putchar('\n');
	puts("A - Start    "); putchar('\r'); putchar('\n');
	puts("D - Direita  "); putchar('\r'); putchar('\n');
	puts("C - Fire     "); putchar('\r'); putchar('\n');
	puts("D - Esquerda "); putchar('\r'); putchar('\n');
	
	//Aguarda o pino 8 para ligar o scheduler
	while(!read_input_pin(8));
		
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');

	//Se a chave 7 está em 1 roda o scheduler por hardware
	if(read_input_pin(7))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0(0);
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0(0);
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM JOGO SPACE INVADERS NO PC (TESTE DE DESEMPENHO)
//TODA LÓGICA DE CONTROLE FICA NO PROGRAMA GRAVADO NO PLASMA E JOYSTICK VIA IR
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/*
//----------------------------------------------------//
//Formato do protocolo de comunição							//
//1o bytes - n múltiplos de 3 bytes - último byte		//
//Tamanho  - Sprites/Sons/Status		- Tamanho			//
//----------------------------------------------------//

void inicializa_tela()	//OK
{
	unsigned int random, i;
	//Inicializa os objetos do tipo SpriteSpaceInvaders
	//Cada objeto é descrito por um inteiro sem sinal
	//Campos:	Tipo 		Live	Height 	Width 	CoordY 	CoordX (X e Y sempre divididos por 2 para caber em 8 bits)
	//Byte:		5			4		3			2			1			0
	//Bits:		7..0		7..0	7..0		7..0		7..0		7..0
	//Nave
	nave[T] = 0;
	nave[L] = 30;
	nave[H] = naveH;
	nave[W] = naveW;	
	nave[Y] = (telaH - naveH)>>1;	
	nave[X] = ((telaW>>1) - (naveW>>1))>>1;
	//Inimigos
	random = MemoryRead(COUNTER_REG)%3;	//sorteia o tipo inicial de inimigo
	for (i = 0; i < 18; i++)
	{
		inimigos[i][T]	= random;
		inimigos[i][L]	= 30;
		inimigos[i][H] = inimigoH;
		inimigos[i][W] = inimigoW;
		inimigos[i][Y] = ((inimigoH + 20) * (i / 6) + 30)>>1;
		//inimigos[i][Y] = ((inimigoH>>1) + 20) * (i / 6) + 15;
		inimigos[i][X] = ((telaW/6)*(i%6) + (inimigoW>>1))>>1;
		//inimigos[i][X] = ((telaW>>1)/6)*(i%6) + (inimigoW>>2);
		random = (random+1)%3;
	}
	//Tiros
	for (i = 0; i < 18; i++)
	{
		tiros[i][T]	= 0;
		tiros[i][L]	= 0;
		tiros[i][H] = tiroH;	
		tiros[i][W] = tiroW;	
		tiros[i][Y] = 0;	
		tiros[i][X] = 0;
	}
	//Lasers
	for (i = 0; i < 18; i++)
	{
		lasers[i][T]	= 0;
		lasers[i][L]	= 0;
		lasers[i][H] = laserH;	
		lasers[i][W] = laserW;	
		lasers[i][Y] = 0;	
		lasers[i][X] = 0;		
	}
}

void monta_frame()	//OK
{
	int i, f=1;
	
	//Nave
	//Se a nave está viva
	if(nave[L])
	{
		if(nave[T] == 0)
			string_frame[f++] = snave;			//TIPO 
		else
			string_frame[f++] = sexplosao;	//TIPO
	}
	string_frame[f++] = nave[Y]; string_frame[f++] = nave[X];	//Y,X
	//Inimigos
	for (i = 0; i < 18; i++)
	{
		//Se o inimigo está vivo
		if(inimigos[i][L])
		{
			switch (inimigos[i][T])
			{
				case 0: 	string_frame[f++] = sinimigo1; break;	//TIPO
				case 1:	string_frame[f++] = sinimigo2; break;
				case 2:	string_frame[f++] = sinimigo3; break;
				default: string_frame[f++] = sexplosao; break;
			}
			string_frame[f++] = inimigos[i][Y]; string_frame[f++] = inimigos[i][X];	//Y,X
		}
	}
	//Tiros
	for (i = 0; i < 18; i++)
	{
		//Se o tiro está 'vivo'
		if(tiros[i][L])
		{
			string_frame[f++] = stiro; string_frame[f++] = tiros[i][Y]; string_frame[f++] = tiros[i][X]; //TIPO,Y,X
		}
	}
	//Lasers
	for (i = 0; i < 18; i++)
	{
		//Se o laser está 'vivo'
		if(lasers[i][L])
		{
			string_frame[f++] = slaser; string_frame[f++] = lasers[i][Y]; string_frame[f++] = lasers[i][X]; //TIPO,Y,X
		}
	}
	//Tamanho do pacote a ser enviado
	//O 1o e o último bytes identificam o tamanho do pacote
	string_frame[f] = (char)(f+1);
	string_frame[0] = (char)(f+1);
	string_frame[f+1] = 0;	//Terminador!!!
}

void envia_frame()	//OK
{
	puts(string_frame);
}

void atualiaza_sprites(char tecla, int frame)	//OK
{
	int i;
	//Atualiza posição da nave
	if(!(tecla&0x02))
	{
		nave[X] += 5;	//Atualiza a coordenada X
	}
	else if(!(tecla&0x08))
	{
		nave[X] -= 5;	//Atualiza a coordenada X
	}
	//Atualiza posição dos inimigos
	if (frame % 60 == 0)
	{
		for (i = 0; i < 18; i++)
		{
			//Atualiza posição
			inimigos[i][Y] += 5;	//Metade do desejado (10 pixels)
			if(inimigos[i][Y] > (telaH>>1))
				inimigos[i][L] = 0;	//Live = 0		
		}
	}
	//Atualiza posição dos lasers
	for (i = 0; i < 18; i++)
	{
		//Atualiza posição
		lasers[i][Y] += 2;	//Metade do desejado (4 pixels)
		if(lasers[i][Y] > (telaH>>1))
			lasers[i][L] = 0;	//Live = 0		
	}
	//Atualiza posição dos tiros
	for (i = 0; i < 18; i++)
	{
		//Atualiza posição
		tiros[i][Y] -= 0x2;	//Metade do desejado (4 pixels)
		if(tiros[i][Y] < 4)
			tiros[i][L] = 0;	//Live = 0		
	}		
}

int verifica_fim_do_jogo()	//OK
{
	int count = 0, i;
	//Verifica quantos inimigos vivos existem
   for (i = 0; i < 18; i++)
   {
      if (inimigos[i][L])		//Verifica o bit Live
			count++;
		if ((inimigos[i][L] < 30) && (inimigos[i][L] > 0))	//Verifica se está morrendo
         inimigos[i][L]--;		
   }
	if((nave[L] < 30) && (nave[L]>0))	//Verifica se está morrendo
		nave[L]--;
	
	if ((count == 0) && nave[L])	//Se não existem mais inimigos e a nave está viva 
	{
		putchar((char)5);
		putchar('e'); putchar('W'); putchar('i');
		putchar((char)5);
		return 1;
	}
	else if ((count > 0) && !nave[L])	//Se existem mais inimigos e a nave não está viva 
	{
		putchar((char)5);
		putchar('e'); putchar('L'); putchar('o');
		putchar((char)5);
		return 2;
	}
	
//	putchar((char)5);
//	putchar('e'); putchar(' '); putchar(' ');
//	putchar((char)5);
	return 0;
}

void cria_sprites(char tecla)	//OK
{
	int count = 0, i, j;
	//Cria lasers
   //Verifica quantos inimigos vivos existem
   for (i = 0; i < 18; i++)
   {
      if (inimigos[i][L] == 30)
			count++;
   }
	//Se existirem inimigos vivos pode soltar novo laser
	if (count > 0)
	{
		//Define a probabilidade de soltar um novo laser
		if ((MemoryRead(COUNTER_REG) & 0x0000000F) == 15)
		{
			//Sorteia um inimigo para soltar o laser
			do
			{
				j = MemoryRead(COUNTER_REG)%18;
			}
			while (inimigos[j][L] < 30);
			//Procura o próximo laser desativado na lista de objetos
			for (i = 0; i < 18; i++)
			{
				if (!lasers[i][L])
				{
					lasers[i][L] = 1;	//Live = 1
					//X = X do inimigo + metade da largura dele (como a ccord. x tem escala 1/2, devemos dividir W por 2)	
					lasers[i][X] = inimigos[j][X] + (inimigoW>>2);
					//Y = Y do inimigo + a altura dele
					lasers[i][Y] = inimigos[j][Y] + (inimigoH>>1);
					
				//Dispara o som de laser	OK!
				putchar((char)5);
				putchar('s');putchar('L');putchar(' ');
				putchar((char)5);
					
					break;
				}
			}
		}		
	}
	//Cria tiros
	if (!(tecla&0x04) && (nave[L] == 30))
	{
		//Procura tiro livre (live = 0)
		for (i = 0; i < 18; i++)
		{
			if (!tiros[i][L])
			{
				tiros[i][L] = 1;	//Live = 1
				//X = X do inimigo + metade da largura dele (como a ccord. x tem escala 1/2, devemos dividir W por 2)	
				tiros[i][X] = nave[X] + (naveW>>2);
				//Y = Y do inimigo + metade da altura dele
				tiros[i][Y] = nave[Y] - (naveH>>1);
				
				//Dispara o som de tiro	OK!
				putchar((char)5);
				putchar('s');putchar('T');putchar(' ');
				putchar((char)5);

				break;
			}
		}
	}	
}

int verifica_impacto(unsigned char superior[], unsigned char inferior[])	//OK
{
	// *
	// *          Top             .------> X
	// *        _______           |
	// *  Left |  sup  | Right    |
	// *       |     __|___       |
	// *       |    |  |   |      V
	// *       |____|__|   |      Y
	// *            | inf  |
	// *            |______| 
	// *             Botton
	// *
	unsigned int sL,sR,sT,sB,iL,iR,iT,iB;
	
	sL = superior[X];
	sR = sL + (superior[W]>>1);
	sT = superior[Y];
	sB = sT + (superior[H]>>1);
	
	iL = inferior[X];
	iR = iL + (inferior[W]>>1);
	iT = inferior[Y];
	iB = iT + (inferior[H]>>1);
	
	//Se ocorreu sobreposição de retângulos
	if ((sB >= iT) && (sT <= iB) && (sR >= iL) && (sL <= iR))
	{
//Debug
//		puts("Impacto: 1"); putchar('\r'); putchar('\n');
		return 1;
	}
	else
	{
//Debug
//		puts("Impacto: 0"); putchar('\r'); putchar('\n');
		return 0;
	}
}

void verifica_impactos()	//OK
{
	int i, j;
	//Laser x Nave
   if (nave[L])
   {
      for (i = 0; i < 18; i++)
      {
         if (lasers[i][L])
         {
            if (verifica_impacto(lasers[i], nave))
            {
               nave[L]--;				//Começa a morrer (Live = 29)
               lasers[i][L] = 0;		//Live = 0
               nave[T] = 3;			//Tipo = 3 (explosão)
               
					//Dispara o som de explosão
					putchar((char)5);
					putchar('s');putchar('E');putchar(' ');
					putchar((char)5);
               break;
            }
         }
      }
   }
	//Inimigo x Tiro
	for (j = 0; j < 18; j++)
	{
		//Se o tiro está 'vivo'
		if (tiros[j][L])
		{
			for (i = 0; i < 18; i++)
			{
				//Se o inimigo está vivo
				if (inimigos[i][L] == 30)
				{
					if (verifica_impacto(inimigos[i], tiros[j]))
					{
						inimigos[i][L]--;				//Começa a morrer (Live = 29)
						tiros[j][L] = 0;				//Live = 0
						inimigos[i][T] = 3;			//Tipo = 3 (explosão)
						
						//Dispara o som de explosão
						putchar((char)5);
						putchar('s');putchar('E');putchar(' ');
						putchar((char)5);
						
						break;
					}
				}
			}
		}
	}
	//Inimigo x Nave
	if (nave[L])
   {
      for (i = 0; i < 18; i++)
      {
         if (inimigos[i][L])
         {
            if (verifica_impacto(inimigos[i], nave))
            {
               nave[L]--;					//Começa a morrer (Live = 29)
               inimigos[i][L]--;			//Começa a morrer (Live = 29)
               nave[T] = 3;				//Tipo = 3 (explosão)
               inimigos[i][T] = 3;		//Tipo = 3 (explosão)
					
					//Dispara o som de explosão
					putchar((char)5);
					putchar('s');putchar('E');putchar(' ');
					putchar((char)5);

               break;
            }
         }
      }
   }
}

int task0(int v)
{
	//Dummy
	int i;
	
	putchar('\r'); putchar('\n');
	puts("Task Idle "); putchar((char)v);
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Pisca led
		for(i = 1000000;i>0;i--);		
		set_output_pin(0);			//LED 0
		set_output_pin(15);			//GPIO_0 pino 20
		for(i = 1000000;i>0;i--);		
		clear_output_pin(0);			//LED 0
		clear_output_pin(15);		//GPIO_0 pino 20
		//puts("T1 ");
	}		
}

int taskJoystick(int v)	//Conectado ao kit - coloca na fifo a tecla pressionada
{
	int tecla;

	putchar('\r'); putchar('\n');
	puts("Task Joystick "); putchar((char)v);
	putchar('\r'); putchar('\n');

	//Verifica se há alguma tecla pressionada
	while(1)
	{
		//Lê a entrada do teclado
		tecla = MemoryRead(GPIOA_IN);
		//Coloca os 4 bits do joystick nas 4 1as posições e formata como char antes de colocar na fifo
		fifo_in((char)(tecla>>10));
		//ANTES:	Tecla A Start(bit 10), B Direita(bit 11), C Fire(bit 12), D Esquerda(bit 13)
		//DEPOIS:                   0                  1               2                   3
		//Roda 1 vez a cada ... time slices
		if(read_input_pin(7))
			sleep_yield_task(100*16);		//Hardware
		else
			sw_sleep_yield_task(100*16);	//Software
//Debug		
//putchar((char)(~(tecla>>10) + '0'));
	}	
}

int taskFrame(int v)	//Ajustada para IR
{
	unsigned int frame = 0, tecla, game = 0;
	putchar('\r'); putchar('\n');
	puts("Task Frame "); putchar((char)v);
	putchar('\r'); putchar('\n');

	//Faz pedido de atualização de frame
	while(1)
	{
		//Controla a velocidade dos frames
		//puts("fr"); putchar('\r'); putchar('\n');
		frame++;
		
		//Verifica tecla
		tecla = 0xFF;
		if(!fifo_empty())
		{
			tecla = fifo_out();
			//putchar(tecla+'0');
			//Ajustes para IR
#ifdef Controle_IR
			switch(tecla)
			{
				case 1: tecla = ~0x08;	break;	//Esquerda
				case 2: tecla = ~0x04;	break;	//Tiro
				case 3: tecla = ~0x02;	break;	//Dreita
				case 4: tecla = ~0x0C;	break;	//Esquerda + Tiro
				case 6: tecla = ~0x06;	break;	//Dreita + Tiro
				case 5: tecla = ~0x01;	break;	//Start
				default: tecla = ~0x00; break;
			}
#endif
//Debug
//putchar((char)~tecla + '0');
		}
		
		//Se apertar Start
		if(!(tecla&0x01))
		{
			//Se o jogo está ligado --> para
			if(game)
			{
				game = 0;
				inicializa_tela();
				monta_frame();
				envia_frame();
			}
			//Senão --> ativa
			else
			{
				game = 1;
				inicializa_tela();
				//Dispara o som de ufo	OK!
				putchar((char)5);
				putchar('s');putchar('U');putchar(' ');
				putchar((char)5);
			}
		}
		
		//Se  o jogo está ativo
		if(game)
		{
			//1
			atualiaza_sprites(tecla, frame);
			//2
			cria_sprites(tecla);
			//3
			monta_frame();
			envia_frame();
			//4
			verifica_impactos();
			//5
			//verifica_fim_do_jogo();
			if(verifica_fim_do_jogo())
			{
				game = 0;
//				putchar((char)5);
//				puts("000");
//				putchar((char)5);				
			}
		}		
		//Roda 1 vez a cada ... time slices
		if(read_input_pin(7))
			sleep_yield_task(33*16*4);		//Hardware
		else
			sw_sleep_yield_task(33*16*4);	//Software
	}	
}

int taskRX(int print)
{
	char mensagem[2];
	
	putchar('\r'); putchar('\n');
	puts("Task RX");
	putchar('\r'); putchar('\n');

	//Recebe um caracter
	while(1)
	{
		//Se a fifo não está vazia, chegou dado pela serial (vide função de trataInt)
		if(!fifo_empty())
		{
			mensagem[0] = fifo_out();
			//Se chegou uma letra 'o'
			if(mensagem[0] == 'o')
			{
				//Se a fifo esvaziou desabilita a tarefa, ela será habilitada pela função de trataInt
				if(fifo_empty())
				{
					//Desabilita a tarefa de recepção
					DisableTask(tR);		//Habilita a tarefaRX
					sleep_yield_task(0);	//Realiza o yield da tarefa;
				}
				mensagem[1] = fifo_out();
				//Off
				if(mensagem[1] == 'f')
				{
					DisableTask(tF);		//Desabilita a tarefaFrame
					puts("okf"); putchar('\r'); putchar('\n');
				}
				//On
				else if(mensagem[1] == 'n')
				{
					EnableTask(tF);		//Habilita a tarefaFrame
					puts("okn"); putchar('\r'); putchar('\n');
				}				
			}
		}
		else
		{
			//Desabilita a tarefa de recepção
			DisableTask(tR);		//Habilita a tarefaRX
			sleep_yield_task(0);	//Realiza o yield da tarefa;
		}
	}
}

int taskDebug(int print)
{
	int i, temp;

	putchar('\r'); putchar('\n');
	puts("TaskDebug");
	putchar('\r'); putchar('\n');

	while(1)
	{
		puts("Tasks:");
		temp = MemoryRead(TASK_VECTOR);
		i = 0x100;
		while(i>=1)
		{
			if(temp&i) 	putchar('1');
			else			putchar('0');
			i = i>>1;				
		}
		putchar('\r'); putchar('\n');
		
		puts("UartM:");
		temp = MemoryRead(IRQ_MASK);
		if(temp&1) 	putchar('1');
		else			putchar('0');
		putchar('\r'); putchar('\n');

		puts("UartS:");
		temp = MemoryRead(IRQ_STATUS);
		if(temp&1) 	putchar('1');
		else			putchar('0');
		putchar('\r'); putchar('\n');
		
		sleep_yield_task(2000);
	}	
}

int taskIR(int v)
{
	#define IDLE		0
	#define GUIDANCE	1
	#define DATAREAD	2
	#define IR_RX		31
	
	int state = IDLE, count, bitcount, bitRX, dado, comando, comandoB;

	putchar('\r'); putchar('\n');
	puts("Task IR "); putchar((char)v);
	putchar('\r'); putchar('\n');

	while(1)
	{
		//Máquina de estados com 3 etapas
		//1 - Recebe LEAD
		//Zera contador de tempo
		count = 0;
		while(state == IDLE)
		{
			if(read_input_pin(IR_RX) == 0)
			{
				count++;
				//if(count > 70)	//LEAD = 9 ms (70*81,92 us = 5,66 ms)
				//if(count > 50)	//LEAD = 9 ms
				if(count > 200)	//LEAD = 9 ms
					state = GUIDANCE;	//Troca de estado
			}
			else
			{
				count = 0;
			}
			//Yield, libera processamento para outras tarefas
			//sleep_yield_task(0);
			if(read_input_pin(7))
				sleep_yield_task(0);		//Hardware
			else
				sw_sleep_yield_task(0);	//Software
		}
		//2 - Recebe SPACE
		//Zera contador de tempo
		count = 0;
		while(state == GUIDANCE)
		{
			if(read_input_pin(IR_RX) == 1)
			{
				count++;
				//if(count > 35)	//SPACE = 4,5 ms (70*81,92 us = 2,87 ms)
				//if(count > 20)	//SPACE = 4,5 ms
				if(count > 80)	//SPACE = 4,5 ms
					state = DATAREAD;	//Troca de estado
			}
			else
			{
				count = 0;
			}
			//Yield, libera processamento para outras tarefas
			//sleep_yield_task(0);
			if(read_input_pin(7))
				sleep_yield_task(0);		//Hardware
			else
				sw_sleep_yield_task(0);	//Software
		}
		//Aguarda terminar o SPACE
		while(read_input_pin(IR_RX) == 1)
		{
			//Yield, libera processamento para outras tarefas
			//sleep_yield_task(0);
			if(read_input_pin(7))
				sleep_yield_task(0);		//Hardware
			else
				sw_sleep_yield_task(0);	//Software
		}
		//Zera contador de tempo, contador de bits e o dado recebido, define a máscara de bit recebido como 1
		count = 0; bitcount = 0; dado = 0; bitRX = 1;
		//3 - Recebe o dado
		while(state == DATAREAD && bitcount != 32)
		{
			//Mede o tempo do bit em 1 (lógica invertida) para saber se é 0 ou 1
			if(read_input_pin(IR_RX) == 1)
			{
				count++;
			}
			else
			{
				if(count > 0)
				{
					//if(count > 10) //BIT 0 = 562,5 us, BIT 1 = 1,687 ms (10*81,92 = 819,2 us)
					//if(count > 7) //BIT 0 = 562,5 us, BIT 1 = 1,687 ms
					if(count > 28) //BIT 0 = 562,5 us, BIT 1 = 1,687 ms
					{
						dado |= bitRX;	//Recebeu um bit 1
						//putchar('1');
					}
					//else
					//{
					//	//Recebeu um bit 0
					//	putchar('0');		
					//}
					bitRX = bitRX << 1;	//Desloca a máscara para o próximo bit
					bitcount++;				//Incrementa a contador de bits
					count = 0;				//Zera a medição do tempo do bit em 1
				}
			}
			//Yield, libera processamento para outras tarefas
			//sleep_yield_task(0);
			if(read_input_pin(7))
				sleep_yield_task(0);		//Hardware
			else
				sw_sleep_yield_task(0);	//Software			
		}
		
		state = IDLE;									//Troca de estado
		comando = (dado >> 16) & 0x000000FF;	//Separa comando
		comandoB = dado >> 24;						//Separa comando barrado
		//Se o comando foi recebido corretamente
		if(comando == ~comandoB)				
		{
			//Coloca o comando na fifo para tratamento por outra tarefa
			fifo_in((char)comando);
		}
		//Yield, libera processamento para outras tarefas
		//sleep_yield_task(0);
		if(read_input_pin(7))
			sleep_yield_task(0);		//Hardware
		else
			sw_sleep_yield_task(0);	//Software		
	}	
}

int taskComando(int v)	//Conectado ao IR - coloca na fifo a tecla pressionada
{
	char comando, cH, cL;
	
	putchar('\r'); putchar('\n');
	puts("Task Comando "); putchar((char)v);
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Se tem algum comando na fifo
		if(!fifo_empty())
		{
			comando = fifo_out();		//Lê o comando
			
			//putchar(comando+0x30);		//Envia pela serial
			
			cH = comando >> 4;
			cL = comando & 0xf;
			
			putchar(cH + (cH < 10 ? '0' : 'A' - 10));
			putchar(cL + (cL < 10 ? '0' : 'A' - 10));
						
			putchar('\r'); putchar('\n');
		}
		//Yield, libera processamento para outras tarefas
		//sleep_yield_task(10);	//Roda a cada 12*81,92 us = 983 us ~ 1 ms
		if(read_input_pin(7))
			sleep_yield_task(10*4*4);		//Hardware
		else
			sw_sleep_yield_task(10*4*4);	//Software		
	}
}

int main()
{
//#define uart_rx												//Mudar a forma de tratar interrupções

	puts("Space Inverdes - controlado pelo Plasma_RT"); putchar('\r'); putchar('\n');
	
	//Cria as tarefas (para usar escalonamento por software v2: colocar em ordem
	//de prioridade crescente)
	//				  função			task	prio	arg
	create_task_a(task0, 			0, 	1, '1');
	create_task_a(taskFrame, 		2, 	2, '2');
//	create_task_a(taskJoystick, 	4, 	3, '3');
	create_task_a(taskIR,			4, 	4, '4');	
//	create_task_a(taskComando,		6, 	5, '5'); 
	//Reseta a fifo
	fifo_reset();
	
	//Habilita interrupção de RX
//	EnableUartRX();
	//Habilita interrupções (coprocessador reg 12 bit 0)
//	OS_AsmInterruptEnable(0x01);
	
	//Desabilita a taskFrame e taskRX (dependem de comando externo)
	//DisableTask(2);
	//DisableTask(3);
	
	//Define o time slice	
	// TimeSlice(25000);		//1ms
	// TimeSlice(50000);		//2ms
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	//	TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	// TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	puts("Integração Plasma_RT com jogo SPACE INVADERS"); putchar('\r'); putchar('\n');
	puts("Teclas do Joystick:"); putchar('\r'); putchar('\n');
	puts("A - Start    "); putchar('\r'); putchar('\n');
	puts("D - Direita  "); putchar('\r'); putchar('\n');
	puts("C - Fire     "); putchar('\r'); putchar('\n');
	puts("D - Esquerda "); putchar('\r'); putchar('\n');
	
	//Aguarda o pino 8 para ligar o scheduler
	while(!read_input_pin(8));
		
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');

	//Se a chave 7 está em 1 roda o scheduler por hardware
	if(read_input_pin(7))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0(0);
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
#if defined software_scheduler && ! defined software_scheduler_v2
		sw_scheduler_init();
#endif
#ifdef software_scheduler_v2
		sw_scheduler_init_v2();
#endif
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0(0);
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW SCHEDULER COM 1 ou 2 TAREFAS (TESTE DE TEMPO PARA TROCA)
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/*
int task6(int v)
{
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		clear_output_pin(v);			//LED v
		clear_output_pin(0);			//LED v
	}	
}

int task5(int v)
{
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		set_output_pin(v);			//LED v
		set_output_pin(0);			//LED v
	}	
}

int task4(int v)
{
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		set_output_pin(1);			//LED 1
		clear_output_pin(1);			//LED 1
		set_output_pin(1);			//LED 1
		clear_output_pin(1);			//LED 1
	}	
}

int task3(int v)
{
	int i;
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		sleep_yield_task(1);
		set_output_pin(2);			//LED 2
		for(i=20; i>0; i--);
		//set_output_pin(2);			//LED 2
		clear_output_pin(2);			//LED 2
	}	
}
 
int task2(int v)
{
	putchar('\r'); putchar('\n');
	puts("Task "); puts(itoa2(v));
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		set_output_pin(1);			//LED 1
		clear_output_pin(1);			//LED 1
		set_output_pin(1);			//LED 1
		clear_output_pin(1);			//LED 1
		set_output_pin(1);			//LED 1
		clear_output_pin(1);			//LED 1
		set_output_pin(1);			//LED 1
		clear_output_pin(1);			//LED 1
	}	
}

int task1(int v)
{
	putchar('\r'); putchar('\n');
	puts("Task "); putchar((char)v);
	putchar('\r'); putchar('\n');

	//Dummy
	while(1)
	{
		putchar((char)v);
	}	
}

int task0(int v)
{
	int i;
	
	putchar('\r'); putchar('\n');
	puts("Task "); putchar((char)v); puts(" principal");
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Pisca led
		for(i = 100000;i>0;i--);		
		set_output_pin(2);			//LED 2
		set_output_pin(11);			//GPIO_0 pino 16
		for(i = 100000;i>0;i--);		
		clear_output_pin(2);			//LED 2
		clear_output_pin(11);		//GPIO_0 pino 16
		//puts("T1 ");
	}	
}

int main()
{
	int i,j,k;
	
//Debug
asm volatile("addi $26,$0,1212;");
set_output_pin(2);	
	
	
	MemoryWrite(TASK_VECTOR, 0);		//Reset das tarefas ativas
	
	//				  função			task	prio	arg
	//create_task_a(task0, 			0, 	1, '0');	//Idle - Gerador de onda quadrada
	//create_task_a(task1, 			2, 	1, '1');	//Dummy
	
	create_task_a(task4, 			0, 	1, '4');	//Dummy
	create_task_a(task3, 			2, 	1, '3');	//Dummy
	
	//create_task_a(task5, 			0, 	1, 15);	//Dummy
	//create_task_a(task6, 			1, 	1, 15);	//Dummy
	
	// Clock 25MHz
	// TimeSlice(25000);		//1ms			
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	// TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	// TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	// TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	//Clock 1kHz
	j = 300;
	TimeSlice(j);		//1ms	
	puts("---"); puts(itoa10(j)); puts(" clocks/time slice---"); putchar('\r'); putchar('\n'); 
	
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 8 está em 1 roda o scheduler por hardware
	if(read_input_pin(8))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
//		task0('0');
		task4('4');
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0('0');
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD/SW UART VIRTUAL
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/*
int task0(int v)
{
	int i;
	
	putchar('\r'); putchar('\n');
	puts("Task "); putchar((char)v); puts(" principal");
	putchar('\r'); putchar('\n');
	
	while(1)
	{
		//Pisca led
		for(i = 100000;i>0;i--);		
		set_output_pin(2);			//LED 2
		set_output_pin(11);			//GPIO_0 pino 16
		for(i = 100000;i>0;i--);		
		clear_output_pin(2);			//LED 2
		clear_output_pin(11);		//GPIO_0 pino 16
		//puts("T1 ");
	}	
}

//Cria uma UART VIRTUAL para recepção de dados
//Supõe time slice de 20,48us
int taskUART_VIRTUAL_RX(int RXpin)
{
	#define START		0
	#define DATAREAD	1
	#define STOP		2
	
	int state = START, bitcount, dado;

	putchar('\r'); putchar('\n');
	puts("Task UART_VIRTUAL_RX "); putchar(RXpin+'A');
	putchar('\r'); putchar('\n');

	while(1)
	{
		//Máquina de estados com 3 etapas
		//1 - Recebe START
		while(state == START)
		{
			if(read_input_pin(RXpin) == 0)
			{
				//Sleep, tenta ler o próximo bit bem no meio de seu período (2+5 time slices de 20,48us)
				if(read_input_pin(8))
					//sleep_yield_task(7);		//Hardware
					sleep_yield_task(5);		//Hardware
				else
					//sw_sleep_yield_task(7);	//Software
					sw_sleep_yield_task(5);	//Software
				state = DATAREAD;				//Troca de estado
			}
			else
			{
				//Yield, aguarda 1/5 do tempo de um bit para testar novamente
				if(read_input_pin(8))
					sleep_yield_task(0);		//Hardware
				else
					sw_sleep_yield_task(0);	//Software
			}
		}
		
		//2 - Recebe DADO
		//Zera contador contador de bits e o dado recebido
		bitcount = 0; dado = 0;
		while(state == DATAREAD && bitcount != 8)
		{
			dado = dado >> 1;					//Desloca a máscara para o próximo bit
			
			//Se recebeu um bit 1
			if(read_input_pin(RXpin) == 1)
			{
				dado |= 0x80;
			}
			
			bitcount++;							//Incrementa o número de bits recebidos
			
			//Sleep do tempo de um bit, libera processamento para outras tarefas
			if(read_input_pin(8))
				//sleep_yield_task(5);			//Hardware
				sleep_yield_task(4);			//Hardware
			else
				//sw_sleep_yield_task(5);		//Software
				sw_sleep_yield_task(4);		//Software
		}
		state = STOP;							//Troca de estado
		
		//3 - Recebe o STOP
		//Se recebeu um bit 1
		if(read_input_pin(RXpin) == 1)
		{
			if(!fifo_full())
			{
				//Coloca o comando na fifo para tratamento por outra tarefa
				fifo_in((char)dado);
			}
		}
		//Sleep do tempo de um bit, libera processamento para outras tarefas
		//Completa o tempo do bit (3 time slices de 20,48us)
		if(read_input_pin(8))
			sleep_yield_task(3);				//Hardware
		else
			sw_sleep_yield_task(3);			//Software		

		state = START;							//Troca de estado
	}	
}

//Cria uma UART VIRTUAL para transmissão de dados
//Supõe time slice de 20,48us
int taskUART_VIRTUAL_TX(int TXpin)
{
	int bitcount, dado;

	putchar('\r'); putchar('\n');
	puts("Task UART_VIRTUAL_TX "); putchar(TXpin+'A');
	putchar('\r'); putchar('\n');

	while(1)
	{
		//Aguarda um ter um dado para transmitir
		while(fifo_empty())
		{
			//Sleep do tempo de um frame (10 bits)
			if(read_input_pin(8))
				sleep_yield_task(50);		//Hardware
			else
				sw_sleep_yield_task(50);	//Software
		}
		//Lê o dado a ser transmitido
		dado = fifo_out();
		
		//1 - Transmite START
		clear_output_pin(TXpin);
		//Sleep do tempo de um bit
		if(read_input_pin(8))
			sleep_yield_task(5);				//Hardware
		else
			sw_sleep_yield_task(5);			//Software
				
		//2 - Transmite DADO
		//Zera contador de tempo, contador de bits e o dado recebido, define a máscara de bit recebido como 1
		bitcount = 0; 
		dado = 0;
		while(bitcount != 8)
		{
			if(dado & 0x01)
				set_output_pin(TXpin);
			else
				clear_output_pin(TXpin);
						
			dado = dado >> 1;					//Desloca o dado para o próximo teste
									
			bitcount++;							//Incrementa o número de bits recebidos
			
			//Sleep do tempo de um bit
			if(read_input_pin(8))
				sleep_yield_task(5);			//Hardware
			else
				sw_sleep_yield_task(5);		//Software			
		}
				
		//3 - Envia o STOP
		set_output_pin(TXpin);
		//Sleep do tempo de um bit
		if(read_input_pin(8))
			sleep_yield_task(5);				//Hardware
		else
			sw_sleep_yield_task(5);			//Software		
	}	
}

//Cria uma UART para transmissão de dados
int taskUART_TX(int v)
{
	int dado;

	putchar('\r'); putchar('\n');
	puts("Task UART_TX "); putchar((char)v);
	putchar('\r'); putchar('\n');

	while(1)
	{
		while(!fifo_empty())
		{
			dado = fifo_out();
			putchar(dado);
		}
		//Sleep do tempo de um frame
		if(read_input_pin(8))
			sleep_yield_task(50);				//Hardware
		else
			sw_sleep_yield_task(50);			//Software		
	}	
}

int main()
{
	int j;
	
//Debug
asm volatile("addi $26,$0,5555;");
set_output_pin(2);	
	
	
	MemoryWrite(TASK_VECTOR, 0);		//Reset das tarefas ativas
	
	//				  função							task	prio	arg
	create_task_a(task0, 							0, 	1, '0');	//Idle - Gerador de onda quadrada
	
	create_task_a(taskUART_VIRTUAL_RX, 			1, 	3, 17);	//arg = pin (CONECTOR JP1 PINO 38)
	//create_task_a(taskUART_VIRTUAL_TX, 			2, 	1, 11);	//arg = pin
	create_task_a(taskUART_TX,			 			3, 	2, '3');	//
	
	// Clock 25MHz
	// TimeSlice(25000);		//1ms			
	// TimeSlice(128);		//5,12us		- 2^7 usado para interrupção do Contador
	// TimeSlice(256);		//10,24us	- 2^8 usado para interrupção do Contador
	// TimeSlice(512);		//20,48us	- 2^9 usado para interrupção do Contador
	// TimeSlice(1024);		//40,96us	- 2^10 usado para interrupção do Contador
	// TimeSlice(2048);		//81,92us	- 2^11 usado para interrupção do Contador
	// TimeSlice(4096);		//163,84us	- 2^12 usado para interrupção do Contador
	// TimeSlice(8192);		//327,68us	- 2^13 usado para interrupção do Contador
	// TimeSlice(16384);		//655,36us	- 2^14 usado para interrupção do Contador
	// TimeSlice(32768);		//1,31072ms	- 2^15 usado para interrupção do Contador
	// TimeSlice(65536);		//2,62144ms	- 2^16 usado para interrupção do Contador
	// TimeSlice(131072);	//5,24288ms	- 2^17 usado para interrupção do Contador
	// TimeSlice(262144);	//10,48576ms	- 2^18 usado para interrupção do Contador
	
	j = 512;
	TimeSlice(j);
	puts("---"); puts(itoa10(j)); puts(" clocks/time slice---"); putchar('\r'); putchar('\n'); 
	
	putchar('\r'); putchar('\n');
	puts("---Scheduler ON---");
	putchar('\r'); putchar('\n');
	
	//Se a chave 8 está em 1 roda o scheduler por hardware
	if(read_input_pin(8))
	{
		puts("---by Hardware---");
		putchar('\r'); putchar('\n');
		//Hardware------------------------------------------
		EnableScheduler();
	
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
		//o PC atual é salvo como PC da task0.
		task0('0');
		//--------------------------------------------------
	}
	//Senão roda o scheduler por software
	else
	{	
		puts("---by Software---");
		putchar('\r'); putchar('\n');
		//Software------------------------------------------
		//Inicializa o escalonador por software
		sw_scheduler_init();
		//Setup de interrupção
		EnableCounter();
		//Habilita interrupções (coprocessador reg 12 bit 0)
		OS_AsmInterruptEnable(0x01);
		
		//Força o desvio para task0, tarefa idle.
		//Isso é necessário porque quando ocorre a primeira interupção de timer,
		//o SP atual é salvo como SP da task0.
		task0('0');
		//--------------------------------------------------
	}
	
   for(;;);
}
// */

//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
//TESTE HD SCHEDULER COM 4 TAREFAS PARA ASIC
//xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
/*
//Input para UART 
int task3(int v)
{
	int i, ant;
	ant = 0;
	
	putchar('T'); putchar((char)v);	
	while(1)
	{
		i = MemoryRead(GPIOA_IN) & 0x000000FF;
		if(i != ant)
		{
			ant = i;
			putchar((char)i);
		}		
		sleep_yield_task(10);
	}	
}
//UART para Output
int task2(int v)
{
	int i;
	
	putchar('T'); putchar((char)v);

	while(1)
	{
		if(kbhit())
		{
			i = MemoryRead(UART_READ);
			MemoryWrite(GPIO0_CLEAR, 0x000000FF);
			MemoryWrite(GPIO0_SET, 0x000000FF & i);
		}
		sleep_yield_task(10);
	}	
}
//Pisca-pisca por sleep
int task1(int v)
{
	putchar('T'); putchar((char)v);

	while(1)
	{
		//Pisca led
		set_output_pin(9);			//GPIO_0 JP1 pino 14
		sleep_yield_task(1000);	
		clear_output_pin(9);			//GPIO_0 JP1 pino 14
		sleep_yield_task(1000);
	}	
}
//Idle - pisca-pisca por delay
int task0(int v)
{
	int i;

	putchar('T'); putchar((char)v);
	
	while(1)
	{
		//Pisca led
		for(i = 100000;i>0;i--);		
		set_output_pin(8);			//GPIO_0 JP1 pino 13
		for(i = 100000;i>0;i--);		
		clear_output_pin(8);			//GPIO_0 JP1 pino 13
	}	
}

int main()
{
	int j,k;
	
	MemoryWrite(TASK_VECTOR, 0);		//Reset das tarefas ativas
	
	k = (MemoryRead(GPIOA_IN)&0x30)>>8;
	
	//				  função	task	prio	arg
	create_task_a(task0, 0, 	1, 	'0');	//Idle - Gerador de onda quadrada
	create_task_a(task1, 1, 	2, 	'1');
	create_task_a(task2, 2, 	3, 	'2');
	create_task_a(task3, 3, 	3, 	'3');
	
	//Time slice
	switch(k)
	{
		case 0:	j = 25000;	break;	//1ms @ 25MHz
		case 1:	j = 12500;	break;	//500us @ 25MHz
		case 2:	j = 2500;	break;	//100us @ 25MHz
		case 3:	j = 500;		break;	//20us @ 25MHz
		default:	j = 25000;	break;	//1ms @ 25MHz
	}
	TimeSlice(j);
	
	puts("IHM-Plasma by LPD (2018)"); putchar('\r'); putchar('\n'); 
	puts("TS="); putchar(k+'0'); putchar('\r'); putchar('\n'); 
	
	EnableScheduler();
	
	//Força o desvio para task0, tarefa idle.
	//Isso é necessário porque quando ocorre a primeira interupção do microkernel,
	//o PC atual é salvo como PC da task0.
	task0(0);
	//Looping infinito (evita erros caso a tarefa 0 falhe)	
   for(;;);
}
// */