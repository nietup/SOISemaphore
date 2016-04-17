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

void initializeSemaphores() {
	sem_init(&(sharedMem->BYmutex), 1, 1);
	sem_init(&(sharedMem->BYfillCount), 1, 0);
	sem_init(&(sharedMem->BYemptyCount), 1, LINE_SIZE);

	sem_init(&(sharedMem->BZmutex), 1, 1);
	sem_init(&(sharedMem->BZfillCount), 1, 0);
	sem_init(&(sharedMem->BZemptyCount), 1, LINE_SIZE);

	sem_init(&(sharedMem->Pmutex), 1, 1);
}

void initializeSharedMemory() {

}

void initialize() {
	initializeSemaphores();
	initializeSharedMemory();

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

void putItemIntoLine(int item, struct AssemblyLine * line) {
	line->last++;
	line->values[line->last] = item;
}

int removeItemFromLine(struct AssemblyLine * line) {
	line->first++;
	return line->values[line->first-1];
}

void assembleItem(int itemY, int itemZ) {
	sharedMem->productCount++;
	printf("Item No. %d; Item value: %d", sharedMem->productCount, itemY*itemZ);
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
	}
}

void assembler() {
	int itemY = -1;
	int itemZ = -1;

	while (1) {
		if (itemY == -1) {
			sem_wait(&(sharedMem->BYfillCount));
				sem_wait(&(sharedMem->BYmutex));
					int item = removeItemFromLine();
				sem_post(&(sharedMem->BYmutex));
			sem_post(&(sharedMem->BYemptyCount));
		}
		else if (itemZ == -1) {
			sem_wait(&(sharedMem->BZfillCount));
				sem_wait(&(sharedMem->BZmutex));
					int item = removeItemFromLine();
				sem_post(&(sharedMem->BZmutex));
			sem_post(&(sharedMem->BZemptyCount));
		}
		else {
			sem_wait(&(sharedMem->Pmutex));
				assembleItem(itemY, itemZ);
			sem_post(&(sharedMem->Pmutex));

			itemY = -1;
			itemZ = -1;
		}
    }
}

int main(void) {
	int m, n, p, r, s, t;
	m = 1;
	n = 1;
	p = 1;
	r = 1;
	s = 1;
	t = 1;



	return 0;
}
