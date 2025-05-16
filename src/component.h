#pragma once
#include <glad/glad.h>
#include <glfw/glfw3.h>
/* #include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/quaternion_float.hpp> */
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <string_view>
/* #include <glm/gtx/string_cast.hpp> */

#include <Jolt/Jolt.h>
#include <Jolt/Math/Float2.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Renderer/DebugRendererSimple.h>
#include <glm/vec2.hpp>
#include <glm/gtc/type_ptr.hpp>

JPH_SUPPRESS_WARNINGS
constexpr uint32_t INVALID_ID = 0xFFFFFFFF;
using namespace JPH::literals;
using mat4 = JPH::Mat44;
using vec2 = JPH::Float2;
using vec3 = JPH::Vec3;
using vec4 = JPH::Vec4;
using quat = JPH::Quat;

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
    std::vector<Texture> textures;
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

struct KeyFramePosition {
    vec3 position;
    float time;
};

struct KeyFrameRotation {
    quat rotation;
    float time;
};

struct KeyFrameScale {
    vec3 scale;
    float time;
};

struct AnimationChannel {
    std::string name;
    std::vector<KeyFramePosition> positions;
    std::vector<KeyFrameRotation> rotations;
    std::vector<KeyFrameScale> scales;
    uint32_t nextPositionKey = 0;
    uint32_t nextRotationKey = 0;
    uint32_t nextScaleKey = 0;
};

struct Animation {
    std::string name;
    float duration;
    std::vector<AnimationChannel*> channels;
};

struct ModelNode {
    std::string name;
    ModelNode* parent;
    Mesh* mesh;
    mat4 transform;
    mat4 localTransform;
    std::vector<ModelNode*> children;
};

struct Model {
    std::string name;
    ModelNode* rootNode;
    mat4 RootNodeTransform;
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;
    std::vector<Animation*> animations;
    std::unordered_map<ModelNode*, AnimationChannel*> channelMap;
};
struct Entity {
    uint32_t entityID;
    std::string name;
    bool isActive;
};

struct Transform {
    uint32_t entityID;
    uint32_t parentEntityID = INVALID_ID;
    vec3 localPosition = vec3(0.0f, 0.0f, 0.0f);
    quat localRotation = quat(0.0f, 0.0f, 0.0f, 1.0f);
    vec3 localScale = vec3(1.0f, 1.0f, 1.0f);
    mat4 worldTransform = mat4::sIdentity();
    std::vector<uint32_t> childEntityIds;
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

struct RigidBody {
    uint32_t entityID;
    JPH::BodyID joltBody;
    vec3 lastPosition = vec3(0.0f, 0.0f, 0.0f);
    quat lastRotation = quat(0.0f, 0.0f, 0.0f, 1.0f);
    bool rotationLocked = false;
};

struct Animator {
    uint32_t entityID;
    uint32_t currentIndex = 0;
    float playbackTime = 0.0f;
    Animation* currentAnimation = nullptr;
    std::vector<Animation*> animations;
    std::unordered_map<std::string, Animation*> animationMap;
    std::unordered_map<AnimationChannel*, uint32_t> channelMap;
};

struct Camera {
    uint32_t entityID;
    float fov;
    float fovRadians;
    float aspectRatio;
    float nearPlane;
    float farPlane;
};

struct CameraController {
    uint32_t entityID;
    uint32_t cameraTargetEntityID;
    uint32_t cameraEntityID;
    Camera* camera;
    float pitch = 0;
    float yaw = 0;
    float sensitivity = .3;
    float moveSpeed = 10;
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
    bool enableShadows = true;
    bool isActive;
};

struct WindowData {
    GLsizei width = 800;
    GLsizei height = 600;
    GLsizei viewportWidth = 800;
    GLsizei viewportHeight = 600;
};

struct Player {
    uint32_t entityID;
    CameraController* cameraController;
    uint32_t armsID;
    bool isGrounded = false;
    bool canJump = true;
    bool canSpawnCan = true;
    float jumpHeight = 10.0f;
    float moveSpeed = 10.0f;
    float groundCheckDistance = 0.2f;
};

struct GlobalUBO {
    mat4 view = mat4::sIdentity();
    mat4 projection = mat4::sIdentity();
};

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

class MyDebugRenderer : public JPH::DebugRendererSimple {
   public:
    std::vector<DebugLine> lines;
    std::vector<DebugTri> triangles;

    void DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override {
        lines.push_back({inFrom,
                         inTo,
                         inColor});
    }

    void DrawTriangle(JPH::RVec3Arg inV1, const JPH::RVec3Arg inV2, const JPH::RVec3Arg inV3, JPH::ColorArg inColor, ECastShadow inCastShadow) override {
        triangles.push_back({inV1,
                             inV2,
                             inV3,
                             inColor});
    }

    virtual void DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override {
        // Implement
    }

    void Clear() {
        lines.clear();
        triangles.clear();
    }
};

struct Scene {
    std::string name = "../data/scenes/default.scene";
    WindowData windowData;
    MyDebugRenderer* debugRenderer;
    JPH::BodyInterface* bodyInterface;
    JPH::PhysicsSystem* physicsSystem;
    uint32_t trashCanEntity;
    GLuint pickingFBO, pickingRBO, litFBO, litRBO, ssaoFBO;
    GLuint pickingTex, litColorTex, bloomSSAOTex, blurTex, ssaoNoiseTex, ssaoPosTex, ssaoNormalTex;
    GLuint blurFBO[2], blurSwapTex[2];
    GLuint fullscreenVAO, fullscreenVBO;
    GLuint editorFBO, editorRBO, editorTex;
    GLuint lightingShader, postProcessShader, blurShader, depthShader, ssaoShader, pickingShader, shadowBlurShader, debugShader;
    uint32_t nodeClicked = INVALID_ID;

