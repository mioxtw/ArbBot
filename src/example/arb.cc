#define _CRT_SECURE_NO_WARNINGS
#include "rest/restclient.h"
#include "ws/wsclient.h"
#include "brest/brestclient.h"
#include "bws/bwsclient.h"
#include <external/json.hpp>
#include <thread>
#include <chrono>
#include "utilws/Time.h"
#include <atomic>
#include <boost/algorithm/string.hpp>
#include <fstream>

using namespace std;
using json = nlohmann::json;


static int mode = 0;
static string api_key = "";
static string api_secret = "";
static string subaccount_name = "";
static string openclose = "";
static string FTXSpotMarketName = "";
static string FTXFuturesMarketName = "";
static string BinanceFuturesMarketName = "";
static string coin = "";
static double place_size = 0;
static double premium = 0;
static double batch_size = 0;

static ftx::WSClient ftx_wsClient;
static ftx::RESTClient ftx_restClient;
static binance::BWSClient binance_wsClient;
static binance::BRESTClient binance_restClient;

static atomic<double>  BinanceFuturesBidPrice(0);
static atomic<double>  BinanceFuturesAskPrice(0);
static atomic<double>  BinanceFuturesBidSize(0);
static atomic<double>  BinanceFuturesAskSize(0);

static atomic<double>  FTXFuturesBidPrice(0);
static atomic<double>  FTXFuturesAskPrice(0);
static atomic<double>  FTXFuturesBidSize(0);
static atomic<double>  FTXFuturesAskSize(0);

static atomic<double>  FTXSpotAskPrice(0);
static atomic<double>  FTXSpotBidPrice(0);
static atomic<double>  FTXSpotAskSize(0);
static atomic<double>  FTXSpotBidSize(0);

static double openBidPrice = 0;
static double openAskPrice = 0;
static double openBidSize = 0;
static double openAskSize = 0;
static double closeAskPrice = 0;
static double closeBidPrice = 0;
static double closeAskSize = 0;
static double closeBidSize = 0;

//static double sPos = 0;
//static double fPos = 0;
static double openPriceDiff = 0;
static double closePriceDiff = 0;

static double lastOpenPriceDiff = 0;
static double lastOpenSizeMin = 0;
static double lastClosePriceDiff = 0;
static double lastCloseSizeMin = 0;

static atomic<bool> openSignal(false);
static atomic<bool> closeSignal(false);
static atomic<bool> premiumStop(false);
static atomic<double> openSizeMin(0);
static atomic<double> closeSizeMin(0);

static atomic<bool> ftxSpotWsStart(false);
static atomic<bool> ftxFuturesWsStart(false);
static atomic<bool> binanceFuturesWsStart(false);


static bool binanceDualSide = false;
static double binanceMinOrderSize = 0;

static mutex m1;
static mutex m2;
static condition_variable cond_var1;
static condition_variable cond_var2;

long long BinanceShortFutures(double size) {
	long long id = 0;
	json ord = binance_restClient.place_order(BinanceFuturesMarketName, "SELL", size, binanceDualSide?"SHORT":"BOTH");
	//cout << ord.dump(4) << "\n";
	if (ord.contains("orderId"))
		id = ord["orderId"].get<long long>();
	else if (ord.contains("code"))
		cout << "[Error] [" << ord["code"].get<int>() << "] "<< ord["msg"].get<string>()<<"                                           \n";
	return id;

}

long long BinanceLongFutures(double size) {
	long long id = 0;
	json ord = binance_restClient.place_order(BinanceFuturesMarketName, "BUY", size, binanceDualSide ? "SHORT" : "BOTH");
	//cout << ord.dump(4) << "\n";
	if (ord.contains("orderId"))
		id = ord["orderId"].get<long long>();
	else if (ord.contains("code"))
		cout << "[Error] [" << ord["code"].get<int>() << "] " << ord["msg"].get<string>() << "                                           \n";
	return id;

}


double BinanceGetOrderPrice(string market,long long id) {
	double price = 0;
	json ord = binance_restClient.get_orders(market, id);
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("orderId"))
		price = atof(ord["avgPrice"].get<string>().c_str());
	else if (ord.contains("code"))
		cout << "[Error] [" << ord["code"].get<int>() << "] " << ord["msg"].get<string>() << "                                           \n";
	return price;
}

