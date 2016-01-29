
#include <condition_variable>
#include <map>
#include <mutex>
#include <string>

namespace concurrent {
class map {

  public:
    using key_t = std::string;
    using value_t = std::string;


    class access_token {
      private:
        map *map_;   /// \< \fixme get rid raw pointer
        map::key_t key_;

        bool was_resealed_ = false;

      public:
        access_token( map *map, const map::key_t &key ) :
            map_( map ),
            key_( key ) {
        }
        access_token( const access_token & ) = delete;
        access_token( access_token && ) = default;

        void release() {
            if ( !was_resealed_ ) {
                map_->release_exclusive_token( key_ );
                was_resealed_ = true;
            }
        }
        ~access_token() {
            release();
        }
    };

    access_token get_exclusive_access( const key_t &key ) {
        std::unique_lock<std::mutex> lock( map_mutex_ );

        if ( map_sync_.count( key ) ) {
            auto &value_sync = map_sync_.at( key );

            while ( value_sync->is_locked ) {
                value_sync->cond_var.wait( lock );
            }
        }
        else {
            map_sync_[key] = std::make_shared<synchronized_value>();
        }

        map_sync_[key]->thread_id = std::this_thread::get_id();
        map_sync_[key]->is_locked = true;

        return access_token( this, key );
    }

    value_t get( const key_t &key ) {
        std::unique_lock<std::mutex> lock( map_mutex_ );

        if ( map_sync_.count( key ) ) {
            auto &value_sync = map_sync_.at( key );

            while ( value_sync->is_locked ) {
                if ( value_sync->thread_id == std::this_thread::get_id() ) {
                    break;
                }

                value_sync->cond_var.wait( lock );
            }
        }

        if ( map_.count( key ) ) {
            return map_.at( key );
        }

        map_[key] = value_t();
        return map_[key];
    }

    void set( const key_t &key, value_t value ) {
        std::unique_lock<std::mutex> lock( map_mutex_ );

        if ( map_sync_.count( key ) ) {
            auto &value_sync = map_sync_.at( key );

            while ( value_sync->is_locked ) {
                if ( value_sync->thread_id == std::this_thread::get_id() ) {
                    break;
                }

                value_sync->cond_var.wait( lock );
            }
        }

        map_[key] = std::move( value );
    }

  private:

    void release_exclusive_token( const key_t &key ) {
        std::unique_lock<std::mutex> lock( map_mutex_ );

        if ( map_sync_.count( key ) ) {
            auto &value = map_sync_.at( key );

            value->is_locked = false;
            value->cond_var.notify_one();
        }
    }

    struct synchronized_value {
        bool is_locked;
        std::thread::id thread_id;
        std::condition_variable cond_var;

        synchronized_value() = default;
        synchronized_value( const synchronized_value & ) = delete;
        synchronized_value( synchronized_value && ) = default;
        ~synchronized_value() = default;
    };

    std::map<key_t, value_t> map_;
    std::map<key_t, std::shared_ptr<synchronized_value>> map_sync_;
    std::mutex map_mutex_;

};
}  // namespace concurrent

