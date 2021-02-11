#pragma once
#include<chrono>
#include<iostream>
#include<math.h>

using namespace std::chrono;

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<Clock>;
	
class Log
{
public:
	typedef std::chrono::high_resolution_clock Time;
	
	void InitLog(string funcName)
	{
		this->funcName=funcName;
		start = high_resolution_clock::now();
	}
	
	void EndLog()
	{
		end= high_resolution_clock::now();
		std::cout << funcName << " execution time:" << std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count()*pow(10,-6) << " ms\n";
	}
	
private:
	string funcName;
	TimePoint start;
	TimePoint end;
};