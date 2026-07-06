#include "AccessDB.hpp"
#include <cstdio>

static const char* CA_CERT_PEM =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDejCCAmKgAwIBAgIQf+UwvzMTQ77dghYQST2KGzANBgkqhkiG9w0BAQsFADBX\n"
    "MQswCQYDVQQGEwJCRTEZMBcGA1UEChMQR2xvYmFsU2lnbiBudi1zYTEQMA4GA1UE\n"
    "CxMHUm9vdCBDQTEbMBkGA1UEAxMSR2xvYmFsU2lnbiBSb290IENBMB4XDTIzMTEx\n"
    "NTAzNDMyMVoXDTI4MDEyODAwMDA0MlowRzELMAkGA1UEBhMCVVMxIjAgBgNVBAoT\n"
    "GUdvb2dsZSBUcnVzdCBTZXJ2aWNlcyBMTEMxFDASBgNVBAMTC0dUUyBSb290IFI0\n"
    "MHYwEAYHKoZIzj0CAQYFK4EEACIDYgAE83Rzp2iLYK5DuDXFgTB7S0md+8Fhzube\n"
    "Rr1r1WEYNa5A3XP3iZEwWus87oV8okB2O6nGuEfYKueSkWpz6bFyOZ8pn6KY019e\n"
    "WIZlD6GEZQbR3IvJx3PIjGov5cSr0R2Ko4H/MIH8MA4GA1UdDwEB/wQEAwIBhjAd\n"
    "BgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwDwYDVR0TAQH/BAUwAwEB/zAd\n"
    "BgNVHQ4EFgQUgEzW63T/STaj1dj8tT7FavCUHYwwHwYDVR0jBBgwFoAUYHtmGkUN\n"
    "l8qJUC99BM00qP/8/UswNgYIKwYBBQUHAQEEKjAoMCYGCCsGAQUFBzAChhpodHRw\n"
    "Oi8vaS5wa2kuZ29vZy9nc3IxLmNydDAtBgNVHR8EJjAkMCKgIKAehhxodHRwOi8v\n"
    "Yy5wa2kuZ29vZy9yL2dzcjEuY3JsMBMGA1UdIAQMMAowCAYGZ4EMAQIBMA0GCSqG\n"
    "SIb3DQEBCwUAA4IBAQAYQrsPBtYDh5bjP2OBDwmkoWhIDDkic574y04tfzHpn+cJ\n"
    "odI2D4SseesQ6bDrarZ7C30ddLibZatoKiws3UL9xnELz4ct92vID24FfVbiI1hY\n"
    "+SW6FoVHkNeWIP0GCbaM4C6uVdF5dTUsMVs/ZbzNnIdCp5Gxmx5ejvEau8otR/Cs\n"
    "kGN+hr/W5GvT1tMBjgWKZ1i4//emhA1JG1BbPzoLJQvyEotc03lXjTaCzv8mEbep\n"
    "8RqZ7a2CPsgRbuvTPBwcOMBBmuFeU88+FSBX6+7iP0il8b4Z0QFqIwwMHfs/L6K1\n"
    "vepuoxtGzi4CZ68zJpiq1UvSqTbFJjtbD4seiMHl\n"
    "-----END CERTIFICATE-----\n";

AccessDB::AccessDB(const char* supabaseUrl, const char* anonKey)
    : _url(supabaseUrl), _key(anonKey)
{
}

AccessDB::~AccessDB()
{
}

