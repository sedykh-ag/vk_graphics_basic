#version 450

layout(push_constant) uniform params_t
{
  mat4 mProjView;
  mat4 mModel;
  float tessLevel;
  float minHeight;
  float maxHeight;
} params;

layout (vertices = 4) out;

void main()
{
  if (gl_InvocationID == 0)
  {
    gl_TessLevelInner[0] = params.tessLevel;
    gl_TessLevelInner[1] = params.tessLevel;

    gl_TessLevelOuter[0] = params.tessLevel;
    gl_TessLevelOuter[1] = params.tessLevel;
    gl_TessLevelOuter[2] = params.tessLevel;
    gl_TessLevelOuter[3] = params.tessLevel;
  }

	gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;
}