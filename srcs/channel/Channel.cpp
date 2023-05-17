#include "Channel.hpp"

Channel::Channel(std::string channelName)
{
	_channelName = channelName;
	_topic = "";
	_password = "";
	_limit = 0;
	_mode = 0;
}

Channel::~Channel()
{
}

void Channel::addClient(Client* client)
{
	_clients[client->getNickname()] = client;
}

void Channel::removeClient(Client* client)
{
	std::string nickname = client->getNickname();
	std::map<std::string, Client*>::iterator it = _clients.find(nickname);

	if (it != _clients.end())
	{
		_clients.erase(nickname);
	}
}

void Channel::removeClientByNickname(std::string nickname)
{
	std::map<std::string, Client*>::iterator it = _clients.find(nickname);

	if (it != _clients.end())
	{
		_clients.erase(nickname);
	}
}

void Channel::addOperator(std::string nickname)
{
	_operator.push_back(nickname);
}

void Channel::removeOperator(std::string nickname)
{
	_operator.remove(nickname);
}

bool Channel::isOperator(std::string nickname)
{
	std::list<std::string>::iterator it;
	for (it = _operator.begin(); it != _operator.end(); it++)
	{
		if (*it == nickname)
		{
			return true;
		}
	}
	return false;
}

bool Channel::isClientInChannel(std::string nickname)
{
	std::map<std::string, Client*>::iterator it = _clients.find(nickname);

	if (it != _clients.end())
	{
		return true;
	}
	return false;
}

std::list<std::string> Channel::getClientsName()
{
	std::list<std::string> clientsName;

	std::map<std::string, Client*>::iterator it;
	for (it = _clients.begin(); it != _clients.end(); it++)
	{
		clientsName.push_back(it->first);
	}
	return clientsName;
}

std::string &Channel::getChannelName()
{
	return _channelName;
}

int Channel::getClientCount()
{
	return _clients.size();
}

std::map<std::string, Client*> &Channel::getClients()
{
	return _clients;
}

bool Channel::isModeOn(int mode)
{
	return _mode & mode;
}

void Channel::setMode(int mode)
{
	_mode |= mode;
}

void Channel::unSetMode(int mode)
{
	_mode &= ~mode;
}

bool Channel::isLimitOver()
{
	if (_clients.size() > _limit)
	{
		return false;
	}
	return true;
}

bool Channel::isPasswordTrue(std::string password)
{
	if (_password != password)
	{
		return false;
	}
	return true;
}

void Channel::setTopic(std::string topic)
{
	_topic = topic;
}

void Channel::setLimit(std::string limit)
{
	_limit = std::stol(limit);
}

void Channel::setPassword(std::string password)
{
	_password = password;
}