bool AccessDB::httpPost(const char* endpoint, const char* jsonBody)
{
    std::string fullUrl = _url + endpoint;

    esp_http_client_config_t config = {};
    config.url = fullUrl.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 10000;
    config.cert_pem = CA_CERT_PEM;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "apikey", _key.c_str());
    esp_http_client_set_header(client, "Authorization", ("Bearer " + _key).c_str());
    esp_http_client_set_header(client, "Prefer", "return=minimal");
    esp_http_client_set_post_field(client, jsonBody, strlen(jsonBody));

    esp_err_t err = esp_http_client_perform(client);
    bool ok = (err == ESP_OK);
    if (ok) {
        int status = esp_http_client_get_status_code(client);
        ok = (status == 200 || status == 201 || status == 204);
        ESP_LOGI(TAG, "POST %s -> %d %s", endpoint, status, jsonBody);
    } else {
        ESP_LOGE(TAG, "POST %s fallo: %s", endpoint, esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return ok;
}

bool AccessDB::httpPatch(const char* endpoint, const char* jsonBody)
{
    std::string fullUrl = _url + endpoint;

    esp_http_client_config_t config = {};
    config.url = fullUrl.c_str();
    config.method = HTTP_METHOD_PATCH;
    config.timeout_ms = 10000;
    config.cert_pem = CA_CERT_PEM;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "apikey", _key.c_str());
    esp_http_client_set_header(client, "Authorization", ("Bearer " + _key).c_str());
    esp_http_client_set_header(client, "Prefer", "return=minimal");
    esp_http_client_set_post_field(client, jsonBody, strlen(jsonBody));

    esp_err_t err = esp_http_client_perform(client);
    bool ok = (err == ESP_OK);
    if (ok) {
        int status = esp_http_client_get_status_code(client);
        ok = (status == 200 || status == 204);
        ESP_LOGI(TAG, "PATCH %s -> %d %s", endpoint, status, jsonBody);
    } else {
        ESP_LOGE(TAG, "PATCH %s fallo: %s", endpoint, esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return ok;
}

bool AccessDB::httpGet(const char* endpoint, std::string& response)
{
    std::string fullUrl = _url + endpoint;

    esp_http_client_config_t config = {};
    config.url = fullUrl.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 10000;
    config.cert_pem = CA_CERT_PEM;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return false;

    esp_http_client_set_header(client, "apikey", _key.c_str());
    esp_http_client_set_header(client, "Authorization", ("Bearer " + _key).c_str());

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GET %s open fallo: %s", endpoint, esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int64_t len = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    if (status != 200) {
        ESP_LOGW(TAG, "GET %s -> HTTP %d", endpoint, status);
        esp_http_client_cleanup(client);
        return false;
    }

    std::string raw;
    if (len > 0) {
        raw.resize(len);
        int readLen = esp_http_client_read(client, &raw[0], len);
        if (readLen > 0) raw.resize(readLen);
        else raw.clear();
    } else {
        char tmp[128];
        int readLen;
        while ((readLen = esp_http_client_read(client, tmp, sizeof(tmp))) > 0) {
            raw.append(tmp, readLen);
        }
    }

    esp_http_client_cleanup(client);
    response = raw;
    ESP_LOGI(TAG, "GET %s -> %d, len=%zu", endpoint, status, raw.size());
    return true;
}

int AccessDB::jsonFindInt(const std::string& json, const char* key, int defaultValue)
{
    std::string search = "\"";
    search += key;
    search += "\":";

    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultValue;

    pos += search.size();
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) {
        pos++;
    }

    if (pos >= json.size() || (json[pos] < '0' || json[pos] > '9')) {
        return defaultValue;
    }

    int val = 0;
    bool neg = false;
    if (json[pos] == '-') { neg = true; pos++; }
    while (pos < json.size() && json[pos] >= '0' && json[pos] <= '9') {
        val = val * 10 + (json[pos] - '0');
        pos++;
    }

    return neg ? -val : val;
}

std::string AccessDB::jsonFindString(const std::string& json, const char* key, const std::string& defaultValue)
{
    std::string search = "\"";
    search += key;
    search += "\":\"";

    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultValue;

    pos += search.size();
    std::string val;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.size()) {
            char next = json[pos + 1];
            switch (next) {
                case 'n': val += '\n'; break;
                case 'r': val += '\r'; break;
                case 't': val += '\t'; break;
                case '"': val += '"'; break;
                case '\\': val += '\\'; break;
                default: val += next; break;
            }
            pos += 2;
        } else {
            val += json[pos];
            pos++;
        }
    }
    return val;
}

