#include "rest/restclient.h"

namespace ftx {

RESTClient::RESTClient()
{
    //http_client.configure(uri, api_key, api_secret, subaccount_name);
}

void RESTClient::set_apikey(std::string apikey, std::string apisecret, std::string subaccountname)
{
    http_client.configure(uri, apikey, apisecret, subaccountname);
}

json RESTClient::list_futures()
{
    auto response = http_client.get("/api/futures","");
    return json::parse(response.body());
}

json RESTClient::list_markets()
{
    auto response = http_client.get("/api/markets", "");
    return json::parse(response.body());
}

json RESTClient::get_orderbook(const std::string market, int depth)
{
    auto response =
      http_client.get("/api/markets/" + market, "depth=" + std::to_string(depth));
    return json::parse(response.body());
}

json RESTClient::get_trades(const std::string market)
{
    auto response = http_client.get("/api/markets/" + market + "/trades", "");
    return json::parse(response.body());
}

json RESTClient::get_account_info()
{
    auto response = http_client.get("/api/account", "");
    return json::parse(response.body());
}

json RESTClient::get_open_orders()
{
    auto response = http_client.get("/api/orders", "");
    return json::parse(response.body());
}

json RESTClient::get_orders(long long id)
{
    auto response = http_client.get("/api/orders/"+std::to_string(id), "");
    return json::parse(response.body());
}

json RESTClient::place_order(const std::string market,
                             const std::string side,
                             double price,
                             double size,
                             bool ioc,
                             bool post_only,
                             bool reduce_only)
{
    json payload = {{"market", market},
                    {"side", side},
                    {"price", price},
                    {"type", "limit"},
                    {"size", size},
                    {"ioc", ioc},
                    {"postOnly", post_only},
                    {"reduceOnly", reduce_only}};
    auto response = http_client.post("/api/orders", payload);
    return json::parse(response.body());
}

json RESTClient::place_order(const std::string market,
                             const std::string side,
                             double size,
                             bool ioc,
                             bool post_only,
                             bool reduce_only)
{
    json payload = {{"market", market},
                    {"side", side},
                    {"price", NULL},
                    {"type", "market"},
                    {"size", size},
                    {"ioc", ioc},
                    {"postOnly", post_only},
                    {"reduceOnly", reduce_only}};
    auto response = http_client.post("/api/orders", payload);
    return json::parse(response.body());
}

json RESTClient::cancel_order(const std::string order_id)
{
    auto response = http_client.delete_("/api/orders/" + order_id,"");
    return json::parse(response.body());
}

json RESTClient::get_fills()
{
    auto response = http_client.get("/api/fills", "");
    return json::parse(response.body());
}

json RESTClient::get_balances()
{
    auto response = http_client.get("/api/wallet/balances", "");
    return json::parse(response.body());
}

json RESTClient::get_deposit_address(const std::string ticker)
{
    auto response = http_client.get("/api/wallet/deposit_address/" + ticker, "");
    return json::parse(response.body());
}

}
