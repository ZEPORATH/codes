#Akash and GCD
'''
A[1] = 3, A[2] = 4, A[3] = 3
F(3) = GCD(1, 3) + GCD(2, 3) + GCD(3, 3) = 1 + 1 + 3 = 5.
F(4) = GCD(1, 4) + GCD(2, 4) + GCD(3, 4) + GCD(4, 4) = 1 + 2 + 1 + 4 = 8.

First query, the sum will be F(3) + F(4) = 5 + 8 = 13 (mod 10^9 + 7).
Second query, the sum will be F(3) + F(4) + F(3) = 5 + 8 + 5 = 18 (mod 10^9 + 7).
Third query, the sum will be F(3) = 5 (mod 10^9 + 7).
Fourth query will update A[1] = 4.
Fifth query, the sum will be F(4) + F(4) + F(3) = 8 + 8 + 5 = 21 (mod 10^9 + 7).
Sixth query, the sum will be F(4) + F(4) = 8 + 8 = 16 (mod 10^9 + 7).
'''
import sys, os , traceback,  linecache
from fractions import gcd
def PrintException():
    exc_type, exc_obj, tb = sys.exc_info()
    f = tb.tb_frame
    lineno = tb.tb_lineno
    filename = f.f_code.co_filename
    linecache.checkcache(filename)
    line = linecache.getline(filename, lineno, f.f_globals)
    print 'EXCEPTION IN ({}, LINE {} "{}"): {}'.format(filename, lineno, line.strip(), exc_obj)
def Fx(n):
    i = 1
    res = 0
    while i<=n:
        res+= gcd(i,n)
        i+=1
    return res
try:
    d = {}
    arr_len = int(raw_input())
    arr = raw_input()
    arr = map(int, arr.split(' '))
    for i in arr:
        d[i] = Fx(i)
    #print d
    #print arr, type(arr)
    kases = int(raw_input())
    while kases>0:
        con = raw_input()
        con = con.split(' ')
        if con[0] == 'C':
            x = int(con[1])
            y = int(con[2])
            res =0
            while x<=y:
                if d.has_key(x):
                    res+=d[x]
                    x+=1
                else:
                    d[x] = Fx(i)
                    res +=d[x]
                    x+=1
            print res
        elif con[0] == 'U':
            i = int(con[1])
            n = int(con[2])
            d[i] = Fx[n]
        kases-=1
        
except Exception as e:
    PrintException()
