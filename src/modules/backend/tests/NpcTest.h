/**
 * @file
 */

#pragma once

#include "EntityTest.h"
#include "backend/entity/Npc.h"
#include "backend/entity/ai/filter/SelectEntitiesOfTypes.h"
#include "backend/entity/ai/zone/Zone.h"

namespace backend {

class NpcTest: public EntityTest {
private:
	using Super = EntityTest;
protected:
	NpcPtr setVisible(const NpcPtr& npc) {
		const NpcPtr& partner = create();
		npc->updateVisible({partner});

		const FilterFactoryContext filterCtx(network::EnumNameEntityType(partner->entityType()));
		const FilterPtr& filter = SelectEntitiesOfTypes::getFactory().create(&filterCtx);
		filter->filter(npc->ai());
		return partner;
	}

	inline NpcPtr create(network::EntityType type = network::EntityType::ANIMAL_RABBIT) {
		glm::ivec3 pos = glm::ivec3(0);
		const NpcPtr& npc = map->spawnMgr().spawn(type, &pos);
		map->zone()->update(0L);
		return npc;
	}
};

}
