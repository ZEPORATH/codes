#include <bits/stdc++.h>
using namespace std;
int BinarySearch(int A[],int n,int x, bool searchFirst){
    int low = 0,high = n-1,result = -1;
    int mid = (low+ high)/2;
    while(low<=high){
    if(A[mid]==x){
    result = mid;
    if(searchFirst)
        high = mid-1;
    else
        low = mid+1;
    }
    else if(x<A[mid]) high = mid-1;
    else low = mid+1;
    }
    return result;
}
int BinarySearch(int A[], int n, int x){
    int low = 0,high = n-1;
    int mid = (low+ high)/2;
    while(low<=high){
    if(A[mid]==x)return mid;
    else if(x<A[mid]) high = mid-1;
    else low = mid+1;
}
    return -1;
}

int count_Occurances(int A[] ,int n, int x){
    int hi = -1,lo=-1;

     lo = BinarySearch(A,n,x,true);

     if(lo==-1) return 0;
     else{
        hi = BinarySearch(A,n,x,false);
        return (hi-lo+1);
     }
    return (hi-lo+1);
}

int main(){
    int arr[] = {1,2,3,3,3,3,3,7,8,9,11,12,13,14,15,56};
    int x = 3;
    int c =0;
    int n = sizeof(arr)/sizeof(arr[0]);
    c = count_Occurances(arr,n,x);
    if (c==-1)cout<<"There is no number %d"<<x<<endl;
    else cout<<"The number %d is number of %d"<<x<<c<<endl;
    return 0;

}
