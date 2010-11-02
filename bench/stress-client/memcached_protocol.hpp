
#ifndef __MEMCACHED_PROTOCOL_HPP__
#define __MEMCACHED_PROTOCOL_HPP__

#include <libmemcached/memcached.h>
#include <stdlib.h>
#include "protocol.hpp"

struct memcached_protocol_t : public protocol_t {
    memcached_protocol_t()
        {
        memcached_create(&memcached);
        
        // NOTE: we don't turn on noreply behavior because the stress
        // client is designed to emulate real-world behavior, which
        // commonly requires a reply. In order to do this properly we
        // should pipeline sets according to batch factor, and then
        // wait for store notifications for the batch.
        
        //memcached_behavior_set(&memcached, MEMCACHED_BEHAVIOR_NOREPLY, 1);
    }

    virtual ~memcached_protocol_t() {
        // Disconnect
        memcached_free(&memcached);
    }
    
    virtual void connect(const char *host, int port) {
        memcached_server_add(&memcached, host, port);
    }
    
    virtual void remove(const char *key, size_t key_size) {
        memcached_return_t _error = memcached_delete(&memcached, key, key_size, 0);
        if(_error != MEMCACHED_SUCCESS) {
            fprintf(stderr, "Error performing delete operation (%d)\n", _error);
            exit(-1);
        }
    }
    
    virtual void update(const char *key, size_t key_size,
                        const char *value, size_t value_size)
    {
        memcached_return_t _error = memcached_set(&memcached, key, key_size,
                                                  value, value_size, 0, 0);
        if(_error != MEMCACHED_SUCCESS) {
            fprintf(stderr, "Error performing insert/update operation (%d)\n", _error);
            exit(-1);
        }
    }
    
    virtual void insert(const char *key, size_t key_size,
                        const char *value, size_t value_size)
    {
        update(key, key_size, value, value_size);
    }
    
    virtual void read(payload_t *keys, int count) {
        char *_value;
        size_t _value_length;
        uint32_t _flags;
        memcached_return_t _error;

        // Convert keys to arrays suitable for memcached
        char* __keys[count];
        size_t __sizes[count];
        for(int i = 0; i < count; i++) {
            __keys[i] = keys[i].first;
            __sizes[i] = keys[i].second;
        }
        
        // Do the multiget
        _error = memcached_mget(&memcached, __keys, __sizes, count);
        if(_error != MEMCACHED_SUCCESS) {
            fprintf(stderr, "Error performing multiread operation (%d)\n", _error);
            exit(-1);
        }

        // Fetch the results
        int i = 0;
        do {
            _value = memcached_fetch(&memcached, __keys[i], &__sizes[i],
                                     &_value_length, &_flags, &_error);
            if(_error != MEMCACHED_SUCCESS && _error != MEMCACHED_END) {
                fprintf(stderr, "Error performing read operation (%d)\n", _error);
                exit(-1);
            }
            free(_value);
            i++;
        } while(_value != NULL);
    }

private:
    memcached_st memcached;
};


#endif // __MEMCACHED_PROTOCOL_HPP__

