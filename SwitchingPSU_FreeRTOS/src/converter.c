/*
 * converter.c
 *
 *  Created on: Dec 14, 2022
 *      Author: dHoundenko & samppa
 */
#include <stdio.h>
#include <stdlib.h>
#include "converter.h"

float model(float PID_out) {
    unsigned int i,j;

    static const float A[6][6]={
            { 0.9652, -0.0172, 0.0057, -0.0058, 0.0052, -0.0251 },
            { 0.7732, 0.1252, 0.2315, 0.07, 0.1282,  0.7754     },
            { 0.8278, -0.7522, -0.0956, 0.3299, -0.4855, 0.3915 },
            { 0.9948, 0.2655, -0.3848, 0.4212, 0.3927, 0.2899   },
            { 0.7648, -0.4165, -0.4855, -0.3366, -0.0986, 0.7281},
            { 1.1056, 0.7587, 0.1179, 0.0748, -0.2192, 0.1491   },
    };

    static const float B[6]=
    { 0.0471, 0.0377, 0.0404, 0.0485, 0.0373, 0.0539 };

    /* states represent new values i1-3 and u1-3 in the circuit */
    static float states[6]={ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    /* states represent old values i1-3 and u1-3 in the circuit */
    static float oldstates[6]={ 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };


    /* matrix calculation */
    for (i=0; i<6; i++) {
        states[i] = 0.0;
        for (j=0; j<6; j++){
            states[i] = states[i]+A[i][j]*oldstates[j];
            }
        /* PID_out is output from PI(D) converter (u_in) */
        states[i] = states[i]+B[i]*PID_out;
    }
    for (i=0; i<6; i++){
        oldstates[i] = states[i];
    }

//output is u3
return states[5];
}
