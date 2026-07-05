#ifndef ACCESSDB_HPP_
#define ACCESSDB_HPP_

#include <cstdint>
#include <string>
#include <cstring>
#include "esp_http_client.h"
#include "esp_log.h"

class AccessDB {
public:
    AccessDB(const char* supabaseUrl, const char* anonKey);
    ~AccessDB();

    bool registerCard(const char* uidHex, uint16_t fingerID, const char* name = "");
    bool checkCard(const char* uidHex, uint16_t& fingerID, std::string& name, bool& active);
    bool logAttempt(const char* uidHex, bool success, const char* reason, uint16_t fingerID);

private:
    std::string _url;
    std::string _key;

    static constexpr const char* TAG = "ACCESS_DB";

    bool httpPost(const char* endpoint, const char* jsonBody);
    bool httpPatch(const char* endpoint, const char* jsonBody);
    bool httpGet(const char* endpoint, std::string& response);
    int jsonFindInt(const std::string& json, const char* key, int defaultValue);
    std::string jsonFindString(const std::string& json, const char* key, const std::string& defaultValue);
};

#endif
