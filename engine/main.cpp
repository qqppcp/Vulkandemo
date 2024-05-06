#include "window.h"
#include "Context.h"

int main(int argc, char** argv)
{
	CreateWindow(1280, 720, "demo");
	Context::InitContext();
	while (!WindowShouleClose())
	{
		WindowEventProcessing();
	}

	DestroyWindow();
	return 0;
}