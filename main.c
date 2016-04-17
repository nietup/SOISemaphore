#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

const int LINE_SIZE = 5;
const int TARGET_PRODUCTION = 10;

struct AssemblyLine {
	int values[5];
	int first;
	int last;
};

struct SharedMemory {
	sem_t BYmutex;
	sem_t BYfillCount;
	sem_t BYemptyCount;
	struct AssemblyLine * BY;

	sem_t BZmutex;
	sem_t BZfillCount;
	sem_t BZemptyCount;
	struct AssemblyLine * BZ;

	sem_t Pmutex;
	int productCount;
} * sharedMem;
int sharedMemKey;

void initializeSemaphores() {
	sem_init(&(sharedMem->BYmutex), 1, 1);
	sem_init(&(sharedMem->BYfillCount), 1, 0);
	sem_init(&(sharedMem->BYemptyCount), 1, LINE_SIZE);

	sem_init(&(sharedMem->BZmutex), 1, 1);
	sem_init(&(sharedMem->BZfillCount), 1, 0);
	sem_init(&(sharedMem->BZemptyCount), 1, LINE_SIZE);

	sem_init(&(sharedMem->Pmutex), 1, 1);
}

void destroySemaphores() {
	sem_destroy(&(sharedMem->BYmutex));
	sem_destroy(&(sharedMem->BYfillCount));
	sem_destroy(&(sharedMem->BYemptyCount));
	sem_destroy(&(sharedMem->BZmutex));
	sem_destroy(&(sharedMem->BZfillCount));
	sem_destroy(&(sharedMem->BZemptyCount));
	sem_destroy(&(sharedMem->Pmutex));
}

int initializeSharedMemory() {
	sharedMemKey = shmget(IPC_PRIVATE, sizeof(struct SharedMemory), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	if(sharedMemKey == -1)
		return 0;

	sharedMem = (struct SharedMemory*)shmat(sharedMemKey, NULL, 0);

	if(sharedMem == (struct SharedMemory*)-1)
		return 0;

	memset(sharedMem, 0, sizeof(struct SharedMemory));

	return 1;
}

void freeSharedMemory() {
	shmdt(sharedMem);
	shmctl(sharedMemKey, IPC_RMID, NULL);
}

void initializeData() {printf("debug 0\n");
	sharedMem->BY->first = 0;
	sharedMem->BY->last = 0;

	sharedMem->BZ->first = 0;
	sharedMem->BZ->last = 0;
printf("debug 1\n");
	int i;
	for (i = 0; i < 5; i++) {
		sharedMem->BY->values[i] = 0;
		sharedMem->BZ->values[i] = 0;
	}
printf("debug 2\n");
	sharedMem->productCount = 0;
}

int produceItem() {
	return rand() % 10;
}

void putItemIntoLine(int item, struct AssemblyLine * line) {
	line->last++;
	line->last = line->last % 5;
	line->values[line->last] = item;
}

int removeItemFromLine(struct AssemblyLine * line) {
	line->first++;
	int result = line->values[line->first-1];
	line->first = line->first % 5;
	return result;
}

void assembleItem(int itemY, int itemZ) {
	sharedMem->productCount++;
	printf("Item No. %d; Item value: %d\n", sharedMem->productCount, 10*itemY+itemZ);
}

int producer(char type) {
	printf("Starting robot type %c\n", type);

	while (1) {
		int item = produceItem();
		printf("Locked here type %c - step 1\n", type);

		if (type == 'N') {printf("Locked here type %c - step 2\n", type);
			sem_wait(&(sharedMem->BYemptyCount));printf("Locked here type %c - step 3\n", type);
				sem_wait(&(sharedMem->BYmutex));printf("Locked here type %c - step 4\n", type);
					printf("Putting item %d to line BY\n", item);
					putItemIntoLine(item, sharedMem->BY);
				sem_post(&(sharedMem->BYmutex));
			sem_post(&(sharedMem->BYfillCount));
		}
		else if (type == 'M') {printf("Locked here type %c - step 2\n", type);
			sem_wait(&(sharedMem->BYemptyCount));printf("Locked here type %c - step 3\n", type);
				sem_wait(&(sharedMem->BYmutex));printf("Locked here type %c - step 4\n", type);
					printf("Putting item %d to line BZ\n", item);
					putItemIntoLine(item, sharedMem->BZ);
				sem_post(&(sharedMem->BYmutex));
			sem_post(&(sharedMem->BYfillCount));
		}
		sem_wait(&(sharedMem->Pmutex));
			if (sharedMem->productCount >= TARGET_PRODUCTION) {
				printf("Terminating robot type %c\n", type);
				return 0;
			}
		sem_post(&(sharedMem->Pmutex));
	}
}

int assembler() {
	int itemY = -1;
	int itemZ = -1;

	printf("Starting robot type P\n");

	while (1) {
		printf("Locked here type P - step 1\n");
		
		if (itemY == -1) {printf("Locked here type P - step 1.5\n");
			sem_wait(&(sharedMem->BYfillCount));printf("Locked here type P - step 1.57\n");
				sem_wait(&(sharedMem->BYmutex));
				printf("Locked here type P - step 1.625\n");
					itemY = removeItemFromLine(sharedMem->BY);
					printf("Locked here type P - step 1.7\n");
				sem_post(&(sharedMem->BYmutex));
			sem_post(&(sharedMem->BYemptyCount));
			printf("Locked here type P - step 1.75\n");
		}
		else if (itemZ == -1) {printf("Locked here type P - step 2\n");
			sem_wait(&(sharedMem->BZfillCount));
				sem_wait(&(sharedMem->BZmutex));
					itemZ = removeItemFromLine(sharedMem->BZ);
				sem_post(&(sharedMem->BZmutex));
			sem_post(&(sharedMem->BZemptyCount));
		}
		else {printf("Locked here type P - step 3\n");
			sem_wait(&(sharedMem->Pmutex));
				assembleItem(itemY, itemZ);
				if (sharedMem->productCount >= TARGET_PRODUCTION) {
					printf("Terminating robot type P\n");
					return 0;
				}
			sem_post(&(sharedMem->Pmutex));

			itemY = -1;
			itemZ = -1;
		}
		printf("Locked here type P - step 4s\n");
    }
    printf("Locked here type P - step 5\n");
}

int main(int argc, char * argv[]) {

	int m, n, p, r, s, t;
	if (argc != 7) {
		m = 1;
		n = 1;
		p = 1;
		r = 1;
		s = 1;
		t = 1;
	}
	else
	{
		m = atoi(argv[1]);
		n = atoi(argv[2]);
		p = atoi(argv[3]);
		r = atoi(argv[4]);
		s = atoi(argv[5]);
		t = atoi(argv[6]);
	}

	printf("Initializing shared memory\n");
	initializeSharedMemory();

	printf("Initializing semaphores\n");
	initializeSemaphores();
	
	printf("Initializing data\n");
	initializeData();
	
	printf("Starting production\n");

	srand(time(NULL));

	int i, child_pid, status;
	for(i=0; i<m; i++)
		if((child_pid = fork()) == 0)
			return producer('M');
		else 
			printf("Created robot type M, pid: %d\n",child_pid);

	for(i=0; i<n; i++)
		if((child_pid = fork()) == 0)
			return producer('N');
		else
			printf("Created robot type N, pid: %d\n",child_pid);

	for(i=0; i<p; i++)
		if((child_pid = fork()) == 0)
			return assembler();
		else
			printf("Created robot type P, pid: %d\n",child_pid);

	//wait(&status);

	printf("Terminating\n");
	freeSharedMemory();
	destroySemaphores();
	return 0;
}
