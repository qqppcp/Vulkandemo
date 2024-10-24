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
    mat4 model;
    mat4 view;
    mat4 proj;
};

struct Flow
{
  vec2 texCoord;
  vec3 position;
  vec3 normal;
  mat3 TBN;
};
layout (location = 0) out Flow flow;
layout (location = 6) out flat int omaterialID;

layout(set = 2, binding = 0) readonly buffer VertexBuffer {
  Vertex vertices[];
}
vertexBuffer;

layout(set = 2, binding = 1) readonly buffer IndexBuffer {
  uint indices[];
}
indexBuffer;

void main()
{
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];
    flow.texCoord = vec2(vertex.uvX, vertex.uvY);
    vec3 position = vec3(vertex.posX, vertex.posY, vertex.posZ);
    flow.position  = (model * vec4(position, 1.0)).xyz;
    flow.normal = mat3(transpose(inverse(model))) * vec3(vertex.normalX, vertex.normalY, vertex.normalZ);
    omaterialID = vertex.materialID;
    vec3 normal = vec3(vertex.normalX, vertex.normalY, vertex.normalZ);
    vec3 tangent = vec3(vertex.tangentX, vertex.tangentY, vertex.tangentZ);
    vec3 bitangent = vec3(vertex.bitangentX, vertex.bitangentY, vertex.bitangentZ);

    vec3 T = normalize(vec3(model * vec4(tangent,   0.0)));
    vec3 B = normalize(vec3(model * vec4(bitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(normal,    0.0)));
    flow.TBN = mat3(T, B, N);

    //flow.normal = mat3(transpose(inverse(model))) * vec3(vertex.normalX, vertex.normalY, vertex.normalZ);
    vec4 pos = proj * view * model * vec4(position, 1.0);
    
    gl_Position = pos;
}