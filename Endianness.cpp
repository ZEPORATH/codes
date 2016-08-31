// a character pointer c is pointing to an integer i. Since size of character is 1 byte 
// when the character pointer is de-referenced 
// it will contain only first byte of integer. If machine is little endian 
// then *c will be 1 (because last byte is stored first) 
// and if machine is big endian then *c will be 0.

#include <bits/stdc++.h>
using namespace std;

int main(int argc, char const *argv[])
{	unsigned int i = 1;
	char *c = (char*)&i;
	if(*c) cout<<"Little Endian";
	else cout<<"Big Endian";
	cout<<endl;
	return 0;
}