
#include <bits/stdc++.h>
using namespace std;
class Solution{
public: 
	std::vector<std::vector<string> v> v;

	void printers(std::vector<int> A, int n){
		std::vector<string> r;
		for (int i = 0; i < n; ++i)
		{
			string str(n,'.');
			str[A[i]]='Q';
			r.push_back(str);
		}	
		res.push_back(r);
	}

	bool isValid(std::vector<int> A, int r){
		for (int i = 0; i < r; ++i)
		{
			if(A[i]=A[r]||(abs(A[i]-A[r]) == (r-i))){
				return false;
			}
		}
		return true;
	}
	void nQueens(std::vector<int> A, int curr, int n){
		if (curr == n){printers(A,n);}
		else{
			for (int i = 0; i < n; ++i)
			{
				A[curr] = i;
				if (isValid(A,curr))
				{
					nQueens(A,curr+1,n);
				}
			}
		}
	}
	vector<std::vector<string>> solveNQueens(int n){
		res.clear()
		std::vector<int>A(n-i);
		nQueens(A,0,n);
		return res;
	}
};

int main(int argc, char const *argv[])
{
	
	return 0;
}