import math
from enum import Enum, IntEnum
from typing import NamedTuple, Tuple

SIMULATE = False

""" Set it to True if not connected to the OpenCR board """

DEVICE_IP = "10.2.1.177"
DEVICE_PORT = 8888

SCALE_LENGTH = 335 #332  # mm
""" Scale length of the violin - distance between the nut and the bridge in mm"""

NUT_POSITION_MM = 14
FINGERBOARD_LENGTH = 270  # mm

BOW_MIN_VELOCITY = 0.15  # % of bow_length/sec
""" Minimum speed of the bow rotor """
BOW_MAX_VELOCITY = 1  # % of bow_length/sec
""" Maximum speed of the bow rotor """

BOW_ANGLE_LIMIT = math.pi / 4
""" Bow Angle limit in radians """

BOW_CENTER_RAIL_DISTANCE = 72.0  # mm
BOW_GEAR_RADIUS = 6.35  # mm

GT2_PULLEY_RADIUS = 5.05  # mm
ENCODER_TICKS_PER_TURN = [2048, 1024, 1024, 1024, 1024, 1024, 1024]  # ticks per 90 deg turn
FINGER_GEAR_RADIUS = 12.0  # mm

FINGER_PRESS_TIME_MS = 50  # ms
STRING_CHANGE_TIME_MS = 150  # ms

# center of the fingerboard wrt string change reference bolt (- finger stage length)
CENTER_LINE: float = 26.0    # 24.5

BOW_HEIGHT_VERTICAL_DEVIATION = 3  # mm (positive value => more pressure for higher positions)
BOW_HEIGHT_CHANGE_FOR_OPEN_STRING_NOTE = -0.25  # mm


class Tuning(NamedTuple):
    E: int = 74
    A: int = 69
    D: int = 62
    G: int = 57


class MotorId(IntEnum):
    """
    Represents the node id of the motors.
    P.S: it is 0 indexed. For epos, 1st node starts with node ID = 1!!!
    """
    FINGER = 0
    STRING_CHG = 1
    LEFT_HAND = 2
    BOW_D_LEFT = 3
    BOW_D_RIGHT = 4
    BOW_SLIDE = 5
    BOW_ROTOR = 6


class Ticks(NamedTuple):
    """
    Conversion factors to transform values in mm (except bow slide) to encoder ticks.
    """
    FINGER: float = ENCODER_TICKS_PER_TURN[MotorId.FINGER] * 4 / (2 * math.pi * FINGER_GEAR_RADIUS)
    STRING_CHG: float = ENCODER_TICKS_PER_TURN[MotorId.STRING_CHG] * 4 / (2 * math.pi * GT2_PULLEY_RADIUS)
    LEFT_HAND: float = ENCODER_TICKS_PER_TURN[MotorId.LEFT_HAND] * 4 / (2 * math.pi * GT2_PULLEY_RADIUS)
    BOW_D_LEFT: float = ENCODER_TICKS_PER_TURN[MotorId.BOW_D_LEFT] * 4 / (2 * math.pi * GT2_PULLEY_RADIUS)
    BOW_D_RIGHT: float = ENCODER_TICKS_PER_TURN[MotorId.BOW_D_RIGHT] * 4 / (2 * math.pi * GT2_PULLEY_RADIUS)
    BOW_SLIDE: float = 1000
    BOW_ROTOR: float = ENCODER_TICKS_PER_TURN[MotorId.BOW_ROTOR] * 4 / (2 * math.pi * BOW_GEAR_RADIUS)


class Range(NamedTuple):
    MIN: float
    MAX: float


class StringChangePosition(NamedTuple):
    """
    Position from reference bolt measured at the (fingerboard nut, bridge) - in mm
    """
    EA: Range = Range(CENTER_LINE + 5, CENTER_LINE + 14)
    AD: Range = Range(CENTER_LINE, CENTER_LINE)
    DG: Range = Range(CENTER_LINE - 5.5, CENTER_LINE - 14)


class StringHeight(NamedTuple):
    """
    Height in mm of the string at nut and at fingerboard end
    """
    NUT: float = 2.0
    END: float = 7.5


class FingerHeight(NamedTuple):
    """ units in mm. Rest is the top most position the finger can move to."""
    REST: float = 0
    ON: float = 19.5
    OFF: float = 10  # Near nut


class BowAngle(NamedTuple):
    """
    Angle of the bow in radians to play each string
    """
    E: float = -22 * math.pi / 180
    A: float = -4.75 * math.pi / 180
    D: float = 4.75 * math.pi / 180
    G: float = 25 * math.pi / 180
    REST: float = 0


class BowHeight(NamedTuple):
    """
    Denotes the height of bow from the string. \n
    Minimum and maximum bow height corresponding to amplitudes 0.1, 1.0 respectively. \n
    MAX is when the bow is fully touching the strings without distortion. \n
    MIN is when the bow is just above the strings.
    Measurements are at bow angle = 0
    """
    E: Range = Range(47, 50)    # Range(47, 50)
    A: Range = Range(57, 59.5)  # Range(57, 59.5)
    D: Range = Range(57, 59)    # Range(57, 60)
    G: Range = Range(47, 52)    # Range(47, 52)
    REST: float = 46


class BowRotor(NamedTuple):
    """
    Minimum and Maximum position of the bow in mm
    """
    MIN: float = 0
    MAX: float = 380
    REST: float = 0


# We will need to create instances of the NamedTuple before using them
TUNING = Tuning()  # E A D G strings
TICKS = Ticks()
STR_CHG_POS = StringChangePosition()
STRING_HEIGHT = StringHeight()
FINGER_HEIGHT = FingerHeight()
BOW_ANGLE = BowAngle()
BOW_HEIGHT = BowHeight()
BOW_ROTOR = BowRotor()
BRIDGE_CURVATURE = [2, 0, 0, 2]


class Command(IntEnum):
    STOP = 0
    START = 1
    HOME = 2
    READY = 3
    REQUEST_DATA = 4
    CURRENT_VALUES = 5
    SERVO_OFF = 6
    SERVO_ON = 7
    DEFAULTS = 8
    RESTART = 9
