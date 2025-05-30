#include <fstream>
#include <iostream>

#include "sceneloader.h"
#include "scene.h"
#include "loader.h"

void parseList(std::string memberString, std::vector<uint32_t>* out) {
    size_t currentPos = 0;
    size_t commaPos = 0;
    int currentIndex = 0;

    while (commaPos != std::string::npos) {
        commaPos = memberString.find(",", currentPos);
        uint32_t childID = std::stoi(memberString.substr(currentPos, commaPos - currentPos));
        out->push_back(childID);
        currentPos = commaPos + 1;
    }
}

void parseList(std::string memberString, float* out) {
    size_t currentPos = 0;
    size_t commaPos = 0;
    int currentIndex = 0;

    while (commaPos != std::string::npos) {
        commaPos = memberString.find(",", currentPos);
        out[currentIndex] = std::stof(memberString.substr(currentPos, commaPos - currentPos));
        currentPos = commaPos + 1;
        currentIndex++;
    }
}

bool findLastScene(Scene* scene) {
    // std::string path = "../data/scenes/";

    if (!std::filesystem::exists(scenePath) || !std::filesystem::is_directory(scenePath)) {
        std::cerr << "ERROR::BAD_PATH::No directory found at: " << scenePath << std::endl;
        return false;
    }

    for (const auto& file : std::filesystem::directory_iterator(scenePath)) {
        if (file.path().extension() == ".scene") {
            scene->scenePath = file.path().string();
            return true;
        }
    }

    return false;
}

void skipWhitespace(std::ifstream* stream) {
    while (std::isspace(stream->peek())) {
        stream->get();
    }
}

void getNextToken(std::ifstream* stream, std::vector<Token>* tokens) {
    char c = static_cast<char>(stream->get());

    std::string text;
    Token token;
    int newLine = int('\n');

    switch (c) {
        case '{':
            token.type = TokenType::BlockOpen;
            token.text = "{";
            tokens->push_back(token);
            break;
        case '}':
            token.type = TokenType::BlockClose;
            token.text = "}";
            tokens->push_back(token);
            break;
        case ':':
            token.type = TokenType::ValueSeparator;
            token.text = ":";
            tokens->push_back(token);
            break;
        default:
            if (!std::isalnum(c) && c != ',' && c != '.' && c != '-' && c != '_' && c != '\\') {
                std::cerr << "ERROR::UNKNOWN_TOKEN--->  " << c << std::endl;
                return;
            }

            text += c;

            while (std::isalnum(stream->peek()) || stream->peek() == ',' || stream->peek() == '.' || stream->peek() == '-' || stream->peek() == '_' || stream->peek() == '\\') {
                c = static_cast<char>(stream->get());
                text += c;
                if (c == ',') {
                    text += " ";
                    skipWhitespace(stream);
                }
            }

            token.type = TokenType::Value;
            token.text = text;
            tokens->push_back(token);
            break;
    }
}

void getTokens(std::ifstream* stream, std::vector<Token>* tokens) {
    skipWhitespace(stream);

    while (!stream->eof()) {
        getNextToken(stream, tokens);
        skipWhitespace(stream);
    }

    Token token;
    token.type = TokenType::EndOfFile;
    tokens->push_back(token);
}

int buildComponentBlock(int currentIndex, std::vector<Token>* tokens, std::vector<ComponentBlock>* components) {
    Token token = tokens->at(currentIndex++);
    ComponentBlock block;
    std::string blockKey;

    if (token.type != TokenType::Value) {
        std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name. Received: " << token.text << " on: " << block.type << std::endl;
        return currentIndex;
    }

    block.type = token.text;
    token = tokens->at(currentIndex++);

    if (token.type != TokenType::BlockOpen) {
        std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected block open. Received: " << token.text << " on: " << block.type << std::endl;
        return currentIndex;
    }

    token = tokens->at(currentIndex++);

    while (token.type != TokenType::BlockClose && token.type != TokenType::EndOfFile) {
        if (token.type != TokenType::Value) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name. Received: " << token.text << " on: " << block.type << std::endl;
            return currentIndex;
        }

        blockKey = token.text;
        token = tokens->at(currentIndex++);

        if (token.type != TokenType::ValueSeparator) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected value separator. Received: " << token.text << " on: " << block.type << std::endl;
            return currentIndex;
        }

        token = tokens->at(currentIndex++);

        if (token.type != TokenType::Value) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name. Received: " << token.text << " on: " << block.type << std::endl;
            return currentIndex;
        }

        block.memberValueMap[blockKey] = token.text;

        token = tokens->at(currentIndex++);
    }

    components->push_back(block);
    return currentIndex;
}

void createComponentBlocks(std::vector<Token>* tokens, std::vector<ComponentBlock>* components) {
    int currentIndex = 0;

    while (tokens->at(currentIndex).type != TokenType::EndOfFile) {
        currentIndex = buildComponentBlock(currentIndex, tokens, components);
    }
}

void logComponentBlocks(std::vector<ComponentBlock>* components) {
    std::ofstream outStream("../data/scenes/help.txt");
    for (int i = 0; i < components->size(); i++) {
        ComponentBlock tokenCheck = components->at(i);
        outStream << tokenCheck.type << std::endl;
        for (auto& pair : tokenCheck.memberValueMap) {
            outStream << pair.first << ": " << pair.second << std::endl;
        }
        outStream << std::endl;
    }
}

void createEntity(Scene* scene, ComponentBlock block) {
    std::string name = "Entity";
    uint32_t id = INVALID_ID;
    bool isActive = true;

    if (block.memberValueMap.count("id") != 0) {
        id = std::stoi(block.memberValueMap["id"]);
    }

    if (block.memberValueMap.count("name") != 0) {
        name = block.memberValueMap["name"];
    }

    if (block.memberValueMap.count("isActive") != 0) {
        isActive = block.memberValueMap["isActive"] == "true" ? true : false;
    }

    Entity* entity = getNewEntity(scene, name, id, false);
    entity->entityID = id;
    entity->isActive = isActive;
}

