#include "mythread.h"
#include <malloc.h>
#include <ucontext.h>
#include <stdlib.h>

#define STACK_SIZE 1024*64


typedef struct Node {
	ucontext_t *element;
	int nodeId;
	int isBlocking;
	int isJoined;
	int hasYielded;
	int waitOnSemaphore;
	struct Node *child;
	struct Node *otherChild;
	struct Node *parent;
	struct Node *next;
} Node;

//Queue 
typedef struct Queue{
	Node* front;
	Node* rear;
	int isEmpty;
} Queue;

typedef struct SemaphoreNode{
	int s;
	//int isAlive;
	int runningCount;
	Queue semaphore_Queue;	
}SemaphoreNode;



Queue ready_Queue = {NULL, NULL, -1};

Queue block_Queue = {NULL, NULL, -1};

ucontext_t markComplete, runningContext;
ucontext_t *mainContext;

Node *currentNode;
Node *parentNode;

int uniqueId;

void Enqueue(Node *con, Queue *queue) {
	if(queue->front == NULL && queue->rear == NULL){
		queue->front = queue->rear = con;
		queue->isEmpty = 0;
		con->next = NULL;
		return;
	}
	queue->rear->next = con;
	queue->rear = con;
	con->next = NULL;

}

Node* Dequeue(Queue *queue) {
	Node* temp = queue->front;
	if(queue->front == NULL) {
		return;
	}
	if(queue->front == queue->rear) {
		queue->front = queue->rear = NULL;
		queue->isEmpty = -1;
	}
	else {
		queue->front = queue->front->next;
	}
	return temp;
}

void RemoveFromQueue(int id, Queue *queue){
	if(queue->front == NULL) {
		return;
	}
	if(queue->front == queue->rear) {
		if(queue->front->nodeId == id){
			queue->front = queue->rear = NULL;
			queue->isEmpty = -1;
		}
	}
	else{
		Node* temp = queue->front;
		while(temp != NULL){
			if(temp->nodeId == id){
				temp->next = temp->next->next;
				break;
			}
			temp=temp->next;
		}
	}
}


void markContextComplete(){
	if (ready_Queue.isEmpty != -1){
		Node *temp;
		temp = Dequeue(&ready_Queue);
		swapcontext( &markComplete,temp->element );
	}
}
ucontext_t getCompletionContext()
{
    static int isCreated;
    
    if(!isCreated)
    {
        getcontext(&markComplete);
        markComplete.uc_stack.ss_sp = malloc(STACK_SIZE);
        markComplete.uc_stack.ss_size = STACK_SIZE;
        markComplete.uc_stack.ss_flags= 0;
		markComplete.uc_link = 0;
        makecontext( &markComplete, (void (*)(void))&markContextComplete, 0);
        isCreated = 1;
    }
    return markComplete;
}


// Create a new thread.
MyThread MyThreadCreate(void(*start_funct)(void *), void *args){
	Node *newChild;
	newChild = (Node*) malloc(sizeof(Node));
	newChild->element = (ucontext_t*) malloc(sizeof(ucontext_t));
	newChild->element->uc_link = NULL; 
	newChild->element->uc_stack.ss_sp = malloc( STACK_SIZE );
	newChild->element->uc_stack.ss_size = STACK_SIZE;
	newChild->element->uc_stack.ss_flags = 0;        
	if ( newChild->element->uc_stack.ss_sp == 0 )
	{
		perror( "malloc: Could not allocate stack" );
		exit(-1);
	}
	newChild->nodeId = uniqueId;
	uniqueId++;
	newChild->isBlocking = 0;
	newChild->isJoined = 0;
	newChild->waitOnSemaphore =0;
	newChild->child=NULL;
	newChild->parent=currentNode;
	newChild->otherChild=NULL;
	newChild->hasYielded=0;
	getcontext(newChild->element );
	makecontext( newChild->element,(void (*)(void))start_funct,1,args);
	
	//Add this new thread as a child to the current thread
	if(currentNode->child == NULL){
		currentNode->child=newChild;
	}
	else{
		Node *tmp;
		tmp = currentNode->child;
		while(tmp->otherChild != NULL){
			tmp = tmp->otherChild;
		}
		tmp->otherChild=newChild;
	}
	Enqueue(newChild, &ready_Queue);
	return (MyThread)newChild;
}

