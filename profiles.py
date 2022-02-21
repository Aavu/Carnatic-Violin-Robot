import numpy as np
from matplotlib import pyplot as plt

PDO_RATE = 5
p0 = 8000
pf = 40000
n = 1500
t = np.arange(n) / n

f = 0.5
o1 = np.round(((40000 - 8000)/2)*(1 + np.cos(2*np.pi*f*t + np.pi)) + 8000)

# 3rd order poly
a = [p0, 3*(pf-p0), 2*(pf - p0)]
o3 = a[0] + a[1]*(t**2) - a[2]*(np.power(t, 3))

#5th order poly
a = [p0, 0, 0, (pf - p0)*10/3, -(pf - p0)*5/3, -(pf - p0)*2/3]
o5 = a[0] + a[1]*t + a[2]*np.power(t, 2) + a[3]*np.power(t, 3) + a[4]*np.power(t, 4) + a[5]*np.power(t, 5)

plt.plot(t, o3, 'r')
plt.plot(t, o5, 'y')
plt.plot(t, o1)
plt.show()