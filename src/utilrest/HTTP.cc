#include "utilrest/HTTP.h"
#include "utilrest/Encoding.h"
#include "utilrest/Time.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/version.hpp>
#include <iostream>

namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;


namespace utilrest {

void HTTPSession::configure(string _uri,
                            string _api_key,
                            string _api_secret,
                            string _subaccount_name)
{
    uri = _uri;
    api_key = _api_key;
    api_secret = _api_secret;
    subaccount_name = _subaccount_name;
}

http::response<http::string_body> HTTPSession::get(const string target, const string params)
{
    string endpoint;
    if (uri == FTX && params != "")
        endpoint = target + "?" + params;
    else
        endpoint = target;

    http::request<http::string_body> req{ http::verb::get, endpoint, 11 };

    if (uri == BINANCE)
        req.body() = params;

    return request(req);
}

http::response<http::string_body> HTTPSession::post(const string target,
                                                    const json payload)
{
    string endpoint = target;
    http::request<http::string_body> req{http::verb::post, endpoint, 11};

    if (uri == FTX)
        req.body() = payload.dump();
    else if (uri == BINANCE)
        req.body() = URLSearchParams(payload);

    req.prepare_payload();
    return request(req);
}

http::response<http::string_body> HTTPSession::delete_(const string target, const string params)
{
    string endpoint;
    if (uri == FTX && params != "")
        endpoint = target + "?" + params;
    else
        endpoint = target;

    http::request<http::string_body> req{http::verb::delete_, endpoint, 11};

    if (uri == BINANCE)
        req.body() = params;


    return request(req);
}

http::response<http::string_body> HTTPSession::request(
  http::request<http::string_body> req)
{
    req.set(http::field::host, uri.c_str());
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    ssl::context ctx{ssl::context::sslv23_client};
    ctx.set_default_verify_paths();

    tcp::resolver resolver{ioc};
    ssl::stream<tcp::socket> stream{ioc, ctx};

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream.native_handle(), uri.c_str())) {
        boost::system::error_code ec{static_cast<int>(::ERR_get_error()),
                                     net::error::get_ssl_category()};
        throw boost::system::system_error{ec};
    }

    auto const results = resolver.resolve(uri.c_str(), "443");
    net::connect(stream.next_layer(), results.begin(), results.end());
    stream.handshake(ssl::stream_base::client);

    if (uri == FTX) {
        authenticate(req);
        if (req.method() == http::verb::post) {
            req.set(http::field::content_type, "application/json");
        }
    }
    else if (uri == BINANCE) {
        authenticateB(req);
        if (req.method() == http::verb::post) {
            req.set(http::field::content_type, "application/x-www-form-urlencoded");
        }
    }

    req.set(http::field::content_length, req.body().size());


    http::write(stream, req);
    boost::beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(stream, buffer, response);

    boost::system::error_code ec;
    stream.shutdown(ec);
    if (ec == boost::asio::error::eof) {
        // Rationale:
        // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
        ec.assign(0, ec.category());
    }

    return response;
}

void HTTPSession::authenticate(http::request<http::string_body>& req)
{

    string method(req.method_string());
    string path(req.target());
    string body(req.body());

    long long ts = get_ms_timestamp(current_time()).count();
    string data = to_string(ts) + method + path;
    if (!body.empty()) {
        data += body;
    }
    string hmacced = encoding::hmac(string(api_secret), data, 32);
    string sign =
      encoding::hmac_string_to_hex((unsigned char*)hmacced.c_str(), 32);

    req.set("FTX-KEY", api_key);
    req.set("FTX-TS", to_string(ts));
    req.set("FTX-SIGN", sign);
    if (!subaccount_name.empty()) {
        req.set("FTX-SUBACCOUNT", subaccount_name);
    }
}

void HTTPSession::authenticateB(http::request<http::string_body>& req)
{

    string method(req.method_string());
    string path(req.target());
    string body(req.body());


    long long ts = get_ms_timestamp(current_time()).count();
    string querystring;
    if (!body.empty()) {
        querystring.append(body+"&");
    }
    querystring.append("recvWindow=5000&timestamp=");
    querystring.append(to_string(ts));



    //string signature = hmac_sha256(secret_key.c_str(), querystring.c_str());
    string hmacced = encoding::hmac(string(api_secret), querystring, 32);
    string sign =
        encoding::hmac_string_to_hex((unsigned char*)hmacced.c_str(), 32);

    querystring.append("&signature=");
    querystring.append(sign);




    if (method == "POST" || method == "DELETE") {
        req.body() = querystring;
    }
    else if (method == "GET") {
        //cout << "target: "<<req.target() << "\n";
        req.target(path + "?" + querystring);
        //cout << "target: " << req.target() << "\n";
        req.body() = "";
    }
    //cout << req.body() << "\n";
    req.set("X-MBX-APIKEY", api_key);
}


string HTTPSession::URLSearchParams(json params) {
    std::string usp;
    int count = 0;
    for (auto& key : params.items()) {
        if (count > 0) {
            usp += "&";
        }
        count++;
        string k = key.key();
        string v = key.value().dump();
        v.erase(remove(v.begin(), v.end(), '\"'), v.end());
        usp += k + "=" + v;
    }
    return usp;
}



}
