#pragma once

#include <string>

class Application
{
public:
	Application(int width, int height, std::string name = "demo");
	~Application();
	void commonInit();
	void commonShutdown();
	void run();
private:

};