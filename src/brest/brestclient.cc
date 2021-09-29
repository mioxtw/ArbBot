#include "brest/brestclient.h"
#include <iostream>

namespace binance {

    BRESTClient::BRESTClient()
    {
        //http_client.configure(uri, api_key, api_secret, subaccount_name);
    }

    void BRESTClient::set_apikey(std::string apikey, std::string apisecret, std::string subaccountname)
    {
        http_client.configure(uri, apikey, apisecret, subaccountname);
    }

    json BRESTClient::list_futures()
    {
        auto response = http_client.get("", "");
        return json::parse(response.body());
    }

    json BRESTClient::list_markets()
    {
        auto response = http_client.get("", "");
        return json::parse(response.body());
    }

    json BRESTClient::get_orderbook(const std::string market, int depth)
    {
        auto response =
            http_client.get("/fapi/v1/depth", "symbol=" + market + "&limit=" + to_string(depth));
        return json::parse(response.body());
    }

    json BRESTClient::get_trades(const std::string market)
    {
        auto response = http_client.get("/fapi/v1/trades", "symbol=" + market);
        return json::parse(response.body());
    }

    json BRESTClient::get_account_info()
    {
        auto response = http_client.get("/fapi/v2/account", "");
        return json::parse(response.body());
    }

    json BRESTClient::get_open_orders()
    {
        auto response = http_client.get("", "");
        return json::parse(response.body());
    }

    json BRESTClient::get_orders(string market, long long id)
    {
        auto response = http_client.get("/fapi/v1/order", "symbol=" + market + "&orderId=" + to_string(id));
        return json::parse(response.body());
    }

    json BRESTClient::place_order(const std::string market,
        const std::string side,
        double price,
        double size,
        const std::string positionSide)
    {
        json payload = { {"symbol", market},
                        {"side", side},
                        {"price", price},
                        {"type", "LIMIT"},
                        {"size", size},
                        {"positionSide",positionSide} };
        auto response = http_client.post("/fapi/v1/order", payload);
        return json::parse(response.body());
    }

    json BRESTClient::place_order(const std::string market,
        const std::string side,
        double size,
        const std::string positionSide)
    {
        json payload = { {"symbol", market},
                        {"side", side},
                        {"type", "MARKET"},
                        {"quantity", size},
                        {"positionSide",positionSide} };
        auto response = http_client.post("/fapi/v1/order", payload);
        return json::parse(response.body());
    }

    json BRESTClient::cancel_order(const string market, const std::string order_id)
    {
        auto response = http_client.delete_("/fapi/v1/order", "symbol=" + market + "&orerId=" + order_id);
        return json::parse(response.body());
    }

    json BRESTClient::get_fills()
    {
        auto response = http_client.get("", "");
        return json::parse(response.body());
    }

    json BRESTClient::get_balances()
    {
        auto response = http_client.get("/fapi/v2/balance", "");
        return json::parse(response.body());
    }

    json BRESTClient::get_dual() {
        auto response = http_client.get("/fapi/v1/positionSide/dual", "");
        return json::parse(response.body());
    }

    json BRESTClient::get_position(string market) {
        auto response = http_client.get("/fapi/v2/positionRisk", "symbol="+ market);
        return json::parse(response.body());
    }
}
