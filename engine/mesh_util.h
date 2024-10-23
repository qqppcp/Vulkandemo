#pragma once

inline unsigned int cycle3(unsigned int i)
{
	unsigned int imod3 = i % 3;
	return i - imod3 + ((1 << imod3) & 3);
}
inline unsigned int cycle3(unsigned int i, unsigned int ofs)
{
	return i - i % 3 + (i + ofs) % 3;
}