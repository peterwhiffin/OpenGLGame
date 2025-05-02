#include <filesystem>
#include <fstream>

#include "sceneloader.h"
#include "transform.h"

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

bool findLastScene(std::string* outScene) {
    std::string path = "../data/scenes/";

    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        std::cerr << "ERROR::BAD_PATH::No directory found at: " << path << std::endl;
        return false;
    }

    for (const auto& file : std::filesystem::directory_iterator(path)) {
        if (file.path().extension() == ".scene") {
            *outScene = file.path().string();
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
            if (!std::isalnum(c) && c != ',' && c != '.' && c != '-') {
                std::cerr << "ERROR::UNKNOWN_TOKEN" << std::endl;
                return;
            }

            text += c;

            while (std::isalnum(stream->peek()) || stream->peek() == ',' || stream->peek() == '.' || stream->peek() == '-') {
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
        std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name: " << token.text << std::endl;
        return currentIndex;
    }

    block.type = token.text;
    token = tokens->at(currentIndex++);

    if (token.type != TokenType::BlockOpen) {
        std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected block open" << token.text << std::endl;
        return currentIndex;
    }

    token = tokens->at(currentIndex++);

    while (token.type != TokenType::BlockClose && token.type != TokenType::EndOfFile) {
        if (token.type != TokenType::Value) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name" << token.text << std::endl;
            return currentIndex;
        }

        blockKey = token.text;
        token = tokens->at(currentIndex++);

        if (token.type != TokenType::ValueSeparator) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected value separator" << token.text << std::endl;
            return currentIndex;
        }

        token = tokens->at(currentIndex++);

        if (token.type != TokenType::Value) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name" << token.text << std::endl;
            return currentIndex;
        }

        block.memberValueMap[blockKey] = token.text;

        token = tokens->at(currentIndex++);
    }

    components->push_back(block);
    return currentIndex;
}

void createComponentBlocks(Scene* scene, std::vector<Token>* tokens, std::vector<ComponentBlock>* components) {
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
    glm::vec3 localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);

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
        localPosition.x = floatComps[0];
        localPosition.y = floatComps[1];
        localPosition.z = floatComps[2];
    }

    if (block.memberValueMap.count("localRotation")) {
        memberString = block.memberValueMap["localRotation"];
        parseList(memberString, floatComps);
        localRotation.w = floatComps[0];
        localRotation.x = floatComps[1];
        localRotation.y = floatComps[2];
        localRotation.z = floatComps[3];
    }

    if (block.memberValueMap.count("localScale")) {
        memberString = block.memberValueMap["localScale"];
        parseList(memberString, floatComps);
        localScale.x = floatComps[0];
        localScale.y = floatComps[1];
        localScale.z = floatComps[2];
    }

    Transform* transform = addTransform(scene, entityID);
    transform->parentEntityID = parentEntityID;
    transform->childEntityIds = childEntityIds;
    transform->localPosition = localPosition;
    transform->localRotation = localRotation;
    transform->localScale = localScale;
}

void createMeshRenderer(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    Mesh* mesh = nullptr;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("mesh")) {
        std::string meshName = block.memberValueMap["mesh"];
        if (scene->meshMap.count(meshName)) {
            mesh = scene->meshMap[meshName];
        } else {
            std::cerr << "ERROR::MISSING_MESH::No mesh in mesh map with name: " << meshName << std::endl;
        }
    }

    MeshRenderer* meshRenderer = addMeshRenderer(scene, entityID);
    meshRenderer->mesh = mesh;
}

void createBoxCollider(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    bool isActive = true;
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 extent = glm::vec3(3.0f, 0.2f, 3.0f);
    std::string memberString = block.memberValueMap["childEntityIds"];
    float floatComps[3];

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("isActive")) {
        isActive = block.memberValueMap["isActive"] == "true" ? true : true;
    }

    if (block.memberValueMap.count("center")) {
        memberString = block.memberValueMap["center"];
        parseList(memberString, floatComps);
        center.x = floatComps[0];
        center.y = floatComps[1];
        center.z = floatComps[2];
    }

    if (block.memberValueMap.count("extent")) {
        memberString = block.memberValueMap["extent"];
        parseList(memberString, floatComps);
        extent.x = floatComps[0];
        extent.y = floatComps[1];
        extent.z = floatComps[2];
    }

    BoxCollider* collider = addBoxCollider(scene, entityID);
    collider->isActive = true;
    collider->center = center;
    collider->extent = extent;
}

