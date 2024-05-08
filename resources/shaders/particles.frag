#version 450

layout (location = 1) in float inRemainingLifetime;
layout (location = 0) out vec4 outFragColor;

void main () 
{
  outFragColor = vec4(1.0f, 1.0f, 1.0f, inRemainingLifetime);
}
