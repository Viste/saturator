#include "BufferArray.hpp"
#include <cstring>
#include <iostream>

BufferArray::BufferArray() : dataSize(0), data(nullptr) {}

BufferArray::BufferArray(int size) : dataSize(size), data(std::make_unique<double[]>(size))
{
    std::fill(data.get(), data.get() + dataSize, 0.0);
}

BufferArray::BufferArray(const BufferArray &other) : dataSize(other.dataSize), index(other.index), data(std::make_unique<double[]>(other.dataSize))
{
    std::copy(other.data.get(), other.data.get() + dataSize, data.get());
}

BufferArray::BufferArray(BufferArray &&other) noexcept : dataSize(other.dataSize), index(other.index), data(std::move(other.data))
{
    other.dataSize = 0;
    other.index = 0;
}

BufferArray &BufferArray::operator=(const BufferArray &other)
{
    if (this == &other)
    {
        return *this;
    }

    dataSize = other.dataSize;
    index = other.index;
    data = std::make_unique<double[]>(dataSize);
    std::copy(other.data.get(), other.data.get() + dataSize, data.get());
    return *this;
}

BufferArray &BufferArray::operator=(BufferArray &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    dataSize = other.dataSize;
    index = other.index;
    data = std::move(other.data);

    other.dataSize = 0;
    other.index = 0;
    return *this;
}

double &BufferArray::get(int i)
{
    return data[(index + i) % dataSize];
}

const double &BufferArray::get(int i) const
{
    return data[(index + i) % dataSize];
}

void BufferArray::push(const double *inputData, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        data[index++ % dataSize] = inputData[i];
    }
}

void BufferArray::push(int idx, double value)
{
    data[((index++) + idx) % dataSize] = value;
}

void BufferArray::push(double value)
{
    data[index++ % dataSize] = value;
}

int BufferArray::getSize() const
{
    return dataSize;
}

double BufferArray::getMax() const
{
    return *std::max_element(data.get(), data.get() + dataSize);
}
