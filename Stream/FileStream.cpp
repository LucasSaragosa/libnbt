#include "filestream.h"
#include <stdio.h>
#include <sys/stat.h> 

int file_exists(const char* filename) {
	struct stat   buffer;
	return stat(filename, &buffer);
}

bool filestream::valid() {
	return this->in;
}

filestream::filestream(const char* filepath) {
	fopen_s(&in, filepath, "rb");
	if (this->in) {
		this->pos = ftell(this->in);
		fseek(this->in, 0, SEEK_END);
		this->size = ftell(this->in);
		fseek(this->in, this->pos, SEEK_SET);
	}
	else this->v = false;
}

filestream::filestream(FILE * f) {
	this->order = LITTLE_ENDIAN;
	this->mark = 0;
	if (f == nullptr) {
		this->in = NULL;
		this->v = false;
		return;
	}
	this->in = f;
	this->pos = 0;
	fseek(f, 0, SEEK_END);
	this->size = ftell(f);
	fseek(f, this->pos, SEEK_SET);
	
}

uint8* filestream::read(uint32 size) {
	if (this->size < this->pos + size)return NULL;
	fseek(this->in, this->pos, SEEK_SET);
	uint8* buf = (uint8*)malloc(size);
	fread(buf, 1, size, this->in);
	this->pos += size;
	return buf;
}

filestream::~filestream() {
	if (this->in)fclose(this->in);
	this->bytestream::~bytestream();
}
