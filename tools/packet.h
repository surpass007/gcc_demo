#pragma once

#include <stdint.h>
#include "Blockbuffer.h"
#include <string>
#include <iterator>
#include <stdexcept>
#include <map>
#include <vector>
#include <set>
#include <string.h>
#include "SysDefine.h"
#ifdef SYS_WINDOWS
#include<winsock2.h>
#else
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

namespace gcc_demo
{
struct Varstr
{
	const char* m_data;
	size_t m_size;

	Varstr(const char* data = "", size_t size = 0)
	{
		set(data, size);
	}
	void set(const char* data, size_t size)
	{
		m_data = data;
		m_size = size;
	}
	bool empty() const
	{
		return m_size == 0;
	}

	const char* data() const
	{
		return m_data;
	}
	size_t size() const
	{
		return m_size;
	}

	template<class T> // std::string cstr blockbuffer
	explicit Varstr(T &s)
	{
		*this = s;
	}

	template<class T> Varstr &operator =(T &s)
	{
		m_data = s.data();
		m_size = s.size();
		return *this;
	}
};

inline bool isBigendian()
{
	union {
		uint16_t	n;
		uint8_t		s[2];
	} u;

	u.n = 0x0102;
	return 0x01 == u.s[0];
}

#define uint16reverse(n)	(((uint16_t)(n) & 0xff00) >> 8) | (((uint16_t)(n) & 0xff) << 8)

#define uint32reverse(n)	(((uint32_t)(n) & 0xff000000) >> 24) | (((uint32_t)(n) & 0xff0000) >> 8) | \
							(((uint32_t)(n) & 0xff00) << 8) | (((uint32_t)(n) & 0xff) << 24)

#define uint64reverse(n)	(((uint64_t)(n) & 0xff00000000000000) >> 56) | (((uint64_t)(n) & 0xff000000000000) >> 40) | \
							(((uint64_t)(n) & 0xff0000000000) >> 24) | (((uint64_t)(n) & 0xff00000000) >> 8) | \
							(((uint64_t)(n) & 0xff000000) << 8) | (((uint64_t)(n) & 0xff0000) << 24) | \
							(((uint64_t)(n) & 0xff00) << 40) | (((uint64_t)(n) & 0xff) << 56)

class PackBuffer
{
public:
	char* data()
	{
		return bb.data();
	}

	size_t size() const
	{
		return bb.size();
	}

	bool resize(size_t n)
	{
		return bb.resize(n);
	}

	bool append(const char* data, size_t size)
	{
		return bb.append(data, size);
	}

	bool append(const char* data)
	{
		return append(data, ::strlen(data));
	}

	bool replace(size_t pos, const char* rep, size_t n)
	{
		return bb.replace(pos, rep, n);
	}

	bool reserve(size_t n)
	{
		return bb.reserve(n);
	}

private:
	// use big-block. more BIG? MAX 64K*4k = 256M
	typedef BlockBuffer<def_block_alloc_4k, 65536> BB;
	BB bb;
};

class Pack
{
private:
	Pack(const Pack &o);
	Pack &operator = (const Pack &o);

public:
	uint16_t xhtons(uint16_t i16)
	{
		if (m_bLocal)
		{
			return i16;
		}
		else
		{
			return htons(i16);
		}
	}

	uint32_t xhtonl(uint32_t i32)
	{
		if (m_bLocal)
		{
			return i32;
		}
		else
		{
			return htonl(i32);
		}
	}

	uint64_t xhtonll(uint64_t i64)
	{
		if (m_bLocal)
		{
			return i64;
		}
		else
		{
			return isBigendian() ? i64 : uint64reverse(i64);
		}
	}

	// IMPORTANT remember the buffer-size before pack. see data(), size()
	// reserve a space to replace packet header after pack parameter
	// sample below: OffPack. see data(), size()
	Pack(PackBuffer &pb, size_t off = 0)
	: m_buffer(pb)
	, m_bPackError(false)
	, m_bLocal(false)
	{
		m_offset = pb.size() + off;
		if (!m_buffer.resize(m_offset))
		{
			m_bPackError = true;
		}
	}

	Pack(bool bLocal, PackBuffer &pb)
	: m_buffer(pb)
	, m_bPackError(false)
	, m_bLocal(bLocal)
	{
		m_offset = pb.size();
		if (!m_buffer.resize(m_offset))
		{
			m_bPackError = true;
		}
	}

	// access this packet.
	char* data()
	{
		return m_buffer.data() + m_offset;
	}

	const char* data() const
	{
		return m_buffer.data() + m_offset;
	}

	size_t size() const
	{
		return m_buffer.size() - m_offset;
	}

