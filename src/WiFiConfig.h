#if (!defined(_WEB_CONFIG_H_))
#define _WEB_CONFIG_H_


#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#else
#error Board not supported
#endif


#include <vector>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>

#include "StorageNVM.h"


#define WC_MAX(a, b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })
#define WC_MIN(a, b) \
    ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#define WC_ABS(N) ((N < 0) ? (-N) : (N))


const char wificonfig_html[] PROGMEM = R"rawstring(<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta http-equiv="X-UA-Compatible" content="IE=edge"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Document</title><style>*{font-family:Helvetica,Arial,sans-serif}:root{--color-dark:#425F57;--color-main:#749F82;--color-accent:#c2e7c2}.grid-container{display:inline-grid;grid-template-columns:auto;margin-left:auto;margin-right:auto;border:2px solid var(--color-dark);border-radius:3px}.scan-row{display:inline-grid;background-color:var(--color-accent);grid-template-columns:auto auto auto;transition:.15s}.grid-single{padding:10px 0;justify-content:center}.scan-non-header:hover{filter:brightness(.94)}.scan-non-header:active{filter:brightness(.87)}.scan-item{padding:10px}.scan-header{font-size:large;font-weight:700;padding:10px 10px;border-bottom:3px solid var(--color-dark)}.reload{font-family:Lucida Sans Unicode;margin-left:.7em;padding:0 .35em;transition:.15s}.title{margin-left:auto;margin-right:auto;width:9rem}.params-wifi{display:inline-grid;background-color:var(--color-accent);grid-template-columns:auto auto auto;border-radius:4px}.link-btn{text-decoration:none;padding:.5rem .5rem;background-color:var(--color-accent);border-radius:4px;transition:.15s;border-width:0;font-weight:700}.link-btn:hover{filter:brightness(.94)}.link-btn:active{filter:brightness(.87)}.main{width:fit-content;background-color:var(--color-main);padding:.5rem 1rem;padding-bottom:1rem;border-radius:5px;margin:1rem;margin-left:auto;margin-right:auto}.grid-header{font-size:large;font-weight:700;padding:10px 10px;border-bottom:3px solid var(--color-dark)}.grid-elem{padding:10px}.link-btn{text-decoration:none;padding:.5rem .5rem;background-color:var(--color-accent);border-radius:4px;transition:.15s}.link-btn:hover{filter:brightness(.94)}.link-btn:active{filter:brightness(.87)}body{background-color:var(--color-dark)}</style></head><body><div class="main"><h2 class="title">WiFi Config</h2><div id="scan-div" class="grid-container"><div class="scan-row grid-single"><div class="scan-header">ssid</div><div class="scan-header">pasword</div><div class="scan-header">strength<button id="reload" class="reload" onclick="refreshScan()">&#x21bb;</button></div></div><div class="scan-row grid-single">loading</div></div><br><br><div id="nets" class="params-wifi"><div class="grid-header">Number</div><div class="grid-header">SSID</div><div class="grid-header">Password</div></div><br><br><button class="link-btn" id="submit">Set</button></div><script>const scanURL = '/wifiscan';
        const postURL = '/json';
        const infoURL = '/wifiinfo';

        Number.prototype.map = function (in_min, in_max, out_min, out_max) {
            return (this - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        }
        Number.prototype.clamp = function (min, max) {
            return Math.min(Math.max(this, min), max);
        }

        function header(btnState = true) {
            return `<div class="scan-row"><div class="scan-header">ssid</div><div class="scan-header">pasword</div><div class="scan-header">strength<button id="reload" class="reload" onclick="refreshScan()"${btnState ? '' : ' disabled'}>&#x21bb;</button></div></div>`;
        }
        let scanComplete = false;

        function rssiPer(rssi) {
            let rssiMin = -100;
            let rssiMax = -50;

            return `${rssi.clamp(rssiMin, rssiMax).map(rssiMin, rssiMax, 0, 100)} %`;
        }

        function emptyScanTable() {
            document.getElementById('scan-div').innerHTML = header() + '<div class="scan-row grid-single"><i>loading...</i></div>';
        }

        function fillScanTable(data) {
            const btnState = !document.getElementById("reload").disabled;
            let div = document.getElementById('scan-div');
            div.innerHTML = header(btnState);
            for (let i = 0; i < data.length; i++) {
                let e = data[i];
                div.innerHTML += `<div class="scan-row scan-non-header" onclick="buttonClick(event)"><div class="scan-item" scan-ssid>${e.ssid}</div><div class="scan-item"><input type="checkbox" onclick="event.preventDefault()"${(e.secure == 7 ? '' : ' checked')}></div><div class="scan-item">${rssiPer(e.rssi)}</div></div>`;
            }
        }

        function httpGetAsync(theUrl, cb) {
            var xmlHttp = new XMLHttpRequest();
            xmlHttp.onreadystatechange = function () {
                if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
                    cb(xmlHttp.responseText);
            }
            xmlHttp.open("GET", theUrl, true);
            xmlHttp.send(null);
        }

        function httpPostAsync(theUrl, data) {
            var xmlHttp = new XMLHttpRequest();
            xmlHttp.open('POST', theUrl);
            xmlHttp.setRequestHeader("Content-Type", "application/json");
            xmlHttp.send(/*'json_name='+*/JSON.stringify(data));
        }

        function refreshScan() {
            httpGetAsync(scanURL, (data) => {
                scanComplete = true;
                if (!data.length) {
                    emptyScanTable();
                    console.log(data);
                } else {
                    let json = JSON.parse(data);
                    json.sort((a, b) => (a.rssi < b.rssi) ? 1 : -1);
                    fillScanTable(data);
                }
            });

            document.getElementById("reload").disabled = true;
            setTimeout(function () { document.getElementById("reload").disabled = false }, 3000);
        }

        function buttonClick(evt) {
            const elem = evt.currentTarget;
            const ssid = [...elem.children].find((e) => e.hasAttribute('scan-ssid')).textContent;
            [...document.getElementById("nets").children].find((e) => e.hasAttribute('ssid')).firstChild.value = ssid;
        }

        document.getElementById('submit').addEventListener('click', function () {
            const ssids = [...document.querySelectorAll('[ssid]')];
            const pswds = [...document.querySelectorAll('[pswd]')];
            let data = [];
            for (let i = 0; i < ssids.length; i++) {
                data.push({
                    ssid: ssids[i].firstChild.value,
                    pswd: pswds[i].firstChild.value,
                });
            }

            console.log('send', data);
            httpPostAsync(postURL, data);
        });

        emptyScanTable();

        httpGetAsync(infoURL, (data) => {
            let net_elem = document.getElementById('nets');
            for (let i = 1; i < data.length+1; i++) {
                nets.innerHTML += `<div class="grid-elem"><label for="ssid${i}">net ${i}</label></div><div class="grid-elem" ssid><input type="text" name="ssid${i}"></div><div class="grid-elem" pswd><input type="text" name="pswd${i}"></div>`;
            }
        });

        refreshScan();</script></body></html>)rawstring";


