#include <iostream>
using namespace std;
int BinaryRank(int A[],int N,int x){
	int low=0,high =N-1,mid = (low+high)/2;
	if(A[mid]==x)return mid;
	while(low<=high){
		if(A[mid]>x)high = mid-1;
		else if(A[mid]<x)low  = mid+1;
		else if(A[mid]==x)return mid;
	}
	return 0;
}
int main()
{
    int N,q,x,m;
    int out;
    cin>>N;
    int A[N];
    for(int i=0;i<N;i++){
    	cin>>m;
    	A[i]=m;
    }
    for(int i=0;i<N;i++)cout<<A[i]<<"\n";
    cin>>q;
    while(q>0){

    	cin>>x;
    	out  = BinaryRank(A,N,x);
    	cout<<out;
    	q--;
    }
    return 0;
}
