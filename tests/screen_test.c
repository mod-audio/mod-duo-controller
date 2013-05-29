
#include "config.h"
#include "screen.h"
#include "glcd.h"
#include "utils.h"

void screen_control_test(void)
{
    screen_controls_index(0, 10, 34);
    screen_controls_index(1, 2, 5);
    screen_controls_index(2, 5, 20);
    screen_controls_index(3, 99, 99);

    control_t control;

    control.label = "Control One";
    control.unit = "ms";
    control.minimum = 0.0;
    control.maximum = 100.0;
    control.value = 50.0;
    control.properties = CONTROL_PROP_LINEAR;
    screen_control(0, &control);

    control.label = "Control Two";
    control.unit = "Hz";
    control.minimum = 20.0;
    control.maximum = 440.0;
    control.value = 120.0;
    control.properties = CONTROL_PROP_LOGARITHMIC;
    screen_control(1, &control);

    scale_point_t **scale_points;
    scale_points = MALLOC(5 * sizeof(scale_point_t *));

    scale_point_t points[5];
    points[0].label = "one";
    points[0].value = 1.0;
    points[1].label = "two";
    points[1].value = 2.0;
    points[2].label = "three";
    points[2].value = 3.0;
    points[3].label = "four";
    points[3].value = 4.0;
    points[4].label = "five";
    points[4].value = 5.0;

    uint8_t i;
    for (i = 0; i < 5; i++) scale_points[i] = &points[i];

    control.label = "Control Three";
    control.unit = NULL;
    control.value = 1.0;
    control.properties = CONTROL_PROP_ENUMERATION;
    control.scale_points_count = 5;
    control.scale_points = scale_points;
    screen_control(2, &control);

    control.label = "Control Four";
    control.unit = "cru";
    control.minimum = -10.0;
    control.maximum = 10.0;
    control.value = 1.0;
    control.properties = CONTROL_PROP_LINEAR;
    //screen_control(3, &control);
    screen_control(3, NULL);


    screen_footer(0, "Footer1", "Yes");
    screen_footer(1, "Footer2", NULL);
    screen_footer(2, NULL, "value");
    screen_footer(3, NULL, NULL);

    glcd_update();
    while (1);
}
