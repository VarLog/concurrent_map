#include <cstdlib>

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "concurrent_map/concurrent_map.h"

using map_ptr_t = std::shared_ptr<concurrent::map>;

std::atomic_bool g_is_done{false};

void producer( const map_ptr_t &map, const std::string &key, std::string value ) {
    while ( !g_is_done.load( std::memory_order_relaxed ) ) {
        auto access_token = map->get_exclusive_access( key );
        map->set( key, value );
    }
}

void consumer( const map_ptr_t &map, const std::string &key ) {
    while ( !g_is_done.load( std::memory_order_relaxed ) ) {
        std::cout << "value for key [" << key << "] == " << map->get( key ) << std::endl;
    }
}

int main( int argc, char **argv ) {

    auto map = std::make_shared<concurrent::map>();

    auto key = "foo";

    auto t1 = std::thread( producer, map, key, "thread1" );
    auto t2 = std::thread( producer, map, key, "thread2" );

    auto t3 = std::thread( consumer, map, key );

    std::this_thread::sleep_for( std::chrono::seconds( 2 ) );

    g_is_done.store( true );

    t1.join();
    t2.join();

    t3.join();

    return EXIT_SUCCESS;
}

