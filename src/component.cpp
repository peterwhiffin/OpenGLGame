#include "component.h"
#include "renderer.h"

Component::Component(Entity* newEntity) : entity(newEntity), transform(&newEntity->transform) {}
MeshRenderer::MeshRenderer(Entity* newEntity) : Component(newEntity) {}

MeshRenderer* addMeshRenderer(Entity* entity, std::vector<MeshRenderer>* renderers) {
    MeshRenderer* newMeshRenderer = new MeshRenderer(entity);
    entity->components.push_back(newMeshRenderer);
    renderers->push_back(*newMeshRenderer);
    return newMeshRenderer;
}