#pragma once
#include <Framework/Graphics/Shader.hpp>

class GameShaders {
public:
    static GameShaders& Get() {
        static GameShaders instance;
        return instance;
    }
    Shader polyShader{ "./src/Shaders/PolygonVS.glsl", "./src/Shaders/PolygonFS.glsl" };
private:
    GameShaders();
};