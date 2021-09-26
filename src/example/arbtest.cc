#include "rest/restclient.h"
#include "ws/wsclient.h"
#include <external/json.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <time.h>


using namespace std;
using json = nlohmann::json;
static float futuresBidPrice = 0;
static float spotAskPrice = 0;
static float spotBidPrice = 0;
static float futuresAskPrice = 0;

static float futuresBidSize = 0;
static float spotAskSize = 0;
static float spotBidSize = 0;
static float futuresAskSize = 0;

static float openPriceDiff = 0;
static float closePriceDiff = 0;

static float openSizeCMin = 0;
static float closeSizeCMin = 0;

static float lastOpenPriceDiff = 0;
static float lastOpenSizeCMin = 0;
static float lastClosePriceDiff = 0;
static float lastCloseSizeCMin = 0;

//static float totalSize = 100;
static thread WSRecive;
//static float sizeSum = 0;
static bool positionOpened = false;
static bool openSignal = false;
static bool closeSignal = false;


#if 1
long buySpot() {
    ftx::RESTClient client;
    json ord = client.place_order("FTT/USD", "buy", 0.1);
    //cout<<ord<<"\n";
    long id = ord.at("result").at("id").get<long>();

    return id;

}

long shortFutures() {
    ftx::RESTClient client;
    json ord = client.place_order("FTT-PERP", "sell", 0.1);
    //cout<<ord<<"\n";
    long id = ord.at("result").at("id").get<long>();
    return id;

}


long sellSpot() {
    ftx::RESTClient client;
    json ord = client.place_order("FTT/USD", "sell", 0.1);
    //cout<<ord<<"\n";
    long id = ord.at("result").at("id").get<long>();
    return id;
}

long longFutures() {
    ftx::RESTClient client;
    json ord = client.place_order("FTT-PERP", "buy", 0.1);
    //cout<<ord<<"\n";
    long id = ord.at("result").at("id").get<long>();
    return id;

}


float getOrderPrice(long id) {
    ftx::RESTClient client;
    json ord = client.get_orders(id);
    //cout<<ord<<"\n";
    float price = ord.at("result").at("avgFillPrice").get<float>();
    return price;
}


#endif


void printTime() {
    char buffer[30];
    struct timeval tv;
    time_t curtime;

    gettimeofday(&tv, NULL);
    curtime=tv.tv_sec;

    strftime(buffer,30,"[%m-%d-%Y %T.",localtime(&curtime));
    printf("%s%ld] ",buffer,tv.tv_usec);

}


void ws_ticks() {

    ftx::WSClient client;
    client.subscribe_ticker("FTT-PERP");
    client.subscribe_ticker("FTT/USD");

    client.on_message([](string msg) {
        json j = json::parse(msg);
        //cout << "bid: " << j.at("msg").at("data").at("bid").get<float>() <<"ask: " << j.at("msg").at("data").at("ask").get<float>() <<"\n"; 
        if (j.at("type").get<string>() == "update" && j.at("market").get<string>() =="FTT-PERP") {
            futuresBidPrice = j.at("data").at("bid").get<float>();
            futuresAskPrice = j.at("data").at("ask").get<float>();
            futuresBidSize = j.at("data").at("bidSize").get<float>();
            futuresAskSize = j.at("data").at("askSize").get<float>();
        }
        else if (j.at("type").get<string>() == "update" && j.at("market").get<string>() =="FTT/USD") {
            spotAskPrice = j.at("data").at("ask").get<float>();
            spotBidPrice = j.at("data").at("bid").get<float>();
            spotAskSize = j.at("data").at("askSize").get<float>();
            spotBidSize = j.at("data").at("bidSize").get<float>();
         }


        

        openPriceDiff = futuresBidPrice - spotAskPrice;
        closePriceDiff = futuresAskPrice - spotBidPrice;
        openSizeCMin = min(futuresBidSize, spotAskSize);
        closeSizeCMin = min(futuresAskSize, spotBidSize);



	//auto now = chrono::system_clock::now();
	//time_t t = chrono::system_clock::to_time_t(now);
	//auto end_time = std::chrono::high_resolution_clock::now();
	//auto time = end_time - start_time;


        if (futuresBidPrice != 0 && spotAskPrice != 0  && (openPriceDiff != lastOpenPriceDiff ||  openSizeCMin != lastOpenSizeCMin)) {
            if (openPriceDiff/spotAskPrice > 0.001){
                //sizeSum += sizeCMin;
                openSignal = true;
                closeSignal = false;
                printTime();
                cout << round(openPriceDiff/spotAskPrice*100000)/1000 << "%:"<< "  ask:" <<spotAskPrice<<"   bid:"<<futuresBidPrice<<"\n";

            } 
            lastOpenPriceDiff = openPriceDiff;
            lastOpenSizeCMin = openSizeCMin;
        }



        if (futuresAskPrice != 0 && spotBidPrice != 0  && (closePriceDiff != lastClosePriceDiff ||  closeSizeCMin != lastCloseSizeCMin)) {
            if (closePriceDiff/spotBidPrice < -0.001){
                //sizeSum += sizeCMin;
                closeSignal = true;
                openSignal = false;
                printTime();
                cout <<round(closePriceDiff/spotBidPrice*100000)/1000 << " %:"<< "  bid:" <<spotBidPrice<<"   ask:"<<futuresAskPrice<<"\n";
            }
        }

    });

    client.connect();
}



int main()
{

  if (!WSRecive.joinable()) {  
       //開啟報價
       WSRecive = thread(&ws_ticks);
  }



  while(true) {
        while(true) {

            if (openSignal && !positionOpened) {
                auto t1 = async(shortFutures);
                auto t2 = async(buySpot);


	       	this_thread::sleep_for(1000ms);
	        float shortPrice = getOrderPrice(t1.get());
	        float buyPrice = getOrderPrice(t2.get());
                float diffP = (shortPrice-buyPrice)/buyPrice*100;

	        printTime();
                cout<< round(diffP*1000)/1000 <<"%  "<<"buy: "<< buyPrice << " short:"<<shortPrice <<" ==========>已下單\n";


	        positionOpened = true;
                break;
            }
        }

        while(true) {
            if(closeSignal && positionOpened) {
                auto t1 = async(longFutures);
                auto t2 = async(sellSpot);

       		this_thread::sleep_for(1000ms);
	        float longPrice = getOrderPrice(t1.get());
	        float sellPrice = getOrderPrice(t2.get());
                float diffP = (longPrice-sellPrice)/sellPrice*100;

	        printTime();
                cout<< round(diffP*1000)/1000 <<"%  "<< "sell: "<< sellPrice << " long:"<<longPrice <<" ==========>已下單\n";

        	positionOpened = false;
                break;
            }
        }

   }


   WSRecive.join();
   return 0;
}
