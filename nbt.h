#ifndef _NBT
#define _NBT

#ifndef _NBT_NO_COMPRESS
#include <zlib/zlib.h>
#define _NBT_GZIP_MAGIC 0x1f
#define _NBT_GZIP_CHUNK 0x2000
#endif

#include <string>
#include <cstdio>
#include <numbers>
#include <exception>
#include "Stream/ByteStream.h"
#include "Stream/ByteOutStream.h"
#include <unordered_map>
#include <vector>

namespace nbt {

	class exception : public std::exception {
	public:
		explicit inline exception(const char* const msg) throw() : std::exception(msg) {}
		virtual char const* what() const override {
			return std::exception::what();//..
		}

	};

	class size_tracker {
		const std::int64_t m_max;
	public:
		std::int64_t m_read;

		void read(std::uint64_t bits) {
			m_read += bits / 8l;
			if (m_read > m_max)
				throw exception("Tried to read NBT tag that was too big");
		}

		constexpr size_tracker(const std::int64_t max) : m_max(max), m_read(0) {}

	};

	constexpr size_tracker inf(std::numeric_limits<std::int64_t>::max());

	class base {
	public:

		static base* create(std::int8_t id);

		static constexpr const char* const types[] = {
			"end",
			"byte",
			"short",
			"int",
			"long",
			"float",
			"double",
			"byte[]",
			"string",
			"list",
			"compound",
			"int[]",
			"long[]",
		};

		virtual void write(byteoutstream&) = 0;
		virtual void read(bytestream&, int depth, size_tracker&) = 0;
		inline virtual std::int8_t get_id() const = 0;

		virtual ~base() {}

		static constexpr const char* const get_typename(int id) {
			switch (id) {
			case 0:
				return "TAG_End";
			case 1:
				return "TAG_Byte";
			case 2:
				return "TAG_Short";
			case 3:
				return "TAG_Int";
			case 4:
				return "TAG_Long";
			case 5:
				return "TAG_Float";
			case 6:
				return "TAG_Double";
			case 7:
				return "TAG_Byte_Array";
			case 8:
				return "TAG_String";
			case 9:
				return "TAG_List";
			case 10:
				return "TAG_Compound";
			case 11:
				return "TAG_Int_Array";
			case 12:
				return "TAG_Long_Array";
			case 99:
				return "Any Numeric Tag";
			default:
				break;
			}
			return "UNKNOWN";
		}

		const char* const get_tn() const {
			return get_typename(get_id());
		}


		inline virtual bool is_empty() const {
			return false;
		}

		inline bool operator==(const base& rhs) const {
			return rhs.get_id() == get_id();
		}

	};

	class primitive : public virtual base {
	public:
		inline virtual std::int64_t get_long() const = 0;
		inline virtual std::int32_t get_int() const = 0;
		inline virtual std::int16_t get_short() const = 0;
		inline virtual std::int8_t get_byte() const = 0;
		inline virtual double get_double() const = 0;
		inline virtual float get_float() const = 0;
	};

	template<class T>
	class typed : public virtual base {
	public:
		static constexpr const std::int32_t t_size = sizeof(T);
	};

	template<class T>
	class typed_primitive : public typed<T>, public primitive {};

	class tag_byte : public typed_primitive<std::int8_t> {
	public:

		std::int8_t m_data;
		tag_byte() = default;

		inline virtual std::int8_t get_id() const {
			return 1;
		}

		virtual void write(byteoutstream& out) override {
			out.write_int(8, m_data);
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(72);
			m_data = (int8_t)input.read();
		}

		inline virtual std::int16_t get_short() const override {
			return static_cast<std::int16_t>(m_data);
		}

		inline virtual std::int64_t get_long() const override {
			return static_cast<std::int64_t>(m_data);
		}

		inline virtual std::int32_t get_int() const override {
			return static_cast<std::int32_t>(m_data);
		}

		inline virtual std::int8_t get_byte() const override {
			return m_data;
		}

		inline virtual float get_float() const override {
			return static_cast<float>(m_data);
		}

