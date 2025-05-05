#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <glm/gtx/string_cast.hpp>

constexpr uint32_t INVALID_ID = 0xFFFFFFFF;

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec2 texCoord;
};

struct Texture {
    std::string path;
    unsigned int id;
};

struct Shader {
    uint32_t id;
    std::string name;
};

struct Material {
    unsigned int shader;
    std::string name;
    std::vector<Texture> textures;
    glm::vec4 baseColor;
    float shininess;
};

struct SubMesh {
    unsigned int indexOffset;
    unsigned int indexCount;
    Material material;
};

struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<SubMesh> subMeshes;
    glm::vec3 center;
    glm::vec3 extent;
    glm::vec3 min;
    glm::vec3 max;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int EBO;
};

struct KeyFramePosition {
    glm::vec3 position;
    float time;
};

struct KeyFrameRotation {
    glm::quat rotation;
    float time;
};

struct KeyFrameScale {
    glm::vec3 scale;
    float time;
};

struct AnimationChannel {
    std::string name;
    std::vector<KeyFramePosition> positions;
    std::vector<KeyFrameRotation> rotations;
    std::vector<KeyFrameScale> scales;
};

struct Animation {
    std::string name;
    float duration;
    std::vector<AnimationChannel*> channels;
};

struct ModelNode {
    std::string name;
    ModelNode* parent;
    glm::mat4 transform;
    std::vector<ModelNode*> children;
    Mesh* mesh;
};

struct Model {
    std::string name;
    std::vector<Mesh*> meshes;
    std::vector<Material*> materials;
    std::vector<Animation*> animations;
    std::unordered_map<ModelNode*, AnimationChannel*> channelMap;
    ModelNode* rootNode;
};
struct Entity {
    uint32_t entityID;
    std::string name;
    bool isActive;
};

struct Transform {
    uint32_t entityID;
    uint32_t parentEntityID = INVALID_ID;
    std::vector<uint32_t> childEntityIds;
    glm::vec3 localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::mat4 worldTransform = glm::mat4(1.0f);
};

struct MeshRenderer {
    uint32_t entityID;
    Mesh* mesh;
};

struct BoxCollider {
    uint32_t entityID;
    bool isActive = true;
    glm::vec3 center;
    glm::vec3 extent;
};

struct RigidBody {
    uint32_t entityID;
    glm::vec3 linearVelocity;
    float linearMagnitude;
    float linearDrag;
    float mass = 1.0f;
    float friction = 10.0f;
};

struct Animator {
    uint32_t entityID;
    std::vector<Animation*> animations;
    Animation* currentAnimation = nullptr;
    float playbackTime = 0.0f;
    std::unordered_map<AnimationChannel*, uint32_t> channelMap;
    std::unordered_map<AnimationChannel*, int> nextKeyPosition;
    std::unordered_map<AnimationChannel*, int> nextKeyRotation;
    std::unordered_map<AnimationChannel*, int> nextKeyScale;
};

struct Camera {
    uint32_t entityID;
    float fov;
    float fovRadians;
    float aspectRatio;
    float nearPlane;
    float farPlane;
    // glm::mat4 projectionMatrix;
    // glm::mat4 viewMatrix;
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
    glm::vec3 position;
    glm::vec3 lookDirection;
    glm::vec3 color;
    glm::vec3 brightness;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float ambientBrightness;
    float diffuseBrightness;

    bool isEnabled;
};

struct PointLight {
    uint32_t entityID;
    bool isActive;
    float brightness;
    glm::vec3 color;
};

struct SpotLight {
    uint32_t entityID;
    bool isActive;
    glm::vec3 color;
    float brightness;
    float cutoff;
    float outerCutoff;
    glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
    bool enableShadows = true;
    unsigned int depthFrameBuffer, blurDepthFrameBuffer, depthTex, blurDepthTex;
    unsigned int shadowWidth = 800;
    unsigned int shadowHeight = 600;
};

struct WindowData {
    unsigned int width = 800;
    unsigned int height = 600;
    unsigned int viewportWidth = 800;
    unsigned int viewportHeight = 600;
};

struct Player {
    uint32_t entityID;
    CameraController* cameraController;
    bool isGrounded = false;
    bool canJump = true;
    bool canSpawnCan = true;
    float jumpHeight = 10.0f;
    float moveSpeed = 10.0f;
    float groundCheckDistance = 0.2f;
};

struct GlobalUBO {
    glm::mat4 view = glm::mat4(1.0);
    glm::mat4 projection = glm::mat4(1.0);
};

struct Scene {
    std::string name = "../data/scenes/default.scene";
    WindowData windowData;

