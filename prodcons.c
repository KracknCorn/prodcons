#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <linux/unistd.h>

#define __NR_sys_cs1550_down 325
#define __NR_sys_cs1550_up 326

struct cs1550_sem {
	int data;
	struct cs_node *head;
	struct cs_node *tail;
};

void down(struct cs1550_sem *sem);
void up(struct cs1550_sem *sem);
int validate_args(int argc, char **argv);

int main(int argc, char *argv[])
{
	int prods; 		/* the number of producers */
	int cons; 		/* the number of consumers */
	int item_lim; 	/* number of items in buff */
	
	/*cmd-line validation*/
	if (validate_args(argc, argv) == 0) {
		printf("Invalid command line args\n");
		exit(-1);
	} 
	else { /* args were legit; set basic variables */
		prods 		= atoi(argv[1]); 
		cons 		= atoi(argv[2]);
		item_lim 	= atoi(argv[3]);
	}
	/* allocate memory for buffer */
	int *buff = (int*)mmap(NULL, sizeof(int) * item_lim, PROT_READ|PROT_WRITE,
			MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	memset(buff, 0, sizeof(buff));
	/* allocate memory for the counting variables "produced" and "consumed" */
	int *var_mem = (int*)mmap(NULL, sizeof(int) * 2, PROT_READ|PROT_WRITE,
			MAP_SHARED|MAP_ANONYMOUS, 0,0);
	int *produced 	= var_mem;
	*produced 		= 0;	// initially 0

	int *consumed 	= var_mem + 1;
	*consumed		= 0;	// initially 0

	/* allocate memory for the semaphores */
	struct cs1550_sem *sem_mem =
		(struct cs1550_sem*)mmap(NULL, sizeof (struct cs1550_sem)*3, 
		PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);

	struct cs1550_sem *empty 	= sem_mem;
	struct cs1550_sem *full 	= sem_mem + 1;
	struct cs1550_sem *mutex	= sem_mem + 2;
	
	/*
	   init values of semaphores:
	   	mutex needs to be initially unlocked;
		empty needs to be initially full or else no producer 
		could produce
	 */
	mutex->data = 1;
	mutex->head = NULL;
	mutex->tail = NULL;

	/*
	   there are initially item_lim empty spaces in the buffer, 
	   so as the buffer is filled up this is decremented by the producers;
	   conversely, the number of empty spaces increases as consumers increment
	   it upon consumption
	 */
	empty->data = item_lim;
	empty->head = NULL;
	empty->tail = NULL;

	/*
	 	there are initially zero full spaces in the buffer for no item has
		been placed there yet.

		PRODUCERS:
			as producers produce items, they increment full to reflect the number
			of spaces in the buffer that are occupied
		CONSUMERS:
		 	as consumers consume items, they decrement full to reflect the number
			of spaces in th buffer that are occupied
	 */
	full->data = 0;
	full->head = NULL;
	full->tail = NULL;
	
	int i;
	/*
	 forking of producer threads
	 */
	for (i = 0; i < prods; i++) { /*create the producers*/
		if (fork() == 0) {
			int curr_item;
			while (1) {
				/* put down the empty count */
				down(empty);
				/* obtain mutex */
				down(mutex);

				/* do put an item in the buffer */
				curr_item = *produced;
				buff[*produced % item_lim] = curr_item;

				/* display which item was put in the buffer */
				printf("chef %c produced: pancake%d \n", 65+i, curr_item);

				/* increment the count produced */
				*produced += 1;

				/* release the mutex */
				up(mutex);
				/* put buffer count up */ 
				up(full);
			}
		}
	}
	
	/*
	 forking of consumer threads
	 */
	for (i = 0; i < cons; i++) { /* create the consumers */
		if (fork() == 0) {
			int curr_item;
			while (1) {
				/* put down full count */
				down(full);

				/* obtain mutex */
				down(mutex);
				
				/* consume the item in the buffer */
				curr_item = buff[*consumed % item_lim];

				/* display the item consumed */
				printf("customer %c consumed: pancake%d\n", 65+i, curr_item);
				
				/* increment the consumed count */
				*consumed += 1;

				/* release the mutex */
				up(mutex);

				/* put empty count up */ 
				up(empty);
			}
		}
	}
	int status;
	wait(&status);
	return 0;
}

void down(struct cs1550_sem *sem) 
{
	syscall(__NR_sys_cs1550_down, sem);
}

void up(struct cs1550_sem *sem) 
{
	syscall(__NR_sys_cs1550_up, sem);
}

int validate_args(int argc, char **argv) 
{
	if (argc != 4) {
		return 0;
	}

	int p = atoi(argv[1]);
	int c = atoi(argv[2]);
	int b = atoi(argv[3]);

	if ((p <= 0) || (c <= 0) || (b <= 0)) {
		return 0;
	} 
	else {
		return 1;
	}
}
