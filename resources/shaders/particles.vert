#version 450

layout (location = 0) in vec2 inPos;
layout (location = 1) in float inRemainingLifetime;
layout (location = 1) out float outRemainingLifetime;

out gl_PerVertex
{
	vec4 gl_Position;
	float gl_PointSize;
};

void main () 
{
  outRemainingLifetime = inRemainingLifetime;
  gl_PointSize = 5.0;
  gl_Position = vec4(inPos.xy, 1.0, 1.0);
}
