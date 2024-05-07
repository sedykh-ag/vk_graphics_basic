#version 450

layout(location = 0) in vec2 tex;
layout(location = 0) out vec4 color;

layout(binding = 0) uniform sampler2D colorMap;

const float gamma = 2.2;

void main()
{
  color = texture(colorMap, tex);
  color.rgb = pow ( color.rgb, vec3 ( 1.0 / gamma ) );
}
