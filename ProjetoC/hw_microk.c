/*--------------------------------------------------------------------
 * TITLE: Hardware Microkernel Software Defines
 * AUTHOR: Leandro Poloni Dantas (leandro.poloni@gmail.com) 
 * DATE CREATED: 05/08/17
 * FILENAME: hw_microk.c
 * PROJECT: Plasma Real-Time CPU core
 * COPYRIGHT: Software placed into the public domain by the author.
 *    Software 'as is' without warranty.  Author liable for nothing.
 * DESCRIPTION:
 *    Hardware Microkernel Software Defines
 * MODIFICAÇÕES:
 * 05/08/17 - Foram criadas as macros para habilitação, desabilitação
 *				  e definição do time slice do scheduler, funções para 
 *				  inicializar o stack pointer de cada tarefa, sleep e 
 *				  criar tarefa
 * 20/08/17 - Foram criadas as funções paraA função sw_scheduler_init() e sw_scheduler
 *				  para uso de escalonador por software.
 * 24/08/17 - A função sw_scheduler() foi ajustada para receber o stack pointer da
 *				  tarefa interrompida para fazer sua backup e passou a retornar o stack pointer
 *				  da próxima tarefa a ser executada.
 *				  Essa função para ser ajustada para qualquer algoritmo.
 * 03/09/17 - Criei uma função para criação de tarefas com passagem de argumento.
 * 06/09/17 - Ajustei a função de criação de tarefas com passagem de argumento
 *				  para funcionar com escalonamento por software também.
 * 17/09/17 - Alterei a variável estática local "atual" para global, isso permite
 *				  que ele seja reseta ao iniciar o escalonador por software.
 * 30/12/17 - Acrescentei o recurso de sleep_yield_task para o escalonador por software.
 * 04/01/18 - Acrescentei novo algoritmo de escalonamento por software baseado em
 *				  prioridades (sw_scheduler_init_v2() e sw_scheduler_v2()).
 *--------------------------------------------------------------------*/

#include "plasma_rt.h"

#define MemoryRead(A) (*(volatile unsigned int*)(A))
#define MemoryWrite(A,V) *(volatile unsigned int*)(A)=(V)
#define EnableScheduler() *(volatile unsigned int*)(SCHEDULER_EN)=(1)
#define DisableScheduler() *(volatile unsigned int*)(SCHEDULER_EN)=(0)
#define EnableCounter() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)|IRQ_COUNTER18)
#define DisableCounter() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)&~IRQ_COUNTER18)
#define EnableNotCounter() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)|IRQ_COUNTER18_NOT)
#define DisableNotCounter() *(volatile unsigned int*)(IRQ_MASK)=(MemoryRead(IRQ_MASK)&~IRQ_COUNTER18_NOT)
#define TimeSlice(V) *(volatile unsigned int*)(TIME_SLICE)=(V)
#define SP 0
#define PC 1
#define PROX 2
#define SLEEP 3

unsigned int tarefa[32][4];	//[32] tarefas: [0] SP, [1] PC, [2] próxima, [3] sleep
unsigned int atual;				//Armazena o valor da tarefa em execução
unsigned int time_slice;		//Armazena o número do time slice
unsigned int ultima;				//Armazena o valor da última tarefa habilitada

/* int init_task(unsigned int number)
{
	//Stack termina em 0x00010000
	//Cada tarefa tem 64 words (256 bytes) de stack
	//Exemplo da tarefa 0:
	//		lui	$sp,0x0001		#Ponteiro para o stack pointer (High)
	//		addi	$sp,$sp,0xFEFC	#64 words por tarefa				 (Low)
	//Para enterder como usar variáveis C em assembly vide link:
	//https://pt.stackoverflow.com/questions/49892/como-executar-assembly-inline-em-um-c%C3%B3digo-com-vari%C3%A1veis-em-c
	
	//Calcula os 16 lsbs do endereço final do stack (decrescente)
	unsigned int stackp = 0xFFFC - (256*number);
	//Salva o valor no registrador $sp
	asm volatile(
						"lui	$sp,0x0001"
						"addu	$sp,$sp,%1"
						: "=r" (stackp)
						: "b" (stackp)
					);
	return 0;
} */

