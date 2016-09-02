from scipy.stats import norm
import matplotlib.pyplot as plt
import numpy as np
plt.xkcd()
x = np.arange(-3,3,0.001)
axes = plt.axes()
axes.grid()
plt.xlabel('Greebles')
plt.ylabel('Probaility')
plt.plot(x,norm.pdf(x))
plt.plot(x,norm.pdf(x,1.0,0.5),'r:')
plt.legend(['Sneetches','Gacks'],loc=1)
plt.savefig('MyPlot.png', format='png')
plt.show()
