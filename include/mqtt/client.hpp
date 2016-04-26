// Copyright Takatoshi Kondo 2015
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#if !defined(MQTT_CLIENT_HPP)
#define MQTT_CLIENT_HPP

#include <string>
#include <vector>
#include <functional>
#include <set>
#include <memory>

#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>

#if !defined(MQTT_NO_TLS)
#include <boost/asio/ssl.hpp>
#endif // !defined(MQTT_NO_TLS)

#include <mqtt/endpoint.hpp>

namespace mqtt {

namespace as = boost::asio;
namespace mi = boost::multi_index;

template <typename Socket>
class client : public endpoint<Socket> {
    using this_type = client<Socket>;
    using base = endpoint<Socket>;
public:
    using close_handler = typename base::close_handler;
    using error_handler = typename base::error_handler;
    using connack_handler = typename base::connack_handler;
    using puback_handler = typename base::puback_handler;
    using pubrec_handler = typename base::pubrec_handler;
    using pubcomp_handler = typename base::pubcomp_handler;
    using publish_handler = typename base::publish_handler;
    using suback_handler = typename base::suback_handler;
    using unsuback_handler = typename base::unsuback_handler;
    using pingresp_handler = typename base::pingresp_handler;

    /**
     * @breif Destructor
     *        If client is connected, send a disconnect packet to the connected broker.
     */
    ~client() {
        disconnect();
    }

    /**
     * @breif Move constructor
     */
    client(client&&) = default;

    /**
     * @breif Move assign operator
     */
    client& operator=(client&&) = default;

    /**
     * @breif Create no tls client.
     * @param ios io_service object.
     * @param host hostname
     * @param port port number
     * @return client object
     */
    friend client<as::ip::tcp::socket>
    make_client(as::io_service& ios, std::string host, std::string port);

#if !defined(MQTT_NO_TLS)
    /**
     * @breif Create tls client.
     * @param ios io_service object.
     * @param host hostname
     * @param port port number
     * @return client object
     */
    friend client<as::ssl::stream<as::ip::tcp::socket>>
    make_tls_client(as::io_service& ios, std::string host, std::string port);

    void set_ca_cert_file(std::string file) {
        ctx_.load_verify_file(std::move(file));
    }
    void set_client_cert_file(std::string file) {
        ctx_.use_certificate_file(std::move(file), as::ssl::context::pem);
    }
    void set_client_key_file(std::string file) {
        ctx_.use_private_key_file(std::move(file), as::ssl::context::pem);
    }
#endif // !defined(MQTT_NO_TLS)

    /**
     * @breif Set a keep alive second and a pimg milli seconds.
     * @param keep_alive_sec keep alive seconds
     * @param ping_ms ping sending interval
     *
     * When a endpoint connects to a broker, the endpoint notifies keep_alive_sec to
     * the broker.
     * After connecting, the broker starts counting a timeout, and the endpoint starts
     * sending ping packets for each ping_ms.
     * When the broker receives a ping packet, timeout timer is reset.
     * If the broker doesn't receive a ping packet within keep_alive_sec, the endpoint
     * is disconnected.<BR>
     * See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718030<BR>
     * 3.1.2.10 Keep Alive
     */
    void set_keep_alive_sec_ping_ms(std::uint16_t keep_alive_sec, std::size_t ping_ms) {
        if (ping_duration_ms_ != 0 && base::connected() && ping_ms == 0) {
            tim_->cancel();
        }
        keep_alive_sec_ = keep_alive_sec;
        ping_duration_ms_ = ping_ms;
    }

    /**
     * @breif Set a keep alive second and a pimg milli seconds.
     * @param keep_alive_sec keep alive seconds
     *
     * Call set_keep_alive_sec_ping_ms(keep_alive_sec, keep_alive_sec * 1000 / 2)<BR>
     * ping_ms is set to a half of keep_alive_sec.<BR>
     * See http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718030<BR>
     * 3.1.2.10 Keep Alive
     */
    void set_keep_alive_sec(std::uint16_t keep_alive_sec) {
        set_keep_alive_sec_ping_ms(keep_alive_sec, keep_alive_sec * 1000 / 2);
    }

    /**
     * @breif Connect to a broker
     * Before calling connect(), call set_xxx member functions to configure the connection.
     */
    void connect() {
        as::ip::tcp::resolver r(ios_);
        as::ip::tcp::resolver::query q(host_, port_);
        auto it = r.resolve(q);
        setup_socket(base::socket());
        as::async_connect(
            base::socket()->lowest_layer(), it,
            [this]
            (boost::system::error_code const& ec, as::ip::tcp::resolver::iterator) mutable {
                base::set_close_handler([this](){ handle_close(); });
                base::set_error_handler([this](boost::system::error_code const& ec){ handle_error(ec); });
                if (!ec) {
                    base::set_connect();
                    if (ping_duration_ms_ != 0) {
                        tim_->expires_from_now(boost::posix_time::milliseconds(ping_duration_ms_));
                        tim_->async_wait(
                            [this](boost::system::error_code const& ec) {
                                handle_timer(ec);
                            }
                        );
                    }
                }
                if (base::handle_close_or_error(ec)) return;
                handshake_socket(base::socket());
            });
    }

