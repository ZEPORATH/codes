import numpy as np
import matplotlib.pyplot as plt
from sklearn.metrics import r2_score


np.random.seed(2)

pageSpeeds = np.random.normal(3.0,1.0,100)
purchaseAmount = np.random.normal(50.0,30.0,100)/pageSpeeds

plt.scatter(pageSpeeds,purchaseAmount)
plt.show()

#seperate the data
#80# -- Training data
#20% -- Testing Data

trainX = pageSpeeds[:80]
trainY = purchaseAmount[:80]

testX = pageSpeeds[80:]
testY = purchaseAmount[80:]

plt.close()
plt.scatter(trainX, trainY)
plt.show()
plt.close()

x = np.array(trainX)
y = np.array(trainY)
#use a polynomial regression
p4 = np.poly1d(np.polyfit(x,y,8))
xp = np.linspace(0,7,100)

axes = plt.axes()
axes.set_xlim([0,7])
axes.set_ylim([0,200])
plt.scatter(x,y)
plt.plot(xp, p4(xp) ,c='r')
plt.show()
plt.close()
#check r2 score for test data
r2 = r2_score(testY, p4(testX))
print r2

#check r2 score for train data
r2 = r2_score(np.array(trainY),p4(np.array(trainX)))
print r2
                      
