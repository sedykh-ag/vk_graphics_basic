#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable

#include "common.h"

layout(triangles) in;
layout(triangle_strip, max_vertices=4) out;

layout(push_constant) uniform params_t
{
    mat4 mProjView;
    mat4 mModel;
} params;

layout(binding = 0, set = 0) uniform AppData
{
  UniformParams UBO;
};

layout(location = 0) in GS_IN
{
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
} gIn[];

struct GS_OUT {
    vec3 wPos;
    vec3 wNorm;
    vec3 wTangent;
    vec2 texCoord;
};

layout(location = 0) out GS_OUT gOut;

void main()
{
    GS_OUT base_vertices[3];
    for (int i = 0; i < 3; ++i)
    {
        GS_OUT vertex;
        vertex.wNorm = gIn[i].wNorm;
        vertex.wPos = gIn[i].wPos;
        vertex.wTangent = gIn[i].wTangent;
        vertex.texCoord = gIn[i].texCoord;

        base_vertices[i] = vertex;
    }


    vec3 newNormal = (gIn[0].wNorm + gIn[1].wNorm) / 2.0f;

    vec3 k = vec3(1.0f, 1.0f, 1.0f);
    float w = 2.0f;
    vec3 newPos = (gIn[0].wPos + gIn[1].wPos) / 2.0f;
    newPos += 0.1f * newNormal * max(0.0f, sin(dot(k, newPos) - w * UBO.time));

    vec3 newTangent = (gIn[0].wTangent + gIn[1].wTangent) / 2.0f;
    vec2 newTexCoord = (gIn[0].texCoord + gIn[1].texCoord) / 2.0f;

    GS_OUT newVertex;
    newVertex.wNorm = newNormal;
    newVertex.wPos = newPos;
    newVertex.wTangent = newTangent;
    newVertex.texCoord = newTexCoord;

    GS_OUT vertices[] = { base_vertices[0], newVertex, base_vertices[2], base_vertices[1] };
    int vertices_count = 4;

    for (int i = 0; i < vertices_count; ++i)
    {
        gOut.wPos     = (params.mModel * vec4(vertices[i].wPos.xyz, 1.0f)).xyz;
        gOut.wNorm    = normalize(mat3(transpose(inverse(params.mModel))) * vertices[i].wNorm.xyz);
        gOut.wTangent = normalize(mat3(transpose(inverse(params.mModel))) * vertices[i].wTangent.xyz);
        gOut.texCoord = vertices[i].texCoord;

        gl_Position = params.mProjView * vec4(gOut.wPos, 1.0);

        EmitVertex();
    }

    EndPrimitive();
}