//
// Created by jesse on 09.11.23.
//

#include "renderer.h"
#include "ssd1306.h"
#include "FreeRTOS.h"
#include "task.h"

typedef struct {
    int length;
    float* values;
} InterpolationResult_t;


// TODO: measure performance of this function, compare to using fixed size array for result
InterpolationResult_t interpolate(int i0, float d0, int i1, float d1) {

    InterpolationResult_t result;
    result.length = i1 - i0;

    if (result.length == 0) {
        result.values = (float*) pvPortCalloc(sizeof (float), 1);
        result.values[0] = d0;
        return result;
    }

    for(int i = i0; i <= i1; i++) {



    }

    return result;
}

static void draw_line(ssd1306_t *disp, struct Point p1, struct Point p2) {

    int length = p2.x - p1.x;

    float* pLine = (float*) pvPortCalloc(sizeof (float), length);

    ssd1306_draw_pixel(disp, p1.x, p1.y);
}

static void draw_wireframe_triangle( ssd1306_t *disp, struct Point p1, struct Point p2, struct Point p3) {
    draw_line(disp, p1, p2);
    draw_line(disp, p2, p3);
    draw_line(disp, p3, p1);
}

_Noreturn void renderingTask(void *pvParameters) {

    ssd1306_t disp;
    disp.external_vcc = false;
    ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
    ssd1306_clear(&disp);

    ssd1306_contrast(&disp, 0xFF);
    ssd1306_invert(&disp, 0);
    ssd1306_clear(&disp);
    ssd1306_show(&disp);

    // Buffer for all points
    struct Point points[3];
    points[0].x = 0;
    points[0].y = 63;
    points[1].x = 6;
    points[1].y = 0;
    points[2].x = 127;
    points[2].y = 63;


    while (1) {
        draw_wireframe_triangle(&disp, points[0], points[1], points[2]);
        ssd1306_show(&disp);
        vTaskDelay(1000);
    }
}