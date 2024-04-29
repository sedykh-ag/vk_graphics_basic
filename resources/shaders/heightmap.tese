#version 450

layout(push_constant) uniform params_t
{
  mat4 mProjView;
  mat4 mModel;
  float tessLevel;
  float minHeight;
  float maxHeight;
} params;

struct TS
{
  vec3 wPos;
  vec3 wNorm;
};

layout(quads, equal_spacing, ccw) in;

layout (location = 0) out TS tOut;

layout (binding = 2) uniform sampler2D displacementMap; 

vec3 getDisplacementNormal()
{
  const float EPS = 1e-3;
  float f_x0 = textureLod(displacementMap, gl_TessCoord.xy + vec2(-EPS, 0.0), 0.0).r;
  float f_x1 = textureLod(displacementMap, gl_TessCoord.xy + vec2(+EPS, 0.0), 0.0).r;

  float f_y0 = textureLod(displacementMap, gl_TessCoord.xy + vec2(0.0, -EPS), 0.0).r;
  float f_y1 = textureLod(displacementMap, gl_TessCoord.xy + vec2(0.0, +EPS), 0.0).r;

  vec3 dfdx = vec3(2.0 * EPS, 0.0, f_x1 - f_x0);
  vec3 dfdy = vec3(0.0, 2.0 * EPS, f_y1 - f_y0);
  vec3 normal = normalize(cross(dfdx, dfdy));
  return normal;
}

void main()
{
  float height = (textureLod(displacementMap, gl_TessCoord.st, 0.0).r - 0.5) * 2.0;
  if (height < 0.0)
    height *= -params.minHeight;
  else
    height *= params.maxHeight;

  height = clamp(height, params.minHeight, params.maxHeight);
  tOut.wPos = vec3(gl_TessCoord.xy, height);
  tOut.wNorm = getDisplacementNormal();

  // model transform
  tOut.wPos = (params.mModel * vec4(tOut.wPos, 1.0)).xyz;
  tOut.wNorm = normalize(mat3(transpose(inverse(params.mModel))) * tOut.wNorm);

  // view projection transform
  gl_Position = params.mProjView * vec4(tOut.wPos, 1.0);
}
