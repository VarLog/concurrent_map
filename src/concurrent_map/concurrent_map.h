
#include <condition_variable>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>

namespace concurrent {

class invalid_access_exception : public std::runtime_error {
  public:
    invalid_access_exception( const std::string &reason ) : std::runtime_error( reason ) {};
};

class map {

  public:
    using key_t = std::string;
    using value_t = std::string;


    /// \brief RAII class to provide exclusive access to the entry into the map.
    class access_token {
      private:

        /// \fixme get rid the raw pointer.
        /// It will be possible if map inherited from std::enable_shared_from_this<map>.
        map *map_;

        map::key_t key_;

        bool was_resealed_ = false;

        bool is_valid_ = false;

      public:
        access_token( map *map, const map::key_t &key ) :
            map_( map ),
            key_( key ),
            is_valid_( true ) {
        }
        access_token( const access_token & ) = delete;  /// \< \fixme cannot be copyable because of the raw pointer *map_.
        access_token( access_token && ) = default;

        /// \brief Release ownership of the entry.
        void release() {
            if ( !was_resealed_ && is_valid_ ) {
                map_->release_exclusive_access( key_ );
                was_resealed_ = true;
            }
        }

        /// \brief Invalidate the token. It is necessary when the map destroyed.
        void reset() {
            map_ = nullptr;
            is_valid_ = false;
        }

        /// \brief Release in the destructor. RAII.
        ~access_token() {
            release();
        }
    };

    using access_token_ptr_t = std::shared_ptr<access_token>;
    using access_token_weak_ptr_t = std::weak_ptr<access_token>;

    ~map() {
        /// We must invalidate all access tokens because they contain raw pointers to this map.
        std::for_each( map_sync_.begin(), map_sync_.end(), []( std::pair<const key_t, synchronized_value_ptr_t> &p ) {
            if( auto token = p.second->token_weak.lock() ) {
                token->reset();
            }
        } );
    }

    /// \brief Provide exclusive access to the entry with specific key by access_token.
    access_token_ptr_t get_exclusive_access( const key_t &key ) {
        std::unique_lock<std::mutex> lock( map_mutex_ );

        if ( map_sync_.count( key ) ) {
            auto &value_sync = map_sync_[key];

            while ( value_sync->is_locked ) {
                value_sync->cond_var.wait( lock );  /// \< \fixme Avoid deadlock if it is the same thread
            }
        }
        else {
            map_sync_[key] = std::make_shared<synchronized_value>();
        }

        map_sync_[key]->thread_id = std::this_thread::get_id();
        map_sync_[key]->is_locked = true;

        auto token = std::make_shared<access_token>( this, key );
        map_sync_[key]->token_weak = token;

        return token;
    }

    /// \brief Get a value of the entry by key.
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

    /// \brief Set the value to the entry by key.
    /// \note Only the thread that has an exclusive access can modify an entry of the map.
    void set( const key_t &key, value_t value ) {
        std::unique_lock<std::mutex> lock( map_mutex_ );

        if ( map_sync_.count( key ) ) {
            auto &value_sync = map_sync_[key];

            if ( value_sync->is_locked && value_sync->thread_id == std::this_thread::get_id() ) {
                map_[key] = std::move( value );
                return;
            }
        }

        throw invalid_access_exception( "Only the thread that has an exclusive access can modify an entry of the map" );
    }

  private:

    /// \brief Release exclusive access to the entry of the map.
    /// \note This method is called from access_token::release() method.
    void release_exclusive_access( const key_t &key ) {
        std::unique_lock<std::mutex> lock( map_mutex_ );

        if ( map_sync_.count( key ) ) {
            auto &value = map_sync_[key];

            value->is_locked = false;
            value->cond_var.notify_one();
        }
    }

    /// \brief This struct contains synchronization information and primitives to
    /// \brief provide exclusive access.
    struct synchronized_value {
        bool is_locked;
        std::thread::id thread_id;
        std::condition_variable cond_var;
        access_token_weak_ptr_t token_weak;

        synchronized_value() = default;
        synchronized_value( const synchronized_value & ) = delete;
        synchronized_value( synchronized_value && ) = default;
        ~synchronized_value() = default;
    };
    using synchronized_value_ptr_t = std::shared_ptr<synchronized_value>;

    std::map<key_t, value_t> map_;
    std::map<key_t, synchronized_value_ptr_t> map_sync_;
    std::mutex map_mutex_;

};

}  // namespace concurrent

