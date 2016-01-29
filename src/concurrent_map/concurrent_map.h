
#include <map>
#include <mutex>
#include <string>

namespace concurrent {
class map {

  public:
    using key_t = std::string;
    using value_t = std::string;

    value_t get( const key_t &key ) {
        std::lock_guard<std::mutex> lock( map_mutex_ );

        if ( map_.count( key ) ) {
            return map_.at( key );
        }

        auto new_value = value_t();
        map_[key] = new_value;

        return new_value;
    }

    void set( const key_t &key, value_t value ) {
        std::lock_guard<std::mutex> lock( map_mutex_ );

        map_[key] = std::move( value );
    }

  private:
    std::map<key_t, value_t> map_;
    std::mutex map_mutex_;

};
}  // namespace concurrent

