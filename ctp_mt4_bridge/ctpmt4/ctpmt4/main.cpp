#define _CRT_SECURE_NO_WARNINGS

#pragma comment(lib, "thosttraderapi.lib")
#include "ThostFtdcTraderApi.h"
#include "ThostFtdcUserApiDataType.h"
#include "ThostFtdcUserApiStruct.h"
#include "tcpserver.h"

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <chrono>
#include <thread>
#include <fstream>
using namespace std;

#define CONNECT_LOCAL
#define Fctp1_TradeAddress 

std::string userid;
std::string password;
std::string brokerid;
char frontaddr[1024];
tcpserver* server;

void QryPosition(CThostFtdcTraderApi *api);

class CTraderBaseSpiService : public CThostFtdcTraderSpi
{
public:
	FILE *fp;
	CTraderBaseSpiService(CThostFtdcTraderApi *userApi)
	{
		std::ifstream param = std::ifstream("config.txt", std::ios::in);
		param >> userid;
		param >> password;
		param >> brokerid;
		param >> frontaddr;
		int port;
		param >> port;
		server = new tcpserver(port);

		m_pUserApi = userApi;
		fopen_s(&fp, "out", "w");
	}
	virtual void OnFrontConnected()
	{
		cout << "On Front Connected" << endl;
		CThostFtdcReqUserLoginField field;
		strcpy_s(field.BrokerID, brokerid.c_str());
		strcpy_s(field.UserID, userid.c_str());
		strcpy_s(field.Password, password.c_str());
		int rtn = m_pUserApi->ReqUserLogin(&field, 1);
		cout << "send login " << " " << rtn << endl;
	}
	virtual void OnFrontDisconnected(int nReason) {};
	virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};
	virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
		{
			printf("Login ErrorID[%d] ErrMsg[%s]\n", pRspInfo->ErrorID, pRspInfo -> ErrorMsg);
			return;
		}
		else
		{
			printf("Login ErrrorID[%d] ErrMsg[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			QryPosition(m_pUserApi);
		}
	};
	
	virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, 
		CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
	{
		if (pRspInfo && pRspInfo->ErrorID != 0)
		{
			printf("Login ErrorID[%d] ErrMsg[%s]\n", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
			return;
		}
		map<std::string, int>::iterator it;
		it = holding.find(pInvestorPosition->InstrumentID);
		if (it == holding.end())
		{
			std::stringstream ss;
			ss << pInvestorPosition->InstrumentID << ": " << pInvestorPosition->Position;
			server->sendmsg(ss.str());
			std::cout << ss.str() << std::endl;
			holding.insert(std::pair<std::string, int>(pInvestorPosition->InstrumentID, pInvestorPosition->Position));
		}
		else
		{
			int volume_orgn = holding[pInvestorPosition->InstrumentID];
			if (volume_orgn != pInvestorPosition->Position)
			{
				std::stringstream ss;
				ss << pInvestorPosition->InstrumentID << ": " << pInvestorPosition->Position - volume_orgn;
				server->sendmsg(ss.str());
				std::cout << ss.str() << std::endl;
				holding[pInvestorPosition->InstrumentID] = pInvestorPosition->Position;
			}
		}
	}

	std::map<std::string, int> holding;
	

private:
	CThostFtdcTraderApi *m_pUserApi;
};

void QryPosition(CThostFtdcTraderApi *api)
{
	CThostFtdcQryInvestorPositionField req = { 0 };
	strcpy(req.BrokerID, brokerid.c_str());
	strcpy(req.InvestorID, userid.c_str());
	api->ReqQryInvestorPosition(&req, 0);
	std::this_thread::sleep_for(1s);
}

int main()
{
	CThostFtdcTraderApi *pUserApi;
	pUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi();
	CThostFtdcTraderSpi *pUserSpi;
	pUserSpi = new CTraderBaseSpiService(pUserApi);
	pUserApi->RegisterSpi(pUserSpi);
	int nFrontAddressPos = 0;
	pUserApi->RegisterFront(frontaddr);
	pUserApi->SubscribePrivateTopic(THOST_TERT_RESTART);
	pUserApi->SubscribePublicTopic(THOST_TERT_RESTART);
	pUserApi->Init();

	std::this_thread::sleep_for(3s);
	while (true)
	{
		QryPosition(pUserApi);
	}

	pUserApi->Join();
	return 0;
}