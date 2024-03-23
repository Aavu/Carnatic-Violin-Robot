import math
from enum import Enum, IntEnum
from typing import NamedTuple, Tuple

SIMULATE = True

""" Set it to True if not connected to the OpenCR board """

DEVICE_IP = "10.2.1.177"
DEVICE_PORT = 8888

SCALE_LENGTH = 330  # mm
""" Scale length of the violin - distance between the nut and the bridge in mm"""

FINGERBOARD_LENGTH = 270  # mm

BOW_MIN_VELOCITY = 0.25  # % of bow_length/sec
BOW_MAX_VELOCITY = 1.0  # % of bow_length/sec
""" Maximum speed of the bow rotor """

BOW_ANGLE_LIMIT = math.pi / 4
""" Bow Angle limit in radians """

BOW_CENTER_RAIL_DISTANCE = 72.0  # mm
BOW_GEAR_RADIUS = 6.35  # mm

GT2_PULLEY_RADIUS = 5.05  # mm
ENCODER_TICKS_PER_TURN = [2048, 1024, 1024, 1024, 1024, 1024, 1024]  # ticks per 90 deg turn
FINGER_GEAR_RADIUS = 12.0  # mm

FINGER_PRESS_TIME_MS = 50  # ms

# center of the fingerboard wrt string change reference bolt (- finger stage length)
CENTER_LINE = 24.5

BOW_HEIGHT_VERTICAL_DEVIATION = 5


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
    Conversion factors to transform values in mm to encoder ticks.
    """
    FINGER: float = ENCODER_TICKS_PER_TURN[MotorId.FINGER] * 4 / (2 * math.pi * FINGER_GEAR_RADIUS)
    STRING_CHG: float = ENCODER_TICKS_PER_TURN[MotorId.STRING_CHG] * 4 / (2 * math.pi * GT2_PULLEY_RADIUS)
    LEFT_HAND: float = ENCODER_TICKS_PER_TURN[MotorId.LEFT_HAND] * 4 / (2 * math.pi * GT2_PULLEY_RADIUS)
    BOW_D_LEFT: float = ENCODER_TICKS_PER_TURN[MotorId.BOW_D_LEFT] * 4 / (2 * math.pi * GT2_PULLEY_RADIUS)
    BOW_D_RIGHT: float = ENCODER_TICKS_PER_TURN[MotorId.BOW_D_RIGHT] * 4 / (2 * math.pi * GT2_PULLEY_RADIUS)
    BOW_SLIDE: float = 1  # Not used
    BOW_ROTOR: float = ENCODER_TICKS_PER_TURN[MotorId.BOW_ROTOR] * 4 / (2 * math.pi * BOW_GEAR_RADIUS)


# class Limit(IntEnum):
#     """
#     Max Limits of actuators in encoder ticks
#     """
#     LEFT_HAND = 26000
#     STRING_CHANGE = 1024
#     FINGER = 100
#     BOW_LEFT_DIFF = 9700
#     BOW_RIGHT_DIFF = 9700
#     BOW_SLIDE = 0
#     BOW_ROTOR = 38000


class EndPoint(NamedTuple):
    NUT: float
    BRIDGE: float


class Range(NamedTuple):
    MIN: float
    MAX: float


class StringChangePosition(NamedTuple):
    """
    Position from reference bolt measured at the (fingerboard nut, bridge) - in mm
    """
    EA: EndPoint = EndPoint(CENTER_LINE + 5, CENTER_LINE + 18)
    AD: EndPoint = EndPoint(CENTER_LINE, CENTER_LINE)
    DG: EndPoint = EndPoint(CENTER_LINE - 5, CENTER_LINE + 18)


class StringDistanceOnBridge(NamedTuple):
    """
    Distance (in mm) of each string on the bridge wrt bridge center
    """
    E: float = 18
    A: float = 6
    D: float = 6
    G: float = 18


class StringHeight(NamedTuple):
    """
    Height in mm of the string at nut and at fingerboard end
    """
    NUT: float = 2.0
    END: float = 7.5


class FingerHeight(NamedTuple):
    """ units in mm. Rest is the top most position the finger can move to."""
    REST: float = 0
    ON: float = 20
    OFF: float = 12  # Near nut


class BowAngle(NamedTuple):
    """
    Angle of the bow in radians to play each string
    """
    E: float = -24 * math.pi / 180
    A: float = -8 * math.pi / 180
    D: float = 8 * math.pi / 180
    G: float = 24 * math.pi / 180
    REST: float = 0


class BowHeight(NamedTuple):
    """
    Denotes the height of bow from the string. \n
    Minimum and maximum bow height corresponding to amplitudes 0.0, 1.0 respectively. \n
    MAX is when the bow is fully touching the strings without distortion. \n
    MIN is when the bow is just above the strings.
    Measurements are at bow angle = 0
    """
    # bow placements
    # G - 44mm, 27deg
    # D - 55mm, 8 deg
    # A - 53mm, -8deg
    # E - 42mm, -25deg
    E: Range = Range(40, 42.5)
    A: Range = Range(50.5, 60)
    D: Range = Range(53, 60)
    G: Range = Range(44, 48)
    REST: float = 0


class BowRotor(NamedTuple):
    """
    Minimum and Maximum position of the bow in mm
    """
    MIN: float = 0
    MAX: float = 380 - MIN
    REST: float = MIN


# We will need to create instances of the NamedTuple before using them
TUNING = Tuning()  # E A D G strings
TICKS = Ticks()
STR_CHG_POS = StringChangePosition()
STR_DIST_BRIDGE = StringDistanceOnBridge()
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
