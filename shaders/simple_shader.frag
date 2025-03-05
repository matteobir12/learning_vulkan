#version 450

layout(location = 0) in vec3 vNormal;
layout(location = 1) in vec3 surfaceWorldPosition;
layout(location = 2) in vec3 vSurfaceToViewer;
layout(location = 3) in vec2 vTexCoord;


layout(set = 0, binding = 1) uniform LightBlock {
    vec3 uLightPosition[3];
    vec3 uLightDirection[3];
    vec3 uLightColor[3]; // To use
    bool uLightIsOn[3];
    bool uLightIsDirectional[3];
    float uShininess;
    float lightCutoff;
    vec3 ambientLight;
    vec3 specColor;
} lights;

layout(set = 1, binding = 0) uniform sampler2D uTexture;

layout(push_constant) uniform MyPushConstants {
    mat4 uModel;
    int uUseTexture;
    vec4 u_color;
} pushConst;

layout(location = 0) out vec4 outColor;

void main() {
  vec4 color = pushConst.u_color;
  if (pushConst.uUseTexture != 0) {
    color = texture(uTexture, vTexCoord);
  }

  vec3 diffuse = vec3(0.0);
  vec3 specular = vec3(0.0);
  vec3 ambient = lights.ambientLight * color.xyz;
  vec3 normal = normalize(vNormal);
  
  for(int i = 0; i < 3; i++) {
    if (!lights.uLightIsOn[i]) {
      continue;
    }

    vec3 lightDirection = normalize(lights.uLightPosition[i] - surfaceWorldPosition);
    vec3 halfVector = normalize(lightDirection + normalize(vSurfaceToViewer));
    if (lights.uLightIsDirectional[i]) {
      float dp = dot(lightDirection, normalize(lights.uLightDirection[i]));
      if ( dp >= lights.lightCutoff) {
        diffuse += max(dot(normal, normalize(lights.uLightDirection[i])), 0.0) * color.xyz;
        float s = max(dot(normal, halfVector), 0.0);
        float specIntense = pow(s, lights.uShininess);
        specular += s * specIntense * lights.specColor;
      }
    } else {
      diffuse += max(dot(normal, normalize(lightDirection)), 0.0) * color.xyz;
      float s = max(dot(normal, halfVector), 0.0);
      float specIntense = pow(s, lights.uShininess);
      specular += s * specIntense * lights.specColor;
    }
  }

  //outColor = vec4(normal,1.0);
  outColor = vec4(diffuse + specular + ambient, color.w);
}
