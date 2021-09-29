#include "bws/bwsclient.h"
//#include "utilws/Encoding.h"
//#include "utilws/Time.h"
#include <utility>

//namespace encoding = utilws::encoding;

namespace binance {

BWSClient::BWSClient()
{
    //ws.configure(uri, api_key, api_secret, subaccount_name);
    ws.set_on_open_cb([this]() { return this->on_open(); });
}

void BWSClient::on_message(utilws::WS::OnMessageCB cb)
{
    ws.set_on_message_cb(cb);
}

void BWSClient::connect()
{
    ws.connect();
}

void BWSClient::close()
{
    ws.close();
}

std::vector<json> BWSClient::on_open()
{
    std::vector<json> msgs;
#if 0
    if (!(api_key.empty() || api_secret.empty())) {
        long long ts = utilws::get_ms_timestamp(utilws::current_time()).count();
        std::string data = std::to_string(ts) + "websocket_login";
        std::string hmacced = encoding::hmac(std::string(api_secret), data, 32);
        std::string sign =
          encoding::hmac_string_to_hex((unsigned char*)hmacced.c_str(), 32);
        json msg = {{"op", "login"},
                    {"args", {{"key", api_key}, {"sign", sign}, {"time", std::to_string(ts)}}}};
        if (!subaccount_name.empty()) {
            msg.push_back({"subaccount", subaccount_name});
        }
        msgs.push_back(msg);
    }
#endif
    int id = 100;
    for (auto& [market, channel] : subscriptions) {
        json msg;

        msg["method"] = "SUBSCRIBE";
        msg["params"] = { market + "@" + channel };
        msg["id"] = id++;
        //msg = { 
		//  {"method", "SUBSCRIBE"}, {"params",{market + "@" + channel} }, {"id", id++} 
		//};

        msgs.push_back(msg);
    }
    return msgs;
}

void BWSClient::subscribe_orders(std::string market)
{
    subscriptions.push_back(std::make_pair(market, ""));
}

void BWSClient::subscribe_orderbook(std::string market)
{
    subscriptions.push_back(std::make_pair(market, "depth"));
}

void BWSClient::subscribe_fills(std::string market)
{
    subscriptions.push_back(std::make_pair(market, ""));
}

void BWSClient::subscribe_trades(std::string market)
{
    subscriptions.push_back(std::make_pair(market, "aggTrade"));
}

void BWSClient::subscribe_ticker(std::string market)
{
    ws.configure(uri + market+"@bookTicker", api_key, api_secret, subaccount_name);
    //subscriptions.push_back(std::make_pair(market, "bookTicker"));
}

}