	Pack &push(const void* s, size_t n)
	{
		if (!m_buffer.append((const char*)s, n))
		{
			m_bPackError = true;
		}
		return *this;
	}

	Pack &push(const void* s)
	{
		if (!m_buffer.append((const char*)s))
		{
			m_bPackError = true;
		}
		return *this;
	}

	Pack &push_uint8(uint8_t u8)
	{
		return push(&u8, 1);
	}

	Pack &push_uint16(uint16_t u16)
	{
		u16 = xhtons(u16);
		return push(&u16, 2);
	}

	Pack &push_uint32(uint32_t u32)
	{
		u32 = xhtonl(u32);
		return push(&u32, 4);
	}

	Pack &push_uint64(uint64_t u64)
	{
		u64 = xhtonll(u64);
		return push(&u64, 8);
	}

	Pack &push_varstr(const Varstr &vs)
	{
		return push_varstr(vs.data(), vs.size());
	}

	Pack &push_varstr(const void* s)
	{
		return push_varstr(s, strlen((const char*)s));
	}

	Pack &push_varstr(const std::string &s)
	{
		return push_varstr(s.data(), s.size());
	}

	Pack &push_varstr(const void* s, size_t len)
	{
		if (len > 0xFFFF)
		{
			m_bPackError = true;
			len = 0;
		}
		return push_uint16(uint16_t(len)).push(s, len);
	}

	Pack &push_varstr32(const void* s, size_t len)
	{
		return push_uint32(uint32_t(len)).push(s, len);
	}

	virtual ~Pack() {}

public:
	// replace. pos is the buffer offset, not this Pack m_offset
	size_t replace(size_t pos, const void* data, size_t rplen)
	{
		if (!m_buffer.replace(pos, (const char*)data, rplen))
		{
			m_bPackError = true;
		}
		return pos + rplen;
	}

	size_t replace_uint8(size_t pos, uint8_t u8)
	{
		return replace(pos, &u8, 1);
	}

	size_t replace_uint16(size_t pos, uint16_t u16)
	{
		u16 = xhtons(u16);
		return replace(pos, &u16, 2);
	}

	size_t replace_uint32(size_t pos, uint32_t u32)
	{
		u32 = xhtonl(u32);
		return replace(pos, &u32, 4);
	}

	bool isPackError() const
	{
		return m_bPackError;
	}

	void setPackError(bool val)
	{
		m_bPackError = val;
	}

protected:
	PackBuffer &m_buffer;
	size_t m_offset;
	bool m_bPackError;
	bool m_bLocal;
};

class Unpack
{
public:
	Unpack(const void* data, size_t size, bool bLocal = false)
	: m_bUnpackError(false)
	, m_bLocal(bLocal)
	{
		reset(data, size);
	}

	virtual ~Unpack()
	{
		m_data = NULL;
	}

public:
	uint16_t xntohs(uint16_t i16) const
	{
		if (m_bLocal)
		{
			return i16;
		}
		else
		{
			return ntohs(i16);
		}
	}

	uint32_t xntohl(uint32_t i32) const
	{
		if (m_bLocal)
		{
			return i32;
		}
		else
		{
			return ntohl(i32);
		}
	}

	uint64_t xntohll(uint64_t i64) const
	{
		if (m_bLocal)
		{
			return i64;
		}
		else
		{
			return isBigendian() ? i64 : uint64reverse(i64);
		}
	}

	void reset(const void* data, size_t size) const
	{
		m_data = (const char*)data;
		m_size = size;
	}

	operator const void* () const
	{
		return m_data;
	}

	bool operator!() const
	{
		return (NULL == m_data);
	}

	std::string pop_varstr() const
	{
		Varstr vs = pop_varstr_ptr();
		return std::string(vs.data(), vs.size());
	}

	std::string pop_varstr32() const
	{
		Varstr vs = pop_varstr32_ptr();
		return std::string(vs.data(), vs.size());
	}

	std::string pop_fetch(size_t k) const
	{
		return std::string(pop_fetch_ptr(k), k);
	}

	void finish() const
	{
		if (!empty())
		{
			m_bUnpackError = true;
		}
	}

	uint8_t pop_uint8() const
	{
		if (m_size < 1u)
		{
			m_bUnpackError = true;
			return 0;
		}

		uint8_t i8 = *((uint8_t*)m_data);
		m_data += 1u;
		m_size -= 1u;
		return i8;
	}

	uint16_t pop_uint16() const
	{
		if (m_size < 2u)
		{
			m_bUnpackError = true;
			return 0;
		}

		uint16_t i16 = 0;
		memcpy(&i16, m_data, sizeof(uint16_t));
		i16 = xntohs(i16);

		m_data += 2u;
		m_size -= 2u;
		return i16;
	}

