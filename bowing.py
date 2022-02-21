import numpy as np
from matplotlib import pyplot as plt

bow_change = [73, 141, 200]
n_pitches = 723
BOW_ENCODER_MIN = 0 # 8000
BOW_ENCODER_MAX = 1 # 40000

dir = True

num_segments = len(bow_change) + 1
t_seg = np.empty((num_segments, 2))
t_seg[0] = np.array((0, bow_change[0]))
for i in range(len(bow_change) - 1):
    t_seg[i+1] = np.array((bow_change[i], bow_change[i+1]))
t_seg[-1] = np.array((bow_change[-1], n_pitches))

bow = np.zeros(n_pitches)

for i in range(num_segments):
    seg = (int(t_seg[i, 0]), int(t_seg[i, 1]))
    seg_len = seg[1] - seg[0]
    print(seg, seg_len)
    t = np.arange(seg_len) / seg_len
    p0 = BOW_ENCODER_MIN if (dir) else BOW_ENCODER_MAX
    pf = BOW_ENCODER_MAX if (dir) else BOW_ENCODER_MIN
    a = [p0, 3*(pf-p0), 2*(pf - p0)]

    # Line
    curve = np.linspace(p0, pf, seg_len)

    if (seg_len < 100):
        # Generate 3rd order poly
        curve = a[0] + a[1]*(t**2) - a[2]*(np.power(t, 3))

    if i == 0:
        bow[:seg[-1]] = curve
    elif i == num_segments - 1:
        bow[seg[0]:] = curve
    else:
        bow[seg[0]:seg[1]] = curve
    dir ^= True

# bow = np.round(bow).astype(int)
np.savetxt("bow.txt", [bow], delimiter=",")
plt.plot(bow)
plt.show()