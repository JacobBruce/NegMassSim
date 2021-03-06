#include "Timer.h"

Timer::Timer() : start(clock_::now()) {}

void Timer::ResetTimer()
{
	start = clock_::now();
}

int64_t Timer::NanoCount() const
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(clock_::now() - start).count();
}

int64_t Timer::MicroCount() const
{
	return std::chrono::duration_cast<std::chrono::microseconds>(clock_::now() - start).count();
}

float Timer::MilliCount() const
{
	return MicroCount() * 0.001f;
}
