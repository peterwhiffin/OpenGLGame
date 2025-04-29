#include <filesystem>
#include <fstream>

#include "sceneloader.h"
#include "transform.h"

bool findLastScene(std::string* outScene) {
    std::string path = "../data/scenes/";

    if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
        std::cerr << "ERROR::BAD_PATH::No directory found at: " << path << std::endl;
        return false;
    }

    for (const auto& file : std::filesystem::directory_iterator(path)) {
        /* if (std::filesystem::is_regular_file(file) && file.path().extension() == ".scene") {
            *outScene = file.path().string();
        } */
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
            if (!std::isalnum(c) && c != ',' && c != '.') {
                std::cerr << "ERROR::UNKNOWN_TOKEN" << std::endl;
                return;
            }

            text += c;

            while (std::isalnum(stream->peek()) || stream->peek() == ',' || stream->peek() == '.') {
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
        std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name" << std::endl;
        return currentIndex;
    }

    block.type = token.text;
    token = tokens->at(currentIndex++);

    if (token.type != TokenType::BlockOpen) {
        std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected block open" << std::endl;
        return currentIndex;
    }

    token = tokens->at(currentIndex++);

    while (token.type != TokenType::BlockClose && token.type != TokenType::EndOfFile) {
        if (token.type != TokenType::Value) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name" << std::endl;
            return currentIndex;
        }

        blockKey = token.text;
        token = tokens->at(currentIndex++);

        if (token.type != TokenType::ValueSeparator) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected value separator" << std::endl;
            return currentIndex;
        }

        token = tokens->at(currentIndex++);

        if (token.type != TokenType::Value) {
            std::cerr << "ERROR::WRONG_TOKEN_TYPE::Expected type name" << std::endl;
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
    for (int i = 0; i < components->size(); i++) {
        ComponentBlock* block = &components->at(i);
        std::cout << "-------------Component Block----------------" << std::endl;
        std::cout << block->type << ": " << std::endl;

        for (const auto& pair : block->memberValueMap) {
            std::cout << "  " << pair.first << ": " << pair.second << std::endl;
        }
    }
}

void createEntity(Scene* scene, ComponentBlock block) {
    std::string name = "Entity";
    uint32_t id = INVALID_ID;
    bool isActive = true;

    if (block.memberValueMap.count("id") != 0) {
        id = std::stoi(block.memberValueMap["id"]);
    } else {
        // id = getEntityID(scene);
    }

    if (block.memberValueMap.count("name") != 0) {
        name = block.memberValueMap["name"];
    }

    if (block.memberValueMap.count("isActive") != 0) {
        isActive = block.memberValueMap["isActive"] == "true" ? true : false;
    }

    Entity* entity = getNewEntity(scene, name);
    entity->id = id;
    entity->isActive = isActive;
}

