// matth-x/ArduinoOcpp
// Copyright Matthias Akstaller 2019 - 2022
// MIT License


#include <ArduinoOcpp/MessagesV16/RemoteStartTransaction.h>
#include <ArduinoOcpp/Core/OcppModel.h>
#include <ArduinoOcpp/Tasks/ChargePointStatus/ChargePointStatusService.h>
#include <Variants.h>

using ArduinoOcpp::Ocpp16::RemoteStartTransaction;

RemoteStartTransaction::RemoteStartTransaction() {
  
}

const char* RemoteStartTransaction::getOcppOperationType() {
    return "RemoteStartTransaction";
}

void RemoteStartTransaction::processReq(JsonObject payload) {
    connectorId = payload["connectorId"] | -1;

    if (payload.containsKey("idTag")) {
        String idTag = payload["idTag"] | String("Invalid");
        if (ocppModel && ocppModel->getChargePointStatusService()) { 
            ocppModel->getChargePointStatusService()->authorize(idTag);
        }
    }
}

std::unique_ptr<DynamicJsonDocument> RemoteStartTransaction::createConf(){
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(1)));
    JsonObject payload = doc->to<JsonObject>();
    
    bool canStartTransaction = false;
    if (connectorId >= 1) {
        //connectorId specified for given connector, try to start Transaction here
        if (ocppModel && ocppModel->getConnectorStatus(connectorId)){
            auto connector = ocppModel->getConnectorStatus(connectorId);
            if (connector->getTransactionId() < 0) {
                canStartTransaction = true;
            }
        }
    } else {
        //connectorId not specified. Find free connector
        if (ocppModel && ocppModel->getChargePointStatusService()) {
            auto cpStatusService = ocppModel->getChargePointStatusService();
            for (int i = 1; i < cpStatusService->getNumConnectors(); i++) {
                auto connIter = cpStatusService->getConnector(i);
                if (connIter->getTransactionId() < 0) {
                    canStartTransaction = true; 
                }
            }
        }
    }

    if (canStartTransaction){
        payload["status"] = "Accepted";
    } else {
        payload["status"] = "Rejected";
    }
    
    return doc;
}

std::unique_ptr<DynamicJsonDocument> RemoteStartTransaction::createReq() {
    auto doc = std::unique_ptr<DynamicJsonDocument>(new DynamicJsonDocument(JSON_OBJECT_SIZE(1)));
    JsonObject payload = doc->to<JsonObject>();

    payload["idTag"] = "A0-00-00-00";

    return doc;
}

void RemoteStartTransaction::processConf(JsonObject payload){
    String status = payload["status"] | "Invalid";

    if (status.equals("Accepted")) {
        if (DEBUG_OUT) Serial.print(F("[RemoteStartTransaction] Request has been accepted!\n"));
    } else {
        Serial.print(F("[RemoteStartTransaction] Request has been denied!"));
    }
}
