flag_1 =0
flag_2 = 0
tabcount = 0
ans = []
word = ""
bad_braces_flag = False

def openbrace1():
    global flag_1, ans, tabcount
    flag_1 += 1
    ans.append("[")
    tabcount += 1

def openbrace2():
    global flag_2, ans, tabcount
    flag_2 +=1
    ans.append("{")
    tabcount += 1

def closebrace1():
    global flag_1,tabcount, ans
    flag_1 -=1
    tabcount -= 1
    ans.append("]")
   
    
def closebrace2():
    global flag_2,tabcount, ans
    flag_2 -= 1
    tabcount -= 1
    ans.append("}")
    
    
def addtoword(c):
    global word
    word+= c
    
def addtoList():
    global word, ans
    word+=","
    ans.append("\t"*(tabcount+1)+ word)
    word = ""

    # flag_1 =0
# flag_2 = 0
# tabcount = 0
# ans = []
# word = ""
# bad_braces_flag = False

# def openbrace1():
#     flag_1 +=1
#     ans.append("[")
#     tabcount += 1

# def openbrace2():
#     flag_2 +=1
#     ans.append("{")
#     tabcount += 1

# def closebrace1():
#     flag_1 -=1
#     tabcount -= 1
#     ans.append("]")
   
    
# def closebrace2():
#     flag_2 -= 1
#     tabcount -= 1
#     ans.append("}")
    
    
# def addtoword(c):
#     word+= c
    
# def addtoList():
#     word+=","
#     ans.append("\t"*(tabcount+1)+"word")
#     word = ""

    
def preettyJson(A):
    global word, ans, tabcount, bad_braces_flag, flag_1, flag_2
    for i in A:
        
        if i == "[" and flag_1>=0 and flag_2>=0:
            ans.append("\t"*tabcount+"[")
            openbrace1()
        elif i == "{" and flag_1>=0 and flag_2>=0:
            ans.append("\t"*tabcount+"{")
            openbrace2()
        elif i == "}" and flag_1>=0 and flag_2>=0:
            ans.append("\t"*tabcount+"}")
            closebrace2()
        elif i == "]" and flag_1>=0 and flag_2>=0:
            ans.append("\t"*tabcount+"]")
            closebrace1()
        elif i == "(" or i == ")":
            ans = []
            bad_braces_flag = True
            return
        elif i==",":
            addtoList()
        else : addtoword(i)
       
            
def printList(A):
    global bad_braces_flag
    if len(A)==0 and bad_braces_flag == True: 
        print "Bad braces used"
        return 0
    
    else:
        print "Nothing to print"
        return 
        
    for i in A:
        print i
        print "\n"
    
   
    
A = str("A:"B",C:{D:""E"",F:{G:""H"",I:""J""}}}")
preettyJson(A)
print ans
printList(ans)

           
# def preettyJson(A):
#     for i in A:
        
#         if i == "[" and flag_1>=0 and flag_2>=0:
#             ans.append("\t"*tabcount+"[")
#             openbrace1()
#         elif i == "{" and flag_1>=0 and flag_2>=0:
#             ans.append("\t"*tabcount+"{")
#             openbrace2()
#         elif i == "}" and flag_1>=0 and flag_2>=0:
#             ans.append("\t"*tabcount+"}")
#             closebrace2()
#         elif i == "]" and flag_1>=0 and flag_2>=0:
#             ans.append("\t"*tabcount+"]")
#             closebrace1()
#         elif i == "(" or i == ")":
#             ans = []
#             bad_braces_flag = True
#             return
#         elif i==",":
#             addtoList()
#         else : addtoword()
       
            
# def printList(A):
#     if len(A)==0 and bad_braces_flag == True: 
#         print "Bad braces used"
#         return 0
    
#    	else:
#    	    print "Nothing to print"
#    	    return 
        
#     for i in A:
#         print i
#         print "\n"
    
   
    
# A = "A:"B",C:{D:"E",F:{G:"H",I:"J"}}}"
# preettyJson(A)
# print ans
# printList(ans)

#           