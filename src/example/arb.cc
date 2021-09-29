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



static string api_key = "";
static string api_secret = "";
static string subaccount_name = "";
static string openclose = "";
static string futuresMarketName = "";
static string spotMarketName = "";
static string coin = "";
static double place_size = 0;
static double premium = 0;
static double batch_size = 0;

static ftx::WSClient wsClient;
static ftx::RESTClient restClient;
static binance::BWSClient bwsClient;
static binance::BRESTClient brestClient;



static atomic<double> futuresBidPrice(0);
static atomic<double> spotAskPrice(0);
static atomic<double> spotBidPrice(0);
static atomic<double> futuresAskPrice(0);

static atomic<double>  futuresBidSize(0);
static atomic<double>  spotAskSize(0);
static atomic<double>  spotBidSize(0);
static atomic<double>  futuresAskSize(0);

static double openPriceDiff = 0;
static double closePriceDiff = 0;

static double lastOpenPriceDiff = 0;
static double lastOpenSizeMin = 0;
static double lastClosePriceDiff = 0;
static double lastCloseSizeMin = 0;
static double sPos = 0;
static double fPos = 0;


static atomic<bool> openSignal(false);
static atomic<bool> closeSignal(false);
static atomic<bool> premiumStop(false);
static atomic<double> openSizeMin(0);
static atomic<double> closeSizeMin(0);
static atomic<bool> ftxWsStart(false);
static atomic<bool> binanceWsStart(false);


static bool binanceDualSide = false;

/*
long long BbuySpot(double size) {
	long long id = 0;
	json ord = brestClient.place_order(spotMarketName, "BUY", size, "LONG");
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}
*/
long long BshortFutures(double size) {
	long long id = 0;
	json ord = brestClient.place_order(futuresMarketName, "SELL", size, binanceDualSide?"SHORT":"BOTH");
	//cout << ord.dump(4) << "\n";
	if (ord.contains("orderId"))
		id = ord["orderId"].get<long long>();
	else if (ord.contains("code"))
		cout << "[Error] [" << ord["code"].get<int>() << "] "<< ord["msg"].get<string>()<<"                                           \n";
	return id;

}

/*
long long BsellSpot(double size) {
	long long id = 0;
	json ord = brestClient.place_order(spotMarketName, "SELL", size, "SHORT");
	//cout << ord.dump(4) << "\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;
}
*/

long long BlongFutures(double size) {
	long long id = 0;
	json ord = brestClient.place_order(futuresMarketName, "BUY", size, binanceDualSide ? "SHORT" : "BOTH");
	//cout << ord.dump(4) << "\n";
	if (ord.contains("orderId"))
		id = ord["orderId"].get<long long>();
	else if (ord.contains("code"))
		cout << "[Error] [" << ord["code"].get<int>() << "] " << ord["msg"].get<string>() << "                                           \n";
	return id;

}


double BgetOrderPrice(string market,long long id) {
	double price = 0;
	json ord = brestClient.get_orders(market, id);
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("orderId"))
		price = atof(ord["avgPrice"].get<string>().c_str());
	else if (ord.contains("code"))
		cout << "[Error] [" << ord["code"].get<int>() << "] " << ord["msg"].get<string>() << "                                           \n";
	return price;
}

#if 1
long long buySpot(double size) {
	long long id = 0;
	json ord = restClient.place_order(spotMarketName, "buy", size);
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}

long long shortFutures(double size) {
	long long id = 0;
	json ord = restClient.place_order(coin + "-PERP", "sell", size);
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}


long long sellSpot(double size) {
	long long id = 0;
	json ord = restClient.place_order(spotMarketName, "sell", size);
	//cout << ord.dump(4) << "\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;
}

