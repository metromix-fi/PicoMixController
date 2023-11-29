//
// Created by jesse on 08.11.23.
//

#ifndef METROMIX_ANIMATIONTASK_H
#define METROMIX_ANIMATIONTASK_H

void setup_display_gpios(void);

_Noreturn void animationTask(void *param);

#define MAX_CUP_DISTANCE 50
#define DISTANCE_MEASURE_DELAY 1000


// States

typedef enum MenuState {
    DRINK_SELECT = 0,
    SIZE_SELECT,
    MIXTURE_SELECT,
    AUTHENTICATION,
    ERROR,
    POURING,
    NO_CUP,
    DONE,
    IDLE
} MenuState;

typedef enum Drink {
    DRINK_1 = 0,
    DRINK_2,
    DRINK_3,
} Drink;

typedef enum Size {
    SMALL = 0,
    MEDIUM,
    LARGE,
} Size;

typedef struct CocktailConfig {
    Drink drink1;
    Drink drink2;
    Size size;
    int mixture[3];
} CocktailConfig;

#endif //METROMIX_ANIMATIONTASK_H