long long FTXBuySpot(double size) {
	long long id = 0;
	json ord = ftx_restClient.place_order(FTXSpotMarketName, "buy", size);
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}

long long FTXSellSpot(double size) {
	long long id = 0;
	json ord = ftx_restClient.place_order(FTXSpotMarketName, "sell", size);
	//cout << ord.dump(4) << "\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;
}

long long FTXShortFutures(double size) {
	long long id = 0;
	json ord = ftx_restClient.place_order(FTXFuturesMarketName, "sell", size);
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}




long long FTXLongFutures(double size) {
	long long id = 0;
	json ord = ftx_restClient.place_order(FTXFuturesMarketName, "buy", size);
	//cout << ord.dump(4) << "\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}


double FTXGetOrderPrice(long long id) {
	double price = 0;
	json ord = ftx_restClient.get_orders(id);
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("result") && !ord.at("result").at("avgFillPrice").is_null())
		price = ord.at("result").at("avgFillPrice").get<double>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return price;
}

void printTime() {

	auto msTime = utilws::get_ms_timestamp(utilws::current_time());
	long int ms = (msTime.count() % 1000);

	auto tp = chrono::time_point<chrono::system_clock, chrono::milliseconds>(msTime);
	auto tt = chrono::system_clock::to_time_t(tp);
	//std::tm* now = gmtime(&tt);
	std::tm* now = localtime(&tt);
	printf("[%4d/%02d/%02d %02d:%02d:%02d.%02ld] ", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, ms);
}

void ftx_ws_ticks() {
	if (mode == 1) {
		ftx_wsClient.subscribe_ticker(FTXSpotMarketName);
		ftx_wsClient.subscribe_ticker(FTXFuturesMarketName);
	}
	else if (mode == 2)
		ftx_wsClient.subscribe_ticker(FTXSpotMarketName);
	else if (mode == 3)
		ftx_wsClient.subscribe_ticker(FTXFuturesMarketName);


	ftx_wsClient.on_message([](string msg) {
		json j = json::parse(msg);
		//cout << j.dump(4) << endl;
		if (j["type"].get<string>() == "update" && j["market"].get<string>() == FTXFuturesMarketName) {
			FTXFuturesBidPrice = j.at("data").at("bid").get<double>();
			FTXFuturesAskPrice = j.at("data").at("ask").get<double>();
			FTXFuturesBidSize = j.at("data").at("bidSize").get<double>();
			FTXFuturesAskSize = j.at("data").at("askSize").get<double>();
			ftxFuturesWsStart = true;
			cond_var2.notify_all();
		} 
		else if (j["type"].get<string>() == "update" && j["market"].get<string>() == FTXSpotMarketName) {
			FTXSpotAskPrice = j["data"]["ask"].get<double>();
			FTXSpotBidPrice = j["data"]["bid"].get<double>();
			FTXSpotAskSize = j["data"]["askSize"].get<double>();
			FTXSpotBidSize = j["data"]["bidSize"].get<double>();
			ftxSpotWsStart = true;
			cond_var2.notify_all();
		}
		});
	ftx_wsClient.connect();
}



void binance_ws_ticks() {
	string lower_str = boost::algorithm::to_lower_copy(BinanceFuturesMarketName);
	binance_wsClient.subscribe_ticker(lower_str);

	binance_wsClient.on_message([](string msg) {
		json j = json::parse(msg);
		//cout << j.dump(4) << "\n";
		if (j["e"].get<string>() == "bookTicker" && j["s"].get<string>() == BinanceFuturesMarketName) {
			BinanceFuturesBidPrice = atof(j["b"].get<string>().c_str());
			BinanceFuturesAskPrice = atof(j["a"].get<string>().c_str());
			BinanceFuturesBidSize = atof(j["B"].get<string>().c_str());
			BinanceFuturesAskSize = atof(j["A"].get<string>().c_str());
			binanceFuturesWsStart = true;
			cond_var2.notify_all();
		}


		});

	binance_wsClient.connect();
}

