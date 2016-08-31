//Generate paranthesis

#include <bits/stdc++.h>
using namespace std;

std::vector<string>  GenerateParanthesis(int n);
void dfs(std::vector<string> &result,string s, int left, int right);
int main(int argc, char const *argv[])
{	
	int n;
	cin>>n;
	vector<string> v = GenerateParanthesis(n);
	for (vector<string>::const_iterator it = v.begin(); it != v.end(); ++it){
		cout<<	*it<<" "<<endl;
	}
	return 0;
}

vector<string>  GenerateParanthesis(int n){
	std::vector<string> result;
	dfs(result,"",n,n);
	return result;
}
void dfs(vector<string> &result,string s, int left, int right){
	if(left>right) return;
	if (left==0  && right==0)
	{
		result.push_back(s);
		return;
	}

	if(left>0){
		dfs(result,s+"(",left-1,right);
	}
	if(right > 0){
		dfs(result,s+")",left,right-1);
	}

}