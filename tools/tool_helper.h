
#pragma once

#include <sys/time.h>
#include <time.h>
#include <string>
#include "protocol.h"

#define PROTO_HEAD_LEN     (sizeof(uint32_t) + sizeof(uint16_t))

using namespace gcc_demo;

inline int64_t GetCurTimeMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

inline uint32_t GetCurTime() {
    return time(NULL);
}

inline void PaddingString(std::string& str, int num) {
    if (str.size() > num)  str = str.substr(0, num);
    int pad_size = num - str.size();
    std::string padding(pad_size, '0');
    str += padding;
}

inline bool ProtoMarshal(uint32_t uri, const Marshallable& m, std::string& output)
{
	PackBuffer buffer;
	Pack hpk(buffer);
	Pack pk(buffer, PROTO_HEAD_LEN);
	m.marshal(pk);

	uint16_t len = (uint16_t)(PROTO_HEAD_LEN + pk.size());

	hpk.replace_uint32(0, uri);
	hpk.replace_uint16(sizeof(uri), len);

	output.assign(hpk.data(), hpk.size());
	return !hpk.isPackError();
}

inline bool PacketUnmarshal(const std::string& input, Marshallable& m)
{
	if (input.empty())
	{
		return false;
	}

	Unpack pk(input.c_str(), input.size());
	m.unmarshal(pk);
	return !pk.isUnpackError();
}


inline bool ProtoUnmarshal(const std::string& input, uint32_t& uri, std::string& data)
{
	if (input.size() < PROTO_HEAD_LEN)
	{
		return false;
	}

	Unpack hpk(input.c_str(), input.size());
	uri = hpk.pop_uint32();
	uint32_t len = hpk.pop_uint16();
	if (len != input.size())
	{
		return false;
	}

	data.assign(input.c_str() + PROTO_HEAD_LEN, input.size() - PROTO_HEAD_LEN);
	return true;
}

inline bool isBiggerUint64(uint64_t src, uint64_t dest)
{
	return (src != dest && (uint64_t)(src - dest) < 0x8000000000000000);
}

template <class T>
inline T Fabs(T val) {
	if (val < 0) return val * (-1);
	return val;
}

template<class T, class L, class H>
inline T SafeClamp(T x, L min, H max) {
    if (x < min) return min;
	else if (x > max)  return max;
	return x;
}

