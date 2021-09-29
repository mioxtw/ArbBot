#pragma once

#include "utilws/WS.h"
#include <external/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;

namespace binance {

class BWSClient
{
  public:
    BWSClient();

    void on_message(utilws::WS::OnMessageCB cb);
    void connect();
    void close();
    std::vector<json> on_open();

    void subscribe_orders(std::string market);
    void subscribe_orderbook(std::string market);
    void subscribe_fills(std::string market);
    void subscribe_trades(std::string market);
    void subscribe_ticker(std::string market);

  private:
    std::vector<std::pair<std::string, std::string>> subscriptions;
    utilws::WS::OnMessageCB message_cb;
    utilws::WS ws;
    std::string uri = BINANCEWS;
    //std::string uri = "wss://stream.binance.com:9443/ws";
    std::string api_key = "";
    std::string api_secret = "";
    std::string subaccount_name = "";
};

}