	uint32_t pop_uint32() const
	{
		if (m_size < 4u)
		{
			m_bUnpackError = true;
			return 0;
		}

		uint32_t i32 = 0;
		memcpy(&i32, m_data, sizeof(uint32_t));
		i32 = xntohl(i32);
		m_data += 4u;
		m_size -= 4u;
		return i32;
	}

	uint32_t peek_uint32() const
	{
		if (m_size < 4u)
		{
			m_bUnpackError = true;
			return 0;
		}

		uint32_t i32 = 0;
		memcpy(&i32, m_data, sizeof(uint32_t));
		i32 = xntohl(i32);
		return i32;
	}

	uint64_t pop_uint64() const
	{
		if (m_size < 8u)
		{
			m_bUnpackError = true;
			return 0;
		}

		uint64_t i64 = 0;
		memcpy(&i64, m_data, sizeof(uint64_t));
		i64 = xntohll(i64);
		m_data += 8u;
		m_size -= 8u;
		return i64;
	}

	Varstr pop_varstr_ptr() const
	{
		Varstr vs;
		vs.m_size = pop_uint16();
		vs.m_data = pop_fetch_ptr(vs.m_size);
		return vs;
	}

	Varstr pop_varstr32_ptr() const
	{
		Varstr vs;
		vs.m_size = pop_uint32();
		vs.m_data = pop_fetch_ptr(vs.m_size);
		return vs;
	}

	const char* pop_fetch_ptr(size_t &k) const
	{
		if (m_size < k)
		{
			m_bUnpackError = true;
			k = m_size;
		}

		const char* p = m_data;
		m_data += k;
		m_size -= k;
		return p;
	}

	bool empty() const
	{
		return m_size == 0;
	}

	const char* data() const
	{
		return m_data;
	}

	size_t size() const
	{
		return m_size;
	}

	bool isUnpackError() const
	{
		return m_bUnpackError;
	}

private:
	mutable const char* m_data;
	mutable size_t m_size;
	mutable bool m_bUnpackError;
	mutable bool m_bLocal;
};

struct Marshallable
{
	virtual void marshal(Pack &) const = 0;
	virtual void unmarshal(const Unpack &) = 0;
	virtual ~Marshallable() {}
};

inline Pack &operator << (Pack &p, const Marshallable &m)
{
	m.marshal(p);
	return p;
}

inline const Unpack &operator >> (const Unpack &p, const Marshallable &m)
{
	const_cast<Marshallable &>(m).unmarshal(p);
	return p;
}

template <class T1, class T2>
inline Pack &operator << (Pack &p, const std::map<T1, T2> &map)
{
	marshal_container(p, map);
	return p;
}

template <class T1, class T2>
inline const Unpack &operator >> (const Unpack &p, std::map<T1, T2> &map)
{
	unmarshal_container(p, std::inserter(map, map.begin()));
	return p;
}

struct Voidmable
	: public Marshallable
{
	virtual void marshal(Pack &) const {}
	virtual void unmarshal(const Unpack &) {}
};

struct Mulmable
	: public Marshallable
{
	Mulmable(const Marshallable &m1, const Marshallable &m2)
		: mm1(m1)
		, mm2(m2)
	{
	}

	const Marshallable &mm1;
	const Marshallable &mm2;

	virtual void marshal(Pack &p) const
	{
		p << mm1 << mm2;
	}

	virtual void unmarshal(const Unpack &p)
	{
		(void)p;
		assert(false);
	}
};

struct Mulumable : public Marshallable
{
	Mulumable(Marshallable &m1, Marshallable &m2)
		: mm1(m1)
		, mm2(m2)
	{
	}

	Marshallable &mm1;
	Marshallable &mm2;

	virtual void marshal(Pack &p) const
	{
		p << mm1 << mm2;
	}