int sleep_yield_task(unsigned int period)
{
	unsigned int p1,p2;			//Variáveis para gerar o pulso
	p1 = period | 0x00010000;	//Bit 16 em 1, 15..0 valor em time slices
	p2 = period & 0x0000FFFF;	//Bit 16 em 0, 15..0 valor em time slices
	MemoryWrite(SLEEP_TIME,p1);
	MemoryWrite(SLEEP_TIME,p2);
	return 0;
}

int create_task(int (*task_function_pointer)(), int number, int priority)
{
	unsigned int temp,nibble,taskpc,tasksp;
	
	//1a PARTE:	Grava o endereço da função correspondente a tarefa na posição do pogram counter do seu TCB
	taskpc = 0x00020000 + (number*0x0100);	//Endereço de PC no TCB da tarefa em criação
	MemoryWrite(taskpc,(unsigned int)task_function_pointer); //Grava o endereço da tarefa no PC da tarefa
	
	//2a PARTE:	Grava o endereço do stack tarefa na posição do stack pointer do seu TCB
	tasksp = 0x00020074 + (number*0x0100);	//Endereço de SP no TCB da tarefa em criação
	temp = 0x0001FFFC - (256*number);		//Stack decrescente: 0x0001FFFC~0x00010000 (memória de dados)
	MemoryWrite(tasksp,temp); 					//Grava o endereço do SP
	
	//3a PARTE: Configura a prioridade da tarefa
	priority &= 0x00000007;						//8 níveis de prioridade
	priority = priority<<((number%8)*4);	//Rotaciona a prioridade para o nibble corrspondente
	nibble = 0x7<<((number%8)*4);				//Máscara de prioridade
	if(number<8)
	{
		temp = MemoryRead(PRI1_VECTOR);		//Lê o vetor com a prioridade de 8 tarefas
		temp &= ~nibble;							//Zera os bits de prioridade da tarefa
		temp |= priority;							//Atualiza a prioridade da tarefa
		MemoryWrite(PRI1_VECTOR,temp);		//Grava o vetor com a prioridade atualizada		
	}
	else if(number<16)
	{
		temp = MemoryRead(PRI2_VECTOR);		//Lê o vetor com a prioridade de 8 tarefas
		temp &= ~nibble;							//Zera os bits de prioridade da tarefa
		temp |= priority;							//Atualiza a prioridade da tarefa
		MemoryWrite(PRI2_VECTOR,temp);		//Grava o vetor com a prioridade atualizada		
	}
	else if(number<24)
	{
		temp = MemoryRead(PRI3_VECTOR);		//Lê o vetor com a prioridade de 8 tarefas
		temp &= ~nibble;							//Zera os bits de prioridade da tarefa
		temp |= priority;							//Atualiza a prioridade da tarefa
		MemoryWrite(PRI3_VECTOR,temp);		//Grava o vetor com a prioridade atualizada		
	}
	else if(number<32)
	{
		temp = MemoryRead(PRI4_VECTOR);		//Lê o vetor com a prioridade de 8 tarefas
		temp &= ~nibble;							//Zera os bits de prioridade da tarefa
		temp |= priority;							//Atualiza a prioridade da tarefa
		MemoryWrite(PRI4_VECTOR,temp);		//Grava o vetor com a prioridade atualizada		
	}
	
	//4a PARTE:	Indica que a tarefa existe (live ou enable)
	temp = 1<<number;								//Máscara de enable da tarefa
	temp |= MemoryRead(TASK_VECTOR);			//Atualiza o vetor de tarefas
	MemoryWrite(TASK_VECTOR,temp); 			//Habilita a tarefa
	
	//5a PARTE: Grava o endereço da função correspondente a tarefa na matriz tarefa
	//>>>> Somente para scheduler por software <<<<
	tarefa[number][PC] = (unsigned int)task_function_pointer;
//Debug
//puts("tarefa["); putchar(number+'0'); puts("][PC] = "); puts(itoa10(tarefa[number][PC])); putchar('\r'); putchar('\n');
	
	return 0;
}

