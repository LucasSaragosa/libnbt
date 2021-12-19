#include "bytestream.h"
#include <cstring>
#include <stdlib.h>
#include "ByteOutStream.h"
#include <iostream>

using namespace std;

bool bytestream::seek_beg(uint64 pos) {
	if (pos > this->get_stream_size())return false;
	this->pos = pos;
	return true;
}

bytestream::bytestream(const char* str) : bytestream(){
	if (!str)throw "no buffer";
	this->size = strlen(str);
	this->buf = (uint8*)malloc(this->size);
	memcpy(this->buf, str, this->size);
}

uint8* bytestream::get_buffer() {
	return this->buf;
}

bool bytestream::valid() {
	return v ? true : (bool)this->buf;
}

bytestream::bytestream(uint8* buf, uint64 size) : bytestream() {
	this->size = size;
	this->buf = buf;
}

uint8* bytestream::read(uint32 size) {
	if (this->buf == nullptr) {
		throw "no buffer";
	}
	if (this->pos + size > this->size) {
		throw "cannot read that many bytes";
	}
	uint8* out = (uint8*)malloc(size);
	memcpy(out, this->buf + this->pos, size);
	this->pos += size;
	return out;
}

uint8 bytestream::read() {
	static uint8 buf = 0;
	read_to(&buf, 1);
	return buf;
}

unsigned char* bytestream::read_string() {
	byteoutstream* out = new byteoutstream(512);
	uint32 z = 0;
	while(this->pos <= this->get_stream_size()){
		unsigned char read = this->read();
		z++;
		out->write_int(8, read);
		if (!read) {
			out->write_int(8, 0);
			break;
		}
	}
	unsigned char* buf = (unsigned char*)malloc(z);
	memcpy(buf, out->get_buffer(), z);
	delete out;
	return buf;
}

unsigned char* bytestream::read_string(uint32 size) {
	unsigned char* buf = (unsigned char*)calloc(size + 1,1);
	if (!buf)return 0;
	unsigned char* read = this->read(size);
	memcpy(buf, read, size);
	delete read;
	return buf;
}

uint64 bytestream::read_int(uint8 width) {
	if (width % 8 != 0)throw "bad int width, has to be a multiple of 8";
	uint64 ret = 0;
	width /= 8;
	uint8* buff = this->read(width);
	if (!buff)return NULL;
	if (this->order == LITTLE_ENDIAN) {
		int x = 0;
		for (int i = 0; i < width; i++) {
			ret |= ((uint64)buff[x] << ((uint64)i << (uint64)3));
			x++;
		}
	} else {
		int x = 0;
		for (int i = width - 1; i >= 0; i++) {
			ret |= buff[x] << (i * 8);
			x++;
		}
	}
	free(buff);
	return ret;
}

bool bytestream::seek_end(uint64 pos) {
	int64 to = this->size - pos;
	if (0 > to)return false;
	this->seek_beg(to);
	return true;
}

bool bytestream::seek_cur(uint64 pos) {
	uint64 to = this->pos + pos;
	if (to > this->size)return false;
	this->seek_beg(to);
	return true;
}

void bytestream::set_endian(endian e) {
	if (!(e == LITTLE_ENDIAN || e == BIG_ENDIAN))return;
	this->order = e;
}

void bytestream::rewind() {
	this->pos = this->mark;
}

uint64 bytestream::get_mark() {
	return this->mark;
}

uint64 bytestream::get_position() {
	return this->pos;
}

uint64 bytestream::get_stream_size() {
	return this->size;
}

endian bytestream::get_endian() {
	return this->order;
}

void bytestream::mark_pos(uint64 pos) {
	if (pos > this->get_stream_size())return;
	this->mark = pos;
}

void bytestream::read_to(uint8* buf, uint32 size) {
	uint8* bufsrc = this->read(size);
	memcpy(buf, bufsrc, size);
	free(bufsrc);
}

void bytestream::keep_buffer(bool b) {
	this->b = b;
}

bytestream::bytestream() {
	this->pos = 0;
	this->b = false;
	this->order = LITTLE_ENDIAN;
	this->mark = 0;
	this->size = 0;
}

bytestream::~bytestream() {
	this->pos = 0;
	this->order = 0;
	this->mark = 0;
	this->size = 0;
	if (this->buf && !b) {
		free(this->buf);
		this->buf = NULL;
	}
}
