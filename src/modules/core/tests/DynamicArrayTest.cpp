/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/Common.h"
#include "core/collection/DynamicArray.h"
#include "core/String.h"
#include "core/Algorithm.h"

namespace core {

struct DynamicArrayStruct {
	DynamicArrayStruct() : _foo(""), _bar(1337) {
	}
	DynamicArrayStruct(const core::String &foo, int bar) :
			_foo(foo), _bar(bar) {
	}
	core::String _foo;
	int _bar;
};

template<size_t SIZE>
::std::ostream& operator<<(::std::ostream& ostream, const DynamicArray<DynamicArrayStruct, SIZE>& v) {
	int idx = 0;
	for (auto i = v.begin(); i != v.end();) {
		ostream << "'";
		ostream << i->_bar;
		ostream << "' (" << idx << ")";
		if (++i != v.end()) {
			ostream << ", ";
		}
		++idx;
	}
	return ostream;
}

TEST(DynamicArrayTest, testEmplaceBack) {
	DynamicArray<DynamicArrayStruct> array;
	array.emplace_back("", 0);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testPushBack) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testClear) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	array.clear();
	EXPECT_EQ(0u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testRelease) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 0));
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	array.release();
	EXPECT_EQ(0u, array.size()) << array;
	EXPECT_EQ(0u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testSort) {
	DynamicArray<int> array;
	array.push_back(3);
	array.push_back(5);
	array.push_back(1);
	array.push_back(11);
	array.push_back(9);
	array.sort(core::Greater<int>());
	EXPECT_EQ(1, array[0]);
	EXPECT_EQ(3, array[1]);
	EXPECT_EQ(5, array[2]);
	EXPECT_EQ(9, array[3]);
	EXPECT_EQ(11, array[4]);
}

TEST(DynamicArrayTest, testIterate) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	int i = 1;
	for (const DynamicArrayStruct& d : array) {
		EXPECT_EQ(i++, d._bar) << array;
	}
}

TEST(DynamicArrayTest, testCopy) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(32u, array.capacity()) << array;
	DynamicArray<DynamicArrayStruct> copy(array);
	EXPECT_EQ(3u, copy.size()) << array;
	EXPECT_EQ(32u, copy.capacity()) << array;
}

TEST(DynamicArrayTest, testTriggerResize) {
	DynamicArray<DynamicArrayStruct, 2> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	EXPECT_EQ(2u, array.size()) << array;
	EXPECT_EQ(2u, array.capacity()) << array;
	array.push_back(DynamicArrayStruct("", 3));
	EXPECT_EQ(3u, array.size()) << array;
	EXPECT_EQ(4u, array.capacity()) << array;
}

TEST(DynamicArrayTest, testResize) {
	DynamicArray<DynamicArrayStruct, 2> array;
	array.push_back(DynamicArrayStruct("", 1));
	array.push_back(DynamicArrayStruct("", 2));
	EXPECT_EQ(2u, array.size()) << array;
	EXPECT_EQ(2u, array.capacity()) << array;
	array.resize(3);
	EXPECT_EQ(4u, array.capacity()) << array;
	ASSERT_EQ(3u, array.size()) << array;
	EXPECT_EQ(1337, array[2]._bar) << array;
}

TEST(DynamicArrayTest, testErase) {
	DynamicArray<DynamicArrayStruct> array;
	for (int i = 0; i < 128; ++i) {
		array.push_back(DynamicArrayStruct("", i));
	}
	EXPECT_EQ(128u, array.size()) << array;
	EXPECT_EQ(128u, array.capacity()) << array;
	array.erase(0, 10);
	EXPECT_EQ(118u, array.size()) << array;
	EXPECT_EQ(10, array[0]._bar) << array;
	array.erase(1, 10);
	EXPECT_EQ(108u, array.size()) << array;
	EXPECT_EQ(10, array[0]._bar) << array;
	array.erase(100, 100);
	EXPECT_EQ(100u, array.size()) << array;
	EXPECT_EQ(10, array[0]._bar) << array;
	EXPECT_EQ(119, array[99]._bar) << array;
	array.erase(0, 99);
	EXPECT_EQ(1u, array.size()) << array;
	EXPECT_EQ(119, array[0]._bar) << array;
	array.erase(0, 1);
	EXPECT_EQ(0u, array.size()) << array;
}

