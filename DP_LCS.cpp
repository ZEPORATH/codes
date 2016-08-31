#include <bits/stdc++.h>
using namespace std;

string LPalindromeS(string A){
	int n = A.size();
	string pald = "";
	bool table[n][n];
	memset(table,0,sizeof(table));//All values set to false
	int maxLength = 1;
	for(int i=0;i<n;i++) table[i][i] = true;//All single characcter are palindrome to itself

	int start = 0;

for(int i=0;i<n-1;i++){
	if (A[i] == A[i+1])
	{
		table[i][i+1] = true;
		start  =i;
		maxLength =2;
	}
}

for (int k = 3; k < n; ++k)
{
	//Ending index of substring from
	//starting indx i and length k
	for (int i = 0; i < n-k+1; ++i)
	{
		int j = i+k-1;
	//Check if substring from ith to jth 
	//index is a palindrome
	if (table[i+1][j-1] && A[i]==A[j])
	{
		table[i][j] = true;

		if(k > maxLength){
			maxLength = k;
			start = i;
		}
	}

	}

}
for(int i = start;i<start+maxLength;i++){
		pald+=A[i];
	}
	return pald;
}

int main(int argc, char const *argv[])
{
	string A;
	cin>>A;
	cout<<LPalindromeS(A);
	return 0;
}                                                                                                                         )