		inline virtual double get_double() const override {
			return static_cast<double>(m_data);
		}

	};

	class tag_bytearray : public typed<std::int8_t*> {
	public:

		std::int8_t *mp_data;
		int m_dataSize;
		tag_bytearray() : mp_data(NULL), m_dataSize(0) {}

		tag_bytearray(const tag_bytearray&) = delete;
		tag_bytearray& operator=(const tag_bytearray&) = delete;

		tag_bytearray(tag_bytearray&& rhs) {

			clear_buffer();
			mp_data = rhs.mp_data;
			m_dataSize = rhs.m_dataSize;
			rhs.mp_data = NULL;
			rhs.m_dataSize = 0;
		}

		tag_bytearray& operator=(tag_bytearray&& rhs) {
			clear_buffer();
			mp_data = rhs.mp_data;
			m_dataSize = rhs.m_dataSize;
			rhs.mp_data = NULL;
			rhs.m_dataSize = 0;
			return *this;
		}

		inline virtual std::int8_t get_id() const {
			return 7;
		}

		virtual void write(byteoutstream& output) override {
			output.write_int(32, m_dataSize);
			if (m_dataSize && mp_data)
				output.write((uint8*)mp_data, m_dataSize);
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			clear_buffer();
			size_tracker.read(192);
			m_dataSize = input.read_int(32);
			if (m_dataSize) {
				size_tracker.read(8 * 1 * m_dataSize);
				mp_data = new std::int8_t[m_dataSize];
				input.read_to((uint8*)mp_data, m_dataSize);
			}
		}

		inline void clear_buffer() {
			if (m_dataSize && mp_data)
				delete[] mp_data;
			m_dataSize = 0;
			mp_data = NULL;
		}

		virtual ~tag_bytearray() {
			clear_buffer();
		}

	};

	class tag_double : public typed_primitive<double> {
	public:

		double m_data;
		tag_double() = default;

		inline virtual std::int8_t get_id() const {
			return 6;
		}

		virtual void write(byteoutstream& out) override {
			out.write_int(64, *(uint64*)&m_data);
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(128);
			uint64 i = input.read_int(64);
			m_data = *(double*)&i;
		}

		inline virtual std::int16_t get_short() const override {
			return static_cast<std::int16_t>(m_data);
		}

		inline virtual std::int64_t get_long() const override {
			return static_cast<std::int64_t>(m_data);
		}

		inline virtual std::int32_t get_int() const override {
			return static_cast<std::int32_t>(m_data);
		}

		inline virtual std::int8_t get_byte() const override {
			return static_cast<std::uint8_t>(m_data);
		}

		inline virtual float get_float() const override {
			return static_cast<float>(m_data);
		}

		inline virtual double get_double() const override {
			return m_data;
		}

	};

	class tag_float : public typed_primitive<float> {
	public:

		float m_data;
		tag_float() = default;

		inline virtual std::int8_t get_id() const {
			return 5;
		}

		virtual void write(byteoutstream& out) override {
			out.write_int(32, *(uint32*)&m_data);
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(96);
			uint32 i = (uint32)input.read_int(32);
			m_data = *(float*)&i;
		}

		inline virtual std::int16_t get_short() const override {
			return static_cast<std::int16_t>(m_data);
		}

		inline virtual std::int64_t get_long() const override {
			return static_cast<std::int64_t>(m_data);
		}

		inline virtual std::int32_t get_int() const override {
			return static_cast<std::int32_t>(m_data);
		}

		inline virtual std::int8_t get_byte() const override {
			return static_cast<std::uint8_t>(m_data);
		}

		inline virtual float get_float() const override {
			return static_cast<float>(m_data);
		}

		inline virtual double get_double() const override {
			return static_cast<double>(m_data);
		}

	};

	class tag_short : public typed_primitive<std::int16_t> {
	public:

		std::int16_t m_data;
		tag_short() = default;

		inline virtual std::int8_t get_id() const {
			return 2;
		}

		virtual void write(byteoutstream& out) override {
			out.write_int(16, m_data);
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(80);
			m_data = (int8_t)input.read_int(16);
		}