void get_premium() {
	{
		std::unique_lock<std::mutex> lock(m2);
		cond_var2.wait(lock, [] {
			bool con1 = mode == 1 && ftxSpotWsStart && ftxFuturesWsStart;
			bool con2 = mode == 2 && ftxSpotWsStart && binanceFuturesWsStart;
			bool con3 = mode == 3 && ftxFuturesWsStart && binanceFuturesWsStart;
			return con1 || con2 || con3;
			});
	}
	while (true) {
		{
			std::unique_lock<std::mutex> lock(m2);
			cond_var2.wait(lock, [] {

				if (mode == 1) {
					openBidPrice = FTXFuturesBidPrice;
					openAskPrice = FTXSpotAskPrice;
					openBidSize = FTXFuturesBidSize;
					openAskSize = FTXSpotAskSize;
					closeAskPrice = FTXFuturesAskPrice;
					closeBidPrice = FTXSpotBidPrice;
					closeAskSize = FTXFuturesAskSize;
					closeBidSize = FTXSpotBidSize;
				}
				else if (mode == 2) {
					openBidPrice = BinanceFuturesBidPrice;
					openAskPrice = FTXSpotAskPrice;
					openBidSize = BinanceFuturesBidSize;
					openAskSize = FTXSpotAskSize;
					closeAskPrice = BinanceFuturesAskPrice;
					closeBidPrice = FTXSpotBidPrice;
					closeAskSize = BinanceFuturesAskSize;
					closeBidSize = FTXSpotBidSize;
				}
				else if (mode == 3) {
					openBidPrice = BinanceFuturesBidPrice;
					openAskPrice = FTXFuturesAskPrice;
					openBidSize = BinanceFuturesBidSize;
					openAskSize = FTXFuturesAskSize;
					closeAskPrice = BinanceFuturesAskPrice;
					closeBidPrice = FTXFuturesBidPrice;
					closeAskSize = BinanceFuturesAskSize;
					closeBidSize = FTXFuturesBidSize;
				}

				openPriceDiff = openBidPrice - openAskPrice;
				closePriceDiff = closeAskPrice - closeBidPrice;
				openSizeMin = min(openBidSize, openAskSize);
				closeSizeMin = min(closeAskSize, closeBidSize);

				bool con1 = openclose == "open" && openBidPrice != 0 && openAskPrice != 0 && openSizeMin != 0 && (openPriceDiff != lastOpenPriceDiff || openSizeMin != lastOpenSizeMin);
				//bool con2 = openPriceDiff / openAskPrice > premium;
				bool con3 = openclose == "close" && closeAskPrice != 0 && closeBidPrice != 0 && (closePriceDiff != lastClosePriceDiff || closeSizeMin != lastCloseSizeMin);
				//bool con4 = closePriceDiff / closeBidPrice < premium;
				return con1  || con3 || premiumStop;
				});
		}

		if (premiumStop) {
			//cout << "premiumThared stopped!! \n";
			break;
		}

		if (openclose == "open" && openBidPrice != 0 && openAskPrice != 0 && openSizeMin != 0 && (openPriceDiff != lastOpenPriceDiff || openSizeMin != lastOpenSizeMin)) {
			if (openPriceDiff / openAskPrice > premium) {
				openSignal = true;
				cond_var1.notify_all();
				printTime();
				cout << "[Websocket][open] Premium:[" << round(openPriceDiff / openAskPrice * 100000) / 1000 << "%]" << "  Ask:[" << openAskPrice << "]  Bid:[" << openBidPrice << "]  Size:[" << openSizeMin << "]                  \n";
			}
			else {
				openSignal = false;
			}
			printTime();
			cout << "[Websocket][open] Premium:[" << round(openPriceDiff / openAskPrice * 100000) / 1000 << " %]" << "  Ask:[" << openAskPrice << "]  Bid:[" << openBidPrice << "]  Size:[" << openSizeMin << "]                  \r" << flush;
			lastOpenPriceDiff = openPriceDiff;
			lastOpenSizeMin = openSizeMin.load();
		}
		else if (openclose == "close" && closeAskPrice != 0 && closeBidPrice != 0 && (closePriceDiff != lastClosePriceDiff || closeSizeMin != lastCloseSizeMin)) {
			if (closePriceDiff / closeBidPrice < premium) {
				closeSignal = true;
				cond_var1.notify_all();
				printTime();
				cout << "[Websocket][close] Premium:[" << round(closePriceDiff / closeBidPrice * 100000) / 1000 << " %]" << "  Bid:[" << closeBidPrice << "]   Ask:[" << closeAskPrice << "]  Size:[" << closeSizeMin << "]                  \n";
			}
			else {
				closeSignal = false;
			}
			printTime();
			cout << "[Websocket][close] Premium:[" << round(closePriceDiff / closeBidPrice * 100000) / 1000 << " %]" << "  Bid:[" << closeBidPrice << "]   Ask:[" << closeAskPrice << "]  Size:[" << closeSizeMin << "]                  \r" << flush;
			lastClosePriceDiff = closePriceDiff;
			lastCloseSizeMin = closeSizeMin;
		}
	}
}

