import numpy as np
from scipy import stats
import matplotlib.pyplot as plt
import sys
output  = open('TestforML.txt','w')
output.seek(0)
output.truncate()
ages  = np.random.randint(18, high = 90, size = 500)
#plt.hist(ages,10)

#print >>output,stats.mode(ages)
#print >>output, "\n\n\n\n"
#print >>output,ages

#print stats.mode(ages)
#print output,ages
output.close()
#plt.show()
#plt.savefig('fig1.pdf')

incomes = np.random.normal(100.0,20.0,10000)
#plt.hist(incomes,50)

print incomes.std()
print incomes.var()

values = np.random.uniform(-10.0,10.0,100000)
x = np.arange(-3,3,0.001)
plt.plot(x,stats.norm.pdf(x))
#plt.hist(values,50)

mu = 5.0
sigma = 2.0
values = np.random.normal(mu,sigma,100000)
y=np.arange(3,7,0.0001)

plt.plot(x,stats.norm.pdf(x))
plt.plot(x,stats.expon.pdf(x))
#plt.plot(x,stats.binom.pmf(x))
mu = 500
x = np.arange(400,600,0.5)
plt.plot(x,stats.poisson.pmf(x,mu))

plt.show()
