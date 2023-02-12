/**
 * @file
 */

#include "image/AVI.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "core/RGBA.h"
#include "core/StringUtil.h"
#include "image/Image.h"
#include "io/File.h"
#include "io/FileStream.h"

namespace image {

class AVITest : public app::AbstractTest {};

TEST_F(AVITest, testCreate) {
	AVI avi;
	const io::FilePtr &avifile = io::filesystem()->open("test.avi", io::FileMode::SysWrite);
	io::FileStream stream(avifile);
	ASSERT_TRUE(stream.valid());

	const core::RGBA r = core::RGBA(255, 0, 0);
	const core::RGBA b = core::RGBA(0, 0, 0);
	const core::RGBA img1[]{
		r, r, b, b, b, b,
		r, r, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
	};
	const core::RGBA img2[]{
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		r, r, b, b, b, b,
		r, r, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
	};
	const core::RGBA img3[]{
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, b, b, b, b,
		b, b, r, r, b, b,
		b, b, r, r, b, b,
	};
	ASSERT_TRUE(avi.open(stream, 6, 6));
	for (int i = 0; i < 100; ++i) {
		ASSERT_TRUE(avi.writeFrame(stream, (const uint8_t *)img1, 6, 6));
		ASSERT_TRUE(avi.writeFrame(stream, (const uint8_t *)img2, 6, 6));
		ASSERT_TRUE(avi.writeFrame(stream, (const uint8_t *)img3, 6, 6));
	}
	ASSERT_TRUE(avi.close(stream));
}

} // namespace image
