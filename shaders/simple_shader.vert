#version 450

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 vertexNorm;
layout (location = 2) in vec2 aTextCoord;

layout(push_constant) uniform MyPushConstants {
    mat4 uModel;
    int uUseTexture;
    vec4 u_color;
} pushConst;

layout(set = 0, binding = 0) uniform SceneBlock {
    mat4 uViewProjection;
    mat4 uWorld;
    mat4 uWorldInverseTranspose;
    vec3 uViewerWorldPosition;
} scene;

layout(location = 0) out vec3 vNormal;
layout(location = 1) out vec3 surfaceWorldPosition;
layout(location = 2) out vec3 vSurfaceToViewer;
layout(location = 3) out vec2 vTextCoord;

void main() {
  vTextCoord = aTextCoord;
  gl_Position = scene.uViewProjection * pushConst.uModel * vec4(aPosition, 1.0);

  vNormal = mat3(scene.uWorldInverseTranspose) * vertexNorm;

  surfaceWorldPosition = mat3(scene.uWorld) * aPosition;
  vSurfaceToViewer = scene.uViewerWorldPosition - surfaceWorldPosition;
}