		inline virtual std::int16_t get_short() const override {
			return static_cast<std::int16_t>(m_data);
		}

		inline virtual std::int64_t get_long() const override {
			return static_cast<std::int64_t>(m_data);
		}

		inline virtual std::int32_t get_int() const override {
			return static_cast<std::int32_t>(m_data);
		}

		inline virtual std::int8_t get_byte() const override {
			return m_data;
		}

		inline virtual float get_float() const override {
			return static_cast<float>(m_data);
		}

		inline virtual double get_double() const override {
			return static_cast<double>(m_data);
		}

	};

	class tag_int : public typed_primitive<std::int32_t> {
	public:

		std::int32_t m_data;
		tag_int() = default;

		inline virtual std::int8_t get_id() const {
			return 3;
		}

		virtual void write(byteoutstream& out) override {
			out.write_int(32, m_data);
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(96);
			m_data = (int32_t)input.read_int(32);
		}

		inline virtual std::int16_t get_short() const override {
			return static_cast<std::int16_t>(m_data);
		}

		inline virtual std::int64_t get_long() const override {
			return static_cast<std::int64_t>(m_data);
		}

		inline virtual std::int32_t get_int() const override {
			return static_cast<std::int32_t>(m_data);
		}

		inline virtual std::int8_t get_byte() const override {
			return m_data;
		}

		inline virtual float get_float() const override {
			return static_cast<float>(m_data);
		}

		inline virtual double get_double() const override {
			return static_cast<double>(m_data);
		}

	};

	class tag_long : public typed_primitive<std::int64_t> {
	public:

		std::int64_t m_data;
		tag_long() = default;

		inline virtual std::int8_t get_id() const {
			return 2;
		}

		virtual void write(byteoutstream& out) override {
			out.write_int(64, m_data);
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(128);
			m_data = (int64_t)input.read_int(64);
		}

		inline virtual std::int16_t get_short() const override {
			return static_cast<std::int16_t>(m_data);
		}

		inline virtual std::int64_t get_long() const override {
			return static_cast<std::int64_t>(m_data);
		}

		inline virtual std::int32_t get_int() const override {
			return static_cast<std::int32_t>(m_data);
		}

		inline virtual std::int8_t get_byte() const override {
			return m_data;
		}

		inline virtual float get_float() const override {
			return static_cast<float>(m_data);
		}

		inline virtual double get_double() const override {
			return static_cast<double>(m_data);
		}

	};

	class tag_intarray : public typed<std::int32_t*> {
	public:

		std::int32_t* mp_data;
		int m_dataSize;
		tag_intarray() : mp_data(NULL), m_dataSize(0) {}

		tag_intarray(const tag_intarray&) = delete;
		tag_intarray& operator=(const tag_intarray&) = delete;

		tag_intarray(tag_intarray&& rhs) {

			clear_buffer();
			mp_data = rhs.mp_data;
			m_dataSize = rhs.m_dataSize;
			rhs.mp_data = NULL;
			rhs.m_dataSize = 0;
		}

		tag_intarray& operator=(tag_intarray&& rhs) {
			clear_buffer();
			mp_data = rhs.mp_data;
			m_dataSize = rhs.m_dataSize;
			rhs.mp_data = NULL;
			rhs.m_dataSize = 0;
			return *this;
		}

		inline virtual std::int8_t get_id() const {
			return 11;
		}

		virtual void write(byteoutstream& output) override {
			output.write_int(32, m_dataSize);
			if (m_dataSize && mp_data) {
				for (int i = 0; i < m_dataSize; i++)
					output.write_int(32, mp_data[i]);
			}
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			clear_buffer();
			size_tracker.read(192);
			m_dataSize = input.read_int(32);
			if (m_dataSize) {
				size_tracker.read(8 * 4 * m_dataSize);
				mp_data = new std::int32_t[m_dataSize];
				for (int i = 0; i < m_dataSize; i++)
					mp_data[i] = input.read_int(32);
			}
		}

		inline void clear_buffer() {
			if (m_dataSize && mp_data)
				delete[] mp_data;
			m_dataSize = 0;
			mp_data = NULL;
		}

