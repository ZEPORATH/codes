#include <iostream>
#include <iomanip>
#include <string>
using namespace std;
int main(){
string s;
string p;
do{
    getline(cin,s);
    p = p+s+"\n";
}while(s!="");
cout<<p<<endl;
int k;
cin>>k;
int j=0;
char sep = ' ';
for(int i=0;i<p.size();i++){
    if(p[i]==sep){
    	if((i-j)==k) cout<<p.substr(j,i);
    	j=i;
    }

}


return 0;
}
