
from scipy.stats import norm
import matplotlib.pyplot as plt
import numpy as np
import random

pageSpeeds = np.random.normal(3.0,1.0,1000)

purchaseAmount = 100 - (pageSpeeds + np.random.normal(0,0.1,1000))*3

plt.scatter(pageSpeeds,purchaseAmount)

from scipy import stats
slope, intercept, r_value, p_value, std_err = stats.linregress(pageSpeeds,purchaseAmount)
print slope, intercept, r_value, p_value, std_err

def predict(x):
    return slope*x + intercept

fitline = predict(pageSpeeds)
plt.plot(pageSpeeds, fitline,c = 'r')

plt.show()