int create_task_a(int (*task_function_pointer)(), int number, int priority, int argumento)
{
	unsigned int temp,nibble,taskpc,tasksp,taska0;

//Debug
//asm volatile("addi $26,$0,2233;");
	
	//1a PARTE:	Grava o endereço da função correspondente a tarefa na posição do pogram counter do seu TCB
	taskpc = 0x00020000 + (number*0x0100);	//Endereço de PC no TCB da tarefa em criação
	MemoryWrite(taskpc,(unsigned int)task_function_pointer); //Grava o endereço da tarefa no PC da tarefa
	
	//2a PARTE:	Grava o endereço do stack da tarefa na posição do stack pointer do seu TCB
	tasksp = 0x00020074 + (number*0x0100);	//Endereço de SP no TCB da tarefa em criação
	temp = 0x0001FFFC - (256*number);		//Stack decrescente: 0x0001FFFC~0x00010000 (memória de dados)
	MemoryWrite(tasksp,temp); 					//Grava o endereço do SP
	
	//3a PARTE: Configura a prioridade da tarefa
	priority &= 0x00000007;						//8 níveis de prioridade
	priority = priority<<((number%8)*4);	//Rotaciona a prioridade para o nibble corrspondente
	nibble = 0x7<<((number%8)*4);				//Máscara de prioridade
	if(number<8)
	{
		temp = MemoryRead(PRI1_VECTOR);		//Lê o vetor com a prioridade de 8 tarefas
		temp &= ~nibble;							//Zera os bits de prioridade da tarefa
		temp |= priority;							//Atualiza a prioridade da tarefa
		MemoryWrite(PRI1_VECTOR,temp);		//Grava o vetor com a prioridade atualizada		
	}
	else if(number<16)
	{
		temp = MemoryRead(PRI2_VECTOR);		//Lê o vetor com a prioridade de 8 tarefas
		temp &= ~nibble;							//Zera os bits de prioridade da tarefa
		temp |= priority;							//Atualiza a prioridade da tarefa
		MemoryWrite(PRI2_VECTOR,temp);		//Grava o vetor com a prioridade atualizada		
	}
	else if(number<24)
	{
		temp = MemoryRead(PRI3_VECTOR);		//Lê o vetor com a prioridade de 8 tarefas
		temp &= ~nibble;							//Zera os bits de prioridade da tarefa
		temp |= priority;							//Atualiza a prioridade da tarefa
		MemoryWrite(PRI3_VECTOR,temp);		//Grava o vetor com a prioridade atualizada		
	}
	else if(number<32)
	{
		temp = MemoryRead(PRI4_VECTOR);		//Lê o vetor com a prioridade de 8 tarefas
		temp &= ~nibble;							//Zera os bits de prioridade da tarefa
		temp |= priority;							//Atualiza a prioridade da tarefa
		MemoryWrite(PRI4_VECTOR,temp);		//Grava o vetor com a prioridade atualizada		
	}
	
	//4a PARTE:	Indica que a tarefa existe (live ou enable)
	temp = 1<<number;								//Máscara de enable da tarefa
	temp |= MemoryRead(TASK_VECTOR);			//Atualiza o vetor de tarefas
	MemoryWrite(TASK_VECTOR,temp); 			//Habilita a tarefa
	
	//5a PARTE: Grava o endereço da função correspondente a tarefa na matriz tarefa
	//>>>> Somente para scheduler por software <<<<
	tarefa[number][PC] = (unsigned int)task_function_pointer;
//Debug
//puts("tarefa["); putchar(number+'0'); puts("][PC] = "); puts(itoa10(tarefa[number][PC])); putchar('\r'); putchar('\n');

//Debug
//asm volatile("addi $26,$0,3322;");
	
	//6a PARTE: Grava o endereço do argumento da tarefa na posição de $a0 do seu TCB
	taska0 = 0x00020010 + (number*0x0100);	//Endereço de $a0 no TCB da tarefa em criação
	MemoryWrite(taska0,argumento); //Grava o valor de argumento tarefa no $a0 da tarefa
	
	//7a PARTE: Grava o endereço do argumento da tarefa na posição de $a0 do seu stack
	//>>>> Somente para scheduler por software <<<<
	taska0 = 0x0001FFFC - (256*number) - 136 + 28;		//Stack decrescente: 0x0001FFFC~0x00010000 (memória de dados)
	MemoryWrite(taska0,argumento); //Grava o valor de argumento tarefa no $a0 da tarefa
	
	return 0;
}