// https://medium.com/@sddkal/my-take-on-c-argsort-function-bfb59845d30c
template <typename Iter, typename Compare>
std::vector<int> argsort(Iter begin, Iter end, Compare comp)
{
    // Begin Iterator, End Iterator, Comp
    std::vector<std::pair<int, Iter>> pairList; // Pair Vector
    std::vector<int> ret;                       // Will hold the indices

    int i = 0;
    for (auto it = begin; it < end; it++)
    {
        std::pair<int, Iter> pair(i, it); // 0: Element1, 1:Element2...
        pairList.push_back(pair);         // Add to list
        i++;
    }
    // Stable sort the pair vector
    std::stable_sort(pairList.begin(), pairList.end(),
                     [comp](std::pair<int, Iter> prev, std::pair<int, Iter> next) -> bool
                     { return comp(*prev.second, *next.second); } // This is the important part explained below
    );

    /*
        Comp is a templated function pointer that makes a basic comparison
        std::stable_sort takes a function pointer that takes
        (std::pair<int, Iter> prev, std::pair<int, Iter> next)
        and returns bool. We passed a corresponding lambda to stable sort
        and used our comp within brackets to capture it as an outer variable.
        We then applied this function to our iterators which are dereferenced.
    */
    for (auto i : pairList)
        ret.push_back(i.first); // Take indices

    return ret;
}


#ifndef WIFI_CONFIG_N_NETWORKS
    #define WIFI_CONFIG_N_NETWORKS 2
#endif