// Yield invoking thread
void MyThreadYield(void){
	//printf("inside yield\n");
	Node *current;
	currentNode->hasYielded = 1;
	current=currentNode;
	getcontext(current->element);
	if (ready_Queue.isEmpty != -1 && (currentNode->hasYielded == 1)){
		//printf("inside yield if 1\n");
		Node *temp;
		temp = Dequeue(&ready_Queue);
		temp->hasYielded =0;
		Enqueue(current, &ready_Queue);
		currentNode=temp;
		setcontext(temp->element );
	}
	// else if((ready_Queue.isEmpty == -1) && currentNode->hasYielded == 1){
		// //printf("inside yield   2\n");
		// currentNode = parentNode;
		// setcontext(parentNode-> element);
	// }
}

// Join with a child thread
int MyThreadJoin(MyThread thread){
	Node *temp = currentNode->child;
	Node *callingThread = currentNode; 
	Node *blockingChild = (Node *)thread;
	while(temp!= NULL){
		if(temp->nodeId == blockingChild->nodeId){
			temp->isJoined++;
			currentNode->isBlocking++;
			currentNode->hasYielded=1;
			getcontext(currentNode->element);
			//Enqueue(currentNode, &block_Queue);
			if (ready_Queue.isEmpty != -1 && (currentNode->hasYielded == 1)){
				Node *new;
				new = Dequeue(&ready_Queue);
				currentNode = new;
				currentNode->hasYielded =0;
				return swapcontext(callingThread->element,currentNode->element);
			}
			// else if((ready_Queue.isEmpty == -1) && currentNode->hasYielded == 1){
				// currentNode = parentNode;
				// return swapcontext(callingThread->element,parentNode-> element);
			// }
			return 0;
		}
		temp = temp->otherChild;
	}
	return -1;
}

// Join with all children
void MyThreadJoinAll(void){
	////printf("Inside joinAll \n ");
	Node *temp = currentNode->child;
			//printf("inside joinall\n");

	while(temp != NULL){
				//printf("inside joinall while loop\n");

		////printf("Inside joinAll- while %d \n ", temp->nodeId);
		currentNode->isBlocking++;
		temp->isJoined++;
		temp = temp->otherChild;
	}
	currentNode->hasYielded = 1;
	getcontext(currentNode->element);
	//Enqueue(currentNode, &block_Queue);
	if (ready_Queue.isEmpty != -1 && (currentNode->hasYielded == 1) ){
		//printf("inside joinall dequque loop\n");
		Node *temp;
		temp = Dequeue(&ready_Queue);
		currentNode = temp;
		currentNode->hasYielded = 0;
		////printf("Inside joinAll- queue pop %d \n ", temp->nodeId);
		setcontext(currentNode->element);
	}
	// else if((ready_Queue.isEmpty == -1) && currentNode->hasYielded == 1){
		// //printf("inside joinall main quque\n");
		// currentNode = parentNode;
		// setcontext(parentNode-> element);
	// }
}

// Terminate invoking thread
void MyThreadExit(void){
	//printf("inside myexit %d \n", currentNode->nodeId);
	Node *tmpParent = currentNode->parent;
	Node *tmpChild = currentNode->child;
	while(tmpChild != NULL){
		tmpChild->parent=NULL;
		tmpChild = tmpChild->otherChild;
	}
	if(tmpParent->child != NULL){
			Node *temp = tmpParent->child;
			if(currentNode->nodeId == temp->nodeId){
				tmpParent->child = temp->otherChild;
			}
			else{
				while(temp->otherChild != NULL){
					if(currentNode->nodeId == temp->otherChild->nodeId){
						temp->otherChild = temp->otherChild->otherChild;
						break;
					}
					temp = temp->otherChild;
				}
			}	
		}
	printf("inside 3");
	if((currentNode->isJoined !=0) && (tmpParent != NULL) && (tmpParent->isBlocking != 0)){
		tmpParent->isBlocking--;
		if(tmpParent->isBlocking ==0){
			//RemoveFromQueue(tmpParent->nodeId, &block_Queue);
			Enqueue(tmpParent, &ready_Queue);
		}
	}
	printf("inside 2");
	if (ready_Queue.isEmpty != -1){
		Node *temp;
		temp = Dequeue(&ready_Queue);
		currentNode = temp;
		currentNode->hasYielded =0 ;
		setcontext(currentNode->element);
	}
	// else{
		// currentNode = parentNode;
		// setcontext(parentNode-> element);
	// }
}

