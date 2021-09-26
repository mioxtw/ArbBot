#define _CRT_SECURE_NO_WARNINGS
#include "rest/restclient.h"
#include "ws/wsclient.h"
#include <external/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include "utilws/Time.h"
#include <atomic>

using namespace std;
using json = nlohmann::json;



static string api_key = "";
static string api_secret = "";
static string subaccount_name = "";
static string openclose = "";
static string spotMarketName = "";
static string coin = "";
static double place_size = 0;
static double premium = 0;
static double batch_size = 0;

static ftx::WSClient wsClient;
static ftx::RESTClient restClient;
static double futuresBidPrice = 0;
static double spotAskPrice = 0;
static double spotBidPrice = 0;
static double futuresAskPrice = 0;

static double futuresBidSize = 0;
static double spotAskSize = 0;
static double spotBidSize = 0;
static double futuresAskSize = 0;

static double openPriceDiff = 0;
static double closePriceDiff = 0;

static double lastOpenPriceDiff = 0;
static double lastOpenSizeMin = 0;
static double lastClosePriceDiff = 0;
static double lastCloseSizeMin = 0;

static atomic<bool> openSignal(false);
static atomic<bool> closeSignal(false);
static atomic<double> openSizeMin(0);
static atomic<double> closeSizeMin(0);


