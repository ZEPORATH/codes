from scipy.stats import norm
import matplotlib.pyplot as plt
import numpy as np
import random

random.seed(0)

totals = {20:0,30:0,40:0,50:0,60:0,70:0}
purchases = {20:0,30:0,40:0,50:0,60:0,70:0}
totalPurchases =0
for _ in range(100000):
    ageDecade = random.choice([20,30,40,50,60,70])
    purchaseProbaility = float(ageDecade) /100.0
    totals[ageDecade] +=1
    if(random.random() < purchaseProbaility):
        totalPurchases +=1
        purchases[ageDecade] +=1

PEF = float(purchases[30])/float(totals[30])
print "P(purcharse | 30s): " , PEF

print totals
print purchases
print totalPurchases

