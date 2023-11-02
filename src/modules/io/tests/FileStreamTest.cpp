/**
 * @file
 */

#include "io/FileStream.h"
#include "core/FourCC.h"
#include "io/Filesystem.h"
#include <gtest/gtest.h>

namespace io {

class FileStreamTest : public testing::Test {
protected:
	io::Filesystem _fs;

public:
	void SetUp() override {
		_fs.init("test", "test");
	}

	void TearDown() override {
		_fs.shutdown();
	}
};

TEST_F(FileStreamTest, testInvalidFile) {
	const FilePtr file;
	FileStream stream(file);
	EXPECT_TRUE(stream.empty());
	EXPECT_TRUE(stream.eos());
	EXPECT_EQ(0, stream.size());
	int8_t val = 0;
	EXPECT_EQ(-1, stream.readInt8(val));
	EXPECT_FALSE(stream.writeInt8(val));
	EXPECT_EQ(0, stream.size());
}

TEST_F(FileStreamTest, testFileStreamReadPastEndOfFile) {
	const FilePtr &file = _fs.open("iotest.txt");
	int8_t val = 0;
	FileStream stream(file);
	EXPECT_EQ(0, stream.readInt8(val));
	EXPECT_TRUE(stream.seek(0, SEEK_END) > 0);
	EXPECT_EQ(-1, stream.readInt8(val));
}

TEST_F(FileStreamTest, testFileStreamRead) {
	const FilePtr &file = _fs.open("iotest.txt");
	ASSERT_TRUE(file->exists());

	FileStream stream(file);
	const int64_t remaining = stream.remaining();
	EXPECT_EQ((int64_t)file->length(), remaining);

	uint32_t magic;
	EXPECT_EQ(0, stream.peekUInt32(magic)) << "Error in peekInt";
	EXPECT_EQ(0, stream.pos()) << "peekInt should not modify the position of the stream";
	EXPECT_EQ(remaining, stream.remaining()) << "Error regarding position tracking of peek method. Position should be 0 but is: " << stream.pos();
	EXPECT_EQ(FourCC('W', 'i', 'n', 'd'), magic) << "Failed to read the proper value in peekInt";

	uint8_t chr;
	EXPECT_EQ(0, stream.readUInt8(chr));
	EXPECT_EQ(remaining, stream.remaining() + 1);
	EXPECT_EQ('W', chr);
	EXPECT_EQ(0, stream.readUInt8(chr));
	EXPECT_EQ(remaining, stream.remaining() + 2);
	EXPECT_EQ('i', chr);
	EXPECT_EQ(0, stream.readUInt8(chr));
	EXPECT_EQ(remaining, stream.remaining() + 3);
	EXPECT_EQ('n', chr);
	EXPECT_EQ(0, stream.peekUInt8(chr));
	EXPECT_EQ(remaining, stream.remaining() + 3);
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekUInt8(chr));
	EXPECT_EQ(remaining, stream.remaining() + 3);
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekUInt8(chr));
	EXPECT_EQ(remaining, stream.remaining() + 3);
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.readUInt8(chr));
	EXPECT_EQ(remaining, stream.remaining() + 4);
	EXPECT_EQ('d', chr);
	EXPECT_EQ(0, stream.peekUInt8(chr));
	EXPECT_EQ(remaining, stream.remaining() + 4);
	EXPECT_EQ('o', chr);
	char buf[8];
	stream.readString(6, buf);
	EXPECT_EQ(remaining, stream.remaining() + 10);
	buf[6] = '\0';
	EXPECT_STREQ("owInfo", buf);
}

TEST_F(FileStreamTest, testFileStreamWrite) {
	io::Filesystem fs;
	EXPECT_TRUE(fs.init("test", "test")) << "Failed to initialize the filesystem";
	const FilePtr &file = fs.open("filestream-writetest", io::FileMode::SysWrite);
	ASSERT_TRUE(file->validHandle());
	FileStream stream(file);
	EXPECT_TRUE(stream.writeUInt32(1));
	EXPECT_EQ(4l, stream.size());
	EXPECT_TRUE(stream.writeUInt32(1));
	EXPECT_EQ(8l, stream.size());
	file->close();
	file->open(io::FileMode::Read);
	EXPECT_TRUE(file->exists());
	EXPECT_EQ(8l, file->length());
}

} // namespace io
