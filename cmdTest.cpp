#include "command.hpp"
#include <string>
#include <iostream>
int main (int argc, char** argv)
{
    Utility::Command command;
    command.add<std::string> ("protocal", 's', "transfer protocol", true, "http");
    command.add<std::string> ("domain", 'd', "domain", false);
    command.add<int> ("port", 'p', "port", true, 80, [] (int x) -> bool{return x > 0;});
    command.parse (argc, argv);
    std::cout << command.usage () << std::endl;
    std::cout << command.get<std::string> ("protocal") << "://" << command.get<std::string> ("domain") << ":" << command.get<int> ("port") << std::endl;
}