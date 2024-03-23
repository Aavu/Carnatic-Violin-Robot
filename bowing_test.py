import numpy as np
from matplotlib import pyplot as plt


# refer: https://cs.gmu.edu/~kosecka/cs685/cs685-trajectories.pdf
# def linear_interp(q0, qf, N, tb_percentage=0.2):
#     _curve = np.zeros(N)
#     nb = int(round(tb_percentage * N))
#     a = (qf - q0) / (nb * (N - nb))
#     Nb = np.arange(nb + 1)
#     qA = q0 + 0.5 * a * (Nb ** 2)
#     qB = qf - qA + q0
#     _curve[:nb] = qA[:-1]
#     _curve[N - nb:] = qB[:-1][::-1]
#     _curve[nb:N - nb] = np.linspace(qA[-1], qB[-1], N - 2 * nb)
#     return _curve
#
#
# def generate_bow_trajectory(changes: list,
#                             output_size: int,
#                             last_bow_direction=None,
#                             current_bow_position_cent=0,
#                             max_velocity=0.008,  # %/sec
#                             min_bow_length=5,
#                             bow_min=0.0, bow_max=1.0,
#                             tb_cent=0.2) -> np.ndarray:
#     direction = 0 if last_bow_direction is None else last_bow_direction  # 0 means down bow, 1 means up bow
#     changes = [0] + changes + [output_size]  # ex: [0, 73, 141, 723]
#     changes = filter_bow_changes(changes, tb_cent, min_bow_length)
#     num_segments = len(changes) - 1
#     # ex: if changes = [0, 73, 141, 723], t_seg = [(0, 73), (73, 141), (141, 723)]
#     t_seg = np.zeros((num_segments, 2), dtype=int)
#
#     k = 1
#     for i in range(num_segments - 1):
#         t_seg[i, 1] = changes[k]
#         t_seg[i + 1, 0] = changes[k]
#         k += 1
#     t_seg[-1, 1] = changes[-1]
#
#     bow = np.zeros(output_size)
#
#     p0 = current_bow_position_cent if current_bow_position_cent is not None else (bow_min if direction == 0 else bow_max)
#
#     for i in range(num_segments):
#         seg = (t_seg[i, 0], t_seg[i, 1])
#         seg_len = seg[1] - seg[0]
#         bow_len = max_velocity * seg_len * (bow_max - bow_min)
#         if bow_len < (bow_max - bow_min):  # faster than max velocity
#             pf = min(bow_max, p0 + bow_len) if direction == 0 else max(0, p0 - bow_len)
#         else:
#             pf = bow_max if direction == 0 else bow_min
#
#         print(seg, bow_len, p0, pf)
#         curve = linear_interp(p0, pf, seg_len, tb_cent)
#
#         if i == 0:
#             bow[:seg[-1]] = curve
#         elif i == num_segments - 1:
#             bow[seg[0]:] = curve
#         else:
#             bow[seg[0]:seg[1]] = curve
#         direction ^= 1
#         p0 = pf
#     return bow
#
#
# def filter_bow_changes(changes, nb_cent, min_bow_length):
#     temp = []
#     if len(changes) > 0:
#         temp.append(changes[0])
#     for i in range(1, len(changes)):
#         n = changes[i] - temp[-1]
#         min_space = round(nb_cent * n)
#         if min_space > 0 and n - min_space > 0 and n > min_bow_length:
#             temp.append(changes[i])
#     return temp


if __name__ == "__main__":
    bow_change = [73, 141]  # for tb_cent = 0.2, min space required is 5 samples. This list must be sorted
    n_pitches = 723
    BOW_ENCODER_MIN = 0  # 10000
    BOW_ENCODER_MAX = 1  # 40000

    # traj = generate_bow_trajectory(bow_change, n_pitches)
    #
    # plt.plot(traj)
    # plt.show()
