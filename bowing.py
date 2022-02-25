from tokenize import single_quoted
import numpy as np
from matplotlib import pyplot as plt

# refer: https://cs.gmu.edu/~kosecka/cs685/cs685-trajectories.pdf
def linear_interp(q0, qf, N, tb_percentage=0.1):
    _curve = np.zeros(N)
    nb = int(tb_percentage*N)
    a = (qf - q0) / (nb*(N - nb))
    Nb = np.arange(nb + 1)
    qA = q0 + 0.5*a*(Nb**2)
    qB = qf - qA + q0
    _curve[:nb] = qA[:-1]
    _curve[N-nb:] = qB[:-1][::-1]
    _curve[nb:N-nb] = np.linspace(qA[-1], qB[-1], N-2*nb)
    return _curve

def test(q0, qf, N, tb=0.1):
    _curve = np.zeros(N)
    nb = int(tb*N)
    a = (qf - q0) / (nb*(N - nb))
    for i in range(nb):
        _curve[i] = q0 + 0.5*a*(i**2)
     
    for i in range(N-nb, N):
        _curve[N - nb - i - 1] = qf - 0.5*a*((i - N + nb)**2)

    qa = q0 + 0.5*a*(nb**2)
    qb = qf - 0.5*a*(nb**2)
    # _curve[nb:N-nb] = np.linspace(qa, qb, N - (2*nb))

    for i in range(N - 2*nb):
        _curve[i + nb] = qa + (i / (N - 2*nb - 1)) * (qb - qa)

    return _curve


bow_change = [73, 141, 300]
n_pitches = 723
BOW_ENCODER_MIN = 0 # 8000
BOW_ENCODER_MAX = 1 # 40000

MAX_VELOCITY = 0.008 # %/sec

dir = True  # True : Down bow

num_segments = len(bow_change) + 1
t_seg = np.empty((num_segments, 2))
t_seg[0] = np.array((0, bow_change[0]))
for i in range(len(bow_change) - 1):
    t_seg[i+1] = np.array((bow_change[i], bow_change[i+1]))
t_seg[-1] = np.array((bow_change[-1], n_pitches))

bow = np.zeros(n_pitches)

p0 = BOW_ENCODER_MIN if (dir) else BOW_ENCODER_MAX

### Eval ###
# pf = BOW_ENCODER_MAX if dir else BOW_ENCODER_MIN
# seg_len = 100
# curve = linear_interp(p0, pf, seg_len, 0.1)
# curve1 = test(0, 1, seg_len, 0.1)
# print(np.mean(np.square(curve - curve1)))
# plt.plot(curve)
# plt.plot(curve1)
# plt.show()
# exit()

# BOW_ENCODER_MAX = BOW_ENCODER_MIN + (vel * seg_len)
for i in range(num_segments):
    seg = (int(t_seg[i, 0]), int(t_seg[i, 1]))
    seg_len = seg[1] - seg[0]
    bow_len = MAX_VELOCITY * seg_len
    if bow_len < 1: # faster than max velocity
        pf = p0 + bow_len if dir else p0 - bow_len
    else:
        pf = BOW_ENCODER_MAX if dir else BOW_ENCODER_MIN
    
    print(bow_len, p0, pf)
    # t = np.arange(seg_len) / seg_len
    # a = [p0, 3*(pf-p0), 2*(pf - p0)]

    # Line
    # curve = np.linspace(p0, pf, seg_len)

    # if (seg_len < 100):
        # Generate 3rd order poly
        # curve = a[0] + a[1]*(t**2) - a[2]*(np.power(t, 3))

    # linear interp with parabolic blend
    curve = linear_interp(p0, pf, seg_len, 0.1)

    if i == 0:
        bow[:seg[-1]] = curve
    elif i == num_segments - 1:
        bow[seg[0]:] = curve
    else:
        bow[seg[0]:seg[1]] = curve
    dir ^= True
    p0 = pf

# bow = np.round(bow).astype(int)
np.savetxt("bow.txt", [bow], delimiter=",")
# plt.plot(bow)
# plt.show()