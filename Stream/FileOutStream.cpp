#pragma once
#include "fileoutstream.h"

void fileoutstream::write(const uint8* buf, uint32 size) {
	if (this->position + size > this->size)this->grow(this->size + size);
	fseek(file, position, SEEK_SET);
	fwrite(buf, size, 1, this->file);
	this->position += size;
}

fileoutstream::fileoutstream(const char* fp) {
	if (!fp) {
		this->v = false;
		return;
	}
	FILE* f;
	fopen_s(&f, fp, "wb");
	if(!f) {
		this->v = false;
		return;
	}
	this->mark = 0;
	this->order = LITTLE_ENDIAN;
	this->position = ftell(f);
	fseek(f, 0, SEEK_END);
	this->size = ftell(f);
	fseek(f, this->position, SEEK_SET);
	this->file = f;
}

fileoutstream::~fileoutstream() {
	if (this->buf)free(this->buf);
	if (this->v)fclose(this->file);
}

FILE* fileoutstream::get_out_file() {
	return this->file;
}

void fileoutstream::grow(uint64 dest_size) {
	//if (DEFAULT_BUF_INCREMENT > dest_size - this->size) {
	//	grow(DEFAULT_BUF_INCREMENT + this->size);
	//	return;
	//}
	uint8* buf = (uint8*)calloc(1,dest_size - this->size);
	if (!buf)throw "out of memory";
	fseek(this->file, this->size, SEEK_SET);
	fwrite(buf, dest_size - this->size, 1, this->file);
	fseek(this->file, this->position, SEEK_SET);
	this->size = dest_size;
	free(buf);
}

fileoutstream::fileoutstream(FILE* f) {
	if (!f) {
		this->v = false;
		this->file = NULL;
		return;
	}
	this->byteoutstream::byteoutstream(0);
	this->position = ftell(f);
	fseek(f, 0, SEEK_END);
	this->size = ftell(f);
	fseek(f, this->position, SEEK_SET);
	this->file = f;
}
