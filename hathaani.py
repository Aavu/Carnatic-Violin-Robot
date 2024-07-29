import librosa
import numpy as np
from matplotlib import pyplot as plt
from utils import Util
from definitions import *
from udpHandler import UDPHandler
from typing import Tuple, List, Optional


class Hathaani:
    # Total range of notes that can be played on a violin with tuning D A D A (on E A D G)
    NOTE_RANGE = Range(57, 94)
    # Maximum semitones a string can support
    RANGE_PER_STRING = 14
    TONIC = librosa.note_to_midi('D3')  # 50

    class Util:
        @staticmethod
        def filter_bow_changes(changes, nb_cent, min_bow_length):
            """
            Filter bow changes such that each segment is at least min_bow_length
            :param changes: Bow changes
            :param nb_cent:
            :param min_bow_length:
            :return:
            """
            temp = []
            if len(changes) > 0:
                temp.append(changes[0])
            for i in range(1, len(changes)):
                n = changes[i] - temp[-1]
                min_space = round(nb_cent * n)
                if min_space > 0 and n - min_space > 0 and n > min_bow_length:
                    temp.append(changes[i])

            # Make sure the last change matches the original since this is denotes length of the array
            temp[-1] = changes[-1]
            return temp

        @staticmethod
        def filter_string_changes(changes: List[Tuple[int, int, int, float]],
                                  min_length_suggestion: int, range_per_string: int):
            i = 0
            while i < len(changes) - 1:
                s, e, sid, maxp = changes[i]
                ns, ne, n_sid, n_maxp = changes[i + 1]
                if (e - s < min_length_suggestion) and (sid < n_sid) and (maxp - TUNING[n_sid] < range_per_string):
                    changes[i + 1] = (s, ne, n_sid, max(maxp, n_maxp))
                    changes.remove(changes[i])
                else:
                    i += 1
            return changes

        @staticmethod
        def to_bow_height_mm(amplitudes: np.ndarray,
                             segments: List[Tuple[int, int, int, float]],
                             string_change_period=0):
            """
            Convert amplitude envelope trajectory [0, 1) to bow height max and min.
            :param amplitudes: The amplitude envelope of the phrase.
            :param segments: String changes
            :param string_change_period: time period of string change traj
            :return: Bow height (mm) trajectory
            """
            out = np.zeros_like(amplitudes)
            o = 0  # string change time offset
            for (s, e, sid, _) in segments:
                s, e = s + o, e + o
                out[s: e] = (amplitudes[s: e] * (BOW_HEIGHT[sid].MAX - BOW_HEIGHT[sid].MIN)) + BOW_HEIGHT[sid].MIN
                o += string_change_period

            o = 0
            for i in range(1, len(segments)):
                _, e0, _, _ = segments[i - 1]
                s, _, _, _ = segments[i]
                e0 = e0 + o
                o += string_change_period
                s = s + o
                out[e0: s] = Util.interp(out[e0 - 1], out[s], s - e0)
            out[out < 0] = 0
            return Util.zero_lpf(out, 0.5, restore_zeros=False)

        @staticmethod
        def to_left_hand_position_mm(fret_number: np.ndarray or float) -> np.ndarray:
            return SCALE_LENGTH * (1 - (2 ** (-fret_number / 12))) + NUT_POSITION_MM

        @staticmethod
        def from_left_hand_position_mm(fret_pos: np.ndarray) -> np.ndarray:
            """
            Convert absolute position in mm to fret number. \n
            Uses:
            .. math::
            -12*log(1 - (y / s)) = x

            :param fret_pos: Fret Positions
            :return: Pitch values
            """
            fret_pos -= NUT_POSITION_MM
            return -12 * np.log2(1 - (fret_pos / SCALE_LENGTH))

        @staticmethod
        def string_id_to_mm(current_id, prev_id=None):
            """

            :param current_id:
            :param prev_id:
            :return:
            """
            # Possibilities
            # [[scp.EA, scp.EA, scp.EA, scp.EA, scp.EA],
            #  [scp.AD, scp.EA, scp.AD, scp.AD, scp.AD],
            #  [scp.AD, scp.AD, scp.AD, scp.AD, scp.DG],
            #  [scp.DG, scp.DG, scp.DG, scp.DG, scp.DG]]

            sid_map = np.array([[0, 0, 0, 0, 0],
                                [1, 0, 1, 1, 1],
                                [1, 1, 1, 1, 2],
                                [2, 2, 2, 2, 2]])

            assert current_id < len(sid_map), f"Current ID out of range"

            prev_id = 0 if prev_id is None else prev_id + 1
            assert prev_id < len(sid_map[0]), f"Prev ID out of range"

            return sid_map[current_id, prev_id]

        @staticmethod
        def to_bow_differential(positions: np.ndarray or float,
                                angles: np.ndarray or float,
                                is_radian=True,
                                smoothing_alpha=0.15) -> Tuple[np.ndarray or float, np.ndarray or float]:
            """
            Convert the position and angles to Left and Right differentials
            :param positions: Height of the bow.
            :param angles: bow angles
            :param is_radian: Whether the input is in radians. Degrees if False
            :param smoothing_alpha: smoothing coefficient for zero phase lpf applied to differential values
            :return: Left, right differential trajectories
            """
            if not is_radian:
                angles = np.deg2rad(angles)

            if np.max(np.abs(angles)) > BOW_ANGLE_LIMIT:
                raise Exception(f"Angle limit exceeded. Max angle: {np.max(angles)}, Min angle: {np.min(angles)}")

            phi_x = BOW_CENTER_RAIL_DISTANCE * np.tan(angles)

            # Same sign for phi because of the nature of the design, rotations are inverted
            left = positions - phi_x
            right = positions + phi_x

            if isinstance(positions, np.ndarray):
                left = Util.zero_lpf(left, smoothing_alpha, False)
                right = Util.zero_lpf(right, smoothing_alpha, False)

            return left, right

        @staticmethod
        def from_bow_differential(left: np.ndarray or float,
                                  right: np.ndarray or float,
                                  return_radian=True) -> Tuple[np.ndarray or float, np.ndarray or float]:
            phi_d = (right + left) / 2
            phi_x = (right - left) / 2
            positions = phi_d * GT2_PULLEY_RADIUS
            x = phi_x * GT2_PULLEY_RADIUS
            angles = np.arctan(x / BOW_CENTER_RAIL_DISTANCE)

            if not return_radian:
                angles = np.rad2deg(angles)
            return positions, angles

        @staticmethod
        def fix_open_string_pitches(cents, sta, threshold):
            # make sure the first sta idx is 0
            if len(sta) > 0 and sta[0] != 0:
                sta = [0] + sta

            def open_string(pitch, th):
                if abs(pitch - TUNING[0]) < th:
                    return TUNING[0]
                elif abs(pitch - TUNING[1]) < th:
                    return TUNING[1]
                elif abs(pitch - TUNING[2]) < th:
                    return TUNING[2]
                elif abs(pitch - TUNING[3]) < th:
                    return TUNING[3]
                return None

            # Since the first sta idx is 0, no interpolation from prev sta is required
            if len(sta) > 0:
                os = open_string(cents[sta[0]], threshold)
                if os is not None:
                    cents[sta[0]] = os

            for i in range(1, len(sta)):
                p = open_string(cents[sta[i]], threshold)
                # if it is an open string pitch, interpolate from last sta to this one
                if p is not None:
                    s = sta[i - 1]
                    e = min(sta[i] + 1, len(cents))
                    cents[s: e] = Util.interp(cents[s], p, e - s)

            return cents

        """Adjustments for deviations"""

        @staticmethod
        def adjust_finger_height(finger: np.ndarray or float, left_hand: np.ndarray or float):
            assert len(finger) == len(left_hand)
            cent = np.minimum(1.0, left_hand * 1.0 / FINGERBOARD_LENGTH)
            dev = 1 - ((finger - FINGER_HEIGHT.OFF) / (FINGER_HEIGHT.ON - FINGER_HEIGHT.OFF))
            finger -= dev * cent * STRING_HEIGHT.END
            return np.maximum(finger, 0)

        @staticmethod
        def adjust_string_change_deviation(str_chg: np.ndarray, left_hand: np.ndarray):
            assert len(str_chg) == len(left_hand)
            nut_dev = max(STR_CHG_POS.EA.MIN, STR_CHG_POS.DG.MIN)
            bridge_dev = max(STR_CHG_POS.EA.MAX, STR_CHG_POS.DG.MAX)
            diff = str_chg - STR_CHG_POS.AD.MIN
            dev = (diff / nut_dev) * bridge_dev
            return str_chg + (dev * left_hand / SCALE_LENGTH)

        @staticmethod
        def adjust_bow_height_for_left_hand(height: np.ndarray, left_hand_mm: np.ndarray, deviation: int):
            dev = deviation * np.minimum(1.0, left_hand_mm / SCALE_LENGTH)
            return height + dev

        @staticmethod
        def get_bow_slide_traj(left_hand_mm: np.ndarray):
            return Util.zero_lpf(np.minimum(1.0, left_hand_mm / SCALE_LENGTH), 0.85)

        @staticmethod
        def adjust_for_deviations(x: np.ndarray) -> np.ndarray:
            """
            Adjust for all deviations.
            :param x: (N, 7) values corresponding to 7 DoF
            :return: Adjusted values
            """
            x = x.copy()

            # Finger height deviation
            x[:, MotorId.FINGER] = Hathaani.Util.adjust_finger_height(x[:, MotorId.FINGER], x[:, MotorId.LEFT_HAND])
            # String change deviation
            x[:, MotorId.STRING_CHG] = Hathaani.Util.adjust_string_change_deviation(x[:, MotorId.STRING_CHG],
                                                                                    x[:, MotorId.LEFT_HAND])
            # Bow Height deviation
            x[:, MotorId.BOW_D_LEFT] = Hathaani.Util.adjust_bow_height_for_left_hand(x[:, MotorId.BOW_D_LEFT],
                                                                                     x[:, MotorId.LEFT_HAND],
                                                                                     BOW_HEIGHT_VERTICAL_DEVIATION)
            # Bow slide
            x[:, MotorId.BOW_SLIDE] = Hathaani.Util.get_bow_slide_traj(x[:, MotorId.LEFT_HAND])

            return x

        ########################################################

        @staticmethod
        def to_motor_data(x: np.ndarray) -> np.ndarray:
            """
            Convert all the values to the respective encoder ticks. Transform zPos and angle to differential
            :param x: (N, 7) values corresponding to 7 DoF
            :return: transformed values
            """
            data = Hathaani.Util.adjust_for_deviations(x)

            # BOW_LEFT_DIFF stores height and BOW_RIGHT_DIFF stores the rotation
            data[:, MotorId.BOW_D_LEFT], data[:, MotorId.BOW_D_RIGHT] = Hathaani.Util.to_bow_differential(
                data[:, MotorId.BOW_D_LEFT],
                data[:, MotorId.BOW_D_RIGHT],
                is_radian=True)

            return np.round(data * np.array(TICKS)).astype(np.uint16)

        @staticmethod
        def from_motor_data(x: np.ndarray) -> np.ndarray:
            """
            Convert all the values from encoder ticks to mm. Transform differential to zPos and angle
            :param x: (N, 7) values corresponding to 7 DoF
            :return: transformed values
            """
            data = x.copy().astype(float)

            data = data / np.array(TICKS)
            # data[:, MotorId.STRING_CHG] = Hathaani.Util.from_string_change_angle(data[:, MotorId.STRING_CHG])
            data[:, MotorId.BOW_D_LEFT], data[:, MotorId.BOW_D_RIGHT] = Hathaani.Util.from_bow_differential(
                data[:, MotorId.BOW_D_LEFT],
                data[:, MotorId.BOW_D_RIGHT],
                return_radian=True)

            return data

    def __init__(self, tonic: str or int, hop_size=160, sample_rate=16000):
        self.TONIC = librosa.note_to_midi(tonic) if isinstance(tonic, str) else tonic
        self.hop = hop_size
        self.fs = sample_rate
        self.time_per_sample = hop_size / sample_rate
        self.finger_press_time = int(round(FINGER_PRESS_TIME_MS / (self.time_per_sample * 1000)))
        self.string_change_time = int(round(STRING_CHANGE_TIME_MS / (self.time_per_sample * 1000)))
        self.bow_direction = 0

        self.network_handler = None
        if not SIMULATE:
            self.network_handler = UDPHandler(DEVICE_IP, DEVICE_PORT, cmd_callback=self.network_callback)
            self.network_handler.start()

        # Node-id order, units in mm. Bow angle in radians and Finger in thousands of torque
        self.initial_positions: np.ndarray = np.array([FINGER_HEIGHT.REST,  # FINGER Torque
                                                       STR_CHG_POS.AD.MIN,  # STRING_CHG
                                                       0,  # LEFT_HAND
                                                       BOW_HEIGHT.REST,  # Bow position i.e. BOW_D_LEFT
                                                       BOW_ANGLE.REST,  # Bow angle i.e. BOW_D_RIGHT
                                                       0,  # BOW_SLIDE
                                                       0])  # BOW_ROTOR
        self.current: np.ndarray = self.initial_positions.copy()

        self.is_initialized = False  # this flag is set when all actuators are initialized and homed

    @property
    def ready(self):
        return self.is_initialized

    def terminate(self):
        if not SIMULATE:
            self.network_handler.send(command=Command.RESTART)
        self.network_handler.close()

    def home(self):
        print("Homing robot...")
        if not SIMULATE:
            data = Hathaani.Util.to_motor_data(self.initial_positions.reshape(1, -1)).flatten()
            self.network_handler.send(command=Command.DEFAULTS, data=data)
            self.network_handler.send(command=Command.HOME)
        else:
            self.network_callback(Command.READY)

    def network_callback(self, cmd):
        if cmd == Command.READY:
            print("Robot Ready")
            self.is_initialized = True
        if cmd == Command.RESTART:
            print("Terminating")
            self.terminate()

    def update_current_values_from_robot(self):
        """
        Test and only use this function if the motor dies due to trajectory discontinuity.
        Discontinuity can happen if the actual positions don't align with the calculated positions
        """
        if not SIMULATE:
            data = self.network_handler.request(Command.CURRENT_VALUES)
            data = np.frombuffer(data, dtype=np.uint16).reshape(-1, len(MotorId))
            self.current = Hathaani.Util.from_motor_data(data)

    def perform_trajectory(self, traj: np.ndarray, motor_ids: List[MotorId], add_transition: bool = False):
        assert traj.shape[-1] == len(motor_ids), f"Invalid trajectory shape"
        for i in motor_ids:
            assert 0 <= i < len(MotorId), f"Invalid MotorId: {i}"

        data = np.zeros((len(traj), len(MotorId)))
        k = 0
        for i in MotorId:
            for j in motor_ids:
                if i == j:
                    data[:, i] = traj[:, k]
                    k += 1
                    break
                else:
                    data[:, i] = self.current[i]

        if add_transition:
            data = self.add_transitions(data, duration_ms=STRING_CHANGE_TIME_MS)

        self.current[motor_ids] = data[-1, motor_ids]
        motor_data = Hathaani.Util.to_motor_data(data)
        if not SIMULATE:
            self.network_handler.add_to_queue(motor_data)

        # for i, d in enumerate(motor_data.T):
        #     n = len(motor_data.T) * 100 + 10 + (i + 1)
        #     plt.subplot(n)
        #     plt.plot(d)
        # plt.show()

        self.terminate()

    def play_note(self, midi_note: float, amplitude: float, duration, tonic,
                  duration_in_sec=True, can_play_open_string=True):
        self.play_note_sequence(notes=[midi_note], duration=[duration], amplitude=[amplitude], tonic=tonic,
                                duration_in_sec=duration_in_sec, bow_changes=None)

    def play_note_sequence(self, notes: List[float], duration: List[float], amplitude: List[float],
                           tonic, bow_changes=None, duration_in_sec=True,
                           fix_open_string_pitches=False, snap_to_sta=False):
        assert len(notes) == len(duration)
        assert len(notes) == len(amplitude)
        notes = np.array(notes, dtype=float)
        # Rests are coded as 0. Convert them to nan
        notes[notes < 1e-2] = np.nan

        bow_changes = bow_changes or []

        N = np.zeros_like(duration, dtype=int)
        for i in range(len(duration)):
            N[i] = int(duration[i] * self.fs / self.hop) if duration_in_sec else int(duration[i])

        s = 0
        for i, d in enumerate(N):
            bow_changes.append((s + d, 0 if i < len(N) - 1 and np.isnan(notes[i + 1]) else 1))
            s += d

        total_length = np.cumsum(N)[-1]
        cents = np.zeros(total_length)
        e = np.zeros(total_length)

        nans = []
        start_idx = 0
        for i in range(len(N)):
            cents[start_idx: start_idx + N[i]] = notes[i]
            if np.isnan(notes[i]):
                nans.append((start_idx, start_idx + N[i]))
            e[start_idx: start_idx + N[i]] = amplitude[i]
            start_idx += N[i]
        cents = Util.zero_lpf(cents, 0.3, ignore_nans=True)
        e = Util.zero_lpf(e, 0.5, restore_zeros=False)

        self.perform(np.vstack([cents, e]), phrase_tonic=tonic, bow_changes=bow_changes,
                     interpolate_start=True, fix_open_string_pitches=fix_open_string_pitches, snap_to_sta=snap_to_sta,
                     amplitude_compression=0, pitch_correction_amount=0, nans=nans, remove_invalid_bow_changes=False)

    def preprocess_phrase(self, phrase: np.ndarray, phrase_tonic: float, bow_changes,
                          smoothing_alpha, min_bow_length_ms, amplitude_compression, pitch_correction_amount,
                          nans=None):
        # phrase is of shape (2, N)
        # 1st dim is pitches and 2nd dim is envelope
        pitches, envelope = phrase
        pitches[pitches < 10] = np.nan
        # Invalid pitches have 0 amplitudes
        envelope[np.isnan(pitches)] = 0
        dips = []
        pitches[envelope < 0.01] = np.nan
        envelope[envelope < 0.01] = 0

        start_idx, end_idx = Util.truncate(pitches)
        pitches = pitches[start_idx: end_idx + 1]
        envelope = envelope[start_idx: end_idx + 1]
        if nans is not None:
            nans = (np.array(nans) - start_idx).tolist()

        if not bow_changes:
            dips, window = Util.pick_dips(envelope, self.fs, self.hop, smoothing_alpha, silence_width_ms=100,
                                          wait_ms=min_bow_length_ms)
            print("raw dips", dips)
            print("window", window)

            for _s, _e in window:
                # Make sure start and end of pitch is not nan
                _s = max(0, _s)
                _e = min(len(pitches) - 1, _e)
                pitches[_s: _e] = np.nan
        else:
            if bow_changes[-1][0] != len(pitches):
                bow_changes.append((len(pitches), 1))
            if bow_changes[0][0] != 0:
                bow_changes = [(0, 1)] + bow_changes

        nans = nans or []
        pitches, nan = Util.interpolate_nan(pitches)
        for s, e in nans:
            pitches[s: e] = np.nan
        nan = nans + nan

        pitches = Util.zero_lpf(pitches, 0.15)
        if pitch_correction_amount > 0:
            pitches = (1 - pitch_correction_amount) * pitches + pitch_correction_amount * Util.pitch_correct(pitches)

        # filter nans that are less than finger press time
        nan = Util.filter_nan(nan, length=self.finger_press_time)
        nan = list(set(nan))  # remove duplicates
        print(f"nan: {nan}")
        pitches = pitches - phrase_tonic + self.TONIC
        pitches = Util.transpose_to_range(pitches, self.NOTE_RANGE.MIN)
        assert np.max(pitches) <= self.NOTE_RANGE.MAX

        # compress envelope before generating bow trajectory
        envelope = self.compress_envelope(envelope, compression_amount=amplitude_compression)
        print(f"mean amplitude: {np.mean(envelope)}")

        bounds = []
        e = 0
        if len(nan) > 0:
            nan = sorted(nan)
            bounds.append((0, nan[0][0]))
            e = nan[0][1]
        for i in range(1, len(nan)):
            bounds.append((e, nan[i][0]))
            e = nan[i][1]

        if e is not None:
            bounds.append((e, len(pitches)))

        # Split dips and bow changes into bounds
        i = 0
        temp_bc = []
        final_bc = []
        incremented = False
        for s, e in bounds:
            while i < len(bow_changes) and s <= bow_changes[i][0] <= e:
                idx, valid = bow_changes[i]
                temp_bc.append((idx - s, valid))
                i += 1
                incremented = True
            final_bc.append(temp_bc)
            temp_bc = []
            if not incremented:
                i += 1
        bow_changes = final_bc
        return pitches, envelope, bounds, nan, [dips], bow_changes

    def perform(self, phrase: np.ndarray, phrase_tonic: float, bow_changes=None, interpolate_start=False,
                min_bow_length_ms=100, smoothing_alpha=0.95, tb_cent=0.01, amplitude_compression=0.8,
                fix_open_string_pitches=True, snap_to_sta=True, pitch_correction_amount=1, nans=None,
                remove_invalid_bow_changes=True):

        # correct the bow changes list structure
        if bow_changes is not None and len(bow_changes) > 0 and not isinstance(bow_changes[0], tuple):
            for i in range(len(bow_changes)):
                bow_changes[i] = (bow_changes[i], 1)

        pitches_full, envelope_full, bounds, nan_full, dips_full, bow_changes_full = self.preprocess_phrase(phrase,
                                                                                                            phrase_tonic,
                                                                                                            bow_changes,
                                                                                                            smoothing_alpha,
                                                                                                            min_bow_length_ms,
                                                                                                            amplitude_compression,
                                                                                                            pitch_correction_amount,
                                                                                                            nans=nans)

        full_data = []
        prev_data = None

        print(f"bounds: {bounds}")
        assert len(bounds) > 0 and len(bow_changes_full) > 0, "Provide atleast 1 bound to work with!"

        for i in range(len(bounds)):
            s, e = bounds[i]
            pitches = pitches_full[s: e]
            envelope = envelope_full[s: e]
            bow_changes = bow_changes_full[i]
            dips = []
            nan = []

            data = self.process_chunk(pitches, envelope, bow_changes, dips, nan, fix_open_string_pitches, snap_to_sta,
                                      min_bow_length_ms, tb_cent, remove_invalid_bow_changes)
            if i > len(bounds) - 2:
                data[:, MotorId.LEFT_HAND] += 2
            # if len(bounds) < 2:
            #     data = self.add_transitions(data, duration_ms=STRING_CHANGE_TIME_MS * 2)
            #     full_data.append(data)

            if prev_data is not None:
                targets = data[0, :].copy()
                targets[MotorId.BOW_D_LEFT] = BOW_HEIGHT.REST
                targets[MotorId.BOW_ROTOR] = 0
                prev_data = self.reset_positions(prev_data, targets=targets, length=100)
                self.current = prev_data[-1, :]
                full_data.append(prev_data)

            data = self.add_transitions(data, duration_ms=STRING_CHANGE_TIME_MS * 2)

            if i == len(bounds) - 1:
                full_data.append(data)

            prev_data = data.copy()

        data = np.vstack(full_data)
        self.current = data[-1, :]

        motor_data = Hathaani.Util.to_motor_data(data)

        if not SIMULATE:
            self.network_handler.add_to_queue(motor_data)

        for i, d in enumerate(motor_data.T):
            n = len(motor_data.T) * 100 + 10 + (i + 1)
            plt.subplot(n)
            plt.plot(d)
        plt.show()

    def process_chunk(self, pitches, envelope, bow_changes, dips, nan, fix_open_string_pitches, snap_to_sta,
                      min_bow_length_ms, tb_cent, remove_invalid_bow_changes):
        sta = Util.pick_stationary_points(pitches, return_sta=True)
        if fix_open_string_pitches:
            pitches = Hathaani.Util.fix_open_string_pitches(pitches, sta, threshold=0.25)

        # replace changes with the closest sta
        if snap_to_sta:
            for i, c in enumerate(dips):
                closest_sta = Util.get_nearest_sta(sta, c, direction=0, min_distance=0)
                dips[i] = closest_sta

        dips = [0] + dips + [len(envelope)]  # ex: [0, 73, 141, 723]

        if bow_changes is None:
            bow_changes = self.get_bow_changes(dips, nan, min_bow_length_ms, tb_cent=tb_cent)
        bow_changes = sorted(bow_changes)

        str_changes = self.get_string_changes_by_bow_change(pitches, bow_changes, sta, min_bow_length_ms,
                                                            STRING_CHANGE_TIME_MS,
                                                            fix_tracking_error=fix_open_string_pitches)

        print(f"string changes: {str_changes}")
        print(f"bow changes: {bow_changes}")
        out = self.get_fingerboard_traversal_trajectories(pitches, str_changes, string_change_time_ms=0)
        left_hand, string_change, finger, finger_off_idx, ostr = out

        bow_rotor, bow_height, bow_rotation = self.get_bowing_trajectories(envelope, str_changes, bow_changes,
                                                                           finger_off_idx,
                                                                           envelope_smoothing=0.5,
                                                                           string_change_time_ms=0,
                                                                           remove_invalid_bow_changes=remove_invalid_bow_changes)
        data = np.zeros((len(left_hand), 7))
        data[:, MotorId.FINGER] = finger
        data[:, MotorId.STRING_CHG] = string_change
        data[:, MotorId.LEFT_HAND] = left_hand
        data[:, MotorId.BOW_D_LEFT] = bow_height
        data[:, MotorId.BOW_D_RIGHT] = bow_rotation
        data[:, MotorId.BOW_ROTOR] = bow_rotor

        return data

    @staticmethod
    def compress_envelope(envelope, compression_amount):
        assert 0 <= compression_amount < 1
        zero_idx = np.argwhere(envelope < 0.01).flatten()
        envelope = (envelope * (1 - compression_amount)) + compression_amount
        envelope[zero_idx] = 0
        return envelope

    def get_string(self, pitch: float, current_string_id=None):
        pitch = np.round(pitch, 6)
        candidates = np.zeros_like(TUNING) - 1
        for i in range(len(TUNING)):
            if 0 <= pitch - TUNING[i] < self.RANGE_PER_STRING:
                # Possible to play on ith string
                if current_string_id is not None:
                    candidates[i] = abs(current_string_id - i)
                else:
                    return i, TUNING[i]

        sid = len(TUNING)
        min_loss = len(TUNING)
        for i, c in enumerate(candidates):
            if 0 <= c < min_loss:
                sid = i
                min_loss = c

        # If the algorithm cannot find an open string,
        # remove the RANGE_PER_STRING constraint and return the nearest open string
        if sid >= len(TUNING):
            for i in range(len(TUNING)):
                if pitch - TUNING[i] >= 0:
                    return i, TUNING[i]

        return sid, TUNING[sid]

    def get_string_changes(self, pitches, sta, min_length_suggestion_ms, offset=0, max_pitch=None, min_pitch=None):
        sta = sta or Util.pick_stationary_points(pitches, return_sta=True)

        min_length_suggestion = int(round(min_length_suggestion_ms / (self.time_per_sample * 1000)))
        changes: List[Tuple[int, int, int, float]] = []
        sid, os_note = self.get_string(pitches[0])

        # check if it can play in 1 string.
        # String id is guaranteed to be not None since we transpose pitches to range.
        min_pitch, max_pitch = min_pitch or np.min(pitches), max_pitch or np.max(pitches)
        string_id, open_string_note = self.get_string(min_pitch)

        if np.floor(max_pitch - open_string_note) < self.RANGE_PER_STRING:
            print("Can be played in 1 string!")
            changes.append((offset, len(pitches) + offset, string_id, max_pitch))
            return changes

        l = 0
        length = 1
        for r in sta:
            if l == r:
                continue

            min_pitch = np.min(pitches[l: r])
            max_pitch = np.max(pitches[l: r])
            # if increasing trend
            if pitches[r] - pitches[l] > 0:
                if max_pitch - os_note >= self.RANGE_PER_STRING:
                    sid, os_note = self.get_string(min_pitch, sid)
                # if the pitch is over 7 + 3 semitones,
                elif max_pitch - os_note > 10 and length > min_length_suggestion:
                    sid, os_note = self.get_string(min_pitch, sid)
            else:
                if min_pitch - os_note < 0:
                    sid, os_note = self.get_string(min_pitch, sid)
                elif min_pitch - os_note < 4 and length > min_length_suggestion:
                    sid, os_note = self.get_string(min_pitch, sid)

            if r == len(pitches) - 1:
                r = len(pitches)

            if len(changes) > 0 and changes[-1][2] == sid:
                changes[-1] = (changes[-1][0], r + offset, sid, max(changes[-1][-1], max_pitch))
            else:
                changes.append((l + offset, r + offset, sid, max_pitch))

            l = r

        changes = Hathaani.Util.filter_string_changes(changes, min_length_suggestion, self.RANGE_PER_STRING)
        return changes

    def get_string_changes_by_bow_change(self,
                                         pitches: np.ndarray,
                                         bow_changes: List[Tuple[int, int]],
                                         sta: Optional[list] = None,
                                         min_length_suggestion_ms: int = 100,
                                         string_change_time_ms: int = STRING_CHANGE_TIME_MS,
                                         fix_tracking_error: bool = False) -> Optional[
        List[Tuple[int, int, int, float]]]:
        string_change_period: int = int(round(string_change_time_ms / (self.time_per_sample * 1000)))
        sta = sta if sta is not None else Util.pick_stationary_points(pitches, return_sta=True)

        def add_to_changes(changes_to_add: List[Tuple[int, int, int, float]],
                           list_of_changes: List[Tuple[int, int, int, float]]):
            for (_l, _r, _sid, _mp) in changes_to_add:
                # move r such that current_l - prev_r > string change time
                if _l > 0:
                    tmp = Util.get_nearest_sta(sta, _l, 1, string_change_period // 2)
                    _l = _l + string_change_period // 2 if abs(tmp - _l) > string_change_period else tmp
                if _r < len(pitches):
                    tmp = Util.get_nearest_sta(sta, _r, -1, string_change_period // 2)
                    _r = _r - string_change_period // 2 if abs(tmp - _r) > string_change_period else tmp
                if len(list_of_changes) > 0 and list_of_changes[-1][2] == _sid:
                    list_of_changes[-1] = (list_of_changes[-1][0], _r, _sid, max(list_of_changes[-1][-1], _mp))
                else:
                    list_of_changes.append((_l, _r, _sid, _mp))

        # If it cannot be played in 1 string
        # Split the phrase into chunks using open tuning ranges
        # For each chunk, assign a string
        # Return changes

        min_length_suggestion = int(round(min_length_suggestion_ms / (self.time_per_sample * 1000)))
        changes: List[Tuple[int, int, int, float]] = []

        sid, os_note = self.get_string(pitches[0])

        # check if it can play in 1 string.
        # String id is guaranteed to be not none since we transpose pitches to range.
        pitch_low, pitch_high = np.min(pitches).round(6), np.max(pitches).round(6)
        string_id, open_string_note = self.get_string(pitch_low)
        # print(pitch_high, pitch_low, open_string_note, string_id)
        if pitch_high - open_string_note < self.RANGE_PER_STRING:
            print("In string changes by bow change: Can be played in 1 string!")
            changes.append((0, len(pitches), string_id, pitch_high))
            return changes

        l = 0
        for r, valid in bow_changes:
            if l == r:
                continue

            if r == len(pitches) - 1:
                r = len(pitches)

            min_pitch = np.min(pitches[l: r])
            max_pitch = np.max(pitches[l: r])

            # if min pitch is slightly off, it is probably tracking error. Fix it here
            if -1 < min_pitch - os_note < 0 and fix_tracking_error:
                print("os tracking error fix:", min_pitch, max_pitch, os_note, min_pitch - os_note, max_pitch - os_note)
                min_pitch = os_note
                idx = np.argwhere(pitches[l: r] < min_pitch).flatten() + l
                pitches[idx] = min_pitch

            # print(l, r, min_pitch, max_pitch, os_note, max_pitch - os_note, self.RANGE_PER_STRING, sid)
            if np.floor(max_pitch - os_note) > self.RANGE_PER_STRING or min_pitch < os_note:
                sid, os_note = self.get_string(min_pitch, sid)

            temp = [(l, r, sid, max_pitch)]
            # If sub-phrase is still > RANGE_PER_STRING, revert back to v1
            if np.floor(max_pitch - os_note) > self.RANGE_PER_STRING or min_pitch < os_note:
                print(f"Range for {os_note} is {max_pitch - os_note}. Reverting to string change computation using STA")
                temp = self.get_string_changes(pitches[l: r], sta=None,
                                               min_length_suggestion_ms=min_length_suggestion_ms,
                                               offset=l, max_pitch=max_pitch, min_pitch=min_pitch)
            add_to_changes(temp, changes)
            l = r

        changes = Hathaani.Util.filter_string_changes(changes, min_length_suggestion, self.RANGE_PER_STRING)
        return changes

    def get_fingerboard_traversal_trajectories(self, pitches, segments, string_change_time_ms=0):
        string_change_period: int = int(round(string_change_time_ms / (self.time_per_sample * 1000)))
        extra_length = max(0, (len(segments) - 1) * string_change_period)
        ostr = np.zeros((len(TUNING), len(pitches) + extra_length))
        left_hand = np.zeros(len(pitches) + extra_length)
        string_change = np.zeros(len(pitches) + extra_length)
        finger = np.zeros(len(pitches) + extra_length) + FINGER_HEIGHT.OFF

        finger_off_idx = []
        prev_sid = None
        prev_sc = None
        prev_r_idx = None

        o = 0  # string change time offset
        for i, (l, r, sid, _) in enumerate(segments):
            p = pitches[l: r] - TUNING[sid]
            # Don't have to go below the 0.5th fret
            p[p < 0.5] = 0.5
            lh = Hathaani.Util.to_left_hand_position_mm(p)
            l, r = l + o, r + o
            left_hand[l: r] = lh

            fpt_offset = 1 + self.finger_press_time // 2
            string_change_position_id = Hathaani.Util.string_id_to_mm(sid, prev_sid)
            if prev_sc is not None:
                p = STR_CHG_POS[string_change_position_id].MIN
                string_change[l: l + fpt_offset] = Util.interp(prev_sc, p, fpt_offset, 0.2)
            else:
                string_change[l: l + fpt_offset] = STR_CHG_POS[string_change_position_id].MIN

            string_change[l + fpt_offset: r] = STR_CHG_POS[string_change_position_id].MIN

            finger[l + fpt_offset: r - fpt_offset] = FINGER_HEIGHT.ON

            # interpolate traj during string change
            if prev_r_idx and (l - prev_r_idx) > 1:  # skips first iteration
                N = l - prev_r_idx
                left_hand[prev_r_idx: l] = Util.interp(left_hand[prev_r_idx], left_hand[l], N, 0.2)
                string_change[prev_r_idx: l] = Util.interp(string_change[prev_r_idx], string_change[l], N, 0.2)
                finger[prev_r_idx: l] = Util.interp(finger[prev_r_idx], finger[l], N, 0.2)

            # If left hand is very close to 0, finger off
            th = Hathaani.Util.to_left_hand_position_mm(0.75)
            tmp_idx = np.argwhere(lh < th).flatten() + l
            finger_off_idx.extend(list(tmp_idx))
            finger[tmp_idx] = FINGER_HEIGHT.OFF
            ostr[sid, l: r] = left_hand[l: r]
            prev_sid = sid
            prev_sc = string_change[r - 1]

            # update string change time offset
            prev_r_idx = r - 1
            o += string_change_period

        p = max(0.25, pitches[-1] - TUNING[prev_sid])
        left_hand[-1] = Hathaani.Util.to_left_hand_position_mm(p)

        ostr[prev_sid, -1] = left_hand[-1]
        left_hand = Util.zero_lpf(left_hand, 0.1, restore_zeros=False)
        string_change = Util.zero_lpf(string_change, 0.3, restore_zeros=False)
        finger = Util.zero_lpf(finger, 0.3, restore_zeros=False)

        finger_off_idx = np.array(finger_off_idx)
        return left_hand, string_change, finger, finger_off_idx, ostr

    def get_bow_changes(self, dips, nan, min_bow_length_ms=100, tb_cent=0.01) -> List[Tuple[int, int]]:
        min_bow_length = int(round(min_bow_length_ms / (self.time_per_sample * 1000)))

        dips = Hathaani.Util.filter_bow_changes(dips, tb_cent, min_bow_length=min_bow_length)

        # For all changes, if the idx is in-between nans, remove that index
        temp = set()
        for c in dips:
            should_add = True
            for n in nan:
                if n[0] - min_bow_length <= c <= n[1] + min_bow_length:
                    should_add = False
                    break
            if should_add or len(nan) == 0:
                temp.add((c, 1))

        dips = temp.copy()

        # Add nan boundaries to changes
        for n in nan:
            dips.add((n[0], 0))
            dips.add((n[1], 1))

        return sorted(list(dips))

    def get_bowing_trajectories(self, envelope: np.ndarray, segments, bow_changes, finger_off_idx,
                                envelope_smoothing=0.9, string_change_time_ms=0, remove_invalid_bow_changes=False):
        string_change_period: int = int(round(string_change_time_ms / (self.time_per_sample * 1000)))

        extra_length = max(0, (len(segments) - 1) * string_change_period)
        env = np.zeros(len(envelope) + extra_length)
        o = 0  # string change time offset
        for s, e, sid, _ in segments:
            env[s + o: e + o] = envelope[s: e]
            o += string_change_period

        env = Util.zero_lpf(env, envelope_smoothing, restore_zeros=False)

        # remove invalid bow changes
        if remove_invalid_bow_changes:
            bc = []
            for c, valid in bow_changes:
                if valid:
                    bc.append((c, 1))
            bow_changes = bc

        # for each segment change bow_changes at segment end to invalid,
        # Add a bow change after string_change_time_ms and mark it valid
        if string_change_period > 0:
            i = 0
            temp = set()
            o = 0
            for s, e, _, _ in segments:
                while s <= bow_changes[i][0] < e and i < len(bow_changes):
                    temp.add((bow_changes[i][0] + o, bow_changes[i][1]))
                    i += 1
                o += string_change_period
            if i < len(bow_changes):
                temp.add((bow_changes[i][0] + o - string_change_period, bow_changes[i][1]))

            o = 0
            for i, (s, e, _, _) in enumerate(segments):
                if i < len(segments) - 1:
                    temp.add((e + o, 0))
                    o += string_change_period
                temp.add((e + o, 1))

            bow_changes = sorted(list(temp))

        print(f"bow changes (with transition time): {(np.array(bow_changes) + 30).tolist()}")
        # We don't have to pass segments and sc_time since we add these info in bow_changes above
        bow_rotor, velocity = self.generate_bow_rotor_trajectory(bow_changes, env,
                                                                 min_velocity=BOW_MIN_VELOCITY,
                                                                 bow_min=BOW_ROTOR.MIN, bow_max=BOW_ROTOR.MAX)

        # use velocity to change bow height according to the speed of bowing.
        # Slower bowing, we need to play with less pressure for the same amplitude value
        bow_height = self.generate_bow_height_trajectory(env * np.abs(velocity), segments, finger_off_idx,
                                                         string_change_period)
        bow_rotation = self.generate_bow_rotation_trajectory(segments, string_change_period=string_change_period)

        return bow_rotor, bow_height, bow_rotation

    def generate_bow_height_trajectory(self, envelope: np.ndarray, segments: list, finger_off_idx,
                                       string_change_period: int):
        # When the phrase ends, move bow to rest before finishing
        # mask = np.ones_like(envelope)
        # mask[-self.string_change_time:] = np.linspace(1, 0, self.string_change_time)

        bow_height = Hathaani.Util.to_bow_height_mm(envelope, segments, string_change_period)
        if len(finger_off_idx) > 0:
            bow_height[finger_off_idx] += BOW_HEIGHT_CHANGE_FOR_OPEN_STRING_NOTE

        bow_height[-self.finger_press_time:] = Util.interp(bow_height[-self.finger_press_time],
                                                           BOW_HEIGHT.REST,
                                                           self.finger_press_time)

        return Util.zero_lpf(bow_height, 0.5)

    def generate_bow_rotor_trajectory(self,
                                      bow_changes: list,
                                      envelope: np.ndarray,
                                      min_velocity=0.25,  # % of bow_length/sec
                                      bow_min=0.0, bow_max=1.0) -> Tuple[np.ndarray, np.ndarray]:
        # 0 speed at 0 amplitude
        # 0.2 speed at 0.25 amplitude
        # 0.5 speed at 1 amplitude
        # y = ax2 + bx + c
        # y = -0.4x2 + 0.9x

        min_v = -0.4 * envelope ** 2 + 0.9 * envelope
        min_v = np.maximum(min_velocity, min_v)

        length = (bow_max - bow_min)
        bow = np.zeros_like(envelope)
        t = self.hop / self.fs
        bow[0] = self.current[MotorId.BOW_ROTOR]
        self.bow_direction = int(bow_max - bow[0] < bow[0] - bow_min)

        # Since for loop starts at 1, if bow changes has index 0, discard that
        if len(bow_changes) > 0 and bow_changes[0][0] == 0:
            bow_changes = bow_changes[1:]

        def can_change_direction(last_point):
            return True
            # return not ((self.bow_direction == 0 and last_point < 0.25 * length) or
            #             (self.bow_direction == 1 and bow[i - 1] > 0.75 * length))

        extra_changes = []
        valid = True
        j = 0
        for i in range(1, len(bow)):
            if j < len(bow_changes) and i == bow_changes[j][0]:
                valid = bow_changes[j][1]
                if valid and can_change_direction(bow[i - 1]):
                    self.bow_direction ^= 1
                j += 1

            increment = min_v[i] * length * t if valid else 0

            d = 1 if self.bow_direction == 0 else -1
            if not (bow_min < bow[i - 1] + (increment * d) < bow_max):
                self.bow_direction ^= 1
                d = 1 if self.bow_direction == 0 else -1
                extra_changes.append(i)

            bow[i] = bow[i - 1] + (increment * d)

        print(f"extra changes: {extra_changes}")
        bow = Util.zero_lpf(bow, 0.5, restore_zeros=False)
        v = np.diff(bow, prepend=bow[0]) / (min_v * t * length)
        return bow, np.maximum(-1, np.minimum(1, v))

    # Trajectory for bow angle
    @staticmethod
    def generate_bow_rotation_trajectory(segments, tb_cent=0.45, string_change_period: int = 0):
        extra_length = max(0, (len(segments) - 1) * string_change_period)
        bow_rot_traj = np.zeros(segments[-1][1] + extra_length)
        o = 0  # string change time offset
        for s, e, sid, _ in segments:
            s, e = s + o, e + o
            bow_rot_traj[s: e] = BOW_ANGLE[sid]
            o += string_change_period

        o = 0
        for i in range(1, len(segments)):
            _, e0, _, _ = segments[i - 1]
            s, _, _, _ = segments[i]
            e0 = e0 + o
            o += string_change_period
            s = s + o
            bow_rot_traj[e0: s] = Util.interp(bow_rot_traj[e0 - 1], bow_rot_traj[s], s - e0, tb_percentage=tb_cent)

        return bow_rot_traj

    def add_transitions(self, data: np.ndarray, duration_ms: int = STRING_CHANGE_TIME_MS, tb_cent: float = 0.2):
        """
        Append transition trajectories from current values to the corresponding first traj points
        :param data: (N, 7) shaped array having N trajectory points
        :param duration_ms: Interpolation length
        :param tb_cent: Blend percentage
        :return: data with transitions added. shape = (N + length, 7)
        """
        length = int(round(duration_ms / (self.time_per_sample * 1000)))
        size = data.shape
        out = np.zeros((size[0] + length, size[1]), dtype=data.dtype)
        for i in range(size[1]):
            traj = Util.interp(self.current[i], data[0, i], length, tb_cent, dtype=data.dtype)
            out[:length, i] = traj
            out[length:, i] = data[:, i]

        return out

    @staticmethod
    def reset_positions(traj, targets, length: int, tb_cent: float = 0.2):
        size = traj.shape
        out = np.zeros((size[0] + length, size[1]), dtype=traj.dtype)
        for i in range(size[1]):
            t = Util.interp(traj[-1, i], targets[i], length, tb_cent, dtype=traj.dtype)
            out[:size[0], i] = traj[:, i]
            out[size[0]:, i] = t
        return out
