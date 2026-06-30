#include "JsonParser.hpp"

JsonParser::JsonParser() {
    _root = cJSON_CreateObject();
    _owner = true;
    if (!_root) {
        ESP_LOGE(TAG, "Error al crear objeto JSON vacío");
    }
}

JsonParser::JsonParser(const char* rawJson) {
    _owner = true;
    if (rawJson == nullptr) {
        _root = nullptr;
        ESP_LOGE(TAG, "Error: rawJson es nullptr");
        return;
    }
    _root = cJSON_Parse(rawJson);
    if (!_root) {
        ESP_LOGE(TAG, "Error al parsear el JSON");
    }
}

JsonParser::JsonParser(const char* rawJson, size_t len) {
    _owner = true;
    if (rawJson == nullptr || len == 0) {
        _root = nullptr;
        ESP_LOGE(TAG, "Error: rawJson inválido");
        return;
    }
    _root = cJSON_ParseWithLength(rawJson, len);
    if (!_root) {
        ESP_LOGE(TAG, "Error al parsear el JSON con longitud");
    }
}

JsonParser::~JsonParser() {
    if (_owner && _root != nullptr) {
        cJSON_Delete(_root);
        _root = nullptr;
    }
}

bool JsonParser::isValid() const {
    return _root != nullptr;
}

void JsonParser::addString(const std::string& key, const std::string& value) {
    if (!_root) return;
    cJSON* item = cJSON_CreateString(value.c_str());
    if (item) {
        cJSON_AddItemToObject(_root, key.c_str(), item);
    }
}

void JsonParser::addInt(const std::string& key, int value) {
    if (!_root) return;
    cJSON* item = cJSON_CreateNumber(value);
    if (item) {
        cJSON_AddItemToObject(_root, key.c_str(), item);
    }
}

void JsonParser::addDouble(const std::string& key, double value) {
    if (!_root) return;
    cJSON* item = cJSON_CreateNumber(value);
    if (item) {
        cJSON_AddItemToObject(_root, key.c_str(), item);
    }
}

void JsonParser::addBool(const std::string& key, bool value) {
    if (!_root) return;
    cJSON* item = cJSON_CreateBool(value);
    if (item) {
        cJSON_AddItemToObject(_root, key.c_str(), item);
    }
}

std::string JsonParser::toString() const {
    if (!_root) return "";
    char* raw = cJSON_PrintUnformatted(_root);
    if (!raw) return "";
    std::string result(raw);
    cJSON_free(raw);
    return result;
}

std::string JsonParser::getString(const std::string& key) const {
    if (!_root) return "";
    cJSON* item = cJSON_GetObjectItem(_root, key.c_str());
    if (item != nullptr && cJSON_IsString(item) && item->valuestring != nullptr) {
        return item->valuestring;
    }
    return "";
}

int JsonParser::getInt(const std::string& key) const {
    if (!_root) return 0;
    cJSON* item = cJSON_GetObjectItem(_root, key.c_str());
    if (item != nullptr && cJSON_IsNumber(item)) {
        return item->valueint;
    }
    return 0;
}

double JsonParser::getDouble(const std::string& key) const {
    if (!_root) return 0.0;
    cJSON* item = cJSON_GetObjectItem(_root, key.c_str());
    if (item != nullptr && cJSON_IsNumber(item)) {
        return item->valuedouble;
    }
    return 0.0;
}

bool JsonParser::getBool(const std::string& key) const {
    if (!_root) return false;
    cJSON* item = cJSON_GetObjectItem(_root, key.c_str());
    if (item != nullptr && cJSON_IsBool(item)) {
        return cJSON_IsTrue(item);
    }
    return false;
}

std::string JsonParser::getFirstKey() const {
    if (!_root || !_root->child) return "";
    cJSON* child = _root->child;
    return child->string ? child->string : "";
}

std::string JsonParser::getFirstValue() const {
    if (!_root || !_root->child) return "";
    cJSON* child = _root->child;
    if (cJSON_IsString(child) && child->valuestring != nullptr) {
        return child->valuestring;
    }
    if (cJSON_IsNumber(child)) {
        return std::to_string(child->valuedouble);
    }
    if (cJSON_IsBool(child)) {
        return cJSON_IsTrue(child) ? "true" : "false";
    }
    return "";
}
