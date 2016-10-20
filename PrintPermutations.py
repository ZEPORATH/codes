#Print Permutations

def permutations(head,lst, tail=''):
    
	
    if len(head) == 0:
		#print tail
        lst.append(tail)
    else:
        for i in range(len(head)):
            permutations(head[0:i] + head[i+1:],lst, tail+head[i])


def permute(A):
    str1 = ''.join(str(e) for e in A)
    lst2 = []
    lst = []
    permutations(str1,lst)
    res = []
    #print lst
    lst = list(set(lst))
    for i in lst:
        dum=[]
        for e in list(i):
            dum.append(int(e))
        
        res.append(dum)
    print res
    print len(res)
    

permute([ 1, 2, 3,1])

def permute1(self, num):
    length = len(num)
    if length == 0: return []
    if length == 1: return [num]
    num.sort()
    res = []
    previousNum = None
    for i in range(length):
        if num[i] == previousNum: continue
        previousNum = num[i]
        for j in self.permute(num[:i] + num[i+1:]):
            res.append([num[i]] + j)
    return res
