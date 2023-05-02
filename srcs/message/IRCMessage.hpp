#ifndef IRCMESSAGE_HPP
#define IRCMESSAGE_HPP

#include <list>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "IRCMessage.hpp"

class IRCMessage
{
	public:
		static std::list<IRCMessage> parse(const char* requests);
		static std::list<std::string> split(const std::string& str, const std::string& delimiter);

		std::string toString();

		std::string _prefix;
		std::string _command;
		std::vector<std::string> _parameters;
		std::string _trailing;

	private:
		static IRCMessage parseMessage(const std::string &request);
};

#endif