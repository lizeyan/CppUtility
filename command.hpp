#ifndef __COMMAND___
#define __COMMAND___
#include <string>
#include <typeinfo>
#include <vector>
#include <utility>
#include <functional>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>
#include <sstream>
#include <memory>
#include <iterator>
#include <iomanip>
namespace Utility
{
    class Command
    {
        public:
            Command ();// construct a command
            void parse (const std::string& cmdline);
            void parse (int argc, char** argv);
            bool valid () const;// check whether the command is valid
            const std::string& what () const;//return the error message when the command is invalid
            const std::string& usage () const;
            template <typename T>
            const T& get (const std::string& key) const;//get the required argument. Throw a invalid_argument exception when there is no such key.
            template <typename T>
            void add (const std::string& key, char abbr, const std::string& description = "", bool optional = false, const T& defaultValue = T (), std::function<bool(const T&)> validator = [] (const T&) {return true;});
            template <typename T>
            void add (const std::string& key, const std::string& description = "", bool optional = false, const T& defaultValue = T (), const std::function<bool(const T&)>& validator = [] (const T&) {return true;});//add without abbr
            bool exist (const std::string& key) const;//check if a key is provided
            const std::string& head () const;//return the first argument
            const std::vector<std::string>& nake () const;//return all arguments that can't be bound to a key
        private:
            struct ArgumentDetailBase
            {
                std::string key;
                char abbr;
                std::string description;
                virtual void set (const std::string& value) = 0;
                virtual std::string getType () const = 0;
                bool optional;
                ArgumentDetailBase (const std::string& k, char a, const std::string& d, bool o): key(k), abbr (a), description (d), optional (o) {}
            };
            template <typename T>
            struct ArgumentDetail: public ArgumentDetailBase
            {
                T value;
                std::function<bool(const T&)> validator;
                void set (const std::string& value)
                {
                    this->value = read<T> (value);
                }
                std::string getType () const
                {
                    return typeid (value).name ();
                }
                ArgumentDetail (const std::string& k, char a, const std::string& d, bool o, const T& v, const std::function<bool(const T&)>& va): ArgumentDetailBase (k, a, d, o), value (v), validator (va) {}
            };
            typedef std::unordered_map<std::string, std::unique_ptr<ArgumentDetailBase> > Key2Detail;
            typedef std::unordered_map<char, ArgumentDetailBase*> Abbr2Detail;
            Key2Detail _map;
            Abbr2Detail _abbrMap;
            std::vector<std::string> _errors;
            std::vector<std::string> _nakeWords;
            std::vector<ArgumentDetailBase*> _essentials, _optionals;


