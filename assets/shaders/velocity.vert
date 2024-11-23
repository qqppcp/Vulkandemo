#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

#extension GL_GOOGLE_include_directive : require
#include "CommonStructs.glsl"
#include "IndirectCommon.glsl"

layout(location = 0) out vec4 outClipSpacePos;
layout(location = 1) out vec4 outPrevClipSpacePos;

void main()
{
    Vertex vertex = vertexAlias[VERTEX_INDEX]
                      .vertices[/*gl_InstanceIndex + */ gl_VertexIndex];

    vec3 position = vec3(vertex.posX, vertex.posY, vertex.posZ);

    outClipSpacePos = MVP.projection * MVP.view * MVP.model * vec4(position, 1.0);
    outPrevClipSpacePos = MVP.projection * MVP.prevView * MVP.model * vec4(position, 1.0);
    gl_Position = outClipSpacePos;
}