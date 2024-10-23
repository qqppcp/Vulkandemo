#include "application.h"

int main(int argc, char** argv)
{
	Application* app = new Application(1280, 720);
	app->run();
	delete app;
	return 0;
}