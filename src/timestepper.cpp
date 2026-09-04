#include "Timestepper.hpp"

Timestepper::Timestepper()
{
}

Timestepper::Timestepper(uint32_t p_gameUpdateFPS)
    : gameUpdateFPS(p_gameUpdateFPS), accumulator(0.0f), currentTime(utils::hireTimeInSeconds()), alpha(0.0f), startTicks(0), frameTicks(0), frameTime(0.0f)
{}
void Timestepper::process_frame_start() {
    fg.stopStopwatch();
    fg.startStopwatch();
    accumulator += (float)fg.getSecondsElapsed();
}
void Timestepper::drain()
{
    accumulator -= 1.0f / gameUpdateFPS;
}
bool Timestepper::accumulator_full() {
    return accumulator >= 1.0f / gameUpdateFPS;
}

void Timestepper::calculate_alpha() {
    alpha = accumulator * gameUpdateFPS; // did some quick maf, same thing as accumulator / (1 / gameUpdateFPS)
}