#ifndef PAQ8PX_RINGBUFFER_HPP
#define PAQ8PX_RINGBUFFER_HPP

#include <cstdint>
#include <cassert>

template<class T>
class RingBuffer {
private:
    Array<T> b;
    uint32_t offset; /**< Number of input bytes in buffer (not wrapped), will be masked when used for indexing */
    uint32_t mask;

public:
    /**
     * RingBuffer(size) buf; creates an array of \size bytes (must be a power of 2).
     * @param size
     */
    RingBuffer(const uint32_t size = 0) : b(size), offset(0), mask(size - 1) {
      assert(isPowerOf2(size));
    }

    void setSize(uint32_t newSize) {
      assert(newSize > 0 && isPowerOf2(newSize));
      b.resize(newSize);
      offset = 0;
      mask = newSize - 1;
    }

    [[nodiscard]] uint32_t getpos() const {
      return offset;
    }

    void fill(const T B) {
      const auto n = (uint32_t) b.size();
      for( uint32_t i = 0; i < n; i++ )
        b[i] = B;
    }

    void add(const T B) {
      b[offset & mask] = B;
      offset++;
    }

    /**
     * Returns a reference to the i'th byte with wrap (no out of bounds).
     * @param i
     * @return
     */
    T operator[](const uint32_t i) const {
      return b[i & mask];
    }

    void set(const uint32_t i, const T B) {
      b[i & mask] = B;
    }

    /**
     * Returns i'th byte back from pos (i>0) with wrap (no out of bounds)
     * @param i
     * @return
     */
    T operator()(const uint32_t i) const {
      //assert(i!=0);
      return b[(offset - i) & mask];
    }

    void reset() {
      fill(0);
      offset = 0;
    }

    /**
     * @return the size of the \RingBuffer
     */
    uint32_t size() {
      return (uint32_t) b.size();
    }

    void copyTo(RingBuffer &dst) {
      dst.setSize(size());
      dst.offset = offset;
      auto n = (uint32_t) b.size();
      for( uint32_t i = 0; i < n; i++ )
        dst.b[i] = b[i];
    }
};

typedef RingBuffer<uint8_t> Buf;
#endif //PAQ8PX_RINGBUFFER_HPP