#define WIFI_CONFIG_SSID_SIZE 32
#define WIFI_CONFIG_PSWD_SIZE 32


struct WiFiNetwork
{
    char ssid[WIFI_CONFIG_SSID_SIZE]{0}, pswd[WIFI_CONFIG_PSWD_SIZE]{0};

    inline void setCreds(const char *newSSID, const char *newPSWD=nullptr) {
        strlcpy(ssid, newSSID, WIFI_CONFIG_SSID_SIZE);
        if (newPSWD)
            strlcpy(pswd, newPSWD, WIFI_CONFIG_PSWD_SIZE);
    }

    inline bool populated() const { return strlen(ssid) > 0; }
};

struct WiFiParams
{
    uint8_t mode = 0; // 0 means try STATION unless manual switch
    WiFiNetwork nets[WIFI_CONFIG_N_NETWORKS];
};


typedef void (*WCJsonCallback_T)(JsonObjectConst &json);
typedef void (*WCCallback_T)(AsyncWebServerRequest *request);

class WiFiConfig : public AsyncWebServer
{
public:
    WiFiParams nets;
    int nvmAddress;
    bool hasNVM;

public:
    WiFiConfig(uint16_t port = 80) : AsyncWebServer(port)
    {
        nvmAddress = StorageNVM.addToScheme<WiFiParams>();
        hasNVM = nvmAddress >= 0;

        if (hasNVM) {
            StorageNVM.get(nvmAddress, nets);
        }

        _lastConnectTime = millis();
    }

#define WIFI_CONFIG_URL_CONFIG "/wificonfig"
#define WIFI_CONFIG_URL_JSON "/json"
#define WIFI_CONFIG_URL_SCAN "/wifiscan"
#define WIFI_CONFIG_URL_INFO "/wifiinfo"
    void createRoutes(WCJsonCallback_T jsonCB=nullptr, WCCallback_T configCB=nullptr) {
        on(WIFI_CONFIG_URL_CONFIG, HTTP_GET, [configCB](AsyncWebServerRequest *request) {
            if (configCB)
                configCB(request);
            request->send_P(200, "text/html", wificonfig_html);
        });

        _handlerWiFiConfig = new AsyncCallbackJsonWebHandler(WIFI_CONFIG_URL_JSON, [this, jsonCB](AsyncWebServerRequest *request, JsonVariant &jsonVar) {
            JsonObjectConst json = jsonVar.as<JsonObjectConst>();
            if (json.isNull()) {
                request->send(400, "Bad format: is not object.");
                return;
            }

            // wifi creds
            if (json.containsKey("wifi")) {
                JsonArrayConst jsonArr = json["wifi"].as<JsonArrayConst>();
                if (!jsonArr.isNull()) {
                    size_t i = 0;
                    for (JsonVariantConst value : jsonArr) {
                        if (i >= WIFI_CONFIG_N_NETWORKS) break;
                        JsonObjectConst net = value.as<JsonObjectConst>();
                        if (net.isNull()) continue;
                        if (!(net.containsKey("ssid") && net.containsKey("pswd"))) continue;
                        const char *ssid = net["ssid"];
                        const char *pswd = net["pswd"];
                        this->nets.nets[i++].setCreds(ssid, pswd);
                        if (!this->_changedWiFiNets) {
                            this->_changedWiFiNets = true;
                            if (this->hasNVM)
                                StorageNVM.put(nvmAddress, nets);
                        }
                    }

                    if (!jsonCB) {
                        const size_t resLen = 32;
                        char res[resLen]{0};
                        snprintf(res, resLen, "OK. Received %u nets.", (i > 0 ? (i-1)%1000 : 0));
                        request->send(200, "text/plain", res);
                        return;
                    }
                }
            }
            if (jsonCB) {
                jsonCB(json);
            } else {
                request->send(200, "text/plain", "OK. Done nothing.");
            }

        });
        addHandler(_handlerWiFiConfig);

#define _WC_SCAN_COOLDOWN 5000
        on(WIFI_CONFIG_URL_SCAN, HTTP_GET, [this](AsyncWebServerRequest *request)
           {
            String json = "[";
            int n = WiFi.scanComplete();
            if (n == -2)
            {
                WiFi.scanNetworks(true);
            } else if (n)
            {

#ifdef WC_SCAN_DONT_ORDER_NETS_BY_RSSID
                for (int i = 0; i < n; ++i)
                {
                    if(i) json += ",";
                    json += "{";
                    json += "\"rssi\":"+String(WiFi.RSSI(i));
                    json += ",\"ssid\":\""+WiFi.SSID(i)+"\"";
                    json += ",\"bssid\":\""+WiFi.BSSIDstr(i)+"\"";
                    json += ",\"channel\":"+String(WiFi.channel(i));
                    json += ",\"secure\":"+String(WiFi.encryptionType(i));
                    json += "}";
                }
#else
                std::vector<int32_t> rssi;
                for (size_t i = 0; (int)i < n; i++) rssi.push_back(WiFi.RSSI(i));
                std::vector<int> indices = argsort(rssi.begin(), rssi.end(), std::greater<int32_t>());

                for (int i = 0; i < n; ++i)
                {
                    int idx = indices[i];
                    if(i) json += ",";
                    json += "{";
                    json += "\"rssi\":"+String(WiFi.RSSI(idx));
                    json += ",\"ssid\":\""+WiFi.SSID(idx)+"\"";
                    json += ",\"bssid\":\""+WiFi.BSSIDstr(idx)+"\"";
                    json += ",\"channel\":"+String(WiFi.channel(idx));
                    json += ",\"secure\":"+String(WiFi.encryptionType(idx));
                    json += "}";
                }
#endif

                if (WC_ABS(millis() - this->_lastScanTime) > _WC_SCAN_COOLDOWN) {
                    Serial.println("BEGIN SCAN");
                    WiFi.scanDelete();
                    if (WiFi.scanComplete() == -2)
                    {
                        WiFi.scanNetworks(true);
                    }
                    this->_lastScanTime = millis();
                }
            }
            json += "]";
            request->send(200, "application/json", json);
            json = String();
        });
        
        on(WIFI_CONFIG_URL_INFO, HTTP_GET, [this](AsyncWebServerRequest *request) {
            String json = "[";
            for (size_t i = 0; i < WIFI_CONFIG_N_NETWORKS; i++) {
                WiFiNetwork *net = &this->nets.nets[i];
                json += "{\"ssid\":";
                if (net->ssid)
                    json += net->ssid;
                json += "\",\"pswd\":\"";
                if (net->pswd)
                    json += net->pswd;
                json += "\"}";
                if (i < WIFI_CONFIG_N_NETWORKS - 1)
                    json += ",";
            }
            json += "]";
            request->send(200, "application/json", json);
            json = String();
        });
    }

#define WIFI_CONFIG_CONNECT_TIMEOUT 30000
    void loop() { // manage wifi mode here?
        if (WiFi.status() != WL_CONNECTED || _changedWiFiNets) {
            for (size_t i = 0; i < WIFI_CONFIG_N_NETWORKS; i++) {
                WiFiNetwork *net = &nets.nets[i];
                if (!net->populated()) continue;
                if (_changedWiFiNets)
                    WiFi.disconnect();
                _changedWiFiNets = false;
                WiFi.begin(net->ssid, net->pswd);
                _lastConnectTime = millis();
                Serial.printf("Connecting to net %d: '%s'... ", i, net->ssid);
                while (WiFi.status() != WL_CONNECTED && WC_ABS(millis() - _lastConnectTime) > WIFI_CONFIG_CONNECT_TIMEOUT) {
                    yield();
                    delay(10);
                }
                if (WiFi.status() != WL_CONNECTED) {
                    Serial.println("done");
                    Serial.printf("IP: %s", WiFi.localIP().toString().c_str());
                    break;
                } else {
                    Serial.println("unable");
                }
            }
        }
    }

    bool addNetwork(size_t index, const char *ssid, const char *pswd=nullptr) {
        if (index >= WIFI_CONFIG_N_NETWORKS) return false;
        nets.nets[index].setCreds(ssid, pswd);
        if (hasNVM) {
            return StorageNVM.put(nvmAddress, nets);
        }
        return true;
    }

private:
    // auto connect stuff
    unsigned long _lastConnectTime = 0;
public:
    bool _changedWiFiNets = false;
private:

    // scan stuff
    unsigned long _lastScanTime = 0;

    AsyncCallbackJsonWebHandler *_handlerWiFiConfig;
};


#endif