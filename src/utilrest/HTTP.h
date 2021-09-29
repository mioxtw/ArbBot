#pragma once

#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <external/json.hpp>

#define FTX "ftx.com"
#define BINANCE "fapi.binance.com"


namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using json = nlohmann::json;
using namespace std;

namespace utilrest {

class HTTPSession
{

    using Request = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

  public:
    void configure(std::string _uri,
                   std::string _api_key,
                   std::string _api_secret,
                   std::string _subaccount_name);

    http::response<http::string_body> get(const std::string target,
                                           const string params);
    http::response<http::string_body> post(const std::string target,
                                           const json payload);
    http::response<http::string_body> delete_(const std::string target, const string params);

  private:
    http::response<http::string_body> request(
      http::request<http::string_body> req);

    void authenticate(http::request<http::string_body>& req);
    void authenticateB(http::request<http::string_body>& req);
    string URLSearchParams(json params);

  private:
    net::io_context ioc;

    std::string uri;
    std::string api_key;
    std::string api_secret;
    std::string subaccount_name;
};
}