//Inicializa escalonador definindo os SPs e próximas tarefas
void sw_scheduler_init(void)
{
	unsigned int i, j;
	unsigned int temp;
	
	atual = 0;	//Define que a tarefa de inicial é sempre a task0;
	time_slice = 0;	//Define que ao iniciar o escalonador, estamos no time slice número 0;
	
//Debug
puts("sw_scheduler_init"); putchar('\r'); putchar('\n');
	
	//Lê TASK_VECTOR para verificar as tarefas habilitadas
	temp = MemoryRead(TASK_VECTOR);
	
	//Define o SP de cada tarefa
	for(i=0; i<32; i++)
	{
		//Grava o endereço do SP
		//tarefa[i][0] = 0x00020000 + ((i+1)*0x0100);	//Stack Pointer conta invertido
		tarefa[i][SP] = 0x0001FFFC - (256*i);		//Stack decrescente: 0x0001FFFC~0x00010000 (memória de dados)
//Debug
//puts("tarefa["); putchar(i+'0'); puts("][SP] = "); puts(itoa10(tarefa[i][SP])); putchar('\r'); putchar('\n');
		//Ajusta o SP ao atendimento de interrupção
		tarefa[i][SP] = tarefa[i][SP] - 104 - 32;
//Debug
//puts("tarefa["); puts(itoa10(i)); puts("][SP] = "); puts(itoa10(tarefa[i][SP])); putchar('\r'); putchar('\n');
		//Grava o número da próxima tarefa
		tarefa[i][PROX] = 0;
		//Grava o endereço do PC
		//MemoryWrite(tarefa[i][SP]-104+88,tarefa[i][PC]);
		MemoryWrite(tarefa[i][SP] + 88, tarefa[i][PC]);
	}
	//Define a ordem de execução das tarefas com base na tarefa em execução
	//Esse é o algoritmo de escalonamento mais simples possível, onde as prioridades
	//não são consideradas
	for(i=0; i<32; i++)
	{
		//Grava número seguencial para próxima tarefa
		//Se a tarefa existe
		if(temp & (1<<i))
		{
			//Procuar a próxima tarefa existente
			for(j=i+1; j<32; j++)
			{
				//Se a tarefa existe
				if(temp & (1<<j))
				{
					//Define essa tarefa como o próxima da anterior em ordem crescente
					tarefa[i][PROX] = j;
					break;
				}
			}
///Debug
puts("tarefa["); putchar(i+'0'); puts("][SP] = "); puts(itoa10(tarefa[i][SP])); putchar('\r'); putchar('\n');
puts("tarefa["); putchar(i+'0'); puts("][PC] = "); puts(itoa10(tarefa[i][PC])); putchar('\r'); putchar('\n');
puts("tarefa["); putchar(i+'0'); puts("][PROX] = "); putchar(tarefa[i][PROX]+'0'); putchar('\r'); putchar('\n');
		}
	}	
}

