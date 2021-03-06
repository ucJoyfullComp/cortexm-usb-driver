#ifndef TRANSFERS_H
#define TRANSFERS_H

#include <cstddef>
#include <cstdint>


class IRxTransfer {
public:
    virtual unsigned char* get_buffer(size_t bytes) = 0;
    virtual size_t get_remaining() = 0;
    virtual void on_filled(uint8_t* buffer, size_t bytes) = 0;
    virtual void on_complete() = 0;
};


class ITxTransfer {
public:
    virtual unsigned char const* get_data_ptr() = 0;
    virtual size_t get_remaining() = 0;
    virtual void on_transferred(size_t bytes) = 0;
    virtual void on_complete() = 0;
};


class ZeroLengthRxTransfer: public IRxTransfer {
public:
    unsigned char* get_buffer(size_t bytes) override {
        return nullptr;
    }

    size_t get_remaining() override {
        return 0;
    }

    void on_filled(uint8_t* buffer, size_t bytes) override { }

    void on_complete() override { }
};


class ZeroLengthTxTransfer: public ITxTransfer {
    unsigned char const* get_data_ptr() override {
        return nullptr;
    }

    size_t get_remaining() override {
        return 0;
    }

    void on_transferred(size_t bytes) override {

    }

    void on_complete() override {

    }
};


template<typename Handler>
class TxTransfer: public ITxTransfer {
public:
    using Callback = void (*)(Handler&, TxTransfer<Handler>&);

    TxTransfer() { }

    // for handler

    size_t get_transferred() {
        return size - remaining_bytes;
    }

    TxTransfer& init(
        unsigned char const* data,
        size_t size_,
        Handler* handler_,
        Callback callback_ = nullptr
    ) {
        data_to_tx = data;
        size = size_;
        remaining_bytes = size;
        handler = handler_;
        callback = callback_;

        return *this;
    }

    // for driver

    unsigned char const* get_data_ptr() override {
        return data_to_tx;
    }

    size_t get_remaining() override {
        return remaining_bytes;
    }

    void on_transferred(size_t bytes) override {
        data_to_tx += bytes;
        remaining_bytes -= bytes;
    }

    void on_complete() override {
        if (callback != nullptr) {
            callback(*handler, *this);
        }
    }

private:
    Handler* handler;
    Callback callback;
    unsigned char const* data_to_tx;
    size_t size;
    size_t remaining_bytes;
};


template<size_t size, typename Handler>
class BufferRxTransfer: public IRxTransfer {
public:
    using Callback = void (*)(Handler&, BufferRxTransfer<size, Handler>&);

    BufferRxTransfer() :
        data_to_tx(buffer),
        remaining_bytes(size)
    {  }

    // for handler

    unsigned char* get_buffer() {
        return buffer;
    }

    size_t get_transferred() {
        return size - remaining_bytes;
    }

    BufferRxTransfer<size, Handler>& init(Handler* handler_, Callback callback_ = nullptr) {
        handler = handler_;
        callback = callback_;
        data_to_tx = buffer;
        remaining_bytes = size;

        return *this;
    }

    BufferRxTransfer<size, Handler>& reinit() {
        data_to_tx = buffer;
        remaining_bytes = size;

        return *this;
    }

    // for driver

    unsigned char* get_buffer(size_t bytes) override {
        return data_to_tx;
    }

    size_t get_remaining() override {
        return remaining_bytes;
    }

    void on_filled(unsigned char* buffer, size_t bytes) override {
        data_to_tx += bytes;
        remaining_bytes -= bytes;
    }

    void on_complete() override {
        if (callback != nullptr) {
            callback(*handler, *this);
        }
    }

private:
    Handler* handler;
    Callback callback;
    unsigned char* data_to_tx;
    size_t remaining_bytes;
    unsigned char buffer[size];
};


template<typename Handler>
class RxTransfer: public IRxTransfer {
public:
    using Callback = void (*)(Handler&, RxTransfer<Handler>&);

    RxTransfer() {  }

    // for handler

    unsigned char* get_buffer() {
        return buffer;
    }

    size_t get_transferred() {
        return transferred;
    }

    RxTransfer<Handler>& init(
        unsigned char* data,
        size_t size_,
        Handler* handler_,
        Callback callback_ = nullptr
    ) {
        handler = handler_;
        callback = callback_;
        buffer = data;
        transfer_size = size_;
        transferred = 0;

        return *this;
    }

    RxTransfer<Handler>& reinit() {
        transferred = 0;

        return *this;
    }

    // for driver

    unsigned char* get_buffer(size_t bytes) override {
        return buffer + transferred;
    }

    size_t get_remaining() override {
        return transfer_size - transferred;
    }

    void on_filled(unsigned char* buffer, size_t bytes) override {
        transferred += bytes;
    }

    void on_complete() override {
        if (callback != nullptr) {
            callback(*handler, *this);
        }
    }

private:
    Handler* handler;
    Callback callback;
    unsigned char* buffer;
    size_t transfer_size;
    size_t transferred;
};

#endif // TRANSFERS_H