bool get_dual() {
	bool dual = false;
	json ord = binance_restClient.get_dual();
	if (ord.contains("dualSidePosition"))
		dual = ord["dualSidePosition"].get<bool>();
	else if (ord.contains("code"))
		cout << "[Error] [" << ord["code"].get<int>() << "] " << ord["msg"].get<string>() << "                                           \n";
	return dual;
}


int printInfo() {

	double ftx_margin_balance = 0;
	double binance_margin_balance = 0;

	double ftx_max_leverage = 0;
	double ftx_current_leverage = 0;
	double binance_max_leverage = 0;
	double binance_current_leverage = 0;

	double ftx_spot_position = 0;
	double ftx_futures_position = 0;
	double binance_futures_position = 0;


	//FTX Balance
	auto ret = ftx_restClient.get_balances();
	//cout << ret.dump(4) << endl;
	if (ret.contains("result")) {
		if (!ret["result"].empty()) {
			for (auto& key : ret["result"].items()) {
				ftx_margin_balance += key.value()["usdValue"].get<double>();
				if (key.value()["total"].get<double>() != 0)
					cout << "[FTX] Balance:              [" << key.value()["coin"].get<string>() << "] :[" << key.value()["total"].get<double>() << "]" << "                                                \n";
				if (key.value()["coin"].get<string>() == coin)
					ftx_spot_position = key.value()["total"].get<double>();					
			}
			cout << "[FTX] Margin Balance:       [USD] :[" << ftx_margin_balance << "]                                                                      \n";

		}
		else {
			cout << "[Error] Please deposit USD or USDT !!" << "                                                \n";
			return -1;
		}

	}
	else if (ret.contains("error")) {
		cout << "[Error] " << ret["error"].get<string>() << "                                                \n";
		return -1;
	}



	//Binance Balance and Leverage and position
	if (mode == 2 || mode == 3) {
		auto ret2 = binance_restClient.get_account_info();
		//cout << ret2.dump(4) << "\n";
		if (ret2.contains("code")) {
			cout << "[Error] [" << ret2["code"].get<int>() << "] " << ret2["msg"].get<string>() << "                                           \n";
			return -1;
		}
		else {
			double totalPositionSize = 0;
			for (auto& key : ret2["positions"].items()) {
				if (key.value()["symbol"].get<string>() == BinanceFuturesMarketName) {
					totalPositionSize = abs(atof(key.value()["notional"].get<string>().c_str()));
					binance_max_leverage = atof(key.value()["leverage"].get<string>().c_str());
					binance_futures_position = atof(key.value()["positionAmt"].get<string>().c_str());
				}
			}
			for (auto& key : ret2["assets"].items()) {

				if (key.value()["asset"].get<string>() == "BUSD") {
					binance_margin_balance = atof(key.value()["marginBalance"].get<string>().c_str());
					cout << "[Binance] Margin Balance:   [BUSD]:[" << binance_margin_balance << "]" << "                                                \n";
				}
			}
			if (binance_margin_balance)
				binance_current_leverage = totalPositionSize / binance_margin_balance;
		}
	}

	//FTX Leverage and futures position
	auto ret3 = ftx_restClient.get_account_info();
	//cout << ret3.dump(4) << endl;
	if (ret3.contains("result")) {
		ftx_max_leverage = ret3["result"]["leverage"].get<double>();
		ftx_current_leverage = ret3["result"]["totalPositionSize"].get<double>() / ret3["result"]["totalAccountValue"].get<double>();
		cout << "\n[FTX] Max Leverage:         [" << ftx_max_leverage << "]" << "                                                \n";
		cout << "[FTX] Current Leverage:     [" << ftx_current_leverage << "]" << "                                                \n";

		for (auto& key : ret3["result"]["positions"].items()) {
			if (key.value()["future"].get<string>() == FTXFuturesMarketName)
				ftx_futures_position = key.value()["netSize"].get<double>();
		}

	}
	else if (ret3.contains("error")) {
		cout << "[Error] " << ret3["error"].get<string>() << "                                                \n";
		return -1;
	}

	//Binance Leverage
	if (mode == 2 || mode == 3) {
		cout << "[Binance] Max Leverage:     [" << binance_max_leverage << "]\n";
		if (binance_margin_balance)
			cout << "[Binance] Current Leverage: [" << binance_current_leverage << "]\n\n";
		else
			cout << "[Binance] Current Leverage: [0]\n\n";
	}

	//FTX Spot position
	if (mode == 1 || mode == 2)
		cout << "[FTX] Spot Positions:       [" << coin << "]    :[" << ftx_spot_position << "]" << "                                                \n";

	//FTX Futures position
	if (mode == 1 || mode == 3)
		cout << "[FTX] Futures Positions:    [" << FTXFuturesMarketName << "]:[" << ftx_futures_position << "]" << "                                                \n";

	//Binance Futures position
	if (mode == 2 || mode == 3)
		cout << "[Binance] Futures Positions:[" << BinanceFuturesMarketName << "]:[" << binance_futures_position << "]" << "                                                \n\n";

	return 0;
}