//Inicializa escalonador definindo os SPs e próximas tarefas
//Considera que as tarefas são criadas em ordem de prioridade crescente e
//define que a PROX será sempre a com prioridade anferior ou igual
void sw_scheduler_init_v2(void)
{
	unsigned int i, j;
	unsigned int temp;
	
	atual = 0;	//Define que a tarefa de inicial é sempre a task0;
	time_slice = 0;	//Define que ao iniciar o escalonador, estamos no time slice número 0;
	
//Debug
puts("sw_scheduler_init_v2"); putchar('\r'); putchar('\n');
	
	//Lê TASK_VECTOR para verificar as tarefas habilitadas
	temp = MemoryRead(TASK_VECTOR);
	
	//Define o SP de cada tarefa
	for(i=0; i<32; i++)
	{
		//Grava o endereço do SP
		//tarefa[i][0] = 0x00020000 + ((i+1)*0x0100);	//Stack Pointer conta invertido
		tarefa[i][SP] = 0x0001FFFC - (256*i);		//Stack decrescente: 0x0001FFFC~0x00010000 (memória de dados)
//Debug
//puts("tarefa["); putchar(i+'0'); puts("][SP] = "); puts(itoa10(tarefa[i][SP])); putchar('\r'); putchar('\n');
		//Ajusta o SP ao atendimento de interrupção
		tarefa[i][SP] = tarefa[i][SP] - 104 - 32;
//Debug
//puts("tarefa["); puts(itoa10(i)); puts("][SP] = "); puts(itoa10(tarefa[i][SP])); putchar('\r'); putchar('\n');
		//Grava o número da próxima tarefa
		tarefa[i][PROX] = 0;
		//Grava o endereço do PC
		//MemoryWrite(tarefa[i][SP]-104+88,tarefa[i][PC]);
		MemoryWrite(tarefa[i][SP] + 88, tarefa[i][PC]);
	}
	//Define a ordem de execução das tarefas com base na tarefa em execução
	//Supõe que a as tarefas mais elevadas tem prioridades maiores
	for(i=31; i>0; i--)
	{
		//Grava número seguencial para próxima tarefa
		//Se a tarefa existe
		if(temp & (1<<i))
		{
			//Procuar a próxima tarefa existente
			for(j=i-1; j>=0; j--)
			{
				//Se a tarefa existe
				if(temp & (1<<j))
				{
					//Define essa tarefa como o próxima da anterior em ordem decrescente
					tarefa[i][PROX] = j;
					break;
				}
			}
//Debug
puts("tarefa["); puts(itoa2(i)); puts("][SP] = "); 	puts(itoa10(tarefa[i][SP])); 	putchar('\r'); putchar('\n');
puts("tarefa["); puts(itoa2(i)); puts("][PC] = "); 	puts(itoa10(tarefa[i][PC])); 	putchar('\r'); putchar('\n');
puts("tarefa["); puts(itoa2(i)); puts("][PROX] = "); 	puts(itoa2(tarefa[i][PROX]));	putchar('\r'); putchar('\n');
		}
	}
	//Procura a última tarefa habilitada
	for(i=31; i>0; i--)
	{
		//Grava número seguencial para próxima tarefa
		//Se a tarefa existe
		if(temp & (1<<i))
		{
			ultima = i;
//Debug
puts("ultima tarefa = "); puts(itoa2(i)); putchar('\r'); putchar('\n');
			break;
		}
	}	
}

//Realiza os escalonamento preemptivo
unsigned int sw_scheduler(unsigned int stackp)
{
	//	static unsigned int atual = 0;	//Era local virou global para poder ser resetada
	
	//Incrementa o time slice a cada interrupção "real" por timer
	//Quando SLEEP > time_slice é sinal que a tarefa fez um pedido de sleep usando
	//a função sw_sleep_yield_task(unsigned int period)
	if(tarefa[atual][SLEEP] <= time_slice)
		time_slice++;
		
//Debug
//puts("sw_scheduler"); putchar('\r'); putchar('\n');
	
	//Atualiza o SP da tarefa interrompida
	tarefa[atual][SP] = stackp;
	
	//Atualiza a tarefa atual
	atual = tarefa[atual][PROX];
	//Enquanto o próxima selacionada tem sleep maior que o time slice,
	//ela não pode ser habilitada
	while(tarefa[atual][SLEEP] > time_slice)
	{
		atual = tarefa[atual][PROX];
	}
	//Zera o sleep da tarefa atual para evitar problemas quando o time slice é resetado
	tarefa[atual][SLEEP] = 0;

//Debug
//puts("Prox T="); putchar(atual+'0'); putchar('\r'); putchar('\n');

	//Carrega o novo valor do SP ( $29 = tarefa[atual][SP] )
	stackp = tarefa[atual][SP];
//Debug
// asm volatile("addi $26,$0,1111;");
					

	//Carrega o novo valor do SP ( $29 = tarefa[atual][0] )
//Debug
//asm volatile("addi $26,$0,1111;");
	// asm volatile("move $v0,%0;"
					 // "move %0, $v0;"
					 // : "=r"(aux)
					 // : "b" (tarefa[atual][SP])
					// );

	return stackp;
}

