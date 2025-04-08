#include "component.h"
#include "renderer.h"

Component::Component() {}
Component::Component(Entity* newEntity) : entity(newEntity), transform(&newEntity->transform) {}