void createTransform(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    uint32_t parentEntityID = INVALID_ID;
    std::vector<uint32_t> childEntityIds;
    vec3 localPosition = vec3(0.0f, 0.0f, 0.0f);
    quat localRotation = quat(0.0f, 0.0f, 0.0f, 1.0f);
    vec3 localScale = vec3(1.0f, 1.0f, 1.0f);

    std::string memberString;
    size_t currentPos = 0;
    size_t commaPos = 0;
    float floatComps[4];
    int currentIndex = 0;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("parentEntityID")) {
        parentEntityID = std::stoi(block.memberValueMap["parentEntityID"]);
    }

    if (block.memberValueMap.count("childEntityIds")) {
        memberString = block.memberValueMap["childEntityIds"];

        if (memberString != "None") {
            parseList(memberString, &childEntityIds);
        }
    }

    if (block.memberValueMap.count("localPosition")) {
        memberString = block.memberValueMap["localPosition"];
        parseList(memberString, floatComps);
        localPosition.SetX(floatComps[0]);
        localPosition.SetY(floatComps[1]);
        localPosition.SetZ(floatComps[2]);
    }

    if (block.memberValueMap.count("localRotation")) {
        memberString = block.memberValueMap["localRotation"];
        parseList(memberString, floatComps);
        localRotation.SetX(floatComps[0]);
        localRotation.SetY(floatComps[1]);
        localRotation.SetZ(floatComps[2]);
        localRotation.SetW(floatComps[3]);
    }

    if (block.memberValueMap.count("localScale")) {
        memberString = block.memberValueMap["localScale"];
        parseList(memberString, floatComps);
        localScale.SetX(floatComps[0]);
        localScale.SetY(floatComps[1]);
        localScale.SetZ(floatComps[2]);
    }

    Transform* transform = addTransform(scene, entityID);
    transform->parentEntityID = parentEntityID;
    transform->childEntityIds = childEntityIds;
    transform->localPosition = localPosition;
    transform->localRotation = localRotation;
    transform->localScale = localScale;
}

void createMeshRenderer(Scene* scene, Resources* resources, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    uint32_t rootEntity = INVALID_ID;
    Mesh* mesh = nullptr;
    std::vector<Material*> materials;
    std::string memberString;
    std::string materialName;
    size_t currentPos = 0;
    size_t commaPos = 0;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }
    if (block.memberValueMap.count("rootEntity")) {
        rootEntity = std::stoi(block.memberValueMap["rootEntity"]);
    }

    if (block.memberValueMap.count("mesh")) {
        std::string meshName = block.memberValueMap["mesh"];
        if (resources->meshMap.count(meshName)) {
            mesh = resources->meshMap[meshName];
        } else {
            std::cerr << "ERROR::MISSING_MESH::No mesh in mesh map with name: " << meshName << std::endl;
        }
    }

    if (block.memberValueMap.count("materials")) {
        memberString = block.memberValueMap["materials"];

        while (commaPos != std::string::npos) {
            commaPos = memberString.find(",", currentPos);
            materialName = memberString.substr(currentPos, commaPos - currentPos);
            if (resources->materialMap.count(materialName)) {
                materials.push_back(resources->materialMap[materialName]);
            } else {
                Material* material = resources->materialMap["default"];
                // scene->materialMap[materialName] = material;
                materials.push_back(material);
                std::cerr << "ERROR::MISSING_MATERIAL::No material in map with name: " << materialName << std::endl;
                for (auto& pair : resources->materialMap) {
                    std::cout << pair.first << std::endl;
                }
            }

            currentPos = commaPos + 2;
        }
    }

    MeshRenderer* meshRenderer = addMeshRenderer(scene, entityID);
    meshRenderer->rootEntity = rootEntity;
    meshRenderer->materials = materials;
    meshRenderer->mesh = mesh;

    for (int i = 0; i < meshRenderer->mesh->subMeshes.size(); i++) {
        if (meshRenderer->materials.size() > i) {
            meshRenderer->mesh->subMeshes[i].material = meshRenderer->materials[i];
        }
    }
}

void createMaterial(Resources* resources, RenderState* renderer, ComponentBlock block, std::string fileName) {
    std::string name = fileName;
    std::vector<Texture*> textures;
    glm::vec2 textureTiling = glm::vec2(1.0f, 1.0f);
    vec4 baseColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float roughness = 0.5f;
    float metalness = 0.0f;
    float aoStrength = 1.0f;
    float normalStrength = 1.0f;
    std::string memberString = "";
    std::string textureName = "white";
    size_t currentPos = 0;
    size_t commaPos = 0;
    float floatComps[4];

    if (block.memberValueMap.count("textures")) {
        memberString = block.memberValueMap["textures"];

        while (commaPos != std::string::npos) {
            commaPos = memberString.find(",", currentPos);
            textureName = memberString.substr(currentPos, commaPos - currentPos);
            if (resources->textureMap.count(textureName)) {
                textures.push_back(resources->textureMap[textureName]);
            } else {
                textures.push_back(resources->textureMap["white"]);
                std::cerr << "ERROR::MISSING_TEXTURE::No texture in map with name: " << textureName << std::endl;
                for (auto& pair : resources->textureMap) {
                    std::cout << pair.first << std::endl;
                }
            }

            currentPos = commaPos + 2;
        }
    }

    if (block.memberValueMap.count("textureTiling")) {
        memberString = block.memberValueMap["textureTiling"];
        parseList(memberString, floatComps);
        textureTiling.x = floatComps[0];
        textureTiling.y = floatComps[1];
    }

    if (block.memberValueMap.count("baseColor")) {
        memberString = block.memberValueMap["baseColor"];
        parseList(memberString, floatComps);
        baseColor.SetX(floatComps[0]);
        baseColor.SetY(floatComps[1]);
        baseColor.SetZ(floatComps[2]);
        baseColor.SetW(floatComps[3]);
    }

    if (block.memberValueMap.count("roughness")) {
        roughness = std::stof(block.memberValueMap["roughness"]);
    }

    if (block.memberValueMap.count("metalness")) {
        metalness = std::stof(block.memberValueMap["metalness"]);
    }

    if (block.memberValueMap.count("aoStrength")) {
        aoStrength = std::stof(block.memberValueMap["aoStrength"]);
    }

    if (block.memberValueMap.count("normalStrength")) {
        normalStrength = std::stof(block.memberValueMap["normalStrength"]);
    }

    Material* material;

    if (resources->materialMap.count(name)) {
        material = resources->materialMap[name];
    } else {
        material = new Material();
        resources->materialMap[name] = material;
    }

    material->name = name;
    material->shader = renderer->lightingShader;
    material->textures = textures;
    material->textureTiling = textureTiling;
    material->baseColor = baseColor;
    material->roughness = roughness;
    material->metalness = metalness;
    material->aoStrength = aoStrength;
    material->normalStrength = normalStrength;
}

