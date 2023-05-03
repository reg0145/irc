#include "PacketManager.hpp"
#include <iostream>

void (*PacketManager::_sendPacketFunc)(int sessionIndex, std::string &res) = 0;

void PacketManager::init(char* password)
{
	_password = password;
	PacketManager::_sendPacketFunc = &SessionManager::sendPacketFunc;

	_recvFuntionDictionary["DISCONNECT"] = &PacketManager::processDisconnect;
	_recvFuntionDictionary["NICK"] = &PacketManager::processNick;
	_recvFuntionDictionary["PASS"] = &PacketManager::processPass;
	_recvFuntionDictionary["USER"] = &PacketManager::processUser;
	_recvFuntionDictionary["PING"] = &PacketManager::processPing;
	_recvFuntionDictionary["JOIN"] = &PacketManager::processJoin;
	_recvFuntionDictionary["PRIVMSG"] = &PacketManager::processPrivmsg;
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
void PacketManager::broadcastChannel(const std::string &channelName, std::string &res)
{
	Channel *channel = _channelManager.getChannel(channelName);

	std::map<std::string, Client*>::iterator itClient;
	std::map<std::string, Client*> &clients = channel->getClients();

	for (itClient = clients.begin(); itClient != clients.end(); itClient++)
	{
		int sessionIndex = itClient->second->getSessionIndex();
		_sendPacketFunc(sessionIndex, res);
	}
}

/* 모든 채널에 브로드캐스트 */
void PacketManager::broadcastChannels(const std::set<std::string> &channelNames, std::string &res)
{
	std::set<std::string>::iterator itChannelName;

	for (itChannelName = channelNames.begin(); itChannelName != channelNames.end(); itChannelName++)
	{
		broadcastChannel(*itChannelName, res);
	}
}

void PacketManager::broadcastChannelsWithoutMe(int sessionIndex, const std::set<std::string> &channelNames, std::string &res)
{
	std::set<std::string>::iterator itChannelName;

	for (itChannelName = channelNames.begin(); itChannelName != channelNames.end(); itChannelName++)
	{
		Channel *channel = _channelManager.getChannel(*itChannelName);

		std::map<std::string, Client*>::iterator itClient;
		std::map<std::string, Client*> &clients = channel->getClients();
		for (itClient = clients.begin(); itClient != clients.end(); itClient++)
		{
			int clientSessionIndex = itClient->second->getSessionIndex();
			if (sessionIndex != clientSessionIndex)
			{
				_sendPacketFunc(clientSessionIndex, res);
			}
		}
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
}

void PacketManager::processNick(int sessionIndex, IRCMessage &req)
{
	IRCMessage message;

	if (_clientManager.isFailedPass(sessionIndex))
	{
		message._command = "ERROR";
		message._trailing = "Authentication failed";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	/* 파라미터 1개 이상 존재하는가?*/
	if (req._parameters.size() != 1)
	{
		message._command = "461";
		message._trailing = "Not enough parameters";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	/* 닉네임 유효성 검사 */
	std::string &newNickname = req._parameters[0];
	if (!_clientManager.isValidNickname(newNickname))
	{
		message._command = "432";
		message._parameters.push_back(newNickname);
		message._trailing = "Erroneous nickname";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	/* 닉네임 중복여부 확인 */
	if (_clientManager.isUsedNickname(newNickname))
	{
		message._command = "433";
		message._parameters.push_back(newNickname);
		message._trailing = newNickname;
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	/* 닉네임 처음 등록된 경우 */
	Client* client = _clientManager.getClient(sessionIndex);
	std::string oldNickname = client->getNickname();
	if (oldNickname == "")
	{
		_clientManager.changeNickname(sessionIndex, oldNickname, newNickname);
		message._command = "001";
		message._parameters.push_back(newNickname);
		message._trailing = "Welcome to the Internet Relay Network, " + newNickname;
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	/* 닉네임 이미 등록된 경우 */
	_clientManager.changeNickname(sessionIndex, oldNickname, newNickname);
	_channelManager.changeNickname(client, oldNickname, newNickname);

	/* - 가입된 채널의 유저들에게 NICK 변경 알림 */
	std::set<std::string> joinedChannelNames = client->getChannels();
	if (joinedChannelNames.size() > 0)
	{
		message._prefix = oldNickname + "!~b@" + client->getServername();
		message._command = "NICK";
		message._trailing = newNickname;
		std::string res = message.toString();
		broadcastChannelsWithoutMe(sessionIndex, joinedChannelNames, res);
	}

	message._prefix = oldNickname + "!~b@" + client->getServername();
	message._command = "NICK";
	message._trailing = newNickname;
	std::string res = message.toString();
	_sendPacketFunc(sessionIndex, res);
}

void PacketManager::processUser(int sessionIndex, IRCMessage &req)
{
	IRCMessage message;

	if (_clientManager.checkClient(sessionIndex))
	{
		//sendPacketFunc();
		return ;
	}

	if (req._parameters.size() != 3)
	{
		//sendPacketFunc();
		return ;
	}

	std::string &nickname = req._parameters[0];
	if (_clientManager.isUsedNickname(nickname))
	{
		//sendPacketFunc();
		return ;
	}

	Client* client = _clientManager.getClient(sessionIndex);
	client->setUsername(req._parameters[0]);
	client->setHostname(req._parameters[1]);
	client->setServername(req._parameters[2]);
	client->setName(req._trailing);
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

void PacketManager::processJoin(int sessionIndex, IRCMessage &req)
{
	IRCMessage message;
	Client* client = _clientManager.getClient(sessionIndex);

	if (_clientManager.checkClient(sessionIndex))
	{
		return ;
	}

	if (req._parameters.size() != 1)
	{
		message._command = "461";
		message._trailing = "Not enough parameters";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return ;
	}

	std::list<std::string> channelNames = IRCMessage::split(req._parameters[0], ",");
	std::list<std::string>::iterator itChannelName;
	for (itChannelName = channelNames.begin(); itChannelName != channelNames.end(); itChannelName++)
	{
		if (_channelManager.isValidChannelName(*itChannelName) == false)
		{
			message._command = "403";
			message._parameters.push_back(*itChannelName);
			message._trailing = "No such channel";
			std::string res = message.toString();
			_sendPacketFunc(sessionIndex, res);
			return ;
		}
	}

	for (itChannelName = channelNames.begin(); itChannelName != channelNames.end(); itChannelName++)
	{
		message._parameters.clear();
		if (_clientManager.isJoinedChannel(sessionIndex, *itChannelName))
		{
			message._command = "443";
			message._parameters.push_back(client->getNickname());
			message._parameters.push_back(*itChannelName);
			message._trailing = "is already on channel";
			std::string res = message.toString();
			_sendPacketFunc(sessionIndex, res);
			continue ;
		}
		if (_channelManager.enterClient(*itChannelName, client) == FAIL)
		{
			/* new Channel 실패(malloc 실패) 코드 */
			return ;
		}
		message._command = "353";
		message._parameters.push_back(client->getNickname());
		message._parameters.push_back("=");
		message._parameters.push_back(*itChannelName);
		message._trailing = _channelManager.getChannelInfo(*itChannelName);
		std::string res = message.toString();
		broadcastChannel(*itChannelName, res);
	}
}

void PacketManager::processPrivmsg(int sessionIndex, IRCMessage &req)
{
	IRCMessage message;
	std::string nickname = _clientManager.getClient(sessionIndex)->getNickname();

	if (_clientManager.checkClient(sessionIndex) == FAIL)
	{
		return;
	}
	if (req._parameters.size() != 1)
	{
		message._command = "411";
		message._trailing = "No recipient given (PRIVMSG)";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return;
	}
	if (req._trailing == "")
	{
		message._command = "412";
		message._trailing = "No text to send";
		std::string res = message.toString();
		_sendPacketFunc(sessionIndex, res);
		return;
	}
	std::list<std::string> targets = IRCMessage::split(req._parameters[0], ",");
	for (std::list<std::string>::const_iterator it = targets.begin(); it != targets.end(); ++it)
	{
		memset(&message, 0, sizeof(IRCMessage));
		if ((*it)[0] == '#')
		{
			Channel* channel = _channelManager.getChannel(*it);
			if (!channel)
			{
				message._command = "401";
				message._parameters.push_back(*it);
				message._trailing = "No such nick/channel";
				std::string res = message.toString();
				_sendPacketFunc(sessionIndex, res);
				continue;
			}
			if (!channel->isClientInChannel(nickname))
			{
				message._command = "404";
				message._parameters.push_back(*it);
				message._trailing = "Cannot send to channel";
				std::string res = message.toString();
				_sendPacketFunc(sessionIndex, res);
				continue;
			}
			message._prefix = nickname;
			message._command = "PRIVMSG";
			message._parameters.push_back(*it);
			message._trailing = req._trailing;
			std::string res = message.toString();
			broadcastChannel(channel->getChannelName(), res);
		}
		else
		{
			Client* client = _clientManager.getClientByNickname(*it);
			if (!client)
			{
				message._command = "401";
				message._parameters.push_back(*it);
				message._trailing = "No such nick/channel";
				std::string res = message.toString();
				_sendPacketFunc(sessionIndex, res);
				continue;
			}
			message._prefix = nickname;
			message._command = "PRIVMSG";
			message._parameters.push_back(*it);
			message._trailing = req._trailing;
			std::string res = message.toString();
			_sendPacketFunc(client->getSessionIndex(), res);
		}
	}
}
