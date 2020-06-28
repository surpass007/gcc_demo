#pragma once

#include <new>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

namespace gcc_demo
{
// boost::pool min accord
template <unsigned BlockSize>
struct default_block_allocator_malloc_free
{
	enum { requested_size = BlockSize };

	static char* ordered_malloc(size_t n)
	{
		return (char*)malloc(requested_size * n);
	}
	static void ordered_free(char* const block, size_t)
	{
		free(block);
	}
};

template <unsigned BlockSize>
struct default_block_allocator_new_delete
{
	enum { requested_size = BlockSize };

	static char* ordered_malloc(size_t n)
	{
		return new (std::nothrow) char[requested_size * n];
	}
	static void ordered_free(char* const block, size_t)
	{
		delete[] block;
	}
};

#ifdef  USE_ALLOCATOR_NEW_DELETE

typedef default_block_allocator_new_delete<1 * 1024> def_block_alloc_1k;
typedef default_block_allocator_new_delete<2 * 1024> def_block_alloc_2k;
typedef default_block_allocator_new_delete<4 * 1024> def_block_alloc_4k;
typedef default_block_allocator_new_delete<8 * 1024> def_block_alloc_8k;
typedef default_block_allocator_new_delete<16 * 1024> def_block_alloc_16k;
typedef default_block_allocator_new_delete<32 * 1024> def_block_alloc_32k;

#else

typedef default_block_allocator_malloc_free<1 * 1024> def_block_alloc_1k;
typedef default_block_allocator_malloc_free<2 * 1024> def_block_alloc_2k;
typedef default_block_allocator_malloc_free<4 * 1024> def_block_alloc_4k;
typedef default_block_allocator_malloc_free<8 * 1024> def_block_alloc_8k;
typedef default_block_allocator_malloc_free<16 * 1024> def_block_alloc_16k;
typedef default_block_allocator_malloc_free<32 * 1024> def_block_alloc_32k;

#endif

template <typename BlockAllocator = def_block_alloc_4k, unsigned MaxBlocks = 2>
class BlockBuffer
{
public:
	typedef BlockAllocator allocator;
	enum { max_blocks = MaxBlocks };
	enum { npos = size_t(-1) };

	BlockBuffer()
	{
		m_block = 0;
		m_size = 0;
		m_data = NULL;
	}
	virtual ~BlockBuffer()
	{
		free();
	}

	inline bool   empty() const
	{
		return size() == 0;
	}
	inline size_t block() const
	{
		return m_block;
	}
	inline size_t blocksize() const
	{
		return allocator::requested_size;
	}
	inline size_t capacity() const
	{
		return m_block * allocator::requested_size;
	}
	inline size_t maxsize() const
	{
		return max_blocks * allocator::requested_size;
	}
	inline size_t maxfree() const
	{
		return maxsize() - size();
	}
	inline size_t freespace() const
	{
		return capacity() - size();
	}

	inline char* data()
	{
		return m_data;
	}
	inline size_t size() const
	{
		return m_size;
	}

	bool resize(size_t n, char c = 0);
	bool reserve(size_t n);
	bool append(const char* app, size_t len);
	bool replace(size_t pos, const char* rep, size_t n);
	void erase(size_t pos = 0, size_t n = npos, bool hold = false);

	static size_t current_total_blocks()
	{
		return s_current_total_blocks;
	}
	static size_t peak_total_blocks()
	{
		return s_peak_total_blocks;
	}

protected:
	bool increase_capacity(size_t increase_size);
	char* tail()
	{
		return m_data + m_size;
	}
	void size(size_t size)
	{
		assert(size <= capacity());
		m_size = size;
	}

private:
	void free();
	static size_t s_current_total_blocks;
	static size_t s_peak_total_blocks;

	char* m_data;
	size_t m_size;
	size_t m_block;