    unsigned int pickingFBO, pickingRBO, litFBO, litRBO, ssaoFBO;
    unsigned int pickingTex, litColorTex, bloomSSAOTex, blurTex, ssaoNoiseTex, ssaoPosTex, ssaoNormalTex;
    unsigned int blurFBO[2], blurSwapTex[2];
    unsigned int fullscreenVAO, fullscreenVBO;
    unsigned int lightingShader, postProcessShader, blurShader, depthShader, ssaoShader, pickingShader, shadowBlurShader;
    uint32_t nodeClicked = INVALID_ID;

    float FPS = 0.0f;
    float frameTime = 0.0f;
    float timeAccum = 0.0f;
    float frameCount = 0;
    float currentFrame = 0.0f;
    float lastFrame = 0.0f;
    float deltaTime;
    float gravity = -18.81f;
    float normalStrength = 1.06f;
    float exposure = 1.0f;
    float bloomThreshold = 0.39f;
    float bloomAmount = 0.1f;
    float ambient = 0.108f;
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

    unsigned int matricesUBO;
    GlobalUBO matricesUBOData;
    uint32_t nextEntityID = 1;
    glm::vec3 wrenchOffset = glm::vec3(0.3f, -0.3f, -0.5f);

    Model* trashcanModel;
    Model* testRoom;
    Model* wrench;
    Player* player;

    DirectionalLight sun;
    std::unordered_map<uint32_t, uint32_t> usedIds;
    std::vector<Model*> models;
    std::vector<glm::vec3> ssaoKernel;
    std::vector<glm::vec3> ssaoNoise;

    std::vector<PointLight> pointLights;
    std::vector<SpotLight> spotLights;
    std::vector<Texture> textures;
    std::vector<Entity> entities;
    std::vector<Transform> transforms;
    std::vector<MeshRenderer> meshRenderers;
    std::vector<BoxCollider> boxColliders;
    std::vector<RigidBody> rigidbodies;
    std::vector<Animator> animators;
    std::vector<Camera*> cameras;

    std::unordered_map<std::string, Mesh*> meshMap;
    std::unordered_map<std::string, Animation*> animationMap;

    std::unordered_map<uint32_t, size_t> entityIndexMap;
    std::unordered_map<uint32_t, size_t> transformIndexMap;
    std::unordered_map<uint32_t, size_t> meshRendererIndexMap;
    std::unordered_map<uint32_t, size_t> boxColliderIndexMap;
    std::unordered_map<uint32_t, size_t> rigidbodyIndexMap;
    std::unordered_map<uint32_t, size_t> animatorIndexMap;
    std::unordered_map<uint32_t, size_t> pointLightIndexMap;
    std::unordered_map<uint32_t, size_t> spotLightIndexMap;
};

uint32_t getEntityID(Scene* scene);
Transform* addTransform(Scene* scene, uint32_t entityID);
Entity* getNewEntity(Scene* scene, std::string name, uint32_t id = -1, bool createTransform = true);
MeshRenderer* addMeshRenderer(Scene* scene, uint32_t entityID);
BoxCollider* addBoxCollider(Scene* scene, uint32_t entityID);
RigidBody* addRigidbody(Scene* scene, uint32_t entityID);
Animator* addAnimator(Scene* scene, uint32_t entityID, Model* model);
Animator* addAnimator(Scene* scene, uint32_t entityID, std::vector<Animation*> animations);
uint32_t createEntityFromModel(Scene* scene, ModelNode* node, uint32_t parentEntityID, bool addColliders);
Camera* addCamera(Scene* scene, uint32_t entityID, float fov, float aspectRatio, float nearPlane, float farPlane);
PointLight* addPointLight(Scene* scene, uint32_t entityID);
SpotLight* addSpotLight(Scene* scene, uint32_t entityID);

void destroyEntity(Scene* scene, uint32_t entityID);

Entity* getEntity(Scene* scene, uint32_t entityID);
Transform* getTransform(Scene* scene, uint32_t entityID);
MeshRenderer* getMeshRenderer(Scene* scene, uint32_t entityID);
BoxCollider* getBoxCollider(Scene* scene, uint32_t entityID);
RigidBody* getRigidbody(Scene* scene, uint32_t entityID);
Animator* getAnimator(Scene* scene, uint32_t entityID);
PointLight* getPointLight(Scene* scene, uint32_t entityID);
SpotLight* getSpotLight(Scene* scene, uint32_t entityID);
Camera* getCamera(Scene* scene, uint32_t entityID);

template <typename Component>
bool destroyComponent(std::vector<Component>& components, std::unordered_map<uint32_t, size_t>& indexMap, uint32_t entityID) {
    if (!indexMap.count(entityID)) {
        return false;
    }

    size_t indexToRemove = indexMap[entityID];
    size_t lastIndex = indexMap.size() - 1;

    if (indexToRemove != lastIndex) {
        uint32_t lastID = components[lastIndex].entityID;
        std::swap(components[lastIndex], components[indexToRemove]);
        indexMap[lastID] = indexToRemove;
    }

    components.pop_back();
    indexMap.erase(entityID);
    return true;
}