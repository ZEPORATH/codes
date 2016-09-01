import pandas as pd

df = pd.read_excel('http://cdn.sundog-soft.com/Udemy/DataScience/cars.xls')
print df.head()
import statsmodels.api as sm
df.head()
df['Model_ord'] = pd.Categorical(df.Model).codes
X = df[['Mileage', 'Model_ord', 'Doors']]
Y = df[['Price']]

X1 = sm.add_constant(X)
est = sm.OLS(Y,X1).fit()

print est.summary()
print Y.groupby(df.Doors).mean()
