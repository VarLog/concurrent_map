
#include <map>
#include <string>

namespace concurrent {
class map {
  private:
    using key_t = std::string;
    using value_t = std::string;

    std::map<key_t, value_t> map_;
  public:

    value_t get( const key_t &key ) {
        if ( map_.count( key ) ) {
            return map_.at( key );
        }

        auto new_value = value_t();
        map_[key] = new_value;

        return new_value;
    }

    void set( const key_t &key, value_t value ) {
        map_[key] = std::move( value );
    }

};
}  // namespace concurrent