	BlockBuffer(const BlockBuffer &);
	void operator = (const BlockBuffer &);
};

template <typename BlockAllocator, unsigned MaxBlocks>
size_t BlockBuffer<BlockAllocator, MaxBlocks >::s_current_total_blocks = 0;

template <typename BlockAllocator, unsigned MaxBlocks>
size_t BlockBuffer<BlockAllocator, MaxBlocks >::s_peak_total_blocks = 0;

template <typename BlockAllocator, unsigned MaxBlocks>
inline void BlockBuffer<BlockAllocator, MaxBlocks >::free()
{
	if (m_block > 0)
	{
		allocator::ordered_free(m_data, m_block);
		s_current_total_blocks -= m_block;
		m_data = NULL;
		m_block = 0;
	}
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::append(const char* app, size_t len)
{
	if (len == 0)
		return true; // no data

	if (increase_capacity(len))
	{
		memmove(tail(), app, len); // append
		m_size += len;
		return true;
	}
	return false;
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::reserve(size_t n)
{
	return (n <= capacity() || increase_capacity(n - capacity()));
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::resize(size_t n, char c)
{
	if (n > size()) // increase
	{
		size_t len = n - size();
		if (!increase_capacity(len))
			return false;
		memset(tail(), c, len);
	}
	m_size = n;
	return true;
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::replace(size_t pos, const char* rep, size_t n)
{
	if (pos >= size()) // out_of_range ?
		return append(rep, n);

	if (pos + n >= size()) // replace all beginning with position pos
	{
		m_size = pos;
		return append(rep, n);
	}
	if (n > 0)
		memmove(m_data + pos, rep, n);
	return true;
}

template <typename BlockAllocator, unsigned MaxBlocks>
inline void BlockBuffer<BlockAllocator, MaxBlocks >::erase(size_t pos, size_t n, bool hold)
{
	assert(pos <= size()); // out_of_range debug.

	size_t m = size() - pos; // can erase
	if (n >= m)
		m_size = pos; // all clear after pos
	else
	{
		m_size -= n;
		memmove(m_data + pos, m_data + pos + n, m - n);
	}

	if (empty() && !hold)
		free();
}

/*
 * after success increase_capacity : freespace() >= increase_size
 * if false : does not affect exist data
 */
template <typename BlockAllocator, unsigned MaxBlocks>
inline bool BlockBuffer<BlockAllocator, MaxBlocks >::increase_capacity(size_t increase_size)
{
	if (increase_size == 0) return true;

	size_t newblock = m_block;

	size_t free = freespace();
	//if (increase_size > maxsize() - free) throw std::out_of_range("BlockBuffer out of range");
	if (free >= increase_size) return true;
	increase_size -= free;
	newblock += increase_size / allocator::requested_size;
	if ((increase_size % allocator::requested_size) > 0)
		newblock++;

	if (newblock > max_blocks) return false;
	char* newdata = (char*)(allocator::ordered_malloc(newblock));
	if (0 == newdata) return false;

	if (m_block > 0)
	{
		// copy old data and free old block
		memcpy(newdata, m_data, m_size);
		allocator::ordered_free(m_data, m_block);
	}

	s_current_total_blocks += newblock - m_block;
	if (s_current_total_blocks > s_peak_total_blocks)
		s_peak_total_blocks = s_current_total_blocks;

	m_data = newdata;
	m_block = newblock;
	return true;
}

template < typename Buffer >
class ForwardBuffer
{
public:
	explicit ForwardBuffer(Buffer &sb) : m_sb(&sb), m_pos(0) {}
	~ForwardBuffer()
	{
		if (m_sb) m_sb->erase(0, m_pos);
	}

	char* data()
	{
		return m_sb->data() + m_pos;
	}
	size_t size()
	{
		return m_sb->size() - m_pos;
	}

	void erase(size_t pos, size_t n)
	{
		if (pos == 0)
		{
			m_pos += n;
			assert(m_pos <= m_sb->size());
		}
		else
			m_sb->erase(m_pos + pos, n);
	}
	bool empty() const
	{
		return m_sb->size() == m_pos;
	}
	void release()
	{
		m_sb = NULL;
	}
private:
	Buffer* m_sb;  // release need pointer.
	size_t m_pos;
};
}