//Realiza os escalonamento por prioridade e baseado no tempo de sleep de cada tarefa
//Não faz preempção quando a tarefa em execução tem ordenação superior a um próxima
//com mesma prioridade
unsigned int sw_scheduler_v2(unsigned int stackp)
{
	//	static unsigned int atual = 0;	//Era local virou global para poder ser resetada
	
	//Incrementa o time slice a cada interrupção "real" por timer
	//Quando SLEEP > time_slice é sinal que a tarefa fez um pedido de sleep usando
	//a função sw_sleep_yield_task(unsigned int period)
	if(tarefa[atual][SLEEP] <= time_slice)
		time_slice++;
		
//Debug
//puts("sw_scheduler_v2"); putchar('\r'); putchar('\n');
	
	//Atualiza o SP da tarefa interrompida
	tarefa[atual][SP] = stackp;
	
	//Atualiza a tarefa atual
	//Começa a busca pela última, que possui maior prioridade
	atual = ultima;
	//Enquanto o próxima selacionada tem sleep maior que o time slice,
	//ela não pode ser habilitada
	while(tarefa[atual][SLEEP] > time_slice)
	{
		atual = tarefa[atual][PROX];
	}
	//Zera o sleep da tarefa atual para evitar problemas quando o time slice é resetado
	tarefa[atual][SLEEP] = 0;

//Debug
//puts("Prox T="); putchar(atual+'0'); putchar('\r'); putchar('\n');

	//Carrega o novo valor do SP ( $29 = tarefa[atual][SP] )
	stackp = tarefa[atual][SP];
//Debug
// asm volatile("addi $26,$0,1111;");
					

	//Carrega o novo valor do SP ( $29 = tarefa[atual][0] )
//Debug
//asm volatile("addi $26,$0,1111;");
	// asm volatile("move $v0,%0;"
					 // "move %0, $v0;"
					 // : "=r"(aux)
					 // : "b" (tarefa[atual][SP])
					// );

	return stackp;
}

//Realiza o sleep ou yield de tarefa por software
unsigned int sw_sleep_yield_task(unsigned int period)
{
	//Salva o número do time slice que a tarefa pode voltar a rodar
	tarefa[atual][SLEEP] = time_slice + period + 1;
	//ATENÇÃO: PODE DAR INCONSISTÊNCIA QUANDO A SOMA É MAIOR QUE 0xFFFFFFFF
	//PENSAR NO QUE PODE SER FEITO

//Debug
//puts("T="); puts(itoa2(atual)); putchar('\r'); putchar('\n');
//puts("S="); puts(itoa10(tarefa[atual][SLEEP])); putchar('\r'); putchar('\n');
//puts("TS="); puts(itoa10(time_slice)); putchar('\r'); putchar('\n');
	
	//Se é interrupção de COUNTER
	if(MemoryRead(IRQ_MASK) & IRQ_COUNTER18)
	{
		//Inverte o tipo de interrupção
		DisableCounter();		EnableNotCounter();
	}
	//Se é interrupção de NOT_COUNTER
	else if(MemoryRead(IRQ_MASK) & IRQ_COUNTER18_NOT)
	{
		//Inverte o tipo de interrupção
		DisableNotCounter();	EnableCounter();
	}
	
	return 0;
}


