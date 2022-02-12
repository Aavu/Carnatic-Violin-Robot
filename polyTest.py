import numpy as np
import matplotlib.pyplot as plt

m = int(1000/6)
t = np.arange(m)

p = 1

x = (3*p*np.square(t/m)) - (2*p*np.power(t/m, 3))

plt.plot(t, x)
plt.show()