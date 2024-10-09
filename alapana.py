"""
Author: Raghavasimhan Sankaranarayanan
Date Created: 7/21/24
"""

import time

import matplotlib.pyplot as plt
import numpy as np

from hathaani import Hathaani
import librosa
from pitchTrack import PitchTrack
from utils import Util
import sys
import signal
from definitions import *

np.set_printoptions(precision=6, threshold=sys.maxsize, suppress=True)

robot = Hathaani(tonic='D3')  # TONIC(D3) = 50


def handle_signal(sig, frame):
    robot.terminate()


if __name__ == '__main__':
    signal.signal(signal.SIGINT, handler=handle_signal)

    robot.home()
    while not robot.ready:
        time.sleep(0.25)

    audio, fs = librosa.load("audio/shankara_1.wav", sr=16000)
    hop_size = int(fs // 100)

    pitch_tracker = PitchTrack('rmvpe', hop_size=hop_size, rmvpe_threshold=0.01)

    e = Util.envelope(audio, hop_size)
    cents = pitch_tracker.track(audio, fs, return_cents=True, fill_gaps=False)
    phrase = np.vstack([cents, e])
    print(f"min pitch: {np.min(cents)}")
    plt.subplot(211)
    plt.plot(cents)
    plt.subplot(212)
    plt.plot(e)
    plt.show()

    bow_changes = [
        0, 90, 312, 446, 663, 741, 965, 1025, 1115, 1307, 1403, 1586, 1718, 1811,
        2024, 2181, 2288, 2616, 2830, 2890, 2932, 2991, 3050, 3157, 3343, 3588, 3666, 3907,
        4011, 4178, 4436, 4468, 4506, 4678, 5000, 5100, 5162, 5233, 5338, 5413, 5714, 5867,
        6017, 6137, 6344, 6567, 6624, 6663, 6718, 6880, 7028, 7200, 7330
    ]
    robot.perform(phrase, phrase_tonic=50,
                  interpolate_start=True,
                  bow_changes=bow_changes,
                  amplitude_compression=0.9,
                  pitch_correction_amount=0.5,
                  fix_open_string_pitches=True)
