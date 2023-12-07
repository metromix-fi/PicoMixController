//
// Created by jesse on 07.12.23.
//

#ifndef METROMIX_BUZZERTASK_H
#define METROMIX_BUZZERTASK_H

#define BUZZER_PIN 17

#define P 0
#define B0  1
#define C1  2
#define CS1 3
#define D1  4
#define DS1 5
#define E1  6
#define F1  7
#define FS1 8
#define G1  9
#define GS1 10
#define A1  11
#define AS1 12
#define B1  13
#define C2  14
#define CS2 15
#define D2  16
#define DS2 17
#define E2  18
#define F2  19
#define FS2 20
#define G2  21
#define GS2 22
#define A2  23
#define AS2 24
#define B2  25
#define C3  26
#define CS3 27
#define D3  28
#define DS3 29
#define E3  30
#define F3  31
#define FS3 32
#define G3  33
#define GS3 34
#define A3  35
#define AS3 36
#define B3  37
#define C4  38
#define CS4 39
#define D4  40
#define DS4 41
#define E4  42
#define F4  42
#define FS4 43
#define G4  44
#define GS4 45
#define A4  46
#define AS4 47
#define B4  48
#define C5  49
#define CS5 50
#define D5  51
#define DS5 52
#define E5  53
#define F5  54
#define FS5 55
#define G5  56
#define GS5 57
#define A5  58
#define AS5 59
#define B5  60
#define C6  61
#define CS6 62
#define D6  63
#define DS6 64
#define E6  65
#define F6  66
#define FS6 67
#define G6  68
#define GS6 69
#define A6  70
#define AS6 71
#define B6  72
#define C7  73
#define CS7 74
#define D7  75
#define DS7 76
#define E7  77
#define F7  78
#define FS7 79
#define G7  80
#define GS7 81
#define A7  82
#define AS7 83
#define B7  84
#define C8  85
#define CS8 86
#define D8  87
#define DS8 88

typedef enum BuzzerEvent {
    MELODY_1 = 0,
    MELODY_2,
} BuzzerEvent;

_Noreturn void buzzerTask(void *pvParameters);

#endif //METROMIX_BUZZERTASK_H