void createRigidbody(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    std::string memberString = "";
    JPH::ObjectLayer objectLayer = Layers::NON_MOVING;
    JPH::EMotionType motionType = JPH::EMotionType::Static;
    JPH::ShapeSettings::ShapeResult shapeResult;
    JPH::ShapeRefC shape;
    vec3 halfExtents = vec3(1.0f, 1.0f, 1.0f);
    float halfHeight;
    float radius;
    float mass = 1.0f;
    float floatComps[3];
    bool rotationLocked = false;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("mass")) {
        mass = std::stof(block.memberValueMap["mass"]);
    }

    if (block.memberValueMap.count("radius")) {
        radius = std::stof(block.memberValueMap["radius"]);
    }

    if (block.memberValueMap.count("halfHeight")) {
        halfHeight = std::stof(block.memberValueMap["halfHeight"]);
    }

    if (block.memberValueMap.count("rotationLocked")) {
        memberString = block.memberValueMap["rotationLocked"];
        if (memberString == "true") {
            rotationLocked = true;
        }
    }

    if (block.memberValueMap.count("motionType")) {
        memberString = block.memberValueMap["motionType"];

        if (memberString == "static") {
            motionType = JPH::EMotionType::Static;
        } else if (memberString == "kinematic") {
            motionType = JPH::EMotionType::Kinematic;
        } else if (memberString == "dynamic") {
            motionType = JPH::EMotionType::Dynamic;
        }
    }

    if (block.memberValueMap.count("layer")) {
        memberString = block.memberValueMap["layer"];
        if (memberString == "MOVING") {
            objectLayer = Layers::MOVING;
        } else if (memberString == "NON_MOVING") {
            objectLayer = Layers::NON_MOVING;
        }
    }

    if (block.memberValueMap.count("halfExtents")) {
        memberString = block.memberValueMap["halfExtents"];
        parseList(memberString, floatComps);
        halfExtents.SetX(floatComps[0]);
        halfExtents.SetY(floatComps[1]);
        halfExtents.SetZ(floatComps[2]);
    }
    if (block.memberValueMap.count("shape")) {
        memberString = block.memberValueMap["shape"];

        if (memberString == "box") {
            JPH::BoxShapeSettings boxSettings(halfExtents);
            shapeResult = boxSettings.Create();
            shape = shapeResult.Get();
        } else if (memberString == "sphere") {
            JPH::SphereShapeSettings sphereSettings(radius);
            shapeResult = sphereSettings.Create();
            shape = shapeResult.Get();
        } else if (memberString == "capsule") {
            JPH::CapsuleShapeSettings capsuleSettings(halfHeight, radius);
            shapeResult = capsuleSettings.Create();
            shape = shapeResult.Get();
        } else if (memberString == "cylinder") {
            JPH::CylinderShapeSettings cylinderSettings(halfHeight, radius);
            shapeResult = cylinderSettings.Create();
            shape = shapeResult.Get();
        }
    }

    JPH::BodyCreationSettings bodySettings(shape, JPH::RVec3(0.0_r, 0.0_r, 0.0_r), quat::sIdentity(), motionType, objectLayer);
    if (rotationLocked) {
        bodySettings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX | JPH::EAllowedDOFs::TranslationY | JPH::EAllowedDOFs::TranslationZ;
    }

    bodySettings.mAllowDynamicOrKinematic = true;
    JPH::Body* body = scene->bodyInterface->CreateBody(bodySettings);
    scene->bodyInterface->AddBody(body->GetID(), JPH::EActivation::DontActivate);

    RigidBody* rb = addRigidbody(scene, entityID);
    rb->rotationLocked = rotationLocked;
    rb->joltBody = body->GetID();
}

void createAnimator(Scene* scene, Resources* resources, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    std::vector<Animation*> animations;
    std::string memberString;
    std::string animationName;
    size_t currentPos = 0;
    size_t commaPos = 0;
    int currentIndex = 0;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("animations")) {
        memberString = block.memberValueMap["animations"];

        while (commaPos != std::string::npos) {
            commaPos = memberString.find(",", currentPos);
            animationName = memberString.substr(currentPos, commaPos - currentPos);
            if (resources->animationMap.count(animationName)) {
                animations.push_back(resources->animationMap[animationName]);
            } else {
                std::cerr << "ERROR::MISSING_ANIMATION::No animation in map with name: " << animationName << std::endl;
                for (auto& pair : resources->animationMap) {
                    std::cout << pair.first << std::endl;
                }
            }

            currentPos = commaPos + 2;
        }
    }

    Animator* animator = addAnimator(scene, entityID, animations);
}

void createCamera(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    float fov = 68.0f;
    float nearPlane = 0.1f;
    float farPlane = 800.0f;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("fov")) {
        fov = std::stof(block.memberValueMap["fov"]);
    }

    if (block.memberValueMap.count("nearPlane")) {
        nearPlane = std::stof(block.memberValueMap["nearPlane"]);
    }

    if (block.memberValueMap.count("farPlane")) {
        farPlane = std::stof(block.memberValueMap["farPlane"]);
    }

    Camera* camera = addCamera(scene, entityID, fov, nearPlane, farPlane);
}