		virtual ~tag_intarray() {
			clear_buffer();
		}

	};

	class tag_longarray : public typed<std::int64_t*> {
	public:

		std::int64_t* mp_data;
		int m_dataSize;
		tag_longarray() : mp_data(NULL), m_dataSize(0) {}

		tag_longarray(const tag_longarray&) = delete;
		tag_longarray& operator=(const tag_longarray&) = delete;

		tag_longarray(tag_longarray&& rhs) {

			clear_buffer();
			mp_data = rhs.mp_data;
			m_dataSize = rhs.m_dataSize;
			rhs.mp_data = NULL;
			rhs.m_dataSize = 0;
		}

		tag_longarray& operator=(tag_longarray&& rhs) {
			clear_buffer();
			mp_data = rhs.mp_data;
			m_dataSize = rhs.m_dataSize;
			rhs.mp_data = NULL;
			rhs.m_dataSize = 0;
			return *this;
		}

		inline virtual std::int8_t get_id() const {
			return 12;
		}

		virtual void write(byteoutstream& output) override {
			output.write_int(32, m_dataSize);
			if (m_dataSize && mp_data) {
				for (int i = 0; i < m_dataSize; i++)
					output.write_int(64, mp_data[i]);
			}
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			clear_buffer();
			size_tracker.read(192);
			m_dataSize = input.read_int(32);
			if (m_dataSize) {
				size_tracker.read(8 * 8 * m_dataSize);
				mp_data = new std::int64_t[m_dataSize];
				for (int i = 0; i < m_dataSize; i++)
					mp_data[i] = input.read_int(64);
			}
		}

		inline void clear_buffer() {
			if (m_dataSize && mp_data)
				delete[] mp_data;
			m_dataSize = 0;
			mp_data = NULL;
		}

		virtual ~tag_longarray() {
			clear_buffer();
		}

	};

	class tag_string : public base {
	public:

		std::string m_data;

		inline virtual std::int8_t get_id() const {
			return 8;
		}

		virtual void write(byteoutstream& output) override {
			if (m_data.length() > std::numeric_limits<std::uint16_t>::max())
				throw exception("cannot write string: more than 2^16-1 bytes");
			output.write_int(16, m_data.length());
			if(m_data.length() > 0)
				output.write((const uint8*)m_data.c_str(), m_data.length());
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(36 * 8);
			uint32 size = input.read_int(16);
			size_tracker.read(16 * size);
			uint8* buffer = input.read(size);
			m_data.assign((const char*)buffer, size);
			free(buffer);
		}

		virtual bool is_empty() const override {
			return m_data.empty();//delegate
		}

	};

	class tag_compound : public base {
	public:

		std::unordered_map<std::string, base*> m_tagMap;

		inline virtual std::int8_t get_id() const {
			return 10;
		}

		virtual void write(byteoutstream& output) override {
			tag_string name_proxy{};
			std::uint8_t id;
			for (auto it = m_tagMap.begin(); it != m_tagMap.end(); it++) {
				id = it->second->get_id();
				if (id != 0) {//!=end
					output.write_int(8, id);
					name_proxy.m_data = it->first;
					name_proxy.write(output);
					it->second->write(output);
				}
			}
			output.write_int(8, 0);//end footer
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(384);
			if (depth > 0x200)
				throw exception("Tried to read NBT with too high complexity, depth > 512");
			clear();
			std::uint64_t id;
			tag_string name_proxy{};
			while ((id = input.read_int(8)) != 0) {
				name_proxy.read(input, depth, size_tracker);//size off by a few bytes, not important (288-224)
				base* tag = base::create(static_cast<std::int8_t>(id));
				if (!tag)
					throw exception("error reading compound tag: tag id invalid. corrupt tag?");
				tag->read(input, depth + 1, size_tracker);
				m_tagMap.insert(std::make_pair(name_proxy.m_data, tag));
				size_tracker.read(288);
			}
		}

		void clear() {
			for (auto it = m_tagMap.begin(); it != m_tagMap.end(); it++) {
				delete it->second;
			}
			m_tagMap.clear();
		}