	virtual void unmarshal(const Unpack &p)
	{
		p >> mm1 >> mm2;
	}
};

struct Rawmable
	: public Marshallable
{
	Rawmable(const char* data, size_t size)
		: m_data(data)
		, m_size(size)
	{
	}

	template < class T >
	explicit Rawmable(T &t)
		: m_data(t.data())
		, m_size(t.size())
	{
	}

	const char* m_data;
	size_t m_size;

	virtual void marshal(Pack &p) const
	{
		p.push(m_data, m_size);
	}

	virtual void unmarshal(const Unpack &)
	{
		assert(false);
	}
};

inline Pack &operator << (Pack &p, bool sign)
{
	p.push_uint8(sign ? 1 : 0);
	return p;
}

inline Pack &operator << (Pack &p, uint8_t i8)
{
	p.push_uint8(i8);
	return p;
}

inline Pack &operator << (Pack &p, uint16_t i16)
{
	p.push_uint16(i16);
	return p;
}

inline Pack &operator << (Pack &p, uint32_t i32)
{
	p.push_uint32(i32);
	return p;
}
inline Pack &operator << (Pack &p, uint64_t i64)
{
	p.push_uint64(i64);
	return p;
}

inline Pack &operator << (Pack &p, const std::string &str)
{
	p.push_varstr(str);
	return p;
}

inline Pack &operator << (Pack &p, const Varstr &pstr)
{
	p.push_varstr(pstr);
	return p;
}

inline const Unpack &operator >> (const Unpack &p, Varstr &pstr)
{
	pstr = p.pop_varstr_ptr();
	return p;
}

inline const Unpack &operator >> (const Unpack &p, uint32_t &i32)
{
	i32 = p.pop_uint32();
	return p;
}

inline const Unpack &operator >> (const Unpack &p, uint64_t &i64)
{
	i64 = p.pop_uint64();
	return p;
}

inline const Unpack &operator >> (const Unpack &p, std::string &str)
{
	str = p.pop_varstr();
	return p;
}

inline const Unpack &operator >> (const Unpack &p, uint16_t &i16)
{
	i16 = p.pop_uint16();
	return p;
}

inline const Unpack &operator >> (const Unpack &p, uint8_t &i8)
{
	i8 = p.pop_uint8();
	return p;
}

inline const Unpack &operator >> (const Unpack &p, bool &sign)
{
	sign = (p.pop_uint8() == 0) ? false : true;
	return p;
}

template <class T1, class T2>
inline std::ostream &operator << (std::ostream &s, const std::pair<T1, T2> &p)
{
	s << p.first << '=' << p.second;
	return s;
}

template <class T1, class T2>
inline Pack &operator << (Pack &s, const std::pair<T1, T2> &p)
{
	s << p.first << p.second;
	return s;
}

template <class T1, class T2>
inline const Unpack &operator >> (const Unpack &s, std::pair<const T1, T2> &p)
{
	const T1 &m = p.first;
	T1 &m2 = const_cast<T1 &>(m);
	s >> m2 >> p.second;
	return s;
}

template <class T1, class T2>
inline const Unpack &operator >> (const Unpack &s, std::pair<T1, T2> &p)
{
	s >> p.first >> p.second;
	return s;
}

template <class T>
inline Pack &operator << (Pack &p, const std::vector<T> &vec)
{
	marshal_container(p, vec);
	return p;
}

template <class T>
inline const Unpack &operator >> (const Unpack &p, std::vector<T> &vec)
{
	unmarshal_container(p, std::back_inserter(vec));
	return p;
}

template <class T>
inline Pack &operator << (Pack &p, const std::set<T> &set)
{
	marshal_container(p, set);
	return p;
}

template <class T>
inline const Unpack &operator >> (const Unpack &p, std::set<T> &set)
{
	unmarshal_container(p, std::inserter(set, set.begin()));
	return p;
}

template < typename ContainerClass >
inline void marshal_container(Pack &p, const ContainerClass &c)
{
	p.push_uint16(uint16_t(c.size())); // use uint16 ...
	for (typename ContainerClass::const_iterator i = c.begin(); i != c.end(); ++i)
		p << *i;
}

template < typename OutputIterator >
inline void unmarshal_container(const Unpack &p, OutputIterator i)
{
	for (uint16_t count = p.pop_uint16(); count > 0; --count)
	{
		typename OutputIterator::container_type::value_type tmp;
		p >> tmp;
		if (p.isUnpackError())
		{
			break;
		}
		*i = tmp;
		++i;
	}
}

template < typename OutputIterator >
inline void unmarshal_container_pair(const Unpack &p, OutputIterator i)
{
	for (uint16_t count = p.pop_uint16(); count > 0; --count)
	{
		typename OutputIterator::container_type::value_type::second_type d;
		typename OutputIterator::container_type::value_type tmp(0, d);
		p >> tmp;
		if (p.isUnpackError())
		{
			break;
		}
		*i = tmp;
		++i;
	}
}

template < typename OutputContainer>
inline void unmarshal_containerEx(const Unpack &p, OutputContainer &c)
{
	for (uint16_t count = p.pop_uint16(); count > 0; --count)
	{
		typename OutputContainer::value_type tmp;
		tmp.unmarshal(p);
		c.push_back(tmp);
	}
}

template < typename ContainerClass >
inline std::ostream &trace_container(std::ostream &os, const ContainerClass &c, char div = '\n')
{
	for (typename ContainerClass::const_iterator i = c.begin(); i != c.end(); ++i)
		os << *i << div;
	return os;
}


}
