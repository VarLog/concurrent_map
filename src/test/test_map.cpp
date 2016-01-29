
#include <memory>

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

    ASSERT_STREQ( value.c_str(), actual.c_str() );
}

TEST_F( MapTest, should_emplace_new_value_if_not_exist ) {
    auto key = std::string( "key" );
    auto expected_value = std::string( "" );

    auto actual = map_->get( key );

    ASSERT_STREQ( expected_value.c_str(), actual.c_str() );
}
