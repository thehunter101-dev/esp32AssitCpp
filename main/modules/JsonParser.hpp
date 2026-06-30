#ifndef JSONPARSER_H_
#define JSONPARSER_H_

#include "cJSON.h"
#include <string>
#include "esp_log.h"

class JsonParser {
public:
    JsonParser();
    JsonParser(const char* rawJson);
    JsonParser(const char* rawJson, size_t len);
    ~JsonParser();

    JsonParser(const JsonParser&) = delete;
    JsonParser& operator=(const JsonParser&) = delete;

    bool isValid() const;

    void addString(const std::string& key, const std::string& value);
    void addInt(const std::string& key, int value);
    void addDouble(const std::string& key, double value);
    void addBool(const std::string& key, bool value);

    std::string toString() const;

    std::string getString(const std::string& key) const;
    int getInt(const std::string& key) const;
    double getDouble(const std::string& key) const;
    bool getBool(const std::string& key) const;
    std::string getFirstKey() const;
    std::string getFirstValue() const;

private:
    cJSON* _root;
    bool _owner;
    static constexpr const char* TAG = "JSON_PARSER";
};

#endif // JSONPARSER_H_
