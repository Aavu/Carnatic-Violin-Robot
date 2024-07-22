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

    # robot.play_note(midi_note=64, amplitude=.5, duration=5, tonic=50)

    # notes = [62, 64, 66, 67, 69, 71, 73, 74, 0, 74, 73, 71, 69, 67, 66, 64, 62]
    # durations = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    # amplitudes = [0.75, 0.75, 0.75, 0.75, 0.75, 0.75, 0.75, 0.75, 0, 0.75, 0.75, 0.75, 0.75, 0.75, 0.75, 0.75, 0.75]
    # robot.play_note_sequence(notes, durations, amplitudes, tonic=50)

    audio, fs = librosa.load("audio/anandha.wav", sr=16000)
    hop_size = int(fs // 100)

    # bounds = Util.get_silence_bounds(audio, fs)
    # temp = Util.split(audio, bounds, int(fs / 2))
    # a = temp[4][0]

    pitch_tracker = PitchTrack('rmvpe', hop_size=hop_size, rmvpe_threshold=0.01)

    # for a, _ in temp:
    e = Util.envelope(audio, hop_size)
    cents = pitch_tracker.track(audio, fs, return_cents=True, fill_gaps=False)
    phrase = np.vstack([cents, e])
    print(f"min pitch: {np.min(cents)}")
    # Anandha - [0, 80, 130, 178, 227, 274]
    # Arabhi - [0, 57, 161, 198, 235]
    # Sahana - [0, 59, 145, 217] , start 150mm
    # bhairavi - [0, 57, 96, 190, 236]
    # Thodi - [0, 40, 74, 215]
    # Suruti - [0, 56, 100, 209]
    # neelambari - [0, 56, 97, 170, 360, 434, 535]
    # Sindhu - [0, 28, 62, 228, 266, 300]
    # Kaanada - [0, 33, 67, 115, 227, 267]
    # Mohanam - [0, 41, 84, 166, 241, 295, 339, 424]
    # Dhanyasi - [0, 38, 143, 198, 240], start 150mm
    # Saveri - [0, 114, 223]

    # spurita0 - [0, 48, 91, 127, 167, 204, 245, 285, 327]
    # spurita1 - [0, 46, 86, 126, 163, 200, 240, 275, 320]
    # spurita2 - [0, 46, 93, 132, 178, 217, 260, 303, 351]
    # Kampita0 - [0, 94, 122, 153, 205]
    # Kampita1 - [0, 104, 150, 197]
    # Kampita2 - [0, 114]
    # Nokku - [0, 36, 73, 109, 135, 175]
    # Orikkai - [0, 46, 89, 133, 178]
    # Kandippu2 - [0, 54, 98, 147]

    bow_changes = [0, 80, 130, 178, 227, 274]
    robot.perform(phrase, phrase_tonic=50,
                  bow_changes=bow_changes,
                  interpolate_start=True,
                  amplitude_compression=0.9,
                  pitch_correction_amount=0.5,
                  fix_open_string_pitches=True)


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
