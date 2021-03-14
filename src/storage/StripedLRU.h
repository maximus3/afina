#ifndef AFINA_STORAGE_STRIPED_LRU_H
#define AFINA_STORAGE_STRIPED_LRU_H

#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <afina/Storage.h>
#include "ThreadSafeSimpleLRU.h"

namespace Afina {
namespace Backend {

/**
 * # SimpleLRU thread safe version
 *
 *
 */

constexpr std::size_t MIN_STRIPE_SIZE = 1u * 1024 * 1024;

class StripedLRU : public Afina::Storage {
private:
    StripedLRU(std::size_t stripe_limit, std::size_t stripes_cnt)
        : _stripes_cnt(stripes_cnt)  {
            for (size_t i = 0; i < stripes_cnt; i++) {
                _stripes.push_back(std::unique_ptr<ThreadSafeSimplLRU> (new ThreadSafeSimplLRU(stripe_limit)));
            }
        }

public:
    ~StripedLRU() {}

    static std::unique_ptr<StripedLRU>
    BuildStripedLRU(std::size_t memory_limit = 16 * MIN_STRIPE_SIZE, std::size_t stripes_cnt = 4) {
        std::size_t stripe_limit = memory_limit / stripes_cnt;

        if (stripe_limit < MIN_STRIPE_SIZE) {
            throw std::runtime_error("Stripe size < MIN_STRIPE_SIZE");
        }

        return std::move(std::unique_ptr<StripedLRU>(new StripedLRU(stripe_limit, stripes_cnt)));
    }

    bool Put(const std::string &key, const std::string &value) override;

    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    bool Set(const std::string &key, const std::string &value) override;

    bool Delete(const std::string &key) override;

    bool Get(const std::string &key, std::string &value) override;

private:
    std::vector<std::unique_ptr<ThreadSafeSimplLRU>> _stripes;
    std::hash<std::string> _stripes_hash;
    std::size_t _stripes_cnt;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_STRIPED_LRU_H
