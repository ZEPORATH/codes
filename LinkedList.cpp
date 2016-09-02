#include "LinkedList.h"

LinkedList::LinkedList(){
	head = new Node;
	head->next = NULL;
	length = 0;
}

LinkedList::InserItem(Node* newItem){
	if(!head->next){
		head->next = newItem;
		length++;
		return;
	}
	Node* p = head;Node* q = head;
	while(q->next!=NULL){
		q = q->next;
	}
	q->next = newItem;
	newItem->next = NULL;
	length++;


}
bool LinkedList::removeItem(int key){
	if(!head->next)return false;
	Node* p = head;Node* q = head;
	while(q){
		if(q->data == key){
			p->next = q->next;
			delete(q);
			length--;
			return true;
		}

	}
}