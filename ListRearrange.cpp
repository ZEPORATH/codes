#include <bits/stdc++.h>
#include <iostream>
#include <list>
using namespace std;

int main(){
list<int> lst;
int len,element;
cout<<"Enter the no of values you want to enter"<<endl;
cin>>len;
while(len>0){
    cin>>element;
    lst.push_back(element);
    len--;
}
list<int>::iterator i;
list<int>::iterator i1;
for (i =lst.begin(); i!= lst.end();++i) {
cout <<*i<<' ';
}
cout<<endl;
int j = *(lst.end());
while((*i1)!= j ){
  lst.insert(i1,lst.back());
   lst.pop_back();
   ++i1;++i1;
}
for (i =lst.begin(); i!= lst.end();++i) {
cout <<*i<<' ';
//cout<<lst.back()<<' ';
//lst.pop_back();

}
cout<<endl;
return 0;
}
