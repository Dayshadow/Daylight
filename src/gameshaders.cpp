#include "GameShaders.hpp"

GameShaders::GameShaders() 
{
    glm::mat4 tmp;
    polyShader.addMat4Uniform("transform", tmp);
}