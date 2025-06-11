#pragma once

#define SYSTEM_TIMER(deltaTime,durationSec)\
    static float timer_value = 0;\
    int timer_ticks = 0;\
    timer_value += deltaTime;\
    while(timer_value > durationSec){\
        timer_value -= durationSec;\
        timer_ticks++;\
    }
