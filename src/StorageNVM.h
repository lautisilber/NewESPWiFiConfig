#if (!defined(_STORAGE_NVM_H_))
#define _STORAGE_NVM_H_

#include <Arduino.h>
#include <EEPROM.h>
#include <vector>

#define STORAGE_SIZE 512


struct StorageMetadata {
    StorageMetadata(size_t _size, size_t _trailingBytes) {
        size = _size;
        trailingBytes = _trailingBytes;
    }

    size_t size;
    size_t trailingBytes;
};


class StorageClass
{
private:
    uint8_t _data[STORAGE_SIZE]{0};
    std::vector<StorageMetadata> _meta;

public:
    StorageClass();
    ~StorageClass();

    template<typename T>
    T &get(int address, T &v) const;

    template<typename T>
    bool put(int address, const T &v, bool commit=true);

    bool clear();

    void PrintAll();

    template <typename T>
    int addToScheme();

private:
    inline void _load();
    inline bool _validAddress(int address) const;
    inline bool _commit();
    inline bool _commitIndex(int address);
    inline bool _clearIndex(int address);
};





StorageClass::StorageClass()
{
    EEPROM.begin(STORAGE_SIZE);
    _load();
}

StorageClass::~StorageClass()
{
    EEPROM.end();
}


inline bool StorageClass::_clearIndex(int address) {
    if (!_validAddress(address)) return false;
    for (size_t i = _meta[address].trailingBytes; i < _meta[address].trailingBytes + _meta[address].size; i++) {
        EEPROM.write(i, 0);
    }
    bool res = EEPROM.commit();
    if (res)
        memset(_data + _meta[address].trailingBytes, 0, _meta[address].size);
    return res;
}

inline bool StorageClass::_commitIndex(int address) {
    if (!_validAddress(address)) return false;
    for (size_t i = _meta[address].trailingBytes; i < _meta[address].trailingBytes + _meta[address].size; i++) {
        EEPROM.write(i, _data[i]);
    }
    return EEPROM.commit();
}

inline bool StorageClass::_validAddress(int address) const {
    return address >= 0 && address < _meta.size();
}

template<typename T>
int StorageClass::addToScheme() {
    if (_meta.size() == 0) {
        if (sizeof(T) <= STORAGE_SIZE) {
            _meta.push_back(StorageMetadata(sizeof(T), 0));
            return 0;
        }
    } else {
        size_t trailingBytes = _meta[_meta.size()-1].trailingBytes + _meta[_meta.size()-1].size;
        if (trailingBytes + sizeof(T) <= STORAGE_SIZE) {
            _meta.push_back(StorageMetadata(sizeof(T), trailingBytes));
            return _meta.size() - 1;
        }
    }
    return -1;
}


inline void StorageClass::_load()
{
    for (size_t i = 0; i < STORAGE_SIZE; i++)
    {
        _data[i] = EEPROM.read(i);
    }
}

inline bool StorageClass::_commit() {
    for (size_t i = 0; i < STORAGE_SIZE; i++) {
        EEPROM.write(i, _data[i]);
    }
    return EEPROM.commit();
}


template<typename T>
T &StorageClass::get(int address, T &v) const {
    if (_validAddress(address)) {
        if (sizeof(T) == _meta[address].size) {
            memcpy((uint8_t *) &v, _data + _meta[address].trailingBytes, sizeof(T));
        }
    }
    return v;
}


template<typename T>
bool StorageClass::put(int address, const T &v, bool commit) {
    if (_validAddress(address)) {
        if (sizeof(T) == _meta[address].size) {
            memcpy(_data + _meta[address].trailingBytes, (uint8_t *) &v, sizeof(T));
            if (commit)
                return _commitIndex(address);
            return true;
        }
    }
    return false;
}


bool StorageClass::clear() {
    for (size_t i = 0; i < STORAGE_SIZE; i++)
    {
        EEPROM.write(i, 0);
    }
    bool res = EEPROM.commit();
    if (res)
        memset(_data, 0, STORAGE_SIZE);
    return res;
}


void StorageClass::PrintAll() {
    _load();
    const uint8_t div = 8;
    for (size_t i = 0; i < STORAGE_SIZE / div; i++) {
        for (size_t j = 0; j < div; j++) {
            Serial.print(_data[i * div + j]);
        }
        Serial.println();
    }
}

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_STORAGE)
//extern StorageClass StorageNVM;
StorageClass StorageNVM;
#endif

#endif