TEST(DynamicArrayTest, testEraseSmall) {
	DynamicArray<DynamicArrayStruct> array;
	array.push_back(DynamicArrayStruct(core::String(1024, 'a'), 0));
	array.push_back(DynamicArrayStruct(core::String(1024, 'b'), 1));
	array.push_back(DynamicArrayStruct(core::String(4096, 'c'), 2));
	array.push_back(DynamicArrayStruct(core::String(1337, 'd'), 3));
	array.erase(0, 1);
	EXPECT_EQ(1, array[0]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	EXPECT_EQ(2, array[1]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	EXPECT_EQ(3, array[2]._bar) << "After erasing index 0 from 0, 1, 2, 3, it is expected to have 1, 2, 3 left: " << array;
	array.erase(2, 1);
	EXPECT_EQ(1, array[0]._bar) << array;
	EXPECT_EQ(2, array[1]._bar) << array;
}

TEST(DynamicArrayTest, testAppend) {
	DynamicArray<DynamicArrayStruct> array;
	const DynamicArrayStruct buf[] = {
		DynamicArrayStruct(core::String(1024, 'a'), 0),
		DynamicArrayStruct(core::String(1024, 'b'), 1),
		DynamicArrayStruct(core::String(4096, 'c'), 2),
		DynamicArrayStruct(core::String(1337, 'd'), 3)
	};
	array.append(buf, 2);
	ASSERT_EQ(2u, array.size());
	EXPECT_EQ(0, array[0]._bar);
	EXPECT_EQ(1, array[1]._bar);
	array.append(&buf[2], 2);
	ASSERT_EQ(4u, array.size());
	EXPECT_EQ(2, array[2]._bar);
	EXPECT_EQ(3, array[3]._bar);
}

TEST(DynamicArrayTest, testInsertSingle) {
	const DynamicArrayStruct buf[] = {
		DynamicArrayStruct(core::String(1024, 'a'), 0),
		DynamicArrayStruct(core::String(1024, 'b'), 1)
	};
	DynamicArray<DynamicArrayStruct> array;
	array.reserve(2);
	array.insert(array.begin(), &buf[0], 1);
	array.insert(array.begin(), &buf[1], 1);
	ASSERT_EQ(2u, array.size());
	EXPECT_EQ(1, array[0]._bar);
	EXPECT_EQ(0, array[1]._bar);
}

TEST(DynamicArrayTest, testInsertMultiple) {
	const DynamicArrayStruct buf[] = {
		DynamicArrayStruct(core::String(1024, 'a'), 0),
		DynamicArrayStruct(core::String(1024, 'b'), 1)
	};
	DynamicArray<DynamicArrayStruct> array;
	array.reserve(2);
	array.insert(array.begin(), buf, 2);
	ASSERT_EQ(2u, array.size());
	EXPECT_EQ(0, array[0]._bar);
	EXPECT_EQ(1, array[1]._bar);
}

TEST(DynamicArrayTest, testInsertMiddle) {
	const DynamicArrayStruct buf[] = {
		DynamicArrayStruct(core::String(1024, 'a'), 0),
		DynamicArrayStruct(core::String(1024, 'b'), 1),
		DynamicArrayStruct(core::String(4096, 'c'), 2),
		DynamicArrayStruct(core::String(1337, 'd'), 3),
		DynamicArrayStruct(core::String(0xEE, 'e'), 4),
		DynamicArrayStruct(core::String(0xFF, 'f'), 5)
	};
	DynamicArray<DynamicArrayStruct> array;
	array.reserve(32);
	array.insert(array.begin(), &buf[0], 4);
	ASSERT_EQ(4u, array.size());
	EXPECT_EQ(0, array[0]._bar);
	EXPECT_EQ(1, array[1]._bar);
	EXPECT_EQ(2, array[2]._bar);
	EXPECT_EQ(3, array[3]._bar);

	array.insert(core::next(array.begin(), 2), buf, 6);
	ASSERT_EQ(10u, array.size());
	EXPECT_EQ(0, array[0]._bar); // previously at [0]
	EXPECT_EQ(1, array[1]._bar); // previously at [1]

	EXPECT_EQ(0, array[2]._bar); // new insert complete array - 6 entries - 0-5
	EXPECT_EQ(1, array[3]._bar);
	EXPECT_EQ(2, array[4]._bar);
	EXPECT_EQ(3, array[5]._bar);
	EXPECT_EQ(4, array[6]._bar);
	EXPECT_EQ(5, array[7]._bar);

	EXPECT_EQ(2, array[8]._bar); // previously at [2]
	EXPECT_EQ(3, array[9]._bar); // previously at [3]
}

TEST(DynamicArrayTest, testInsertMiddleInt) {
	const int buf[] = {
		0,
		1,
		2,
		3,
		4,
		5
	};
	DynamicArray<int> array;
	array.reserve(32);
	array.insert(array.begin(), buf, 6);
	array.insert(array.begin(), buf, 6);
	array.insert(core::next(array.begin(), 4), buf, 1);
	ASSERT_EQ(13u, array.size());
	EXPECT_EQ(0, array[0]);
	EXPECT_EQ(1, array[1]);
	EXPECT_EQ(2, array[2]);
	EXPECT_EQ(3, array[3]);
	EXPECT_EQ(0, array[4]);
	EXPECT_EQ(4, array[5]);
	EXPECT_EQ(5, array[6]);
	EXPECT_EQ(0, array[7]);
	EXPECT_EQ(1, array[8]);
	EXPECT_EQ(2, array[9]);
	EXPECT_EQ(3, array[10]);
	EXPECT_EQ(4, array[11]);
	EXPECT_EQ(5, array[12]);
}

TEST(DynamicArrayTest, testInsertMiddleDynamicArrayStruct) {
	const DynamicArrayStruct buf[] = {
		DynamicArrayStruct(core::String(1024, 'a'), 0),
		DynamicArrayStruct(core::String(1024, 'b'), 1),
		DynamicArrayStruct(core::String(4096, 'c'), 2),
		DynamicArrayStruct(core::String(1337, 'd'), 3),
		DynamicArrayStruct(core::String(0xEE, 'e'), 4),
		DynamicArrayStruct(core::String(0xFF, 'f'), 5)
	};
	DynamicArray<DynamicArrayStruct> array;
	array.reserve(32);
	array.insert(array.begin(), buf, 6);
	array.insert(array.begin(), buf, 6);
	array.insert(core::next(array.begin(), 4), buf, 1);
	ASSERT_EQ(13u, array.size());
	EXPECT_EQ(0, array[0]._bar);
	EXPECT_EQ(1, array[1]._bar);
	EXPECT_EQ(2, array[2]._bar);
	EXPECT_EQ(3, array[3]._bar);
	EXPECT_EQ(0, array[4]._bar);
	EXPECT_EQ(4, array[5]._bar);
	EXPECT_EQ(5, array[6]._bar);
	EXPECT_EQ(0, array[7]._bar);
	EXPECT_EQ(1, array[8]._bar);
	EXPECT_EQ(2, array[9]._bar);
	EXPECT_EQ(3, array[10]._bar);
	EXPECT_EQ(4, array[11]._bar);
	EXPECT_EQ(5, array[12]._bar);
}

TEST(DynamicArrayTest, testInsertIterMultiple) {
	const DynamicArrayStruct buf[] = {
		DynamicArrayStruct(core::String(1024, 'a'), 0),
		DynamicArrayStruct(core::String(1024, 'b'), 1),
		DynamicArrayStruct(core::String(4096, 'c'), 2),
		DynamicArrayStruct(core::String(1337, 'd'), 3),
		DynamicArrayStruct(core::String(0xEE, 'e'), 4),
		DynamicArrayStruct(core::String(0xFF, 'f'), 5)
	};
	DynamicArray<DynamicArrayStruct> other;
	other.insert(other.begin(), buf, 6);

	DynamicArray<DynamicArrayStruct> array;
	array.insert(array.begin(), other.begin(), other.end());
}

TEST(DynamicArrayTest, testInsertIteratorDistance) {
	const DynamicArrayStruct buf[] = {
		DynamicArrayStruct(core::String(1024, 'a'), 0),
		DynamicArrayStruct(core::String(1024, 'b'), 1),
		DynamicArrayStruct(core::String(4096, 'c'), 2),
		DynamicArrayStruct(core::String(1337, 'd'), 3),
		DynamicArrayStruct(core::String(0xEE, 'e'), 4),
		DynamicArrayStruct(core::String(0xFF, 'f'), 5)
	};
	DynamicArray<DynamicArrayStruct> other;
	other.insert(other.begin(), buf, 6);
	EXPECT_EQ(6, other.end() - other.begin());
}

TEST(DynamicArrayTest, testInsertIteratorOperatorInt) {
	const DynamicArrayStruct buf[] = {
		DynamicArrayStruct(core::String(1024, 'a'), 0),
		DynamicArrayStruct(core::String(1024, 'b'), 1),
		DynamicArrayStruct(core::String(4096, 'c'), 2),
		DynamicArrayStruct(core::String(1337, 'd'), 3),
		DynamicArrayStruct(core::String(0xEE, 'e'), 4),
		DynamicArrayStruct(core::String(0xFF, 'f'), 5)
	};
	DynamicArray<DynamicArrayStruct> other;
	other.insert(other.begin(), buf, 6);
	auto iter = other.begin();
	for (int i = 0; i < 6; ++i) {
		DynamicArrayStruct s = *iter;
		EXPECT_EQ(i, s._bar);
		s = *iter++;
		EXPECT_EQ(i, s._bar);
	}
	EXPECT_EQ(6, other.end() - other.begin());
}

}
