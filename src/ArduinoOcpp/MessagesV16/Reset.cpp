// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License

#include <ArduinoOcpp/MessagesV16/Reset.h>

using ArduinoOcpp::Ocpp16::Reset;

Reset::Reset() {
  
}

const char* Reset::getOcppOperationType(){
    return "Reset";
}

void Reset::processReq(JsonObject payload) {
    /*
     * Process the application data here. Note: you have to implement the device reset procedure in your client code. You have to set
     * a onSendConfListener in which you initiate a reset (e.g. calling ESP.reset() )
     */
    const char *type = payload["type"] | "Invalid";
    if (!strcmp(type, "Hard")){
        Serial.print(F("[Reset] Warning: received request to perform hard reset, but this implementation is only capable of soft reset!\n"));
    }
}

std::unique_ptr<DynamicJsonDocument> Reset::createConf(){
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(1)));
    JsonObject payload = doc->to<JsonObject>();
    payload["status"] = "Accepted";
    return doc;
}
