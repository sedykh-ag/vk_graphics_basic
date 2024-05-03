#version 450

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outColor;
layout (binding = 0) uniform sampler2D inColorTex;

layout(push_constant) uniform params_t
{
  int tonemappingType;
  float exposure;
} params;

const float gamma = 2.2;

vec3 reinhard(vec3 hdrColor)
{
  // reinhard tone mapping
  vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
  return mapped;
}

vec3 exposure(vec3 hdrColor)
{
  // exposure tone mapping
  vec3 mapped = vec3(1.0) - exp(-hdrColor * params.exposure);
  return mapped;
}

// Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
vec3 aces(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
  const vec3 hdrColor = textureLod(inColorTex, inUV, 0.0).rgb;
  vec3 ldrColor;

  if (params.tonemappingType == 0)
    ldrColor = hdrColor;
  else if (params.tonemappingType == 1)
    ldrColor = reinhard(hdrColor);
  else if (params.tonemappingType == 2)
    ldrColor = exposure(hdrColor);
  else if (params.tonemappingType == 3)
    ldrColor = aces(hdrColor);
  
  // gamma correction
  // ldrColor = pow(ldrColor, vec3(1.0 / gamma));

  outColor = vec4(ldrColor, 1.0);
}