    void disconnect() {
        if (base::connected()) {
            if (ping_duration_ms_ != 0) tim_->cancel();
            base::disconnect();
        }
    }

    /**
     * @brief Set close handler
     * @param h handler
     */
    void set_close_handler(close_handler h) {
        h_close_ = std::move(h);
    }

    /**
     * @brief Set error handler
     * @param h handler
     */
    void set_error_handler(error_handler h) {
        h_error_ = std::move(h);
    }

private:
    client(as::io_service& ios,
           std::string host,
           std::string port,
           bool tls)
        :ios_(ios),
         tim_(new boost::asio::deadline_timer(ios_)),
         host_(std::move(host)),
         port_(std::move(port)),
         tls_(tls),
         keep_alive_sec_(0),
         ping_duration_ms_(0)
#if !defined(MQTT_NO_TLS)
         ,
         ctx_(as::ssl::context::tlsv12)
#endif // !defined(MQTT_NO_TLS)
    {}

#if !defined(MQTT_NO_TLS)
    template <typename T>
    typename std::enable_if<
        std::is_same<T, std::unique_ptr<as::ssl::stream<as::ip::tcp::socket>>>::value
    >::type setup_socket(T& socket) {
        socket.reset(new Socket(ios_, ctx_));
        socket->set_verify_mode(as::ssl::verify_peer);
        socket->set_verify_callback([](bool preverified, as::ssl::verify_context& ctx) -> bool {
                char subject_name[256];
                X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
                X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
                return preverified;
            });
    }
#endif // defined(MQTT_NO_TLS)

    template <typename T>
    typename std::enable_if<
        std::is_same<T, std::unique_ptr<as::ip::tcp::socket>>::value
    >::type setup_socket(T& socket) {
        socket.reset(new Socket(ios_));
    }

#if !defined(MQTT_NO_TLS)
    template <typename T>
    typename std::enable_if<
        std::is_same<T, std::unique_ptr<as::ssl::stream<as::ip::tcp::socket>>>::value
    >::type handshake_socket(T& socket) {
        socket->async_handshake(
            as::ssl::stream_base::client,
            [this]
            (boost::system::error_code const& ec) mutable {
                if (base::handle_close_or_error(ec)) return;
                base::async_read_control_packet_type();
                base::send_connect(keep_alive_sec_);
            });
    }
#endif // defined(MQTT_NO_TLS)
    template <typename T>
    typename std::enable_if<
        std::is_same<T, std::unique_ptr<as::ip::tcp::socket>>::value
    >::type handshake_socket(T&) {
        base::async_read_control_packet_type();
        base::send_connect(keep_alive_sec_);
    }

    void handle_timer(boost::system::error_code const& ec) {
        if (!ec) {
            base::send_pingreq();
            tim_->expires_from_now(boost::posix_time::milliseconds(ping_duration_ms_));
            tim_->async_wait(
                [this](boost::system::error_code const& ec) {
                    handle_timer(ec);
                }
            );
        }
    }

    void handle_close() {
        if (ping_duration_ms_ != 0) tim_->cancel();
        if (h_close_) h_close_();
    }

    void handle_error(boost::system::error_code const& ec) {
        if (ping_duration_ms_ != 0) tim_->cancel();
        if (h_error_) h_error_(ec);
    }


private:
    as::io_service& ios_;
    std::unique_ptr<as::deadline_timer> tim_;
    std::string host_;
    std::string port_;
    bool tls_;
    std::uint16_t keep_alive_sec_;
    std::size_t ping_duration_ms_;
#if !defined(MQTT_NO_TLS)
    as::ssl::context ctx_;
#endif // !defined(MQTT_NO_TLS)
    close_handler h_close_;
    error_handler h_error_;
};


inline client<as::ip::tcp::socket>
make_client(as::io_service& ios, std::string host, std::string port) {
    return client<as::ip::tcp::socket>(ios, std::move(host), std::move(port), false);
}

inline client<as::ip::tcp::socket>
make_client(as::io_service& ios, std::string host, std::uint16_t port) {
    return make_client(ios, std::move(host), boost::lexical_cast<std::string>(port));
}

#if !defined(MQTT_NO_TLS)

inline client<as::ssl::stream<as::ip::tcp::socket>>
make_tls_client(as::io_service& ios, std::string host, std::string port) {
    return client<as::ssl::stream<as::ip::tcp::socket>>(ios, std::move(host), std::move(port), true);
}

inline client<as::ssl::stream<as::ip::tcp::socket>>
make_tls_client(as::io_service& ios, std::string host, std::uint16_t port) {
    return make_tls_client(ios, std::move(host), boost::lexical_cast<std::string>(port));
}

#endif // !defined(MQTT_NO_TLS)

} // namespace mqtt

#endif // MQTT_CLIENT_HPP
