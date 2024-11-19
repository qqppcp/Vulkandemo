#version 460

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
  float tangentX;
  float tangentY;
  float tangentZ;
  float tangentW;
  float uvX;
  float uvY;
  float uvX2;
  float uvY2;
  int materialID;
};

layout(push_constant) uniform Camera {
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(set = 0, binding = 0) readonly buffer VertexBuffer {
  Vertex vertices[];
}
vertexBuffer;

layout(set = 0, binding = 1) readonly buffer IndexBuffer {
  uint indices[];
}
indexBuffer;

void main()
{
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    vec3 position = vec3(vertex.posX, vertex.posY, vertex.posZ);
    vec4 pos = proj * view * model * vec4(position, 1.0);
    
    gl_Position = pos;
}