long long longFutures(double size) {
	long long id = 0;
	json ord = restClient.place_order(coin + "-PERP", "buy", size);
	//cout << ord.dump(4) << "\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}


double getOrderPrice(long long id) {
	double price = 0;
	json ord = restClient.get_orders(id);
	//cout<<ord.dump(4)<<"\n";
	if (ord.contains("result") && !ord.at("result").at("avgFillPrice").is_null())
		price = ord.at("result").at("avgFillPrice").get<double>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return price;
}

#endif






void printTime() {

	auto msTime = utilws::get_ms_timestamp(utilws::current_time());
	long int ms = (msTime.count() % 1000);

	auto tp = chrono::time_point<chrono::system_clock, chrono::milliseconds>(msTime);
	auto tt = chrono::system_clock::to_time_t(tp);
	//std::tm* now = gmtime(&tt);
	std::tm* now = localtime(&tt);
	printf("[%4d/%02d/%02d %02d:%02d:%02d.%02ld] ", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, ms);
}

void ws_ticks() {
	//wsClient.subscribe_ticker(coin + "-PERP");
	wsClient.subscribe_ticker(spotMarketName);

	wsClient.on_message([](string msg) {
		json j = json::parse(msg);
		//cout << j.dump(4) << endl;
		//cout << "bid: " << j.at("msg").at("data").at("bid").get<double>() <<"ask: " << j.at("msg").at("data").at("ask").get<double>() <<"\n"; 
		//if (j.at("type").get<string>() == "update" && j.at("market").get<string>() == (coin + "-PERP")) {
		//	futuresBidPrice = j.at("data").at("bid").get<double>();
		//	futuresAskPrice = j.at("data").at("ask").get<double>();
		//	futuresBidSize = j.at("data").at("bidSize").get<double>();
		//	futuresAskSize = j.at("data").at("askSize").get<double>();
		//}
        if (j["type"].get<string>() == "update" && j["market"].get<string>() == (spotMarketName)) {
			spotAskPrice = j["data"]["ask"].get<double>();
			spotBidPrice = j["data"]["bid"].get<double>();
			spotAskSize = j["data"]["askSize"].get<double>();
			spotBidSize = j["data"]["bidSize"].get<double>();
			ftxWsStart = true;
		}
		});
	wsClient.connect();
}









void bws_ticks() {
	string lower_str = boost::algorithm::to_lower_copy(futuresMarketName);
	bwsClient.subscribe_ticker(lower_str);

	bwsClient.on_message([](string msg) {
		json j = json::parse(msg);
		//cout << j.dump(4) << "\n";
		if (j["e"].get<string>() == "bookTicker" && j["s"].get<string>() == futuresMarketName) {
			futuresBidPrice = atof(j["b"].get<string>().c_str());
			futuresAskPrice = atof(j["a"].get<string>().c_str());
			futuresBidSize = atof(j["B"].get<string>().c_str());
			futuresAskSize = atof(j["A"].get<string>().c_str());
			binanceWsStart = true;
		}


		});

	bwsClient.connect();
}

void get_premium() {
	while (true) {
		if (ftxWsStart && binanceWsStart)
			break;
	}

	while (true) {
		if (premiumStop)
			break;

		//cout << futuresBidPrice << "   " << spotAskPrice << "\n";

		openPriceDiff = futuresBidPrice - spotAskPrice;
		closePriceDiff = futuresAskPrice - spotBidPrice;
		openSizeMin = min(futuresBidSize.load(), spotAskSize.load());
		closeSizeMin = min(futuresAskSize.load(), spotBidSize.load());

		//auto start_time = std::chrono::high_resolution_clock::now();
		//auto end_time = std::chrono::high_resolution_clock::now();
		//auto time = end_time - start_time;

		if (openclose == "open" && futuresBidPrice != 0 && spotAskPrice != 0 && openSizeMin != 0 && (openPriceDiff != lastOpenPriceDiff || openSizeMin != lastOpenSizeMin)) {

			if (openPriceDiff / spotAskPrice > premium) {
				openSignal = true;
				printTime();
				cout << "[Websocket][open] Premium:[" << round(openPriceDiff / spotAskPrice * 100000) / 1000 << "%]" << "  Ask:[" << spotAskPrice << "]  Bid:[" << futuresBidPrice << "]  Size:[" << openSizeMin << "]                  \n";
			}
			else {
				openSignal = false;
			}

			printTime();
			cout << "[Websocket][open] Premium:[" << round(openPriceDiff / spotAskPrice * 100000) / 1000 << " %]" << "  Ask:[" << spotAskPrice << "]  Bid:[" << futuresBidPrice << "]  Size:[" << openSizeMin << "]                  \r" << flush;

			lastOpenPriceDiff = openPriceDiff;
			lastOpenSizeMin = openSizeMin.load();
		}



		else if (openclose == "close" && futuresAskPrice != 0 && spotBidPrice != 0 && (closePriceDiff != lastClosePriceDiff || closeSizeMin != lastCloseSizeMin)) {

			if (closePriceDiff / spotBidPrice < premium) {
				closeSignal = true;
				printTime();
				cout << "[Websocket][close] Premium:[" << round(closePriceDiff / spotBidPrice * 100000) / 1000 << " %]" << "  Bid:[" << spotBidPrice << "]   Ask:[" << futuresAskPrice << "]  Size:[" << closeSizeMin << "]                  \n";
			}
			else {
				closeSignal = false;
			}

			printTime();
			cout << "[Websocket][close] Premium:[" << round(closePriceDiff / spotBidPrice * 100000) / 1000 << " %]" << "  Bid:[" << spotBidPrice << "]   Ask:[" << futuresAskPrice << "]  Size:[" << closeSizeMin << "]                  \r" << flush;

			lastClosePriceDiff = closePriceDiff;
			lastCloseSizeMin = closeSizeMin;
		}

	}
}


int printBalance() {
	auto ret = restClient.get_balances();
	//cout << ret.dump(4) << endl;
	if (ret.contains("result")) {
		if (!ret["result"].empty()) {
			cout << "[FTX] Balance:" << "                                                                   \n";
			for (auto& key : ret["result"].items()) {
				if (key.value()["total"].get<double>() != 0)
					cout << "                      [" << key.value()["coin"].get<string>() << "]:[" << key.value()["total"].get<double>() << "]"<< "                                                \n";
			}

		}
		else {
			cout << "[Error] Please deposit USD or USDT !!"<< "                                                \n";
			return -1;
		}

	}
	else if (ret.contains("error")) {
		cout << "[Error] " << ret.at("error").get<string>() << "                                                \n";
		return -1;
	}

	return 0;
}

int printLeverage() {
	auto ret = restClient.get_account_info();
	//cout << ret.dump(4) << endl;
	if (ret.contains("result")) {
		cout << "\n[FTX] Max Leverage:         [" << ret["result"]["leverage"].get<double>() << "]"<< "                                                \n";
		cout << "[FTX] Current Leverage:     [" << ret["result"]["totalPositionSize"].get<double>()/ ret["result"]["totalAccountValue"].get<double>() << "]"<< "                                                \n\n";
	}
	else if (ret.contains("error")) {
		cout << "[Error] " << ret.at("error").get<string>() << "                                                \n";
		return -1;
	}

	return 0;
}

int printPositions() {
	string sName = coin;
	string fName = coin + "-PERP";
	auto ret = restClient.get_balances();
	if (ret.contains("result")) {
		if (!ret["result"].empty()) {
			for (auto& key : ret["result"].items()) {
				if (key.value()["coin"].get<string>() == coin) {
					cout << "[FTX] Spot Positions:           [" << key.value()["coin"].get<string>() << "]:     [" << key.value()["total"].get<double>() << "]" << "                                                \n";
					sPos = key.value()["total"].get<double>();
				}
			}

		}
		else {
			cout << "[Error] Please deposit USD or USDT !!"<< "                                                \n";
			return -1;
		}

	}
	else if (ret.contains("error")) {
		cout << "[Error] " << ret.at("error").get<string>() << "                                                \n";
		return -1;
	}



#if 0
	auto ret2 = restClient.get_account_info();
	if (ret2.contains("result")) {
		for (auto& key : ret2["result"]["positions"].items()) {
			if (key.value()["future"].get<string>() == coin + "-PERP") {
				cout << "Futures Positions:    [" << key.value()["future"].get<string>() << "]:[" << key.value()["netSize"].get<double>() << "]" << "                                                \n\n";
				fPos = key.value()["netSize"].get<double>();
			}
		}
	}
	else if (ret2.contains("error")) {
		cout << "[Error] " << ret2.at("error").get<string>() << "                                                \n";
		return -1;
	}

	if (sPos != fPos * (-1))
		cout << "\n[Warning]"<<"[" << sName << "]["<<sPos<<"] <-> ["<< fName<<"]["<<fPos<<"]" << " Please check your positions now  !!!!!!!!!!!!!!!!!!!!!!!!!\n\n";
#endif


	auto ret2 = brestClient.get_position(futuresMarketName);
	//cout << ret2.dump(4) << "\n";
	if (ret2.contains("code")) {
		cout << "[Error] [" << ret2["code"].get<int>() << "] " << ret2["msg"].get<string>() << "                                           \n";
		return -1;
	}
	else {
		for (auto& key : ret2.items()) {
			if (binanceDualSide) {
				if (key.value()["positionSide"].get<string>() == "SHORT") {
					cout << "[Binance] Futures Positions:    [" << key.value()["symbol"].get<string>() << "]:[" << key.value()["positionAmt"].get<string>() << "]" << "                                                \n\n";
					fPos = atof(key.value()["positionAmt"].get<string>().c_str());
				}
			}
			else {
				cout << "[Binance] Futures Positions:    [" << key.value()["symbol"].get<string>() << "]:[" << key.value()["positionAmt"].get<string>() << "]" << "                                                \n\n";
				fPos = atof(key.value()["positionAmt"].get<string>().c_str());
			}
		}
	}


	return 0;
}

bool get_dual() {
	bool dual = false;
	json ord = brestClient.get_dual();
	if (ord.contains("dualSidePosition"))
		dual = ord["dualSidePosition"].get<bool>();
	else if (ord.contains("code"))
		cout << "[Error] [" << ord["code"].get<int>() << "] " << ord["msg"].get<string>() << "                                           \n";
	return dual;
}


int main(int argc, char* argv[])
{
	string ver = "v0.1.2";
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

		coin = config["hedge-coin"].get<string>();
		spotMarketName = config["ftx-spotMarketName"].get<string>();
		futuresMarketName = config["binance-futuresMarketName"].get<string>();
		openclose = config["openclose"].get<string>();
		place_size = config["total-place-size"].get<double>();
		premium = config["premium"].get<double>();
		batch_size = config["batch-place-size"].get<double>();
		ftx_api_key = config["ftx-api-key"].get<string>();
		ftx_api_secret = config["ftx-api-secret"].get<string>();
		ftx_subaccount = config["ftx-subaccount"].get<string>();
		binance_api_key = config["binance-api-key"].get<string>();
		binance_api_secret = config["binance-api-secret"].get<string>();

		cout << "Open/Close positions: [" << openclose << "]\n";
		cout << "Spot market name:     [" << spotMarketName << "]\n";
		cout << "Futures market name:  [" << futuresMarketName << "]\n";
		cout << "Size:                 [" << place_size << "]\n";
		cout << "Premium:              [" << premium*100 << " %]\n";
		cout << "Batch size limit:     [" << batch_size << "]\n\n";
	}
	else {
		cout << "Usage: " << argv[0] << " [config json]" << "\n";
		cout << "Example:\n";
		cout << "       " << argv[0] << " config.json" << "\n";
		cout << "       " << argv[0] << " config2.json" << "\n";
		return 0;
	}



	brestClient.set_apikey(binance_api_key, binance_api_secret, "");
	binanceDualSide = get_dual();

	//set rest apikey
	restClient.set_apikey(ftx_api_key, ftx_api_secret, ftx_subaccount);

	if (printBalance() == -1)
		return 0;

	if (printLeverage() == -1)
		return 0;

	if (printPositions() == -1)
		return 0;

	if (openclose == "close" && place_size > min(sPos, fPos)) {
		place_size = min(sPos, fPos);
		cout << "Size Changed:                 [" << place_size << "]\n";

		if (place_size == 0)
			return 0;
	}


	//open websocket thread
	thread WSRecive;
	thread BWSRecive;
	thread premiumT;

	if (argc == 2) {
		WSRecive = thread(&ws_ticks);
		BWSRecive = thread(&bws_ticks);
		premiumT = thread(&get_premium);
	}



	double remainSize = place_size;
	double placed_size = 0;
	while (true) {
		if (openclose == "open") {
			if (openSignal) {
				double size = min(min(openSizeMin.load(), remainSize),batch_size);
				auto t1 = async(BshortFutures, size);
				auto t2 = async(buySpot, size);
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




				//太快查order ID，average price會拿到null，所以sleep一下再查
				this_thread::sleep_for(1000ms);
				double shortPrice = BgetOrderPrice(futuresMarketName,r1);
				double buyPrice = getOrderPrice(r2);
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
				printLeverage();
				printPositions();
				if (remainSize <= 0)
					break;
			}

		}
		else if (openclose == "close") {
			if (closeSignal) {
				double size = min(min(closeSizeMin.load(), remainSize),batch_size);
				auto t1 = async(BlongFutures, size);
				auto t2 = async(sellSpot, size);
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
				double longPrice = BgetOrderPrice(futuresMarketName,r1);
				double sellPrice = getOrderPrice(r2);
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
				printLeverage();
				printPositions();
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

	printLeverage();
	printPositions();

	if (argc == 2) {
		premiumStop = true;
		wsClient.close();
		bwsClient.close();

		premiumT.join();
		WSRecive.join();
		BWSRecive.join();
	}
	return 0;
}
