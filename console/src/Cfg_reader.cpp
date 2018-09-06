#include "Cfg_reader.h"

Cfg_reader::Cfg_reader()
{
    names = "names.json";
    types = "types.json";
    methods = "methods.json";
    ReadFile();
}

Cfg_reader::~Cfg_reader()
{
    //dtor
}

std::vector<std::string> Cfg_reader::GetMacs()
{
    std::vector<std::string> macs;

    assert(json1.IsArray());
    for(auto& it : json1.GetArray())
    {
        macs.push_back(it.GetObject()["mac"].GetString());
    }

    return macs;
}

std::vector<std::string> Cfg_reader::GetCommandsForMac(std::string mac)
{
    int type = 0;

    assert(json2.IsArray());
    for (auto& it : json2.GetArray())
    {
        assert(it.IsObject());
        if (it.GetObject()["mac"].GetString() == mac)
        {
            type = it.GetObject()["typ"].GetInt();
        }
    }

    std::vector<std::string> rozkazy;

    assert(json3.IsArray());
    for (auto& it : json3.GetArray())
    {
        assert(it.IsObject());
        if (it.GetObject()["typ"].GetInt() == type)
        {
            //const rapidjson::Array& a = it.GetObject()["rozkazy"].GetArray();
            rapidjson::Value a = it.GetObject()["rozkazy"].GetArray();
            for (size_t i=0;i<a.Size();++i)
            {
                rozkazy.push_back(a[i].GetString());
            }
        }
    }
    return rozkazy;
}


void Cfg_reader::ReadFile()
{
    std::fstream plik;
    std::string json, temp;

    plik.open(names.c_str(), std::ios::in);
    while(!plik.eof())
    {
        plik >> temp;
        json += temp;
    }
    json1.Parse(json.c_str());
    json = "";
    plik.close();

    //==============

    plik.open(types.c_str(), std::ios::in);
    while(!plik.eof())
    {
        plik >> temp;
        json += temp;
    }
    json2.Parse(json.c_str());
    json = "";
    plik.close();

    //==============

    plik.open(methods.c_str(), std::ios::in);
    while(!plik.eof())
    {
        plik >> temp;
        json += temp;
    }
    json3.Parse(json.c_str());
    json = "";
    plik.close();
}

std::vector<std::string> Cfg_reader::tokenizacja(std::string const& line)
{
    std::vector<std::string> _s;
    std::vector<std::string>& s = _s;
    std::string temp = "";
    for(size_t i=0;i<line.size();++i)
    {
        if(line[i] == ' ')
        {
            s.push_back(temp);
            temp = "";
        }
        else
        {
            temp += line[i];
        }
    }
    s.push_back(temp);
    return s;
}

void Cfg_reader::translate_k2_k0(std::string const& line)
{
    std::vector<std::string> tokeny = tokenizacja(line);
    std::string ramka;
    ramka += "/d/k/";
    ramka += tokeny[0];
    ramka += "/"
}
