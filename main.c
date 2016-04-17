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

int initializeSharedMemory() {
	sharedMemKey = shmget(IPC_PRIVATE, sizeof(struct SharedMemory), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	if(sharedMemKey == -1)
		return 0;

	sharedMem = (struct SharedSegment*)shmat(sharedMemKey, NULL, 0);

	if(sharedMem == (struct SharedSegment*)-1)
		return 0;

	memset(sharedMem, 0, sizeof(struct SharedMemory));

	return 1;
}

void initializeData() {
	sharedMem->BY->first = 0;
	sharedMem->BY->last = 0;

	sharedMem->BZ->first = 0;
	sharedMem->BZ->last = 0;

	int i;
	for (i = 0; i < 5; i++) {
		sharedMem->BY->values[i] = 0;
		sharedMem->BZ->values[i] = 0;
	}

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

void producer(char type) {
	while (1) {
		int item = produceItem();

		if (type == 'N') {
			sem_wait(&(sharedMem->BYemptyCount));
				sem_wait(&(sharedMem->BYmutex));
					putItemIntoLine(item, sharedMem->BY);
				sem_post(&(sharedMem->BYmutex));
			sem_post(&(sharedMem->BYfillCount));
		}
		else if (type == 'M') {
			sem_wait(&(sharedMem->BYemptyCount));
				sem_wait(&(sharedMem->BYmutex));
					putItemIntoLine(item, sharedMem->BZ);
				sem_post(&(sharedMem->BYmutex));
			sem_post(&(sharedMem->BYfillCount));
		}
		sem_wait(&(sharedMem->Pmutex));
			if (sharedMem->productCount >= TARGET_PRODUCTION)
				return;
		sem_post(&(sharedMem->Pmutex));
	}
}

void assembler() {
	int itemY = -1;
	int itemZ = -1;

	while (1) {
		if (itemY == -1) {
			sem_wait(&(sharedMem->BYfillCount));
				sem_wait(&(sharedMem->BYmutex));
					itemY = removeItemFromLine(sharedMem->BY);
				sem_post(&(sharedMem->BYmutex));
			sem_post(&(sharedMem->BYemptyCount));
		}
		else if (itemZ == -1) {
			sem_wait(&(sharedMem->BZfillCount));
				sem_wait(&(sharedMem->BZmutex));
					itemZ = removeItemFromLine(sharedMem->BZ);
				sem_post(&(sharedMem->BZmutex));
			sem_post(&(sharedMem->BZemptyCount));
		}
		else {
			sem_wait(&(sharedMem->Pmutex));
				assembleItem(itemY, itemZ);
				if (sharedMem->productCount >= TARGET_PRODUCTION)
					return;
			sem_post(&(sharedMem->Pmutex));

			itemY = -1;
			itemZ = -1;
		}
    }
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

	srand(time(NULL));

	int i, child_pid;
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

	return 0;
}
