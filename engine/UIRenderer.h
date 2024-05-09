#pragma once



class ImGuiRenderer
{
public:
	ImGuiRenderer();
	~ImGuiRenderer();
	void render(float delta_time = 0);
};