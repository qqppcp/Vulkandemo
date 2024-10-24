#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 0) uniform UBO {
    vec4 color;
} ubo;

struct MaterialData {
	vec4 Ka_illum;
	vec4 Kd_dissolve;
	vec4 Ks_shininess;
	vec4 emission_ior;
    int diffuseTextureId;
	int normalTextureId;
	int specularTextureId;
	int reflectTextureId;
};

struct Flow
{
  vec2 texCoord;
  vec3 position;
  vec3 normal;
  mat3 TBN;
};

layout (location = 0) in Flow flow;
layout (location = 6) in flat int materialID;

layout(set = 1, binding = 0) uniform sampler2D image[];
layout(set = 3, binding = 0) readonly buffer MaterialBufferForAllMesh {
  MaterialData materials[];
};

layout(push_constant) uniform Camera {
    layout(offset = 192) vec3 viewPos;
};

vec3 lightPos = vec3(10, 10, 10);

void main()
{
    MaterialData material = materials[materialID];
    vec3 normal = flow.normal;
    if (material.normalTextureId != -1)
    {
        normal = texture(image[material.normalTextureId], flow.texCoord).rgb;
    }
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(flow.TBN * normal);

    vec3 lightDir = normalize(lightPos - flow.position);
    vec3 diffuseColor = vec3(0);
    if (material.diffuseTextureId != -1)
    {
        diffuseColor = texture(image[material.diffuseTextureId], flow.texCoord).rgb;
    }
    vec3 ambient = 0.1f * diffuseColor;

    float costheta = max(dot(normal, lightDir), 0.0);
    vec3 kd = material.Kd_dissolve.rgb;
    vec3 diffuse = kd * costheta * diffuseColor * ubo.color.rgb;

    vec3 specular = vec3(0);
    if (material.specularTextureId != -1)
    {
        vec3 viewDir = normalize(viewPos - flow.position);
        float shiness = material.Ks_shininess.a;
        vec3 ks = material.Ks_shininess.rgb;
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), shiness);
        specular = ks * spec * texture(image[material.specularTextureId], flow.texCoord).rgb * ubo.color.rgb;
    }
    
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}