void createPointLights(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    bool isActive = false;
    float brightness = 1.0f;
    vec3 color = vec3(1.0f, 1.0f, 1.0f);

    std::string memberString;
    float floatComps[3];

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("isActive")) {
        isActive = block.memberValueMap["isActive"] == "true" ? true : false;
    }

    if (block.memberValueMap.count("brightness")) {
        brightness = std::stof(block.memberValueMap["brightness"]);
    }

    if (block.memberValueMap.count("color")) {
        memberString = block.memberValueMap["color"];
        parseList(memberString, floatComps);
        color.SetX(floatComps[0]);
        color.SetY(floatComps[1]);
        color.SetZ(floatComps[2]);
    }

    PointLight* light = addPointLight(scene, entityID);
    light->isActive = isActive;
    light->brightness = brightness;
    light->color = color;
}

void createSpotLights(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    bool isActive = false;
    float brightness = 1.0f;
    float cutoff = 45.0f;
    float outerCutoff = 60.0f;
    vec3 color = vec3(1.0f, 1.0f, 1.0f);
    bool shadowsEnabled = false;
    unsigned int shadowWidth = 800;
    unsigned int shadowHeight = 600;

    std::string memberString;
    float floatComps[3];

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("isActive")) {
        isActive = block.memberValueMap["isActive"] == "true" ? true : false;
    }

    if (block.memberValueMap.count("brightness")) {
        brightness = std::stof(block.memberValueMap["brightness"]);
    }

    if (block.memberValueMap.count("cutoff")) {
        cutoff = std::stof(block.memberValueMap["cutoff"]);
    }

    if (block.memberValueMap.count("outerCutoff")) {
        outerCutoff = std::stof(block.memberValueMap["outerCutoff"]);
    }

    if (block.memberValueMap.count("shadows")) {
        shadowsEnabled = block.memberValueMap["shadows"] == "true" ? true : false;
    }

    if (block.memberValueMap.count("shadowWidth")) {
        shadowWidth = std::stof(block.memberValueMap["shadowWidth"]);
    }

    if (block.memberValueMap.count("shadowHeight")) {
        shadowHeight = std::stof(block.memberValueMap["shadowHeight"]);
    }

    if (block.memberValueMap.count("color")) {
        memberString = block.memberValueMap["color"];
        parseList(memberString, floatComps);
        color.SetX(floatComps[0]);
        color.SetY(floatComps[1]);
        color.SetZ(floatComps[2]);
    }

    SpotLight* light = addSpotLight(scene, entityID);
    light->isActive = isActive;
    light->brightness = brightness;
    light->cutoff = cutoff;
    light->outerCutoff = outerCutoff;
    light->color = color;
    light->enableShadows = shadowsEnabled;
    light->shadowWidth = shadowWidth;
    light->shadowHeight = shadowHeight;

    if (light->enableShadows) {
        createSpotLightShadowMap(light);
    }
}

void createPlayer(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    uint32_t armsID = INVALID_ID;
    float jumpHeight = 10.0f;
    float moveSpeed = 10.0f;
    float groundCheckDistance = 0.2f;
    uint32_t cameraController_EntityID;
    uint32_t cameraController_CameraTargetEntityID;
    uint32_t cameraController_CameraEntityID;
    float sensitivity = 0.3f;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("armsID")) {
        armsID = std::stoi(block.memberValueMap["armsID"]);
    }

    if (block.memberValueMap.count("jumpHeight")) {
        jumpHeight = std::stof(block.memberValueMap["jumpHeight"]);
    }

    if (block.memberValueMap.count("moveSpeed")) {
        moveSpeed = std::stof(block.memberValueMap["moveSpeed"]);
    }

    if (block.memberValueMap.count("groundCheckDistance")) {
        groundCheckDistance = std::stof(block.memberValueMap["groundCheckDistance"]);
    }

    if (block.memberValueMap.count("cameraControllerEntityID")) {
        cameraController_EntityID = std::stoi(block.memberValueMap["cameraControllerEntityID"]);
    }

    if (block.memberValueMap.count("cameraControllerCameraTargetEntityID")) {
        cameraController_CameraTargetEntityID = std::stoi(block.memberValueMap["cameraControllerCameraTargetEntityID"]);
    }

    if (block.memberValueMap.count("cameraControllerCameraEntityID")) {
        cameraController_CameraEntityID = std::stoi(block.memberValueMap["cameraControllerCameraEntityID"]);
    }

    if (block.memberValueMap.count("sensitivity")) {
        sensitivity = std::stof(block.memberValueMap["sensitivity"]);
    }

    Player* player = new Player();
    player->cameraController = new CameraController();
    player->entityID = entityID;
    player->armsID = armsID;
    player->jumpHeight = jumpHeight;
    player->moveSpeed = moveSpeed;
    player->groundCheckDistance = groundCheckDistance;
    player->cameraController->entityID = cameraController_EntityID;
    player->cameraController->cameraTargetEntityID = cameraController_CameraTargetEntityID;
    player->cameraController->cameraEntityID = cameraController_CameraEntityID;
    player->cameraController->sensitivity = sensitivity;
    scene->player = player;
}

void createModelSettings(Resources* resources, ComponentBlock block) {
    std::string path = "";

    if (block.memberValueMap.count("path")) {
        path = block.memberValueMap["path"];
    }

    ModelSettings settings;
    settings.path = path;
    resources->modelImportMap[path] = settings;
}

void createTextureSettings(Resources* resources, ComponentBlock block) {
    std::string path = "";
    bool gamma = true;
    GLenum filter = GL_NEAREST;

    if (block.memberValueMap.count("path")) {
        path = block.memberValueMap["path"];
    }

    if (block.memberValueMap.count("gamma")) {
        if (block.memberValueMap["gamma"] == "false") {
            gamma = false;
        }
    }

    if (block.memberValueMap.count("filter")) {
        std::string filterString = block.memberValueMap["filter"];
        if (filterString == "GL_LINEAR") {
            filter = GL_LINEAR;
        } else if (filterString == "GL_NEAREST") {
            filter = GL_NEAREST;
        }
    }

    TextureSettings settings;
    settings.path = path;
    settings.gamma = gamma;
    settings.filter = filter;
    resources->textureImportMap[path] = settings;
}

