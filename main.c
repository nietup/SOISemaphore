#include <stdio.h>

semaphore mutex = 1;
semaphore fillCount = 0; // items produced
semaphore emptyCount = BUFFER_SIZE; // remaining space

void producer() {
    while (1) {
        item = produceItem();
        down(emptyCount);
        	down(mutex);
        		putItemIntoBuffer(item);
        	up(mutex);
        up(fillCount);
    }
}

void consumer() {
    while (1) {
        down(fillCount);
        	down(mutex);
        		item = removeItemFromBuffer();
        	up(mutex);
        	up(emptyCount);
        consumeItem(item);
    }
}

int main(void) {
	printf("dupa");

	return 0;
}
