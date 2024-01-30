/**
 * @file
 */

#pragma once

#include "io/Archive.h"
#include "io/Stream.h"

namespace io {

/**
 * @ingroup IO
 */
class ZipArchive : public Archive {
private:
	void *_zip = nullptr;
	void reset();

public:
	ZipArchive();
	virtual ~ZipArchive();

	static bool validStream(io::SeekableReadStream &stream);

	bool init(const core::String &path, io::SeekableReadStream *stream) override;
	bool load(const core::String &filePath, io::SeekableWriteStream &out) override;
	void shutdown() override;
};

} // namespace io
