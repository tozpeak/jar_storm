#pragma once

#include <ecs.h>

#define ECS_GET_NEW(cName,entityId,cType) cType##Component *cName = (cType##Component*) ecs_get((entityId), CID_##cType);


#define ECS_GET_INTO(cNamePtr,entityId,cType) cNamePtr = (cType##Component*) ecs_get((entityId), CID_##cType);

#define ECS_SET_FLAG(entityId,cType) ecs_add((entityId), CID_##cType, NULL);
