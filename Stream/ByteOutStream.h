#include "byteoutstream.h"
#include <string>

endian byteoutstream::get_endian() {
	return this->order;
}

void byteoutstream::seek_beg(uint64 pos) {
	if (pos > this->size)this->grow(pos);
	this->position = pos;
}

void byteoutstream::set_endian(endian e) {
	this->order = e;
}

void byteoutstream::write_zeros(uint32 count) {
	uint8* buf = (uint8*)calloc(1, count);
	this->write(buf, count);
	free(buf);
}

void byteoutstream::keep_buffer(bool b) {
	this->b = b;
}

byteoutstream::~byteoutstream() {
	if (!this->b&&this->buf)free(this->buf);
}

void byteoutstream::write(const uint8* buf, uint32 size) {
	if (this->position + size > this->size)this->grow(this->size + size);
	this->buf += this->position;
	memcpy(this->buf, buf, size);
	this->buf -= this->position;
	this->position += size;
}


void byteoutstream::write_int(uint8 width, uint64 val) {
	if (width % 8 != 0)return;
	int bytes = width / 8;
	uint8* buffer = (uint8*)malloc(bytes);
	if (!buffer)throw "out of memory";
	if (this->order == LITTLE_ENDIAN) {
		for (int i = 0; i < bytes; i++) {
			buffer[i] = (uint64)val >> ((uint64)i << (uint64)3);
		}
	}
	else {
		uint64 mask = 0xFF << ((bytes-1) * 8);
		for (int i = 0; i < bytes; i++) {
			buffer[i] = (val & mask) >> ((bytes-i-1) * 8);
			mask >>= 8;
		}
	}
	this->write(buffer, bytes);
	free(buffer);
}

void byteoutstream::grow(uint64 dest_size) {
	uint8* tmp = (uint8*)realloc(this->buf, dest_size);
	if (!tmp)throw "out of memory";
	this->buf = tmp;
	this->size = dest_size;
}

void byteoutstream::seek_cur(uint64 pos) {
	this->seek_beg(this->position + pos);
}

void byteoutstream::seek_end(uint64 pos) {
	int64 boff = this->size - pos;
	if (0 > boff)return;
	this->seek_beg(boff);
}

byteoutstream::byteoutstream(uint32 s) {
	this->order = LITTLE_ENDIAN;
	this->size = s;
	this->v = true;
	this->mark = 0;
	this->position = 0;
	this->buf = s ? (uint8*)calloc(1,s) : NULL;
	this->b = false;
}

byteoutstream::byteoutstream() {
	this->position = 0;
	this->buf = NULL;
	this->order = LITTLE_ENDIAN;
	this->size = 0;
	this->v = true;
	this->mark = 0;
	this->b = false;
}

bool byteoutstream::valid() {
	return this->v;
}

uint64 byteoutstream::get_position() {
	return this->position;
}

uint64 byteoutstream::get_stream_size() {
	return this->size;
}

uint64 byteoutstream::get_mark() {
	return this->mark;
}

void byteoutstream::rewind() {
	if (this->mark > this->size)this->grow(this->mark);
	this->position = this->mark;
}

void byteoutstream::mark_pos(uint64 pos) {
	this->mark = pos;
}

uint8* byteoutstream::get_buffer() {
	return this->buf;
}

byteoutstream::byteoutstream(uint8* b, uint64 z) {
	this->byteoutstream::byteoutstream();
	if (!b) {
		this->v = 0;
		return;
	}
	this->buf = b;
	this->size = z;
}