void createImportSettings(Resources* resources, std::vector<ComponentBlock>* components) {
    for (int i = 0; i < components->size(); i++) {
        ComponentBlock block = components->at(i);

        if (block.type == "Model") {
            createModelSettings(resources, block);
        } else if (block.type == "Texture") {
            createTextureSettings(resources, block);
        }
    }
}

void createComponents(Scene* scene, Resources* resources, std::vector<ComponentBlock>* components) {
    for (int i = 0; i < components->size(); i++) {
        ComponentBlock block = components->at(i);

        if (block.type == "Entity") {
            createEntity(scene, block);
        } else if (block.type == "Transform") {
            createTransform(scene, block);
        } else if (block.type == "MeshRenderer") {
            createMeshRenderer(scene, resources, block);
        } else if (block.type == "Rigidbody") {
            createRigidbody(scene, block);
        } else if (block.type == "Animator") {
            createAnimator(scene, resources, block);
        } else if (block.type == "Camera") {
            createCamera(scene, block);
        } else if (block.type == "PointLight") {
            createPointLights(scene, block);
        } else if (block.type == "SpotLight") {
            createSpotLights(scene, block);
        } else if (block.type == "Player") {
            createPlayer(scene, block);
        }
    }
}

void loadDefaultScene(Scene* scene, Resources* resources) {
    for (int i = 0; i < 2; i++) {
        Entity* pointLightEntity = getNewEntity(scene, "PointLight");
        PointLight* pointLight = addPointLight(scene, pointLightEntity->entityID);
        setPosition(scene, pointLightEntity->entityID, vec3(2.0f + i / 2, 3.0f, 1.0f + i / 2));
        pointLight->color = vec3(1.0f, 1.0f, 1.0f);
        pointLight->isActive = true;
        pointLight->brightness = 1.0f;
    }

    uint32_t spotLightEntityID = getNewEntity(scene, "SpotLight")->entityID;
    SpotLight* spotLight = addSpotLight(scene, spotLightEntityID);
    spotLight->isActive = true;
    spotLight->color = vec3(1.0f, 1.0f, 1.0f);
    spotLight->brightness = 6.0f;
    spotLight->cutoff = 15.5f;
    spotLight->outerCutoff = 55.5f;
    spotLight->shadowWidth = 800;
    spotLight->shadowHeight = 600;

    const uint32_t levelEntity = createEntityFromModel(scene, resources->modelMap["testroom.gltf"]->rootNode, INVALID_ID, true, INVALID_ID, true, false);
    const uint32_t armsID = createEntityFromModel(scene, resources->modelMap["WrenchArms.gltf"]->rootNode, INVALID_ID, false, INVALID_ID, true, false);
    Transform* armsTransform = getTransform(scene, armsID);
    addAnimator(scene, armsID, resources->modelMap["WrenchArms.gltf"]);

    Entity* wrenchParent = getNewEntity(scene, "WrenchParent");
    Player* player = buildPlayer(scene);
    player->armsID = armsID;
    setParent(scene, armsID, wrenchParent->entityID);
    setParent(scene, wrenchParent->entityID, player->cameraController->cameraTargetEntityID);
    setParent(scene, spotLightEntityID, player->cameraController->cameraEntityID);
    setLocalRotation(scene, wrenchParent->entityID, quat::sEulerAngles(vec3(0.0f, 0.0f, 0.0f)));
    // setLocalPosition(scene, wrenchParent->entityID, scene->wrenchOffset);
    setLocalRotation(scene, spotLightEntityID, quat::sEulerAngles(vec3(0.0f, 0.0f, 0.0f)));
    setLocalPosition(scene, spotLightEntityID, vec3(0.0f, 0.0f, 1.0f));
}

void loadMaterials(Resources* resources, RenderState* renderer) {
    for (const std::filesystem::directory_entry& dir : std::filesystem::directory_iterator("..\\resources\\")) {
        if (dir.is_regular_file()) {
            std::filesystem::path path = dir.path();
            std::string fileString = path.filename().string();
            std::string ext = path.extension().string();
            if (ext == ".mat") {
                std::ifstream stream(path);
                std::vector<Token> tokens;
                std::vector<ComponentBlock> components;
                getTokens(&stream, &tokens);
                createComponentBlocks(&tokens, &components);
                // createComponents(scene, &components);
                createMaterial(resources, renderer, components[0], fileString);
            }
        }
    }
}

void loadResourceSettings(Resources* resources, std::unordered_set<std::string>& metaPaths) {
    for (std::string path : metaPaths) {
        std::ifstream stream(path);
        std::vector<Token> tokens;
        std::vector<ComponentBlock> components;
        getTokens(&stream, &tokens);
        createComponentBlocks(&tokens, &components);
        createImportSettings(resources, &components);
    }
}

void loadTempScene(Resources* resources, Scene* scene) {
    std::string fileName = scene->scenePath.substr(scene->scenePath.find_last_of('/') + 1);
    scene->name = fileName.substr(0, fileName.find('.'));
    std::ifstream stream("..\\data\\scenes\\temp.tempscene");
    std::vector<Token> tokens;
    std::vector<ComponentBlock> components;
    getTokens(&stream, &tokens);
    createComponentBlocks(&tokens, &components);
    // logComponentBlocks(&components);
    createComponents(scene, resources, &components);

    for (int i = 0; i < scene->transforms.size(); i++) {
        updateTransformMatrices(scene, &scene->transforms[i]);
    }

    for (int i = 0; i < scene->rigidbodies.size(); i++) {
        RigidBody* rb = &scene->rigidbodies[i];
        scene->bodyInterface->SetPositionAndRotation(rb->joltBody, getPosition(scene, rb->entityID), getRotation(scene, rb->entityID), JPH::EActivation::DontActivate);
        rb->lastPosition = getPosition(scene, rb->entityID);
        rb->lastRotation = getRotation(scene, rb->entityID);

        if (scene->bodyInterface->GetObjectLayer(rb->joltBody) == Layers::MOVING) {
            scene->movingRigidbodies.insert(rb->entityID);
        }
    }

    for (MeshRenderer& renderer : scene->meshRenderers) {
        mapBones(scene, &renderer);
    }

    if (scene->player != nullptr) {
        scene->player->cameraController->camera = getCamera(scene, scene->player->cameraController->cameraEntityID);
    }
}

