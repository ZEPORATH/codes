import os
import re
start = '20111124'
end = '20111131'

def resolve_file_name(start,end):
	lst = []
	lst = start.split('-')
	lst[::-1]
	start=''
	for i in lst:
		start+=i
	lst = []
	lst = end.split('-')
	lst[::-1]
	end=''
	for i in lst:
		end+=i
	return start,end

def search_contents(start,end,path):

	for filename in os.listdir(path):
		# print filename.split('.')
	    if start <= filename.split('.')[0] <= end:
	        print "Process %s" % filename	

if __name__ == '__main__':
	m,n = resolve_file_name('24-11-2011','30-11-2011')
	print m," ",n  ,type(m), type(m)
	search_contents(m,n,'log')