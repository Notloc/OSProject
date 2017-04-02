#include <stdio.h>
#include <stdlib.h>

struct fdQueue {
  struct Node* front;
  struct Node* rear;
};

struct fdNode {
        int data;
        struct Node* next;
};

// To Enqueue a node
void fdEnqueue(struct fdQueue *queue, int number) {
        struct fdNode* temp = malloc(sizeof(struct fdNode));
        temp->data = number;
        temp->next = NULL;
        if(queue->front == NULL && queue->rear == NULL){
                queue->front = queue->rear = temp;
                return;
        }
        queue->rear->next = temp;
        queue->rear = temp;
}

// To Dequeue a node.
int fdDequeue(struct fdQueue *queue) {
        struct fdNode* temp = queue->front;
        if(queue->front == NULL) {
                return -1;
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