int main(int argc, char* argv[])
{
	string ver = "v0.2.11";
	cout << "\n";
	cout << "--------------------------------------------------------\n";
	cout << " [Master Mio] ArbBot " << ver << "\n";
	cout << " Auther:  [Mio]\n";
	cout << " Website: [http://miobtc.com]\n";
	cout << " Email:   [miox.tw@gmail.com]\n";
	cout << "--------------------------------------------------------\n\n";

	string ftx_api_key = "";
	string ftx_api_secret = "";
	string ftx_subaccount = "";
	string binance_api_key = "";
	string binance_api_secret = "";

	if (argc == 2) {
		ifstream file(argv[1]);
		json config = json::parse(file);
		//file >> config;

		mode = config["mode"].get<int>();
		FTXSpotMarketName = config["ftx-spotMarketName"].get<string>();
		FTXFuturesMarketName = config["ftx-futuresMarketName"].get<string>();
		BinanceFuturesMarketName = config["binance-futuresMarketName"].get<string>();
		openclose = config["openclose"].get<string>();
		place_size = config["total-place-size"].get<double>();
		premium = config["premium"].get<double>();
		batch_size = config["batch-place-size"].get<double>();
		ftx_api_key = config["ftx-api-key"].get<string>();
		ftx_api_secret = config["ftx-api-secret"].get<string>();
		ftx_subaccount = config["ftx-subaccount"].get<string>();
		binance_api_key = config["binance-api-key"].get<string>();
		binance_api_secret = config["binance-api-secret"].get<string>();

		cout << "Open/Close positions:       [" << openclose << "]\n";
		cout << "Size:                       [" << place_size << "]\n";
		cout << "Premium:                    [" << premium*100 << " %]\n";
		cout << "Batch size limit:           [" << batch_size << "]\n";
	}
	else {
		cout << "Usage: " << argv[0] << " [config json]" << "\n";
		cout << "Example:\n";
		cout << "       " << argv[0] << " config.json" << "\n";
		cout << "       " << argv[0] << " config2.json" << "\n";
		return 0;
	}


	if (mode == 1)
		cout << "Hedge Mode:                 [1] FTX Futures["<< FTXFuturesMarketName <<"] / FTX Spot["<< FTXSpotMarketName <<"]\n\n";
	else if (mode == 2)
		cout << "Hedge Mode:                 [2] Binance Futures[" << BinanceFuturesMarketName << "] / FTX Spot[" << FTXSpotMarketName << "]\n\n";
	else if (mode == 3)
		cout << "Hedge Mode:                 [3] Binance Futures[" << BinanceFuturesMarketName << "] / FTX Futures[" << FTXFuturesMarketName << "]\n\n";
	else
		return 0;

	if (mode == 1 || mode == 2) {
		size_t found = FTXSpotMarketName.find("/");
		if (found != string::npos) {
			coin = FTXSpotMarketName.substr(0, found);
		}
		else {
			cout << "Wrong FTX Spot Market Name!!\n";
			return 0;
		}
	}

	ftx_restClient.set_apikey(ftx_api_key, ftx_api_secret, ftx_subaccount);

	if (mode == 2 || mode == 3) {
		binance_restClient.set_apikey(binance_api_key, binance_api_secret, "");

		binanceDualSide = get_dual();
		if (binanceDualSide) {
			cout << "[Binacne] Current Position Mode is Hesge Mode. Please change to One-way Mode.";
			return 0;
		}

		auto ret2 = binance_restClient.get_exchange_info();
		//cout << ret2.dump(4) << "\n";
		if (ret2.contains("code")) {
			cout << "[Error] [" << ret2["code"].get<int>() << "] " << ret2["msg"].get<string>() << "                                           \n";
			return -1;
		}
		else {
			for (auto& key : ret2["symbols"].items()) {
				if (key.value()["symbol"].get<string>() == BinanceFuturesMarketName) {
					for (auto& key2 : key.value()["filters"].items()) {
						if (key2.value()["filterType"].get<string>() == "MARKET_LOT_SIZE") {
							binanceMinOrderSize = atof(key2.value()["minQty"].get<string>().c_str());
						}
					}
				}
			}
		}

		cout << "[Binance] Min Order Size:   [" << BinanceFuturesMarketName << "]:[" << binanceMinOrderSize << "] \n\n";

		if (batch_size < binanceMinOrderSize) {
			cout << "[Binance][" << BinanceFuturesMarketName << "] Min Market Ortder size is [" << binanceMinOrderSize << "], but Batch Size[" << batch_size << "] is too small!!\n";
			return 0;
		}
	}

	if (printInfo() == -1)
		return 0;


	//open websocket thread
	thread ftxWSReciveThread;
	thread binanceWSReciveThread;
	thread premiumThread;

	if (argc == 2) {
		ftxWSReciveThread = thread(&ftx_ws_ticks);
		if (mode == 2 || mode == 3)
			binanceWSReciveThread = thread(&binance_ws_ticks);
		premiumThread = thread(&get_premium);
	}


	double remainSize = place_size;
	double placed_size = 0;
	while (true) {
		{
			std::unique_lock<std::mutex> lock(m1);
			cond_var1.wait(lock, [] {return (openclose == "open" && openSignal) || (openclose == "close" && closeSignal); });
		}
		if (openclose == "open") {
			if (openSignal) {
				//balance min ordert size
				double size = min(min(openSizeMin.load(), remainSize),batch_size);
				if (mode == 2 || mode == 3) {
					if (size == remainSize && size < binanceMinOrderSize)
						break;
					size = size - fmod(size,binanceMinOrderSize);
				}

				future<long long> t1;
				future<long long> t2;
				if (mode == 1) {
					t1 = async(FTXShortFutures, size);
					t2 = async(FTXBuySpot, size);
				}
				else if (mode == 2) {
					t1 = async(BinanceShortFutures, size);
					t2 = async(FTXBuySpot, size);
				}
				else if (mode == 3) {
					t1 = async(BinanceShortFutures, size);
					t2 = async(FTXLongFutures, size);
				}


				auto r1 = t1.get();
				auto r2 = t2.get();
				if (r1 == 0 || r2 == 0)
					break;
				printTime();
				placed_size += size;
				remainSize -= size;
				cout << "\n======================================================================="<< "                                                \n";
				cout << "[Open] Size:" << size << " (" << placed_size << "/" << place_size << ")"<< "                                                \n";
				cout << "======================================================================="<< "                                                \n";


				this_thread::sleep_for(1000ms);
				double shortPrice;
				double buyPrice;
				if (mode == 1) {
					shortPrice = FTXGetOrderPrice(r1);
					buyPrice = FTXGetOrderPrice(r2);
				}
				else if (mode == 2) {
					shortPrice = BinanceGetOrderPrice(BinanceFuturesMarketName, r1);
					buyPrice = FTXGetOrderPrice(r2);
				}
				else if (mode == 3) {
					shortPrice = BinanceGetOrderPrice(BinanceFuturesMarketName, r1);
					buyPrice = FTXGetOrderPrice(r2);
				}

				if (shortPrice != 0 && buyPrice != 0) {
					double diffP = (shortPrice - buyPrice) / buyPrice * 100;
					cout << "\n======================================================================="<< "                                                \n";
					cout << "[Get Premium] " << round(diffP * 1000) / 1000 << "%  " << "buy: " << buyPrice << " short:" << shortPrice << "                                                \n";
					cout << "======================================================================="<< "                                                \n\n";
				}
				else {
					cout << "[Error] Get order price failed !!"<< "                                                \n";
					break;
				}
				printInfo();

				if (remainSize <= 0)
					break;
			}

		}
		else if (openclose == "close") {
			if (closeSignal) {
				double size = min(min(closeSizeMin.load(), remainSize),batch_size);
				if (mode == 2 || mode == 3) {
					if (size == remainSize && size < binanceMinOrderSize)
						break;
					size = size - fmod(size, binanceMinOrderSize);
				}

				future<long long> t1;
				future<long long> t2;
				if (mode == 1) {
					t1 = async(FTXLongFutures, size);
					t2 = async(FTXSellSpot, size);
				}
				else if (mode == 2) {
					t1 = async(BinanceLongFutures, size);
					t2 = async(FTXSellSpot, size);
				}
				else if (mode == 3) {
					t1 = async(BinanceLongFutures, size);
					t2 = async(FTXShortFutures, size);
				}
				auto r1 = t1.get();
				auto r2 = t2.get();
				if (r1 == 0 || r2 == 0)
					break;
				placed_size += size;
				remainSize -= size;
				cout << "\n======================================================================="<< "                                                \n";
				cout << "[Close] Size:" << size << " (" << placed_size << "/" << place_size << ")"<< "                                                \n";
				cout << "======================================================================="<< "                                                \n";

				
				this_thread::sleep_for(1000ms);
				double longPrice;
				double sellPrice;

				if (mode == 1) {
					longPrice = FTXGetOrderPrice(r1);
					sellPrice = FTXGetOrderPrice(r2);
				}
				else if (mode == 2) {
					longPrice = BinanceGetOrderPrice(BinanceFuturesMarketName,r1);
					sellPrice = FTXGetOrderPrice(r2);
				}
				else if (mode == 3) {
					longPrice = BinanceGetOrderPrice(BinanceFuturesMarketName, r1);
					sellPrice = FTXGetOrderPrice(r2);
				}

				if (longPrice != 0 && sellPrice != 0) {
					double diffP = (longPrice - sellPrice) / sellPrice * 100;
					cout << "\n======================================================================="<< "                                                \n";
					cout << "[Get Premium] " << round(diffP * 1000) / 1000 << "%  " << "sell: " << sellPrice << " long:" << longPrice << "                                                \n";
					cout << "======================================================================="<< "                                                \n\n";
				}
				else {
					cout << "[Error] Get order price failed !!"<< "                                                \n";
					break;
				}

				printInfo();

				if (remainSize <= 0)
					break;
			}
		}
		else {
			cout << "Error: please input 'open' or 'close'"<<"                                                \n";
			return 0;
		}
		//Avoid error:Do not send more then 2 orders total per 200ms
		//this_thread::sleep_for(200ms);
	}

	printInfo();

	if (argc == 2) {
		premiumStop = true;
		cond_var2.notify_all();
		ftx_wsClient.close();
		if (mode == 2 || mode == 3)
			binance_wsClient.close();

		premiumThread.join();
		ftxWSReciveThread.join();
		if (mode == 2 || mode == 3)
			binanceWSReciveThread.join();
	}
	return 0;
}
