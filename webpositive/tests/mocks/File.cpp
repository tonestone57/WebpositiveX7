#include "File.h"
#include "Path.h"

// We need to define this constructor separately to avoid
// a circular dependency between File.h and Path.h
BFile::BFile(const BPath& path, uint32 openMode)
	: fStatus(B_OK)
{
}
