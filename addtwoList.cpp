#include <bits/stdc++.h>
using namespace std;
struct ListNode {
      int val;
      ListNode *next;
      ListNode(int x) : val(x), next(NULL) {}
      ListNode():val(0),next(NULL){}
 };

 ListNode* reverse(ListNode *head){
     ListNode *temp = NULL, *nextnode = NULL;
     while(head){
         nextnode = head->next;
         head->next= temp;
         temp = head;
         head = nextnode;
     }
     return temp;
     
 }
 
int List_to_int(ListNode* head){
    ListNode* p = head;
    int res= 0;
    int count = 0;
    while(p){
        res = res+p->val*pow(10,count);
        p = p->next;
        count++;
    }
    return res;
}
ListNode* Int_to_list(int num){
    ListNode* head = NULL, *curr=NULL;
    ListNode* temp ;
    while(num>0){
        if(head == NULL){
            // temp = new ListNode;
            temp = new ListNode(num%10);
            head = temp;
            // head->val = num%10;
            num /=10;
            head->next = NULL;
            curr = head;
        }
        else{
            // temp = new ListNode;
            temp = new ListNode(num%10);
            // temp->val = num%10;
            num/=10;
            temp->next = NULL;
            curr->next = temp;
            curr = temp;
        }
    }
    return head;
}
 
ListNode* addTwoNumbers(ListNode* A, ListNode* B) {
    // ListNode *p1 = A;
    // ListNode *p2 = B;
    // ListNode *head = NULL;
    // ListNode *curr = NULL;
    // int res =0 , rem =0;
    // while(p1&p2){
    //     if(head == NULL){
    //          res = (p1->val + p2->val + rem)/10;
    //          rem =(p1->val + p2->val + rem)%10;
    //          curr->val = res;
    //          curr->next = NULL;
    //          head = curr;
    //     }
    //     else{
    //         ListNode *temp ;
    //         res = (p1->val + p2->val + rem)/10;
    //          rem =(p1->val + p2->val + rem)%10;
    //          temp->val = res;
    //          temp->next = NULL;
    //          curr->next = temp;
            
    //     }
    //     p1 = p1->next;
    //     p2 = p2->next;
    // }
    // if(p1){
        
    // }

    int A1 = List_to_int(A);
    int B1 = List_to_int(B);
    return(Int_to_list(A1+B1));
}

// void push(ListNode** head_ref, int new_data)
// {
//     /* allocate node */
//     struct node* new_node =
//             (struct node*) malloc(sizeof(struct node));
 
//     /* put in the data  */
//     new_node->data  = new_data;
 
//     /* link the old list off the new node */
//     new_node->next = (*head_ref);
 
//     /* move the head to pochar to the new node */
//     (*head_ref)    = new_node;
// }
 void printList(ListNode *ptr)
{
    while (ptr != NULL)
    {
        printf("%d->", ptr->val);
        ptr = ptr->next;
    }
    printf("NULL\n");
}

ListNode* addTwoNumbers1(ListNode* A, ListNode* B) {
    ListNode *p1 = A;
    ListNode *p2 = B;
    ListNode *head = NULL;
    ListNode *curr = NULL;
    ListNode *temp;
    int res =0 , rem =0,val2;
    while(p1&&p2){
        if(head == NULL){
             res = p1->val + p2->val+ rem;
             val2 = res%10;
             rem = res>9?1:0;
             temp = new ListNode(val2);
             head = temp;
             curr = head;
        }
        else{
            res = p1->val+p2->val+rem;
            val2 = res%10;
             rem = res>9?1:0;
             temp = new ListNode(val2);
             curr->next = temp;
             curr = temp;;
            
        }
        p1 = p1->next;
        p2 = p2->next;
    }
    while(p1){
        res = p1->val+rem;
            val2 = res%10;
             rem = res>9?1:0;
             temp = new ListNode(val2);
             curr->next = temp;
             curr = temp; 
             p1 = p1->next;
    }
    while(p2){
        res = p2->val+rem;
        val2 = res%10;
         rem = res>9?1:0;
         temp = new ListNode(val2);
         curr->next = temp;
         curr = temp; 
         p2 = p2->next;
    }
    if(rem==1){
        temp = new ListNode(rem);
        curr->next = temp;
        curr = temp;
    }
    return head;
}
int main(int argc, char const *argv[])
{
    ListNode *head1 = NULL;
    ListNode *head2 = NULL;
    printList(Int_to_list(199));
    printList(addTwoNumbers1(Int_to_list(199),Int_to_list(1)));
    return 0;
}