bool AccessDB::registerCard(const char* uidHex, uint16_t fingerID, const char* name)
{
    char body[192];

    if (name && name[0]) {
        char escapedName[64] = {};
        size_t ni = 0;
        for (const char* p = name; *p && ni < sizeof(escapedName) - 1; p++) {
            if (*p == '"' || *p == '\\') continue;
            escapedName[ni++] = *p;
        }
        escapedName[ni] = 0;

        snprintf(body, sizeof(body),
            "{\"uid_hex\":\"%s\",\"finger_id\":%u,\"name\":\"%s\"}",
            uidHex, fingerID, escapedName);
    } else {
        snprintf(body, sizeof(body),
            "{\"uid_hex\":\"%s\",\"finger_id\":%u}",
            uidHex, fingerID);
    }

    if (httpPost("/rest/v1/authorized_cards", body)) {
        return true;
    }

    // 409 = already exists → PATCH to update finger_id
    snprintf(body, sizeof(body), "{\"finger_id\":%u}", fingerID);
    std::string patchEndpoint = "/rest/v1/authorized_cards?uid_hex=eq.";
    patchEndpoint += uidHex;
    return httpPatch(patchEndpoint.c_str(), body);
}

bool AccessDB::checkCard(const char* uidHex, uint16_t& fingerID, std::string& name, bool& active)
{
    fingerID = 0;
    name.clear();
    active = true;

    std::string endpoint = "/rest/v1/authorized_cards?uid_hex=eq.";
    endpoint += uidHex;
    endpoint += "&select=*";

    std::string response;
    if (!httpGet(endpoint.c_str(), response)) {
        return false;
    }

    if (response.empty() || response == "[]") {
        return false;
    }

    fingerID = (uint16_t)jsonFindInt(response, "finger_id", 0);
    name = jsonFindString(response, "name", "");
    active = response.find("\"active\":true") != std::string::npos;
    return true;
}

bool AccessDB::logAttempt(const char* uidHex, bool success, const char* reason, uint16_t fingerID)
{
    char body[256];
    snprintf(body, sizeof(body),
        "{\"uid_hex\":\"%s\",\"success\":%s,\"reason\":\"%s\",\"finger_id\":%u}",
        uidHex, success ? "true" : "false", reason, fingerID);

    return httpPost("/rest/v1/access_logs", body);
}

bool AccessDB::deactivateCard(const char* uidHex)
{
    char body[32];
    snprintf(body, sizeof(body), "{\"active\":false}");

    char endpoint[128];
    snprintf(endpoint, sizeof(endpoint), "/rest/v1/authorized_cards?uid_hex=eq.%s", uidHex);

    bool ok = httpPatch(endpoint, body);
    if (ok) {
        ESP_LOGI(TAG, "Tarjeta %s desactivada por 3 fallos", uidHex);
    } else {
        ESP_LOGE(TAG, "Fallo al desactivar tarjeta %s", uidHex);
    }
    return ok;
}

bool AccessDB::checkPendingCommands(std::string& command, int64_t& cmdId)
{
    std::string response;
    if (!httpGet("/rest/v1/pending_commands?executed=eq.false&limit=1", response)) {
        return false;
    }

    if (response.empty() || response == "[]") {
        return false;
    }

    cmdId = jsonFindInt(response, "id", 0);
    command = jsonFindString(response, "command", "");
    return !command.empty();
}

bool AccessDB::markCommandExecuted(int64_t cmdId)
{
    char body[64];
    snprintf(body, sizeof(body), "{\"executed\":true}");

    char endpoint[128];
    snprintf(endpoint, sizeof(endpoint), "/rest/v1/pending_commands?id=eq.%lld", (long long)cmdId);

    return httpPatch(endpoint, body);
}
