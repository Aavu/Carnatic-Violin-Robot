import librosa
import numpy as np
from matplotlib import pyplot as plt
from utils import Util
from definitions import *
from udpHandler import UDPHandler
from typing import Tuple, List


class Hathaani:
    # Total range of notes that can be played on a violin with tuning D A D A (on E A D G)
    NOTE_RANGE = Range(57, 94)
    # Maximum semitones a string can support
    RANGE_PER_STRING = 12
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

            if string_change_period > 0:
                o = 0
                for (s, e, sid, _) in segments:
                    s, e = s + o, e + o
                    if 0 < e < len(out) - string_change_period:
                        out[e: e + string_change_period] = Util.interp(out[e - 1], out[e + string_change_period],
                                                                       string_change_period, 0.45)
                    o += string_change_period
            out[out < 0] = 0
            return Util.zero_lpf(out, 0.7, restore_zeros=False)

        @staticmethod
        def to_left_hand_position_mm(pitches: np.ndarray or float) -> np.ndarray:
            return SCALE_LENGTH * (1 - (2 ** (-pitches / 12))) + NUT_POSITION_MM

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
        def fix_open_string_pitches(cents, sta, cpn_threshold):
            cpn = Util.find_cpn(cents, sta, cp_threshold=cpn_threshold)
            for (n, s, l) in cpn:
                cents[s: s + l] = n

            cents = Util.zero_lpf(cents, 0.3)
            # f, (ax1) = plt.subplots(1, 1, sharex='all', figsize=(15, 8))
            # ax1.plot(cents)
            # labelled = False
            # for note, s, l in cpn:
            #     x = np.arange(s, s + l)
            #     y = np.ones_like(x) * note
            #     if not labelled:
            #         ax1.plot(x, y, 'y--', label="CP Notes")
            #         labelled = True
            #     else:
            #         ax1.plot(x, y, 'y--')
            #
            # plt.show()
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
        def adjust_bow_height_for_center_deviations(height: np.ndarray, angles: np.ndarray, string_changes):
            # Equations for differential is wrt center point. But Strings are away from the center.
            # Correct for the string center deviations
            new_height = height.copy()

            for l, r, sid, _ in string_changes:
                d = STR_DIST_BRIDGE[sid]
                new_height[l: r] += (np.abs(d * np.tan(BOW_ANGLE[sid]))) - BRIDGE_CURVATURE[sid]

            return new_height

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
            # TODO: Bow height for center deviations

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
                x[:, MotorId.BOW_D_LEFT],
                x[:, MotorId.BOW_D_RIGHT],
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
        self.bow_direction = 0

        self.network_handler = None
        if not SIMULATE:
            self.network_handler = UDPHandler(DEVICE_IP, DEVICE_PORT, cmd_callback=self.network_callback)
            self.network_handler.start()

        # Fill this array from the robot after it finishes playing the phrase
        # Node-id order, units in mm. Bow angle in radians and Finger in thousands of torque
        self.initial_positions: np.ndarray = np.array([FINGER_HEIGHT.REST,  # FINGER Torque
                                                       STR_CHG_POS.AD.MIN,  # STRING_CHG
                                                       0,  # LEFT_HAND
                                                       BOW_HEIGHT.REST,  # Bow position i.e. BOW_D_LEFT
                                                       BOW_ANGLE.REST,  # Bow angle i.e. BOW_D_RIGHT
                                                       0,  # BOW_SLIDE
                                                       BOW_ROTOR.REST])  # BOW_ROTOR
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

        bow_changes = bow_changes or []

        N = np.zeros_like(duration, dtype=int)
        for i in range(len(duration)):
            N[i] = int(duration[i] * self.fs / self.hop) if duration_in_sec else int(duration[i])

        s = 0
        for i in N:
            bow_changes.append((s + i, 1))
            s += i

        total_length = np.cumsum(N)[-1]
        cents = np.zeros(total_length)
        e = np.zeros(total_length)

        start_idx = 0
        for i in range(len(N)):
            cents[start_idx: start_idx + N[i]] = notes[i]
            e[start_idx: start_idx + N[i]] = amplitude[i]
            start_idx += N[i]
            e[start_idx - 5: start_idx] = 0
        cents = Util.zero_lpf(cents, 0.3)
        e = Util.zero_lpf(e, 0.5, restore_zeros=False)
        self.perform(np.vstack([cents, e]), phrase_tonic=tonic, bow_changes=bow_changes,
                     interpolate_start=True, fix_open_string_pitches=fix_open_string_pitches, snap_to_sta=snap_to_sta)

    def perform(self, phrase: np.ndarray, phrase_tonic: float, bow_changes=None, interpolate_start=False,
                min_bow_length_ms=100, smoothing_alpha=0.95, tb_cent=0.01,
                fix_open_string_pitches=True, snap_to_sta=True):
        # phrase is of shape (2, N)
        # 1st dim is pitches and 2nd dim is envelope
        pitches, envelope = phrase
        pitches = Util.transpose_to_range(pitches, self.NOTE_RANGE.MIN)
        pitches = Util.zero_lpf(pitches, 0.15)
        pitches[pitches < 1] = np.nan
        pitches, nan = Util.interpolate_nan(pitches)
        # filter nans that are less than finger press time
        nan = Util.filter_nan(nan, length=self.finger_press_time)
        pitches = pitches - phrase_tonic + self.TONIC

        sta = Util.pick_stationary_points(pitches, return_sta=True)
        if fix_open_string_pitches:
            pitches = Hathaani.Util.fix_open_string_pitches(pitches, sta, cpn_threshold=0.25)

        bow_changes = bow_changes or self.get_bow_changes(envelope, nan, sta, min_bow_length_ms, smoothing_alpha,
                                                          tb_cent=tb_cent, snap_to_sta=snap_to_sta)
        str_changes = self.get_string_changes_by_bow_change(pitches, bow_changes, min_bow_length_ms)

        print(str_changes, bow_changes)
        out = self.get_fingerboard_traversal_trajectories(pitches, str_changes, STRING_CHANGE_TIME_MS)
        left_hand, string_change, finger, finger_off_idx, ostr = out

        # compress envelope before generating bow trajectory
        envelope = self.compress_envelope(envelope, compression_amount=0.75)
        print(f"mean amplitude: {np.mean(envelope)}")
        bow_rotor, bow_height, bow_rotation = self.get_bowing_trajectories(envelope, str_changes, bow_changes,
                                                                           finger_off_idx,
                                                                           envelope_smoothing=0.5,
                                                                           string_change_time_ms=STRING_CHANGE_TIME_MS)
        data = np.zeros((len(left_hand), 7))
        data[:, MotorId.LEFT_HAND] = left_hand
        data[:, MotorId.STRING_CHG] = string_change
        data[:, MotorId.FINGER] = finger
        data[:, MotorId.BOW_D_LEFT] = bow_height
        data[:, MotorId.BOW_D_RIGHT] = bow_rotation
        data[:, MotorId.BOW_ROTOR] = bow_rotor

        if interpolate_start:
            data = self.add_transitions(data, duration_ms=STRING_CHANGE_TIME_MS * 2)

        self.current = data[-1, :]

        motor_data = Hathaani.Util.to_motor_data(data)
        # print(motor_data)

        if not SIMULATE:
            self.network_handler.add_to_queue(motor_data)

        # n = (len(ostr) + 1) * 100 + 11
        # plt.subplot(n)
        # plt.plot(pitches)
        # for i, p in enumerate(ostr):
        #     n = (len(ostr) + 1) * 100 + 10 + (i + 2)
        #     plt.subplot(n)
        #     plt.plot(p)
        #
        # plt.show()

        for i, d in enumerate(motor_data.T):
            n = len(motor_data.T) * 100 + 10 + (i + 1)
            plt.subplot(n)
            plt.plot(d)
        plt.show()

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

        if sid >= len(TUNING):
            return None, None
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
                                         min_length_suggestion_ms: int) -> List[Tuple[int, int, int, float]] or None:

        def add_to_changes(changes_to_add: List[Tuple[int, int, int, float]],
                           list_of_changes: List[Tuple[int, int, int, float]]):
            for (_l, _r, _sid, _mp) in changes_to_add:
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

        if pitch_high - open_string_note < self.RANGE_PER_STRING:
            print("Can be played in 1 string!")
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
            if -1 < min_pitch - os_note < 0:
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

    def get_fingerboard_traversal_trajectories(self, pitches, segments, string_change_time_ms):
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
            th = Hathaani.Util.to_left_hand_position_mm(0.95)
            tmp_idx = np.argwhere(lh < th).flatten() + l
            finger_off_idx.extend(list(tmp_idx))
            finger[tmp_idx] = FINGER_HEIGHT.OFF
            ostr[sid, l: r] = left_hand[l: r]
            prev_sid = sid
            prev_sc = string_change[r - 1]

            # update string change time offset
            prev_r_idx = r - 1
            o += string_change_period

        p = max(0.5, pitches[-1] - TUNING[prev_sid])
        left_hand[-1] = Hathaani.Util.to_left_hand_position_mm(p)

        ostr[prev_sid, -1] = left_hand[-1]
        left_hand = Util.zero_lpf(left_hand, 0.1, restore_zeros=False)
        string_change = Util.zero_lpf(string_change, 0.3, restore_zeros=False)
        finger = Util.zero_lpf(finger, 0.3, restore_zeros=False)

        finger_off_idx = np.array(finger_off_idx)
        return left_hand, string_change, finger, finger_off_idx, ostr

    def get_bow_changes(self, envelope: np.ndarray, nan, sta,
                        min_bow_length_ms=100, smoothing_alpha=0.9, tb_cent=0.01,
                        snap_to_sta=True) -> List[Tuple[int, int]]:
        min_bow_length = int(round(min_bow_length_ms / (self.time_per_sample * 1000)))
        changes = Util.pick_dips(envelope, self.fs, self.hop, smoothing_alpha, wait_ms=min_bow_length_ms)
        print("raw", changes)

        # replace changes with the closest sta
        if snap_to_sta:
            for i, c in enumerate(changes):
                closest_sta = Util.get_nearest_sta(sta, c, direction=0, min_distance=0)
                changes[i] = closest_sta

        changes = [0] + changes + [len(envelope)]  # ex: [0, 73, 141, 723]

        changes = Hathaani.Util.filter_bow_changes(changes, tb_cent, min_bow_length=min_bow_length)

        # For all changes, if the idx is in-between nans, remove that index
        temp = set()
        for c in changes:
            should_add = True
            for n in nan:
                if n[0] - min_bow_length <= c <= n[1] + min_bow_length:
                    should_add = False
                    break
            if should_add or len(nan) == 0:
                temp.add((c, 1))

        changes = temp.copy()

        # Add nan boundaries to changes
        for n in nan:
            changes.add((n[0], 0))
            changes.add((n[1], 1))

        return sorted(list(changes))

    def get_bowing_trajectories(self, envelope: np.ndarray, segments, bow_changes, finger_off_idx,
                                envelope_smoothing=0.9, tb_cent=0.01, string_change_time_ms=STRING_CHANGE_TIME_MS):
        string_change_period: int = int(round(string_change_time_ms / (self.time_per_sample * 1000)))

        extra_length = max(0, (len(segments) - 1) * string_change_period)
        env = np.zeros(len(envelope) + extra_length)
        o = 0  # string change time offset
        for s, e, sid, _ in segments:
            env[s + o: e + o] = envelope[s: e]
            o += string_change_period

        env = Util.zero_lpf(env, envelope_smoothing, restore_zeros=False)

        # remove invalid bow changes
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

        print(f"bow changes: {bow_changes}")
        # We don't have to pass segments and sc_time since we add these info in bow_changes above
        bow_rotor, velocity = self.generate_bow_rotor_trajectory(bow_changes, env,
                                                                 min_velocity=BOW_MIN_VELOCITY,
                                                                 bow_min=BOW_ROTOR.MIN, bow_max=BOW_ROTOR.MAX)

        bow_height = self.generate_bow_height_trajectory(env * np.abs(velocity), segments, finger_off_idx,
                                                         string_change_period)
        bow_rotation = self.generate_bow_rotation_trajectory(segments, string_change_period=string_change_period)

        return bow_rotor, bow_height, bow_rotation

    @staticmethod
    def generate_bow_height_trajectory(envelope: np.ndarray, segments: list, finger_off_idx,
                                       string_change_period: int):
        bow_height = Hathaani.Util.to_bow_height_mm(envelope, segments, string_change_period)
        if len(finger_off_idx) > 0:
            bow_height[finger_off_idx] += BOW_HEIGHT_CHANGE_FOR_OPEN_STRING_NOTE

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
            return not ((self.bow_direction == 0 and last_point < 0.25 * length) or
                        (self.bow_direction == 1 and bow[i - 1] > 0.75 * length))

        valid = True
        j = 0
        for i in range(1, len(bow)):
            if i == bow_changes[j][0]:
                valid = bow_changes[j][1]
                if valid and can_change_direction(bow[i - 1]):
                    self.bow_direction ^= 1
                j += 1

            increment = min_v[i] * length * t if valid else 0

            d = 1 if self.bow_direction == 0 else -1
            if not (bow_min < bow[i - 1] + (increment * d) < bow_max):
                self.bow_direction ^= 1
                d = 1 if self.bow_direction == 0 else -1

            bow[i] = bow[i - 1] + (increment * d)

        bow = Util.zero_lpf(bow, 0.75, restore_zeros=False)
        v = np.diff(bow, prepend=bow[0]) / (min_v * t * length)
        return bow, np.maximum(-1, np.minimum(1, v))

    def generate_bow_rotor_trajectory1(self,
                                       bow_changes: list,
                                       envelope: np.ndarray,
                                       envelope_weightage: float,
                                       min_velocity=0.25,  # % of bow_length/sec
                                       velocity=1.0,  # % of bow_length/sec
                                       bow_min=0.0, bow_max=1.0,
                                       tb_cent=0.01) -> np.ndarray:
        def window(M, fade_length=10):
            fade_length = min(fade_length, M // 2)
            w = np.ones(M)
            w[:fade_length] = np.linspace(0, 1, fade_length)
            w[-fade_length:] = np.linspace(1, 0, fade_length)
            return w

        envelope *= window(len(envelope))
        diff = Util.zero_lpf(np.abs(np.diff(envelope, prepend=envelope[0])), 0.9, restore_zeros=False)
        max_diff = np.max(diff)
        min_diff = np.min(diff)
        if max_diff > 1e-6:
            diff = (diff - min_diff) / (max_diff - min_diff)
        diff = (diff * envelope_weightage)

        original_bow_max = bow_max
        bow_max -= np.max(diff)

        bow = np.zeros(bow_changes[-1][0])
        p0 = self.current[MotorId.BOW_ROTOR]

        def get_pf(length, initial_value, vel):
            seg_len = length * self.hop / self.fs
            length = vel * (bow_max - bow_min) * seg_len
            # TODO: Include cumsum of envelope diff while calculating pf
            final_value = min(bow_max, initial_value + length) \
                if self.bow_direction == 0 else max(bow_min, initial_value - length)
            length = abs(final_value - initial_value) / (bow_max - bow_min)
            return final_value, np.round(length / seg_len, 3)

        for i in range(len(bow_changes) - 1):
            s, valid = bow_changes[i]
            e, valid_next = bow_changes[i + 1]

            # If it is an invalid region, don't move the bow
            if not valid:
                bow[s: e] = p0
                self.bow_direction ^= 1
                continue

            pf, velocity = get_pf(e - s, p0, velocity)

            # If velocity is too slow, divide the segment such that each segment has min velocity
            if 0 < velocity < min_velocity:
                div = int(np.ceil(min_velocity / velocity))
                print(f"Velocity for segment ({s}, {e}) : ({velocity:.4f}) < minimum ({min_velocity:.4f}). "
                      f"Dividing segment into {div}")

                l = (e - s) // div

                for d in range(1, div):
                    _e = s + l
                    bow[s: _e] = Util.interp(p0, pf, _e - s, tb_cent)
                    self.bow_direction ^= 1
                    p0 = pf
                    pf, velocity = get_pf(e - s, p0, min_velocity)
                    s = _e

            bow[s: e] = Util.interp(p0, pf, e - s, tb_cent)
            p0 = pf
            if valid_next:
                self.bow_direction ^= 1

        # bow += envelope_weightage
        assert np.max(bow) <= original_bow_max, f"bow max is {np.max(bow)}"
        return bow

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

        if string_change_period > 0:
            o = 0
            for s, e, sid, _ in segments:
                s, e = s + o, e + o
                r = min(string_change_period, e - s)
                if e + r < len(bow_rot_traj):
                    bow_rot_traj[e: e + r] = Util.interp(bow_rot_traj[e - 1], bow_rot_traj[e + r], r,
                                                         tb_percentage=tb_cent)
                o += string_change_period
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
        for i in range(7):
            traj = Util.interp(self.current[i], data[0, i], length, tb_cent, dtype=data.dtype)
            out[:length, i] = traj
            out[length:, i] = data[:, i]

        return out
