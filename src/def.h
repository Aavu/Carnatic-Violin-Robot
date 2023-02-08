//
// Created by Raghavasimhan Sankaranarayanan on 01/22/22.
// 

#pragma once

// CANbus
#define PDO_RATE 6 // ms

// Epos4
#define FINGER_NODE_ID 1
#define BOW_NODE_ID 3

#define SCALE_LENGTH 320
#define P2P_MULT 100.f // 10000.f/100.f // pulse per mm
#define NUT_POS 400 // Abs value. No Negative numbers

// As fret number increases if the encoder reading is more negative, set it to -1
#define ENCODER_DIR -1
#define BOW_ENCODER_DIRECTION -1
#define CALLBACK_ENC_THRESHOLD 5
#define MAX_ENCODER_INC 26000
#define MAX_FOLLOW_ERROR 10000

// Dxl
// To make your ENUM Scoped, you need to declare it enum class
enum class DxlID {
    Left = 0,   // wrt how we look at it!
    Right = 1,
    Slide = 2
};

// string
enum class StringPos {
    E = 72,
    A = 90,
    D = 110,
    G = 120
};

#define MAX_BOW_VELOCITY 0.006 // %/sec
#define BOW_ENCODER_MIN 8000
#define BOW_ENCODER_MAX 38000
#define PROFILE_VELOCITY_PITCH 400
#define BOW_LIFT_MULTIPLIER 1.6 // Increase this number to increase bow lift towards the edges

#define BOW_CENTER_EDGE_RAIL_DISTANCE 75 // mm
#define BOW_GEAR_PITCH_DIA 24 // mm

// Unit: Pulses
// At this position, the bow angle is zero
#define DXL_LEFT_ZERO 2048
#define DXL_RIGHT_ZERO 2600

// Unit: Pulses
#define SLIDE_MAX 1750    // Near bridge
#define SLIDE_MIN 1200    // Near Fingerboard
#define SLIDE_AVG 1475

enum class YawPos {  // Angles in degrees
    Mid = 2048,
    E = 2048 - 24,
    A = 2048 - 8,
    D = 2048 + 8,
    G = 2048 + 24
};

#define FINGER_ON 252.f         // deg
#define FINGER_OFF 246.f        // deg
#define FINGER_RISE_LEVEL 7    // The amount of change in angle as the finger move along the fret

#define DXL_UPDATE_RATE 10

#define DXL_MAX_CURRENT 50