#include "modules/WifiDriver.hpp"
#include "esp_netif.h"

WifiDriver::WifiDriver(std::string ssID, std::string passWord, int maxRetry) {
    _ssID = ssID;
    _passWord = passWord;
    _maxRetry = maxRetry;
    s_retry_num = 0;
    _wifi_event_group = nullptr;
    _payload = nullptr;
    _payload_len = 0;
    _payload_cap = 0;
    _connected = false;
    _everConnected = false;
}

WifiDriver::~WifiDriver() {
    clearPayload();
}

void WifiDriver::init() {
    _wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &WifiDriver::_eventHandler,
                                                        this,
                                                        &instance_any_id));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &WifiDriver::_eventHandler,
                                                        this,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {};
    std::strncpy((char*)wifi_config.sta.ssid, _ssID.c_str(), sizeof(wifi_config.sta.ssid));
    std::strncpy((char*)wifi_config.sta.password, _passWord.c_str(), sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));

    ESP_LOGI(TAG, "Driver Wi-Fi inicializado correctamente.");
}

bool WifiDriver::conect() {
    if (_wifi_event_group == nullptr) {
        ESP_LOGE(TAG, "Error: Debes llamar a init() antes de conect()");
        return false;
    }

    ESP_LOGI(TAG, "Iniciando proceso de conexión a %s...", _ssID.c_str());

    EventBits_t bits = xEventGroupWaitBits(_wifi_event_group,
            WIFI_CONNECTED_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(3000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "¡Conectado exitosamente!");
        _connected = true;
        _everConnected = true;
        return true;
    }

    ESP_LOGW(TAG, "WiFi no disponible aún — el sistema continuara sin red");
    return false;
}

bool WifiDriver::isConnected() {
    return _connected;
}

void WifiDriver::disconect() {
    ESP_LOGI(TAG, "Desconectando del Wi-Fi...");
    esp_wifi_disconnect();
    esp_wifi_stop();
}

void WifiDriver::_eventHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    WifiDriver* driver = static_cast<WifiDriver*>(arg);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "Buscando Punto de Acceso...");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        driver->_connected = false;
        driver->s_retry_num++;
        esp_wifi_connect();
        ESP_LOGW(TAG, "WiFi desconectado, reconectando... intento %d", driver->s_retry_num);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP Asignada por DHCP: " IPSTR, IP2STR(&event->ip_info.ip));

        esp_netif_dns_info_t dns;
        dns.ip.type = ESP_IPADDR_TYPE_V4;
        dns.ip.u_addr.ip4.addr = esp_ip4addr_aton("8.8.8.8");
        esp_netif_set_dns_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), ESP_NETIF_DNS_MAIN, &dns);
        dns.ip.u_addr.ip4.addr = esp_ip4addr_aton("8.8.4.4");
        esp_netif_set_dns_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), ESP_NETIF_DNS_BACKUP, &dns);
        ESP_LOGI(TAG, "DNS configurado: 8.8.8.8 / 8.8.4.4");

        driver->s_retry_num = 0;
        driver->_connected = true;
        driver->_everConnected = true;
        xEventGroupSetBits(driver->_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void WifiDriver::scanNetworks() {

    ESP_LOGI(TAG, "Iniciando escaneo de redes Wi-Fi...");

    wifi_scan_config_t scan_config = {};

    scan_config.ssid = nullptr;

    scan_config.bssid = nullptr;

    scan_config.channel = 0;

    scan_config.show_hidden = true;

    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

    esp_err_t ret = esp_wifi_scan_start(&scan_config, true);

    if (ret != ESP_OK) {

        ESP_LOGE(TAG, "Error al iniciar el escaneo de Wi-Fi: %s", esp_err_to_name(ret));

        return;

    }

    uint16_t number = 20;

    wifi_ap_record_t ap_info[20];

    memset(ap_info, 0, sizeof(ap_info));

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

    ESP_LOGI(TAG, "Escaneo finalizado. Se encontraron %d redes de 2.4 GHz:", number);

    ESP_LOGI(TAG, "-------------------------------------------------------------");

    ESP_LOGI(TAG, "%-32s | %-7s | %-4s", "SSID", "RSSI", "CHAN");

    ESP_LOGI(TAG, "-------------------------------------------------------------");

    for (int i = 0; i < number; i++) {

        ESP_LOGI(TAG, "%-32s | %-7d | %-4d",

                 (char*)ap_info[i].ssid,

                 ap_info[i].rssi,

                 ap_info[i].primary);

    }

    ESP_LOGI(TAG, "-------------------------------------------------------------");

}



static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA: {
            if (evt->user_data) {
                WifiDriver* driver = static_cast<WifiDriver*>(evt->user_data);
                driver->appendPayload((char*)evt->data, evt->data_len);
            }
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        }
        default:
            break;
    }
    return ESP_OK;
}

