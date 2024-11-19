#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

#extension GL_GOOGLE_include_directive : require
#include "CommonStructs.glsl"
#include "IndirectCommon.glsl"

struct Flow
{
  vec2 texCoord;
  vec3 position;
  vec3 normal;
  vec4 tangent;
};
layout (location = 0) out Flow flow;
layout (location = 4) out flat int omaterialID;

void main()
{
    Vertex vertex = vertexAlias[VERTEX_INDEX]
                      .vertices[/*gl_InstanceIndex + */ gl_VertexIndex];

    flow.texCoord = vec2(vertex.uvX, vertex.uvY);
    vec3 position = vec3(vertex.posX, vertex.posY, vertex.posZ);
    flow.position  = (MVP.model * vec4(position, 1.0)).xyz;
    flow.normal = vec3(vertex.normalX, vertex.normalY, vertex.normalZ);
    omaterialID = vertex.material;
    flow.tangent = vec4(vertex.tangentX, vertex.tangentY, vertex.tangentZ, vertex.tangentW);

    //flow.normal = mat3(transpose(inverse(model))) * vec3(vertex.normalX, vertex.normalY, vertex.normalZ);
    vec4 pos = MVP.projection * MVP.view * MVP.model * vec4(position, 1.0);
    
    gl_Position = pos;
}