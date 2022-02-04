/**
 * @file
 */

#include "backend/entity/ai/LUAAIRegistry.h"
#include "backend/entity/ai/aggro/AggroMgr.h"
#include "backend/entity/ai/group/GroupMgr.h"
#include "app/tests/AbstractTest.h"
#include "TestEntity.h"
#include <glm/gtc/epsilon.hpp>
#include <sstream>
#include <iostream>

namespace glm {

inline void PrintTo(const vec2& v, ::std::ostream* os) {
	*os << "glm::vec2(" << v.x << ":" << v.y << ")";
}

inline void PrintTo(const vec3& v, ::std::ostream* os) {
	*os << "glm::vec3(" << v.x << ":" << v.y << ":" << v.z << ")";
}

inline void PrintTo(const vec4& v, ::std::ostream* os) {
	*os << "glm::vec4(" << v.x << ":" << v.y << ":" << v.z << ":" << v.w << ")";
}

}

inline bool operator==(const glm::vec3 &vecA, const glm::vec3 &vecB) {
	return glm::all(glm::epsilonEqual(vecA, vecB, 0.0001f));
}

class TestSuite: public app::AbstractTest {
protected:
	backend::LUAAIRegistry _registry;
	backend::GroupMgr _groupManager;

	core::String printAggroList(backend::AggroMgr& aggroMgr) const {
		const backend::AggroMgr::Entries& e = aggroMgr.getEntries();
		if (e.empty()) {
			return "empty";
		}

		std::stringstream s;
		for (const backend::Entry& ep : e) {
			s << ep.getCharacterId() << "=" << ep.getAggro() << ", ";
		}
		const backend::EntryPtr& highest = aggroMgr.getHighestEntry();
		s << "highest: " << highest->getCharacterId() << "=" << highest->getAggro();
		return core::String(s.str().c_str());
	}

	virtual void SetUp() override;
	virtual void TearDown() override;
};
