#ifndef VSRATURATOR_DEMO_BUFFERARRAY_H
#define VSRATURATOR_DEMO_BUFFERARRAY_H

#include <stddef.h>
#include <memory>
#include <algorithm>

class BufferArray
{
public:
    BufferArray();
    explicit BufferArray(int size);

    BufferArray(const BufferArray &other);
    BufferArray(BufferArray &&other) noexcept;

    BufferArray &operator=(const BufferArray &other);
    BufferArray &operator=(BufferArray &&other) noexcept;

    ~BufferArray() = default;

    int getSize() const;
    double &get(int index);
    const double &get(int index) const;

    void push(double value);
    void push(int index, double value);
    void push(const double *data, size_t size);

    double getMax() const;

private:
    int index = 0;
    int dataSize = 0;
    std::unique_ptr<double[]> data;
};

#endif // VSRATURATOR_DEMO_BUFFERARRAY_H