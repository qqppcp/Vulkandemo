#include "FrameTimer.h"

void FrameTimer::newFrame()
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    mLastFrameTime = std::chrono::duration_cast<std::chrono::nanoseconds>(currentTime - mLastFrameStart);
    *(mCurrentBetweenFrame++) = mLastFrameTime;
    mLastFrameStart = currentTime;
    if (mCurrentBetweenFrame == mTimeBetweenFrames.end())
        mCurrentBetweenFrame = mTimeBetweenFrames.begin();
}

float FrameTimer::framePerSecond() const
{
    auto avg = averageFrameTime<std::chrono::nanoseconds>();
    return static_cast<float>(std::chrono::seconds(1) / avg);
}
