#include <bits/stdc++.h>
using namespace std;

class Base
{
   	public:
      	Base(){ cout<<"Constructing Base";}

	// this is a virtual destructor:
	virtual ~Base(){ cout<<"Destroying Base";}
};

class Base1
{
   	public:
      	Base(){ cout<<"Constructing Base";}
      	
     // this is a destructor:
	
	~Base(){ cout<<"Destroying Base";}
};

class Derive: public Base
{
        public:
       	Derive(){ cout<<"Constructing Derive";}
       	
       	~Derive(){ cout<<"Destroying Derive";}
 };

class Derive1: public Base1
{
        public:
       	Derive1(){ cout<<"Constructing Derive";}
       	
       	~Derive1(){ cout<<"Destroying Derive";}
 };

void main()
{
    	Base *basePtr = new Derive();
        
        delete basePtr;

        Base1 *basePtr1 = new Derive1();
        
        delete basePtr1;
}