#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include <gtest/gtest.h>

namespace io {

class FileTest : public testing::Test {};

TEST_F(FileTest, testGetPath) {
	io::Filesystem fs;
	fs.init("test", "test");
	const io::FilePtr &file = fs.open("foobar/1.txt", io::FileMode::Read);
	ASSERT_TRUE(core::string::endsWith(file->path(), "foobar/"));
	ASSERT_EQ("txt", file->extension());
	ASSERT_EQ("1", file->fileName());
	ASSERT_TRUE(core::string::endsWith(file->name(), "foobar/1.txt"));
	ASSERT_FALSE(file->exists());
}

TEST_F(FileTest, testLoad) {
	io::Filesystem fs;
	fs.init("test", "test");
	const io::FilePtr &file = fs.open("iotest.txt", io::FileMode::Read);
	const core::String &str = file->load();
	ASSERT_NE("", str);
}

} // namespace io