void loadScene(Resources* resources, Scene* scene) {
    std::string fileName = scene->scenePath.substr(scene->scenePath.find_last_of('/') + 1);
    scene->name = fileName.substr(0, fileName.find('.'));
    std::ifstream stream(scene->scenePath);
    std::vector<Token> tokens;
    std::vector<ComponentBlock> components;
    getTokens(&stream, &tokens);
    createComponentBlocks(&tokens, &components);
    // logComponentBlocks(&components);
    createComponents(scene, resources, &components);

    for (int i = 0; i < scene->transforms.size(); i++) {
        updateTransformMatrices(scene, &scene->transforms[i]);
    }

    for (int i = 0; i < scene->rigidbodies.size(); i++) {
        RigidBody* rb = &scene->rigidbodies[i];
        scene->bodyInterface->SetPositionAndRotation(rb->joltBody, getPosition(scene, rb->entityID), getRotation(scene, rb->entityID), JPH::EActivation::DontActivate);
        rb->lastPosition = getPosition(scene, rb->entityID);
        rb->lastRotation = getRotation(scene, rb->entityID);

        if (scene->bodyInterface->GetObjectLayer(rb->joltBody) == Layers::MOVING) {
            scene->movingRigidbodies.insert(rb->entityID);
        }
    }

    for (MeshRenderer& renderer : scene->meshRenderers) {
        mapBones(scene, &renderer);
    }

    if (scene->player != nullptr) {
        scene->player->cameraController->camera = getCamera(scene, scene->player->cameraController->cameraEntityID);
    }

    writeTempScene(scene, resources);
}

void loadFirstFoundScene(Scene* scene, Resources* resources) {
    if (!findLastScene(scene)) {
        loadDefaultScene(scene, resources);
        return;
    }

    std::string fileName = scene->scenePath.substr(scene->scenePath.find_last_of('/') + 1);
    scene->name = fileName.substr(0, fileName.find('.'));
    std::ifstream stream(scene->scenePath);
    std::vector<Token> tokens;
    std::vector<ComponentBlock> components;
    getTokens(&stream, &tokens);
    createComponentBlocks(&tokens, &components);
    // logComponentBlocks(&components);
    createComponents(scene, resources, &components);

    for (int i = 0; i < scene->transforms.size(); i++) {
        updateTransformMatrices(scene, &scene->transforms[i]);
    }

    for (int i = 0; i < scene->rigidbodies.size(); i++) {
        RigidBody* rb = &scene->rigidbodies[i];
        scene->bodyInterface->SetPositionAndRotation(rb->joltBody, getPosition(scene, rb->entityID), getRotation(scene, rb->entityID), JPH::EActivation::DontActivate);
        rb->lastPosition = getPosition(scene, rb->entityID);
        rb->lastRotation = getRotation(scene, rb->entityID);

        if (scene->bodyInterface->GetObjectLayer(rb->joltBody) == Layers::MOVING) {
            scene->movingRigidbodies.insert(rb->entityID);
        }
    }

    for (MeshRenderer& renderer : scene->meshRenderers) {
        mapBones(scene, &renderer);
    }

    if (scene->player != nullptr) {
        scene->player->cameraController->camera = getCamera(scene, scene->player->cameraController->cameraEntityID);
    }
    writeTempScene(scene, resources);
}

