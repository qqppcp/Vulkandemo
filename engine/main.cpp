#include "window.h"
#include "backend.h"

int main(int argc, char** argv)
{
	CreateWindow(1280, 720, "demo");
	Init();
	while (!WindowShouleClose())
	{
		Render();
		WindowEventProcessing();
	}
	Quit();
	DestroyWindow();
	return 0;
}