// ****** SEMAPHORE OPERATIONS ****** 
// Create a semaphore
MySemaphore MySemaphoreInit(int initialValue){
	SemaphoreNode *temp;
	if(initialValue < 0)
		return NULL;
	temp = malloc(sizeof(SemaphoreNode));
	temp->s=initialValue;
	//temp->isAlive=0;
	temp->runningCount=0;
	temp->semaphore_Queue.front = NULL;
	temp->semaphore_Queue.rear = NULL;
	temp->semaphore_Queue.isEmpty = -1;
	return ((MySemaphore) temp);

}

// Signal a semaphore
void MySemaphoreSignal(MySemaphore sem){
	SemaphoreNode *temp = (SemaphoreNode*) sem;
	temp->s++;
	temp->runningCount--;
	if(temp->s <= 0){
		if(temp->semaphore_Queue.isEmpty != -1){
			Node *tempNode;
			tempNode = Dequeue(&temp->semaphore_Queue);
			tempNode->isBlocking--;
			tempNode->waitOnSemaphore =0;
			if(tempNode->isBlocking == 0){
				Enqueue(tempNode, &ready_Queue);
			}
		}
	}
}

// Wait on a semaphore
void MySemaphoreWait(MySemaphore sem){
	SemaphoreNode *temp = (SemaphoreNode*) sem;
	Node *current = currentNode;
	temp->s--;
	temp->runningCount++;
	getcontext(current->element);
	if(temp->s < 0){
		current->waitOnSemaphore =1;
		current->isBlocking++;
		temp->s--;
		Enqueue(current, & temp->semaphore_Queue);
		if (ready_Queue.isEmpty != -1){
			Node *temp;
			temp = Dequeue(&ready_Queue);
			currentNode = temp;
			setcontext(currentNode->element);
		}
	}
}

// Destroy on a semaphore
int MySemaphoreDestroy(MySemaphore sem){
	SemaphoreNode *temp = (SemaphoreNode*) sem;
	if(temp->runningCount != 0)
		return -1;
	else{
		//temp->isAlive=-1;
		free(temp);
		return 0;
	}
}

// ****** CALLS ONLY FOR UNIX PROCESS ****** 
// Create and run the "main" thread
void MyThreadInit(void(*start_funct)(void *), void *args){
	uniqueId=0;
	parentNode = (Node*) malloc(sizeof(Node));
	parentNode->element= (ucontext_t*) malloc(sizeof(ucontext_t));
	getcontext(parentNode->element);
	if(uniqueId ==0){
		parentNode->element->uc_link = NULL;
		parentNode->element->uc_stack.ss_sp =  malloc( STACK_SIZE );
		parentNode->element->uc_stack.ss_size = STACK_SIZE;
		if ( parentNode->element->uc_stack.ss_sp == 0 )
		{
			perror( "malloc: Could not allocate stack" );
			exit(-1);
		}
		parentNode->nodeId = uniqueId;
		uniqueId++;
		parentNode->child = NULL;
		parentNode->otherChild = NULL;
		parentNode->parent = NULL;
		parentNode->isBlocking = 0;
		parentNode->isJoined = 0;
		parentNode->hasYielded = 0;
		parentNode->waitOnSemaphore =0;
		//Enqueue(parentNode, &ready_Queue);
		
		
		//getCompletionContext();
		Node *mainNode;
		mainNode = (Node*) malloc(sizeof(Node));
		mainNode->element = (ucontext_t*) malloc(sizeof(ucontext_t));
		mainNode->element->uc_link = NULL;
		mainNode->element->uc_stack.ss_sp = malloc( STACK_SIZE );
		mainNode->element->uc_stack.ss_size = STACK_SIZE;
		if ( mainNode->element->uc_stack.ss_sp == 0 )
		{
			perror( "malloc: Could not allocate stack" );
			exit(-1);
		}
		getcontext(mainNode->element);
		makecontext(mainNode-> element,(void (*)(void))start_funct,1,args);
		mainNode->nodeId = uniqueId;
		uniqueId++;
		mainNode->child = NULL;
		mainNode->otherChild = NULL;
		mainNode->parent = parentNode;
		mainNode->isBlocking = 0;
		mainNode->isJoined = 0;
		mainNode->hasYielded =1;
		mainNode->waitOnSemaphore =0;
		currentNode = mainNode;
		setcontext(mainNode-> element);
	}
}