void WifiDriver::makeGetRequest(const std::string& ipAddress, int port, const std::string& path) {
    clearPayload();

    std::string url = "http://" + ipAddress + ":" + std::to_string(port) + path;
    ESP_LOGI(TAG, "Enviando petición HTTP GET a: %s", url.c_str());

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.event_handler = _http_event_handler;
    config.user_data = this;
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        ESP_LOGE(TAG, "Error al inicializar el cliente HTTP");
        return;
    }

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "Petición finalizada con éxito. Código de estado HTTP: %d", status_code);
    } else {
        ESP_LOGE(TAG, "Fallo en la petición HTTP: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void WifiDriver::makePostRequest(const std::string& ipAddress, int port, const std::string& path, const char* post_data, size_t post_len, const std::string& contentType) {
    clearPayload();

    std::string url = "http://" + ipAddress + ":" + std::to_string(port) + path;
    ESP_LOGI(TAG, "Enviando petición HTTP POST a: %s", url.c_str());

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.event_handler = _http_event_handler;
    config.user_data = this;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == nullptr) {
        ESP_LOGE(TAG, "Error al inicializar el cliente HTTP");
        return;
    }

    esp_http_client_set_header(client, "Content-Type", contentType.c_str());
    esp_http_client_set_post_field(client, post_data, post_len);

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "POST finalizado con éxito. Código de estado HTTP: %d", status_code);
    } else {
        ESP_LOGE(TAG, "Fallo en la petición POST: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void WifiDriver::setPayload(const char* data, size_t len) {
    clearPayload();
    if (len == 0) return;
    _payload = (char*)malloc(len + 1);
    if (_payload == nullptr) {
        ESP_LOGE(TAG, "Error al asignar memoria en setPayload");
        _payload_len = 0;
        _payload_cap = 0;
        return;
    }
    memcpy(_payload, data, len);
    _payload[len] = '\0';
    _payload_len = len;
    _payload_cap = len + 1;
}

const char* WifiDriver::getPayloadBytes() const {
    return _payload;
}

size_t WifiDriver::getPayloadLength() const {
    return _payload_len;
}

void WifiDriver::clearPayload() {
    if (_payload != nullptr) {
        free(_payload);
        _payload = nullptr;
    }
    _payload_len = 0;
    _payload_cap = 0;
}

void WifiDriver::appendPayload(const char* data, int len) {
    if (data == nullptr || len <= 0) return;

    size_t new_len = _payload_len + len;
    size_t needed = new_len + 1;

    if (needed > _payload_cap) {
        size_t new_cap = _payload_cap == 0 ? 128 : _payload_cap;
        while (new_cap < needed) {
            new_cap *= 2;
        }
        char* new_buf = (char*)realloc(_payload, new_cap);
        if (new_buf == nullptr) {
            ESP_LOGE(TAG, "Error al realocar memoria en appendPayload");
            return;
        }
        _payload = new_buf;
        _payload_cap = new_cap;
    }

    memcpy(_payload + _payload_len, data, len);
    _payload_len = new_len;
    _payload[_payload_len] = '\0';
}
