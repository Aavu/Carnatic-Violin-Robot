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

robot = Hathaani(tonic='D3')    # TONIC(D3) = 50


def handle_signal(sig, frame):
    robot.terminate()


if __name__ == '__main__':
    signal.signal(signal.SIGINT, handler=handle_signal)

    robot.home()
    while not robot.ready:
        time.sleep(0.25)

    # robot.play_note(midi_note=64, amplitude=0.1, duration=5, tonic=50)

    # notes = [62, 64, 66, 67, 69]
    # durations = [2, 2, 2, 2, 4]
    # amplitudes = [1, 0.75, 0.5, 0.25, .1]
    # robot.play_note_sequence(notes, durations, amplitudes, tonic=50)

    audio, fs = librosa.load("audio/1-50-3.wav", sr=16000, duration=120)
    hop_size = int(fs // 100)

    bounds = Util.get_silence_bounds(audio, fs)
    temp = Util.split(audio, bounds, int(fs / 2))
    a = temp[6][0]

    pitch_tracker = PitchTrack('rmvpe', hop_size=hop_size, rmvpe_threshold=0.01)

    # for a, _ in temp:
    e = Util.envelope(a, hop_size)
    cents = pitch_tracker.track(a, fs, return_cents=True, fill_gaps=False)
    # zero amplitudes have invalid pitches
    cents[cents < 10] = np.nan
    cents[e < 0.01] = np.nan
    e[e < 0.01] = 0
    # Invalid pitches have 0 amplitudes
    e[np.isnan(cents)] = 0
    cents, e = Util.truncate_phrase(cents, e)
    n = min(len(cents), len(e))
    phrase = np.vstack([cents, e])

    cents = Util.pitch_correct(cents)
    # plt.subplot(211)
    # plt.plot(cents)
    # plt.subplot(212)
    # plt.plot(e)
    # plt.show()
    robot.perform(phrase, phrase_tonic=50, interpolate_start=True)
    print()

    # fs = 16000
    # hop = 160
    # tp = hop / fs
    # t = 1  # s
    # N = int(t / tp)
    # print(f"Num points: {N}")
    #
    # traj = Util.interp(0, 100, 8*N)
    # traj = np.hstack([traj, Util.interp(100, 0, 8*N)]).reshape(-1, 1)
    #
    # traj = np.hstack([p_traj, a_traj, r_traj])
    # print(traj.shape)
    # robot.perform_trajectory(traj, motor_ids=[MotorId.BOW_ROTOR, MotorId.BOW_D_LEFT, MotorId.BOW_D_RIGHT])
