#pragma once

#include "utilrest/HTTP.h"
#include <external/json.hpp>
#include <string>

using json = nlohmann::json;

namespace ftx {

class RESTClient
{
  public:
    RESTClient();

    void set_apikey(std::string apikey, std::string apisecret, std::string subaccountname);

    json list_futures();

    json list_markets();

    json get_orderbook(const std::string market, int depth = 100);

    json get_trades(const std::string market);

    json get_account_info();

    json get_open_orders();

    json get_orders(long long id);

    json place_order(const std::string market,
                     const std::string side,
                     double price,
                     double size,
                     bool ioc = false,
                     bool post_only = false,
                     bool reduce_only = false);

    // Market order overload
    json place_order(const std::string market,
                     const std::string side,
                     double size,
                     bool ioc = false,
                     bool post_only = false,
                     bool reduce_only = false);

    json cancel_order(const std::string order_id);

    json get_fills();

    json get_balances();

    json get_deposit_address(const std::string ticker);

  private:
    utilrest::HTTPSession http_client;
    std::string uri = "ftx.com";
    std::string api_key = "";
    std::string api_secret = "";
    std::string subaccount_name = "";
};

}
