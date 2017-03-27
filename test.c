/*Queue - Linked List implementation*/
#include<stdio.h>
#include<stdlib.h>
#include "RCB.c"

struct Node {
	struct RCB* data;
	struct Node* next;
};

// Two glboal variables to store address of front and rear nodes. 
struct Node* front = NULL;
struct Node* rear = NULL;

// To Enqueue an integer
void Enqueue(struct RCB* rcb) {
	struct Node* temp =(struct Node*)malloc(sizeof(struct Node));
	temp->data = rcb;
	temp->next = NULL; 

	if(front == NULL && rear == NULL){
		front = rear = temp;
		return;
	}
	
	rear->next = temp;
	rear = temp;
}

// To Dequeue an integer.
struct RCB* Dequeue() {
	struct Node* temp = front;
	struct RCB* ret = front->data;

	if(front == NULL) {
		printf("Queue is Empty\n");
		return;
	}

	if(front == rear) {
		front = rear = NULL;
	} 
	else {
		front = front->next;
	}

	free(temp);
	return ret;
}

struct RCB* Front() {
	if(front == NULL) {
		printf("Queue is empty\n");
		return NULL;
	}

	return front->data;
}

/*
void Print() {
	struct Node* temp = front;
	
	while(temp != NULL) {
		printf("%d ",temp->data);
		temp = temp->next;
	}

	printf("\n");
}

*/

//int main(){ 
	/* Drive code to test the implementation. */
	// Printing elements in Queue after each Enqueue or Dequeue
/*	Enqueue(2); 
	Print();
	Enqueue(4);
	Print();
	Enqueue(6);
	Print();
	Dequeue(); 
	Print();
	Enqueue(8);
	Print();

	return 0;
}*/

int main(){
	return 0;
}
