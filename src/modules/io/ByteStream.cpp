/**
 * @file
 */

#include "ByteStream.h"
#include <SDL_stdinc.h>
#include <stdarg.h>

namespace io {

ByteStream::ByteStream(int size) {
	_buffer.reserve(size);
}

int32_t ByteStream::peekInt() const {
	const int l = 4;
	if (size() < l) {
		return -1;
	}
	uint8_t buf[l];
	Buffer::const_iterator it = begin();
	for (int i = 0; i < l; ++i) {
		buf[i] = *it++;
	}
	const int32_t *word = (const int32_t*) (void*) buf;
	const int32_t val = SDL_SwapLE32(*word);
	return val;
}

int16_t ByteStream::peekShort() const {
	const int l = 2;
	if (size() < l) {
		return -1;
	}
	uint8_t buf[l];
	Buffer::const_iterator it = begin();
	for (int i = 0; i < l; ++i) {
		buf[i] = *it++;
	}
	const int16_t *word = reinterpret_cast<const int16_t*>(buf);
	const int16_t val = SDL_SwapLE16(*word);
	return val;
}

void ByteStream::addFormat(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b':
			addByte((uint8_t) va_arg(ap, int));
			break;
		case 's':
			addShort((uint16_t) va_arg(ap, int));
			break;
		case 'i':
			addInt((uint32_t) va_arg(ap, int));
			break;
		case 'l':
			addLong((uint64_t) va_arg(ap, long));
			break;
		default:
			core_assert(false);
		}
	}

	va_end(ap);
}

void ByteStream::readFormat(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		const char typeID = *fmt++;
		switch (typeID) {
		case 'b':
			*va_arg(ap, int *) = (int)readByte();
			break;
		case 's':
			*va_arg(ap, int *) = (int)readShort();
			break;
		case 'i':
			*va_arg(ap, int *) = (int)readInt();
			break;
		case 'l':
			*va_arg(ap, long *) = (long)readLong();
			break;
		default:
			core_assert(false);
		}
	}

	va_end(ap);
}

core::String ByteStream::readString() {
	core::String strbuff;
	strbuff.reserve(64);
	while (size() > 0) {
		const int8_t chr = (int8_t)readByte();
		if (chr == '\0') {
			break;
		}
		strbuff += chr;
	}
	return strbuff;
}

}
