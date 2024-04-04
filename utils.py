import matplotlib.pyplot as plt
import numpy as np
import torch
from scipy import signal
from typing import List, Tuple, Optional
import librosa


class Util:

    @staticmethod
    def get_silence_bounds(audio, fs, num_zeros=5, hop_sec=0.025, block_sec=0.05, eps=1e-6):
        hop = int(hop_sec * fs)
        e = librosa.feature.rms(y=audio, frame_length=int(block_sec * fs), hop_length=hop)[0]
        bounds = []
        i = 0
        while i < len(e) - num_zeros:
            if np.mean(e[i:i + num_zeros]) < eps:
                temp = i
                while np.mean(e[i:i + num_zeros]) < eps:
                    i += 1
                bounds.append((temp * hop, i * hop))
            else:
                i += 1

        return np.array(bounds)

    @staticmethod
    def interpolate_nan(pitches: np.ndarray) -> Tuple[np.ndarray, List[Tuple[int, int]]]:
        nan = np.argwhere(np.isnan(pitches)).flatten()
        island = []

        for i in range(len(nan)):
            if len(island) > 0 and nan[i] - island[-1][-1] == 1:
                island[-1].append(nan[i])
            else:
                island.append([nan[i]])

        nan = []
        for i in range(len(island)):
            si = island[i][0] - 1
            ei = island[i][-1] + 1
            nan.append((si + 1, ei))
            traj = Util.interp(pitches[si], pitches[ei], ei - si)
            pitches[si: ei] = traj

        return pitches, nan

    @staticmethod
    def filter_nan(nan: List[Tuple[int, int]], length: int) -> List[Tuple[int, int]]:
        out = []
        for (s, e) in nan:
            if e - s >= length:
                out.append((s, e))
        return out

    @staticmethod
    def split(audio: np.ndarray,
              silence_bounds: np.ndarray,
              min_samples: int) -> List[Tuple[np.ndarray, int]]:
        """
        Split the phrases in the audio. This function will split phrases concatenating the respective silence
        :param audio: audio data as numpy array
        :param silence_bounds: array of tuple containing start and stop indexes of silence
        :param min_samples: minimum number of samples to count as a phrase
        :return: list of phrases
        """

        def get_silence_length(idx):
            return silence_bounds[idx, 1] - silence_bounds[idx, 0]

        splits = []
        # starting
        t = audio[:silence_bounds[0, 0]]
        if len(t) > min_samples:
            splits.append((t, get_silence_length(0)))

        # All other bounds
        for i in range(len(silence_bounds) - 1):
            t = audio[silence_bounds[i, 1]: silence_bounds[i + 1, 0]]
            if len(t) > min_samples:
                splits.append((t, get_silence_length(i + 1)))

        # Ending
        t = audio[silence_bounds[-1, 1]:]
        if len(t) > min_samples:
            splits.append((t, get_silence_length(len(silence_bounds) - 1)))

        return splits

    @staticmethod
    def split_chunks(x: np.ndarray, chunk_size: int):
        shape = x.shape
        num_chunks = int(np.ceil(len(x) / chunk_size))
        chunks = np.zeros((num_chunks, chunk_size, *shape[1:]), dtype=x.dtype)
        for i in range(len(chunks) - 1):
            chunks[i] = x[i * chunk_size: (i + 1) * chunk_size]

        residual_idx = len(x) - (len(x) % chunk_size)
        if residual_idx <= len(x):
            chunks[-1] = np.concatenate(
                [x[residual_idx:], np.zeros((chunk_size - (len(x) - residual_idx), *shape[1:]))])
        return chunks

    @staticmethod
    def split_chunks_no_pad(x: np.ndarray, chunk_size: int):
        if chunk_size <= 0:
            return []

        chunks = []

        for i in range(0, len(x), chunk_size):
            chunks.append(x[i: i + chunk_size])

        return chunks

    @staticmethod
    def zero_lpf(x: np.ndarray, alpha, restore_zeros=True, ignore_zeros=False):
        # x0 = Util.lpf(x, alpha, restore_zeros=False, ignore_zeros=ignore_zeros)
        # x0 = torch.flip(x0, dims=(0,)) if type(x) == torch.Tensor else np.flip(x0, axis=(0,))
        # x0 = Util.lpf(x0, alpha, restore_zeros)
        # x0 = torch.flip(x0, dims=(0,)) if type(x) == torch.Tensor else np.flip(x0, axis=(0,))

        eps = 1e-2
        y = Util.fill_zeros(x, eps=eps) if ignore_zeros else x.copy()

        for i in range(1, len(x)):
            cur = y[i - 1] if np.isnan(y[i]) else y[i]
            y[i] = (alpha * y[i - 1]) + ((1 - alpha) * cur)

        for i in range(len(x) - 2, -1, -1):
            y[i] = (alpha * y[i + 1]) + ((1 - alpha) * y[i])

        # restore NULL values
        if restore_zeros:
            y[x < eps] = 0

        return y

    @staticmethod
    def fill_zeros(x: np.ndarray or torch.Tensor, eps=0.01) -> np.ndarray or torch.Tensor:
        """
        This function is designed for causal system.
        This means, for example if the input is [0 1 1 1 1 0 0 0 0 0],
        the output will be [1 1 1 1 1 0 0 0 0 0] and not [1 1 1 1 1 1 1 1 1 1]
        :param x: input
        :param eps: threshold
        :return: filled array
        """
        if type(x) == torch.Tensor:
            _x = x.clone()
        else:
            _x = x.copy()

        for i in range(len(_x)):
            if _x[i] < eps:
                j = i + 1
                while j < len(_x) and _x[j] < eps:
                    j += 1
                if j >= len(_x):
                    break

                start = _x[i - 1] if i > 0 else _x[j]

                if type(_x) == torch.Tensor:
                    _x[i:j] = torch.linspace(start, _x[j], j - i)
                else:
                    _x[i:j] = np.linspace(start, _x[j], j - i)
        return _x

    @staticmethod
    def lpf(x: np.ndarray or torch.Tensor, alpha, restore_zeros=True, ignore_zeros=False) -> np.ndarray or torch.Tensor:
        eps = 0.01
        if ignore_zeros:
            _x = Util.fill_zeros(x, eps=eps)
        else:
            _x = x.clone() if type(x) == torch.Tensor else x.copy()

        y = _x.clone() if type(_x) == torch.Tensor else _x.copy()

        for i in range(1, len(y)):
            y[i] = (alpha * y[i - 1]) + ((1 - alpha) * _x[i])

        # restore NULL values
        if restore_zeros:
            y[x < eps] = 0

        return y

    # https://github.com/botprof/first-order-low-pass-filter/blob/main/first-order-lpf.ipynb
    @staticmethod
    def lpf_o1(x: np.ndarray, omega, t=0.01):
        y = x.copy()
        alpha = (2 - t * omega) / (2 + t * omega)
        beta = t * omega / (2 + t * omega)
        for i in range(1, len(y)):
            y[i] = alpha * y[i - 1] + beta * (x[i] + x[i - 1])
        return y

    @staticmethod
    def transpose_to_range(pitches, min_value):
        p = pitches[pitches > 10]
        min_p = np.min(p)
        inc = 0
        while min_p < min_value:
            inc += 1
            min_p += 12

        pitches += inc * 12
        pitches[pitches < 10 + (inc * 12)] = 0
        return pitches

    @staticmethod
    def fill_gaps(x, threshold=50, zero_threshold=0.001):
        """
                Fill space between 2 notes in midi contour if the space is < 10 samples. Call this function for each frame
                :param x: The array to be filled
                :param threshold: (Optional) number of zeros below which they should be fixed
                :param zero_threshold: (Optional) The zero threshold to compare
                :return filled data
                """

        x[np.isnan(x)] = 0
        x[np.isinf(x)] = 0
        _i = 1
        while _i < len(x):
            if x[_i] < zero_threshold:
                num_zeros = 1
                for _j in range(_i + 1, len(x)):
                    if x[_j] < zero_threshold:
                        num_zeros += 1
                    else:
                        if _i > 0 and np.abs(x[_i - 1] - x[_j]) > 0.01 and num_zeros < threshold:
                            x[_i:_j] = x[_i - 1]
                        _i = _j
                        break
            _i += 1

        x[np.abs(x) == np.inf] = 0
        return x

    @staticmethod
    def envelope_fft(x: np.ndarray or torch.Tensor, sample_rate: float, hop_size: int, normalize: bool = True):
        eps = 1e-6
        # numpy and torch computations return different lengths! Usually len(np_env) - len(torch_env) = 1
        if type(x) == torch.Tensor:
            Zxx = torch.stft(x, n_fft=hop_size * 2, hop_length=hop_size, return_complex=True)
            env = torch.sum(torch.abs(Zxx), dim=0) + eps
            mav_env = torch.max(env)
        else:
            f, t, Zxx = signal.stft(x, sample_rate, nperseg=hop_size * 2, noverlap=hop_size)
            env = np.sum(np.abs(Zxx), axis=0) + eps
            mav_env = np.max(env)

        if normalize:
            env = env / mav_env

        return env

    @staticmethod
    def envelope(x: np.ndarray, hop_size: int, block_size: int = None, normalize: bool = True):
        if block_size is None:
            block_size = hop_size

        n = 1 + len(x) // hop_size
        e = np.zeros(n)
        j = 0
        for i in range(0, len(x), hop_size):
            rms = np.sqrt(np.mean(np.square(x[i: i + block_size])))
            e[j] = rms
            j += 1

        max_e = np.max(e)
        if normalize and max_e > 0:
            e = e / max_e

        return e

    @staticmethod
    def pick_dips(e: np.ndarray,
                  sample_rate: float = 16000,
                  hop_size: int = 160,
                  smoothing_alpha: float = 0.8,
                  silence_width_ms=50,
                  wait_ms: int = 80,
                  normalize: bool = True) -> List:

        max_e = np.max(e)
        if normalize and max_e > 0:
            e = e / max_e

        lpf_e = Util.zero_lpf(e, alpha=smoothing_alpha, restore_zeros=False)
        wait_samples = (wait_ms / 1000) / (hop_size / sample_rate)
        silence_width_samples = (silence_width_ms / 1000) / (hop_size / sample_rate)

        dips = []
        i = 0
        while i < len(e):
            si = i
            while i < len(e) and e[i] < lpf_e[i]:
                i += 1

            if i - si > silence_width_samples:
                min_i = si + np.argmin(lpf_e[si: i])
                mean_e = np.mean(lpf_e[si: i])
                min_e = np.min(e[si: i])
                if abs(mean_e - min_e) > 0.1:
                    dips.append(min_i)
            i += 1

        # rank and filter out until all dips are at least 'wait' distance apart
        def is_success():
            return (np.diff(dips) >= wait_samples).all() or len(dips) < 2

        while not is_success():
            dev = lpf_e[dips] - e[dips]
            min_idx = np.argmin(dev)

            del dips[min_idx]

        if len(dips) > 0 and dips[0] < wait_samples:
            dips = dips[1:]

        # Check if the last dip satisfies the wait time requirement
        if len(dips) > 0 and len(e) - dips[-1] - 1 < wait_samples:
            dips = dips[:-1]

        return dips

    @staticmethod
    def truncate_phrase(midi, envelope=None):
        # Truncate start
        start_idx = 0
        for i in range(len(midi)):
            if not (midi[i] < 1e-3 or np.isnan(midi[i]) or np.isinf(midi[i])):
                start_idx = i
                break

        # Truncate end
        end_idx = len(midi) - 1
        for i in range(len(midi) - 1, -1, -1):
            if not (midi[i] < 1e-3 or np.isnan(midi[i]) or np.isinf(midi[i])):
                end_idx = i
                break

        if envelope is not None:
            envelope = envelope[start_idx: end_idx + 1]
        return midi[start_idx: end_idx + 1], envelope

    @staticmethod
    def get_quick_transitions(pitches, threshold):
        diff = np.abs(np.diff(pitches))
        bounds = np.argwhere(diff > threshold).flatten()
        bounds = np.concatenate([[0], bounds, [len(diff) - 1]])
        return bounds

    @staticmethod
    def fix_octave_errors(pitches, threshold=10):
        bounds = Util.get_quick_transitions(pitches, threshold)
        if len(bounds) == 2:  # No octave errors
            return pitches

        for i in range(1, len(bounds) - 1):
            if bounds[i + 1] - bounds[i] < bounds[i] - bounds[i - 1]:
                d = pitches[bounds[i]] - pitches[bounds[i] + 1]
                while abs(d) > threshold:
                    if d > 0:
                        pitches[bounds[i - 1]: bounds[i]] -= 12
                    else:
                        pitches[bounds[i - 1]: bounds[i]] += 12
                    d = pitches[bounds[i]] - pitches[bounds[i] + 1]

        print(bounds)

    @staticmethod
    def pick_stationary_points(x: np.ndarray, eps: float = 0.1, return_sta=False):
        peaks = []
        dips = []
        sil = []
        # move ptr to the first non zero value
        idx = 0
        while idx < len(x) - 1 and x[idx] < eps:
            sil.append(idx)
            idx += 1

        end_idx = len(x) - 1
        while end_idx > idx and x[end_idx] < eps:
            sil.append(end_idx)
            end_idx -= 1

        # Always make the first valid point as part of the sta. If 2nd pt > 1st, it is dip else peak
        if idx < len(x) - 1:
            if x[idx] < x[idx + 1]:
                dips.append(idx)
            else:
                peaks.append(idx)
            idx += 1

        # Always make the last valid point as part of the sta. If 2nd pt > 1st, it is dip else peak
        if end_idx > idx:
            if x[end_idx] < x[end_idx - 1]:
                dips.append(end_idx)
            else:
                peaks.append(end_idx)
            end_idx -= 1

        in_silent_region = False
        # Scan the entire contour. If a point is a minima, add to dip. If a point is maxima, add to peak
        for i in range(idx + 1, end_idx):
            # First check if the next point is a valid pitch. If not, add current point to sta dip
            if x[i] < eps:
                if not in_silent_region:
                    dips.append(i - 1)
                    in_silent_region = True
                sil.append(i)
                continue

            # If still in silent region but the current point > eps, Add current point to sta
            if in_silent_region:
                if x[i + 1] > x[i]:
                    dips.append(i)
                else:
                    peaks.append(i)
                in_silent_region = False
                continue

            if x[i + 1] <= x[i] and x[i] > x[i - 1]:
                peaks.append(i)

            elif x[i + 1] > x[i] and x[i] <= x[i - 1]:
                dips.append(i)

        if return_sta:
            return np.sort(np.hstack([peaks, dips]))

        peaks = np.sort(np.array(peaks, dtype=int))
        dips = np.sort(np.array(dips, dtype=int))
        sil = np.sort(np.array(sil, dtype=int))

        return peaks, dips, sil

    @staticmethod
    def get_nearest_sta(sta: np.ndarray, index: int, direction: int = 0, min_distance=10):
        # TODO: Improve with binary search
        nearest_lower = sta[0] if len(sta) > 0 else -1
        for i in range(len(sta) - 1, -1, -1):
            if index - sta[i] > min_distance:
                nearest_lower = sta[i]
                break

        nearest_higher = sta[-1] if len(sta) > 0 else -1

        for i in range(len(sta)):
            if sta[i] - index > min_distance:
                nearest_higher = sta[i]
                break

        if direction < 0:
            return nearest_lower
        elif direction > 0:
            return nearest_higher

        return min(nearest_lower, nearest_higher)

    @staticmethod
    def decompose_carnatic_components(x: np.ndarray,
                                      threshold_semitones: float = 0.3,
                                      min_cp_note_length: int = 10,
                                      min_sub_phrase_length: int = 3,
                                      first_sta_value: float = None,
                                      return_raw_sta: bool = True):
        cpn = []
        peaks, dips, sil = Util.pick_stationary_points(x)
        sta = np.sort(np.hstack([peaks, dips]))
        if len(sta) < 2:
            return sta, cpn
        mid_x = np.zeros((len(sta) - 1,))
        mid_idx = np.zeros_like(mid_x)  # required only for plotting

        for i in range(len(mid_x)):
            if i == 0 and first_sta_value is not None:
                mid_x[i] = (first_sta_value + x[sta[i + 1]]) / 2
            else:
                mid_x[i] = (x[sta[i]] + x[sta[i + 1]]) / 2
            mid_idx[i] = (sta[i] + sta[i + 1]) / 2

        raw_sta = sta.copy()
        eps = 0.1
        i = 0
        while i < len(mid_x):
            candidate_idx = None
            j = i + 1

            while j < len(mid_x) and abs(mid_x[j] - mid_x[j - 1]) < threshold_semitones and x[sta[j]] > eps and x[
                sta[j - 1]] > eps:
                if candidate_idx is None:
                    candidate_idx = j - 1
                j += 1

            if candidate_idx is not None:
                j1 = min(len(sta) - 1, j)
                s, e = sta[candidate_idx], sta[j1]
                l = e - s + 1
                if l >= min_cp_note_length:
                    note = np.median(x[s: e])
                    appended = False
                    if len(cpn) > 0:
                        n0, s0, l0 = cpn[-1]
                        if (s + l) - (s0 + l0) < min_sub_phrase_length and abs(note - n0) < threshold_semitones:
                            cpn[-1] = ((n0 + note) / 2, s0, l0 + l)
                            appended = True

                    if not appended:
                        cpn.append((note, s, l))
                    sta[candidate_idx: j1] = -1
            i = j

        # handle last idx of sta
        if len(cpn) > 0:
            n0, s0, l0 = cpn[-1]
            if s0 <= sta[-1] <= s0 + l0:
                sta[-1] = -1

        sta = sta[sta > -1]

        if return_raw_sta:
            return raw_sta, cpn
        return sta, cpn

    @staticmethod
    def find_cpn(phrase, sta, min_cpn_length=10, min_sub_phrase_length=5, cp_threshold=0.35):
        filtered = Util.zero_lpf(phrase, 0.75)
        raw_cpn = [(phrase[0], 0, 1)]

        for i in range(1, len(phrase)):
            if abs(filtered[i] - phrase[i]) < cp_threshold:
                n, s, l = raw_cpn[-1]
                if i - (s + l) < min_sub_phrase_length:
                    raw_cpn[-1] = ((n + phrase[i]) / 2, s, l + 1)
                else:
                    raw_cpn.append((phrase[i], i, 1))

        raw = []
        cpn = []
        for n, s, l in raw_cpn:
            e = s + l
            raw.append((int(np.round(np.median(phrase[s: e]))), s, e - s))
            _s = Util.get_nearest_sta(sta, s, direction=1, min_distance=0)
            e = Util.get_nearest_sta(sta, s + l, direction=-1, min_distance=0)
            l = e - _s
            if l > min_cpn_length:
                cpn.append((int(np.round(np.median(phrase[_s: e]))), _s, e - _s))
        return cpn

    @staticmethod
    def find_cpn_v1(phrase, min_cp_length=10, min_sub_phrase_length=5, cp_threshold=0.3):
        cpn = []

        i = 1
        while i < len(phrase):
            if abs(phrase[i] - phrase[i-1]) < cp_threshold:
                start_idx = i - 1
                while i < len(phrase) and abs(phrase[i] - phrase[i-1]) < cp_threshold:
                    i += 1

                i -= 1
                if i - start_idx > min_cp_length:
                    if len(cpn) > 0 and cpn[-1][1] + cpn[-1][2] < min_sub_phrase_length:
                        n, s, l = cpn[-1]
                        n = round((n + np.median(phrase[start_idx: i + 1])) / 2)
                        cpn[-1] = (int(n), s, l + i + 1 - start_idx)
                    else:
                        n = np.round(np.median(phrase[start_idx: i + 1]))
                        cpn.append((int(n), start_idx, i + 1 - start_idx))
            i += 1

        return cpn

    # refer: https://cs.gmu.edu/~kosecka/cs685/cs685-trajectories.pdf
    @staticmethod
    def interp(q0, qf, N, tb_percentage=0.45, dtype=float):
        if N < 2:
            return np.array([q0, qf])[:N]
        _curve = np.zeros(N, dtype=dtype)
        nb = min(N - 1, max(1, int(tb_percentage * N)))
        a = (qf - q0) / (nb * (N - nb))
        Nb = np.arange(nb + 1)
        qA = q0 + 0.5 * a * (Nb ** 2)
        qB = qf - qA + q0
        _curve[:nb] = qA[:-1]
        _curve[N - nb:] = qB[:-1][::-1]
        _curve[nb:N - nb] = np.linspace(qA[-1], qB[-1], N - 2 * nb, dtype=dtype)
        return _curve

    @staticmethod
    def generate_trajectory(q0, v_max_cent, N, tb_percentage=0.2):
        return Util.interp(q0, q0 + N * v_max_cent, N, tb_percentage)

    @staticmethod
    def pitch_correct(x: np.ndarray):
        """
        Given a pitch contour,
        fix intonation by shifting all the STAs to the closest whole semitone and rebuilt contour by interpolating
        :param x: Input pitch contour
        :return: corrected contour, sta
        """
        sta = Util.pick_stationary_points(x, return_sta=True)
        out = x.copy()
        for i in range(len(sta) - 1):
            s, e = sta[i], sta[i + 1]
            q0, qf = round(x[s]), round(x[e])
            l = e - s + 1 if e == len(x) - 1 else e - s
            out[s: s + l] = Util.interp(q0, qf, l)

        out[np.isnan(x)] = np.nan
        return out
