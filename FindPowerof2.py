def power(A):
    if (int(A)<=1): return 0
    bin_string =bin(int(A)
    if bin_str.count('1') == 1: 
    	return 1
    else: return 0

print power("2")