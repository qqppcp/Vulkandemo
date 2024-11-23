#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "CommonStructs.glsl"
#include "IndirectCommon.glsl"

layout(location = 0) out vec2 outgBufferVelocity;

layout(location = 0) in vec4 inClipSpacePos;
layout(location = 1) in vec4 inPrevClipSpacePos;

void main()
{
    vec2 a = (inClipSpacePos.xy / inClipSpacePos.w);
    a = (a + 1.0f) / 2.0f;
    a.y = 1.0 - a.y;
    vec2 b = (inPrevClipSpacePos.xy / inPrevClipSpacePos.w);
    b = (b + 1.0f) / 2.0f;
    b.y = 1.0 - b.y;
    outgBufferVelocity = (a - b);
}