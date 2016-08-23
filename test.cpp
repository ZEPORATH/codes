#include <bits/stdc++.h>
using namespace std;

int findMin(std::vector<int> A,int start , int end){
	if (start>end) return -1;
	if(start == end) return start;
	int mid = (start+end)/2;
	if (A[mid]>A[end])
	{
		return findMin(A,mid+1,end);
	}
	else{
		int index1 = findMin(A,start,mid-1);
		int index2 = findMin(A,end,mid+1);
		if(index1==-1&& index2==-1){
			if(A[index1]<A[index2])return index1;
			return index2;
		}
		else if(index1==-1)return index1;
		else return index2;
	}
}
int binarySearch(std::vector<int> A,int start,int end, int key){
	if(start>end)return -1;
	if(start==end) return (A[start]==key)?start:-1;
	int mid = (start+mid)/2;
	if(A[mid]==key)return mid;
	if (A[mid]>key) return binarySearch(A,start,mid-1,key);
	else return binarySearch(A,mid+1,end,key);
}
int search(std::vector<int> &A, int B){
	int n = A.size();
	int pivot = findMin(A,0,n-1);
	int index1 = binarySearch(A,0,pivot,B);
	if (index1==-1)return -1;
	return 	binarySearch(A,pivot,n-1,B);
}
int main(int argc, char const *argv[])
{
	int size,B;
	cin>>size;
	char c;
	vector<int> A(size);
	for(int i=0;i<size;i++){
		cin>>A[i];
	}
	for(int i=0;i<size;i++){
		cout<<A[i];
	}
	cin>>B;
	int i = search(A,B);
	cout<<i<<endl;
	cin>>c;
	return 0;
}
