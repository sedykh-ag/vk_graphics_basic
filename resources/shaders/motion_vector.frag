#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec2 out_fragColor;

layout (location = 0) in VS_OUT
{
  vec3 wPos;
  vec3 wNorm;
  vec3 wTangent;
  vec2 texCoord;
  
  vec4 prevClipPos;
  vec4 currentClipPos; 
} vOut;

void main()
{
  vec4 prevClipPos = vOut.prevClipPos;
  vec4 currentClipPos = vOut.currentClipPos;
  
  prevClipPos /= prevClipPos.w;
  prevClipPos.xy = (prevClipPos.xy + 1.0f) / 2.0f;

  currentClipPos /= currentClipPos.w;
  currentClipPos.xy = (currentClipPos.xy + 1.0f) / 2.0f;

  out_fragColor = (currentClipPos - prevClipPos).xy;
}
