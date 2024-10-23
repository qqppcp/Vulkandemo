#pragma once

#include <array>
#include <chrono>

class FrameTimer
{
public:
	void newFrame();
	float framePerSecond() const;

	template <typename Duration>
	Duration lastFrameTime() const
	{
		return std::chrono::duration_cast<Duration>(mLastFrameTime);
	}
	template <typename Duration>
	Duration averageFrameTime() const noexcept
	{
		std::chrono::nanoseconds time{ 0 };
		for (auto const& it : mTimeBetweenFrames)
			time += it;
		time /= mTimeBetweenFrames.size();

		return std::chrono::duration_cast<Duration>(time);
	}
private:
	std::chrono::nanoseconds mLastFrameTime{ 0 };
	std::chrono::high_resolution_clock::time_point mLastFrameStart{ std::chrono::high_resolution_clock::now() };
	std::array<std::chrono::nanoseconds, 128> mTimeBetweenFrames{};
	std::array<std::chrono::nanoseconds, 128>::iterator mCurrentBetweenFrame{ mTimeBetweenFrames.begin() };
};