void writeEntities(Scene* scene, std::ofstream& stream) {
    for (Entity& entity : scene->entities) {
        std::string id = std::to_string(entity.entityID);
        std::string name = entity.name;
        std::string isActive = entity.isActive ? "true" : "false";

        stream << "Entity {" << std::endl;
        stream << "id: " << id << std::endl;
        stream << "name: " << name << std::endl;
        stream << "isActive: " << isActive << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writeTransforms(Scene* scene, std::ofstream& stream) {
    for (Transform& transform : scene->transforms) {
        std::string entityID = std::to_string(transform.entityID);
        std::string parentEntityID = transform.parentEntityID == INVALID_ID ? std::to_string(-1) : std::to_string(transform.parentEntityID);
        std::string childEntityIds = "";

        if (transform.childEntityIds.size() > 0) {
            childEntityIds += std::to_string(transform.childEntityIds[0]);

            for (size_t i = 1; i < transform.childEntityIds.size(); i++) {
                childEntityIds += ", " + std::to_string(transform.childEntityIds[i]);
            }
        } else {
            childEntityIds = "None";
        }

        std::string localPosition = std::to_string(transform.localPosition.GetX()) + ", " + std::to_string(transform.localPosition.GetY()) + ", " + std::to_string(transform.localPosition.GetZ());
        std::string localRotation = std::to_string(transform.localRotation.GetX()) + ", " + std::to_string(transform.localRotation.GetY()) + ", " + std::to_string(transform.localRotation.GetZ()) + ", " + std::to_string(transform.localRotation.GetW());
        std::string localScale = std::to_string(transform.localScale.GetX()) + ", " + std::to_string(transform.localScale.GetY()) + ", " + std::to_string(transform.localScale.GetZ());

        stream << "Transform {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "parentEntityID: " << parentEntityID << std::endl;
        stream << "childEntityIds: " << childEntityIds << std::endl;
        stream << "localPosition: " << localPosition << std::endl;
        stream << "localRotation: " << localRotation << std::endl;
        stream << "localScale: " << localScale << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writeTextureSettings(TextureSettings settings) {
    std::string texPath = settings.path;
    std::string gamma = settings.gamma ? "true" : "false";
    std::string filter = settings.filter == GL_LINEAR ? "GL_LINEAR" : "GL_NEAREST";

    std::ofstream stream(texPath + ".meta");
    stream << "Texture {" << std::endl;
    stream << "path: " << texPath << std::endl;
    stream << "gamma: " << gamma << std::endl;
    stream << "filter: " << filter << std::endl;
    stream << "}" << std::endl
           << std::endl;
}

void writeMaterial(Resources* resources, std::filesystem::path path) {
    std::filesystem::path fileName = path.filename();

    if (fileName.extension() != ".mat") {
        return;
    }

    if (!resources->materialMap.count(fileName.string())) {
        return;
    }

    Material* material = resources->materialMap[fileName.string()];
    std::ofstream stream(path);
    std::string textures = "";
    std::string baseColor = std::to_string(material->baseColor.GetX()) + ", " + std::to_string(material->baseColor.GetY()) + ", " + std::to_string(material->baseColor.GetZ()) + ", " + std::to_string(material->baseColor.GetW());
    std::string roughness = std::to_string(material->roughness);
    std::string metalness = std::to_string(material->metalness);
    std::string aoStrength = std::to_string(material->aoStrength);
    std::string normalStrength = std::to_string(material->normalStrength);
    std::string textureTiling = std::to_string(material->textureTiling.x) + ", " + std::to_string(material->textureTiling.y);

    if (material->textures.size() > 0) {
        textures += material->textures[0]->name;
    } else {
        textures += "white";
    }

    for (int i = 1; i < 5; i++) {
        textures += ", " + material->textures[i]->name;
    }

    stream << "Material {" << std::endl;
    stream << "textures: " << textures << std::endl;
    stream << "textureTiling: " << textureTiling << std::endl;
    stream << "baseColor: " << baseColor << std::endl;
    stream << "roughness: " << roughness << std::endl;
    stream << "metalness: " << metalness << std::endl;
    stream << "aoStrength: " << aoStrength << std::endl;
    stream << "normalStrength: " << normalStrength << std::endl;
    stream << "}" << std::endl
           << std::endl;
}

void writeMaterials(Resources* resources) {
    for (const std::filesystem::directory_entry& dir : std::filesystem::directory_iterator("..\\resources\\")) {
        if (dir.is_regular_file()) {
            std::filesystem::path path = dir.path();
            if (path.extension() == ".mat") {
                writeMaterial(resources, path.filename());
            }
        }
    }
}

void writeMeshRenderers(Scene* scene, std::ofstream& stream) {
    for (MeshRenderer& renderer : scene->meshRenderers) {
        std::string entityID = std::to_string(renderer.entityID);
        std::string rootEntity = std::to_string(renderer.rootEntity);
        std::string mesh = renderer.mesh->name;
        std::string materials = "";

        materials += renderer.mesh->subMeshes[0].material->name;

        for (int i = 1; i < renderer.mesh->subMeshes.size(); i++) {
            materials += ", " + renderer.mesh->subMeshes[i].material->name;
        }

        stream << "MeshRenderer {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "rootEntity: " << rootEntity << std::endl;
        stream << "mesh: " << mesh << std::endl;
        stream << "materials: " << materials << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writeRigidbodies(Scene* scene, std::ofstream& stream) {
    JPH::EShapeSubType shapeType;
    JPH::ObjectLayer objectLayer;
    JPH::EMotionType motionType;

    std::string entityID;
    std::string objectLayerString;
    std::string motionTypeString;
    std::string rotationLocked;
    std::string halfExtentString;
    std::string halfHeight;
    std::string radius;
    std::string mass;

    for (RigidBody& rb : scene->rigidbodies) {
        const JPH::BoxShape* box;
        const JPH::SphereShape* sphere;
        const JPH::CapsuleShape* capsule;
        const JPH::CylinderShape* cylinder;
        const JPH::Shape* shape = scene->bodyInterface->GetShape(rb.joltBody).GetPtr();

        entityID = std::to_string(rb.entityID);
        objectLayer = scene->bodyInterface->GetObjectLayer(rb.joltBody);
        shapeType = shape->GetSubType();
        motionType = scene->bodyInterface->GetMotionType(rb.joltBody);
        rotationLocked = rb.rotationLocked ? "true" : "false";
        mass = std::to_string(shape->GetMassProperties().mMass);

        stream << "Rigidbody {" << std::endl;
        stream << "entityID: " << entityID << std::endl;

        switch (objectLayer) {
            case Layers::MOVING:
                stream << "layer: MOVING" << std::endl;
                break;
            case Layers::NON_MOVING:
                stream << "layer: NON_MOVING" << std::endl;
                break;
        }

        stream << "mass: " << mass << std::endl;
        stream << "rotationLocked: " << rotationLocked << std::endl;

        switch (motionType) {
            case JPH::EMotionType::Static:
                motionTypeString = "static";
                break;
            case JPH::EMotionType::Kinematic:
                motionTypeString = "kinematic";
                break;
            case JPH::EMotionType::Dynamic:
                motionTypeString = "dynamic";
                break;
        }

        stream << "motionType: " << motionTypeString << std::endl;

        switch (shapeType) {
            case JPH::EShapeSubType::Box:
                box = static_cast<const JPH::BoxShape*>(shape);
                vec3 extents = box->GetHalfExtent();
                halfExtentString = std::to_string(extents.GetX()) + ", " + std::to_string(extents.GetY()) + ", " + std::to_string(extents.GetZ());
                stream << "shape: box" << std::endl;
                stream << "halfExtents: " << halfExtentString << std::endl;
                break;
            case JPH::EShapeSubType::Sphere:
                sphere = static_cast<const JPH::SphereShape*>(shape);
                radius = std::to_string(sphere->GetRadius());
                stream << "shape: sphere" << std::endl;
                stream << "radius: " << radius << std::endl;
                break;
            case JPH::EShapeSubType::Capsule:
                capsule = static_cast<const JPH::CapsuleShape*>(shape);
                halfHeight = std::to_string(capsule->GetHalfHeightOfCylinder());
                radius = std::to_string(capsule->GetRadius());
                stream << "shape: capsule" << std::endl;
                stream << "halfHeight: " << halfHeight << std::endl;
                stream << "radius: " << radius << std::endl;
                break;
            case JPH::EShapeSubType::Cylinder:
                cylinder = static_cast<const JPH::CylinderShape*>(shape);
                halfHeight = std::to_string(cylinder->GetHalfHeight());
                radius = std::to_string(cylinder->GetRadius());
                stream << "shape: cylinder" << std::endl;
                stream << "halfHeight: " << halfHeight << std::endl;
                stream << "radius: " << radius << std::endl;
                break;
            case JPH::EShapeSubType::Mesh:
                break;
            case JPH::EShapeSubType::HeightField:
                break;
            case JPH::EShapeSubType::ConvexHull:
                break;
        }

        stream << "}" << std::endl
               << std::endl;
    }
}

void writeAnimators(Scene* scene, std::ofstream& stream) {
    for (Animator& animator : scene->animators) {
        std::string entityID = std::to_string(animator.entityID);
        std::string animations = "";

        if (animator.animations.size() > 0) {
            animations += animator.animations.at(0)->name;

            for (size_t i = 1; i < animator.animations.size(); i++) {
                animations += ", " + animator.animations.at(i)->name;
            }
        }

        stream << "Animator {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "animations: " << animations << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writePointLights(Scene* scene, std::ofstream& stream) {
    for (PointLight& light : scene->pointLights) {
        std::string entityID = std::to_string(light.entityID);
        std::string isActive = light.isActive ? "true" : "false";
        std::string brightness = std::to_string(light.brightness);
        std::string color = std::to_string(light.color.GetX()) + ", " + std::to_string(light.color.GetY()) + ", " + std::to_string(light.color.GetZ());

        stream << "PointLight {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "isActive: " << isActive << std::endl;
        stream << "brightness: " << brightness << std::endl;
        stream << "color: " << color << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writeSpotLights(Scene* scene, std::ofstream& stream) {
    for (SpotLight& light : scene->spotLights) {
        std::string entityID = std::to_string(light.entityID);
        std::string isActive = light.isActive ? "true" : "false";
        std::string brightness = std::to_string(light.brightness);
        std::string cutoff = std::to_string(light.cutoff);
        std::string outerCutoff = std::to_string(light.outerCutoff);
        std::string color = std::to_string(light.color.GetX()) + ", " + std::to_string(light.color.GetY()) + ", " + std::to_string(light.color.GetZ());
        std::string shadows = light.enableShadows ? "true" : "false";
        std::string shadowWidth = std::to_string(light.shadowWidth);
        std::string shadowHeight = std::to_string(light.shadowHeight);

        stream << "SpotLight {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "isActive: " << isActive << std::endl;
        stream << "brightness: " << brightness << std::endl;
        stream << "cutoff: " << cutoff << std::endl;
        stream << "outerCutoff: " << outerCutoff << std::endl;
        stream << "color: " << color << std::endl;
        stream << "shadows: " << shadows << std::endl;
        stream << "shadowWidth: " << shadowWidth << std::endl;
        stream << "shadowHeight: " << shadowHeight << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writePlayer(Scene* scene, std::ofstream& stream) {
    std::string entityID = std::to_string(scene->player->entityID);
    std::string armsID = std::to_string(scene->player->armsID);
    std::string jumpHeight = std::to_string(scene->player->jumpHeight);
    std::string moveSpeed = std::to_string(scene->player->moveSpeed);
    std::string groundCheckDistance = std::to_string(scene->player->groundCheckDistance);
    std::string cameraController_EntityID = std::to_string(scene->player->cameraController->entityID);
    std::string cameraController_cameraTargetEntityID = std::to_string(scene->player->cameraController->cameraTargetEntityID);
    std::string cameraController_cameraEntityID = std::to_string(scene->player->cameraController->cameraEntityID);
    std::string cameraController_Sensitivity = std::to_string(scene->player->cameraController->sensitivity);

    stream << "Player {" << std::endl;
    stream << "entityID: " << entityID << std::endl;
    stream << "armsID: " << armsID << std::endl;
    stream << "jumpHeight: " << jumpHeight << std::endl;
    stream << "moveSpeed: " << moveSpeed << std::endl;
    stream << "groundCheckDistance: " << groundCheckDistance << std::endl;
    stream << "cameraControllerEntityID: " << cameraController_EntityID << std::endl;
    stream << "cameraControllerCameraTargetEntityID: " << cameraController_cameraTargetEntityID << std::endl;
    stream << "cameraControllerCameraEntityID: " << cameraController_cameraEntityID << std::endl;
    stream << "cameraControllerSensitivity: " << cameraController_Sensitivity << std::endl;
    stream << "}" << std::endl
           << std::endl;
}

void writeCameras(Scene* scene, std::ofstream& stream) {
    for (Camera* camera : scene->cameras) {
        std::string entityID = std::to_string(camera->entityID);
        std::string fov = std::to_string(camera->fov);
        std::string nearPlane = std::to_string(camera->nearPlane);
        std::string farPlane = std::to_string(camera->farPlane);

        stream << "Camera {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "fov: " << fov << std::endl;
        stream << "nearPlane: " << nearPlane << std::endl;
        stream << "farPlane: " << farPlane << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void saveScene(Scene* scene, Resources* resources) {
    writeTempScene(scene, resources);
    std::ofstream stream(scene->scenePath);
    writeEntities(scene, stream);
    writeTransforms(scene, stream);
    writeMeshRenderers(scene, stream);
    writeRigidbodies(scene, stream);
    writeAnimators(scene, stream);
    writePointLights(scene, stream);
    writeSpotLights(scene, stream);
    writeCameras(scene, stream);
    writePlayer(scene, stream);
    writeMaterials(resources);
}

void writeTempScene(Scene* scene, Resources* resources) {
    std::ofstream stream("..\\data\\scenes\\temp.tempscene");
    writeEntities(scene, stream);
    writeTransforms(scene, stream);
    writeMeshRenderers(scene, stream);
    writeRigidbodies(scene, stream);
    writeAnimators(scene, stream);
    writePointLights(scene, stream);
    writeSpotLights(scene, stream);
    writeCameras(scene, stream);
    writePlayer(scene, stream);
    // writeMaterials(resources);
}