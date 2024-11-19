#include "defershade.h"
#include "forwardshade.h"
#include "system.h"

int main(int argc, char** argv)
{
	SystemManger::Init(1280, 720);
	{
		Application* app = new ForwardShade();
		app->Init(1280, 720);
		app->run();
		app->Shutdown();
	}
	SystemManger::Shutdown();
	return 0;
}