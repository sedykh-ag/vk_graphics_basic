#version 450

void main(void)
{
  vec3 localPos = vec3(0.0, 0.0, 0.0);
  
  if (gl_VertexIndex == 0)
    localPos = vec3(0.0, 0.0, 0.0);
  else if (gl_VertexIndex == 1)
    localPos = vec3(1.0, 0.0, 0.0);
  else if (gl_VertexIndex == 2)
    localPos = vec3(1.0, 1.0, 0.0);
  else if (gl_VertexIndex == 3)
    localPos = vec3(0.0, 1.0, 0.0);

  gl_Position = vec4(localPos, 1.0);
}