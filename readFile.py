import os
import re
import Tkinter as tk
start = '20111124'
end = '20111131'

class customer(object):
    def __init__(self,name):
        self.id = name
    def no_of_drops():
        self.dropcount+=1
    def no_of_disconnects():
        self.disconnects+=1
    def no_of_avg_ltd_exceeded():
        self.avgltdexceeded+=1
def resolve_file_name(start,end):
    lst = []
    lst = start.split('-')
    lst.reverse()
    #print lst
    start=''
    for i in lst:
        start+=i
    lst = []
    lst = end.split('-')
    lst.reverse()
    #print lst
    end=''
    for i in lst:
        end+=i
    return start,end

def printcontents(filename,path):
    print "Process %s" % filename
                        
    print "**********************************"
    print "Printing from file : %s" %filename
    print "**********************************"
    f = open(path+"\%s"%filename,'r')
    contents = f.readlines()
    for line in contents:
        #print line
        print_no_of_disconnects(line)
    f.close()

def print_no_of_disconnects(line):
    if 'Client is disconnected from agent.' in line:
        print True

def search_contents(start,end,path):

    for filename in os.listdir(path):
        #print filename.split('.')
        #print type(filename)
        if start <= filename.split('.')[0] <= end:
            printcontents(filename,path)        

if __name__ == '__main__':
    m,n = resolve_file_name('24-11-2011','30-11-2011')
    print m," ",n  ,type(m), type(m)
    search_contents(m,n,'log')
    
