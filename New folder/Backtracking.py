import sys
A = [0]*10
n = len(A)
count =1
output = open("D:\\output.txt",'w')
output.seek(0)
output.truncate()
def func1(n):
    global count
    if n<1:
        print >>output, count,":\t" ,A
        count+=1
    else:
        A[n-1]=0
        func1(n-1)
        A[n-1] =1
        func1(n-1)
count1 =1
B=[0]*5
n2 = len(B)
def func2(n,k):
    global count1
    if n<1:
        print >>output, count,"\t",A
        count1+=1
    else:
        for j in range (0,k):
            B[n-1] = j
            func2(n-1,k)

C=[1,2,3,4]
n3 = len(C)
count2 =1;
def func3(start, end):
    global count2;
    if(start==end) :
        print >>output,count,"\t", C
        count2+=1
    else:
        for i in range(start,end+1):
            C[start], C[i] = C[i], C[start]
            func3(start+1,end)
            C[i],C[start] = C[start],C[i]
            
        
    

if __name__ == "__main__":
    print >>output, "Print in ouptut for func1:"
    func1(n)

    print >>output, "\n\n"
    print >>output, "Printin ouptut for func2:"
    func2(n2,5)
    print >>output, "\n\n"
    print >>output, "Printin ouptut for func3:"
    func3(0,n3)
    
    output.close()
