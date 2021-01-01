#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include "responsive.h"


#define DEBUG 0


void analog_responsive_begin(Responsive * x, bool sleepEnable, float snapMultiplier){
    //pinMode(pin, INPUT ); // ensure button pin is an input
    //digitalWrite(pin, LOW ); // ensure pullup is off on button pin
    
    x->sleepEnable = sleepEnable;
    setSnapMultiplier(x, snapMultiplier);
}


void analog_responsive_update(Responsive * x, int rawValueRead)
{
  x->rawValue = rawValueRead;
  x->prevResponsiveValue = x->responsiveValue;
  x->responsiveValue = getResponsiveValue(x);
  x->responsiveValueHasChanged = x->responsiveValue != x->prevResponsiveValue;
}


uint32_t getResponsiveValue(Responsive * x)
{
    // if sleep and edge snap are enabled and the new value is very close to an edge, drag it a little closer to the edges
    // This'll make it easier to pull the output values right to the extremes without sleeping,
    // and it'll make movements right near the edge appear larger, making it easier to wake up

    if(x->sleepEnable && x->edgeSnapEnable) {
        if(x->rawValue < x->activityThreshold) {
            x->rawValue = (x->rawValue * 2) - x->activityThreshold;
        } else if(x->rawValue > x->analogResolution - x->activityThreshold) {
            x->rawValue = (x->rawValue * 2) - x->analogResolution + x->activityThreshold;
        }
    }

#if DEBUG
    printf("x->rawValue: %d\n", x->rawValue);
#endif 
    // get difference between new input value and current smooth value
    uint32_t diff = abs(x->rawValue - x->smoothValue);

#if DEBUG
    printf("diff: %d\n", diff);
#endif
    // measure the difference between the new value and current value
    // and use another exponential moving average to work out what
    // the current margin of error is
    x->errorEMA += ((x->rawValue - x->smoothValue) - x->errorEMA) * 0.4;

#if DEBUG
    printf("errorEMA: %f\n", x->errorEMA);
#endif 

    // if sleep has been enabled, sleep when the amount of error is below the activity threshold
    if(x->sleepEnable) {
        // recalculate sleeping status
        x->sleeping = abs(x->errorEMA) < x->activityThreshold;
    }

#if DEBUG
    printf("x->sleeping: %d\n", x->sleeping);
    // if we're allowed to sleep, and we're sleeping
    // then don't update responsiveValue this loop
    // just output the existing responsiveValue

    printf("primo x->smoothValue: %d\n", (uint32_t)x->smoothValue);
#endif 

    if(x->sleepEnable && x->sleeping) {
        return (uint32_t)x->smoothValue;
    }

    // use a 'snap curve' function, where we pass in the diff (x) and get back a number from 0-1.
    // We want small values of x to result in an output close to zero, so when the smooth value is close to the input value
    // it'll smooth out noise aggressively by responding slowly to sudden changes.
    // We want a small increase in x to result in a much higher output value, so medium and large movements are snappy and responsive,
    // and aren't made sluggish by unnecessarily filtering out noise. A hyperbola (f(x) = 1/x) curve is used.
    // First x has an offset of 1 applied, so x = 0 now results in a value of 1 from the hyperbola function.
    // High values of x tend toward 0, but we want an output that begins at 0 and tends toward 1, so 1-y flips this up the right way.
    // Finally the result is multiplied by 2 and capped at a maximum of one, which means that at a certain point all larger movements are maximally snappy

    // then multiply the input by SNAP_MULTIPLER so input values fit the snap curve better.
    float snap = snapCurve(diff * x->snapMultiplier);

    // when sleep is enabled, the emphasis is stopping on a responsiveValue quickly, and it's less about easing into position.
    // If sleep is enabled, add a small amount to snap so it'll tend to snap into a more accurate position before sleeping starts.
    if(x->sleepEnable) {
        snap *= 0.5 + 0.5;
    }

#if DEBUG
    printf("x->snapMultiplier: %f\n", x->snapMultiplier);
    printf("snap: %f\n", snap);
#endif 

    // calculate the exponential moving average based on the snap
    x->smoothValue += (x->rawValue - x->smoothValue) * snap;

    // ensure output is in bounds
    if(x->smoothValue < 0.0) {
        x->smoothValue = 0.0;
    } else if(x->smoothValue > x->analogResolution - 1) {
        x->smoothValue = x->analogResolution - 1;
    }
#if DEBUG
    printf("secondo x->smoothValue: %d\n", (uint32_t)x->smoothValue);
#endif 

    // expected output is an integer
    return (uint32_t)x->smoothValue;
}

float snapCurve(float x)
{
    float y = 1.0 / (x + 1.0);
    y = (1.0 - y) * 2.0;
    if(y > 1.0) {
        return 1.0;
    }
    return y;
}

void setSnapMultiplier(Responsive * x, float newMultiplier)
{
    if(newMultiplier > 1.0) {
        newMultiplier = 1.0;
    }
    if(newMultiplier < 0.0) {
        newMultiplier = 0.0;
    }
    x->snapMultiplier = newMultiplier;
}

uint32_t getValue(Responsive * x) {
  return x->responsiveValue;
}

uint32_t getRawValue(Responsive * x) {
  return x->rawValue;
}

bool hasChanged(Responsive * x) {
  return x->responsiveValueHasChanged;
}

bool isSleeping(Responsive * x) {
  return x->sleeping;
}

void enableSleep(Responsive * x) {
  x->sleepEnable = true;
}

void disableSleep(Responsive * x) {
  x->sleepEnable = false;
}

void enableEdgeSnap(Responsive * x) {
  x->edgeSnapEnable = true;
}

void disableEdgeSnap(Responsive * x) {
  x->edgeSnapEnable = false;
}

void setActivityThreshold(Responsive * x, float newThreshold) {
  x->activityThreshold = newThreshold;
}

void setAnalogResolution(Responsive * x, int resolution) {
  x->analogResolution = resolution;
}