#include <print>
#include <string>

#include <vector>
#include <tuple>
#include <functional>
#include <random>

struct DefaultCacheRNG {
    static inline std::mt19937_64 mt{ std::random_device{}() };
    static inline std::uniform_int_distribution<uint64_t> dist{ std::numeric_limits<uint64_t>::min(), std::numeric_limits<uint64_t>::max() };
    static uint64_t next() {
        return dist(mt);
    }
};

// Single-threaded data structure, if used in multithreaded contexts, resize reporting will be incorrect.
template <typename ValueType, typename IDType = uint64_t, class Generator = DefaultCacheRNG>
class ContiguousCache {
    using PairType = std::pair<ValueType, IDType>;
    using ContainerType = std::vector<PairType>;
public:
    struct Entry {
        friend class ContiguousCache;
        Entry() : id(0), index(0) {}
        constexpr bool is_invalid() const { return id == 0; }
    private:
        Entry(IDType p_id, size_t p_index) : id(p_id), index(p_index) {}
        IDType id;
        size_t index;
    };

    ContiguousCache();

    void reserve(size_t p_count);

    Entry push_back(const ValueType& p_value);
    Entry push_back(ValueType&& p_value);

    // caller is responsible for the consequences of shifting memory locations
    // returns an iterator after the removed element.
    ContainerType::iterator remove(Entry& p_where);
    void clear();

    // Resets flag on call.
    bool did_resize();
    ValueType* get(Entry& p_where);

    size_t dbg_attempts = 0;
private:
    ContainerType::iterator get_internal(Entry& p_where);
    ContainerType m_internal;
    bool m_hasResized = false;
};

template<typename ValueType, typename IDType, class Generator>
ContiguousCache<ValueType, IDType, Generator>::ContiguousCache()
{
    // default value, this structure is intended for large lookup anyways
    m_internal.reserve(16);
}

template<typename ValueType, typename IDType, class Generator>
inline void ContiguousCache<ValueType, IDType, Generator>::reserve(size_t p_count)
{
    m_internal.reserve(p_count);
    m_hasResized = true;
}

template<typename ValueType, typename IDType, class Generator>
ContiguousCache<ValueType, IDType, Generator>::Entry ContiguousCache<ValueType, IDType, Generator>::push_back(const ValueType& p_value)
{
    PairType* before = m_internal.data();
    m_internal.push_back({ p_value, Generator::next() });
    if (m_internal.data() != before) m_hasResized = true;
    return { m_internal.back().second, m_internal.size() - 1 };
}

template<typename ValueType, typename IDType, class Generator>
ContiguousCache<ValueType, IDType, Generator>::Entry ContiguousCache<ValueType, IDType, Generator>::push_back(ValueType&& p_value)
{
    PairType* before = m_internal.data();
    m_internal.push_back({ p_value, Generator::next() });
    if (m_internal.data() != before) m_hasResized = true;
    return { m_internal.back().second, m_internal.size() - 1 };
}


template<typename ValueType, typename IDType, class Generator>
ContiguousCache<ValueType, IDType, Generator>::ContainerType::iterator ContiguousCache<ValueType, IDType, Generator>::remove(Entry& p_where)
{
    auto it = get_internal(p_where);
    if (it != m_internal.end())
        m_internal.erase(it);
    return it;
}

template<typename ValueType, typename IDType, class Generator>
inline void ContiguousCache<ValueType, IDType, Generator>::clear()
{
    m_internal.clear();
    m_hasResized = true;
}

template<typename ValueType, typename IDType, class Generator>
inline bool ContiguousCache<ValueType, IDType, Generator>::did_resize()
{
    bool ret = m_hasResized;
    m_hasResized = false;
    return ret;
}

template<typename ValueType, typename IDType, class Generator>
ValueType* ContiguousCache<ValueType, IDType, Generator>::get(Entry& p_where)
{
    auto it = get_internal(p_where);
    if (it == m_internal.end()) return nullptr;
    return &it->first;
}

template<typename ValueType, typename IDType, class Generator>
inline ContiguousCache<ValueType, IDType, Generator>::ContainerType::iterator ContiguousCache<ValueType, IDType, Generator>::get_internal(Entry& p_where)
{
    assert(p_where.id != 0 && "Tried to access using an invalidated cache entry.");
    if (m_internal.size() == 0) return m_internal.end();
    // !! mutate input
    size_t& ind = p_where.index;
    size_t base_ind = p_where.index;

    IDType id = p_where.id;
 
    // first check assures there's at least one element, clamp to end
    if (ind >= m_internal.size()) ind = m_internal.size() - 1;

    // early return if access was successful immediately
    if (m_internal[ind].second == id) return m_internal.begin() + ind;

    // search outward from base index
    size_t dist_ctr = 1;

    while (dist_ctr != m_internal.size()) {
        dbg_attempts++;
        ind = (base_ind + m_internal.size() - dist_ctr) % m_internal.size();
        if (m_internal[ind].second == id) return m_internal.begin() + ind;
        ++dist_ctr;
    }

    // indicate that value has been deleted
    p_where.index = 0;
    p_where.id = 0;
    return m_internal.end();
}

#include <array>
#include <ranges>
namespace Tests {
    void contiguous_cache_test() {
        ContiguousCache<int> test;

        constexpr size_t cachesize = 100000;
        std::array<ContiguousCache<int>::Entry, cachesize>* s_entries = new std::array<ContiguousCache<int>::Entry, cachesize>();
        std::array<ContiguousCache<int>::Entry, cachesize>& entries = *s_entries;

        std::println("Running trials... --------------------------------------------------------");

        std::uniform_int_distribution<size_t> dist(0, entries.size() - 1);
        std::mt19937 gen;
        constexpr size_t trial_count = 100;
        constexpr size_t rm_count = 100;
        size_t theorhetical_access_count = 0;
        size_t real_access_count = 0;
        for (size_t ind : std::views::iota(0uz, trial_count)) {
            std::println("Starting new trial... ------------");
            size_t resize_count = 0;
            test.reserve(cachesize);
            for (auto& entry : entries) {
                entry = test.push_back(rand() % cachesize);
            }

            for (size_t tmpidx = 0; tmpidx < rm_count; tmpidx++) {
                size_t rm_ind = dist(gen);
                if (entries[rm_ind].is_invalid()) {
                    tmpidx--;
                    continue;
                }
                test.remove(entries[rm_ind]);
                theorhetical_access_count++;
                real_access_count += test.dbg_attempts;
                test.dbg_attempts = 0;
            }
            for (size_t i = 0; i < 1; i++)
                for (size_t tmpidx = 0; tmpidx < entries.size(); tmpidx++) {
                    size_t rm_ind = dist(gen);

                    if (entries[rm_ind].is_invalid()) {
                        tmpidx--;
                        continue;
                    }

                    int* got_int = test.get(entries[rm_ind]);

                    theorhetical_access_count++;
                    real_access_count += test.dbg_attempts;
                    test.dbg_attempts = 0;
                }

            test.clear();
            std::println("Trial complete.\n Overall Access Attempts: {} ({}/{})", float(real_access_count) / float(theorhetical_access_count), theorhetical_access_count, real_access_count);
        }
        delete s_entries;
    }
}