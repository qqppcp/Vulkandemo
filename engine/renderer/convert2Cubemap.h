#pragma once
#include <memory>

class Texture;

std::shared_ptr<Texture> convert2Cubemap(std::shared_ptr<Texture> flatTex);