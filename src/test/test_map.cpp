
#include <cstdlib>
#include <ctime>

#include <chrono>
#include <future>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <gtest/gtest.h>

#include <concurrent_map/concurrent_map.h>

class MapTest : public ::testing::Test {
  protected:

    using map_ptr_t = std::unique_ptr<concurrent::map>;
    map_ptr_t map_;

    virtual void SetUp() {
        map_ = map_ptr_t( new concurrent::map() );
    }

    virtual void TearDown() {
        map_.reset();
    }

};


TEST_F( MapTest, should_contains_value ) {
    auto key = std::string( "key" );
    auto value = std::string( "value" );

    map_->set( key, value );

    auto actual = map_->get( key );

    EXPECT_STREQ( value.c_str(), actual.c_str() );
}

TEST_F( MapTest, should_emplace_new_value_if_not_exist ) {
    auto key = std::string( "key" );
    auto expected_value = std::string( "" );

    auto actual = map_->get( key );

    EXPECT_STREQ( expected_value.c_str(), actual.c_str() );
}

TEST_F( MapTest, should_be_thread_safe ) {
    std::srand( std::time( 0 ) );

    auto f = [this]( concurrent::map::key_t key, concurrent::map::value_t value ) {
        auto ss = std::stringstream();
        auto id = std::this_thread::get_id();

        ss << id;

        auto i = 100;

        while( i-- ) {
            map_->set( std::to_string( i ), ss.str() );

            map_->set( key, value );

            std::this_thread::sleep_for( std::chrono::milliseconds( std::rand() % 10 ) );
        }
    };

    auto key1 = std::string( "key1" );
    auto key2 = std::string( "key2" );

    auto expected_value1 = std::string( "foo" );
    auto expected_value2 = std::string( "bar" );

    auto fut1 = std::async( std::launch::async, f, key1, expected_value1 );
    auto fut2 = std::async( std::launch::async, f, key2, expected_value2 );

    fut1.wait();
    fut2.wait();

    auto actual1 = map_->get( key1 );
    EXPECT_STREQ( expected_value1.c_str(), actual1.c_str() );

    auto actual2 = map_->get( key2 );
    EXPECT_STREQ( expected_value2.c_str(), actual2.c_str() );
}

TEST_F( MapTest, should_provide_exclusive_access_get ) {
    auto key = std::string( "key" );
    auto value = std::string( "value" );

    auto access_token = map_->get_exclusive_access( key );

    auto fut = std::async( std::launch::async, [this, key] {
        map_->get( key );
    } );

    auto status = fut.wait_for( std::chrono::milliseconds( 500 ) );
    EXPECT_TRUE( status == std::future_status::timeout );

    map_->set( key, value );

    auto actual = map_->get( key );

    EXPECT_STREQ( value.c_str(), actual.c_str() );

    /// release token to avoid a deadlock into the destructor of the std::future from std::async()
    access_token.release();
}

TEST_F( MapTest, should_provide_exclusive_access_set ) {
    auto key = std::string( "key" );
    auto value = std::string( "value" );

    auto access_token = map_->get_exclusive_access( key );

    auto fut = std::async( std::launch::async, [this, key] {
        map_->set( key, "bar" );
    } );

    auto status = fut.wait_for( std::chrono::milliseconds( 500 ) );
    EXPECT_TRUE( status == std::future_status::timeout );

    map_->set( key, value );

    auto actual = map_->get( key );

    EXPECT_STREQ( value.c_str(), actual.c_str() );

    /// release token to avoid a deadlock into the destructor of the std::future from std::async()
    access_token.release();
}

TEST_F( MapTest, should_provide_exclusive_access_different_keys ) {
    auto key1 = std::string( "key1" );
    auto key2 = std::string( "key2" );
    auto value1 = std::string( "value1" );
    auto value2 = std::string( "value2" );

    auto access_token = map_->get_exclusive_access( key1 );

    auto fut = std::async( std::launch::async, [this, key2, value2] {
        map_->set( key2, value2 );
    } );

    auto status = fut.wait_for( std::chrono::milliseconds( 500 ) );
    EXPECT_TRUE( status == std::future_status::ready );

    map_->set( key1, value1 );

    auto actual1 = map_->get( key1 );

    EXPECT_STREQ( value1.c_str(), actual1.c_str() );

    access_token.release();

    auto actual2 = map_->get( key2 );

    EXPECT_STREQ( value2.c_str(), actual2.c_str() );
}


TEST_F( MapTest, should_provide_exclusive_access_many_keys ) {
    auto key1 = std::string( "key1" );
    auto key2 = std::string( "key2" );
    auto value1 = std::string( "value1" );
    auto value2 = std::string( "value2" );

    auto access_token1 = map_->get_exclusive_access( key1 );
    auto access_token2 = map_->get_exclusive_access( key2 );

    auto fut = std::async( std::launch::async, [this, key1, key2, value2] {
        map_->set( key1, value2 );
        map_->set( key2, value2 );
    } );

    auto status = fut.wait_for( std::chrono::milliseconds( 500 ) );
    EXPECT_TRUE( status == std::future_status::timeout );

    map_->set( key1, value1 );
    map_->set( key2, value1 );

    auto actual1 = map_->get( key1 );

    EXPECT_STREQ( value1.c_str(), actual1.c_str() );

    auto actual2 = map_->get( key2 );

    EXPECT_STREQ( value1.c_str(), actual2.c_str() );

    access_token1.release();

    status = fut.wait_for( std::chrono::milliseconds( 500 ) );
    EXPECT_TRUE( status == std::future_status::timeout );

    auto actual3 = map_->get( key1 );

    EXPECT_STREQ( value2.c_str(), actual3.c_str() );

    access_token2.release();

    status = fut.wait_for( std::chrono::milliseconds( 500 ) );
    EXPECT_TRUE( status == std::future_status::ready );

    auto actual4 = map_->get( key2 );

    EXPECT_STREQ( value2.c_str(), actual4.c_str() );
}
