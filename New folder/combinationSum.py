import sys
output = open("output.txt",'w')
output.seek(0)
output.truncate()
def DFS(candidates,target,start,valueList):
    length = len(candidates)
    if target == 0:
        return Solution.ret.append(valueList)
        
    for i in range(start,length):
        if target<candidates[i]:return
        DFS(candidates,target-candidates[i],i, valueList+[candidates[i]])

class Solution:
    # @param A : list of integers
    # @param B : integer
    # @return a list of list of integers
    
            
    
    def combinationSum(self, A,B):
        A.sort()
        A = list(set(A))
        Solution.ret = []
        DFS(A,B,0,[])
        return Solution.ret
    def __init__(self):
        pass

if __name__ == '__main__':
	p = Solution()
	A = [ 8, 10, 6, 11, 1, 16, 8 ]
	B = 28
	R = p.combinationSum(A,B)
    # ans = [[1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 ] [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 6 ], [1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 8 ] ,[1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 10 ] ,[1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 11 ] ,[1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 6 6 ] ,[1 1 1 1 1 1 1 1 1 1 1 1 1 1 6 8 ], [1 1 1 1 1 1 1 1 1 1 1 1 6 10 ] ,[1 1 1 1 1 1 1 1 1 1 1 1 8 8 ] ,[1 1 1 1 1 1 1 1 1 1 1 1 16 ], [1 1 1 1 1 1 1 1 1 1 1 6 11 ] , [1 1 1 1 1 1 1 1 6 6 6 ] ,[1 1 1 1 1 1 1 1 1 1 8 10 ] ,[1 1 1 1 1 1 1 1 1 8 11 ] ,[1 1 1 1 1 1 1 1 6 6 8 ], [1 1 1 1 1 1 1 1 10 10 ] ,[1 1 1 1 1 1 1 10 11 ] ,[1 1 1 1 1 1 6 6 10 ] ,[1 1 1 1 1 1 6 8 8 ], [1 1 1 1 1 1 6 16 ] ,[1 1 1 1 1 1 11 11 ] ,[1 1 1 1 1 6 6 11 ], [1 1 1 1 6 6 6 6 ] ,[1 1 1 1 6 8 10 ] ,[1 1 1 1 8 8 8 ], [1 1 1 1 8 16 ], [1 1 1 6 8 11 ], [1 1 6 6 6 8 ] ,[1 1 6 10 10 ], [1 1 8 8 10 ] ,[1 1 10 16 ] ,[1 6 10 11 ] ,[1 8 8 11 ] ,[1 11 16 ] ,[6 6 6 10 ] ,[6 6 8 8 ] ,[6 6 16 ], [6 11 11 ], [8 10 10 ]]
	#R = list(set(R))
	print >>output,R
	print >>output, ("\n\n\n")
    