void createRigidbody(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    float linearDrag = 0.0f;
    float mass = 1.0f;
    float friction = 1.0f;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("linearDrag")) {
        linearDrag = std::stof(block.memberValueMap["linearDrag"]);
    }

    if (block.memberValueMap.count("mass")) {
        mass = std::stoi(block.memberValueMap["mass"]);
    }

    if (block.memberValueMap.count("friction")) {
        friction = std::stoi(block.memberValueMap["friction"]);
    }

    RigidBody* rb = addRigidbody(scene, entityID);
    rb->linearDrag = linearDrag;
    rb->mass = mass;
    rb->friction = friction;
}

void createAnimator(Scene* scene, ComponentBlock block) {
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
            if (scene->animationMap.count(animationName)) {
                animations.push_back(scene->animationMap[animationName]);
            } else {
                std::cerr << "ERROR::MISSING_ANIMATION::No animation in map with name: " << animationName << std::endl;
                for (auto& pair : scene->animationMap) {
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
    float aspectRatio = (float)800 / 600;
    float nearPlane = 0.1f;
    float farPlane = 800.0f;

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("fov")) {
        fov = std::stof(block.memberValueMap["fov"]);
    }

    if (block.memberValueMap.count("aspectRatio")) {
        aspectRatio = std::stof(block.memberValueMap["aspectRatio"]);
    }

    if (block.memberValueMap.count("nearPlane")) {
        nearPlane = std::stof(block.memberValueMap["nearPlane"]);
    }

    if (block.memberValueMap.count("farPlane")) {
        farPlane = std::stof(block.memberValueMap["farPlane"]);
    }

    fov = glm::degrees(fov);
    Camera* camera = addCamera(scene, entityID, fov, aspectRatio, nearPlane, farPlane);
}

void createPointLights(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    bool isActive = false;
    float brightness = 1.0f;
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);

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
        color.r = floatComps[0];
        color.g = floatComps[1];
        color.b = floatComps[2];
    }

    PointLight* light = addPointLight(scene, entityID);
    light->isActive = isActive;
    light->brightness = brightness;
    light->color = color;
}

void createPlayer(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
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
    player->jumpHeight = jumpHeight;
    player->moveSpeed = moveSpeed;
    player->groundCheckDistance = groundCheckDistance;
    player->cameraController->entityID = cameraController_EntityID;
    player->cameraController->cameraTargetEntityID = cameraController_CameraTargetEntityID;
    player->cameraController->cameraEntityID = cameraController_CameraEntityID;
    player->cameraController->sensitivity = sensitivity;
    scene->player = player;
}

void createComponents(Scene* scene, std::vector<ComponentBlock>* components) {
    for (int i = 0; i < components->size(); i++) {
        ComponentBlock block = components->at(i);

        if (block.type == "Entity") {
            createEntity(scene, block);
        } else if (block.type == "Transform") {
            createTransform(scene, block);
        } else if (block.type == "MeshRenderer") {
            createMeshRenderer(scene, block);
        } else if (block.type == "BoxCollider") {
            createBoxCollider(scene, block);
        } else if (block.type == "Rigidbody") {
            createRigidbody(scene, block);
        } else if (block.type == "Animator") {
            createAnimator(scene, block);
        } else if (block.type == "Camera") {
            createCamera(scene, block);
        } else if (block.type == "PointLight") {
            createPointLights(scene, block);
        } else if (block.type == "Player") {
            createPlayer(scene, block);
        }
    }
}