		~tag_compound() {
			clear();
		}

	};

	class tag_list : public base {
		
		std::int8_t m_tagType;
		std::vector<base*> m_tagList;

	public:

		tag_list() : m_tagType(0) {}

		inline virtual std::int8_t get_id() const {
			return 9;
		}

		virtual void write(byteoutstream& output) override {
			output.write_int(8, m_tagType);
			output.write_int(32, m_tagList.size());
			for (auto it = m_tagList.begin(); it != m_tagList.end(); it++)
				(*it)->write(output);
		}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(296);
			if (depth > 0x200)
				throw exception("Tried to read NBT with too high complexity, depth > 512");
			m_tagType = input.read_int(8);
			size_t size = input.read_int(32);
			if (m_tagType == 0 && size > 0)
				throw exception("missing type on list tag");
			size_tracker.read(size * 32);
			for (int i = 0; i < size; i++) {
				base* tag = base::create(m_tagType);
				if (!tag)
					throw exception("error reading compound tag: tag id invalid. corrupt tag?");
				tag->read(input, depth + 1, size_tracker);
				m_tagList.push_back(tag);
			}
		}

		base* pop_tag() {
			base* end = m_tagList.back();
			m_tagList.pop_back();
			return end;
		}

		std::int8_t get_tag_type() {
			return m_tagType;
		}

		//WARNING: assumes transfer of ownership to this (i.e, deletes after done)
		void append_tag(base* tag) {
			if (!tag)
				throw exception("null tag passed to tag_list::append");
			if (tag->get_id() != 0) {//quietly
				if (m_tagList.size() == 0)
					m_tagType = tag->get_id();
				else if (m_tagType != tag->get_id())
					throw exception("trying to add tag of different type to list tag");
				m_tagList.push_back(tag);
			}
		}

		//check by ptr
		void remove_tag_direct(const base* const tag_ptr) {
			for (auto it = m_tagList.begin(); it != m_tagList.end(); it++)
				if (*it == tag_ptr) {
					m_tagList.erase(it);
					break;
				}
		}

		void clear() {
			for (auto it = m_tagList.begin(); it != m_tagList.end(); it++)
				delete* it;
			m_tagList.clear();
			m_tagType = 0;
		}

