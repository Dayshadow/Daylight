#pragma once

#ifndef TIMESTEPPER_H
#define TIMESTEPPER_H

#include "util/utils.hpp"

// i hate this class and i dont know why it works
class Timestepper {
public:
    Timestepper();
    Timestepper(uint32_t p_gameUpdateFPS);
    void process_frame_start();
    void drain();
    bool accumulator_full();

    // the variable alpha is a number between 0-1 that represents the progress percentage towards the next game loop update
    // for example if the window is being rendered at 120fps and the game is updating at 60fps and you're between
    // physics updates, it will be 0.5. It is used for movement interpolation for higher refresh rates I guess
    void calculate_alpha();
    float alpha;

private:
    float accumulator;
    int gameUpdateFPS;
    float frameTime;
    fpsGauge fg;
    Uint32 startTicks;
    Uint32 frameTicks;
    float currentTime;
};

#endif