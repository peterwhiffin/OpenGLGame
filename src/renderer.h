#pragma once
#include <vector>
#include "forward.h"
#include "utils/mathutils.h"
#include "physics.h"

struct DebugVertex {
    vec3 pos;
    vec4 color;
};

struct DebugLine {
    vec3 start;
    vec3 end;
    JPH::Color color;
};

struct DebugTri {
    vec3 v0, v1, v2;
    JPH::Color color;
};

struct Vertex {
    vec4 position;
    vec3 normal;
    vec3 tangent;
    vec2 texCoord;
    GLint boneIDs[4];
    float weights[4];
};

struct Texture {
    std::string path;
    std::string name;
    GLuint id;
};

struct Shader {
    uint32_t id;
    std::string name;
};

struct Material {
    GLint shader;  // this will probably need to be a shader struct eventually
    std::string name;
    std::vector<Texture*> textures;
    vec4 baseColor;
    glm::vec2 textureTiling = glm::vec2(1.0f, 1.0f);
    float roughness = 1.0f;
    float metalness = 1.0f;
    float aoStrength = 1.0f;
    float normalStrength = 1.0f;
};

struct SubMesh {
    GLsizei indexOffset;
    GLsizei indexCount;
    Material* material;
};

struct BoneInfo {
    uint32_t id;
    mat4 offset;
};

struct Mesh {
    std::string name;
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;
    mat4 globalInverseTransform;
    vec3 center;
    vec3 extent;
    vec3 min;
    vec3 max;
    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    std::vector<SubMesh> subMeshes;
    std::unordered_map<std::string, BoneInfo> boneNameMap;
};

struct MeshRenderer {
    uint32_t entityID;
    uint32_t rootEntity;
    GLint vao;
    Mesh* mesh;
    std::vector<Material*> materials;
    std::vector<SubMesh> subMeshes;
    std::vector<mat4> boneMatrices;
    std::unordered_map<uint32_t, BoneInfo> transformBoneMap;
};

struct DirectionalLight {
    uint32_t entityID;
    vec3 position;
    vec3 lookDirection;
    vec3 color;
    vec3 brightness;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float ambientBrightness;
    float diffuseBrightness;

    bool isEnabled;
};

struct PointLight {
    uint32_t entityID;
    vec3 color;
    float brightness;
    bool isActive;
};

struct SpotLight {
    uint32_t entityID;
    GLuint depthFrameBuffer, blurDepthFrameBuffer, depthTex, blurDepthTex;
    GLsizei shadowWidth = 800;
    GLsizei shadowHeight = 600;
    mat4 lightSpaceMatrix = mat4::sIdentity();
    vec3 color;
    float brightness;
    float cutoff;
    float outerCutoff;
    float lightRadiusUV = 0.005f;
    float blockerSearchUV = 0.0025f;
    float range = 20.0f;
    bool enableShadows = true;
    bool isActive;
};

struct WindowData {
    GLsizei width = 800;
    GLsizei height = 600;
    GLsizei viewportWidth = 800;
    GLsizei viewportHeight = 600;
};

struct GlobalUBO {
    mat4 view = mat4::sIdentity();
    mat4 projection = mat4::sIdentity();
};

struct RenderState {
    GLFWwindow* window;
    WindowData windowData;
    GLuint litFBO, litRBO, ssaoFBO;
    GLuint litColorTex, bloomSSAOTex, blurTex, ssaoNoiseTex, ssaoPosTex, ssaoNormalTex;
    GLuint blurFBO[2], blurSwapTex[2];
    GLuint fullscreenVAO, fullscreenVBO;
    GLuint lightingShader, postProcessShader, blurShader, simpleBlurShader, depthShader, ssaoShader, shadowBlurShader, debugShader;

    GLuint pickingFBO;
    GLuint pickingRBO;
    GLuint pickingShader;
    GLuint pickingTex;
    GLuint editorFBO, editorRBO, editorTex;

    GLuint matricesUBO;
    GlobalUBO matricesUBOData;

    JPH::DebugRendererSimple* debugRenderer;

    float exposure = 1.0f;
    float bloomThreshold = 0.39f;
    float bloomAmount = 0.1f;
    float ambient = 0.004f;
    float AORadius = 0.5f;
    float AOBias = 0.025f;
    float AOAmount = 1.0f;
    float AOPower = 2.0f;

    bool horizontalBlur = true;

    std::vector<vec3> ssaoKernel;
    std::vector<vec3> ssaoNoise;
};

void createContext(Scene* scene, RenderState* renderer);
void initRenderer(RenderState* renderer, Scene* scene, EditorState* editor);
void mapBones(Scene* scene, MeshRenderer* renderer);
void renderScene(RenderState* renderer, Scene* scene, EditorState* editor);
void deleteBuffers(RenderState* scene, Resources* resources);
void createSpotLightShadowMap(Scene* scene, SpotLight* light);
void createSpotLightShadowMapHDRedux(Scene* scene, SpotLight* light);

class MyDebugRenderer : public JPH::DebugRendererSimple {
   public:
    std::vector<DebugLine> lines;
    std::vector<DebugTri> triangles;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
    void DrawTriangle(JPH::RVec3Arg inV1, const JPH::RVec3Arg inV2, const JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override;
    void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;
    void Clear();
};
