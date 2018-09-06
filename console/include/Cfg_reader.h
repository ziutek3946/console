#ifndef CFG_READER_H
#define CFG_READER_H

#include <rapidjson/document.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <mosquittopp.h>

class Cfg_reader
{
    public:
        Cfg_reader();
        virtual ~Cfg_reader();
        std::vector<std::string> GetMacs();
        std::vector<std::string> GetCommandsForMac(std::string mac);
        void translate_k2_k0(std::string const& line);
        //std::vector<std::string> GetArgsForCommand(std::string command, rapidjson::Document& json1, rapidjson::Document& json2);
    protected:
    private:
        std::string names, types, methods;
        rapidjson::Document json1, json2, json3;
        void ReadFile();
        std::vector<std::string> tokenizacja(std::string const& line);
};

#endif // CFG_READER_H
