#include <stdio.h>
#include <stdlib.h>

struct RCB{
  int sequenceNumber;
  int fileDescriptor;
  char *fileName;
  int bytesRemaining;
  int byteQuantum;
};

struct Queue {
  struct Node* front;
  struct Node* rear;
};

struct Node {
        struct RCB data;
        struct Node* next;
};

// To Enqueue a node
void Enqueue(struct Queue *queue, struct RCB newRCB) {
        struct Node* temp = malloc(sizeof(struct Node));
        temp->data = newRCB;
        temp->next = NULL;
        if(queue->front == NULL && queue->rear == NULL){
                queue->front = queue->rear = temp;
                return;
        }
        queue->rear->next = temp;
        queue->rear = temp;
}

// To Dequeue a node.
struct RCB Dequeue(struct Queue *queue) {
        struct Node* temp = queue->front;
        if(queue->front == NULL) {
                printf("Queue is Empty\n");
                return;
        }
        if(queue->front == queue->rear) {
                queue->front = queue->rear = NULL;
		return temp->data;
        }
        else {
                queue->front = queue->front->next;
		return temp->data;
        }
}

void Insert(struct Queue *queue, struct RCB newRCB){
	struct Node *newNode = malloc(sizeof(struct Node));
        newNode->data = newRCB;

   	if(queue->front == NULL){
		queue->front = newNode;
		newNode->next = NULL;
	}
	else{
		if(queue->front->data.bytesRemaining > newRCB.bytesRemaining){
			newNode->next = queue->front;
			queue->front = newNode;
		}
		else{
			struct Node *current = queue->front;

			while(1){
				if(current->next == NULL){
					current->next = newNode;
					newNode->next = NULL;
					break;
				}
				else{
					if(current->next->data.bytesRemaining > newRCB.bytesRemaining){
						newNode->next = current->next;
						current->next = newNode;
						break;
					}
					else{
						current = current->next;
					}
				}
			}
		}
	}

}
