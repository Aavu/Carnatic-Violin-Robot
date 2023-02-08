import numpy as np
from matplotlib import pyplot as plt

PDO_RATE = 5
n = 1500
f = 1/(n * PDO_RATE/1000)
i = np.arange(n)
a = np.round(((40000 - 8000)/2)*(1 + np.cos(2*np.pi*f*i/(1000/PDO_RATE))) + 8000)
plt.plot(a)
plt.show()