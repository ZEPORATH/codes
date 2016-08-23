
#include<bits/stdc++.h>

using namespace std;


int anagram(string a,string b)
 {
    int len_a = a.length();
    int len_b = b.length();

    int x[256] = {0};
    int y[256] = {0};

   if(len_a == len_b)
    {
     for(int i =0 ; i<len_a ; i++)
      {
       x[toascii(a[i])]++;
       y[toascii(b[i])]++;
      }
     for(int i=0 ; i< 256 ; i++)
      {
        if(x[i] == y[i]) ;

        else  return 0;
      }

    }

   else {return 0;}

  return 1;

 }

int main()
 {
   string a,b;
   cin>>a>>b;


   int i = anagram(a,b);

   if(i ==1)  cout<<"anagrams";
   else  cout<<"not anagrams";
   return 0;
 }
 
