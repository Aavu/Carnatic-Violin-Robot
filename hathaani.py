import dataclasses

import librosa
import numpy as np
from matplotlib import pyplot as plt
from utils import Util
from definitions import *
from pitchTrack import PitchTrack
from udpHandler import UDPHandler
from typing import Tuple, List


class Hathaani:
    # Total range of notes that can be played on a violin with tuning D A D A (on E A D G)
    NOTE_RANGE = [57, 94]
    TUNING = [74, 69, 62, 57]  # E A D G strings
    # Maximum semitones a string can support
    RANGE_PER_STRING = 12
    TONIC = librosa.note_to_midi('D3')

    NUM_ACTUATORS = 7
    """
    Number of actuators in the robot. \n
    If this value is changed, make sure to update helper methods - to/from motor data accordingly
    """

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
        def to_bow_height_mm(amplitudes: np.ndarray, changes: List[Tuple[int, int, int, float]]):
            """
            Convert amplitude envelope trajectory [0, 1) to bow height max and min.
            :param amplitudes: The amplitude envelope of the phrase.
            :param changes: String changes
            :return: Bow height (mm) trajectory
            """
            out = amplitudes.copy()
            for (s, e, sid, _) in changes:
                out[s: e] = (out[s: e] * (BOW_HEIGHT[sid].MAX - BOW_HEIGHT[sid].MIN)) + BOW_HEIGHT[sid].MIN
            out[out < 0] = 0
            return out

        @staticmethod
        def to_left_hand_position_mm(pitches: np.ndarray) -> np.ndarray:
            return SCALE_LENGTH * (1 - (2 ** (-pitches / 12)))

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
            #  [scp.EA, scp.EA, scp.EA, scp.AD, scp.AD],
            #  [scp.AD, scp.AD, scp.AD, scp.AD, scp.DG],
            #  [scp.DG, scp.DG, scp.DG, scp.DG, scp.DG]]

            sid_map = np.array([[0, 0, 0, 0, 0],
                                [0, 0, 0, 1, 1],
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

            x = BOW_CENTER_RAIL_DISTANCE * np.tan(angles)
            phi_x = x
            phi_d = positions

            # Same sign for phi because of the nature of the design, rotations are inverted
            left = phi_d - phi_x
            right = phi_d + phi_x

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

        """Adjustments for deviations"""

        @staticmethod
        def adjust_finger_height(finger: np.ndarray or float, left_hand: np.ndarray or float):
            assert len(finger) == len(left_hand)
            cent = np.minimum(1.0, left_hand * 1.0 / FINGERBOARD_LENGTH)
            dev = 1 - ((finger - FINGER_HEIGHT.OFF) / (FINGER_HEIGHT.ON - FINGER_HEIGHT.OFF))
            finger -= dev * cent * STRING_HEIGHT.END
            # return Util.zero_lpf(np.maximum(finger, 0), 0.25)
            return np.maximum(finger, 0)

        @staticmethod
        def adjust_string_change_deviation(str_chg: np.ndarray, left_hand: np.ndarray):
            assert len(str_chg) == len(left_hand)
            nut_dev = max(STR_CHG_POS.EA.NUT, STR_CHG_POS.DG.NUT)
            bridge_dev = max(STR_CHG_POS.EA.BRIDGE, STR_CHG_POS.DG.BRIDGE)
            diff = str_chg - STR_CHG_POS.AD.NUT
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
            # TODO: Implement bow slide conversion
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
        time_per_sample = hop_size / sample_rate
        self.finger_press_time = int(round(FINGER_PRESS_TIME_MS / (time_per_sample * 1000)))
        self.pitch_tracker = PitchTrack('pyin', hop_size=hop_size)
        self.last_bow_direction = 1  # Since its norm to start from down bow, last direction is up

        self.network_handler = None
        if not SIMULATE:
            self.network_handler = UDPHandler(DEVICE_IP, DEVICE_PORT, cmd_callback=self.network_callback)
            self.network_handler.start()

        # Fill this array from the robot after it finishes playing the phrase
        # Node-id order, units in mm. Bow angle in radians and Finger in thousands of torque
        self.initial_positions: np.ndarray = np.array([FINGER_HEIGHT.REST,  # FINGER Torque
                                                       STR_CHG_POS.AD.NUT,  # STRING_CHG
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
            data = np.frombuffer(data, dtype=np.uint16).reshape(-1, Hathaani.NUM_ACTUATORS)
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
            data = self.add_transitions(data)

        self.current[motor_ids] = data[-1, motor_ids]
        motor_data = Hathaani.Util.to_motor_data(data)
        if not SIMULATE:
            self.network_handler.add_to_queue(motor_data)

        print(motor_data.shape)
        print(motor_data.T)

        for i, d in enumerate(motor_data.T):
            n = len(motor_data.T) * 100 + 10 + (i + 1)
            plt.subplot(n)
            plt.plot(d)
        plt.show()

        self.terminate()

    def perform(self, phrase: np.ndarray, phrase_tonic: float, interpolate_start=False, interpolate_end=False):
        # phrase is of shape (2, N)
        # 1st dim is pitches and 2nd dim is envelope
        pitches, envelope = phrase
        pitches = Util.transpose_to_range(pitches, self.NOTE_RANGE[0])

        pitches[pitches < 1] = np.nan
        pitches, nan = Util.interpolate_nan(pitches)
        pitches = Util.zero_lpf(pitches, 0.3, restore_zeros=True, ignore_zeros=True)
        pitches = pitches - phrase_tonic + self.TONIC

        out = self.get_fingerboard_traversal_trajectories(pitches, nan, phrase_tonic,
                                                          interpolate_pitch_start=interpolate_start,
                                                          interpolate_pitch_end=interpolate_end)
        left_hand, string_change, finger, changes, nan, ostr = out
        bow_rotor, bow_height, bow_rotation = self.get_bowing_trajectories(envelope, changes, nan, smoothing_alpha=0.95)
        data = np.zeros((len(pitches), 7))
        data[:, MotorId.LEFT_HAND] = left_hand
        data[:, MotorId.STRING_CHG] = string_change
        data[:, MotorId.FINGER] = finger
        data[:, MotorId.BOW_D_LEFT] = bow_height
        data[:, MotorId.BOW_D_RIGHT] = bow_rotation
        data[:, MotorId.BOW_ROTOR] = bow_rotor

        if interpolate_start:
            data = self.add_transitions(data)

        self.current = data[-1, :]

        motor_data = Hathaani.Util.to_motor_data(data)

        if not SIMULATE:
            self.network_handler.add_to_queue(motor_data)

        n = (len(ostr) + 1) * 100 + 11
        plt.subplot(n)
        plt.plot(pitches)
        for i, p in enumerate(ostr):
            n = (len(ostr) + 1) * 100 + 10 + (i + 2)
            plt.subplot(n)
            plt.plot(p)

        plt.show()

        for i, d in enumerate(motor_data.T):
            n = len(motor_data.T) * 100 + 10 + (i + 1)
            plt.subplot(n)
            plt.plot(d)
        plt.show()

    def get_string(self, pitch: float, current_string_id=None):
        candidates = np.zeros_like(self.TUNING) - 1
        for i in range(len(self.TUNING)):
            if self.TUNING[i] <= pitch < self.RANGE_PER_STRING + self.TUNING[i]:
                # Possible to play on ith string
                if current_string_id is not None:
                    candidates[i] = abs(current_string_id - i)
                else:
                    return i, self.TUNING[i]

        sid = len(self.TUNING)
        min_loss = len(self.TUNING)
        for i, c in enumerate(candidates):
            if 0 < c < min_loss:
                sid = i
                min_loss = c

        if sid >= len(self.TUNING):
            return None, None
        return sid, self.TUNING[sid]

    def get_string_changes(self, pitches, min_length_suggestion):
        sta = Util.pick_stationary_points(pitches, return_sta=True)
        changes: List[Tuple[int, int, int, float]] = []
        sid, os_note = self.get_string(pitches[0])

        # check if it can play in 1 string.
        # String id is guaranteed to be not none since we transpose pitches to range.
        pitch_low, pitch_high = np.min(pitches), np.max(pitches)
        string_id, open_string_note = self.get_string(pitch_low)

        if pitch_high - open_string_note < self.RANGE_PER_STRING:
            print("Can be played in 1 string!")
            changes.append((0, len(pitches), string_id, pitch_high))
        else:
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
                    changes[-1] = (changes[-1][0], r, sid, max(changes[-1][-1], max_pitch))
                else:
                    changes.append((l, r, sid, max_pitch))

                l = r

            i = 0
            while i < len(changes) - 1:
                s, e, sid, maxp = changes[i]
                ns, ne, n_sid, n_maxp = changes[i + 1]
                l = e - s
                if (l < min_length_suggestion) and (sid < n_sid) and (
                        maxp - self.TUNING[n_sid] < self.RANGE_PER_STRING):
                    changes[i + 1] = (s, ne, n_sid, max(maxp, n_maxp))
                    changes.remove(changes[i])
                else:
                    i += 1
        return changes, sta

    def get_fingerboard_traversal_trajectories(self, pitches, nan, min_length_suggestion=100,
                                               interpolate_pitch_start=False, interpolate_pitch_end=False):
        # If it cannot be played in 1 string
        # Split the phrase into chunks using open tuning ranges
        # For each chunk, assign a string
        # Return trajectories

        changes, sta = self.get_string_changes(pitches, min_length_suggestion)

        ostr = np.zeros((len(self.TUNING), len(pitches)))
        left_hand = np.zeros_like(pitches)
        string_change = np.zeros_like(pitches)
        finger = np.zeros_like(pitches) + FINGER_HEIGHT.OFF

        prev_sid = None
        prev_sc = None
        for i, (l, r, sid, _) in enumerate(changes):
            left_hand[l: r] = Hathaani.Util.to_left_hand_position_mm(pitches[l: r] - self.TUNING[sid])
            offset = 1 + self.finger_press_time // 2
            string_change_position_id = Hathaani.Util.string_id_to_mm(sid, prev_sid)
            if prev_sc is not None:
                p = STR_CHG_POS[string_change_position_id].NUT
                string_change[l: l + offset] = Util.interp(prev_sc, p, offset, 0.2)
            else:
                string_change[l: l + offset] = STR_CHG_POS[string_change_position_id].NUT

            string_change[l + offset: r] = STR_CHG_POS[string_change_position_id].NUT

            finger[l + offset: r - offset] = FINGER_HEIGHT.ON
            ostr[sid, l: r] = left_hand[l: r]
            prev_sid = sid
            prev_sc = string_change[r - 1]

        left_hand[-1] = Hathaani.Util.to_left_hand_position_mm(pitches[-1] - self.TUNING[sid])

        # Finger off during invalid pitches
        for n in nan:
            finger[n[0]: n[1]] = FINGER_HEIGHT.OFF

        first_sta = Util.get_nearest_sta(sta, 1, lower=False)
        last_sta = Util.get_nearest_sta(sta, len(pitches) - 2, lower=True)

        if interpolate_pitch_start:
            left_hand[:first_sta] = Util.interp(left_hand[0], left_hand[first_sta], first_sta)
        if interpolate_pitch_end:
            left_hand[last_sta:] = Util.interp(left_hand[last_sta], left_hand[-1], len(left_hand) - last_sta)

        ostr[sid, -1] = left_hand[-1]
        left_hand = Util.zero_lpf(left_hand, 0.3, restore_zeros=False)
        string_change = Util.zero_lpf(string_change, 0.3, restore_zeros=False)
        finger = Util.zero_lpf(finger, 0.3, restore_zeros=False)
        return left_hand, string_change, finger, changes, nan, ostr

    def get_bowing_trajectories(self, envelope: np.ndarray, string_changes, nan,
                                min_bow_length_ms=100, smoothing_alpha=0.9):

        lpf_e = Util.zero_lpf(envelope, smoothing_alpha, restore_zeros=False)

        # Whenever finger is up (to change strings), the bow should be up too...
        for i, _, sid, _ in string_changes:
            offset = self.finger_press_time
            if i - offset >= 0 and i + offset <= len(lpf_e):
                traj1 = Util.interp(lpf_e[i - offset], 0, offset, tb_percentage=0.45)
                traj2 = Util.interp(0, lpf_e[i + offset], offset, tb_percentage=0.45)
                o = 0
                lpf_e[i + o:i + offset + o] = traj1
                lpf_e[i + offset + o: i + (2 * offset) + o] = traj2

        changes = Util.pick_dips(envelope, self.fs, self.hop, smoothing_alpha=0.9, wait_ms=min_bow_length_ms)
        changes = [0] + changes + [len(envelope)]  # ex: [0, 73, 141, 723]
        bow = self.generate_bow_rotor_trajectory(changes, nan,
                                                 min_velocity=BOW_MIN_VELOCITY, max_velocity=BOW_MAX_VELOCITY,
                                                 bow_min=BOW_ROTOR.MIN, bow_max=BOW_ROTOR.MAX)
        bow_height = Hathaani.Util.to_bow_height_mm(lpf_e, string_changes)
        bow_rotation = self.generate_bow_rotation_trajectory(string_changes)
        # correct for differential center deviations
        bow_height = Hathaani.Util.adjust_bow_height_for_center_deviations(bow_height, bow_rotation, string_changes)

        bow_height = Util.zero_lpf(bow_height, 0.3, restore_zeros=False)

        return bow, bow_height, bow_rotation

    def generate_bow_rotor_trajectory(self,
                                      changes: list,
                                      nan: List[Tuple[int, int]],
                                      min_velocity=0.25,  # % of bow_length/sec
                                      max_velocity=1.0,  # % of bow_length/sec
                                      min_bow_length=10,
                                      bow_min=0.0, bow_max=1.0,
                                      tb_cent=0.01) -> np.ndarray:
        direction = 1 ^ self.last_bow_direction  # 0 means down bow, 1 means up bow
        changes = Hathaani.Util.filter_bow_changes(changes, tb_cent, min_bow_length)
        # For all changes, if the idx is in-between nans, remove that index
        temp = set()
        for c in changes:
            if len(nan) > 0:
                for n in nan:
                    if c < n[0] or c > n[1]:
                        temp.add((c, 1))
            else:
                temp.add((c, 1))

        changes = temp.copy()

        # Add nan boundaries to changes
        for n in nan:
            changes.add((n[0], 0))
            changes.add((n[1], 1))

        changes = sorted(list(changes))
        num_segments = len(changes) - 1
        # ex: if changes = [0, 73, 141, 723], t_seg = [(0, 73, 1), (73, 141, 1), (141, 723, 1)]
        # the 3rd value denotes if it is a valid segment (1) or nans (0)
        t_seg = np.ones((num_segments, 3), dtype=int)
        t_seg[0, 0] = changes[0][0]
        t_seg[0, 2] = changes[0][1]

        k = 1
        for i in range(1, num_segments):
            t_seg[i - 1, 1] = changes[k][0]
            t_seg[i, 0] = changes[k][0]
            t_seg[i, 2] = changes[k][1]
            k += 1
        t_seg[-1, 1] = changes[-1][0]

        bow = np.zeros(changes[-1][0])

        p0 = self.current[MotorId.BOW_ROTOR]

        total_bow_length = bow_max - bow_min
        for i in range(num_segments):
            seg = (t_seg[i, 0], t_seg[i, 1])
            seg_len = seg[1] - seg[0]

            # If it is a nan region, dont move the bow
            if t_seg[i, 2] == 0:  # nan region
                direction ^= 1  # revert the direction that was changed during last iteration
                bow[seg[0]:seg[1]] = p0
                continue

            seg_len_sec = seg_len * self.hop / self.fs
            vel = 1.0 / seg_len_sec  # total_bow_length / seg_len_sec
            bow_len = max_velocity * total_bow_length * seg_len_sec
            if vel > max_velocity:
                pf = min(bow_max, p0 + bow_len) if direction == 0 else max(bow_min, p0 - bow_len)
            else:
                pf = bow_max if direction == 0 else bow_min

            bow_len = abs(pf - p0) / bow_max
            vel = bow_len / seg_len_sec
            # if anticipated bow speed is too low, Don't change bow. i.e. flip pf/p0 to min/max bow
            if vel < min_velocity:  # Bowing too slow
                if direction == 0:  # Down bow
                    direction = 1
                    pf = bow_min
                else:
                    direction = 0
                    pf = bow_max

            curve = Util.interp(p0, pf, seg_len, tb_cent)

            if i == 0:
                bow[:seg[-1]] = curve
            elif i == num_segments - 1:
                bow[seg[0]:] = curve
            else:
                bow[seg[0]:seg[1]] = curve
            direction ^= 1
            p0 = pf

        self.last_bow_direction = direction
        return bow

    # Trajectory for bow angle
    def generate_bow_rotation_trajectory(self, string_changes, tb_cent=0.45):
        n = string_changes[-1][1]
        string_change_traj = np.zeros(n)
        a0 = self.current[MotorId.BOW_D_RIGHT]
        for c in string_changes:
            s, e, sid, _ = c
            l = min(2 * self.finger_press_time, e - s)
            traj = Util.interp(a0, float(BOW_ANGLE[sid]), l, tb_percentage=tb_cent)
            string_change_traj[s: s + l] = traj
            string_change_traj[s + l: e] = BOW_ANGLE[sid]
            a0 = BOW_ANGLE[sid]

        return string_change_traj

    def add_transitions(self, data: np.ndarray,
                        length: int = 50, tb_cent: float = 0.2):
        """
        Append transition trajectories from current values to the corresponding first traj points
        :param data: (N, 7) shaped array having N trajectory points
        :param length: Interpolation length
        :param tb_cent: Blend percentage
        :return: data with transitions added. shape = (N + length, 7)
        """
        size = data.shape
        out = np.zeros((size[0] + length, size[1]), dtype=data.dtype)
        for i in range(7):
            traj = Util.interp(self.current[i], data[0, i], length, tb_cent, dtype=data.dtype)
            out[:length, i] = traj
            out[length:, i] = data[:, i]

        return out
