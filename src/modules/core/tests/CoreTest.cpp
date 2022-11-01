/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/FourCC.h"
#include "core/Vector.h"
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <list>
#include <vector>

namespace core {

TEST(CoreTest, testVecSize) {
	std::vector<glm::vec4> vec4;
	EXPECT_EQ(0u, core::vectorSize(vec4));
	vec4.push_back(glm::vec4(0.0f));
	EXPECT_EQ(4 * sizeof(glm::vec4::value_type), core::vectorSize(vec4));
	vec4.push_back(glm::vec4(0.0f));
	EXPECT_EQ(8 * sizeof(glm::vec4::value_type), core::vectorSize(vec4));
}

TEST(CoreTest, testIsVector) {
	EXPECT_TRUE(core::isVector<std::vector<int>>::value);
	EXPECT_TRUE(core::isVector<const std::vector<int>>::value);

	EXPECT_FALSE(core::isVector<std::list<int>>::value);
	EXPECT_FALSE(core::isVector<int>::value);

	EXPECT_TRUE(core::isVector<std::vector<uint8_t>&>::value);

	std::vector<uint8_t> output;
	std::vector<uint8_t> &output2 = output;
	EXPECT_TRUE(core::isVector<decltype(output)>::value);
	EXPECT_TRUE(core::isVector<decltype(output2)>::value);
}

TEST(CoreTest, testFourCC) {
	const uint32_t fcc = FourCC('a', 'b', 'c', 'd');
	uint8_t buf[4];
	FourCCRev(buf, fcc);
	EXPECT_EQ(buf[0], 'a');
	EXPECT_EQ(buf[1], 'b');
	EXPECT_EQ(buf[2], 'c');
	EXPECT_EQ(buf[3], 'd');
}

}
