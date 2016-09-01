import numpy as np
import matplotlib.pyplot as plt

vals = np.random.normal(0,0.5,10000)

plt.hist(vals,50)
#percentile gives the point,at which x% of values are less than the value
print np.percentile(vals,90)
print np.percentile(vals,20)
plt.show()
