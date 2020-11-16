#pragma once
#include<chrono>
#include<iostream>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
	
class Log
{
public:
	typedef std::chrono::high_resolution_clock Time;

	void InitLog(string funcName)
	{
		this->funcName=funcName;
		start=Clock::now();
	}
	
	void EndLog()
	{
		end=Clock::now();
		std::cout << funcName << " execution time:" << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << " ms\n";	
	}
	
private:
	string funcName;
	TimePoint start;
	TimePoint end;
};