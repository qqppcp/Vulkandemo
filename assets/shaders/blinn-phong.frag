#version 460 core
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#include "CommonStructs.glsl"
#include "IndirectCommon.glsl"

layout(location = 0) out vec4 FragColor;

layout(set = 4, binding = 0) uniform Transform {
  mat4 viewProj;
  mat4 viewProjInv;
  mat4 viewInv;
}
cameraData;

layout(set = 4, binding = 1) uniform Lights {
  vec4 lightPos;
  vec4 lightDir;
  vec4 lightColor;
  vec4 ambientColor;  // environment light color
  mat4 lightVP;
  float innerConeAngle;
  float outerConeAngle;
}
lightData;

struct Flow
{
  vec2 texCoord;
  vec3 position;
  vec3 normal;
  vec4 tangent;
};

layout (location = 0) in Flow flow;
layout (location = 4) in flat int materialID;

void main()
{
    MaterialData material = materialDataAlias[MATERIAL_DATA_INDEX].materials[materialID];
    vec3 normal = normalize(flow.normal);
    if (material.normalTextureId != -1)
    {
        normal = texture(
        sampler2D(BindlessImage2D[material.normalTextureId], BindlessSampler[0]),
        flow.texCoord).rgb;
        normal = normalize(normal * 2.0 - 1.0);

        const vec3 n = normalize(flow.normal);
        const vec3 t = normalize(flow.tangent.xyz);
        const vec3 b = normalize(cross(n, t) * flow.tangent.w);
        const mat3 TBN = mat3(t, b, n);
        normal = normalize(TBN * normal);
    }
 
    
    vec3 camPos = cameraData.viewInv[3].xyz;
    vec3 lightDir = normalize(lightData.lightPos.xyz - flow.position);
    vec3 diffuseColor = vec3(0);
    if (material.diffuseTextureId != -1)
    {
        diffuseColor = texture(sampler2D(BindlessImage2D[material.diffuseTextureId],                                            BindlessSampler[0]), flow.texCoord).rgb;
    }
    vec3 ambient = 0.1f * diffuseColor;

    float costheta = max(dot(normal, lightDir), 0.0);
    vec3 kd = material.Kd_dissolve.rgb;
    vec3 diffuse = kd * costheta * diffuseColor * lightData.lightColor.rgb;

    vec3 specular = vec3(0);
    if (material.specularTextureId != -1)
    {
        vec3 viewDir = normalize(camPos - flow.position);
        float shiness = material.Ks_shininess.a;
        vec3 ks = material.Ks_shininess.rgb;
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(normal, halfDir), 0.0), shiness);
        specular = ks * spec * texture(sampler2D(BindlessImage2D[material.specularTextureId],                                            BindlessSampler[0]), flow.texCoord).rgb * lightData.lightColor.rgb;
    }
    
    FragColor = vec4(ambient + diffuse + specular, 1.0);
}