void createTransform(Scene* scene, ComponentBlock block) {
    uint32_t entityID = INVALID_ID;
    uint32_t parentEntityID = INVALID_ID;
    std::vector<uint32_t> childEntityIds;
    glm::vec3 localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::quat localRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 localScale = glm::vec3(1.0f, 1.0f, 1.0f);

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("parentEntityID")) {
        parentEntityID = std::stoi(block.memberValueMap["parentEntityID"]);
    }

    if (block.memberValueMap.count("childEntityIds")) {
        std::string idString = block.memberValueMap["childEntityIds"];

        size_t commaPos = idString.find_first_of(",");
        size_t currentPos = 0;

        while (commaPos != std::string::npos) {
            uint32_t childID = std::stoi(idString.substr(currentPos, commaPos - 1));
            childEntityIds.push_back(childID);
            currentPos = commaPos + 1;
            commaPos = idString.find_first_of(",", currentPos);
        }
    }

    if (block.memberValueMap.count("localPosition")) {
        std::string positionString = block.memberValueMap["localPosition"];

        size_t commaPos = positionString.find_first_of(",");
        size_t currentPos = 0;
        std::vector<float> vectorComps = {0.0f, 0.0f, 0.0f};
        int currentIndex = 0;

        while (commaPos != std::string::npos) {
            vectorComps[currentIndex] = std::stof(positionString.substr(currentPos, commaPos - 1));
            currentPos = commaPos + 1;
            commaPos = positionString.find_first_of(",", currentPos);
            currentIndex++;
            if (currentIndex == 3) {
                break;
            }
        }

        localPosition.x = vectorComps[0];
        localPosition.y = vectorComps[1];
        localPosition.z = vectorComps[2];
    }

    if (block.memberValueMap.count("localRotation")) {
        std::string rotationString = block.memberValueMap["localRotation"];

        size_t commaPos = rotationString.find_first_of(",");
        size_t currentPos = 0;
        std::vector<float> vectorComps = {0.0f, 0.0f, 0.0f, 0.0f};
        int currentIndex = 0;

        while (commaPos != std::string::npos) {
            vectorComps[currentIndex] = std::stof(rotationString.substr(currentPos, commaPos - 1));
            currentPos = commaPos + 1;
            commaPos = rotationString.find_first_of(",", currentPos);
            currentIndex++;
            if (currentIndex == 4) {
                break;
            }
        }

        localRotation.w = vectorComps[0];
        localRotation.x = vectorComps[1];
        localRotation.y = vectorComps[2];
        localRotation.z = vectorComps[3];
    }

    if (block.memberValueMap.count("localScale")) {
        std::string scaleString = block.memberValueMap["localScale"];

        size_t commaPos = scaleString.find_first_of(",");
        size_t currentPos = 0;
        std::vector<float> vectorComps = {0.0f, 0.0f, 0.0f};
        int currentIndex = 0;

        while (commaPos != std::string::npos) {
            vectorComps[currentIndex] = std::stof(scaleString.substr(currentPos, commaPos - 1));
            currentPos = commaPos + 1;
            commaPos = scaleString.find_first_of(",", currentPos);
            currentIndex++;
            if (currentIndex == 3) {
                break;
            }
        }

        localScale.x = vectorComps[0];
        localScale.y = vectorComps[1];
        localScale.z = vectorComps[2];
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
    bool isActive = false;
    glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 extent = glm::vec3(0.0f, 0.0f, 0.0f);

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("isActive")) {
        isActive = block.memberValueMap["isActive"] == "true" ? true : false;
    }

    if (block.memberValueMap.count("center")) {
        std::string centerString = block.memberValueMap["center"];

        size_t commaPos = centerString.find_first_of(",");
        size_t currentPos = 0;
        std::vector<float> vectorComps = {0.0f, 0.0f, 0.0f};
        int currentIndex = 0;

        while (commaPos != std::string::npos) {
            vectorComps[currentIndex] = std::stof(centerString.substr(currentPos, commaPos - 1));
            currentPos = commaPos + 1;
            commaPos = centerString.find_first_of(",", currentPos);
            currentIndex++;
            if (currentIndex == 3) {
                break;
            }
        }

        center.x = vectorComps[0];
        center.y = vectorComps[1];
        center.z = vectorComps[2];
    }

    if (block.memberValueMap.count("extent")) {
        std::string extentString = block.memberValueMap["extent"];

        size_t commaPos = extentString.find_first_of(",");
        size_t currentPos = 0;
        std::vector<float> vectorComps = {0.0f, 0.0f, 0.0f};
        int currentIndex = 0;

        while (commaPos != std::string::npos) {
            vectorComps[currentIndex] = std::stof(extentString.substr(currentPos, commaPos - 1));
            currentPos = commaPos + 1;
            commaPos = extentString.find_first_of(",", currentPos);
            currentIndex++;
            if (currentIndex == 3) {
                break;
            }
        }

        extent.x = vectorComps[0];
        extent.y = vectorComps[1];
        extent.z = vectorComps[2];
    }

    BoxCollider* collider = addBoxCollider(scene, entityID);
    collider->isActive = isActive;
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

    if (block.memberValueMap.count("entityID")) {
        entityID = std::stoi(block.memberValueMap["entityID"]);
    }

    if (block.memberValueMap.count("animations")) {
        std::string animationString = block.memberValueMap["animations"];

        size_t commaPos = animationString.find_first_of(",");
        size_t currentPos = 0;

        while (commaPos != std::string::npos) {
            std::string name = animationString.substr(currentPos, commaPos - 1);
            if (scene->animationMap.count(name)) {
                animations.push_back(scene->animationMap[name]);
            } else {
                std::cerr << "ERROR::MISSING_ANIMATION::No animation in map with name: " << name << std::endl;
            }

            currentPos = commaPos + 1;
            commaPos = animationString.find_first_of(",", currentPos);
        }
    }

    Animator* animator = addAnimator(scene, entityID, animations);
}

void createCamera(Scene* scene, ComponentBlock block) {
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

    for (int i = 0; i < scene->animators.size(); i++) {
    }
}

void writeEntities(Scene* scene, std::ofstream& stream) {
    for (Entity& entity : scene->entities) {
        std::string id = std::to_string(entity.id);
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
        std::string parentEntityID = std::to_string(transform.parentEntityID);
        std::string childEntityIds = "";
        if (transform.childEntityIds.size() > 0) {
            childEntityIds += std::to_string(transform.childEntityIds[0]);

            for (int i = 0; i < transform.childEntityIds.size(); i++) {
                childEntityIds += ", " + std::to_string(transform.childEntityIds[i]);
            }
        }

        std::string localPosition = std::to_string(transform.localPosition.x) + ", " + std::to_string(transform.localPosition.y) + ", " + std::to_string(transform.localPosition.z);
        std::string localRotation = std::to_string(transform.localRotation.w) + ", " + std::to_string(transform.localRotation.x) + ", " + std::to_string(transform.localRotation.y) + ", " + std::to_string(transform.localRotation.z);
        std::string localScale = std::to_string(transform.localScale.x) + ", " + std::to_string(transform.localScale.y) + ", " + std::to_string(transform.localScale.z);

        stream << "Transform {" << std::endl;
        stream << "entityID: " << entityID << std::endl;
        stream << "parentEntityID: " << parentEntityID << std::endl;
        stream << "childEntityIds: " << childEntityIds << std::endl;
        stream << "localPosition: " << localPosition << std::endl;
        stream << "localRotationL " << localRotation << std::endl;
        stream << "localRotationL " << localScale << std::endl;
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
        if (animator.animations.size() != 0) {
            animations += animator.animations.at(0)->name;

            for (int i = 0; i < animator.animations.size(); i++) {
                animations += ", " + animator.animations.at(i)->name;
            }
        }
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

void saveScene(Scene* scene) {
    std::ofstream stream(scene->name);
    writeEntities(scene, stream);
    writeTransforms(scene, stream);
}
