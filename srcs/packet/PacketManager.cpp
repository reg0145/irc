#include "PacketManager.hpp"
#include <iostream>

void(*PacketManager::_sendPacketFunc)(int sessionIndex, std::string &res) = 0;

void PacketManager::init(char *password)
{
	_password = password;
	PacketManager::_sendPacketFunc = &SessionManager::sendPacketFunc;

	_recvFuntionDictionary["DISCONNECT"] = &PacketManager::processDisconnect;
	_recvFuntionDictionary["NICK"] = &PacketManager::processNick;
	_recvFuntionDictionary["PASS"] = &PacketManager::processPass;
	_recvFuntionDictionary["USER"] = &PacketManager::processUser;
	_recvFuntionDictionary["PING"] = &PacketManager::processPing;
}

void PacketManager::process(int sessionIndex, IRCMessage &message)
{
	std::map<std::string, PROCESS_RECV_PACKET_FUNCTION>::iterator it;
	it = _recvFuntionDictionary.find(message._command);

	if (it == _recvFuntionDictionary.end())
	{
		return ;
	}

	(this->*(it->second))(sessionIndex, message);
}

/* 해당 채널에 브로드캐스트 */
void PacketManager::braodcastChannel(const std::string &channelName, std::string &res)
{
	Channel *channel = _channelManager.getChannel(channelName);

	std::map<std::string, Client*>::iterator itClient;
	std::map<std::string, Client*> &clients = channel->getClients();

	for (itClient = clients.begin(); itClient != clients.end(); ++itClient)
	{
		int sessionIndex = itClient->second->getSessionIndex();
		_sendPacketFunc(sessionIndex, res);
	}
}

/* 모든 채널에 브로드캐스트 */
void PacketManager::broadcastChannels(std::set<std::string> &channelNames, std::string &res)
{
	std::set<std::string>::iterator itChannelName;

	for (itChannelName = channelNames.begin(); itChannelName != channelNames.end(); ++itChannelName)
	{
		braodcastChannel(*itChannelName, res);
	}
}

void PacketManager::processDisconnect(int sessionIndex, IRCMessage &req)
{
	(void)req;
	_clientManager.removeClient(sessionIndex);
	std::cout << ">> client[" << sessionIndex << "] Disconnected <<" << std::endl;
}

void PacketManager::processPass(int sessionIndex, IRCMessage &req)
{

	IRCMessage message;
	Client* client = _clientManager.getClient(sessionIndex);
	
	if (req._parameters.size() != 1)
	{
		message._command = "461";
		message._trailing = "Not enough parameters";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	if (req._parameters[0] != _password)
	{
		message._command = "464";
		message._trailing = "Password incorrect!";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}
	
	client->setPassTrue();

	message._command = "001";
	message._trailing = "process pass!";
	std::string res = message.toString();
	_sendPacketFunc(sessionIndex, res);
}

void PacketManager::processNick(int sessionIndex, IRCMessage &req)
{
	IRCMessage message;
	Client* client = _clientManager.getClient(sessionIndex);

//	if (!client->getIsPass())
//	{
//		message._command = "451";
//		message._trailing = "You have not registered";
//		std::string res = message.toString();
//		send(clientSocket, res.c_str(), res.size(), 0);
//		return ;
//	}

	/* 파라미터 1개 이상 존재하는가?*/
	if (req._parameters.size() != 1)
	{
		message._command = "461";
		message._trailing = "Not enough parameters";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	/* 닉네임 중복여부 확인 */
	std::string &nickname = req._parameters[0];
	if (_clientManager.checkNick(nickname))
	{
		message._command = "433";
		message._trailing = "Nickname is already in use";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	client->setNickname(nickname);

	_clientManager.addClient(sessionIndex, nickname);
}

void PacketManager::processUser(int sessionIndex, IRCMessage &req)
{
	if (_clientManager.checkClient(sessionIndex))
	{
		//sendPacketFunc();
		return ;
	}

	/* 파라미터 1개 이상 존재하는가?*/
	if (req._parameters.size() != 3)
	{
		//sendPacketFunc();
		return ;
	}

	std::string &nickname = req._parameters[0];
	if (_clientManager.checkNick(nickname))
	{
		//sendPacketFunc();
		return ;
	}


	Client* client = _clientManager.getClient(sessionIndex);
	std::string &hostname = req._parameters[1];
	std::string &name = req._parameters[2];
	client->setNickname(nickname);
	client->setName(hostname);
	client->setHostname(name);
}

void PacketManager::processPing(int sessionIndex, IRCMessage &req)
{
	if (req._parameters.size() != 1)
	{
		return ;
	}

	IRCMessage message;
	message._command = "PONG";
	message._parameters.push_back(req._parameters[0]);

	std::string res = message.toString();
	_sendPacketFunc(sessionIndex ,res);
}
