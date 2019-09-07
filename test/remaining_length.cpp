// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test_main.hpp"
#include "combi_test.hpp"
#include "checker.hpp"

#include <mqtt/optional.hpp>

BOOST_AUTO_TEST_SUITE(test_remaining_length)

BOOST_AUTO_TEST_CASE( pub_sub_over_127 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        if (c->get_protocol_version() != MQTT_NS::protocol_version::v3_1_1) return;

        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        std::string test_contents;
        for (std::size_t i = 0; i < 128; ++i) {
            test_contents.push_back(static_cast<char>(i));
        }

        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS0
            cont("h_suback"),
            // publish topic1 QoS0
            cont("h_publish"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        c->set_connack_handler(
            [&chk, &c, &pid_sub]
            (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                MQTT_CHK("h_connack");
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                pid_sub = c->subscribe("topic1", MQTT_NS::qos::at_most_once);
                return true;
            });
        c->set_close_handler(
            [&chk, &s]
            () {
                MQTT_CHK("h_close");
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubrec_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubcomp_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_suback_handler(
            [&chk, &c, &pid_sub, &test_contents]
            (packet_id_t packet_id, std::vector<MQTT_NS::optional<std::uint8_t>> results) {
                MQTT_CHK("h_suback");
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == static_cast<std::uint8_t>(MQTT_NS::qos::at_most_once));
                c->publish("topic1", test_contents, MQTT_NS::qos::at_most_once);
                return true;
            });
        c->set_unsuback_handler(
            [&chk, &c, &pid_unsub]
            (packet_id_t packet_id) {
                MQTT_CHK("h_unsuback");
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&chk, &c, &pid_unsub, &test_contents]
            (std::uint8_t header,
             MQTT_NS::optional<packet_id_t> packet_id,
             MQTT_NS::string_view topic,
             MQTT_NS::string_view contents) {
                MQTT_CHK("h_publish");
                BOOST_TEST(MQTT_NS::publish::is_dup(header) == false);
                BOOST_TEST(MQTT_NS::publish::get_qos(header) == MQTT_NS::qos::at_most_once);
                BOOST_TEST(MQTT_NS::publish::is_retain(header) == false);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == test_contents);
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( pub_sub_over_16384 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        if (c->get_protocol_version() != MQTT_NS::protocol_version::v3_1_1) return;

        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        std::string test_contents;
        for (std::size_t i = 0; i < 16384; ++i) {
            test_contents.push_back(static_cast<char>(i & 0xff));
        }

        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS0
            cont("h_suback"),
            // publish topic1 QoS0
            cont("h_publish"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        c->set_connack_handler(
            [&chk, &c, &pid_sub]
            (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                MQTT_CHK("h_connack");
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                pid_sub = c->subscribe("topic1", MQTT_NS::qos::at_most_once);
                return true;
            });
        c->set_close_handler(
            [&chk, &s]
            () {
                MQTT_CHK("h_close");
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubrec_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubcomp_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_suback_handler(
            [&chk, &c, &pid_sub, &test_contents]
            (packet_id_t packet_id, std::vector<MQTT_NS::optional<std::uint8_t>> results) {
                MQTT_CHK("h_suback");
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == static_cast<std::uint8_t>(MQTT_NS::qos::at_most_once));
                c->publish("topic1", test_contents, MQTT_NS::qos::at_most_once);
                return true;
            });
        c->set_unsuback_handler(
            [&chk, &c, &pid_unsub]
            (packet_id_t packet_id) {
                MQTT_CHK("h_unsuback");
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&chk, &c, &pid_unsub, &test_contents]
            (std::uint8_t header,
             MQTT_NS::optional<packet_id_t> packet_id,
             MQTT_NS::string_view topic,
             MQTT_NS::string_view contents) {
                MQTT_CHK("h_publish");
                BOOST_TEST(MQTT_NS::publish::is_dup(header) == false);
                BOOST_TEST(MQTT_NS::publish::get_qos(header) == MQTT_NS::qos::at_most_once);
                BOOST_TEST(MQTT_NS::publish::is_retain(header) == false);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == test_contents);
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

# if 0 // It would make network load too much.

BOOST_AUTO_TEST_CASE( pub_sub_over_2097152 ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        if (c->get_protocol_version() != MQTT_NS::protocol_version::v3_1_1) return;

        using packet_id_t = typename std::remove_reference_t<decltype(*c)>::packet_id_t;
        std::string test_contents;
        for (std::size_t i = 0; i < 2097152; ++i) {
            test_contents.push_back(static_cast<char>(i % 0xff));
        }

        c->set_clean_session(true);

        std::uint16_t pid_sub;
        std::uint16_t pid_unsub;


        checker chk = {
            // connect
            cont("h_connack"),
            // subscribe topic1 QoS0
            cont("h_suback"),
            // publish topic1 QoS0
            cont("h_publish"),
            cont("h_unsuback"),
            // disconnect
            cont("h_close"),
        };

        c->set_connack_handler(
            [&chk, &c, &pid_sub]
            (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                MQTT_CHK("h_connack");
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                pid_sub = c->subscribe("topic1", MQTT_NS::qos::at_most_once);
                return true;
            });
        c->set_close_handler(
            [&chk, &s]
            () {
                MQTT_CHK("h_close");
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->set_puback_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubrec_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_pubcomp_handler(
            []
            (std::uint16_t) {
                BOOST_CHECK(false);
                return true;
            });
        c->set_suback_handler(
            [&chk, &c, &pid_sub, &test_contents]
            (packet_id_t packet_id, std::vector<MQTT_NS::optional<std::uint8_t>> results) {
                MQTT_CHK("h_suback");
                BOOST_TEST(packet_id == pid_sub);
                BOOST_TEST(results.size() == 1U);
                BOOST_TEST(*results[0] == static_cast<std::uint8_t>(MQTT_NS::qos::at_most_once));
                c->publish("topic1", test_contents, MQTT_NS::qos::at_most_once);
                return true;
            });
        c->set_unsuback_handler(
            [&chk, &c, &pid_unsub]
            (packet_id_t packet_id) {
                MQTT_CHK("h_unsuback");
                BOOST_TEST(packet_id == pid_unsub);
                c->disconnect();
                return true;
            });
        c->set_publish_handler(
            [&chk, &c, &pid_unsub, &test_contents]
            (std::uint8_t header,
             MQTT_NS::optional<packet_id_t> packet_id,
             MQTT_NS::string_view topic,
             MQTT_NS::string_view contents) {
                MQTT_CHK("h_publish");
                BOOST_TEST(MQTT_NS::publish::is_dup(header) == false);
                BOOST_TEST(MQTT_NS::publish::get_qos(header) == MQTT_NS::qos::at_most_once);
                BOOST_TEST(MQTT_NS::publish::is_retain(header) == false);
                BOOST_CHECK(!packet_id);
                BOOST_TEST(topic == "topic1");
                BOOST_TEST(contents == test_contents);
                pid_unsub = c->unsubscribe("topic1");
                return true;
            });
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

#endif

BOOST_AUTO_TEST_SUITE_END()
