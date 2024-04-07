#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec4 out_fragColor;

layout (location = 0) in VS_OUT
{
  vec2 texCoord;
} surf;

layout (binding = 0) uniform sampler2D prevFrame;
layout (binding = 1) uniform sampler2D currFrame;
layout (binding = 2) uniform sampler2D motionVectors;

void main()
{
  float weight = 0.95;
  
  vec2 cur_uv = surf.texCoord;

  vec2 velocity = textureLod(motionVectors, cur_uv, 0).xy;
  vec2 prev_uv = cur_uv - velocity;

  vec3 currColor = textureLod(currFrame, cur_uv, 0).rgb;
  vec3 prevColor = textureLod(prevFrame, prev_uv, 0).rgb;

  out_fragColor = vec4(mix(currColor, prevColor, weight), 1.);
}
