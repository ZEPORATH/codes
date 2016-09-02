#ifndef LinkedList_h
#define LinkedList_h

#include <iostream>
#include <string>

using namespace std;

//******************************************************
//	List Item are keys for HashMap
//******************************************************

struct Node
{
	int data;
	Node* next;
};

class LinkedList{
	Node* head;
	int length;
	public:
		LinkedList();
		void InsertItem(Node* newItem);
		bool removeItem(int ItemKey);
		Node* getItem(int ItemKey);
		void printList();
		int getLength();


}
#endif