            void parseWords (const std::vector<std::string>& words);
            static bool isWhite (char x)
            {
                return x == ' ' || x == '\t' || x == '\n' || x == '\r';
            }
            static bool isQuote (char x)
            {
                return x == '\"';
            }
            template <typename T>
            static const T& read (const std::string& value);
    };
};
Utility::Command::Command ()
{
    //nothing
}
void Utility::Command::parse (const std::string& cmdline)
{
    size_t p = 0, q = p;
    std::vector<std::string> words;
    std::string cur;
    cur.reserve (cmdline.size());
    bool inQuote = false;
    char c;
    while (p < cmdline.size())
    {
        if (q >= cmdline.size() || isWhite (c = cmdline.at (q)))
        {
            if (q > p)
                words.push_back (cmdline.substr (p, q - p));
            p = ++q;
        }
        else if (isQuote (c))
        {
            words.push_back (cmdline.substr (p, q - p));
            inQuote = !inQuote;
            p = ++q;
        }
        else
        {
            ++q;
        }
    }
}
void Utility::Command::parse (int argc, char** argv)
{
    std::vector<std::string> words;
    for (int i = 0; i < argc; ++i)
    {
        words.emplace_back (argv[i]);
    }
    parseWords (std::move (words));
}
void Utility::Command::parseWords (const std::vector<std::string>& words)
{
    ArgumentDetailBase* curDetail = nullptr;
    for (const auto& word: words)
    {
        if (curDetail == nullptr)
        {
            if (word.size() > 2 && word[0] == '-' && word[1] == '-')
            {
                auto it = _map.find (word.substr (2, word.size() - 2));
                if (it != _map.end ())
                {
                    curDetail = it->second.get ();
                }
            }
            else if (word.size() == 2 && word[0] == '-')
            {
                auto it = _abbrMap.find (word[1]);
                if (it != _abbrMap.end ())
                    curDetail = it->second;
            }
            if (curDetail == nullptr)
                _nakeWords.push_back (std::move (word));
        }
        else
        {
            curDetail->set (word);
            curDetail = nullptr;
        }
    }
}
inline bool Utility::Command::valid () const
{
    return _errors.size() == 0;
}
const std::string& Utility::Command::what () const
{
    if (_errors.size() == 0)
        return std::move(std::string());
    std::string ret = _errors.front ();
    ret.reserve (ret.size() * _errors.size());
    for (auto it = _errors.begin() + 1; it != _errors.end (); ++it)
    {
        ret = '\n';
        ret = *it;
    }
    return std::move (ret);
}
const std::string& Utility::Command::usage () const
{
    static const int factor = 100;
    std::string ret ("");
    // ret.reserve (factor * _map.size ());
    ret.clear();
    ret.append ("Usage:");
    for (const auto& dptr: _essentials)
    {
        ret.append (" --");
        ret.append (dptr->key);
        ret.append ("=");
        ret.append (dptr->getType ());
    }
    ret.append ("\nOptions:\n");
    for (const auto& pair: _map)
    {
        ArgumentDetailBase* dptr = pair.second.get ();
        ret.append ("\t--");
        ret.append (dptr->key);
        if (isWhite (dptr->abbr))
        {
            ret.append ("     ");
        }
        else
        {
            ret.append (" , -");
            ret.push_back (dptr->abbr);
        }
        ret.append (" , ");
        ret.append (dptr->getType ());
        ret.append (" , ");
        ret.append (dptr->description);
        ret.append ("\n");
    }
    // std::stringstream msg;
    // msg << "usage:";
    // for (const auto& dptr: _essentials)
    // {
    //     msg << " --" << dptr->key << "=";// << dptr->getType ();
    // }
    // // msg << "\n";
    // msg << "options:";// << std::endl;
    // for (const auto& pair: _map)
    // {
    //     msg << "\t--" << pair.second->key << " , -" << std::setw (1) << pair.second->abbr << " , " << pair.second->description;// << std::endl;
    // }
    // return std::move (msg.str ());
    return std::move (ret);
}
bool Utility::Command::exist (const std::string& key) const
{
    return _map.find (key) != _map.end ();
}
const std::string& Utility::Command::head () const
{
    try
    {
        return _nakeWords.front ();
    }
    catch (std::exception& e)
    {
        throw std::range_error ("There is no head in this command.");
    }
}
const std::vector<std::string>& Utility::Command::nake () const
{
    return _nakeWords;
}
#endif
template <typename T>
const T& Utility::Command::get (const std::string& key) const
{
    auto it = _map.find (key);
    if (it == _map.end ())
    {
        throw std::invalid_argument ("No such key found: " + key);
    }
    ArgumentDetail<T>* detail = dynamic_cast<ArgumentDetail<T>*> (it->second.get ());
    if (detail == nullptr)
    {
        throw std::bad_cast ();
    }
    return detail->value;
}
template <typename T>
void Utility::Command::add (const std::string& key, char abbr, const std::string& description, bool optional, const T& defaultValue, std::function<bool(const T&)> validator)
{
    if (_map.find (key) != _map.end ())
        throw std::invalid_argument ("replicated key is provided:" + key);
    ArgumentDetailBase* detail = new ArgumentDetail<T>(key, abbr, std::move(description), optional, std::move(defaultValue), std::move (validator));
    _map[key] = std::move(std::unique_ptr<ArgumentDetailBase> (detail));
    if (!isWhite (abbr))
    {
        if (_abbrMap.find (abbr) != _abbrMap.end ())
            throw std::invalid_argument ("replicated abbr is provided:" + abbr);
        _abbrMap[abbr] = detail;
    }
    if (optional)
    {
        _optionals.push_back (detail);
    }
    else
        _essentials.push_back (detail);
}
template <typename T>
inline void Utility::Command::add (const std::string& key, const std::string& description , bool optional, const T& defaultValue, const std::function<bool(const T&)>& validator)
{
    add (key, 0, description, optional, defaultValue, validator);
}

namespace Utility
{
    template <>
    const std::string& Command::read (const std::string& value)
    {
        return value;
    }
    template <typename T>
    const T& Command::read (const std::string& value)
    {
        T t;
        std::stringstream in (value);
        in >> t;
        return std::move(t);
    }
}