    const float cDeltaTime = 1.0f / 60.0f;
    float FPS = 0.0f;
    float frameTime = 0.0f;
    float timeAccum = 0.0f;
    float physicsAccum = 0.0f;
    float frameCount = 0;
    float currentFrame = 0.0f;
    float lastFrame = 0.0f;
    float deltaTime;
    float gravity = -18.81f;
    float normalStrength = 1.06f;
    float exposure = 1.0f;
    float bloomThreshold = 0.39f;
    float bloomAmount = 0.1f;
    float ambient = 0.004f;
    float AORadius = 0.5f;
    float AOBias = 0.025f;
    float AOAmount = 1.0f;
    float AOPower = 2.0f;

    bool menuOpen = false;
    bool menuCanOpen = true;
    bool useDeferred = false;
    bool horizontalBlur = true;
    bool isPicking = false;
    bool canPick = true;
    bool physicsTicked = false;

    GLuint matricesUBO;
    GlobalUBO matricesUBOData;
    uint32_t nextEntityID = 1;
    vec3 wrenchOffset = vec3(0.0f, -0.42f, 0.37f);

    Model* trashcanModel;
    Model* testRoom;
    Model* wrench;
    Model* arms;
    Model* wrenchArms;
    Player* player;

    DirectionalLight sun;
    // std::unordered_map<uint32_t, uint32_t> usedIds;
    std::vector<Model*> models;
    std::vector<vec3> ssaoKernel;
    std::vector<vec3> ssaoNoise;

    std::vector<Entity> entities;
    std::vector<Transform> transforms;
    std::vector<MeshRenderer> meshRenderers;
    std::vector<Animator> animators;
    std::vector<RigidBody> rigidbodies;
    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<Camera*> cameras;

    std::unordered_map<Material*, std::vector<uint32_t>> renderMap;

    std::unordered_map<uint32_t, size_t> renderGroupIndexMap;

    std::unordered_map<uint32_t, size_t> entityIndexMap;
    std::unordered_map<uint32_t, size_t> transformIndexMap;
    std::unordered_map<uint32_t, size_t> meshRendererIndexMap;
    std::unordered_map<uint32_t, size_t> rigidbodyIndexMap;
    std::unordered_map<uint32_t, size_t> animatorIndexMap;
    std::unordered_map<uint32_t, size_t> pointLightIndexMap;
    std::unordered_map<uint32_t, size_t> spotLightIndexMap;

    std::vector<Texture> textures;
    std::unordered_map<std::string, Mesh*> meshMap;
    std::unordered_map<std::string, Animation*> animationMap;
    std::unordered_map<std::string, Material*> materialMap;
    std::unordered_map<std::string, Texture> textureMap;
};

uint32_t getEntityID(Scene* scene);
Transform* addTransform(Scene* scene, uint32_t entityID);
Entity* getNewEntity(Scene* scene, std::string name, uint32_t id = -1, bool createTransform = true);
MeshRenderer* addMeshRenderer(Scene* scene, uint32_t entityID);
RigidBody* addRigidbody(Scene* scene, uint32_t entityID);
Animator* addAnimator(Scene* scene, uint32_t entityID, Model* model);
Animator* addAnimator(Scene* scene, uint32_t entityID, std::vector<Animation*> animations);
uint32_t createEntityFromModel(Scene* scene, ModelNode* node, uint32_t parentEntityID, bool addColliders, uint32_t rootEntity, bool first, bool isDynamic);
Camera* addCamera(Scene* scene, uint32_t entityID, float fov, float aspectRatio, float nearPlane, float farPlane);
PointLight* addPointLight(Scene* scene, uint32_t entityID);
SpotLight* addSpotLight(Scene* scene, uint32_t entityID);

void destroyEntity(Scene* scene, uint32_t entityID);
void mapBones(Scene* scene, MeshRenderer* renderer);

Entity* getEntity(Scene* scene, uint32_t entityID);
Transform* getTransform(Scene* scene, uint32_t entityID);
MeshRenderer* getMeshRenderer(Scene* scene, uint32_t entityID);
RigidBody* getRigidbody(Scene* scene, uint32_t entityID);
Animator* getAnimator(Scene* scene, uint32_t entityID);
PointLight* getPointLight(Scene* scene, uint32_t entityID);
SpotLight* getSpotLight(Scene* scene, uint32_t entityID);
Camera* getCamera(Scene* scene, uint32_t entityID);

vec3 lerp(const vec3 a, const vec3 b, float t);

template <typename Component>
bool destroyComponent(std::vector<Component>& components, std::unordered_map<uint32_t, size_t>& indexMap, uint32_t entityID) {
    if (!indexMap.count(entityID)) {
        return false;
    }

    size_t indexToRemove = indexMap[entityID];
    size_t lastIndex = components.size() - 1;

    if (indexToRemove != lastIndex) {
        uint32_t lastID = components[lastIndex].entityID;
        std::swap(components[lastIndex], components[indexToRemove]);
        indexMap[lastID] = indexToRemove;
    }

    components.pop_back();
    indexMap.erase(entityID);
    return true;
}