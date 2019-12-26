#pragma once
struct Buffer 
{
	Buffer(int values[], int size)
	{
		this->values = values;
		this->size = size;
	}
	int& operator[] (const int index)
	{
		return *(values + index);
	}
	int GetSize()
	{
		return size;
	}
private:
	int* values;
	int size;
};