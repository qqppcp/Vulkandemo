#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable
// we could use vec3 etc but that can cause alignment issue, so we prefer float
struct Vertex {
  float posX;
  float posY;
  float posZ;
  float normalX;
  float normalY;
  float normalZ;
  float uvX;
  float uvY;
  float tangentX;
  float tangentY;
  float tangentZ;
  float bitangentX;
  float bitangentY;
  float bitangentZ;
  int materialID;
};

layout(push_constant) uniform Camera {
    mat4 view;
    mat4 proj;
};

layout(set = 1, binding = 0) readonly buffer VertexBuffer {
  Vertex vertices[];
}
vertexBuffer;

layout(location = 0) out vec3 WorldPos;

void main()
{
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    WorldPos = vec3(vertex.posX, vertex.posY, vertex.posZ);
    gl_Position =  proj * view * vec4(WorldPos, 1.0);
}