long long buySpot(double size) {
	long long id = 0;
	json ord = restClient.place_order(spotMarketName, "buy", size);
	//cout<<ord<<"\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}

long long shortFutures(double size) {
	long long id = 0;
	json ord = restClient.place_order(coin + "-PERP", "sell", size);
	//cout<<ord<<"\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}


long long sellSpot(double size) {
	long long id = 0;
	json ord = restClient.place_order(spotMarketName, "sell", size);
	//cout << ord << "\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;
}

long long longFutures(double size) {
	long long id = 0;
	json ord = restClient.place_order(coin + "-PERP", "buy", size);
	//cout << ord << "\n";
	if (ord.contains("result"))
		id = ord.at("result").at("id").get<long long>();
	else if (ord.contains("error"))
		cout << "[Error] " << ord.at("error").get<string>() << "                                                \n";
	return id;

}


double getOrderPrice(long long id) {
	double price = 0;
	json ord = restClient.get_orders(id);
	//cout<<ord<<"\n";
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

void ws_ticks() {
	wsClient.subscribe_ticker(coin + "-PERP");
	wsClient.subscribe_ticker(spotMarketName);

	wsClient.on_message([](string msg) {
		json j = json::parse(msg);
		//cout << "bid: " << j.at("msg").at("data").at("bid").get<double>() <<"ask: " << j.at("msg").at("data").at("ask").get<double>() <<"\n"; 
		if (j.at("type").get<string>() == "update" && j.at("market").get<string>() == (coin + "-PERP")) {
			futuresBidPrice = j.at("data").at("bid").get<double>();
			futuresAskPrice = j.at("data").at("ask").get<double>();
			futuresBidSize = j.at("data").at("bidSize").get<double>();
			futuresAskSize = j.at("data").at("askSize").get<double>();
		}
		else if (j.at("type").get<string>() == "update" && j.at("market").get<string>() == (spotMarketName)) {
			spotAskPrice = j.at("data").at("ask").get<double>();
			spotBidPrice = j.at("data").at("bid").get<double>();
			spotAskSize = j.at("data").at("askSize").get<double>();
			spotBidSize = j.at("data").at("bidSize").get<double>();
		}



		openPriceDiff = futuresBidPrice - spotAskPrice;
		closePriceDiff = futuresAskPrice - spotBidPrice;
		openSizeMin = min(futuresBidSize, spotAskSize);
		closeSizeMin = min(futuresAskSize, spotBidSize);

		//auto start_time = std::chrono::high_resolution_clock::now();
		//auto end_time = std::chrono::high_resolution_clock::now();
		//auto time = end_time - start_time;

		if (openclose == "open" && futuresBidPrice != 0 && spotAskPrice != 0 && (openPriceDiff != lastOpenPriceDiff || openSizeMin != lastOpenSizeMin)) {

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
			lastOpenSizeMin = openSizeMin;
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

		});
	wsClient.connect();
}

int printBalance() {
	auto ret = restClient.get_balances();
	//cout << ret << endl;
	if (ret.contains("result")) {
		if (!ret["result"].empty()) {
			cout << "Balance:" << "                                                                   \n";
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
	//cout << ret << endl;
	if (ret.contains("result")) {
		cout << "\nMax Leverage:         [" << ret["result"]["leverage"].get<double>() << "]"<< "                                                \n";
		cout << "Current Leverage:     [" << ret["result"]["totalPositionSize"].get<double>()/ ret["result"]["totalAccountValue"].get<double>() << "]"<< "                                                \n\n";
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
	double sPos = 0;
	double fPos = 0;
	auto ret = restClient.get_balances();
	if (ret.contains("result")) {
		if (!ret["result"].empty()) {
			for (auto& key : ret["result"].items()) {
				if (key.value()["coin"].get<string>() == coin) {
					cout << "Spot Positions:       [" << key.value()["coin"].get<string>() << "]:     [" << key.value()["total"].get<double>() << "]" << "                                                \n";
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

	return 0;
}

int main(int argc, char* argv[])
{

	string ver = "v0.18";
	cout << "\n";
	cout << "--------------------------------------------------------\n";
	cout << " [Master Mio] FTX Spot Futures Arbitrage Bot [C++] " << ver << "\n";
	cout << " Auther:  [Mio]\n";
	cout << " Website: [http://miobtc.com]\n";
	cout << " Email:   [miox.tw@gmail.com]\n";
	cout << "--------------------------------------------------------\n\n";




	if (argc == 9) {
		api_key = argv[1];
		api_secret = argv[2];
		subaccount_name = argv[3];
		openclose = argv[4];
		spotMarketName = argv[5];
		size_t found = spotMarketName.find("/");
		if (found != string::npos) {
			coin = spotMarketName.substr(0, found);
		}
		else {
			cout << "Wrong Spot Market Name!!\n";
			return 0;
		}

		place_size = atof(argv[6]);
		premium = atof(argv[7]);
		batch_size = atof(argv[8]);

		cout << "SubAccount Name:      [" << subaccount_name << "]\n";
		cout << "Open/Close positions: [" << openclose << "]\n";
		cout << "Spot market name:     [" << spotMarketName << "]\n";
		cout << "Futures market name:  [" << coin + "-PERP" << "]\n";
		cout << "Size:                 [" << place_size << "]\n";
		cout << "Premium:              [" << premium*100 << " %]\n";
		cout << "Batch size limit:     [" << batch_size << "]\n\n";
	}
	else {
		cout << "Usage: " << argv[0] << " [APIKey] [APISecret] [SubAccount] [open/close] [spotMarketName] [Size] [Premium] [BatchSizeLimit]" << "\n";
		cout << "Example:\n";
		cout << "       " << argv[0] << " ABCDEFGHIJ 123456789 SUBACCOUNT open  FTT/USD 10 0.001 100" << "\n";
		cout << "       " << argv[0] << " ABCDEFGHIJ 123456789 SUBACCOUNT close FTT/USDT 10 -0.001 100" << "\n";
		return 0;
	}

	//set rest apikey
	restClient.set_apikey(api_key, api_secret, subaccount_name);

	if (printBalance() == -1)
		return 0;

	if (printLeverage() == -1)
		return 0;

	if (printPositions() == -1)
		return 0;


	//open websocket thread
	thread WSRecive;
	if (argc == 9)
		WSRecive = thread(&ws_ticks);


	double remainSize = place_size;
	double placed_size = 0;
	while (true) {
		if (openclose == "open") {
			if (openSignal) {
				double size = min(min(openSizeMin.load(), remainSize),batch_size);
				auto t1 = async(shortFutures, size);
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
				double shortPrice = getOrderPrice(r1);
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
				auto t1 = async(longFutures, size);
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
				double longPrice = getOrderPrice(r1);
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

	if (argc == 9) {
		wsClient.close();
		WSRecive.join();
	}
	return 0;
}
