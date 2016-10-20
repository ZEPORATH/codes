#include <iostream>
#include <bits/stdc++.h>
using namespace std;
// class Node{

// };
// class Edge{

// };
// class Graph{

// };
// int F(vector<vector<pair<int,int>>> G, int i, int j){
// 	int i=0;
// 	int max =0;
// 	vector<vector<pair<int,int>>>::iterator it;
// 	vector<pair<int,int>>::iterator it1;
// 	for(it=G.begin() ; it < G.end(); it++,i++ ) {

// 	    for (it1=G[i].begin();it1<G[i].end();it1++){

// 	    }
// }
int main() {
	int k = 0;
	int n = 0;
	cin>>k;
	while(k>0){
		cin>>n;
		int s = n;
		//Declare Adjacency List
		vector<vector<pair<int, int>>> G;
		G.resize(n+1);
		//Add an edge u,v of weight w
		while(n>0){
			int u=0,v=0,w=0;
			cin>>u>>v>>w;
			G[u].push_back({v,w});
			G[v].push_back({u,w});
			n--;
		}
		cout<<"Graph Created \n";
		int i=0;
		vector<vector<pair<int,int>>>::iterator it;
		vector<pair<int,int>>::iterator it1;
		for(it=G.begin() ; it < G.end(); it++,i++ ) {

		    for (it1=G[i].begin();it1<G[i].end();it1++){
		    	for(pair<int,int> p: G[i]){
		    	cout <<"  "<<i<<"-> (w = "<<p.second<<") -> "<<p.first;
		    }
		    cout<<endl;
		    }


		}
		cout<<G.size()/sizeof(G[0]);
		cout<<G[0].size();
		int res;
		// for (int m=1;m<s-1;m++){
		// 	for (int j=1;j<s;j++){
		// 		res+=F(G,i,j);
		// 	}
		// }
		cout<<"\n"<<res<<endl;
		k--;
	}


	return 0;
}