		~tag_list() {
			clear();
		}

	};

	class tag_end : public base {
	public:

		inline virtual std::int8_t get_id() const {
			return 0;
		}

		virtual void write(byteoutstream& output) override {}

		virtual void read(bytestream& input, int depth, size_tracker& size_tracker) override {
			size_tracker.read(64);
		}

	};

	inline void write_tag(byteoutstream& output, base* input) {
		if (input) {
			endian e = output.get_endian();
			output.set_endian(BIG_ENDIAN);
			output.write_int(8, input->get_id());
			output.write_int(16, 0);//empty utf
			input->write(output);
			output.set_endian(e);
		}
	}

	inline base* read_tag(bytestream& input, size_tracker& tracker) {
		endian e = input.get_endian();
		input.set_endian(BIG_ENDIAN);
		std::int8_t head = input.read();
#ifndef _NBT_NO_COMPRESS
		if (head == _NBT_GZIP_MAGIC) {//compressed
			input.seek_beg(input.get_position() - 1);
			z_stream stream = { 0 };
			uint8* in = input.read((uint32)input.get_stream_size());//load it all, nbt depth prevents bigger files. shouldnt be >1gb
			byteoutstream out_buf = byteoutstream(_NBT_GZIP_CHUNK);
			out_buf.keep_buffer(true);
			uint8 out[_NBT_GZIP_CHUNK];
			memset(out, 0, _NBT_GZIP_CHUNK);
			stream.zalloc = Z_NULL;
			stream.zfree = Z_NULL;
			stream.opaque = 0;
			stream.avail_in = 0;
			stream.next_in = in;
			uint64 z = 0;
			int stat;
			stream.avail_in = (uint32)input.get_stream_size();
			inflateInit2(&stream, 47);//15, add mask of 32 (1bit) to enable gz
			do {
				stream.avail_out = _NBT_GZIP_CHUNK;
				stream.next_out = out;
				stat = inflate(&stream, Z_NO_FLUSH);
				if (!(stat == Z_OK || stat == Z_STREAM_END || stat == Z_BUF_ERROR)) {
					inflateEnd(&stream);
					throw exception("bad gzip compressed data");
				}
				z += (_NBT_GZIP_CHUNK - stream.avail_out);
				out_buf.write(out, _NBT_GZIP_CHUNK - stream.avail_out);

			} while (stream.avail_out == 0);
			inflateEnd(&stream);
			free(in);
			bytestream nstream = bytestream(out_buf.get_buffer(), z);
			return read_tag(nstream, tracker);
		}
#endif
		base* tag = base::create((std::int8_t)head);
		if (!tag)
			throw exception("tag not created (invalid/out of mem)");
		input.seek_cur(input.read_int(16));
		tag->read(input, 0, tracker);
		input.set_endian(e);
	}

	inline base* read_tag(bytestream& input) {
		size_tracker _tracker = size_tracker(inf);
		return read_tag(input, _tracker);
	}

	inline void read_tag_compound(bytestream& input, tag_compound& output, size_tracker& tracker) {
		endian e = input.get_endian();
		input.set_endian(BIG_ENDIAN);
		std::int8_t head = input.read_int(8);
#ifndef _NBT_NO_COMPRESS
		if (head == _NBT_GZIP_MAGIC) {//compressed
			input.seek_beg(input.get_position() - 1);
			z_stream stream = { 0 };
			uint8* in = input.read((uint32)input.get_stream_size());//load it all, nbt depth prevents bigger files. shouldnt be >1gb
			byteoutstream out_buf = byteoutstream(_NBT_GZIP_CHUNK);
			out_buf.keep_buffer(true);
			uint8 out[_NBT_GZIP_CHUNK];
			memset(out, 0, _NBT_GZIP_CHUNK);
			stream.zalloc = Z_NULL;
			stream.zfree = Z_NULL;
			stream.opaque = 0;
			stream.avail_in = 0;
			stream.next_in = in;
			uint64 z=0;
			int stat;
			stream.avail_in = (uint32)input.get_stream_size();
			inflateInit2(&stream, 47);//15, add mask of 32 (1bit) to enable gz
			do {
				stream.avail_out = _NBT_GZIP_CHUNK;
				stream.next_out = out;
				stat = inflate(&stream, Z_NO_FLUSH);
				if (!(stat == Z_OK || stat == Z_STREAM_END || stat == Z_BUF_ERROR)) {
					inflateEnd(&stream);
					throw exception("bad gzip compressed data");
				}
				z += (_NBT_GZIP_CHUNK - stream.avail_out);
				out_buf.write(out, _NBT_GZIP_CHUNK - stream.avail_out);

			} while (stream.avail_out == 0);
			inflateEnd(&stream);
			free(in);
			bytestream nstream = bytestream(out_buf.get_buffer(), z);
			read_tag_compound(nstream, output, tracker);
			return;
		}
#endif
		if (head != output.get_id())
			throw exception("not a compound tag");
		input.seek_cur(input.read_int(16));
		output.read(input, 0, tracker);
		input.set_endian(e);
	}

	inline void read_tag_compound(bytestream& input, tag_compound& output) {
		size_tracker _tracker = size_tracker(inf);
		read_tag_compound(input, output, _tracker);
	}

	base* base::create(std::int8_t id) {
		switch (id) {
		case 0:
			return new tag_end;
		case 1:
			return new tag_byte;
		case 2:
			return new tag_short;
		case 3:
			return new tag_int;
		case 4:
			return new tag_long;
		case 5:
			return new tag_float;
		case 6:
			return new tag_double;
		case 7:
			return new tag_bytearray;
		case 8:
			return new tag_string;
		case 9:
			return new tag_list;
		case 10:
			return new tag_compound;
		case 11:
			return new tag_intarray;
		case 12:
			return new tag_longarray;
		default:
			return NULL;
		}
	}

}

#endif
