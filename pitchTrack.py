import librosa
import crepe
import matplotlib.pyplot as plt
import numpy as np
import torch
from utils import Util
from rmvpe import rmvpe
import os


class PitchTrack:
    def __init__(self, algorithm: str,
                 hop_size=160,
                 fmin=librosa.note_to_hz('C1'),
                 fmax=librosa.note_to_hz('C6'),
                 max_transition_rate=35.92,
                 fill_na=0, beta=(.15, 8),
                 boltzmann_parameter=.1,
                 rmvpe_threshold=0.1,
                 device='cpu'):
        self.algorithm_name = algorithm
        self.algorithm = self._pyin_track_
        self.device = device

        crepe_param = {
            'hop_length': hop_size,
            'fmin': fmin,
            'fmax': fmax,
            'device': device
        }

        pyin_param = {
            'fmin': fmin,
            'fmax': fmax,
            'max_transition_rate': max_transition_rate,
            'beta_parameters': beta,
            'boltzmann_parameter': boltzmann_parameter,
            'fill_na': fill_na,
            'frame_length': 2048,
            'hop_length': hop_size
        }

        rmvpe_param = {
            'thred': rmvpe_threshold
        }

        self.param = pyin_param
        if algorithm == 'crepe':
            self.param = crepe_param
            self.algorithm = self._crepe_track_

        if algorithm == 'rmvpe':
            dev = 'cpu' if device == 'mps' else device
            self.rmvpe = rmvpe.RMVPE(os.path.join("rmvpe", "rmvpe.pt"), is_half=False, hop_size=hop_size, device=dev)
            self.algorithm = self.rmvpe.infer_from_audio
            self.param = rmvpe_param

    def track(self, audio, fs, return_cents: bool = False, fill_gaps=False, zero_threshold=0.001, gap_threshold=50):
        if type(audio) == torch.Tensor:
            audio = audio.detach().cpu().numpy()

        out = self.algorithm(audio=audio, sample_rate=fs, return_cents=return_cents, **self.param)

        zeros = out < 1
        if return_cents:
            out[zeros] = 1
            out = librosa.hz_to_midi(out)
        out[zeros] = 0

        if fill_gaps:
            out = Util.fill_gaps(out, threshold=gap_threshold, zero_threshold=zero_threshold)

        return out

    def _pyin_track_(self, audio, sample_rate, return_cents, **kwargs):
        out, flag, prob = librosa.pyin(y=audio, sr=sample_rate, **kwargs)
        out = Util.zero_lpf(out, 0.1, ignore_zeros=True)
        out[flag == False] = np.nan
        return out

    def _crepe_track_(self, audio, sample_rate, return_cents, **kwargs):
        out = crepe.predict(audio, sample_rate, viterbi=True, verbose=0)
        return out
