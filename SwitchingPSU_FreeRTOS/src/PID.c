/* PID.c */
#include "PID.h"

float PID(float Kp, float Ki, float Kd, float voltageref, float voltage){
    static float e, e_prev, yi_prev;
    float yp, yi, yd, out;
    e = voltageref - voltage; // Calculating difference
    // Calculating yp, yi ja yd
    yp = Kp*e;
    yi = yi_prev+Ki*e;
    yd = Kd*e-Kd*e_prev;
    out = yp + yi + yd; // PID output
    // Updating old values
    yi_prev = yi;
    e_prev = e;
    return out;
}