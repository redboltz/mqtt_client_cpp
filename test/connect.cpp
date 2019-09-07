// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test_main.hpp"
#include "combi_test.hpp"
#include "checker.hpp"

BOOST_AUTO_TEST_SUITE(test_connect)

using namespace MQTT_NS::literals;

BOOST_AUTO_TEST_CASE( connect ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_user_name("dummy");
        c->set_password("dummy");
        c->set_clean_session(true);
        BOOST_TEST(c->connected() == false);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(c->connected() == true);
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    c->disconnect();
                    BOOST_TEST(c->connected() == true);
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(c->connected() == true);
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);

                    c->disconnect();
                    BOOST_TEST(c->connected() == true);
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

        c->set_close_handler(
            [&chk, &s, &c]
            () {
                MQTT_CHK("h_close");
                BOOST_TEST(c->connected() == false);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->connect();
        BOOST_TEST(c->connected() == false);
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
    do_combi_test(test); // for MQTT_NS::client factory test
}

BOOST_AUTO_TEST_CASE( connect_no_strand ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    c->disconnect();
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    c->disconnect();
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

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
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( keep_alive ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        checker chk = {
            // connect
            cont("h_connack"),
            cont("h_pingresp"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

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
        c->set_pingresp_handler(
            [&chk, &c]
            () {
                MQTT_CHK("h_pingresp");
                c->disconnect();
                return true;
            });
        c->set_keep_alive_sec(3);
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( keep_alive_and_send_control_packet ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        checker chk = {
            // connect
            cont("h_connack"),
            cont("2sec"),
            cont("h_pingresp"),
            cont("4sec_cancelled"),
            // disconnect
            cont("h_close"),
        };

        boost::asio::deadline_timer tim(ios);
        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &tim]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    tim.expires_from_now(boost::posix_time::seconds(2));
                    tim.async_wait(
                        [&chk, &c, &tim](boost::system::error_code const& ec) {
                            MQTT_CHK("2sec");
                            BOOST_CHECK(!ec);
                            c->publish("topic1", "timer_reset", MQTT_NS::qos::at_most_once);
                            tim.expires_from_now(boost::posix_time::seconds(4));
                            tim.async_wait(
                                [&chk](boost::system::error_code const& ec) {
                                    MQTT_CHK("4sec_cancelled");
                                    BOOST_TEST(ec == boost::asio::error::operation_aborted );
                                }
                            );
                        }
                    );
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &tim]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    tim.expires_from_now(boost::posix_time::seconds(2));
                    tim.async_wait(
                        [&chk, &c, &tim](boost::system::error_code const& ec) {
                            MQTT_CHK("2sec");
                            BOOST_CHECK(!ec);
                            c->publish("topic1", "timer_reset", MQTT_NS::qos::at_most_once);
                            tim.expires_from_now(boost::posix_time::seconds(4));
                            tim.async_wait(
                                [&chk](boost::system::error_code const& ec) {
                                    MQTT_CHK("4sec_cancelled");
                                    BOOST_TEST(ec == boost::asio::error::operation_aborted );
                                }
                            );
                        }
                    );
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

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
        c->set_pingresp_handler(
            [&chk, &c, &tim]
            () {
                MQTT_CHK("h_pingresp");
                tim.cancel();
                c->disconnect();
                return true;
            });
        c->set_keep_alive_sec_ping_ms(3, 3 * 1000);
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( connect_again ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        bool first = true;

        checker chk = {
            // connect
            cont("h_connack1"),
            // disconnect
            cont("h_close1"),
            // connect
            cont("h_connack2"),
            // disconnect
            cont("h_close2"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&first, &chk, &c]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    if (first) {
                        MQTT_CHK("h_connack1");
                    }
                    else {
                        MQTT_CHK("h_connack2");
                    }
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    c->disconnect();
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&first, &chk, &c]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    if (first) {
                        MQTT_CHK("h_connack1");
                    }
                    else {
                        MQTT_CHK("h_connack2");
                    }
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    c->disconnect();
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

        c->set_close_handler(
            [&first, &chk, &c, &s]
            () {
                if (first) {
                    MQTT_CHK("h_close1");
                    first = false;
                    c->connect();
                }
                else {
                    MQTT_CHK("h_close2");
                    s.close();
                }
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( nocid ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_clean_session(true);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    c->disconnect();
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    c->disconnect();
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

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
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( nocid_noclean ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_error"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::identifier_rejected);
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::client_identifier_not_valid);
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

        c->set_close_handler(
            []
            () {
                BOOST_CHECK(false);
            });
        c->set_error_handler(
            [&chk, &s]
            (boost::system::error_code const&) {
                MQTT_CHK("h_error");
                s.close();
            });
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( noclean ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");

        checker chk = {
            // connect
            cont("h_connack1"),
            // disconnect
            cont("h_close1"),
            // connect
            cont("h_connack2"),
            // disconnect
            cont("h_close2"),
            // connect
            cont("h_connack3"),
            // disconnect
            cont("h_close3"),
            // connect
            cont("h_connack4"),
            // disconnect
            cont("h_close4"),
        };

        int connect = 0;
        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &connect, &c]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    switch (connect) {
                    case 0:
                        MQTT_CHK("h_connack1");
                        BOOST_TEST(sp == false);
                        break;
                    case 1:
                        MQTT_CHK("h_connack2");
                        BOOST_TEST(sp == true);
                        break;
                    case 2:
                        MQTT_CHK("h_connack3");
                        BOOST_TEST(sp == false);
                        break;
                    case 3:
                        MQTT_CHK("h_connack4");
                        BOOST_TEST(sp == false);
                        break;
                    }
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    c->disconnect();
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &connect, &c]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    switch (connect) {
                    case 0:
                        MQTT_CHK("h_connack1");
                        BOOST_TEST(sp == false);
                        break;
                    case 1:
                        MQTT_CHK("h_connack2");
                        BOOST_TEST(sp == true);
                        break;
                    case 2:
                        MQTT_CHK("h_connack3");
                        BOOST_TEST(sp == false);
                        break;
                    case 3:
                        MQTT_CHK("h_connack4");
                        // If clean session is not provided, than there will be a session present
                        // if there was ever a previous connection, even if clean session was provided
                        // on the previous connection.
                        // This is because MQTTv5 change the semantics of the flag to "clean start"
                        // such that it only effects the start of the session.
                        // Post Session cleanup is handled with a timer, not with the  clean session flag.
                        BOOST_TEST(sp == true);
                        break;
                    }
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    c->disconnect();
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

        c->set_close_handler(
            [&chk, &connect, &c, &s]
            () {
                switch (connect) {
                case 0:
                    MQTT_CHK("h_close1");
                    c->connect();
                    ++connect;
                    break;
                case 1:
                    MQTT_CHK("h_close2");
                    c->set_clean_session(true);
                    c->connect();
                    ++connect;
                    break;
                case 2:
                    MQTT_CHK("h_close3");
                    c->set_clean_session(false);
                    c->connect();
                    ++connect;
                    break;
                case 3:
                    MQTT_CHK("h_close4");
                    s.close();
                    break;
                }
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( disconnect_timeout ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_error"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &s]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    s.broker().set_disconnect_delay(boost::posix_time::seconds(2));
                    c->disconnect(boost::posix_time::seconds(1));
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &s]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    s.broker().set_disconnect_delay(boost::posix_time::seconds(2));
                    c->disconnect(boost::posix_time::seconds(1));
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

        c->set_close_handler(
            []
            () {
                BOOST_CHECK(false);
            });
        c->set_error_handler(
            [&chk, &s]
            (boost::system::error_code const&) {
                MQTT_CHK("h_error");
                s.close();
            });
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( disconnect_not_timeout ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &s]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    s.broker().set_disconnect_delay(boost::posix_time::seconds(1));
                    c->disconnect(boost::posix_time::seconds(2));
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &s]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    s.broker().set_disconnect_delay(boost::posix_time::seconds(1));
                    c->disconnect(boost::posix_time::seconds(2));
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

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
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( async_disconnect_timeout ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_error"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &s]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    s.broker().set_disconnect_delay(boost::posix_time::seconds(2));
                    c->async_disconnect(boost::posix_time::seconds(1));
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &s]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    s.broker().set_disconnect_delay(boost::posix_time::seconds(2));
                    c->async_disconnect(boost::posix_time::seconds(1));
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

        c->set_close_handler(
            []
            () {
                BOOST_CHECK(false);
            });
        c->set_error_handler(
            [&chk, &s]
            (boost::system::error_code const&) {
                MQTT_CHK("h_error");
                s.close();
            });
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_async(test);
}

BOOST_AUTO_TEST_CASE( async_disconnect_not_timeout ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& /*b*/) {
        c->set_client_id("cid1");
        c->set_clean_session(true);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_close"),
        };

        switch (c->get_protocol_version()) {
        case MQTT_NS::protocol_version::v3_1_1:
            c->set_connack_handler(
                [&chk, &c, &s]
                (bool sp, MQTT_NS::connect_return_code connack_return_code) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::connect_return_code::accepted);
                    s.broker().set_disconnect_delay(boost::posix_time::seconds(1));
                    c->async_disconnect(boost::posix_time::seconds(2));
                    return true;
                });
            break;
        case MQTT_NS::protocol_version::v5:
            c->set_v5_connack_handler(
                [&chk, &c, &s]
                (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                    MQTT_CHK("h_connack");
                    BOOST_TEST(sp == false);
                    BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                    s.broker().set_disconnect_delay(boost::posix_time::seconds(1));
                    c->async_disconnect(boost::posix_time::seconds(2));
                    return true;
                });
            break;
        default:
            BOOST_CHECK(false);
            break;
        }

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
        c->connect();
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_async(test);
}

BOOST_AUTO_TEST_CASE( connect_disconnect_prop ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& b) {
        if (c->get_protocol_version() != MQTT_NS::protocol_version::v5) return;

        c->set_client_id("cid1");
        c->set_clean_session(true);
        BOOST_TEST(c->connected() == false);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_close"),
        };

        std::vector<MQTT_NS::v5::property_variant> con_ps {
            MQTT_NS::v5::property::session_expiry_interval(0x12345678UL),
            MQTT_NS::v5::property::receive_maximum(0x1234U),
            MQTT_NS::v5::property::maximum_packet_size(0x12345678UL),
            MQTT_NS::v5::property::topic_alias_maximum(0x1234U),
            MQTT_NS::v5::property::request_response_information(true),
            MQTT_NS::v5::property::request_problem_information(false),
            MQTT_NS::v5::property::user_property("key1"_mb, "val1"_mb),
            MQTT_NS::v5::property::user_property("key2"_mb, "val2"_mb),
            MQTT_NS::v5::property::authentication_method("test authentication method"_mb),
            MQTT_NS::v5::property::authentication_data("test authentication data"_mb)
        };

        std::size_t con_user_prop_count = 0;

        std::vector<MQTT_NS::v5::property_variant> discon_ps {
            MQTT_NS::v5::property::session_expiry_interval(0x12345678UL),
            MQTT_NS::v5::property::reason_string("test reason string"_mb),
            MQTT_NS::v5::property::user_property("key1"_mb, "val1"_mb),
            MQTT_NS::v5::property::user_property("key2"_mb, "val2"_mb),
            MQTT_NS::v5::property::server_reference("test server reference"_mb),
        };

        std::size_t discon_user_prop_count = 0;

        b.set_connect_props_handler(
            [&con_user_prop_count, size = con_ps.size()] (std::vector<MQTT_NS::v5::property_variant> const& props) {
                BOOST_TEST(size == props.size());
                for (auto const& p : props) {
                    MQTT_NS::visit(
                        MQTT_NS::make_lambda_visitor<void>(
                            [&](MQTT_NS::v5::property::session_expiry_interval const& t) {
                                BOOST_TEST(t.val() == 0x12345678UL);
                            },
                            [&](MQTT_NS::v5::property::receive_maximum const& t) {
                                BOOST_TEST(t.val() == 0x1234U);
                            },
                            [&](MQTT_NS::v5::property::maximum_packet_size const& t) {
                                BOOST_TEST(t.val() == 0x12345678UL);
                            },
                            [&](MQTT_NS::v5::property::topic_alias_maximum const& t) {
                                BOOST_TEST(t.val() == 0x1234U);
                            },
                            [&](MQTT_NS::v5::property::request_response_information const& t) {
                                BOOST_TEST(t.val() == true);
                            },
                            [&](MQTT_NS::v5::property::request_problem_information const& t) {
                                BOOST_TEST(t.val() == false);
                            },
                            [&](MQTT_NS::v5::property::user_property const& t) {
                                switch (con_user_prop_count++) {
                                case 0:
                                    BOOST_TEST(t.key() == "key1");
                                    BOOST_TEST(t.val() == "val1");
                                    break;
                                case 1:
                                    BOOST_TEST(t.key() == "key2");
                                    BOOST_TEST(t.val() == "val2");
                                    break;
                                default:
                                    BOOST_TEST(false);
                                    break;
                                }
                            },
                            [&](MQTT_NS::v5::property::authentication_method const& t) {
                                BOOST_TEST(t.val() == "test authentication method");
                            },
                            [&](MQTT_NS::v5::property::authentication_data const& t) {
                                BOOST_TEST(t.val() == "test authentication data");
                            },
                            [&](auto&& ...) {
                                BOOST_TEST(false);
                            }
                        ),
                        p
                    );
                }
            }
        );

        b.set_disconnect_props_handler(
            [&discon_user_prop_count, size = discon_ps.size()] (std::vector<MQTT_NS::v5::property_variant> const& props) {
                BOOST_TEST(size == props.size());
                for (auto const& p : props) {
                    MQTT_NS::visit(
                        MQTT_NS::make_lambda_visitor<void>(
                            [&](MQTT_NS::v5::property::session_expiry_interval const& t) {
                                BOOST_TEST(t.val() == 0x12345678UL);
                            },
                            [&](MQTT_NS::v5::property::reason_string const& t) {
                                BOOST_TEST(t.val() == "test reason string");
                            },
                            [&](MQTT_NS::v5::property::user_property const& t) {
                                switch (discon_user_prop_count++) {
                                case 0:
                                    BOOST_TEST(t.key() == "key1");
                                    BOOST_TEST(t.val() == "val1");
                                    break;
                                case 1:
                                    BOOST_TEST(t.key() == "key2");
                                    BOOST_TEST(t.val() == "val2");
                                    break;
                                default:
                                    BOOST_TEST(false);
                                    break;
                                }
                            },
                            [&](MQTT_NS::v5::property::server_reference const& t) {
                                BOOST_TEST(t.val() == "test server reference");
                            },
                            [&](auto&& ...) {
                                BOOST_TEST(false);
                            }
                        ),
                        p
                    );
                }
            }
        );

        c->set_v5_connack_handler(
            [&chk, &c, discon_ps = std::move(discon_ps)]
            (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> /*props*/) {
                MQTT_CHK("h_connack");
                BOOST_TEST(c->connected() == true);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);

                c->disconnect(MQTT_NS::v5::reason_code::success, std::move(discon_ps));
                BOOST_TEST(c->connected() == true);
                return true;
            });

        c->set_close_handler(
            [&chk, &s, &c]
            () {
                MQTT_CHK("h_close");
                BOOST_TEST(c->connected() == false);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->connect(std::move(con_ps));
        BOOST_TEST(c->connected() == false);
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}

BOOST_AUTO_TEST_CASE( connack_prop ) {
    auto test = [](boost::asio::io_service& ios, auto& c, auto& s, auto& b) {
        if (c->get_protocol_version() != MQTT_NS::protocol_version::v5) return;

        c->set_client_id("cid1");
        c->set_clean_session(true);
        BOOST_TEST(c->connected() == false);

        checker chk = {
            // connect
            cont("h_connack"),
            // disconnect
            cont("h_close"),
        };

        std::vector<MQTT_NS::v5::property_variant> ps {
            MQTT_NS::v5::property::session_expiry_interval(0),
            MQTT_NS::v5::property::receive_maximum(0),
            MQTT_NS::v5::property::maximum_qos(MQTT_NS::qos::exactly_once),
            MQTT_NS::v5::property::retain_available(true),
            MQTT_NS::v5::property::maximum_packet_size(0),
            MQTT_NS::v5::property::assigned_client_identifier("test cid"_mb),
            MQTT_NS::v5::property::topic_alias_maximum(0),
            MQTT_NS::v5::property::reason_string("test connect success"_mb),
            MQTT_NS::v5::property::user_property("key1"_mb, "val1"_mb),
            MQTT_NS::v5::property::user_property("key2"_mb, "val2"_mb),
            MQTT_NS::v5::property::wildcard_subscription_available(false),
            MQTT_NS::v5::property::subscription_identifier_available(false),
            MQTT_NS::v5::property::shared_subscription_available(false),
            MQTT_NS::v5::property::server_keep_alive(0),
            MQTT_NS::v5::property::response_information("test response information"_mb),
            MQTT_NS::v5::property::server_reference("test server reference"_mb),
            MQTT_NS::v5::property::authentication_method("test authentication method"_mb),
            MQTT_NS::v5::property::authentication_data("test authentication data"_mb)
        };

        auto prop_size = ps.size();
        b.set_connack_props(std::move(ps));

        std::size_t user_prop_count = 0;

        c->set_v5_connack_handler(
            [&chk, &c, &user_prop_count, prop_size]
            (bool sp, MQTT_NS::v5::connect_reason_code connack_return_code, std::vector<MQTT_NS::v5::property_variant> props) {
                MQTT_CHK("h_connack");
                BOOST_TEST(c->connected() == true);
                BOOST_TEST(sp == false);
                BOOST_TEST(connack_return_code == MQTT_NS::v5::connect_reason_code::success);
                BOOST_TEST(props.size() == prop_size);

                for (auto const& p : props) {
                    MQTT_NS::visit(
                        MQTT_NS::make_lambda_visitor<void>(
                            [&](MQTT_NS::v5::property::session_expiry_interval const& t) {
                                BOOST_TEST(t.val() == 0);
                            },
                            [&](MQTT_NS::v5::property::receive_maximum const& t) {
                                BOOST_TEST(t.val() == 0);
                            },
                            [&](MQTT_NS::v5::property::maximum_qos const& t) {
                                BOOST_TEST(t.val() == 2);
                            },
                            [&](MQTT_NS::v5::property::retain_available const& t) {
                                BOOST_TEST(t.val() == true);
                            },
                            [&](MQTT_NS::v5::property::maximum_packet_size const& t) {
                                BOOST_TEST(t.val() == 0);
                            },
                            [&](MQTT_NS::v5::property::assigned_client_identifier const& t) {
                                BOOST_TEST(t.val() == "test cid");
                            },
                            [&](MQTT_NS::v5::property::topic_alias_maximum const& t) {
                                BOOST_TEST(t.val() == 0);
                            },
                            [&](MQTT_NS::v5::property::reason_string const& t) {
                                BOOST_TEST(t.val() == "test connect success");
                            },
                            [&](MQTT_NS::v5::property::user_property const& t) {
                                switch (user_prop_count++) {
                                case 0:
                                    BOOST_TEST(t.key() == "key1");
                                    BOOST_TEST(t.val() == "val1");
                                    break;
                                case 1:
                                    BOOST_TEST(t.key() == "key2");
                                    BOOST_TEST(t.val() == "val2");
                                    break;
                                default:
                                    BOOST_TEST(false);
                                    break;
                                }
                            },
                            [&](MQTT_NS::v5::property::wildcard_subscription_available const& t) {
                                BOOST_TEST(t.val() == false);
                            },
                            [&](MQTT_NS::v5::property::subscription_identifier_available const& t) {
                                BOOST_TEST(t.val() == false);
                            },
                            [&](MQTT_NS::v5::property::shared_subscription_available const& t) {
                                BOOST_TEST(t.val() == false);
                            },
                            [&](MQTT_NS::v5::property::server_keep_alive const& t) {
                                BOOST_TEST(t.val() == 0);
                            },
                            [&](MQTT_NS::v5::property::response_information const& t) {
                                BOOST_TEST(t.val() == "test response information");
                            },
                            [&](MQTT_NS::v5::property::server_reference const& t) {
                                BOOST_TEST(t.val() == "test server reference");
                            },
                            [&](MQTT_NS::v5::property::authentication_method const& t) {
                                BOOST_TEST(t.val() == "test authentication method");
                            },
                            [&](MQTT_NS::v5::property::authentication_data const& t) {
                                BOOST_TEST(t.val() == "test authentication data");
                            },
                            [&](auto&& ...) {
                                BOOST_TEST(false);
                            }
                        ),
                        p
                    );
                }

                c->disconnect();
                BOOST_TEST(c->connected() == true);
                return true;
            });
        c->set_close_handler(
            [&chk, &s, &c]
            () {
                MQTT_CHK("h_close");
                BOOST_TEST(c->connected() == false);
                s.close();
            });
        c->set_error_handler(
            []
            (boost::system::error_code const&) {
                BOOST_CHECK(false);
            });
        c->connect();
        BOOST_TEST(c->connected() == false);
        ios.run();
        BOOST_TEST(chk.all());
    };
    do_combi_test_sync(test);
}


BOOST_AUTO_TEST_SUITE_END()