void loadScene(Scene* scene, std::string path) {
    scene->name = path;
    std::ifstream stream(path);
    std::vector<Token> tokens;
    std::vector<ComponentBlock> components;
    getTokens(&stream, &tokens);
    createComponentBlocks(scene, &tokens, &components);
    // logComponentBlocks(&components);
    createComponents(scene, &components);

    for (int i = 0; i < scene->transforms.size(); i++) {
        updateTransformMatrices(scene, &scene->transforms[i]);
    }

    if (scene->player != nullptr) {
        scene->player->cameraController->camera = getCamera(scene, scene->player->cameraController->cameraEntityID);
    }
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

        std::string localPosition = std::to_string(transform.localPosition.x) + ", " + std::to_string(transform.localPosition.y) + ", " + std::to_string(transform.localPosition.z);
        std::string localRotation = std::to_string(transform.localRotation.w) + ", " + std::to_string(transform.localRotation.x) + ", " + std::to_string(transform.localRotation.y) + ", " + std::to_string(transform.localRotation.z);
        std::string localScale = std::to_string(transform.localScale.x) + ", " + std::to_string(transform.localScale.y) + ", " + std::to_string(transform.localScale.z);

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

void writeMeshRenderers(Scene* scene, std::ofstream& stream) {
    for (MeshRenderer& renderer : scene->meshRenderers) {
        std::string entityID = std::to_string(renderer.entityID);
        std::string mesh = renderer.mesh->name;

        stream << "MeshRenderer {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "mesh: " << mesh << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writeBoxColliders(Scene* scene, std::ofstream& stream) {
    for (BoxCollider& collider : scene->boxColliders) {
        std::string entityID = std::to_string(collider.entityID);
        std::string isActive = collider.isActive ? "true" : "false";
        std::string center = std::to_string(collider.center.x) + ", " + std::to_string(collider.center.y) + ", " + std::to_string(collider.center.z);
        std::string extent = std::to_string(collider.extent.x) + ", " + std::to_string(collider.extent.y) + ", " + std::to_string(collider.extent.z);

        stream << "BoxCollider {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "isActive: " << isActive << std::endl;
        stream << "center: " << center << std::endl;
        stream << "extent: " << extent << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writeRigidbodies(Scene* scene, std::ofstream& stream) {
    for (RigidBody& rb : scene->rigidbodies) {
        std::string entityID = std::to_string(rb.entityID);
        std::string linearDrag = std::to_string(rb.linearDrag);
        std::string mass = std::to_string(rb.mass);
        std::string friction = std::to_string(rb.friction);

        stream << "Rigidbody {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "linearDrag: " << linearDrag << std::endl;
        stream << "mass: " << mass << std::endl;
        stream << "friction: " << friction << std::endl;
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
        std::string color = std::to_string(light.color.r) + ", " + std::to_string(light.color.g) + ", " + std::to_string(light.color.b);

        stream << "PointLight {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "isActive: " << isActive << std::endl;
        stream << "brightness: " << brightness << std::endl;
        stream << "color: " << color << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void writePlayer(Scene* scene, std::ofstream& stream) {
    std::string entityID = std::to_string(scene->player->entityID);
    std::string jumpHeight = std::to_string(scene->player->jumpHeight);
    std::string moveSpeed = std::to_string(scene->player->moveSpeed);
    std::string groundCheckDistance = std::to_string(scene->player->groundCheckDistance);
    std::string cameraController_EntityID = std::to_string(scene->player->cameraController->entityID);
    std::string cameraController_cameraTargetEntityID = std::to_string(scene->player->cameraController->cameraTargetEntityID);
    std::string cameraController_cameraEntityID = std::to_string(scene->player->cameraController->cameraEntityID);
    std::string cameraController_Sensitivity = std::to_string(scene->player->cameraController->sensitivity);

    stream << "Player {" << std::endl;
    stream << "entityID: " << entityID << std::endl;
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
        std::string aspectRatio = std::to_string(camera->aspectRatio);
        std::string nearPlane = std::to_string(camera->nearPlane);
        std::string farPlane = std::to_string(camera->farPlane);

        stream << "Camera {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "fov: " << fov << std::endl;
        stream << "aspectRatio: " << aspectRatio << std::endl;
        stream << "nearPlane: " << nearPlane << std::endl;
        stream << "farPlane: " << farPlane << std::endl;
        stream << "}" << std::endl
               << std::endl;
    }
}

void saveScene(Scene* scene) {
    std::ofstream stream(scene->name);
    writeEntities(scene, stream);
    writeTransforms(scene, stream);
    writeMeshRenderers(scene, stream);
    writeBoxColliders(scene, stream);
    writeRigidbodies(scene, stream);
    writeAnimators(scene, stream);
    writePointLights(scene, stream);
    writeCameras(scene, stream);
    writePlayer(scene, stream);
}
