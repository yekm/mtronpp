#pragma once
#include <time.h>

class TimeStamp
{
public:
    TimeStamp();
    double operator-(const TimeStamp &);
private:
    timespec m_ts;
};

TimeStamp::TimeStamp()
{
    clock_gettime(CLOCK_MONOTONIC_RAW, &m_ts);
}

double TimeStamp::operator - (const TimeStamp & o)
{
    return m_ts.tv_sec - o.m_ts.tv_sec + (m_ts.tv_nsec - o.m_ts.tv_nsec) / 1e9;
}



class Timer
{
public:
    typedef double timediff_type;
    Timer();
    timediff_type get();
    timediff_type reset();
    timediff_type since(const Timer &);
private:
    TimeStamp m_ts;
};

Timer::Timer()
{
}

Timer::timediff_type Timer::get()
{
    return TimeStamp() - m_ts;
}

Timer::timediff_type Timer::reset()
{
    double d = get();
    m_ts = TimeStamp();
    return d;
}

Timer::timediff_type Timer::since(const Timer & s)
{
    return m_ts - s.m_ts;
}
